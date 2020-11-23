/**
 *
 * redis_list.hpp
 *
 * redis command of list
 *
 * @author  :   yandaren1220@126.com
 * @date    :   2017-04-27
 */

#ifndef __ydk_rediscpp_detail_redis_list_hpp__
#define __ydk_rediscpp_detail_redis_list_hpp__

#include <redis_cpp/detail/config.hpp>
#include <redis_cpp/detail/sync/redis_command_executor.hpp>

namespace redis_cpp
{
namespace detail
{
class redis_list : virtual public redis_command_executor
{
public:
    redis_list(){

    }

public:
    /**
    * @brief left pop the element from the list,
    * if the list is empty, then this cmd will block the connection util this 
    * list not empty any more or wait time out, if given muti keys, 
    * then the cmd will check the keys in order, and pop the first 
    * not empy list's first element
    * @param key
    * @param time_out (in seconds), 0 wait forerver
    * @return true of false
    */
    bool blpop(const std::string& key, 
        int32_t time_out, std::pair<std::string, std::string>& result)
    {
        redis_command cmd("blpop");

        cmd.add_param(key);
        cmd.add_param(time_out);

        hash_slot(key.c_str(), (int32_t)key.size());

        return get_string_pair_result(cmd, result);
    }

    /**
    * @brief left pop the element from the list,
    * if the list is empty, then this cmd will block the connection util this
    * list not empty any more or wait time out, if given muti keys,
    * then the cmd will check the keys in order, and pop the first 
    * not empy list's first element
    * @param keys
    * @param time_out (in seconds), 0 wait forerver
    * @return true of false
    */
    bool blpop(const std::vector<std::string>& keys, 
        int32_t time_out, std::pair<std::string, std::string>& result)
    {
        redis_command cmd("blpop");

        for (std::size_t i = 0; i < keys.size(); ++i)
        {
            cmd.add_param(keys[i]);
        }
        cmd.add_param(time_out);

        reset_hash_slot();

        return get_string_pair_result(cmd, result);
    }

    /**
    * @brief right pop the element from the list, like blpop
    * @param key
    * @param time_out (in seconds), 0 wait forerver
    * @return true of false
    */
    bool brpop(const std::string& key, 
        int32_t time_out, std::pair<std::string, std::string>& result)
    {
        redis_command cmd("brpop");

        cmd.add_param(key);
        cmd.add_param(time_out);

        hash_slot(key.c_str(), (int32_t)key.size());

        return get_string_pair_result(cmd, result);
    }

    /**
    * @brief right pop the element from the list, like blpop
    * @param keys
    * @param time_out (in seconds), 0 wait forerver
    * @return true of false
    */
    bool brpop(const std::vector<std::string>& keys, 
        int32_t time_out, std::pair<std::string, std::string>& result)
    {
        redis_command cmd("brpop");

        for (std::size_t i = 0; i < keys.size(); ++i)
        {
            cmd.add_param(keys[i]);
        }
        cmd.add_param(time_out);

        reset_hash_slot();

        return get_string_pair_result(cmd, result);
    }

    /**
    * @brief the blocking version if brpoplpush
    * if the source list not empty, the cmd is just same as brpoplpush
    * otherwise it will block the connection only the source empty not empty 
    * any more or wait time out
    * @param source_list
    * @param dest_list
    * @param time_out (in seconds)
    * @param out_value - save the poped element(or nil) and waited time
    * @return true of false
    */
    bool brpoplpush(const char* source_list, 
        const char* dest_list, 
        int32_t time_out, std::pair<std::string, std::string>* result)
    {
        redis_command cmd("brpoplpush");

        cmd.add_param(source_list);
        cmd.add_param(dest_list);
        cmd.add_param(time_out);

        reset_hash_slot();

        return get_string_pair_result(cmd, *result);
    }

    /**
    * @brief return the element with specific index
    * @param key
    * @param index
    * @param out_value - save the result
    * @return true of false
    */
    bool lindex(const char* key, int32_t index, std::string& out_value)
    {
        redis_command cmd("lindex");

        cmd.add_param(key);
        cmd.add_param(index);

        hash_slot(key);

        return get_string_result(cmd, out_value);
    }

    /**
    * @brief insert element before specific value[pivot]
    * @param key
    * @param pivot
    * @param value
    * @return -2 if some error happend
    return -1 if can't find pivot
    return 0  if key not exist
    return list length after insert succss
    */
    int32_t insert_before(const char* key, const char* pivot, const char* value)
    {
        redis_command cmd("linsert");

        static const char* before_dict = "before";
        cmd.add_param(key);
        cmd.add_param(before_dict);
        cmd.add_param(pivot);
        cmd.add_param(value);

        hash_slot(key);

        return (int32_t)get_integer_result(cmd);
    }

    /**
    * @brief insert element after specific value[pivot]
    * @param key
    * @param pivot
    * @param value
    * @return -2 if some error happend
    return -1 if can't find pivot
    return 0  if key not exist
    return list length after insert succss
    */
    int32_t insert_after(const char* key, const char* pivot, const char* value)
    {
        redis_command cmd("linsert");

        static const char* after_dict = "after";
        cmd.add_param(key);
        cmd.add_param(after_dict);
        cmd.add_param(pivot);
        cmd.add_param(value);

        hash_slot(key);

        return (int32_t)get_integer_result(cmd);
    }

    /**
    * @brief get the length of the list
    * @param key
    * @return the length of the list
    */
    int32_t llen(const char* key)
    {
        redis_command cmd("llen");

        cmd.add_param(key);

        hash_slot(key);

        return (int32_t)get_integer_result(cmd);
    }

    /**
    * @brief left pop and return the head element
    * @param key
    * @param out_value -save the result
    */
    bool lpop(const char* key, std::string* out_value)
    {
        redis_command cmd("lpop");

        cmd.add_param(key);

        hash_slot(key);

        return get_string_result(cmd, out_value);
    }

    /**
    * @brief left push one or muti value to the list head
    * @param key
    * @param value
    * @return return the list length after push
    */
    int32_t lpush(const char* key, const char* value)
    {
        redis_command cmd("lpush");

        cmd.add_param(key);
        cmd.add_param(value);

        hash_slot(key);

        return (int32_t)get_integer_result(cmd);
    }

    /**
    * @brief left push one or muti value to the list head
    * @param key
    * @param value
    * @return return the list length after push
    */
    int32_t lpush(const char* key, const char* value, int32_t value_len)
    {
        redis_command cmd("lpush");

        cmd.add_param(key);
        cmd.add_param(value, value_len);

        hash_slot(key);

        return (int32_t)get_integer_result(cmd);
    }

    /**
    * @brief left push one or muti value to the list head
    * @param key
    * @param values
    * @return return the list length after push
    */
    int32_t lpush(const char* key, const std::vector<std::string>& values)
    {
        redis_command cmd("lpush");

        cmd.add_param(key);

        hash_slot(key);

        return (int32_t)get_integer_result(cmd, values);
    }

    /**
    * @brief left push value to the list head
    * success only when the key exist and it's a list
    * @param key
    * @param value
    * @return the new list length
    */
    int32_t lpushx(const char* key, const char* value)
    {
        redis_command cmd("lpushx");

        cmd.add_param(key);
        cmd.add_param(value);

        hash_slot(key);

        return (int32_t)get_integer_result(cmd);
    }

    /**
    * @brief get the elements at the specific section
    * @param key
    * @param start [ index start at 0]
    * @param ends  [ can use negtive index, 
    *               -1 means the last element, 
    *               -2 means the last second elements, and so on
    * @param out_list - save the result
    * @return true of false
    */
    bool lrange(const char* key, 
        int32_t start, int32_t end, std::vector<std::string>& result)
    {
        redis_command cmd("lrange");

        cmd.add_param(key);
        cmd.add_param(start);
        cmd.add_param(end);

        hash_slot(key);

        return get_array_result(cmd, result);
    }

    /**
    * @brief remove the element which equal the specific value according the 'count' value
    * @param key
    * @param count - if count > 0, remove from left to right, util the removed count equal count
    *              - if count < 0, remove from right to left, util the removed count equal -count
    *              - if count = 0, remove all the element which equal 'value'
    * @return the removed element count
    */
    int32_t lrem(const char* key, int32_t count, const char* value)
    {
        redis_command cmd("lrem");

        cmd.add_param(key);
        cmd.add_param(count);
        cmd.add_param(value);

        hash_slot(key);

        return (int32_t)get_integer_result(cmd);
    }

    /**
    * @brief set the value of specific index as 'value'
    * @param key
    * @param value
    * return true of false
    */
    bool lset(const char* key, int32_t index, const char* value)
    {
        redis_command cmd("lset");

        cmd.add_param(key);
        cmd.add_param(index);
        cmd.add_param(value);

        hash_slot(key);

        return check_status_ok(cmd);
    }

    /**
    * @brief trim the list, only keep the elements in the given section
    * @param key
    * @param start [ support negtive index]
    * @param end [ support negtive index]
    * @return true or false
    */
    bool ltrim(const char* key, int32_t start, int32_t end)
    {
        redis_command cmd("ltrim");

        cmd.add_param(key);
        cmd.add_param(start);
        cmd.add_param(end);

        hash_slot(key);

        return check_status_ok(cmd);
    }

    /**
    * @brief pop and return the tail element
    * @param key
    * @param out_value - save the result
    * @return true of false
    */
    bool rpop(const char* key, std::string* out_value)
    {
        redis_command cmd("rpop");

        cmd.add_param(key);

        hash_slot(key);

        return get_string_result(cmd, out_value);
    }

    /**
    * @brief rpoplpush atomic do two operate
    * pop the return the tail element of the 'source' list, and push the element
    * to the 'dest' list head
    * @param source_list
    * @param dest_list
    * @param out_value - save the poped element
    * @return true or false
    */
    bool rpoplpush(const char* source_list, 
        const char* dest_list, std::string* out_value)
    {
        redis_command cmd("rpoplpush");

        cmd.add_param(source_list);
        cmd.add_param(dest_list);

        reset_hash_slot();

        return get_string_result(cmd, out_value);
    }

    /**
    * @brief push one value to the list tail
    * @param key
    * @param value
    * @return the new list length
    */
    int32_t rpush(const char* key, const char* value)
    {
        redis_command cmd("rpush");

        cmd.add_param(key);
        cmd.add_param(value);

        hash_slot(key);

        return (int32_t)get_integer_result(cmd);
    }

    /**
    * @brief push one value to the list tail
    * @param key
    * @param value
    * @return the new list length
    */
    int32_t rpush(const char* key, const char* value, int32_t value_size)
    {
        redis_command cmd("rpush");

        cmd.add_param(key);
        cmd.add_param(value, value_size);

        hash_slot(key);

        return (int32_t)get_integer_result(cmd);
    }

    /**
    * @brief push muti value to the list tail
    * @param key
    * @param value
    * @return the new list length
    */
    int32_t rpush(const char* key, const std::vector<std::string>& values)
    {
        redis_command cmd("rpush");

        cmd.add_param(key);

        hash_slot(key);

        return (int32_t)get_integer_result(cmd, values);
    }

    /**
    * @brief push the value to the list tail
    * success only if the key exist and it's a list
    * @param key
    * @param value
    * @return the new length after push
    */
    int32_t rpushx(const char* key, const char* value)
    {
        redis_command cmd("rpushx");

        cmd.add_param(key);
        cmd.add_param(value);

        hash_slot(key);

        return (int32_t)get_integer_result(cmd);
    }
};
}
}

#endif