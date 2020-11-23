/**
 *
 * redis_set.hpp
 *
 * redis command of set
 *
 * @author  :   yandaren1220@126.com
 * @date    :   2017-04-27
 */
#ifndef __ydk_rediscpp_detail_redis_set_hpp__
#define __ydk_rediscpp_detail_redis_set_hpp__

#include <redis_cpp/detail/config.hpp>
#include <redis_cpp/detail/sync/redis_command_executor.hpp>

namespace redis_cpp
{
namespace detail
{
class redis_set : virtual public redis_command_executor
{
public:
    redis_set(){
    }

public:
    /**
    * @brief add one value to the set
    * @param key
    * @param value
    * @return the count of the element added to the set
    */
    int32_t sadd(const char* key, const char* value)
    {
        redis_command cmd("sadd");

        cmd.add_param(key);
        cmd.add_param(value);

        hash_slot(key);

        return (int32_t)get_integer_result(cmd);
    }

    /**
    * @brief add one value to the set
    * @param key
    * @param value
    * @return the count of the element added to the set
    */
    int32_t sadd(const char* key, const char* value, int32_t size)
    {
        redis_command cmd("sadd");

        cmd.add_param(key);
        cmd.add_param(value, size);

        hash_slot(key);

        return (int32_t)get_integer_result(cmd);
    }

    /**
    * @brief add one value to the set
    * @param key
    * @param value
    * @return the count of the element added to the set
    */
    int32_t sadd(const char* key, const std::string& value)
    {
        redis_command cmd("sadd");

        cmd.add_param(key);
        cmd.add_param(value);

        hash_slot(key);

        return (int32_t)get_integer_result(cmd);
    }

    /**
    * @brief add muti value to the set
    * @param key
    * @param value
    * @return the count of the element added to the set
    */
    int32_t sadd(const char* key, const std::vector<std::string>& values)
    {
        redis_command cmd("sadd");

        cmd.add_param(key);

        hash_slot(key);

        return (int32_t)get_integer_result(cmd, values);
    }

    /**
    * @brief return the element count of the set
    * @param key
    * @return the set size
    */
    int32_t scard(const char* key)
    {
        redis_command cmd("scard");

        cmd.add_param(key);

        hash_slot(key);

        return (int32_t)get_integer_result(cmd);
    }

    /**
    * @brief remove and pop a random element of the set
    * @param key
    * @param out_value - save the poped value
    * @return true or false
    */
    bool spop(const char* key, std::string* out_value)
    {
        redis_command cmd("spop");

        cmd.add_param(key);

        hash_slot(key);

        return get_string_result(cmd, out_value);
    }

    /**
    * @brief return a random element of the set
    * @param key
    * @param out_value - save the result
    * @return true or false
    */
    bool srandmember(const char* key, std::string& out_value)
    {
        redis_command cmd("srandmember");

        cmd.add_param(key);

        hash_slot(key);

        return get_string_result(cmd, out_value);
    }

    /**
    * @brief return the specific count random element of the set
    * @param key
    * @param count - if count > 0, each element of the result is different with others,
    - if count < 0, each element of the result my same with others
    * @param out_elements - save the result
    * @return true or false
    */
    bool srandmember(const char* key, 
        int32_t count, std::vector<std::string>& out_elements)
    {
        redis_command cmd("srandmember");

        cmd.add_param(key);
        cmd.add_param(count);

        hash_slot(key);

        std::vector<std::string> param_list;
        return get_array_result(cmd, param_list, out_elements);
    }

    /**
    * @brief return all the elements of the set
    * @param key
    * @param out_elements - save the result
    * @return true or false
    */
    bool smembers(const char* key, std::vector<std::string>& out_elements)
    {
        redis_command cmd("smembers");

        cmd.add_param(key);

        hash_slot(key);

        std::vector<std::string> param_list;
        return get_array_result(cmd, param_list, out_elements);
    }

    /**
    * @brief check the specific value is the member of the set
    * @param key
    * @param value
    * @return true or false
    */
    bool sismember(const char* key, const char* value)
    {
        redis_command cmd("sismember");

        cmd.add_param(key);
        cmd.add_param(value);

        hash_slot(key);

        return get_boolean_result(cmd);
    }

    /**
    * @brief remove specific value from the set
    * @param key
    * @param value
    * @return the element count that been removed
    */
    int32_t srem(const char* key, const char* value)
    {
        redis_command cmd("srem");

        std::vector<std::string> param_list;
        param_list.push_back(key);
        param_list.push_back(value);

        hash_slot(key);

        return get_integer_result(cmd, param_list);
    }

    /**
    * @brief remove muti values from the set
    * @param key
    * @param value
    * @return the element count that been removed
    */
    int32_t srem(const char* key, const std::vector<std::string>& values_to_remove)
    {
        redis_command cmd("srem");

        cmd.add_param(key);

        hash_slot(key);

        return get_integer_result(cmd, values_to_remove);
    }

    /**
    * @brief move specific value from one set to another, the operate is atomic
    * @param source_set
    * @param dest_set
    * @param value
    * @return return or false
    */
    bool smove(const char* source_set, const char* dest_set, const char* value)
    {
        redis_command cmd("smove");

        cmd.add_param(source_set);
        cmd.add_param(dest_set);
        cmd.add_param(value);

        reset_hash_slot();

        return get_boolean_result(cmd);
    }

    /**
    * @brief get the diff set of all the sets given
    * @param set1
    * @param set2
    * @param diff_set - save the result
    * @return true or false
    */
    bool sdiff(const char* set1, 
        const char* set2, std::vector<std::string>& diff_set)
    {
        redis_command cmd("sdiff");

        std::vector<std::string> param_list;
        param_list.push_back(set1);
        param_list.push_back(set2);

        reset_hash_slot();

        return get_array_result(cmd, param_list, diff_set);
    }

    /**
    * @brief get the diff set of all the sets given
    * @param sets
    * @param diff_set - save the result
    * @return true or false
    */
    bool sdiff(const std::vector<std::string>& set_list, 
        std::vector<std::string>& diff_set)
    {
        redis_command cmd("sdiff");

        reset_hash_slot();

        return get_array_result(cmd, set_list, diff_set);
    }

    /**
    * @brief store the diff set of all the sets given to the specific set
    * @param dest_set
    * @param set1
    * @param set2
    * @return the diffset element count
    */
    int32_t sdiffstore(const char* dest_set, const char* set1, const char* set2)
    {
        redis_command cmd("sdiffstore");

        cmd.add_param(dest_set);

        std::vector<std::string> param_list;
        param_list.push_back(set1);
        param_list.push_back(set2);

        reset_hash_slot();

        return get_integer_result(cmd, param_list);
    }

    /**
    * @brief store the diff set of all the sets given to the specific set
    * @param dest_set
    * @param set1
    * @param set2
    * @return the diffset element count
    */
    int32_t sdiffstore(const char* dest_set, 
        const std::vector<std::string>& set_list)
    {
        redis_command cmd("sdiffstore");

        cmd.add_param(dest_set);

        reset_hash_slot();

        return get_integer_result(cmd, set_list);
    }

    /**
    * @brief get the intersection of all the sets given
    * @param set1
    * @param set2
    * @param inter_set - save the result
    * @return true or false
    */
    bool sinter(const char* set1, 
        const char* set2, std::vector <std::string>& inter_set)
    {
        redis_command cmd("sinter");

        std::vector<std::string> param_list;
        param_list.push_back(set1);
        param_list.push_back(set2);

        reset_hash_slot();

        return get_array_result(cmd, param_list, inter_set);
    }

    /**
    * @brief get the intersection of all the sets given
    * @param sets
    * @param inter_set - save the result
    * @return true or false
    */
    bool sinter(const std::vector<std::string>& set_list, 
        std::vector<std::string>& inter_set)
    {
        redis_command cmd("sinter");

        reset_hash_slot();

        return get_array_result(cmd, set_list, inter_set);
    }

    /**
    * @brief store the intersection of all the sets given to specific set
    * @param dest_set
    * @param set1
    * @param set2
    * @return the element count of the intersection set
    */
    int32_t sinterstore(const char* dest_set, 
        const char* set1, const char* set2)
    {
        redis_command cmd("sinterstore");

        std::vector<std::string> param_list;
        param_list.push_back(dest_set);
        param_list.push_back(set1);
        param_list.push_back(set2);

        reset_hash_slot();

        return get_integer_result(cmd, param_list);
    }

    /**
    * @brief store the intersection of all the sets given to specific set
    * @param dest_set
    * @param set1
    * @param set2
    * @return the element count of the intersection set
    */
    int32_t sinterstore(const char* dest_set, 
        const std::vector < std::string>& set_list)
    {
        redis_command cmd("sinterstore");

        cmd.add_param(dest_set);

        reset_hash_slot();

        return get_integer_result(cmd, set_list);
    }

    /**
    * @brief get the union of all the sets given
    * @param set1
    * @param set2
    * @param union_set - save the result
    * @return true or false
    */
    bool sunion(const char* set1, 
        const char* set2, std::vector<std::string>& union_set)
    {
        redis_command cmd("sunion");

        std::vector<std::string> param_list;

        param_list.push_back(set1);
        param_list.push_back(set2);

        reset_hash_slot();

        return get_array_result(cmd, param_list, union_set);
    }

    /**
    * @brief get the union of all the sets given
    * @param set_list
    * @param union_set - save the result
    * @return true or false
    */
    bool sunion(const std::vector<std::string>& set_list, 
        std::vector<std::string>& union_set)
    {
        redis_command cmd("sunion");

        reset_hash_slot();

        return get_array_result(cmd, set_list, union_set);
    }

    /**
    * @brief store the union set of all the sets given to specific set
    * @param dest_set
    * @param set1
    * @param set2
    * @return the element count of the intersection set
    */
    int32_t sunionstore(const char* dest_set, 
        const char* set1, const char* set2)
    {
        redis_command cmd("sunionstore");

        std::vector<std::string> param_list;
        param_list.push_back(dest_set);
        param_list.push_back(set1);
        param_list.push_back(set2);

        reset_hash_slot();

        return get_integer_result(cmd, param_list);
    }

    /**
    * @brief store the union set of all the sets given to specific set
    * @param dest_set
    * @param set1
    * @param set2
    * @return the element count of the intersection set
    */
    int32_t sunionstore(const char* dest_set, 
        const std::vector < std::string>& set_list)
    {
        redis_command cmd("sunionstore");

        cmd.add_param(dest_set);

        reset_hash_slot();

        return get_integer_result(cmd, set_list);
    }

    /**
    * @brief sscan, like scan cmd
    * @param key
    * @param cursor [ start cursor]
    * @param out_result - save the result
    * @param pattern - match pattern
    * @param count - count
    * @return the next cursor
    */
    uint64_t sscan(const char* key, 
        uint64_t cursor, 
        std::vector<std::string>& out_result,
        const char* pattern = nullptr, const int32_t* count = nullptr)
    {
        redis_reply_arr arr;
        uint64_t next_cursor = 
            scan_key("sscan", key, cursor, arr, pattern, count);

        for (std::size_t i = 0; i < arr.size(); ++i)
        {
            out_result.push_back(arr[i].to_string());
        }

        return next_cursor;
    }
};
}
}

#endif