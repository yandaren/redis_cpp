/**
 *
 * redis_reply_util.hpp
 *
 * provide the specific reply parser util
 *
 * @author  :   yandaren1220@126.com
 * @date    :   2017-05-01
 */

#ifndef __ydk_rediscpp_detail_redis_reply_util_hpp__
#define __ydk_rediscpp_detail_redis_reply_util_hpp__

#include <redis_cpp/detail/config.hpp>
#include <redis_cpp/detail/redis_slot_range.hpp>
#include <redis_cpp/redis_reply.hpp>

namespace redis_cpp
{
namespace detail
{
namespace reply_util
{
    /** parser cluster slots */
    static bool parser_cluster_slots(redis_reply* reply, 
        slot_range_map_type& map){

        if (!reply || !reply->is_array()){
            return false;
        }

        redis_reply_arr& arr = reply->to_array();
        for (auto& node : arr){
            if (!node.is_array()){
                return false;
            }

            redis_reply_arr& node_detail = node.to_array();
            if (node_detail.size() < 3){
                return false;
            }

            if (!node_detail[0].is_integer() || 
                !node_detail[1].is_integer() ||
                !node_detail[2].is_array()){
                return false;
            }

            slot_range_info rang_info;
            rang_info.start_slot = node_detail[0].to_integer();
            rang_info.end_slot = node_detail[1].to_integer();
            redis_reply_arr& master_detail = node_detail[2].to_array();
            if (master_detail.size() < 2 ||
                !master_detail[0].is_string() ||
                !master_detail[1].is_integer() ){
                return false;
            }

            rang_info.master.ip = std::move(master_detail[0].to_string());
            rang_info.master.port = master_detail[1].to_integer();

            for (std::size_t i = 3; i < node_detail.size(); ++i){
                if (!node_detail[i].is_array()){
                    return false;
                }

                redis_reply_arr& slave_detail = node_detail[i].to_array();
                if (slave_detail.size() < 2 ||
                    !slave_detail[0].is_string() ||
                    !slave_detail[1].is_integer()){
                    return false;
                }

                rang_info.slave_list.resize(rang_info.slave_list.size() + 1);
                auto& slave_address = rang_info.slave_list.back();
                slave_address.ip = std::move(slave_detail[0].to_string());
                slave_address.port = slave_detail[1].to_integer();
            }

            map.insert(std::make_pair(
                slot_range_key(rang_info.start_slot, rang_info.end_slot), rang_info));
        }

        return true;
    }
}
}
}

#endif