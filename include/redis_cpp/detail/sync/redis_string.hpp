/**
 *
 * redis_string.hpp
 *
 * redis command of string
 *
 * @author  :   yandaren1220@126.com
 * @date    :   2017-04-27
 */

#ifndef __ydk_rediscpp_detail_redis_string_hpp__
#define __ydk_rediscpp_detail_redis_string_hpp__

#include <redis_cpp/detail/config.hpp>
#include <redis_cpp/detail/sync/redis_command_executor.hpp>
#include <unordered_map>

namespace redis_cpp
{
namespace detail
{
class redis_string : virtual public redis_command_executor
{
public:
    redis_string(){
    }

public:
    /**
    * @brief append the value to the tail of the key's old value
    * @param key
    * @param append_value
    * @return the string length of new string
    */
    int32_t append(const char* key, const char* append_value, int32_t size)
    {
        redis_command cmd("append");

        cmd.add_param(key);
        cmd.add_param(append_value, size);

        hash_slot(key);

        return get_integer_result(cmd);
    }

    /**
    * @brief get the total bitcount which set as 1 of the key's value
    * @param  bit_count - save the result
    * @return true of false
    */
    bool bitcount(const char* key, int32_t& bitcount)
    {
        redis_command cmd("bitcount");

        cmd.add_param(key);

        hash_slot(key);

        return get_integer_result(cmd, &bitcount);
    }

    /**
    * @brief bit and operatation
    * @param dest_key - the key to save the bit operation result
    * @param keys - the keys to do bit and
    * @return the result string length
    */
    int32_t bit_and(const char* destkey, const std::vector<std::string>& keys)
    {
        return bitop("AND", destkey, keys);
    }

    /**
    * @brief bit or operatation
    * @param dest_key - the key to save the bit operation result
    * @param keys - the keys to do bit or
    * @return the result string length
    */
    int32_t bit_or(const char* destkey, const std::vector<std::string>& keys)
    {
        return bitop("OR", destkey, keys);
    }

    /**
    * @brief bit operatation
    * @param dest_key - the key to save the bit operation result
    * @param keys - the keys to do bit xor
    * @return the result string length
    */
    int32_t bit_xor(const char* destkey, const std::vector<std::string>& keys)
    {
        return bitop("XOR", destkey, keys);
    }

    /**
    * @brief bit operatation
    * @param dest_key - the key to save the bit operation result
    * @param keys - the keys to do bit not
    * @return the result string length
    */
    int32_t bit_not(const char* destkey, const std::vector<std::string>& keys)
    {
        return bitop("NOT", destkey, keys);
    }

    /**
    * @brief decrease 1 of the value
    * @param key
    * @param return the new value after decrease
    */
    int32_t decr(const char* key)
    {
        redis_command cmd("decr");

        cmd.add_param(key);

        hash_slot(key);

        return get_integer_result(cmd);
    }

    /**
    * @brief decrease specific value of the key's value
    * @param key
    * @param return the new value after decrease
    */
    int32_t decrby(const char* key, int32_t decrement)
    {
        redis_command cmd("decrby");

        cmd.add_param(key);
        cmd.add_param(decrement);

        hash_slot(key);

        return get_integer_result(cmd);
    }

    /**
    * @brief get the value of the key
    * @param key
    * @param out_value - save the result
    * @return nil if key not exist, otherwise return the value
    */
    bool get(const char* key, std::string& out_value)
    {
        redis_command cmd("get");

        cmd.add_param(key);

        hash_slot(key);

        return get_string_result(cmd, out_value);
    }

    /**
    * @brief get the value of the key
    * @param key
    * @param out_value - save the result
    * @return nil if key not exist, otherwise return the value
    */
    bool get(const char* key, int32_t& out_value)
    {
        std::string out_value_str;
        if (get(key, out_value_str))
        {
            sscanf(out_value_str.c_str(), "%d", &out_value);
            return true;
        }
        return false;
    }

    /**
    * @brief get the value of the key
    * @param key
    * @param out_value - save the result
    * @return nil if key not exist, otherwise return the value
    */
    bool get(const char* key, int64_t& out_value)
    {
        std::string out_value_str;
        if (get(key, out_value_str))
        {
            sscanf(out_value_str.c_str(), "%lld", &out_value);
            return true;
        }
        return false;
    }

    /**
    * @brief get the value of the key
    * @param key
    * @param out_value - save the result
    * @return nil if key not exist, otherwise return the value
    */
    bool get(const char* key, double& out_value)
    {
        std::string out_value_str;
        if (get(key, out_value_str))
        {
            sscanf(out_value_str.c_str(), "%lf", &out_value);
            return true;
        }
        return false;
    }

    /**
    * @brief get the bit of according offset
    * @param key
    * @param offset
    * @param bit
    * @return true of false
    */
    bool getbit(const char* key, int32_t offset, int32_t& bit)
    {
        redis_command cmd("getbit");

        cmd.add_param(key);
        cmd.add_param(offset);

        hash_slot(key);

        return get_integer_result(cmd, &bit);
    }

    /**
    * @brief get substring[ start -> end]
    * @param key
    * @param start - 起始位置
    * @param end - 结束位置
    * @param sub_string -存储截取的substring
    * @return true of false
    */
    bool getrange(const char* key, 
        int32_t start, int32_t end, std::string& sub_string)
    {
        redis_command cmd("getrange");

        cmd.add_param(key);
        cmd.add_param(start);
        cmd.add_param(end);

        hash_slot(key);

        return get_string_result(cmd, sub_string);
    }

    /**
    * @brief set the key's value and return the old value
    * @param key
    * @param new_value
    * @param out_old_value - 存储老的值
    * @return true of false
    */
    bool getset(const char* key, 
        const char* value, std::string& out_old_value)
    {
        redis_command cmd("getset");

        cmd.add_param(key);
        cmd.add_param(value);

        hash_slot(key);

        return get_string_result(cmd, out_old_value);
    }

    /**
    * @brief increase 1 of the key's value
    * @param key
    * @return the new value
    */
    int32_t incr(const char* key)
    {
        redis_command cmd("incr");

        cmd.add_param(key);

        hash_slot(key);

        return get_integer_result(cmd);
    }

    /**
    * @brief increase increment of the key's value
    * @param key
    * @param increment
    * @return the new value after increase
    */
    int32_t incrby(const char* key, int32_t increment)
    {
        redis_command cmd("incrby");

        cmd.add_param(key);
        cmd.add_param(increment);

        hash_slot(key);

        return get_integer_result(cmd);
    }

    /**
    * @brief increase by float increment
    * @param key
    * @param increment
    * @param out_new_value - save the new value
    * @return true or false
    */
    bool    incrbyfloat(const char* key, 
        double increment, double* out_new_value)
    {
        redis_command cmd("incrbyfloat");

        cmd.add_param(key);
        cmd.add_param(increment);

        hash_slot(key);

        return get_float_result(cmd, out_new_value);
    }

    /**
    * @brief get muti key's value
    * @param keys - the keys to find
    * @param out_values - the keys' values
    * @return true of false
    */
    bool mget(const std::vector<std::string>& keys, 
        std::vector<std::string>& out_values)
    {
        redis_command cmd("mget");

        reset_hash_slot();

        return get_array_result(cmd, keys, out_values);
    }

    /**
    * @brief muti set key-value pair
    * @param key_value_pairs
    * @return true or false
    */
    bool mset(const std::unordered_map<std::string, std::string>& kv_pairs)
    {
        redis_command cmd("mset");

        auto iter = kv_pairs.begin();
        for (iter; iter != kv_pairs.end(); ++iter)
        {
            cmd.add_param(iter->first);
            cmd.add_param(iter->second);
        }

        reset_hash_slot();

        return check_status_ok(cmd);
    }

    /**
    * @brief muti set key_vlaue pair
    * this operate is atomic, success only when all the keys given not exists
    * @param key_value_pairs
    * @return true or false
    */
    bool msetnx(const std::unordered_map<std::string, std::string>& kv_pairs)
    {
        redis_command cmd("msetnx");

        auto iter = kv_pairs.begin();
        for (iter; iter != kv_pairs.end(); ++iter)
        {
            cmd.add_param(iter->first);
            cmd.add_param(iter->second);
        }

        reset_hash_slot();

        return get_boolean_result(cmd);
    }

    /**
    * @brief set the key's value and lifetime( in seconds)
    * @param key
    * @param value
    * @param lifetime (in seconds)
    * @return true or false
    */
    bool setex(const char* key, const char* value, int32_t lifetime)
    {
        redis_command cmd("setex");

        cmd.add_param(key);
        cmd.add_param(lifetime);
        cmd.add_param(value);

        hash_slot(key);

        return check_status_ok(cmd);
    }

    /**
    * @brief set the key's value and lifetime( in seconds)
    * @param key
    * @param value
    * @param value_len
    * @param lifetime (in seconds)
    * @return true or false
    */
    bool setex(const char* key, 
        const char* value, int32_t value_len, int32_t lifetime)
    {
        redis_command cmd("setex");

        cmd.add_param(key);
        cmd.add_param(lifetime);
        cmd.add_param(value, value_len);

        hash_slot(key);

        return check_status_ok(cmd);
    }

    /**
    * @brief like cmd setex, but the lifetime in milliseconds
    * @param key
    * @param value
    * @param lifetime (in milliseconds)
    * @return true or false
    */
    bool psetex(const char* key, const char* value, int32_t lifetime)
    {
        redis_command cmd("psetex");

        cmd.add_param(key);
        cmd.add_param(lifetime);
        cmd.add_param(value);

        hash_slot(key);

        return check_status_ok(cmd);
    }

    /**
    * @brief like cmd setex, but the lifetime in milliseconds
    * @param key
    * @param value
    * @param value_len
    * @param lifetime (in milliseconds)
    * @return true or false
    */
    bool psetex(const char* key, 
        const char* value, int32_t value_len, int32_t lifetime)
    {
        redis_command cmd("psetex");

        cmd.add_param(key);
        cmd.add_param(lifetime);
        cmd.add_param(value, value_len);

        hash_slot(key);

        return check_status_ok(cmd);
    }

    /**
    * @brief set the key's value, success only if the key not exist
    * @param key
    * @param value
    * @return true of false
    */
    bool setnx(const char* key, const char* value)
    {
        redis_command cmd("setnx");

        cmd.add_param(key);
        cmd.add_param(value);

        hash_slot(key);

        return get_boolean_result(cmd);
    }

    /**
    * @brief set 'key' 'value' nx ex max_lock_time
    * @brief to implement a simple lock
    * @param key
    * @param value
    * @param life_time - (in seconds)
    */
    bool setnxex(const char* key, const std::string& value, int32_t lifetime)
    {
        redis_command cmd("set");

        cmd.add_param(key);
        cmd.add_param(value);

        static const std::string nx_dict = "nx";
        static const std::string ex_dict = "ex";

        cmd.add_param(ex_dict);
        cmd.add_param(lifetime);
        cmd.add_param(nx_dict);

        hash_slot(key);

        return check_status_ok(cmd);
    }

    /**
    * @brief set 'key' 'value' nx ex max_lock_time
    * @brief to implement a simple lock
    * @param key
    * @param value
    * @param life_time - (in milliseconds)
    */
    bool setnxpx(const char* key, const std::string& value, int32_t lifetime)
    {
        redis_command cmd("set");

        cmd.add_param(key);
        cmd.add_param(value);

        static const std::string nx_dict = "nx";
        static const std::string px_dict = "px";

        cmd.add_param(px_dict);
        cmd.add_param(lifetime);
        cmd.add_param(nx_dict);

        hash_slot(key);

        return check_status_ok(cmd);
    }

    /**
    * @brief set 'key' 'value' nx ex max_lock_time
    * @brief to implement a simple lock
    * @param key
    * @param value
    * @param life_time - (in milliseconds)
    * @param has_exception return error code
    */
    bool setnxpx(const char* key, 
        const std::string& value, int32_t lifetime, bool* has_exception)
    {
        redis_command cmd("set");

        cmd.add_param(key);
        cmd.add_param(value);

        static const std::string nx_dict = "nx";
        static const std::string px_dict = "px";

        cmd.add_param(px_dict);
        cmd.add_param(lifetime);
        cmd.add_param(nx_dict);

        hash_slot(key);

        return check_status_ok(cmd, has_exception);
    }

    /**
    * @brief set the key's value
    * @param key { const char*}
    * @param value { const char*}
    * @return true or false
    */
    bool set(const char* key, const char* value)
    {
        redis_command cmd("set");

        cmd.add_param(key);
        cmd.add_param(value);

        hash_slot(key);

        return check_status_ok(cmd);
    }

    /**
    * @brief set the key's value
    * @param key { const char*}
    * @param value { const char*}
    * @param value_len
    * @return true or false
    */
    bool set(const char* key, const char* value, int32_t value_len)
    {
        redis_command cmd("set");

        cmd.add_param(key);
        cmd.add_param(value, value_len);

        hash_slot(key);

        return check_status_ok(cmd);
    }

    /**
    * @brief set the key's value
    * @param key { const char*}
    * @param value { const char*}
    * @return true or false
    */
    bool set(const char* key, int32_t value)
    {
        redis_command cmd("set");

        cmd.add_param(key);
        cmd.add_param(value);

        hash_slot(key);

        return check_status_ok(cmd);
    }

    /**
    * @brief set the key's value
    * @param key { const char*}
    * @param value { const char*}
    * @return true or false
    */
    bool set(const char* key, int64_t value)
    {
        redis_command cmd("set");

        cmd.add_param(key);
        cmd.add_param(value);

        hash_slot(key);

        return check_status_ok(cmd);
    }

    /**
    * @brief set the key's value
    * @param key { const char*}
    * @param value { const char*}
    * @return true or false
    */
    bool set(const char* key, double value)
    {
        redis_command cmd("set");

        cmd.add_param(key);
        cmd.add_param(value);

        hash_slot(key);

        return check_status_ok(cmd);
    }

    /**
    * @brief set the bit of specific offset of the value
    * @param key
    * @param offset
    * @param bit[ 0 or 1]
    * @return the old bit of the offset
    */
    int32_t setbit(const char* key, int32_t offset, bool bit)
    {
        redis_command cmd("setbit");

        cmd.add_param(key);
        cmd.add_param(offset);
        cmd.add_param((int32_t)bit);

        hash_slot(key);

        return get_integer_result(cmd);
    }

    /**
    * @brief override the string from the offset
    * @param key
    * @param offset
    * @param new_value
    * @return the new length of the string
    */
    int32_t setrange(const char* key, int32_t offset, const char* new_value)
    {
        redis_command cmd("setrange");

        cmd.add_param(key);
        cmd.add_param(offset);
        cmd.add_param(new_value);

        hash_slot(key);

        return get_integer_result(cmd);
    }

    /**
    * @brief override the string from the offset
    * @param key
    * @param offset
    * @param new_value
    * @param new_value_len
    * @return the new length of the string
    */
    int32_t setrange(const char* key, 
        int32_t offset, const char* new_value, int32_t new_value_len)
    {
        redis_command cmd("setrange");

        cmd.add_param(key);
        cmd.add_param(offset);
        cmd.add_param(new_value, new_value_len);

        hash_slot(key);

        return get_integer_result(cmd);
    }

    /**
    * @brief get the string length stored in the key
    * @param key
    * @return the string length( 0 if the key not exists)
    */
    int32_t strlen(const char* key)
    {
        redis_command cmd("strlen");

        cmd.add_param(key);

        hash_slot(key);

        return get_integer_result(cmd);
    }

protected:

    /**
    * @brief bit top operatation
    * @param operate - 操作
    * @param destkey - the key to save the result
    * @param keys - the keys to do bit operatation
    */
    int32_t bitop(const char* operate, 
        const char* destkey, const std::vector<std::string>& keys)
    {
        redis_command cmd("bittop");

        cmd.add_param(operate);
        cmd.add_param(destkey);

        reset_hash_slot();

        return get_integer_result(cmd, keys);
    }
};
}
}

#endif