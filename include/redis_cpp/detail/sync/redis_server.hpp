/**
 *
 * redis_server.hpp
 *
 * redis command of server
 *
 * @author  :   yandaren1220@126.com
 * @date    :   2017-04-27
 */

#ifndef __ydk_rediscpp_detail_redis_server_hpp__
#define __ydk_rediscpp_detail_redis_server_hpp__

#include <redis_cpp/detail/config.hpp>
#include <redis_cpp/detail/sync/redis_command_executor.hpp>
#include <redis_cpp/internal/logger_handler.hpp>
#include <vector>

namespace redis_cpp
{
namespace detail
{

struct slave_base_info
{
    std::string ip;
    int32_t     port;
    int32_t     replication_offset;
};

struct master_node_info
{
    int32_t                         replication_offset;
    std::vector<slave_base_info>    slave_list;
};

struct slave_node_info
{
    std::string master_ip;
    int32_t     master_port;
    std::string state;                  // state may be one of "connect", "connecting", "connected"
    int32_t     recved_data_size;       // recved data size
};

struct sentinel_node_info
{
    std::vector<std::string>    master_name_list_monitored;  // 监控的master name list
};

struct node_role_info
{
    std::string                         role;  // role may be one of "master", "slave", "sentinel"
    std::shared_ptr<master_node_info>   master;
    std::shared_ptr<slave_node_info>    slave;
    std::shared_ptr<sentinel_node_info> sentinel;
};

class redis_server : virtual public redis_command_executor
{
public:
    redis_server(){
    }

public:
    /**
    * @brief return server time
    * @param time[seconds] - unix时间戳
    * @param microseconds  - 这一秒逝去的微秒数
    * @return true or false
    */
    bool    time(int64_t* time, int64_t* microseconds)
    {
        redis_command cmd("time");

        hash_slot("time");

        redis_reply_arr arr;
        if (!get_array_reply(cmd, arr))
            return false;

        if (arr.size() != 2)
            return false;

        if (time)
        {
            sscanf(arr[0].to_string().c_str(), "%lld", time);
        }

        if (microseconds)
        {
            sscanf(arr[1].to_string().c_str(), "%lld", microseconds);
        }

        return true;
    }

    /**
    * @brief 是否是master
    */
    bool    check_role_is_master()
    {
        node_role_info r_info;
        return role(r_info) && r_info.role == "master";
    }

    /**
    * @brief get role
    * @param role_name may be one of "master", "slave" and "sentinel"
    */
    bool    role(std::string& role_name)
    {
        node_role_info r_info;
        if (role(r_info))
        {
            role_name = r_info.role;
            return true;
        }
        return false;
    }

    /**
    * @brief brief return the role of the connect server
    * @param role_info - out role info
    * @return true of false
    */
    bool    role(node_role_info& role_info)
    {
        redis_command cmd("role");

        redis_reply_arr arr;
        if (!get_array_reply(cmd, arr))
            return false;

        if (arr.size() < 1)
            return false;

        role_info.role = arr[0].to_string();

        if (role_info.role == "master"){
            return parser_role_master(role_info, arr);
        }
        else if (role_info.role == "slave"){
            return parser_role_slave(role_info, arr);
        }
        else if (role_info.role == "sentinel"){
            return parser_role_sentinel(role_info, arr);
        }

        rds_log_error("role cmd: unsupport role: %s", role_info.role.c_str());
        return false;
    }

protected:
    bool    parser_role_master(node_role_info& role_info, redis_reply_arr& arr){
        if (arr.size() < 2)
            return false;

        role_info.master = std::make_shared<master_node_info>();
        role_info.master->replication_offset = arr[1].to_integer();

        if (arr.size() < 3 || !arr[2].is_array())
            return true;

        redis_reply_arr& slave_info_arr = arr[2].to_array();
        for (std::size_t i = 0; i < slave_info_arr.size(); ++i)
        {
            if (!slave_info_arr[i].is_array())
                continue;

            redis_reply_arr& slave_info = slave_info_arr[i].to_array();
            if (slave_info.size() != 3)
            {
                return false;
            }

            role_info.master->slave_list.resize(role_info.master->slave_list.size() + 1);
            auto& back = role_info.master->slave_list.back();
            back.ip = slave_info[0].to_string();
            std::string& port_str = slave_info[1].to_string();
            sscanf(port_str.c_str(), "%d", &back.port);
            std::string& replication_offset_str = slave_info[2].to_string();
            sscanf(replication_offset_str.c_str(), "%d", &back.replication_offset);
        }

        return true;
    }

    bool    parser_role_slave(node_role_info& role_info, redis_reply_arr& arr){
        if (arr.size() != 5){
            return false;
        }

        role_info.slave = std::make_shared<slave_node_info>();
        role_info.slave->master_ip = arr[1].to_string();
        role_info.slave->master_port = arr[2].to_integer();
        role_info.slave->state = arr[3].to_string();
        role_info.slave->recved_data_size = arr[4].to_integer();
        return true;
    }

    bool    parser_role_sentinel(node_role_info& role_info, redis_reply_arr& arr){

        role_info.sentinel = std::make_shared<sentinel_node_info>();

        if (arr.size() < 2 || !arr[1].is_array())
            return true;

        redis_reply_arr& master_name_list = arr[1].to_array();
        for (std::size_t i = 0; i < master_name_list.size(); ++i)
        {
            role_info.sentinel->master_name_list_monitored.push_back(
                master_name_list[i].to_string());
        }

        return true;
    }
};
}
}

#endif