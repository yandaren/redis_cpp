/**
 *
 * base_sentinel_client.hpp
 *
 * the interface of sentinel_client
 *
 * @author  :   yandaren1220@126.com
 * @date    :   2017-06-03
 */

#ifndef __ydk_rediscpp_detail_base_sentinel_client_hpp__
#define __ydk_rediscpp_detail_base_sentinel_client_hpp__

#include <redis_cpp/detail/config.hpp>
#include <unordered_map>
#include <unordered_set>
#include <mutex>

namespace redis_cpp
{
namespace detail
{

enum event_num
{
    event_master_address_change = 0,
    event_max,
};

struct master_address_change_event_t
{
    std::string master_name;
    std::string old_ip;
    int32_t     old_port;
    std::string new_ip;
    int32_t     new_port;

    master_address_change_event_t() :old_port(0), new_port(0){
    }
};

class event_subscriber
{
public:
    virtual ~event_subscriber(){};
public:
    virtual void on_event(event_num e, void* content) = 0;
};

class event_publisher
{
protected:
    std::mutex  subscribers_mtx_;
    std::unordered_map<event_num, std::unordered_set<event_subscriber*>> event_subscriber_set_;

public:
    event_publisher(){

    }

    virtual ~event_publisher(){

    }

public:
   /**
    * @brief add event subscriber
    */
    virtual void    add_event_subscriber(event_num e, event_subscriber* sub)
    {
        std::lock_guard<std::mutex> locker(subscribers_mtx_);
        event_subscriber_set_[e].insert(sub);
    }

    /**
    * @brief remove event subscriber
    */
    virtual void    remove_event_subscriber(event_num e, event_subscriber* sub)
    {
        std::lock_guard<std::mutex> locker(subscribers_mtx_);
        auto iter = event_subscriber_set_.find(e);
        if (iter != event_subscriber_set_.end())
        {
            iter->second.erase(sub);
        }
    }

    /** publish events */
    void    publish_event(event_num e, void* content)
    {
        std::lock_guard<std::mutex> locker(subscribers_mtx_);
        auto iter = event_subscriber_set_.find(e);
        if (iter != event_subscriber_set_.end())
        {
            for (auto subsciber : iter->second)
            {
                subsciber->on_event(e, content);
            }
        }
    }
};

class base_sentinel_client : public event_publisher
{
public:
    virtual ~base_sentinel_client(){}

public:
    /** 
     * @brief get master address by master name
     */
    virtual bool    get_master_address_by_name(const char* master_name, 
        std::pair<std::string, int32_t>& out_master_address) = 0;

    /** 
     * @brief start
     */
    virtual void    start() = 0;
};
}
}


#endif