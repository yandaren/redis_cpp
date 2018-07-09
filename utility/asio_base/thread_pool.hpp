/**
 *
 * thread_pool.hpp
 *
 * 基于asio的线程池(主要利用asio::io_service做任务管理)
 *
 * @author  :   yandaren1220@126.com
 * @date    :   2017-04-14
 */

#ifndef __ydk_utility_asio_base_thread_pool_hpp__
#define __ydk_utility_asio_base_thread_pool_hpp__

#include "asio_standalone.hpp"
#include <asio/io_service.hpp>
#include <cstdint>
#include <functional>
#include <thread>
#include <atomic>

namespace utility
{
namespace asio_base
{
class thread_pool
{
protected:
    int32_t             thread_count_;
    std::thread**       thread_list_;
    asio::io_service    io_service_;
    std::string         thread_name_;
    std::atomic_bool    started_;
public:
    thread_pool(int32_t thread_count, char* name = 0)
        : thread_count_(thread_count)
        , io_service_()
        , thread_list_(nullptr)
    {
        if (!name){
            thread_name_ = "thread_pool";
        }
        else{
            thread_name_ = name;
        }

        started_ = false;
    }

    ~thread_pool(){
        join_all();

        if (thread_list_){

            for (int32_t i = 0; i < thread_count_; ++i){
                delete thread_list_[i];
            }
            delete[] thread_list_;
        }
    }

    asio::io_service& io_service(){
        return io_service_;
    }

    int32_t thread_count(){
        return thread_count_;
    }

    void start(){
        if (!started_.exchange(true)){
            create_threads();
        }
    }

    void stop(){
        io_service_.stop();
    }

    void wait_for_stop(){
        join_all();
    }

protected:
    void create_threads(){
        if (thread_count_ > 0){
            thread_list_ = new std::thread*[thread_count_];
            for (int32_t i = 0; i < thread_count_; ++i){
                thread_list_[i] = new std::thread(std::bind(&thread_pool::run, this));
            }
        }
    }

    void join_all(){
        if (thread_list_){
            for (int32_t i = 0; i < thread_count_; ++i){
                if (thread_list_[i]->joinable()){
                    thread_list_[i]->join();
                }
            }
        }
    }

    void run(){
        asio::error_code error;
        asio::io_service::work work(io_service_);
        io_service_.run(error);
    }
};
}
}

#endif