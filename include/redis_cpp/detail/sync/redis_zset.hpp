/**
 *
 * redis_zset.hpp
 *
 * redis command of ordered set
 *
 * @author  :   yandaren1220@126.com
 * @date    :   2017-04-27
 */

#ifndef __ydk_rediscpp_detail_redis_zset_hpp__
#define __ydk_rediscpp_detail_redis_zset_hpp__

#include <redis_cpp/detail/config.hpp>
#include <redis_cpp/detail/sync/redis_command_executor.hpp>

namespace redis_cpp
{
namespace detail
{
class redis_zset : virtual public redis_command_executor
{
public:
    redis_zset(){
    }

public:
    /**
    * @brief add one member to the set
    * @param key
    * @param score
    * @param value
    * @return the added count
    */
    int32_t zadd(const char* key, double score, const char* value)
    {
        redis_command cmd("zadd");

        cmd.add_param(key);
        cmd.add_param(score);
        cmd.add_param(value);

        hash_slot(key);

        return (int32_t)get_integer_result(cmd);
    }

    /**
    * @brief add muti member to the set
    * @param key
    * @param sv_pair
    * @return the added count
    */
    int32_t zadd(const char* key, 
        std::vector <std::pair<double, std::string>>& sv_pairs)
    {
        redis_command cmd("zadd");

        cmd.add_param(key);

        for (std::size_t i = 0; i < sv_pairs.size(); ++i)
        {
            cmd.add_param(sv_pairs[i].first);
            cmd.add_param(sv_pairs[i].second);
        }

        hash_slot(key);

        return (int32_t)get_integer_result(cmd);
    }

    /**
    * @brief get the element count of the set
    * @param key
    * @return the count
    */
    int32_t zcard(const char* key)
    {
        redis_command cmd("zcard");

        cmd.add_param(key);

        hash_slot(key);

        return (int32_t)get_integer_result(cmd);
    }

    /**
    * @brief get element count which the score between 'min' and 'max'
    * @param key
    * @param min
    * @param max
    * @param return the count
    */
    int32_t zcount(const char* key, double min, double max)
    {
        redis_command cmd("zcount");

        cmd.add_param(key);
        cmd.add_param(min);
        cmd.add_param(max);

        hash_slot(key);

        return (int32_t)get_integer_result(cmd);
    }

    /**
    * @brief get element which score in specific section[index section]
    * @param key
    * @param start - [start index]
    * @param stop - [end index]
    * @param out_result - save the result [ the result sort by score]
    * @return true or false
    */
    bool zrange(const char* key, 
        int32_t start, int32_t end, std::vector<std::string>& out_result)
    {
        return z_range("zrange", key, start, end, out_result);
    }

    /**
    * @brief get element which score in specific section[index section], with score
    * @param key
    * @param start - [start index]
    * @param stop - [end index]
    * @param out_result - save the result [ the result sort by score]
    * @return true or false
    */
    bool zrangewithscores(const char* key, 
        int32_t start, 
        int32_t end, 
        std::vector<std::pair<std::string, double>>& out_result)
    {
        return z_rangewithscores("zrange", key, start, end, out_result);
    }

    /**
    * @brief get the element by score section
    * @param key
    * @param min_score
    * @param max_score
    * @param out_result - save the result [ the result sort by score]
    * @param offset
    * @param count
    * @return true or false
    */
    bool zrangebyscore(const char* key, 
        double min_score, 
        double max_score, 
        std::vector<std::string>& out_result,
        const int32_t* offset = nullptr, int32_t* count = nullptr)
    {
        return z_rangebyscore("zrangebyscore", 
            key, min_score, max_score, out_result, offset, count);
    }


    /**
    * @brief get the element by score section witch score
    * @param key
    * @param min_score
    * @param max_score
    * @param out_result - save the result [ the result sort by score]
    * @param offset
    * @param count
    * @return true or false
    */
    bool zrangebyscorewithscore(const char* key, 
        double min_score, 
        double max_score, 
        std::vector<std::pair<std::string, double>>& out_result,
        const int32_t* offset = nullptr, int32_t* count = nullptr)
    {
        return z_rangebyscorewithscores("zrangebyscore", 
            key, min_score, max_score, out_result, offset, count);
    }

    /**
    * @brief return the rank of the memeber
    * @param key
    * @param value
    * @param out_rank - save the result
    * @return true or false
    */
    bool zrank(const char* key, const char* value, int32_t& out_rank)
    {
        return z_rank("zrank", key, value, out_rank);
    }

    //////////////////
    /**
    * @brief get element which score in specific section[index section]
    * @param key
    * @param start - [start index]
    * @param stop - [end index]
    * @param out_result - save the result [ the result sort by score desc]
    * @return true or false
    */
    bool zrevrange(const char* key, 
        int32_t start, int32_t end, std::vector<std::string>& out_result)
    {
        return z_range("zrevrange", key, start, end, out_result);
    }

    /**
    * @brief get element which score in specific section[index section], with score
    * @param key
    * @param start - [start index]
    * @param stop - [end index]
    * @param out_result - save the result [ the result sort by score desc]
    * @return true or false
    */
    bool zrevrangewithscores(const char* key, 
        int32_t start, 
        int32_t end, 
        std::vector<std::pair<std::string, double>>& out_result)
    {
        return z_rangewithscores("zrevrange", key, start, end, out_result);
    }

    /**
    * @brief get the element by score section
    * @param key
    * @param max_score
    * @param min_score
    * @param out_result - save the result [ the result sort by score desc]
    * @param offset
    * @param count
    * @return true or false
    */
    bool zrevrangebyscore(const char* key, 
        double max_score, 
        double min_score, 
        std::vector<std::string>& out_result,
        const int32_t* offset = nullptr, int32_t* count = nullptr)
    {
        return z_rangebyscore("zrevrangebyscore", 
            key, max_score, min_score, out_result, offset, count);
    }


    /**
    * @brief get the element by score section witch score
    * @param key
    * @param max_score
    * @param min_score
    * @param out_result - save the result [ the result sort by score desc]
    * @param offset
    * @param count
    * @return true or false
    */
    bool zrevrangebyscorewithscore(const char* key, 
        double max_score, 
        double min_score, 
        std::vector<std::pair<std::string, double>>& out_result,
        const int32_t* offset = nullptr, int32_t* count = nullptr)
    {
        return z_rangebyscorewithscores("zrevrangebyscore", 
            key, max_score, min_score, out_result, offset, count);
    }

    /**
    * @brief return the rank of the memeber
    * @param key
    * @param value
    * @param out_rank - save the result [sort by score desc]
    * @return true or false
    */
    bool zrevrank(const char* key, const char* value, int32_t& out_rank)
    {
        return z_rank("zrevrank", key, value, out_rank);
    }

    /**
    * @brief get the score of specific value
    * @param key
    * @param member
    * @param out_value - [string]
    * @return true or false
    */
    bool zscore(const char* key, const char* value, std::string& out_score)
    {
        redis_command cmd("zscore");

        cmd.add_param(key);
        cmd.add_param(value);

        hash_slot(key);

        return get_string_result(cmd, out_score);
    }

    /**
    * @brief increase the member's score by 'increment'
    * @param key
    * @param increment [ int32 ]
    * @param value
    * @param out_new_score - save the new value
    * @return true or false
    */
    bool zincrby(const char* key, 
        int32_t increment, const char* value, std::string* out_new_score = nullptr)
    {
        char increment_str[64] = { 0 };
        sprintf(increment_str, "%d", increment);
        return z_incrby(key, increment_str, value, out_new_score);
    }

    /**
    * @brief increase the member's score by 'increment'
    * @param key
    * @param increment [double]
    * @param value
    * @param out_new_value - save the new score
    * @return true or false
    */
    bool zincrby(const char* key, 
        double increment, const char* value, std::string* out_new_score = nullptr)
    {
        char increment_str[64] = { 0 };
        sprintf(increment_str, "%f", increment);
        return z_incrby(key, increment_str, value, out_new_score);
    }

    /**
    * @brief remove one member from the set
    * @param key
    * @param value
    * @return the removed count
    */
    int32_t zrem(const char* key, const char* value)
    {
        redis_command cmd("zrem");

        cmd.add_param(key);
        cmd.add_param(value);

        hash_slot(key);

        return (int32_t)get_integer_result(cmd);
    }

    /**
    * @brief remove muti member from the set
    * @param key
    * @param values
    * @return te removed count
    */
    int32_t zrem(const char* key, const std::vector<std::string>& values)
    {
        redis_command cmd("zrem");

        cmd.add_param(key);

        hash_slot(key);

        return (int32_t)get_integer_result(cmd, values);
    }

    /**
    * @brief remove range by rank
    * remove all the member that rank in section[start, end]
    * @param key
    * @param start
    * @param end
    * @return the removed count
    */
    int32_t zremrangebyrank(const char* key, int32_t start_rank, int32_t end_rank)
    {
        redis_command cmd("zremrangebyrank");

        cmd.add_param(key);
        cmd.add_param(start_rank);
        cmd.add_param(end_rank);

        hash_slot(key);

        return (int32_t)get_integer_result(cmd);
    }

    /**
    * @brief remove range by score
    * remove all the rember which score in section[ min, max]
    * @param key
    * @param min - double
    * @param max - double
    * @return the removed count
    */
    int32_t zremrangebyscore(const char* key, double min, double max)
    {
        redis_command cmd("zremrangebyscore");

        cmd.add_param(key);
        cmd.add_param(min);
        cmd.add_param(max);

        hash_slot(key);

        return (int32_t)get_integer_result(cmd);
    }

    /**
    * @brief remove range by lex
    * used for the set that all the member has the same score, the the cmd will 
    * remove all the member that in section[min, max] ( sorted by 字典序)
    * @param key
    * @param min - [ string ], 必须以 '(' 或者 '[' 开头，分别表示开区间和闭区间
    * @param max - [ string ], 必须以 '(' 或者 '[' 开头，分别表示开区间和闭区间
    * @return the removed count
    */
    int32_t zremrangebylex(const char* key, 
        const char* min_member, const char* max_member)
    {
        redis_command cmd("zremrangebylex");

        cmd.add_param(key);
        cmd.add_param(min_member);
        cmd.add_param(max_member);

        hash_slot(key);

        return (int32_t)get_integer_result(cmd);
    }

    /**
    * @brief used for the set that all the member has the same score, the cmd 
    * will return the member count that in section[min, max]
    * @param key
    * @param min - [ string],必须以 '(' 或者 '[' 开头，分别表示开区间和闭区间
    * @param max - [ string],必须以 '(' 或者 '[' 开头，分别表示开区间和闭区间
    * @return the count
    */
    int32_t zlexcount(const char* key, const char* min_member, const char* max_member)
    {
        redis_command cmd("zlexcount");

        cmd.add_param(key);
        cmd.add_param(min_member);
        cmd.add_param(max_member);

        hash_slot(key);

        return (int32_t)get_integer_result(cmd);
    }

    /**
    * @brief used for the set that all the member has the same score, the cmd 
    * will return the member that in section[min, max]
    * @param key
    * @param min - [ string], 必须以 '(' 或者 '[' 开头，分别表示开区间和闭区间
    * @param max - [ string], 必须以 '(' 或者 '[' 开头，分别表示开区间和闭区间
    * @param out_result - save the result
    * @param offset - 偏移量限制
    * @param count - 返回的数量限制
    * @return true or false
    */
    bool zrangebylex(const char* key, 
        const char* min_member, 
        const char* max_member, 
        std::vector<std::string>& out_result,
        const int32_t* offset = nullptr, const int32_t* count = nullptr)
    {
        redis_command cmd("zrangebylex");

        cmd.add_param(key);
        cmd.add_param(min_member);
        cmd.add_param(max_member);

        if (offset && count)
        {
            cmd.add_param("limit");
            cmd.add_param(*offset);
            cmd.add_param(*count);
        }

        hash_slot(key);

        return get_array_result(cmd, out_result);
    }

    /**
    * @brief zunionstore save the union of muti set to the dest_set
    * @param dest_set
    * @param keys [ set-weight pair list ]
    * @param aggregate [ SUM|MIN|MAX ]
    * @return the result set size
    */
    int32_t zunionstore(const char* dest_set, 
        const std::unordered_map<std::string, double>& keys, 
        const char* aggregate = "SUM")
    {
        return z_store("zunionstore", dest_set, keys, aggregate);
    }

    /**
    * @brief zunionstore save the union of muti set to the dest_set
    * @param dest_set
    * @param keys
    * @param weights
    * @param aggregate [ SUM|MIN|MAX ]
    * @return the result set size
    */
    int32_t zunionstore(const char* dest_set, 
        const std::vector<std::string>& keys, 
        const std::vector<double>* weights, 
        const char* aggregate = "SUM")
    {
        return z_store("zunionstore", dest_set, keys, weights, aggregate);
    }

    /**
    * @brief zinterstore save the intersecion of muti set to the dest_set
    * @param dest_set
    * @param keys [ set-weight pair list ]
    * @param aggregate [ SUM|MIN|MAX ]
    * @return the result set size
    */
    int32_t zinterstore(const char* dest_set, 
        const std::unordered_map<std::string, double>& keys, 
        const char* aggregate = "SUM")
    {
        return z_store("zinterstore", dest_set, keys, aggregate);
    }

    /**
    * @brief zinterstore save the intersecion of muti set to the dest_set
    * @param dest_set
    * @param keys
    * @param weights
    * @param aggregate [ SUM|MIN|MAX ]
    * @return the result set size
    */
    int32_t zinterstore(const char* dest_set, 
        const std::vector<std::string>& keys, 
        const std::vector<double>* weights, 
        const char* aggregate = "SUM")
    {
        return z_store("zinterstore", dest_set, keys, weights, aggregate);
    }

    /**
    * @brief zscan like scan
    * @param key
    * @param cursor
    * @param out_result - save the resutl
    * @param pattern
    * @param count
    * @return the next cursor
    */
    uint64_t zscan(const char* key, 
        uint64_t cursor, 
        std::vector<std::pair<std::string, double>>& out_result,
        const char* pattern = nullptr, const int32_t* count = nullptr)
    {
        redis_reply_arr arr;
        uint64_t next_cursor = scan_key("zscan", key, cursor, arr, pattern, count);

        if (arr.size() % 2 != 0)
            return 0;

        for (std::size_t i = 0; i < arr.size(); i += 2)
        {
            out_result.push_back(
                std::move(std::pair<std::string, double>(
                arr[i].to_string(), atof(arr[i + 1].to_string().c_str()))));
        }

        return next_cursor;
    }

protected:

    /**
    * @brief get result with score from array reply
    */
    bool get_result_with_scores(redis_reply_arr& arr, 
        std::vector<std::pair<std::string, double>>& out_result)
    {
        if (arr.size() % 2 != 0)
            return false;

        for (std::size_t i = 0; i < arr.size(); i += 2)
        {
            out_result.push_back(
                std::move(std::pair<std::string, double>(
                arr[i].to_string(), atof(arr[i+1].to_string().c_str()))));
        }
        return true;
    }

    /**
    * @brief get element which score in specific section[index section]
    * @param range_cmd
    * @param key
    * @param start - [start index]
    * @param stop - [end index]
    * @param out_result - save the result
    * @return true or false
    */
    bool z_range(const char* range_cmd, 
        const char* key, 
        int32_t start, 
        int32_t end, 
        std::vector<std::string>& out_result)
    {
        redis_command cmd(range_cmd);

        cmd.add_param(key);
        cmd.add_param(start);
        cmd.add_param(end);

        hash_slot(key);

        return get_array_result(cmd, out_result);
    }

    /**
    * @brief get element which score in specific section[index section], with score
    * @param rangedbyscore_cmd
    * @param key
    * @param start - [start index]
    * @param stop - [end index]
    * @param out_result - save the result
    * @return true or false
    */
    bool z_rangewithscores(const char* rangedbyscore_cmd, 
        const char* key, 
        int32_t start, 
        int32_t end, 
        std::vector<std::pair<std::string, double>>& out_result)
    {
        redis_command cmd(rangedbyscore_cmd);

        cmd.add_param(key);
        cmd.add_param(start);
        cmd.add_param(end);
        cmd.add_param("withscores");

        hash_slot(key);

        redis_reply_arr arr;
        if (get_array_reply(cmd, arr))
        {
            return get_result_with_scores(arr, out_result);
        }

        return false;
    }

    /**
    * @brief get the element by score section
    * @param rangedbyscore_cmd
    * @param key
    * @param min_score
    * @param max_score
    * @param out_result - save the result
    * @param offset
    * @param count
    * @return true or false
    */
    bool z_rangebyscore(const char* rangedbyscore_cmd, 
        const char* key, 
        double min_score, 
        double max_score, 
        std::vector<std::string>& out_result,
        const int32_t* offset = nullptr, int32_t* count = nullptr)
    {
        redis_command cmd(rangedbyscore_cmd);

        cmd.add_param(key);
        cmd.add_param(min_score);
        cmd.add_param(max_score);

        hash_slot(key);

        if (offset && count)
        {
            cmd.add_param("limit");
            cmd.add_param(*offset);
            cmd.add_param(*count);
        }

        return get_array_result(cmd, out_result);
    }


    /**
    * @brief get the element by score section witch score
    * @param rangebyscore_cmd
    * @param key
    * @param start_score
    * @param end_score
    * @param out_result - save the result
    * @param offset
    * @param count
    * @return true or false
    */
    bool z_rangebyscorewithscores(const char* rangebyscore_cmd, 
        const char* key, 
        double start_score, 
        double end_score, 
        std::vector<std::pair<std::string, double>>& out_result,
        const int32_t* offset = nullptr, int32_t* count = nullptr)
    {
        redis_command cmd(rangebyscore_cmd);

        cmd.add_param(key);
        cmd.add_param(start_score);
        cmd.add_param(end_score);
        cmd.add_param("withscores");

        if (offset && count)
        {
            cmd.add_param("limit");
            cmd.add_param(*offset);
            cmd.add_param(*count);
        }

        hash_slot(key);

        redis_reply_arr arr;
        if (get_array_reply(cmd, arr))
        {
            return get_result_with_scores(arr, out_result);
        }

        return false;
    }

    /**
    * @brief return the rank of the memeber
    * @param rank_cmd
    * @param key
    * @param value
    * @param out_rank - save the result
    * @return true or false
    */
    bool z_rank(const char* rank_cmd, 
        const char* key, const char* value, int32_t& out_rank)
    {
        redis_command cmd(rank_cmd);

        cmd.add_param(key);
        cmd.add_param(value);

        hash_slot(key);

        return get_integer_result(cmd, &out_rank);
    }

    /**
    * @brief increase the member by 'increment'
    * @param key
    * @param increment [str]
    * @param value
    * @param out_new_score - save the new score
    * @return true or false
    */
    bool z_incrby(const char* key, 
        const char* increment, const char* value, std::string* out_new_score)
    {
        redis_command cmd("zincrby");

        cmd.add_param(key);
        cmd.add_param(increment);
        cmd.add_param(value);

        hash_slot(key);

        return get_string_result(cmd, out_new_score);
    }

    /**
    * @brief
    * @param zstore_cmd
    * @param dest_set
    * @param keys [ set-weight pair list ]
    * @param aggregate [ SUM|MIN|MAX ]
    * @return the result set size
    */
    int32_t z_store(const char* zstore_cmd, 
        const char* dest_set, 
        const std::unordered_map<std::string, double>& keys, 
        const char* aggregate)
    {
        redis_command cmd(zstore_cmd);

        cmd.add_param(dest_set);
        cmd.add_param(keys.size());

        auto iter = keys.begin();
        auto iter_end = keys.end();
        for (iter; iter != iter_end; ++iter)
        {
            cmd.add_param(iter->first);
        }

        if (keys.size() > 0)
        {
            cmd.add_param("weights");
        }

        iter = keys.begin();
        for (iter; iter != iter_end; ++iter)
        {
            cmd.add_param(iter->second);
        }

        if (aggregate)
        {
            cmd.add_param("aggregate");
            cmd.add_param(aggregate);
        }

        reset_hash_slot();

        return (int32_t)get_integer_result(cmd);

    }

    /**
    * @brief
    * @param zstore_cmd
    * @param dest_set
    * @param keys
    * @param weights
    * @param aggregate [ SUM|MIN|MAX ]
    * @return the result set size
    */
    int32_t z_store(const char* zstore_cmd, 
        const char* dest_set, 
        const std::vector<std::string>& keys, 
        const std::vector<double>* weights, const char* aggregate)
    {
        redis_command cmd(zstore_cmd);

        cmd.add_param(dest_set);
        cmd.add_param(keys.size());

        for (std::size_t i = 0; i < keys.size(); ++i)
        {
            cmd.add_param(keys[i]);
        }

        if (weights && weights->size() > 0)
        {
            cmd.add_param("weights");
            for (std::size_t i = 0; i < weights->size(); ++i)
            {
                cmd.add_param((*weights)[i]);
            }
        }

        if (aggregate)
        {
            cmd.add_param("aggregate");
            cmd.add_param(aggregate);
        }

        reset_hash_slot();

        return (int32_t)get_integer_result(cmd);

    }
};
}
}

#endif