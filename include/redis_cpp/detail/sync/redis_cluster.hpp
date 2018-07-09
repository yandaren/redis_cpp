/**
 *
 * redis_cluster.hpp
 *
 * redis command of cluster
 *
 * @author  :   yandaren1220@126.com
 * @date    :   2017-05-01
 */

#ifndef __ydk_rediscpp_detail_redis_cluster_hpp__
#define __ydk_rediscpp_detail_redis_cluster_hpp__

#include <redis_cpp/detail/config.hpp>
#include <redis_cpp/detail/sync/redis_command_executor.hpp>
#include <redis_cpp/detail/redis_slot_range.hpp>
#include <redis_cpp/detail/redis_reply_util.hpp>

namespace redis_cpp
{
namespace detail
{
class redis_cluster : virtual public redis_command_executor
{
public:
    redis_cluster(){
    }

public:
    /** 
     * @brief get the cluster slots range map
     */
    bool    cluster_slots(slot_range_map_type& map){
        redis_command cmd("cluster");
        cmd.add_param("slots");

        redis_reply* reply = do_command(cmd);
        return reply_util::parser_cluster_slots(reply, map);
    }
};
}
}

#endif