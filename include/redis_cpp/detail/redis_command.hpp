/**
 *
 * redis_command.hpp
 *
 * the redis_command
 *
 * @author  :   yandaren1220@126.com
 * @date    :   2017-04-22
 */

#ifndef __ydk_rediscpp_detail_redis_command_hpp__
#define __ydk_rediscpp_detail_redis_command_hpp__

#include <redis_cpp/detail/config.hpp>
#include <strstream>
#include <vector>

namespace redis_cpp
{
namespace detail
{
static const std::string crlf = "\r\n";
static const std::string debug_crlf = "\\r\\n";

class redis_command
{
protected:
    std::vector<std::string> param_list_;

public:
    redis_command(){}
    redis_command(const char* cmd){ param_list_.push_back(cmd); }
    redis_command(const std::string& cmd){ param_list_.push_back(cmd); }
    redis_command(std::string&& cmd){ param_list_.push_back(std::move(cmd)); }

public:
    void add_param(const char* p){ 
        param_list_.push_back(p); 
    }
    void add_param(const char* p, int32_t size){
        param_list_.resize(param_list_.size() + 1);
        param_list_.back().append(p, size);
    }
    void add_param(const std::string& p){ 
        param_list_.push_back(p); 
    }
    void add_param(std::string&& p){
        param_list_.push_back(std::move(p));
    }
    void add_param(int32_t p){
        param_list_.push_back(std::move(std::to_string(p)));
    }
    void add_param(uint32_t p){
        param_list_.push_back(std::move(std::to_string(p)));
    }
    void add_param(int64_t p){
        param_list_.push_back(std::move(std::to_string(p)));
    }
    void add_param(uint64_t p){
        param_list_.push_back(std::move(std::to_string(p)));
    }
    void add_param(double p){
        param_list_.push_back(std::move(std::to_string(p)));
    }
    void add_param(float p){
        param_list_.push_back(std::move(std::to_string(p)));
    }

    std::string to_string() const{
        return std::move(to_string(crlf));
    }

    std::string to_debug_string() const{
        return std::move(to_string(debug_crlf));
    }

    void clear(){
        param_list_.clear();
    }

protected:
    std::string to_string(const std::string& crlf) const
    {
        std::stringstream  result;
        result << '*' << param_list_.size() << crlf;

        auto iter = param_list_.begin();
        for (iter; iter != param_list_.end(); ++iter)
        {
            result << '$' << (*iter).size() << crlf;
            result << (*iter) << crlf;
        }

        return std::move(result.str());
    }
};
}
}

#endif