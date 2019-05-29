/**
 *
 * base_async_client.hpp
 *
 * the interface of aysnc_client
 *
 * @author  :   yandaren1220@126.com
 * @date    :   2017-04-29
 */

#ifndef __ydk_rediscpp_detail_base_async_client_hpp__
#define __ydk_rediscpp_detail_base_async_client_hpp__

#include <redis_cpp/detail/config.hpp>
#include <redis_cpp/redis_reply.hpp>
#include <functional>
#include <memory>
#include <unordered_map>

namespace redis_cpp
{
namespace detail
{
typedef std::function<void(redis_reply_ptr)> reply_handler;
typedef std::function<void(const std::string& channel_name, 
    const std::string& message)> channel_message_handler;
typedef std::shared_ptr<channel_message_handler> channel_message_handler_ptr;
typedef std::shared_ptr<reply_handler>  reply_handler_ptr;
typedef std::unordered_map<std::string, channel_message_handler_ptr>
    channel_message_handler_map_type;

enum client_state{
    client_state_normal,
    client_state_subscribed,
};

class base_async_client
{
public:
    virtual ~base_async_client(){}

public:
    /** interfaces */

    /** try connect to server */
    virtual void connect() = 0;

    /**
     * @brief subscribe specific channel with specific message handler
     */
    virtual void subscribe(const std::string& channel_name,
        const channel_message_handler& message_handler,
        const reply_handler& reply_handler) = 0;

    /** 
     * @brief unsubscribe specific channel
     */
    virtual void unsubscribe(const std::string& channel_name,
        const reply_handler& reply_handler) = 0;

    /** 
     * @brief publish message to specific channel
     */ 
    virtual void publish(const std::string& channel_name, 
        const std::string& message, 
        const reply_handler& reply_handler) = 0;
};
}
}

#endif