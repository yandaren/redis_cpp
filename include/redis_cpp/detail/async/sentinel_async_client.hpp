/**
 *
 * sentinel_async_client.hpp
 *
 * sentinel_async_client
 *
 * @author  :   yandaren1220@126.com
 * @date    :   2017-06-03
 */

#ifndef __ydk_rediscpp_detail_sentinel_async_client_hpp__
#define __ydk_rediscpp_detail_sentinel_async_client_hpp__

#include <redis_cpp/detail/config.hpp>
#include <redis_cpp/detail/async/standalone_async_client.hpp>
#include <redis_cpp/detail/sentinel/sentinel_client_pool.hpp>

namespace redis_cpp
{
namespace detail
{
class sentinel_async_client : 
    public standalone_async_client,
    public event_subscriber
{
protected:
    sentinel_client_pool_ptr    sentinel_client_pool_;
    std::string                 master_name_;

public:
    sentinel_async_client(const char* master_name, asio::io_service& io_service, const char* uri)
        : master_name_(master_name)
        , standalone_async_client(io_service, uri)
    {
    }

    ~sentinel_async_client()
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
            rds_log_warn("sentinel_async_client[%p] uri[%s] unsupport event[%d].",
                this, uri_uri.c_str(), e);

            break;
        }
        }
    }

};
}
}

#endif