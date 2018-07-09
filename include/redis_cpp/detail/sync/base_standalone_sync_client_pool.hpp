/**
 *
 * base_standalone_sync_client_pool.hpp
 *
 * the interface of standalone_sync_client_pool
 *
 * @author  :   yandaren1220@126.com
 * @date    :   2017-04-26
 */

#ifndef __ydk_rediscpp_detail_base_standalone_sync_client_pool_hpp__
#define __ydk_rediscpp_detail_base_standalone_sync_client_pool_hpp__

#include <redis_cpp/detail/config.hpp>


namespace redis_cpp
{
namespace detail
{
class standalone_sync_client;
class base_standalone_sync_client_pool
{
public:
    virtual ~base_standalone_sync_client_pool(){}

public:
    /** interfaces */

    /** reclaim the client to pool */
    virtual void reclaim_to_pool(standalone_sync_client* client) = 0;

    /** remove and free the client from pool */
    virtual void free_client(standalone_sync_client* client) = 0;
};
}
}

#endif