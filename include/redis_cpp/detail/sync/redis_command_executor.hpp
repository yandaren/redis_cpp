/**
 *
 * redis_command_executor.hpp
 *
 * the command executor
 *
 * @author  :   yandaren1220@126.com
 * @date    :   2017-04-26
 */

#ifndef __ydk_rediscpp_detail_redis_command_executor_hpp__
#define __ydk_rediscpp_detail_redis_command_executor_hpp__

#include <redis_cpp/detail/config.hpp>
#include <redis_cpp/detail/async/base_async_client.hpp>
#include <redis_cpp/detail/redis_slot.hpp>
#include <vector>

namespace redis_cpp
{
namespace detail
{

/** max redirect times */
static int32_t max_redirect_times = 15;

class redis_command_executor
{
protected:
    base_sync_client*   sync_client_;
    int32_t             hash_slot_;
public:
    redis_command_executor()
        : sync_client_(nullptr)
        , hash_slot_(-1)
    {
    }

    virtual ~redis_command_executor(){

    }

public:
    void    set_sync_client(base_sync_client* client){
        sync_client_ = client;
    }

    base_sync_client* get_sync_client(){
        return sync_client_;
    }

    /** execute cmd */
    redis_reply_ptr    do_command(const redis_command& cmd){
        if (!sync_client_){
            return nullptr;
        }

        return sync_client_->do_command(cmd, hash_slot_);
    }

public:
    /** slot related */
    /** record the hash slot of this request */
    void    hash_slot(const char* key){
        if (sync_client_ && sync_client_->cluster_mode()){
            hash_slot_ = redis_slot::slot(key);
        }
    }

    void    hash_slot(const char* key, int32_t len){
        if (sync_client_ && sync_client_->cluster_mode()){
            hash_slot_ = redis_slot::slot(key, len);
        }
    }

    void    reset_hash_slot(){
        hash_slot_ = -1;
    }

protected:
    /** some get reply result util */
    
    /** 
     * @brief get array result
     * @param cmd - the command
     * @param param_list - in param list
     * @param array_result - out reply list
     */
    bool    get_array_result(redis_command& cmd,
        const std::vector<std::string>& param_list, 
        std::vector<std::string>& array_result){
        // append params
        for (auto& param : param_list){
            cmd.add_param(param);
        }

        redis_reply_ptr reply = do_command(cmd);
        if (!reply || !reply->is_array())
            return false;

        redis_reply_arr& arr = reply->to_array();
        for (auto& r : arr){
            if (r.is_string()){
                array_result.push_back(r.to_string());
            }
        }

        return true;
    }

    /**
     * @brief get array result
     * @param cmd - the command
     * @param array_result - out reply list
     */
    bool    get_array_result(redis_command& cmd,
        std::vector<std::string>& array_result){

        redis_reply_ptr reply = do_command(cmd);
        if (!reply || !reply->is_array())
            return false;

        redis_reply_arr& arr = reply->to_array();
        for (auto& r : arr){
            if (r.is_string()){
                array_result.push_back(r.to_string());
            }
        }

        return true;
    }

    /** 
     * @brief get integer result
     * @param cmd - the command
     */
    int32_t get_integer_result(redis_command& cmd, 
        const std::vector<std::string>& param_list){
        // append params
        for (auto& param : param_list){
            cmd.add_param(param);
        }

        redis_reply_ptr reply = do_command(cmd);
        if (!reply || !reply->is_integer())
            return 0;

        return reply->to_integer();
    }

    /** 
     * @brief get integer result
     */
    int32_t get_integer_result(redis_command& cmd){

        redis_reply_ptr reply = do_command(cmd);
        if (!reply || !reply->is_integer())
            return 0;

        return reply->to_integer();
    }

    /**
     * @brief get integer result
     * param out_value - if is not null, store the result
     */
    bool get_integer_result(redis_command& cmd, int32_t* out_value){

        redis_reply_ptr reply = do_command(cmd);
        if (!reply || !reply->is_integer())
            return false;

        if (out_value){
            *out_value = reply->to_integer();
        }
        return true;
    }

    /** 
     * @brief get float result
     * param out_value - if is not null, store the result
     */
    bool get_float_result(redis_command& cmd, double* out_value){

        redis_reply_ptr reply = do_command(cmd);
        if (!reply || !reply->is_string())
            return false;

        if (out_value){
            *out_value = atof(reply->to_string().c_str());
        }

        return true;
    }

    /** 
     * @brief get string result 
     * param out_value - if is not null, store the result
     */
    bool get_string_result(redis_command& cmd, std::string* out_value){
        redis_reply_ptr reply = do_command(cmd);
        if (!reply || !reply->is_string())
            return false;

        if (out_value){
            *out_value = reply->to_string();
        }

        return true;
    }

    /** 
     * @brief get string result
     */
    bool get_string_result(redis_command& cmd, std::string& out_value){
        redis_reply_ptr reply = do_command(cmd);
        if (!reply || !reply->is_string())
            return false;

        out_value = reply->to_string();

        return true;
    }

    /** 
     * @brief get string result
     */
    std::string get_string_result(redis_command& cmd){
        redis_reply_ptr reply = do_command(cmd);
        if (!reply || !reply->is_string())
            return "";

        return reply->to_string();
    }

    /** 
     * @brief get bool result
     */
    bool get_boolean_result(redis_command& cmd){
        redis_reply_ptr reply = do_command(cmd);
        if (!reply || !reply->is_integer())
            return false;

        return !!reply->to_integer();
    }

    /** 
     * @brief get string pair result
     */
    bool    get_string_pair_result(redis_command& cmd, 
        std::pair<std::string, std::string>& out_value){
        
        redis_reply_ptr reply = do_command(cmd);
        if (!reply || !reply->is_array())
            return false;

        redis_reply_arr& arr = reply->to_array();
        if (arr.size() != 2)
            return false;

        if (!arr[0].is_string() || !arr[1].is_string())
            return false;

        out_value.first = arr[0].to_string();
        out_value.second = arr[1].to_string();

        return true;
    }

    /** 
     * @brief check status ok
     */
    bool    check_status_ok(redis_command& cmd, bool* has_exception = nullptr){

        redis_reply_ptr reply = do_command(cmd);

        if (has_exception){
            *has_exception = !reply;
        }

        if (!reply || !reply->is_string()){
            return false;
        }

        return reply->check_status_ok();
    }

    /**
     * @brief get array reply
     */
    bool    get_array_reply(redis_command& cmd, redis_reply_arr& arr){

        redis_reply_ptr reply = do_command(cmd);
        if (!reply || !reply->is_array())
            return false;

        arr = reply->to_array();
        return true;
    }

    /** 
     * @brief scan key
     * @param scmd( scan, hscan and so on)
     * @param cursor - start cursor
     * @param out_result - store the result
     * @param pattern - the match pattern
     * @param count - the limit count( but sometime not exactly this count 
     * limit, maybe greater than the count)
     * @param return the next cursor
     */
    uint64_t scan_key(const char* scmd, const char* key, uint64_t cursor,
        redis_reply_arr& out_result,
        const char* pattern = nullptr,
        const int32_t* count = nullptr){

        redis_command cmd(scmd);

        if (key){
            cmd.add_param(key);
        }

        cmd.add_param(cursor);

        if (pattern){
            cmd.add_param("match");
            cmd.add_param(pattern);
        }

        if (count && *count){
            cmd.add_param("count");
            cmd.add_param(*count);
        }

        if (key){
            hash_slot(key);
        }
        else{
            reset_hash_slot();
        }

        redis_reply_ptr reply = do_command(cmd);
        if (!reply || !reply->is_array()){
            return 0;
        }

        redis_reply_arr& arr = reply->to_array();
        if (arr.size() != 2){
            return 0;
        }

        if (!arr[0].is_string() || !arr[1].is_array())
            return 0;

        std::string& next_cursor_str = arr[0].to_string();
        uint64_t next_cursor = 0;
        sscanf(next_cursor_str.c_str(), "%llu", &next_cursor);
        out_result = arr[1].to_array();

        return next_cursor;
    }

};
}
}

#endif