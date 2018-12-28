/**
 *
 * redis_sync_operator.hpp
 *
 * the sync redis operator
 *
 * @author  :   yandaren1220@126.com
 * @date    :   2017-04-27
 */

#ifndef __ydk_rediscpp_detail_redis_sync_operator_hpp__
#define __ydk_rediscpp_detail_redis_sync_operator_hpp__

#include <redis_cpp/detail/config.hpp>
#include <redis_cpp/detail/sync/base_sync_client.hpp>
#include <redis_cpp/detail/sync/redis_key.hpp>
#include <redis_cpp/detail/sync/redis_string.hpp>
#include <redis_cpp/detail/sync/redis_list.hpp>
#include <redis_cpp/detail/sync/redis_hash.hpp>
#include <redis_cpp/detail/sync/redis_set.hpp>
#include <redis_cpp/detail/sync/redis_zset.hpp>
#include <redis_cpp/detail/sync/redis_pubsub.hpp>
#include <redis_cpp/detail/sync/redis_server.hpp>
#include <redis_cpp/detail/sync/redis_cluster.hpp>
#include <redis_cpp/detail/sync/redis_script.hpp>
#include <redis_cpp/detail/sync/redis_connection.hpp>

namespace redis_cpp
{
namespace detail
{
class redis_sync_operator :
    public redis_key,
    public redis_string,
    public redis_list,
    public redis_hash,
    public redis_set,
    public redis_zset,
    public redis_pubsub,
    public redis_server,
    public redis_cluster,
    public redis_script,
    public redis_connection
{
public:
    redis_sync_operator(base_sync_client* client){
        set_sync_client(client);
    }

    ~redis_sync_operator(){
    }
};
}
}

#endif