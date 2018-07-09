/**
 *
 * redis_cpp.hpp
 *
 *
 * @author  :   yandaren1220@126.com
 * @date    :   2017-04-24
 */

#ifndef __ydk_rediscpp_redis_cpp_hpp__
#define __ydk_rediscpp_redis_cpp_hpp__

#include <redis_cpp/detail/config.hpp>
#include <redis_cpp/internal/logger_handler.hpp>
#include <redis_cpp/detail/sentinel/sentinel_client_pool.hpp>
#include <redis_cpp/detail/sync/standalone_sync_client_pool.hpp>
#include <redis_cpp/detail/sync/cluster_sync_client.hpp>
#include <redis_cpp/detail/sync/sentinel_sync_client.hpp>
#include <redis_cpp/detail/sync/redis_sync_operator.hpp>
#include <redis_cpp/detail/async/standalone_async_client.hpp>
#include <redis_cpp/detail/async/cluster_async_client.hpp>
#include <redis_cpp/detail/async/sentinel_async_client.hpp>
#include <redis_cpp/detail/async/redis_async_operator.hpp>

namespace redis_cpp
{
    /**
    * @brief initialize the environment( mainy initialize the global logger handler)
    */
    static void initialize()
    {
        // initialize the static logger handler
        internal::logger_initialize();
    }

    /**
    * @brief set the custom log handler
    */
    static void set_log_handler(const internal::log_handler_func& func){
        internal::set_log_handler(func);
    }

    /**
    * @brief set the log level
    */
    static void set_log_lvl(internal::rds_log_level lvl){
        internal::set_log_lvl(lvl);
    }
}

#endif