/**
 *
 * timer.hpp
 *
 * 基于asio的定时器(主要利用asio::io_service做任务管理)
 *
 * @author  :   yandaren1220@126.com
 * @date    :   2017-04-14
 */

#ifndef __ydk_utility_asio_base_timer_hpp__
#define __ydk_utility_asio_base_timer_hpp__

#include "asio_standalone.hpp"
#include <asio/high_resolution_timer.hpp>
#include <asio/io_service.hpp>
#include <chrono>
#include <cstdint>
#include <memory>
#include <functional>

namespace utility
{
namespace asio_base
{

class timer : public asio::high_resolution_timer,
              public std::enable_shared_from_this<timer>
{
public:
    typedef std::shared_ptr<timer> ptr;
    typedef std::function<void(ptr, const asio::error_code&)> timer_handler;

protected:
    timer_handler* invoker_;

public:
    ~timer()
    {
        if (invoker_){
            delete invoker_;
            invoker_ = nullptr;
        }
    }

    static ptr create(asio::io_service& io_service)
    {
        return ptr(new timer(io_service));
    }

    void register_handler(timer_handler handler){
        if (invoker_){
            *invoker_ = handler;
        }
        else{
            invoker_ = new timer_handler(handler);
        }
    }

    void start(uint32_t millsec){
        if (invoker_){
            expires_from_now(std::chrono::milliseconds(millsec));
            async_wait(std::bind(*invoker_, shared_from_this(), std::placeholders::_1));
        }
    }

    void start_at(const std::chrono::high_resolution_clock::time_point& time_p){
        if (invoker_){
            expires_at(time_p);
            async_wait(std::bind(*invoker_, shared_from_this(), std::placeholders::_1));
        }
    }

protected:
    timer(asio::io_service& io_service)
        : asio::high_resolution_timer(io_service)
        , invoker_(0)
    {
    }

};
}
}

#endif