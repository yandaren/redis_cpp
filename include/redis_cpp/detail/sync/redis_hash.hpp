/**
 *
 * redis_hash.hpp
 *
 * redis command of hash
 *
 * @author  :   yandaren1220@126.com
 * @date    :   2017-04-27
 */

#ifndef __ydk_rediscpp_detail_redis_hash_hpp__
#define __ydk_rediscpp_detail_redis_hash_hpp__

#include <redis_cpp/detail/config.hpp>
#include <redis_cpp/detail/sync/redis_command_executor.hpp>

namespace redis_cpp
{
namespace detail
{
class redis_hash : virtual public redis_command_executor
{
public:
    redis_hash(){

    }

public:
    /**
    * @brief delete one or muti field of the hash table
    * @param key
    * @param field
    * @param return the count that removed
    */
    int32_t hdel(const char* key, const char* field)
    {
        redis_command cmd("hdel");

        cmd.add_param(key);
        cmd.add_param(field);

        hash_slot(key);

        return (int32_t)get_integer_result(cmd);
    }

    /**
    * @brief delete one or muti field of the hash table
    * @param key
    * @param fields
    * @param return the count that removed
    */
    int32_t hdel(const char* key, const std::vector<std::string>& fields)
    {
        redis_command cmd("hdel");

        cmd.add_param(key);

        hash_slot(key);

        return (int32_t)get_integer_result(cmd, fields);
    }

    /**
    * @brief check the field of the hash table exist
    * @param key
    * @param field
    * @return true of false
    */
    bool hexists(const char* key, const char* field)
    {
        redis_command cmd("hexists");

        cmd.add_param(key);
        cmd.add_param(field);

        hash_slot(key);

        return get_boolean_result(cmd);
    }

    /**
    * @brief get the field value of the hash table
    * @param key
    * @param field
    * @param out_value - save the result
    * @param return true or false
    */
    bool hget(const char* key, const char* field, std::string& out_value)
    {
        redis_command cmd("hget");

        cmd.add_param(key);
        cmd.add_param(field);

        hash_slot(key);

        return get_string_result(cmd, out_value);
    }

    /**
    * @brief set the field's value
    * @param key
    * @param field
    * @param value
    * @return 1 if the field is a new field, and set success
    0 if the field already exists the the old value covered by the new value
    */
    int32_t hset(const char* key, const char* filed, const char* value)
    {
        redis_command cmd("hset");

        cmd.add_param(key);
        cmd.add_param(filed);
        cmd.add_param(value);

        hash_slot(key);

        return (int32_t)get_integer_result(cmd);
    }

    /**
    * @brief set the field's value
    * @param key
    * @param field
    * @return 1 if the field is a new field, and set success
    0 if the field already exists the the old value covered by the new value
    */
    int32_t hset(const char* key, const char* filed, const char* value, int32_t size)
    {
        redis_command cmd("hset");

        cmd.add_param(key);
        cmd.add_param(filed);
        cmd.add_param(value, size);

        hash_slot(key);

        return (int32_t)get_integer_result(cmd);
    }

    /**
    * @brief set the field's value, success only when the field not exist
    * @param key
    * @param field
    * @param value
    * @return true or false
    */
    bool hsetnx(const char* key, const char* filed, const char* value)
    {
        redis_command cmd("hsetnx");

        cmd.add_param(key);
        cmd.add_param(filed);
        cmd.add_param(value);

        hash_slot(key);

        return get_boolean_result(cmd);
    }

    /**
    * @brief get muti field's value
    * @param key
    * @param fields
    * @param out_values - save the result
    * @return true or false
    */
    bool hmget(const char* key, 
        const std::vector<std::string>& fields, std::vector<std::string>& out_values)
    {
        redis_command cmd("hmget");

        cmd.add_param(key);

        hash_slot(key);

        bool f = get_array_result(cmd, fields, out_values);

        return (f && fields.size() == out_values.size());
    }

    /**
    * @brief set muti field-value pairs to the hash table
    * @param key
    * @param field_value_map
    * @return true or false
    */
    bool hmset(const char* key, 
        const std::unordered_map<std::string, std::string>& field_value_map)
    {
        redis_command cmd("hmset");

        cmd.add_param(key);

        auto iter = field_value_map.begin();
        auto iter_end = field_value_map.end();
        for (iter; iter != iter_end; ++iter)
        {
            cmd.add_param(iter->first);
            cmd.add_param(iter->second);
        }

        hash_slot(key);

        return check_status_ok(cmd);
    }

    /**
    * @brief get all the field and value of the hash table
    * @param key
    * @param out_table - save the table
    * @return true or false
    */
    bool hgetall(const char* key, 
        std::unordered_map<std::string, std::string>& out_table)
    {
        redis_command cmd("hgetall");

        cmd.add_param(key);

        hash_slot(key);

        redis_reply_arr arr;
        if (!get_array_reply(cmd, arr)){
            return false;
        }

        if (arr.size() % 2 != 0){
            return false;
        }

        for (int32_t i = 0; i < int32_t(arr.size()) - 1; i = i + 2)
        {
            out_table.insert(
                std::move(std::pair<std::string, std::string>(
                arr[i].to_string(), arr[i+1].to_string())));
        }

        return true;
    }

    /**
    * @brief get all the field of the hash table
    * @param key
    * @param out_fields - save the result
    * @return true or false
    */
    bool hkeys(const char* key, std::vector<std::string>& out_fields)
    {
        redis_command cmd("hkeys");

        cmd.add_param(key);

        hash_slot(key);

        return get_array_result(cmd, out_fields);
    }

    /**
    * @brief get the field's values of the hash table
    * @param key
    * @param out_values - save the result
    * @return true or false
    */
    bool hvals(const char* key, std::vector<std::string>& out_values)
    {
        redis_command cmd("hvals");

        cmd.add_param(key);

        hash_slot(key);

        return get_array_result(cmd, out_values);
    }

    /**
    * @brief get the field count of the hash table
    * @param key
    * @return the count of the hash table
    */
    int32_t hlen(const char* key)
    {
        redis_command cmd("hlen");

        cmd.add_param(key);

        hash_slot(key);

        return (int32_t)get_integer_result(cmd);
    }

    /**
    * @brief increase the field's value by increment
    * @param key
    * @param field
    * @param increment
    * @param out_new_value - save the new value
    * @param return true or false
    */
    bool hincrby(const char* key, 
        const char* filed, int32_t increment, int32_t* out_new_value)
    {
        redis_command cmd("hincrby");

        cmd.add_param(key);
        cmd.add_param(filed);
        cmd.add_param(increment);

        hash_slot(key);

        return get_integer_result(cmd, out_new_value);
    }

    /**
    * @brief increase the field's value by float increment
    * @param key
    * @param field
    * @param increment
    * @param out_new_value - save the new value
    * @param return true or false
    */
    bool hincrbyfloat(const char* key, 
        const char* filed, double increment, double* out_new_value)
    {
        redis_command cmd("hincrbyfloat");

        cmd.add_param(key);
        cmd.add_param(filed);
        cmd.add_param(increment);

        hash_slot(key);

        return get_float_result(cmd, out_new_value);
    }

    /**
    * @brief hscan, like scan cmd
    * @param key
    * @param cursor [ start cursor]
    * @param out_result - save the result
    * @param pattern - match pattern
    * @param count - count
    * @return the next cursor
    */
    uint64_t hscan(const char* key, 
        uint64_t cursor, 
        std::unordered_map<std::string, std::string>& out_result,
        const char* pattern = nullptr, const int32_t* count = nullptr)
    {
        redis_reply_arr arr;
        uint64_t next_cursor = scan_key("hscan", key, cursor, arr, pattern, count);

        if (arr.empty()) {
            return 0;
        }

        if (arr.size() % 2 != 0)
            return 0;

        out_result.clear();
        for (int32_t i = 0; i < int32_t(arr.size()) - 1; i = i + 2){
            if (!arr[i].is_string() || !arr[i + 1].is_string()) {
                continue;
            }

            auto field = arr[i].to_string();
            auto value = arr[i + 1].to_string();

            out_result.insert(std::pair<std::string, std::string>(field, value));
        }

        return next_cursor;
    }
};
}
}

#endif
