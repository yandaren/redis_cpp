/**
 *
 * cluster_sync_client.hpp
 *
 * sync client while the redis server run cluster mode
 *
 * @author  :   yandaren1220@126.com
 * @date    :   2017-04-26
 */

#ifndef __ydk_rediscpp_detail_cluster_sync_client_hpp__
#define __ydk_rediscpp_detail_cluster_sync_client_hpp__

#include <redis_cpp/detail/config.hpp>
#include <redis_cpp/detail/sync/base_sync_client.hpp>
#include <redis_cpp/detail/sync/standalone_sync_client_pool.hpp>
#include <redis_cpp/detail/redis_cluster_slots.hpp>
#include <redis_cpp/internal/logger_handler.hpp>
#include <utility/asio_base/thread_pool.hpp>
#include <utility/asio_base/timer.hpp>
#include <utility/str.hpp>
#include <atomic>
#include <cstdint>
#include <map>
#include <unordered_map>

namespace redis_cpp
{
namespace detail
{

/** check delay free con pool interval in milliseconds */
static const int32_t delay_free_con_pool_interval = 5000;

/** free con pool delay time in seconds */
static const int32_t delay_free_con_pool_time = 5;

struct delay_free_con_pool_info{
    std::string                  address;
    standalone_sync_client_pool* pool;
    time_t                       start_time;
};

class cluster_sync_client : 
    public base_sync_client
{
protected:
    int32_t                         pool_init_size_;
    int32_t                         pool_max_size_;
    std::atomic_bool                stopped_;
    utility::asio_base::thread_pool thread_pool_;
    std::map<std::string, standalone_sync_client_pool*>
                                    client_pool_map_;
    std::mutex                      client_pool_mtx_;
    std::vector<delay_free_con_pool_info>   delay_free_pool_list_;
    int32_t                         random_pool_roll;
    utility::asio_base::timer::ptr  delay_free_con_pool_timer_;
    redis_cluster_slots*            cluster_slots_;

public:
    /** 
     * @brief 
     * @param uri - redis address list(seperate by ';'), 
     * like redis://foobared@127.0.0.1:7000;redis://foobared@127.0.0.1:7001;redis://foobared@127.0.0.1:7002;
     * @param pool_init_size - the initialize pool size of each address connection pool
     * @param pool_max_size - the max pool size of each address connection pool
     */
    cluster_sync_client(const char* uri,
        int32_t pool_init_size,
        int32_t pool_max_size)
        : pool_init_size_(pool_init_size)
        , pool_max_size_(pool_max_size)
        , random_pool_roll(0)
        , thread_pool_(1)
        , cluster_slots_(nullptr)
    {
        thread_pool_.start();
        stopped_ = false;

        std::vector<std::string> uri_list;
        ydk::str::string_splits(uri, ";", uri_list);
        if (uri_list.empty()){
            rds_log_error("cluster_sync_client[%p] redis_uri[%s] parse failed.",
                this, uri);
            return;
        }

        redis_uri r_uri(uri_list[0].c_str());
        std::string redis_passwd = r_uri.get_passwd();
        cluster_slots_ = new redis_cluster_slots(thread_pool_.io_service());
        cluster_slots_->set_uri_list(uri_list);
        cluster_slots_->set_redis_passwd(redis_passwd);

        slot_range_map_type map;
        if (cluster_slots_->get_cluster_slots_map(map)){
            initialize(map);
        }
        else{
            rds_log_error("cluster_sync_client[%p] redis_uri[%s] try get cluster slots failed, init failed.",
                this, uri);
        }
        // set slot change handler
        cluster_slots_->set_cluster_slots_change_handler(
            std::bind(&cluster_sync_client::process_cluster_slot_change,
            this, 
            std::placeholders::_1));
        cluster_slots_->start_timer();

        delay_free_con_pool_timer_ = utility::asio_base::timer::create(thread_pool_.io_service());
        delay_free_con_pool_timer_->register_handler(std::bind(
            &cluster_sync_client::delay_free_con_pool_timer_handler,
            this, 
            std::placeholders::_1,
            std::placeholders::_2));

        delay_free_con_pool_timer_->start(delay_free_con_pool_interval);
    }

    ~cluster_sync_client(){
        stop();

        thread_pool_.wait_for_stop();
    }

    void stop(){
        if (!stopped_.exchange(true)){

            delay_free_con_pool_timer_->cancel();

            thread_pool_.stop();

            // stop the connection pools 
            for (auto& pool_kv : client_pool_map_){
                delete pool_kv.second;
            }
            client_pool_map_.clear();

            // free the delay free pool
            for (auto& pool : delay_free_pool_list_){
                delete pool.pool;
            }
            delay_free_pool_list_.clear();

            if (cluster_slots_){
                delete cluster_slots_;
                cluster_slots_ = nullptr;
            }
        }
    }

public:
    /** implement of base_sync_client **/
    /** do command */
    virtual redis_reply* do_command(const redis_command& cmd, int32_t hash_slot) override
    {
        auto client = get_client_by_slot(hash_slot);

        if (!client){
            rds_log_error("cluster_client[%p] get client of slot[%d] failed. cmd failed",
                this, hash_slot);

            return nullptr;
        }

        int32_t times = 0;
        while (times++ < max_redirect_times){
            redis_reply* reply = client->do_command(cmd, hash_slot);
            if (reply){
                client->close();
            }
            else{
                client->free();
            }

            if (!reply){
                return nullptr;
            }

            if (!reply->is_error()){
                return reply;
            }

            error_reply& err = reply->to_error();
#define EQ(x, y) !strnicmp((x), (y), sizeof(y) -1)
            if (EQ(err.msg.c_str(), "moved")){
                int32_t redirect_slot = -10000;
                const char* address =
                    get_addr_from_redirect_error(err.msg, redirect_slot);
                if (!address){
                    rds_log_error("moved invalid, error_desc:%s", err.msg.c_str());
                    return reply;
                }

                std::string address_str(address);
                client = redirect_client(address_str);
                if (!client){
                    rds_log_error("redirect failed, address: %s", address);
                    return reply;
                }

                rds_log_info("redirect to slot[%d(%d)] located at %s.",
                    redirect_slot, hash_slot, address);
            }
            else if (EQ(err.msg.c_str(), "ask")){
                int32_t redirect_slot = -10000;
                const char* address =
                    get_addr_from_redirect_error(err.msg, redirect_slot);
                if (!address){
                    rds_log_error("ask invalid, error_desc:%s", err.msg.c_str());
                    return reply;
                }

                std::string address_str(address);
                client = redirect_client(address_str);
                if (!client){
                    rds_log_error("redirect failed, address: %s", address);
                    return reply;
                }

                // do ask
                redis_command cmd("asking");
                reply = client->do_command(cmd, hash_slot);
                if (!reply){
                    rds_log_error("asking's reply is null");
                    client->free();
                    return nullptr;
                }
                if (!reply->check_status_ok()){
                    client->close();
                    return nullptr;
                }

                rds_log_info("redirect to slot[%d(%d)] located at %s.",
                    redirect_slot, hash_slot, address);
            }
            else if (EQ(err.msg.c_str(), "clusterdown")){
                rds_log_error("cluster is down...");
                return reply;
            }
            else {
                return reply;
            }
        }

        rds_log_warn("too many redirect: %d, max: %d, for slot.",
            times, redis_slot::max_hash_slot, hash_slot);

        return nullptr;
    }

    /** is cluster mode */
    virtual bool         cluster_mode() override
    {
        return true;
    }

public:
   /**
    * @brief redirect client
    */
    standalone_sync_client* redirect_client(const std::string& address_with_port){
        std::lock_guard<std::mutex> locker(client_pool_mtx_);

        auto pool = redirect_pool(address_with_port);
        if (pool)
            return pool->get_client();
        return nullptr;
    }

   /**
    * @brief get client by slot
    */
    standalone_sync_client* get_client_by_slot(int32_t slot){
        std::lock_guard<std::mutex> locker(client_pool_mtx_);

        auto pool = get_pool_by_slot(slot);
        if (pool){
            return pool->get_client();
        }
        return nullptr;
    }

protected:
    /** 
     * @brief add pool by uri
     */
    standalone_sync_client_pool* add_pool(const std::string& uri){

        standalone_sync_client_pool* pool =
            new standalone_sync_client_pool(uri.c_str(), pool_init_size_, pool_max_size_);

        redis_uri r_uri(uri.c_str());
        std::stringstream ss;
        ss << r_uri.get_ip() << ":" << r_uri.get_port();
        std::string address(std::move(ss.str()));
        client_pool_map_[address] = pool;

        rds_log_info("cluster_client[%p] add pool of uri[%s], cur pool count[%d].",
            this, uri.c_str(), client_pool_map_.size());

        return pool;
    }

    /** 
     * @brief add pool by ip and port
     */
    standalone_sync_client_pool* add_pool(const std::string& ip, int32_t port){
        redis_uri uri;
        uri.set_passwd(cluster_slots_->get_redis_passwd());
        uri.set_ip(ip.c_str());
        uri.set_port(port);
        std::string redis_uri_str = uri.to_string();
        return add_pool(redis_uri_str);
    }

    /** 
     * @brief initialize cluster connection pool
     */
    void    initialize(const slot_range_map_type& map){

        // reset
        cluster_slots_->reset(map);

        // initialize the connection pools
        for (auto& node : map){
            add_pool(node.second.master.ip, node.second.master.port);
        }
    }

    /** 
     * @brief get random pool
     */
    standalone_sync_client_pool* random_pool(){
        if (client_pool_map_.empty())
            return nullptr;

        random_pool_roll = !random_pool_roll;
        if (random_pool_roll){
            for (auto iter = client_pool_map_.begin(); 
                iter != client_pool_map_.end(); ++iter){
                if (!iter->second->empty()){
                    return iter->second;
                }
            }
        }
        else{
            for (auto iter = client_pool_map_.rbegin();
                iter != client_pool_map_.rend(); ++iter){
                if (!iter->second->empty()){
                    return iter->second;
                }
            }
        }

        return nullptr;
    }

    /**
     * @brief get pool by address
     */
    standalone_sync_client_pool* get_pool_by_address(const std::string& address){
        auto iter = client_pool_map_.find(address);
        if (iter != client_pool_map_.end()){
            return iter->second;
        }

        return nullptr;
    }

    /** 
     * @brief get client_pool by slot
     */
    standalone_sync_client_pool* get_pool_by_slot(int32_t slot){

        if (slot < 0)
            return random_pool();

        std::string* address = cluster_slots_->get_address_by_slot(slot);
        if (!address){
            return nullptr;
        }

        return get_pool_by_address(*address);
    }

    /** 
     * @brief redirect client pool
     */
    standalone_sync_client_pool* redirect_pool(const std::string& address_with_port){
        return get_pool_by_address(address_with_port);
    }

private:
    /**
    * @brief 从错误信息里面取出重定向的ip地址
    */
    static const char* get_addr_from_redirect_error(const std::string& error_info, int32_t& slot)
    {
        std::size_t slot_pos = error_info.find(' ');
        if (slot_pos == std::string::npos)
            return nullptr;

        if (slot_pos + 1 < error_info.size() - 1)
            slot_pos++;

        std::size_t addr_pos = error_info.find(' ', slot_pos);
        if (addr_pos == std::string::npos)
            return nullptr;

        if (addr_pos + 1 < error_info.size() - 1)
            addr_pos++;

        std::string slot_str = error_info.substr(slot_pos, addr_pos - slot_pos - 1);
        slot = atoi(slot_str.c_str());

        return &error_info[addr_pos];
    }

private:
    void    reset_connection_pool(const slot_range_map_type& map){
        // remove the old pool
        std::lock_guard<std::mutex> locker(client_pool_mtx_);

        for (auto& pool : client_pool_map_){
            delay_free_con_pool_info info;
            info.address = pool.first;
            info.pool = pool.second;
            info.start_time = time(nullptr);
            delay_free_pool_list_.push_back(info);
        }

        client_pool_map_.clear();

        rds_log_info("[%p] reset_connection_pool, cur_pool[%d], delay free count[%d].",
            this, client_pool_map_.size(), delay_free_pool_list_.size());

        initialize(map);
    }

    void    process_cluster_slot_change(const slot_range_map_type& map){
        rds_log_info("[%p] detectived the slot range changed, try reset the cluster connection pool.", this);
        reset_connection_pool(map);
    }

    void    delay_free_con_pool(){
        std::lock_guard<std::mutex> locker(client_pool_mtx_);
        if (delay_free_pool_list_.empty()){
            return;
        }

        time_t now = time(nullptr);
        for (auto iter = delay_free_pool_list_.begin();
            iter != delay_free_pool_list_.end();){
            const delay_free_con_pool_info& info = *iter;
            int32_t time_elapse = (int32_t)(now - info.start_time);
            if (time_elapse >= delay_free_con_pool_time){
                std::string address = info.address;
                standalone_sync_client_pool* pool = info.pool;
                iter = delay_free_pool_list_.erase(iter);
                rds_log_info("delay free pool[%p] address[%s], remain delay list count[%d], current pool size[%d].",
                    this, address.c_str(), delay_free_pool_list_.size(), client_pool_map_.size());

                delete pool;
            }
            else iter++;
        }
    }

    void    delay_free_con_pool_timer_handler(utility::asio_base::timer::ptr timer_ptr,
        const asio::error_code& error){
        if (!error){
            delay_free_con_pool();

            timer_ptr->start(delay_free_con_pool_interval);
        }
        else{
            rds_log_error("cluster_sync_client[%p] delay_free_pool, timer error[%s:%d].",
                this, error.message().c_str(), error.value());
        }
    }
};

}
}

#endif