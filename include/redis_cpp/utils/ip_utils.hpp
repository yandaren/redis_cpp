/**
 *
 * ip_utils.hpp
 *
 *
 * @author  :   yandaren1220@126.com
 * @date    :   2019-05-10
 */

#ifndef __ydk_rediscpp_detail_utils_ip_utis_hpp__
#define __ydk_rediscpp_detail_utils_ip_utis_hpp__

#include <redis_cpp/detail/config.hpp>
#include <asio.hpp>
#include <vector>

namespace redis_cpp {

namespace ip_utils{

namespace v4{

    static std::vector<std::string> get_ip_list(const char* domain) {
        std::vector<std::string> ip_list;
        std::string hostname = domain ? domain : asio::ip::host_name();

        asio::io_service io_service;
        asio::ip::tcp::resolver resolver(io_service);
        asio::ip::tcp::resolver::query qr(hostname, "");
        asio::ip::tcp::resolver::iterator iter = resolver.resolve(qr);
        asio::ip::tcp::resolver::iterator end;
        while (iter != end) {
            asio::ip::tcp::endpoint ep = *iter++;
            if (ep.address().is_v4()) {
                ip_list.push_back(ep.address().to_string());
            }
        }
        return ip_list;
    }
}

}

}

#endif