/**
 *
 * sentinel_client_pool.hpp
 *
 * sentinel_client_pool
 *
 * @author  :   yandaren1220@126.com
 * @date    :   2017-06-03
 */

#ifndef __ydk_rediscpp_detail_sentinel_client_pool_hpp__
#define __ydk_rediscpp_detail_sentinel_client_pool_hpp__

#include <redis_cpp/detail/config.hpp>
#include <redis_cpp/detail/sentinel/base_sentinel_client.hpp>
#include <redis_cpp/detail/sentinel/sentinel_client.hpp>
#include <utility/asio_base/thread_pool.hpp>
#include <utility/str.hpp>

namespace redis_cpp
{
namespace detail
{
class sentinel_client_pool;
typedef std::shared_ptr<sentinel_client_pool> sentinel_client_pool_ptr;
typedef std::weak_ptr<sentinel_client_pool>   sentinel_client_pool_weak_ptr;

class sentinel_client_pool : 
    public base_sentinel_client
{
protected:
    utility::asio_base::thread_pool thread_pool_;
    std::vector<sentinel_client*>   subscribe_client_list_;
    std::vector<sentinel_client*>   non_subscribe_client_list_;

public:
    /*
     * @param sentinel_srv_list - redis sentinel srv list(seperate by ';'),
     * like 10.0.1.1:26379;10.0.1.2:26379;10.0.1.3:26379
     */
    sentinel_client_pool(const char* sentinel_srv_list)
        :thread_pool_(2){
        thread_pool_.start();

        std::vector<std::pair<std::string, int32_t>> srv_list;
        std::vector<std::string> splits;
        utility::str::string_splits(sentinel_srv_list, ";", splits);
        if (splits.empty()){
            rds_log_error("sentinel_client_pool[%p], sentinel_srv_list[%s] invalid.",
                this, sentinel_srv_list);
        }

        for (auto& split : splits)
        {
            std::vector<std::string> spts;
            utility::str::string_splits(split.c_str(), ":", spts);
            if (spts.size() != 2){
                rds_log_error("sentinel_client_pool[%p], srv_url[%s] invalid, sentinel_srv_list[%s].",
                    this, split.c_str(), sentinel_srv_list);
                continue;
            }

            std::pair<std::string, int32_t> srv_info;
            srv_info.first = spts[0];
            int32_t port = 0;
            sscanf(spts[1].c_str(), "%d", &port);
            srv_info.second = port;

            srv_list.push_back(srv_info);
        }

        initialize(srv_list);
    }

    ~sentinel_client_pool(){
        thread_pool_.stop();

        for (auto client : subscribe_client_list_)
        {
            delete client;
        }

        for (auto client : non_subscribe_client_list_)
        {
            delete client;
        }

        thread_pool_.wait_for_stop();
    }

public:
    static sentinel_client_pool_ptr create(const char* sentinel_srv_list)
    {
        return std::make_shared<sentinel_client_pool>(sentinel_srv_list);
    }

public:
    /** implements of base_sentinel_client */
   /**
    * @brief get master address by master name
    */
    virtual bool    get_master_address_by_name(const char* master_name,
        std::pair<std::string, int32_t>& out_master_address) override
    {
        for (auto client : non_subscribe_client_list_)
        {
            if (client->is_connected() && 
                client->get_master_address_by_name(master_name, out_master_address))
            {
                return true;
            }
        }

        return false;
    }

    /**
    * @brief start
    */
    virtual void    start() override
    {
        for (auto client : subscribe_client_list_)
        {
            client->start();
        }

        for (auto client : non_subscribe_client_list_)
        {
            client->start();
        }
    }

    /** implements of event_publisher */
    /**
    * @brief add event subscriber
    */
    virtual void    add_event_subscriber(event_num e, event_subscriber* sub) override
    {
        event_publisher::add_event_subscriber(e, sub);

        for (auto client : subscribe_client_list_)
        {
            client->add_event_subscriber(e, sub);
        }
    }

    /**
    * @brief remove event subscriber
    */
    virtual void    remove_event_subscriber(event_num e, event_subscriber* sub) override
    {
        event_publisher::remove_event_subscriber(e, sub);

        for (auto client : subscribe_client_list_)
        {
            client->remove_event_subscriber(e, sub);
        }
    }

protected:
    /** initialize the client list */
    void    initialize(const std::vector<std::pair<std::string, int32_t>>& srv_list){
        for (std::size_t i = 0; i < srv_list.size(); ++i){
            redis_uri uri;
            uri.set_ip(srv_list[i].first.c_str());
            uri.set_port(srv_list[i].second);

            std::string uri_str = uri.to_string();
            sentinel_client* client = new sentinel_client(thread_pool_.io_service(), uri_str.c_str());
            subscribe_client_list_.push_back(client);

            rds_log_info("sentinel_client_pool[%p] add subscribe sentinel client[%p] uri[%s] to client pool, current subscribe size[%d].",
                this, client, uri_str.c_str(), subscribe_client_list_.size());

            client = new sentinel_client(thread_pool_.io_service(), uri_str.c_str(), false);
            non_subscribe_client_list_.push_back(client);

            rds_log_info("sentinel_client_pool[%p] add non subscribe sentinel client[%p] uri[%s] to client pool, current subscribe size[%d].",
                this, client, uri_str.c_str(), non_subscribe_client_list_.size());
        }

        rds_log_info("sentinel_client_pool[%p] initialize finished, subscribe_client_size[%d], non_subscribe_size[%d]",
            this, subscribe_client_list_.size(), non_subscribe_client_list_.size());
    }
};

}
}

#endif