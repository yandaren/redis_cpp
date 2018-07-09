/**
 *
 * redis_reply.hpp
 *
 * redis reply types
 *
 * @author  :   yandaren1220@126.com
 * @date    :   2017-04-24
 */

#ifndef __ydk_rediscpp_redis_reply_hpp__
#define __ydk_rediscpp_redis_reply_hpp__

#include <utility/variant/variant.hpp>
#include <cstdint>
#include <vector>
#include <memory>

#if defined(_WIN32)
#define stricmp _stricmp
#define strnicmp _strnicmp
#endif

namespace redis_cpp
{

struct nil_reply{
    inline bool operator == (const nil_reply&) const{
        return true;
    }
};

struct error_reply{
    error_reply(){
    }

    error_reply(const std::string& s) : msg(s){
    }

    error_reply(std::string&& s) : msg(std::move(s)){
    }

    std::string msg;
};

class redis_reply;
typedef std::shared_ptr<redis_reply> redis_reply_ptr;
typedef std::vector<redis_reply>     redis_reply_arr;
class redis_reply
{
protected:
    utility::variant<nil_reply, error_reply, int32_t, std::string, redis_reply_arr> content_;
public:
    redis_reply() : content_(nil_reply()){
    }

    redis_reply(error_reply& error) : content_(error){
    }

    redis_reply(int32_t i) : content_(i){
    }

    redis_reply(std::string& s) : content_(s){
    }

    redis_reply(redis_reply_arr& arr) : content_(arr){
    }

public:
    template<class type>
    bool is() const
    {
        return content_.is<type>();
    }

    template<typename type>
    typename std::decay<type>::type& get()
    {
        return content_.get<type>();
    }

public:
    bool is_nil(){
        return content_.is<nil_reply>();
    }

    bool is_error(){
        return content_.is<error_reply>();
    }

    bool is_integer(){
        return content_.is<int32_t>();
    }

    bool is_string(){
        return content_.is<std::string>();
    }

    bool is_array(){
        return content_.is<redis_reply_arr>();
    }

    nil_reply& to_nil(){
        return content_.get<nil_reply>();
    }

    error_reply& to_error(){
        return content_.get<error_reply>();
    }

    int32_t to_integer(){
        return content_.get<int32_t>();
    }

    std::string& to_string(){
        return content_.get<std::string>();
    }

    redis_reply_arr& to_array(){
        return content_.get<redis_reply_arr>();
    }

    bool    check_status_ok(){
        if (!is_string())
            return false;

        std::string& str = to_string();
        return stricmp(str.c_str(), "OK") == 0;
    }
};
}

#endif