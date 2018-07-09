/**
 *
 * File path.hpp
 * 一些string相关的接口
 *
 * @author  :   yandaren1220@126.com
 * @date    :   2017-03-16
 */

#ifndef __ydk_utility_str_hpp__
#define __ydk_utility_str_hpp__

#include <string>
#include <sstream>

namespace ydk
{
namespace str
{
    static std::string string_replace(const std::string& in, const std::string& from, const std::string& to)
    {
        std::string ret = in;
        std::size_t start_pos = 0;
        while (true)
        {
            std::size_t pos = ret.find(from, start_pos);
            if (pos != std::string::npos)
            {
                ret.replace(pos, from.length(), to);
                start_pos = pos + to.size();
            }
            else break;
        }

        return std::move(ret);
    }

    static void        string_splits(const std::string& in, const std::string& sep, std::vector<std::string>& out_splits)
    {
        std::size_t start_pos = 0;
        while (start_pos < in.size())
        {
            std::size_t pos = in.find(sep, start_pos);
            if (pos != std::string::npos)
            {
                out_splits.push_back(std::move(in.substr(start_pos, pos - start_pos)));
                start_pos = pos + 1;
            }
            else
            {
                out_splits.push_back(std::move(in.substr(start_pos, in.size() - start_pos)));
                break;
            }
        }
    }

    static void        string_splits(const char* in_str, const char* sep_str, std::vector<std::string>& out_splits)
    {
        std::string in(in_str);
        std::string sep(sep_str);
        std::size_t start_pos = 0;
        while (start_pos < in.size())
        {
            std::size_t pos = in.find(sep, start_pos);
            if (pos != std::string::npos)
            {
                out_splits.push_back(std::move(in.substr(start_pos, pos - start_pos)));
                start_pos = pos + 1;
            }
            else
            {
                out_splits.push_back(std::move(in.substr(start_pos, in.size() - start_pos)));
                break;
            }
        }
    }

    static bool        string_endwith(const std::string& str, const std::string& s)
    {
        if (str.empty())
            return false;

        std::size_t pos = str.find(s);
        return pos == str.size() - s.size();
    }

    static bool        string_endwith(const std::string& str, char c)
    {
        if (str.empty())
            return false;

        std::size_t pos = str.find(c);
        return pos == str.size() - 1;
    }

    static bool        string_startwith(const std::string& str, const std::string& s)
    {
        if (str.empty())
            return false;

        std::size_t pos = str.find(s);
        return pos == 0;
    }

    static bool        string_startwith(const std::string& str, char c)
    {
        if (str.empty())
            return false;

        std::size_t pos = str.find(c);
        return pos == 0;
    }

    /**
    * @brief string to hex string
    */
    static const std::string string_to_hex(unsigned char* bytes, std::size_t len){
        std::stringstream ss;
        char buf[32] = { 0 };
        for (std::size_t i = 0; i < len; ++i){
            sprintf(buf, "%02x", (bytes[i] & 0xFF));
            ss << buf;
        }
        return std::move(ss.str());
    }

    /**
    * @brief strint to binary
    */
    static const std::string string_to_binary(unsigned char* bytes, std::size_t len){
        std::stringstream ss;
        char buf[16] = { 0 };
        for (std::size_t i = 0; i < len; ++i){
            unsigned char byte = bytes[i];
            std::size_t index = 0;
            while (byte){
                buf[index++] = byte % 2;
                byte /= 2;
            }

            if (index % 4 != 0){
                std::size_t r = 4 - index % 4;
                for (std::size_t k = 0; k < r; ++k){
                    buf[index++] = 0;
                }
            }

            // revert
            for (std::size_t k = 0; k < index / 2; ++k){
                // swap
                char tmp = buf[k];
                buf[k] = buf[index - 1 - k];
                buf[index - 1 - k] = tmp;
            }

            // end
            buf[index] = '\0';
            ss << std::string(buf, index);
        }
        return std::move(ss.str());
    }
}
}

#endif