/**
 *
 * sentinel_sync_client.hpp
 *
 * sentinel_sync_client
 *
 * @author  :   yandaren1220@126.com
 * @date    :   2017-06-03
 */

#ifndef __ydk_rediscpp_detail_sentinel_sync_client_hpp__
#define __ydk_rediscpp_detail_sentinel_sync_client_hpp__

#include <redis_cpp/detail/config.hpp>
#include <redis_cpp/detail/sentinel/sentinel_client_pool.hpp>
#include <redis_cpp/detail/sync/standalone_sync_client_pool.hpp>

namespace redis_cpp
{
namespace detail
{

class sentinel_sync_client :
    public standalone_sync_client_pool,
    public event_subscriber
{
protected:
    sentinel_client_pool_ptr    sentinel_client_pool_;
    std::string                 master_name_;
public:
    sentinel_sync_client(
        const char* master_name,
        const char* uri,
        int32_t pool_init_size,
        int32_t pool_max_size,
        utility::asio_base::thread_pool* thread_pool = nullptr)
        : master_name_(master_name)
        , standalone_sync_client_pool(uri, pool_init_size, pool_max_size, thread_pool)
    {
    }

    ~sentinel_sync_client()
    {
        set_sentinel_client_pool(nullptr);
    }

public:
    void    set_sentinel_client_pool(sentinel_client_pool_ptr sentinel)
    {
        if (sentinel_client_pool_){
            sentinel_client_pool_->remove_event_subscriber(event_master_address_change, this);
        }

        sentinel_client_pool_ = sentinel;

        if (sentinel_client_pool_){
            sentinel_client_pool_->add_event_subscriber(event_master_address_change, this);
        }
    }

    sentinel_client_pool_ptr get_sentinel_client_pool()
    {
        return sentinel_client_pool_;
    }

public:
    /** implements of event_subscriber */
    virtual void on_event(event_num e, void* content) override
    {
        switch (e)
        {
        case redis_cpp::detail::event_master_address_change:
        {
            master_address_change_event_t* event = (master_address_change_event_t*)content;

            if (event->master_name != master_name_)
                return;

            reset_uri_string(event->new_ip, event->new_port);

            break;
        }
        default:
        {
            std::string uri_uri = uri_string();
            rds_log_warn("sentinel_sync_client[%p] uri[%s] unsupport event[%d].",
                this, uri_uri.c_str(), e);

            break;
        }
        }
    }

protected:
    /**
    * @brief check_client_available_one_time override
    */
    virtual bool check_client_available_one_time(standalone_sync_client* client)
    {
        bool f = standalone_sync_client_pool::check_client_available_one_time(client);
        if (!f)
            return f;

        // check master address changed
        redis_uri pool_uri = get_uri();
        if (client->check_address_change(pool_uri))
        {
            rds_log_error("standalone_sync_client[%p] cur_uri[%s] not fit cur pool[%p] uri[%s].",
                client, client->get_uri_string().c_str(), this, pool_uri.to_string().c_str());
            return false;
        }

        return true;
    }
};

}
}

#endif