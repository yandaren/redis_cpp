/**
 *
 * redis_cluster_slots.hpp
 *
 * the redis_cluster_slots
 *
 * @author  :   yandaren1220@126.com
 * @date    :   2017-05-03
 */

#ifndef __ydk_rediscpp_detail_redis_cluster_slots_hpp__
#define __ydk_rediscpp_detail_redis_cluster_slots_hpp__

#include <redis_cpp/detail/config.hpp>
#include <redis_cpp/detail/sync/standalone_sync_client.hpp>
#include <redis_cpp/detail/sync/redis_sync_operator.hpp>
#include <redis_cpp/detail/redis_slot.hpp>
#include <utility/asio_base/timer.hpp>
#include <vector>
#include <unordered_map>

namespace redis_cpp
{
namespace detail
{
/** check cluster slots interval in milliseconds */
static const int32_t check_cluster_slot_interval = 5000;

typedef std::function<void(const slot_range_map_type&)> cluster_slots_change_handler;

class redis_cluster_slots
{
protected:
    asio::io_service&                           io_service_;
    slot_range_map_type                         slot_range_address_map_;
    std::unordered_map <int32_t, std::string>   slot_addr_map_;
    std::vector<std::string>                    uri_list_;
    std::string                                 redis_passwd_;
    std::mutex                                  slot_mtx_;
    utility::asio_base::timer::ptr              check_cluster_slots_timer_;
    cluster_slots_change_handler*               cluster_slots_change_handler_;
    std::atomic_bool                            started_timer_;

public:
    redis_cluster_slots(asio::io_service& service)
        : io_service_(service)
        , cluster_slots_change_handler_(nullptr){
        started_timer_ = false;
        check_cluster_slots_timer_ = utility::asio_base::timer::create(io_service_);
        check_cluster_slots_timer_->register_handler(
            std::bind(&redis_cluster_slots::check_cluster_slots_timer_handler,
                        this,
                        std::placeholders::_1,
                        std::placeholders::_2));
    }

    ~redis_cluster_slots(){
        check_cluster_slots_timer_->cancel();
        if (cluster_slots_change_handler_){
            delete cluster_slots_change_handler_;
            cluster_slots_change_handler_ = nullptr;
        }
    }

    void    start_timer(){
        if (!started_timer_.exchange(true)){
            check_cluster_slots_timer_->start(check_cluster_slot_interval);
        }
    }

    void    set_cluster_slots_change_handler(
        const cluster_slots_change_handler& handler){
        if (cluster_slots_change_handler_){
            *cluster_slots_change_handler_ = handler;
        }
        else{
            cluster_slots_change_handler_ =
                new cluster_slots_change_handler(handler);
        }
    }

    /** 
     * @brief set redis passwd
     */
    void    set_redis_passwd(const std::string& passwd){
        redis_passwd_ = passwd;
    }

    const std::string& get_redis_passwd(){
        return redis_passwd_;
    }

    void    set_uri_list(std::vector<std::string>& uri_list){
        uri_list_ = uri_list;
    }

    /**
    * @brief try get cluster slots
    */
    bool    get_cluster_slots_map(slot_range_map_type& map){
        std::vector<std::string> uri_list;
        {
            std::lock_guard<std::mutex> locker(slot_mtx_);
            uri_list = uri_list_;
        }
        bool success = false;
        for (auto& uri : uri_list){
            standalone_sync_client* client =
                new standalone_sync_client(io_service_, uri.c_str());

            if (client->connect()){
                redis_sync_operator redis_op(client);
                if (redis_op.cluster_slots(map)){
                    success = true;
                }
            }

            client->destroy();
            if (success){
                break;
            }
        }

        return success;
    }

    /** @brief reset */
    void    reset(const slot_range_map_type& map){
        std::lock_guard<std::mutex> locker(slot_mtx_);
        reset_uri_list(map);
        set_slot_addr_map(map);
    }

    /**
    * @brief get address by slot
    */
    std::string* get_address_by_slot(int32_t slot){
        std::lock_guard<std::mutex> locker(slot_mtx_);

        auto iter = slot_addr_map_.find(slot);
        if (iter != slot_addr_map_.end()){
            return &iter->second;
        }
        return nullptr;
    }

protected:
    /**
    * @brief reset uri list
    */
    void    reset_uri_list(const slot_range_map_type& map){
        uri_list_.clear();

        std::stringstream ss;
        for (auto& node : map){
            redis_uri uri;
            uri.set_passwd(redis_passwd_);
            uri.set_ip(node.second.master.ip.c_str());
            uri.set_port(node.second.master.port);
            std::string uri_string = std::move(uri.to_string());
            uri_list_.push_back(uri_string);
            ss << uri_string << ";";
        }

        std::string uri_list_str(std::move(ss.str()));
        rds_log_info("redis_cluster_slots[%p] reset uri list to[%s], list size[%d].",
            this, uri_list_str.c_str(), uri_list_.size());
    }

    /**
    * @brief set slot address map
    */
    void    set_slot_addr_map(int32_t slot, const std::string& address){
        if (slot < 0 || slot >= redis_slot::max_hash_slot)
            return;

        slot_addr_map_[slot] = address;
    }

    /**
    * @brief set slot_addr_map
    */
    void    set_slot_addr_map(const slot_range_map_type& map){
        slot_range_address_map_ = map;
        for (auto& kv : map){
            int32_t start_slot = kv.first.start_slot;
            int32_t end_slot = kv.first.end_slot;

            if (start_slot < 0 || end_slot >= redis_slot::max_hash_slot || end_slot < start_slot){
                rds_log_error("redis_cluster_slots[%p] set_slot_addr_map, but the start_slot[%d] end_slot[%d] illegal, max[%d].",
                    this, start_slot, end_slot, redis_slot::max_hash_slot);
                return;
            }

            std::stringstream ss;
            ss << kv.second.master.ip << ":" << kv.second.master.port;
            std::string address(std::move(ss.str()));

            for (int32_t i = start_slot; i <= end_slot; ++i){
                set_slot_addr_map(i, address);
            }

            rds_log_info("redis_cluster_slots[%p] slot_range[%d~%d], address[%s].",
                this, start_slot, end_slot, address.c_str());
        }

        rds_log_info("redis_cluster_slots[%p] slot_address_map_size[%d].",
            this, slot_addr_map_.size());
    }

    static bool slot_range_info_eq(const slot_range_info& s1,
        const slot_range_info& s2){
        if (s1.start_slot != s2.start_slot)
            return false;
        if (s1.end_slot != s2.end_slot)
            return false;
        if (s1.master != s2.master)
            return false;
        return true;
    }

    bool    check_cluster_node_change(slot_range_map_type& map){
        std::lock_guard<std::mutex> locker(slot_mtx_);

        if (map.size() != slot_range_address_map_.size())
            return true;

        for (auto& node : map){
            auto iter = slot_range_address_map_.find(node.first);
            if (iter == slot_range_address_map_.end()){
                return true;
            }

            if (!slot_range_info_eq(node.second, iter->second))
                return true;
        }
        return false;
    }

    bool    cluster_slots_change(slot_range_map_type& map){
        if (!get_cluster_slots_map(map)){
            rds_log_error("redis_cluster_slots[%p] try get cluster slots map failed.",
                this);
            return false;
        }

        return check_cluster_node_change(map);
    }

    void    check_cluster_slots_timer_handler(utility::asio_base::timer::ptr timer_ptr,
        const asio::error_code& error){
        if (!error){
            slot_range_map_type map;
            if (cluster_slots_change(map)){
                if ( cluster_slots_change_handler_)
                    (*cluster_slots_change_handler_)(map);
            }

            timer_ptr->start(check_cluster_slot_interval);
        }
        else{
            rds_log_error("redis_cluster_slots[%p] check_cluster_slot, timer error[%s:%d].",
                this, error.message().c_str(), error.value());
        }
    }
};
}
}

#endif