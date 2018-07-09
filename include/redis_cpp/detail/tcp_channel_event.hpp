/**
 *
 * tcp_channel_event.hpp
 *
 * tcp channel events(channel open, close, msg_receive and son on)
 *
 * @author  :   yandaren1220@126.com
 * @date    :   2017-04-22
 */

#ifndef __ydk_rediscpp_detial_tcp_channel__event_hpp__
#define __ydk_rediscpp_detial_tcp_channel__event_hpp__

#include <redis_cpp/detail/config.hpp>
#include <cstdint>
#include <string>
#include <system_error>

namespace redis_cpp
{
namespace detail
{
class tcp_channel_event
{
public:
    virtual ~tcp_channel_event(){}

public:
    /**
     * connection to remote address opened
     */
    virtual void channel_open(const char* ip, int32_t port) = 0;

    /* 
     * connection to remote address closed
     */
    virtual void channel_closed(const char* ip, int32_t port) = 0;
    
    /** 
     * connection to remote address exception
     */
    virtual void channel_exception(const char* ip, int32_t port, 
        const std::error_code& err) = 0;

    /** 
     * received msg from the connection
     */
    virtual void message_received(const char* ip, int32_t port, 
        const char* message, int32_t size) = 0;
};
}
}

#endif