/**
 *
 * redis_key.hpp
 *
 * redis command of key
 *
 * @author  :   yandaren1220@126.com
 * @date    :   2017-04-27
 */

#ifndef __ydk_rediscpp_detail_redis_key_hpp__
#define __ydk_rediscpp_detail_redis_key_hpp__

#include <redis_cpp/detail/config.hpp>
#include <redis_cpp/detail/sync/redis_command_executor.hpp>
#include <unordered_map>

namespace redis_cpp
{
namespace detail
{
enum redis_key_type{
    redis_key_type_none = 0,
    redis_key_type_string,
    redis_key_type_list,
    redis_key_type_set,
    redis_key_type_zset,
    redis_key_type_hash,
};

static const std::unordered_map<std::string, redis_key_type> type_string_num_map = {
        { "string", redis_key_type_string },
        { "list", redis_key_type_list },
        { "set", redis_key_type_set },
        { "zset", redis_key_type_zset },
        { "hash", redis_key_type_hash },
};

static redis_key_type get_key_type_from_string(const std::string& p){
    auto iter = type_string_num_map.find(p);
    if (iter != type_string_num_map.end()){
        return iter->second;
    }
    return redis_key_type_none;
}

class redis_key : virtual public redis_command_executor
{
public:
    redis_key(){
    }

public:
    /**
    * @brief
    * @param key - the key to been delete
    * @return the count of the key which been deleted
    */
    int32_t del(const char* key)
    {
        redis_command cmd("del");

        // make commands
        cmd.add_param(key);

        hash_slot(key);

        return get_integer_result(cmd);
    }

    /**
    * @brief del key list
    * @param keys - key list
    * @return the count of the key which been deleted
    */
    int32_t del(const std::vector<std::string>& keys)
    {
        redis_command cmd("del");

        reset_hash_slot();

        return get_integer_result(cmd, keys);
    }

    /**
    * @brief if key exists
    * @param key - the key to judge
    * @return true of false
    */
    bool exist(const char* key)
    {
        redis_command cmd("exists");

        cmd.add_param(key);

        hash_slot(key);

        return get_boolean_result(cmd);
    }

    /**
    * @brief set a key's lifetime in seconds
    * @param key
    * @param t - lifetime(in seconds)
    * @return true if success, false if failed or the key not exist
    */
    bool expire(const char* key, int32_t t)
    {
        redis_command cmd("expire");

        cmd.add_param(key);
        cmd.add_param(t);

        hash_slot(key);

        return get_boolean_result(cmd);
    }

    /**
    * @brief set key will expire at specific time( unix timestamp)
    * @param key
    * @param stamp - unix time stamp(in seconds)
    * @return true if success, false if failed or the key not exist
    */
    bool expireat(const char* key, time_t stamp)
    {
        redis_command cmd("expireat");

        cmd.add_param(key);
        cmd.add_param((uint32_t)stamp);

        hash_slot(key);

        return get_boolean_result(cmd);
    }

    /**
    * @brief find all the keys which fit the pattern
    * @param pattern
    * @param keys - save the key list
    * @return true of false
    */
    bool keys_pattern(const char* pattern, std::vector<std::string>& out_keys)
    {
        redis_command cmd("keys");

        cmd.add_param(pattern);

        reset_hash_slot();

        return get_array_result(cmd, out_keys);
    }

    /**
    * @brief persist a key
    * @param key - the key to persist
    * @return true if success, otherwise return false
    */
    bool persist(const char* key)
    {
        redis_command cmd("persist");

        cmd.add_param(key);

        hash_slot(key);

        return get_boolean_result(cmd);
    }

    /**
    * @brief set the key's lifetime in millseconds
    * @param key
    * @param t - lifetime in milliseconds
    * @return true of false[key not exist or failed]
    */
    bool pexpire(const char* key, uint32_t t)
    {
        redis_command cmd("pexpire");

        cmd.add_param(key);
        cmd.add_param(t);

        hash_slot(key);

        return get_boolean_result(cmd);
    }

    /**
    * @brief set key will expire at specific time( unix timestamp)
    * @param key
    * @param stamp - unix time stamp(in milliseconds)
    * @return true if success, false if failed or the key not exist
    */
    bool pexpireat(const char* key, int64_t stamp)
    {
        redis_command cmd("pexpireat");

        cmd.add_param(key);
        cmd.add_param(stamp);

        hash_slot(key);

        return get_boolean_result(cmd);
    }

    /**
    * @brief return the remain lifetime of the key in milliseconds
    * @param key
    * @return -3 if error, 
    *         -2 if the key not exist, 
    *         -1 if the key exist but not set lifetime, 
    *          otherwise return the lifetime in milliseconds
    */
    int64_t pttl(const char* key)
    {
        redis_command cmd("pttl");

        cmd.add_param(key);

        hash_slot(key);

        return get_integer_result(cmd);
    }

    /**
    * @brief random return a key from the db
    * @param out_key - save the key from the db
    * @return true of false
    */
    bool randomkey(std::string& out_key)
    {
        redis_command cmd("randomkey");

        reset_hash_slot();

        return get_string_result(cmd, out_key);
    }

    /**
    * @brief rename the key the new key
    * if key is same with newkey, or the key not exist, will failed
    * if newkey exist, then the cmd will cover the old value
    * @param key
    * @param new_key
    * @return true of false
    */
    bool rename(const char* key, const char* new_key)
    {
        redis_command cmd("rename");

        cmd.add_param(key);
        cmd.add_param(new_key);

        hash_slot(key);

        return check_status_ok(cmd);
    }

    /**
    * @brief rename the key to newkey only if the newkey not exist
    * @param key
    * @param new_key
    * @return true of false
    */
    bool renamenx(const char* key, const char* new_key)
    {
        redis_command cmd("renamenx");

        cmd.add_param(key);
        cmd.add_param(new_key);

        hash_slot(key);

        return get_boolean_result(cmd);
    }

    /**
    * @brief simple sort
    * @return true of false
    */
    bool sort(const char* key, std::vector<std::string>& out_elements)
    {
        redis_command cmd("sort");

        cmd.add_param(key);

        hash_slot(key);

        return get_array_result(cmd, out_elements);
    }

    /**
    * @brief sort desc
    * @return true of false
    */
    bool sortDesc(const char* key, std::vector<std::string>& out_elements)
    {
        redis_command cmd("sort");

        cmd.add_param(key);
        cmd.add_param("desc");

        hash_slot(key);

        return get_array_result(cmd, out_elements);
    }

    /**
    * @brief return the key's remain lifetime in seconds
    * @param key
    * @return -3 something is error
    -2 if key not exist
    -1 if key exist, but not set lifetime
    otherwise, return the remain lifetime in seconds
    */
    int32_t ttl(const char* key)
    {
        redis_command cmd("ttl");

        cmd.add_param(key);

        hash_slot(key);

        return get_integer_result(cmd);
    }

    /**
    * @brief return the type of the value which the key store
    * @param key
    * @return key type
    */
    redis_key_type type(const char* key)
    {
        redis_command cmd("type");

        cmd.add_param(key);

        hash_slot(key);

        std::string ret = "";

        get_string_result(cmd, ret);

        return get_key_type_from_string(ret);
    }

    /**
    * @brief incrementally iterate the keys space in the specified database
    * @param cursor
    * @param out_elements - save the result
    * @param pattern - match pattern
    * @param count - limit the max number of the reslts stored in array
    * @param return the next cursor
    */
    uint64_t scan(uint64_t cursor, std::vector <std::string>& out_elements,
        const char* pattern = nullptr, const int32_t* count = nullptr)
    {

        redis_reply_arr array_reply;
        uint64_t next_cursor = 
            scan_key("scan", nullptr, cursor, array_reply, pattern, count);

        for (std::size_t i = 0; i < array_reply.size(); ++i)
        {
            std::string s;
            out_elements.push_back(array_reply[i].to_string());
        }

        return next_cursor;
    }
};
}
}

#endif