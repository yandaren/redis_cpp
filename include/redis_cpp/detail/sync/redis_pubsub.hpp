/**
 *
 * redis_pubsub.hpp
 *
 * redis command of pubsub
 *
 * @author  :   yandaren1220@126.com
 * @date    :   2017-04-27
 */

#ifndef __ydk_rediscpp_detail_redis_pubsub_hpp__
#define __ydk_rediscpp_detail_redis_pubsub_hpp__

#include <redis_cpp/detail/config.hpp>
#include <redis_cpp/detail/sync/redis_command_executor.hpp>

namespace redis_cpp
{
namespace detail
{
class redis_pubsub : virtual public redis_command_executor
{
public:
    redis_pubsub(){
    }

public:
    /**
    * @brief list current active channels
    * @param out_channels - save the result
    * @param pattern - match mode
    * @return true or false
    */
    bool pubsub_channels(std::vector<std::string>& out_channels, 
        const char* pattern = nullptr)
    {
        redis_command cmd("pubsub");

        cmd.add_param("channels");

        if (pattern && *pattern)
        {
            cmd.add_param(pattern);
        }

        reset_hash_slot();

        return get_array_result(cmd, out_channels);
    }

    /**
    * @brief get subscriber count of specific channel
    * @param channel_names
    * @param subscriber_count - save the result
    * @return true or false
    */
    bool pubsub_numsub(const char* channel_name, int32_t& out_subscriber_count)
    {
        redis_command cmd("pubsub");

        out_subscriber_count = 0;

        cmd.add_param("numsub");
        cmd.add_param(channel_name);

        reset_hash_slot();

        redis_reply_arr arr;
        if (!get_array_reply(cmd, arr))
            return false;

        if (arr.size() % 2 != 0)
        {
            return false;
        }

        if (arr.size() == 2)
        {
            out_subscriber_count = arr[1].to_integer();
        }

        return true;
    }

    /**
    * @brief get subscriber count of specific channel
    * @param channels - channel list
    * @param out_result - save the result
    * @return true or false
    */
    bool pubsub_numsub(const std::vector < std::string>& channels, 
        std::unordered_map<std::string, int32_t>& out_result)
    {
        redis_command cmd("pubsub");

        cmd.add_param("numsub");

        for (std::size_t i = 0; i < channels.size(); ++i)
        {
            cmd.add_param(channels[i]);
        }

        reset_hash_slot();

        redis_reply_arr arr;
        if (!get_array_reply(cmd, arr))
            return false;

        if (arr.size() % 2 != 0)
        {
            return false;
        }

        for (std::size_t i = 0; i < arr.size(); i += 2)
        {
            int32_t sub_num = 0;
            sscanf(arr[i+1].to_string().c_str(), "%d", &sub_num);

            out_result.insert(
                std::move(std::pair<std::string, int32_t>(
                std::move(arr[i].to_string()), sub_num)));
        }
        return true;
    }

    /**
    * @brief 获取订阅模式的数量
    * 注意，该命令返回的不是订阅模式的客户端的数量，而是客户端订阅的所有模式的数量总和
    */
    int32_t pubsub_numpat()
    {
        redis_command cmd("pubsub");

        cmd.add_param("numpat");

        reset_hash_slot();

        return get_integer_result(cmd);
    }

    /**
    * @brief 发布消息
    * @param channel
    * @param message
    * @return the count of the subscriber which get the message
    */
    int32_t publish(const char* channel_name, const char* message)
    {
        redis_command cmd("publish");

        cmd.add_param(channel_name);
        cmd.add_param(message);

        reset_hash_slot();

        return get_integer_result(cmd);
    }

    /**
    * @brief 发布消息
    * @param channel
    * @param message
    * @param message_len
    * @return the count of the subscriber which get the message
    */
    int32_t publish(const char* channel_name, 
        const char* message, int32_t message_len)
    {
        redis_command cmd("publish");

        cmd.add_param(channel_name);
        cmd.add_param(message, message_len);

        reset_hash_slot();

        return get_integer_result(cmd);
    }
};
}
}

#endif