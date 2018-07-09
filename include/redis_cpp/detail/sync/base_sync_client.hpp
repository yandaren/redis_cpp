/**
 *
 * base sync client.hpp
 *
 * the interface of sync client
 *
 * @author  :   yandaren1220@126.com
 * @date    :   2017-04-26
 */

#ifndef __ydk_rediscpp_detail_base_sync_client_hpp__
#define __ydk_rediscpp_detail_base_sync_client_hpp__

#include <redis_cpp/detail/config.hpp>
#include <redis_cpp/detail/redis_command.hpp>
#include <redis_cpp/redis_reply.hpp>

namespace redis_cpp
{
namespace detail
{
class base_standalone_sync_client_pool;
class base_sync_client
{
public:
    virtual ~base_sync_client(){}

public:
    /** interface **/
    /** do command */
    virtual redis_reply* do_command(const redis_command& cmd, int32_t hash_slot) = 0;
    
    /** is cluster mode */
    virtual bool         cluster_mode() = 0;
};
}
}

#endif