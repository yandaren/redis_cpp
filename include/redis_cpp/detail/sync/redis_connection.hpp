/**
 *
 * redis_connection.hpp
 *
 * redis command about connection( auth, select, ping, echo, quit)
 *
 * @author  :   yandaren1220@126.com
 * @date    :   2017-04-26
 */

#ifndef __ydk_rediscpp_detail_redis_connection_hpp__
#define __ydk_rediscpp_detail_redis_connection_hpp__

#include <redis_cpp/detail/config.hpp>
#include <redis_cpp/detail/sync/redis_command_executor.hpp>

namespace redis_cpp
{
namespace detail
{
class redis_connection : virtual public redis_command_executor
{
public:
    redis_connection(){

    }

public:
    /**
     * @brief auth the acess with passwd
     * if the server opens the passwd protected, then the redis client need 
     * auth with the passwd first, if auth success, then can use other command
     * @param passwd
     * @return true or false [ success return true]
     */
    bool auth(const char* passwd)
    {
        redis_command cmd("auth");

        cmd.add_param(passwd);

        return check_status_ok(cmd);
    }

    /**
     * @brief echo the given message, for test
     * @param message
     * @param out_message - save the echo message
     */
    void echo(const char* out_message)
    {
        redis_command cmd("echo");

        cmd.add_param(out_message);

        std::string ret(std::move(get_string_result(cmd)));

        printf("%s\n", ret.c_str());
    }

    /**
     * @brief ping cmd, to check the connection is valid, can also use to test 
     * the network delay
     * @param return true or false [ if the connection valid, return true]
     */
    bool ping()
    {
        redis_command cmd("ping");

        std::string ret(std::move(get_string_result(cmd)));

        return stricmp(ret.c_str(), "PONG") == 0;
    }

    /**
     * @brief request the server close the connection with current client
     */
    void quit()
    {
        redis_command cmd("quit");

        do_command(cmd);
    }

    /**
     * @brief switch to specific db
     */
    bool select(int32_t db_number)
    {
        redis_command cmd("select");

        cmd.add_param(db_number);

        reset_hash_slot();

        return check_status_ok(cmd);
    }
};
}
}

#endif