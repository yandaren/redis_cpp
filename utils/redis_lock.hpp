/**
 *
 * redis_lock.hpp
 *
 * the distribute lock implement by redis
 *
 * @author  :   yandaren1220@126.com
 * @date    :   2018-12-28
 */

#ifndef __ydk_rediscpp_redis_utils_redis_lock_hpp
#define __ydk_rediscpp_redis_utils_redis_lock_hpp

#include <cstdint>
#include <string>
#include <chrono>
#include <thread>
#include <redis_cpp/detail/sync/redis_sync_operator.hpp>

namespace redis_cpp
{
namespace utils {

// redis distribute lock
class redis_lock
{
protected:
    enum {
        try_lock_interval = 100,        // in milliseconds
    };
protected:
    detail::redis_sync_operator*    m_redis_op;
    std::string                     m_lock_key;
    std::string                     m_lock_requestor_id;
    int32_t                         m_key_expired_time;
    int32_t                         m_locked_time_out;

public:
    /** 
     * @brief 
     * @param op        : redis_operator
     * @param lock_key  : the redis key
     * @param requestor_id : the lock requestor
     * @param key_expire_time : the key ttl in milliseconds
     * @param locked_time_out : try get the lock time out( in milliseconds)
     */
    redis_lock(detail::redis_sync_operator* op, 
        const std::string& lock_key, 
        const std::string& requestor_id, 
        int32_t key_expire_time, 
        int32_t locked_time_out)
        : m_redis_op(op)
        , m_lock_key(lock_key)
        , m_lock_requestor_id(requestor_id)
        , m_key_expired_time(key_expire_time)
        , m_locked_time_out(locked_time_out){
    }

    ~redis_lock() {
    }

    bool    lock() {
        int32_t timeout = m_locked_time_out;
        while (timeout >= 0) {

            // get lock
            if (m_redis_op->setnxpx(m_lock_key.c_str(), m_lock_requestor_id, m_key_expired_time)) {
                return true;
            }

            // sleep for a while
            std::this_thread::sleep_for(std::chrono::milliseconds(try_lock_interval));
            timeout -= try_lock_interval;
        }

        return false;
    }

    bool    unlock() {
        std::string del_lock_script = "if redis.call('get', KEYS[1]) == ARGV[1] then return redis.call('del', KEYS[1]) else return 0 end";

        std::vector<std::string> keys;
        std::vector<std::string> args;

        keys.push_back(m_lock_key);
        args.push_back(m_lock_requestor_id);
        auto reply  = m_redis_op->eval(del_lock_script.c_str(), keys, args);
        return reply && reply->is_integer() && reply->to_integer() == 1;
    }
};


// redis distribute auto lock
class redis_auto_lock
{
protected:
    bool        m_locked;   
    redis_lock  m_locker;
public:
    redis_auto_lock(detail::redis_sync_operator* op,
        const std::string& lock_key,
        const std::string& requestor_id,
        int32_t key_expire_time,
        int32_t locked_time_out)
            : m_locked(false)
            , m_locker(op, lock_key, requestor_id, key_expire_time, locked_time_out){

        m_locked = m_locker.lock();
    }

    ~redis_auto_lock() {
        if (m_locked) {
            m_locker.unlock();
            m_locked = false;
        }
    }

    bool    islocked() {
        return m_locked;
    }
};

}
}

#endif