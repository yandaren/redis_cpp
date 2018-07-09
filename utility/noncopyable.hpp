/*
 * File noncopyable.hpp
 *
 * 继承该类可以使对象不可copy[在编译期就提示出错]
 *
 *
 * @Author: yandaren1220@126.com
 * @date:	2017-04-22
 */

#ifndef __ydk_utility_noncopyable_hpp__
#define __ydk_utility_noncopyable_hpp__

namespace utility
{
    class noncopyable
    {
    protected:
        noncopyable() {}
        ~noncopyable() {}

    private:
        noncopyable(const noncopyable&);
        noncopyable& operator=(const noncopyable&);
    };
}

#endif

