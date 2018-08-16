/**
 *
 * redis_slot.hpp
 *
 * the key hash to slot
 *
 * @author  :   yandaren1220@126.com
 * @date    :   2017-05-03
 */

#ifndef __ydk_rediscpp_detail_redis_slot_hpp__
#define __ydk_rediscpp_detail_redis_slot_hpp__

#include <utility/codec/crc16.hpp>

namespace redis_cpp
{
namespace detail
{
namespace redis_slot
{
    /** max hash slot */
    static int32_t max_hash_slot = 16384;

    /** get hash slot by key */
    static uint16_t slot(const char* key, int32_t len){
        return utility::codec::crc16(key, len) % max_hash_slot;
    }

    static uint16_t slot(const char* key){
        return slot(key, (int32_t)strlen(key));
    }
}
}

}

#endif