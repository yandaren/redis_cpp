/**
 *
 * redis_slot_range.hpp
 *
 * @author  :   yandaren1220@126.com
 * @date    :   2017-05-01
 */

#ifndef __ydk_rediscpp_detail_redis_slot_range_hpp__
#define __ydk_rediscpp_detail_redis_slot_range_hpp__

#include <redis_cpp/detail/config.hpp>
#include <string>
#include <cstdint>
#include <vector>
#include <map>

namespace redis_cpp
{
namespace detail
{
    struct node_address{
        node_address()
            : port(0){
        }

        bool operator==(const node_address& that) const{
            return port == that.port && ip == that.ip;
        }

        bool operator!=(const node_address& that) const{
            return !(*this == that);
        }

        std::string ip;
        int32_t     port;
    };

    struct slot_range_info{
        int32_t                     start_slot;
        int32_t                     end_slot;
        node_address                master;
        std::vector<node_address>   slave_list;
    };

    struct slot_range_key{
        slot_range_key()
            : start_slot(0)
            , end_slot(0){
        }

        slot_range_key(int32_t start, int32_t end)
            : start_slot(start)
            , end_slot(end){
        }

        bool operator < (const slot_range_key& that) const{
            if (start_slot != that.start_slot){
                return start_slot < that.start_slot;
            }
            return end_slot < that.end_slot;
        }

        int32_t         start_slot;
        int32_t         end_slot;
    };

    typedef std::map<slot_range_key, slot_range_info> slot_range_map_type;
}
}

#endif