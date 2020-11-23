/**
 *
 * redis_set.hpp
 *
 * redis command of script
 *
 * @author  :   yandaren1220@126.com
 * @date    :   2018-12-28
 */
#ifndef __ydk_rediscpp_detail_redis_script_hpp__
#define __ydk_rediscpp_detail_redis_script_hpp__

#include <redis_cpp/detail/config.hpp>
#include <redis_cpp/detail/redis_command.hpp>
#include <redis_cpp/detail/sync/redis_command_executor.hpp>
#include <redis_cpp/internal/logger_handler.hpp>
#include <vector>

namespace redis_cpp
{
namespace detail
{

class redis_script : virtual public redis_command_executor
{
public:
    redis_script() {
    }

public:

   /** 
    * @brief check can dispatch the command
    */
    static bool check_can_run(const std::vector<std::string>& keys) {
        if (keys.empty()) {
            return false;
        }

        if (keys.size() > 1) {
            auto slot_id = redis_slot::slot(keys[0].c_str());
            for (int32_t i = 1; i < (int32_t)keys.size(); ++i) {
                auto next_slot = redis_slot::slot(keys[i].c_str());
                if (next_slot != slot_id) {
                    rds_log_error("redis_script eval failed, case keys have difference slots.");
                    return false;
                }
            }
        }

        return true;
    }

   /**
    * @brief exculate lua script
    * @param script: the lua script
    * @param keys:   the keys
    * @param args:   the args
    * @return redis_reply
    */
    redis_reply_ptr eval(const char* script, const std::vector<std::string>& keys, const std::vector<std::string>& args)
    {
        redis_command cmd("eval");

        cmd.add_param(script);
        cmd.add_param((uint32_t)keys.size());

        for (auto& param : keys) {
            cmd.add_param(param);
        }

        for (auto& param : args) {
            cmd.add_param(param);
        }

        if (!keys.empty()) {
            hash_slot(keys[0].c_str());
        }
        else {
            reset_hash_slot();
        }

        return do_command(cmd);
    }

    /** 
     * @brief script load command
     * a. load the script to the cache, but not excuate it
     * b. while the script load the cache, we can use command 'evalsha' to excuate the script.
     * c. the script will always exists in the cache util we do command 'script flush'.
     * d. we can also do command 'script exists sha1[sha1 ...]' to check the script exists in the cache 
     * @param script : the lua script
     * @return the script sha1 code
     */
    bool    script_load(const char* script, std::string& out_script_sha1) {
        redis_command cmd("script");
        cmd.add_param("load");
        cmd.add_param(script);

        reset_hash_slot();

        return get_string_result(cmd, out_script_sha1);
    }


    /** 
     * @brief check the script exist in the cache
     * @param sha1_list : the script sha1 code list
     * @param the exists( 0 or 1 ) array
     */
    bool    script_exists(const std::vector<std::string>& sha1_list, std::vector<bool>& exists_list) {
        redis_command cmd("script");
        cmd.add_param("exists");

        for (auto& sha1 : sha1_list) {
            cmd.add_param(sha1);
        }

        reset_hash_slot();

        redis_reply_arr arr;
        if (!get_array_reply(cmd, arr)) {
            return false;
        }

        exists_list.clear();
        for (int32_t i = 0; i < (int32_t)arr.size(); ++i) {
            int32_t exist = arr[i].to_integer_32();
            exists_list.push_back(!!exist);
        }

        return true;
    }

    /** 
     * @brief evalsha command
     * @param sha1:   the sha1 code generate by "script load"
     * @param keys:   the keys
     * @param args:   the args
     * @return redis_reply
     */
    redis_reply_ptr evalsha(const char* sha1, const std::vector<std::string>& keys, const std::vector<std::string>& args) {
        redis_command cmd("evalsha");

        cmd.add_param(sha1);
        cmd.add_param((uint32_t)keys.size());

        for (auto& param : keys) {
            cmd.add_param(param);
        }

        for (auto& param : args) {
            cmd.add_param(param);
        }

        if (!keys.empty()) {
            hash_slot(keys[0].c_str());
        }
        else {
            reset_hash_slot();
        }

        return do_command(cmd);
    }

    /** 
     * @brief kill the scripts that is running
     * the command take effects only no write operation has done by the script
     * it can be used to stop the scripts that takes a long time( eg. a dead loop case a bug)
     * when the commad "script kill" excuted, the script that is running will stoped, and the clients that blocked by the 'eval' 
     * command will return, and will received a error reply
     * @return always return 'OK'
     */
    bool    script_kill() {
        redis_command cmd("script");
        cmd.add_param("kill");

        reset_hash_slot();

        return check_status_ok(cmd);
    }

    /** 
     * @brief clean all the script from the cache
     * @return always return 'OK'
     */
    bool    script_flush() {
        redis_command cmd("script");
        cmd.add_param("flush");

        reset_hash_slot();

        return check_status_ok(cmd);
    }
};

}
}

#endif