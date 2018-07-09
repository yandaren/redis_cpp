/**
 *
 * promise_once.hpp
 * 
 * guarantee the promise only set value once to void double set value throw exception.
 *
 * @author  :   yandaren1220@126.com
 * @date    :   2017-04-19
 */

#ifndef __ydk_utility_sync_promise_once_hpp__
#define __ydk_utility_sync_promise_once_hpp__

#include <future>
#include <atomic>

namespace utility
{
namespace sync
{

template<class T>
class promise_once
{
protected:
    std::promise<T>     promise_;
    std::atomic_bool    has_set_;
public:

    promise_once& operator=(const promise_once& that) = delete;
    promise_once(const promise_once& that) = delete;

    promise_once()
    {
        has_set_ = false;
    }

    ~promise_once()
    {

    }

public:
    std::future<T> get_future()
    {
        return promise_.get_future();
    }

    void set_value(const T& value)
    {
        if (!has_set_.exchange(true))
        {
            promise_.set_value(value);
        }
    }

    void set_value(T&& value)
    {
        if (!has_set_.exchange(true))
        {
            promise_.set_value(value);
        }
    }

    void set_value(T& value)
    {
        if (!has_set_.exchange(true))
        {
            promise_.set_value(value);
        }
    }
};
}
}

#endif