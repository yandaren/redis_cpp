/**
 *
 * redis_uri.hpp
 *
 * the redis connection uri
 *
 * @author  :   yandaren1220@126.com
 * @date    :   2017-04-22
 */

#ifndef __ydk_rediscpp_redis_uri_hpp__
#define __ydk_rediscpp_redis_uri_hpp__

#include <redis_cpp/detail/config.hpp>
#include <redis_cpp/utils/ip_utils.hpp>
#include <cstdint>
#include <sstream>

namespace redis_cpp
{

static const std::string uri_prefix = "redis://";

class redis_uri
{
protected:
    std::string ip_;        // remote ip
    int32_t     port_;      // remote port
    std::string passwd_;    // auth passwd
    int32_t     dbnum_;     // the database number

    /** 
     * @brief parse the uri
     */
    void parse_uri(const std::string& uri)
    {
        std::size_t pos = uri.find(uri_prefix);
        if (pos == std::string::npos)
            return;

        pos += uri_prefix.size();
        std::size_t passwd_pos = uri.rfind("@");
        if (passwd_pos == std::string::npos)
            return;

        passwd_ = uri.substr(pos, passwd_pos - pos);

        std::string hostname = "localhost";
        std::size_t host_pos = uri.find(":", passwd_pos);
        if (host_pos != std::string::npos)
        {
            hostname = uri.substr(passwd_pos + 1, host_pos - passwd_pos - 1);

            std::size_t port_pos = uri.find("/", host_pos);
            if (port_pos != std::string::npos)
            {
                port_ = atoi(uri.substr(host_pos + 1, port_pos - host_pos).c_str());
                dbnum_ = atoi(uri.substr(port_pos + 1, uri.size() - port_pos).c_str());
            }
            else
            {
                port_ = atoi(uri.substr(host_pos + 1, uri.size() - host_pos).c_str());
            }
        }
        else
        {
            host_pos = uri.find("/", passwd_pos);
            if (host_pos != std::string::npos){
                hostname = uri.substr(passwd_pos + 1, host_pos - passwd_pos - 1);
                dbnum_ = atoi(uri.substr(host_pos + 1, uri.size() - host_pos).c_str());
            }
            else{
                hostname = uri.substr(passwd_pos + 1, uri.size() - passwd_pos - 1);
            }
        }

        std::vector<std::string> ip_list = ip_utils::v4::get_ip_list(hostname.c_str());
        if (!ip_list.empty()) {
            ip_ = ip_list[0];
        }
        else {
            ip_ = "127.0.0.1";
        }
    }

public:
    redis_uri()
        : ip_("127.0.0.1")
        , port_(6379)
        , passwd_("")
        , dbnum_(0)
    {
    }

    /** 
     * @brief uri like "redis://foobared@localhost:6380/2"
     * format is "redis://passwd@ip:port/dbnum"
     */
    redis_uri(const char* uri)
        : ip_("127.0.0.1")
        , port_(6379)
        , passwd_("")
        , dbnum_(0)
    {
        parse_uri(uri);
    }

public:
    void set_passwd(const std::string& passwd)
    {
        passwd_ = passwd;
    }

    const std::string& get_passwd()
    {
        return passwd_;
    }

    void set_ip(const char* ip)
    {
        ip_ = ip;
    }

    const std::string& get_ip() const
    {
        return ip_;
    }

    void set_port(int32_t port)
    {
        port_ = port;
    }

    int32_t get_port() const
    {
        return port_;
    }

    void set_dbnum(int32_t num)
    {
        dbnum_ = num;
    }

    int32_t get_dbnum()
    {
        return dbnum_;
    }

    /** 
     * @brief to uri string
     * format is "redis://passwd@ip:port/dbnum"
     */
    std::string to_string()
    {
        std::stringstream ss;
        ss << "redis://" << passwd_ << "@" << ip_ << ":" << port_ << "/" << dbnum_;
        return std::move(ss.str());
    }

};
}

#endif
