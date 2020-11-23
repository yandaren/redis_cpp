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

#include <mpark/variant.hpp>
#include <string.h>
#include <cstdint>
#include <vector>
#include <memory>

#if defined(_WIN32)
#define stricmp _stricmp
#define strnicmp _strnicmp
#elif defined(__linux__)
#define stricmp strcasecmp
#define strnicmp strncasecmp
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
    mpark::variant<nil_reply, error_reply, int64_t, std::string, redis_reply_arr> content_;
public:
    redis_reply() : content_(nil_reply()){
    }

    redis_reply(error_reply& error) : content_(error){
    }

    redis_reply(int64_t i) : content_(i){
    }

    redis_reply(std::string& s) : content_(s){
    }

	redis_reply(std::string&& s) : content_(s){
	}

    redis_reply(redis_reply_arr& arr) : content_(arr){
    }

	redis_reply(redis_reply_arr&& arr) : content_(arr){
	}

public:
    bool is_nil(){
        return content_.index() == 0;
    }

    bool is_error(){
        return content_.index() == 1;
    }

    bool is_integer() {
        return content_.index() == 2;
    }

    bool is_string(){
        return content_.index() == 3;
    }

    bool is_array(){
        return content_.index() == 4;
    }

    nil_reply& to_nil(){
        return mpark::get<nil_reply>(content_);
    }

    error_reply& to_error(){
        return mpark::get<error_reply>(content_);
    }

	int64_t to_integer(){
        return mpark::get<int64_t>(content_);
    }

    int32_t to_integer_32() {
        return (int32_t)mpark::get<int64_t>(content_);
    }

    std::string& to_string(){
        return mpark::get<std::string>(content_);
    }

    redis_reply_arr& to_array(){
        return mpark::get<redis_reply_arr>(content_);
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
