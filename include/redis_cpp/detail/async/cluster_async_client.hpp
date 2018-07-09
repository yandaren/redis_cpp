/**
 *
 * cluster_async_client.hpp
 *
 * cluster_async_client
 *
 * @author  :   yandaren1220@126.com
 * @date    :   2017-05-03
 */

#ifndef __ydk_rediscpp_detail_cluster_async_client_hpp__
#define __ydk_rediscpp_detail_cluster_async_client_hpp__

#include <redis_cpp/detail/config.hpp>
#include <redis_cpp/detail/async/standalone_async_client.hpp>
#include <redis_cpp/detail/sync/redis_sync_operator.hpp>
#include <redis_cpp/detail/redis_cluster_slots.hpp>
#include <utility/asio_base/thread_pool.hpp>
#include <utility/str.hpp>

namespace redis_cpp
{
namespace detail
{
class cluster_async_client : 
    public base_async_client
{
protected:
    std::map<std::string, standalone_async_client*> async_client_map_;
    std::mutex                                      client_map_mtx_;
    utility::asio_base::thread_pool                 async_con_thread_pool_;
    utility::asio_base::thread_pool                 other_thread_pool_;
    channel_message_handler_map_type                subscribe_msg_handlers_;
    std::mutex                                      msg_handlers_mtx_;
    redis_cluster_slots*                            cluster_slots_;

public:
    /* 
     * @param uri - redis address list(seperate by ';'), 
     * like redis://foobared@127.0.0.1:7000;redis://foobared@127.0.0.1:7001;redis://foobared@127.0.0.1:7002;
     */
    cluster_async_client(const char* uri)
        : async_con_thread_pool_(2)
        , other_thread_pool_(2)
        , cluster_slots_(nullptr)
    {
        async_con_thread_pool_.start();
        other_thread_pool_.start();

        std::vector<std::string> uri_list;
        ydk::str::string_splits(uri, ";", uri_list);
        if (uri_list.empty()){
            rds_log_error("cluster_async_client[%p] redis_uri[%s] parse failed.",
                this, uri);
            return;
        }

        redis_uri r_uri(uri_list[0].c_str());
        std::string redis_passwd = r_uri.get_passwd();

        cluster_slots_ = new redis_cluster_slots(other_thread_pool_.io_service());
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
            std::bind(&cluster_async_client::process_cluster_slot_change,
            this,
            std::placeholders::_1));
        cluster_slots_->start_timer();
    }

    ~cluster_async_client(){
        async_con_thread_pool_.stop();
        other_thread_pool_.stop();

        if (cluster_slots_){
            delete cluster_slots_;
            cluster_slots_ = nullptr;
        }

        async_con_thread_pool_.wait_for_stop();
        other_thread_pool_.wait_for_stop();
    }

public:
    /** implements of interface of base_async_client */
    /** interfaces */

    /** try connect to server */
    virtual void connect() override{
        std::lock_guard<std::mutex> locker(client_map_mtx_);

        for (auto& c_kv : async_client_map_){
            c_kv.second->try_connect();
        }
    }

    /**
    * @brief subscribe specific channel with specific message handler
    */
    virtual void subscribe(const std::string& channel_name,
        const channel_message_handler& message_handler,
        const reply_handler& reply_handler) override{
        // record the channel msg handlers first, whether the connection is
        // connected or not
        {
            std::lock_guard<std::mutex> locker(msg_handlers_mtx_);
            subscribe_msg_handlers_.insert(
                std::make_pair(channel_name,
                channel_message_handler_ptr(new channel_message_handler(message_handler))));
        }
        standalone_async_client* client = get_async_client(channel_name);
        if (!client){
            rds_log_error("cluster_async_client[%p] can't find available client, subscribe channel[%s] failed.",
                this, channel_name.c_str());
            return;
        }

        client->subscribe(channel_name, message_handler, reply_handler);
    }

    /**
    * @brief unsubscribe specific channel
    */
    virtual void unsubscribe(const std::string& channel_name,
        const reply_handler& reply_handler) override{
        // remove the channel msg handler first, whether the connection is 
        // connected or not
        {
            std::lock_guard<std::mutex> locker(msg_handlers_mtx_);
            subscribe_msg_handlers_.erase(channel_name);
        }

        standalone_async_client* client = get_async_client(channel_name);
        if (!client){
            rds_log_error("cluster_async_client[%p] can't find available client, usubscribe channel[%s] failed.",
                this, channel_name.c_str());
            return;
        }

        client->unsubscribe(channel_name, reply_handler);
    }

    /**
    * @brief publish message to specific channel
    */
    virtual void publish(const std::string& channel_name,
        const std::string& message,
        const reply_handler& reply_handler) override{
        // find a client to to the publish
        standalone_async_client* client = get_async_client(channel_name);
        if (!client){
            rds_log_error("cluster_async_client[%p] can't find available client, publish msg to channel[%s] failed.",
                this, channel_name.c_str());
            return;
        }

        client->publish(channel_name, message, reply_handler);
    }

    /**
    * @brief get client by channel name
    */
    standalone_async_client*   get_async_client(const std::string& channel_name){
        int32_t slot_id = redis_slot::slot(channel_name.c_str(), channel_name.size());

        std::lock_guard<std::mutex> locker(client_map_mtx_);
        return get_client_by_slot(slot_id);
    }

protected:
    standalone_async_client* get_client_by_address(const std::string& address){
        auto iter = async_client_map_.find(address);
        if (iter != async_client_map_.end()){
            return iter->second;
        }

        return nullptr;
    }

    /** 
     * @brief get client by slot
     */
    standalone_async_client* get_client_by_slot(int32_t slot){
        std::string* address = cluster_slots_->get_address_by_slot(slot);
        if (!address)
            return nullptr;

        return get_client_by_address(*address);
    }

    /**
    * @brief add async_client by uri
    */
    standalone_async_client* add_async_client(const std::string& uri){

        standalone_async_client* client =
            new standalone_async_client(async_con_thread_pool_.io_service(), uri.c_str());
        redis_uri r_uri(uri.c_str());
        std::stringstream ss;
        ss << r_uri.get_ip() << ":" << r_uri.get_port();
        std::string address(std::move(ss.str()));
        async_client_map_[address] = client;

        rds_log_info("cluster_async_client[%p] add async_client of uri[%s], cur client count[%d].",
            this, uri.c_str(), async_client_map_.size());

        return client;
    }

    /**
    * @brief add pool by ip and port
    */
    standalone_async_client* add_async_client(const std::string& ip, int32_t port){
        redis_uri uri;
        uri.set_passwd(cluster_slots_->get_redis_passwd());
        uri.set_ip(ip.c_str());
        uri.set_port(port);
        std::string redis_uri_str = uri.to_string();
        return add_async_client(redis_uri_str);
    }

    /**
    * @brief initialize cluster connection pool
    */
    void    initialize(const slot_range_map_type& map){

        // rest 
        cluster_slots_->reset(map);

        // initialize the connection pools
        for (auto& node : map){
            add_async_client(node.second.master.ip, node.second.master.port);
        }
    }

    /** try recover subscribe channels */
    void    try_resubscribe_channels(){
        channel_message_handler_map_type subcribe_list = subscribe_msg_handlers_;
        subscribe_msg_handlers_.clear();

        // resubscribe the subscribe
        for (auto iter = subcribe_list.begin();
            iter != subcribe_list.end(); ++iter){
            subscribe(iter->first,
                *iter->second,
                std::bind(
                &cluster_async_client::default_reply_handler,
                this,
                std::placeholders::_1));
        }
    }

    void   reset_client_list(const slot_range_map_type& map){
        // remove the old client
        {
            std::lock_guard<std::mutex> locker(client_map_mtx_);

            for (auto& c_kv : async_client_map_){
                c_kv.second->shutdown();
                delete c_kv.second;
            }

            async_client_map_.clear();

            rds_log_info("[%p] reset_client_list, cur_list_size[%d]",
                this, async_client_map_.size());

            initialize(map);
        }

        // try connect
        connect();

        // resubscribe the old channels
        try_resubscribe_channels();
    }

    void    process_cluster_slot_change(const slot_range_map_type& map){
        rds_log_info("[%p] detectived the slot range changed, try reset the async client list.", this);
        reset_client_list(map);
    }

    /** default reply handler */
    void default_reply_handler(redis_reply* reply){

    }
};
}
}

#endif