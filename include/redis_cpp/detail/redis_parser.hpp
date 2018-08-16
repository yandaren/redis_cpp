/**
 *
 * redis_parser.hpp
 *
 * the redis_parser
 *
 * @author  :   yandaren1220@126.com
 * @date    :   2017-04-25
 */

#ifndef __ydk_rediscpp_redis_parser_hpp__
#define __ydk_rediscpp_redis_parser_hpp__

#include <redis_cpp/redis_reply.hpp>
#include <redis_cpp/detail/redis_buffer.hpp>
#include <redis_cpp/internal/logger_handler.hpp>
#include <cstdint>
#include <stack>
#include <cassert>

namespace redis_cpp
{
namespace detail{

enum redis_reply_type{
    redis_reply_status  = 0,    // '+'
    redis_reply_error   = 1,    // '-'
    redis_reply_integer = 2,    // ':'
    redis_reply_string  = 3,    // '$'
    redis_reply_array   = 4,    // '*'
};

/* default len */
#define default_parser_buf_len 256

// compcat the buffer size threshold
#define compact_parser_buf_threshold 1024 * 16

struct redis_reply_read_context{
    int32_t                     reply_type;         // reply_type
    redis_buffer                buf;                // cumulative buffer
    redis_reply_ptr             reply;              // template reply pointer
    std::stack<int32_t>         array_size_stack;   // if reply is array, array size stack
    std::stack<redis_reply_arr> array_stack;        // array stack

    redis_reply_read_context() : buf(default_parser_buf_len, true) {
        reset();
    }

    void reset(){
        reply_type = -1;
        buf.clear();
        reply.reset();

        while (!array_size_stack.empty())
            array_size_stack.pop();
        while (!array_stack.empty())
            array_stack.pop();
    }
};

enum parse_result{
    redis_ok = 0,
    redis_incomplete = 1,
    redis_error = 2,
};


static bool read_integer(char* s, int32_t len, int32_t& integer){
    int32_t v = 0;
    int32_t mult = 1;
    int32_t pos = 0;
    char c = 0;
    if (len >= 0){
        if (s[pos] == '-'){
            mult = -1;
            pos++;
        }
        else if (s[pos] == '+'){
            mult = 1;
            pos++;
        }

        while (pos < len){
            c = s[pos];

            if (c < '0' || c > '9'){
                // should not happen
                return false;
            }

            v = v * 10 + (c - '0');
            pos++;
        }
    }

    integer = v * mult;
    return true;
}

static int32_t find_crlf(char* s, int32_t size){
    if (size == 0){
        return -1;
    }

    int32_t pos = 0;
    int32_t _size = size - 1;
    while (pos < _size){
        while (pos < _size && s[pos] != '\r') pos++;
        if (s[pos] != '\r'){
            return -1;
        }
        else{
            if (s[pos + 1] == '\n'){
                return pos;
            }
            else{
                pos++;
            }
        }
    }

    return -1;
}

class redis_parser
{
protected:
    redis_reply_read_context    read_ctx_;
public:
    redis_parser(){
    }

    ~redis_parser(){
    }

public:

    void reset(){
        read_ctx_.reset();
    }

    void push_bytes(char* message){
        push_bytes(message, (int32_t)strlen(message));
    }

    void push_bytes(char* message, int32_t size){
        if (size <= 0)
            return;

        read_ctx_.buf.write_bytes(message, size);
    }

    parse_result parse(){
        parse_result r = redis_ok;
        for(;;){
            r = process_item();
            if (r != redis_ok)
                break;

            if (read_ctx_.array_stack.empty())
                break;

            read_ctx_.array_stack.top().push_back(std::move(*read_ctx_.reply));
            bool need_break = false;
            while (!read_ctx_.array_stack.empty() && 
                   read_ctx_.array_stack.top().size() == read_ctx_.array_size_stack.top()){
                redis_reply_arr top_element(std::move(read_ctx_.array_stack.top()));
                read_ctx_.array_stack.pop();
                read_ctx_.array_size_stack.pop();

                if (read_ctx_.array_stack.empty()){
                    read_ctx_.reply = std::make_shared<redis_reply>(top_element);
                    need_break = true;
                    break;
                }

                read_ctx_.array_stack.top().push_back(top_element);
            }

            if (need_break)
                break;
        } 

        // check compact the parser buffer
        if (read_ctx_.buf.max_capacity() >= compact_parser_buf_threshold && 
            read_ctx_.buf.reader_index() >= read_ctx_.buf.max_capacity() / 2 ){
            read_ctx_.buf.compact();
        }

        return r;
    }

    redis_reply* get_reply(){
        return read_ctx_.reply.get();
    }

protected:
    parse_result process_item(){
        if (read_ctx_.reply_type < 0){
            if (read_ctx_.buf.readable_bytes() == 0){
                return redis_incomplete;
            }

            char byte = (char)read_ctx_.buf.read_int8();
            switch (byte){
            case '+':
                read_ctx_.reply_type = redis_reply_status;
                break;
            case '-':
                read_ctx_.reply_type = redis_reply_error;
                break;
            case ':':
                read_ctx_.reply_type = redis_reply_integer;
                break;
            case '$':
                read_ctx_.reply_type = redis_reply_string;
                break;
            case '*':
                read_ctx_.reply_type = redis_reply_array;
                break;
            default:
                rds_log_error("unsupport first reply char[%c].", byte);
                read_ctx_.reset();
                return redis_error;
            }
        }

        // process typed item
        switch (read_ctx_.reply_type)
        {
        case redis_reply_status:
        case redis_reply_error:
        case redis_reply_integer:
            return process_line_item();
        case redis_reply_string:
            return process_bulk_item();
        case redis_reply_array:
            return process_muti_bulk_item();
        default:
            assert(0);
            read_ctx_.reset();
            return redis_error;
        }
    }


    parse_result process_line_item(){
        int32_t crlf_pos = 
            find_crlf(read_ctx_.buf.data(), read_ctx_.buf.readable_bytes());
        if (crlf_pos == -1){
            return redis_incomplete;
        }

        if (read_ctx_.reply_type == redis_reply_integer){
            int32_t integer = 0;
            if (!read_integer(read_ctx_.buf.data(), crlf_pos, integer)){
                read_ctx_.reset();
                return redis_error;
            }

            read_ctx_.reply = std::make_shared<redis_reply>(integer);
        }
        else if ( read_ctx_.reply_type == redis_reply_error ){
            std::string str(read_ctx_.buf.data(), crlf_pos);
            error_reply er(std::move(str));
            read_ctx_.reply = std::make_shared<redis_reply>(er);

            rds_log_warn("[error reply](%s).", er.msg.c_str());
        }
        else {
            std::string str(read_ctx_.buf.data(), crlf_pos);

            read_ctx_.reply = std::make_shared<redis_reply>(str);
        }

        // skip \r\n
        read_ctx_.buf.drop_read(crlf_pos + 2);

        // move next_parser task( mainly reset the reply type)
        move_next_parse_task();

        return redis_ok;
    }

    parse_result process_bulk_item(){
        int32_t crlf_pos = 
            find_crlf(read_ctx_.buf.data(), read_ctx_.buf.readable_bytes());
        if (crlf_pos == -1){
            return redis_incomplete;
        }
        int32_t bulk_size = 0;
        read_ctx_.buf.mark_reader_index();
        if (!read_integer(read_ctx_.buf.data(), crlf_pos, bulk_size)){
            read_ctx_.reset();
            return redis_error;
        }

        // skill \r\n
        read_ctx_.buf.drop_read(crlf_pos + 2);

        if (bulk_size == -1){
            read_ctx_.reply = std::make_shared<redis_reply>();

            // move next_parser task( mainly reset the reply type)
            move_next_parse_task();

            return redis_ok;
        }
        else if (read_ctx_.buf.readable_bytes() >= bulk_size + 2){

            std::string str(read_ctx_.buf.data(), bulk_size);
            read_ctx_.buf.drop_read(bulk_size + 2);
            read_ctx_.reply = std::make_shared<redis_reply>(str);

            // move next_parser task( mainly reset the reply type)
            move_next_parse_task();

            return redis_ok;
        }

        // reset reader index
        read_ctx_.buf.reset_reader_index();

        return redis_incomplete;
    }

    parse_result process_muti_bulk_item(){
        int32_t crlf_pos = 
            find_crlf(read_ctx_.buf.data(), read_ctx_.buf.readable_bytes());
        if (crlf_pos == -1){
            return redis_incomplete;
        }

        int32_t array_size = 0;
        if (!read_integer(read_ctx_.buf.data(), crlf_pos, array_size)){
            read_ctx_.reset();
            return redis_error;
        }

        // skill \r\n
        read_ctx_.buf.drop_read(crlf_pos + 2);

        if (array_size <= -1){
            read_ctx_.reply = std::make_shared<redis_reply>();

            // move next_parser task( mainly reset the reply type)
            move_next_parse_task();

            return redis_ok;
        }
        else if (array_size == 0){
            read_ctx_.reply = std::make_shared<redis_reply>(std::move(redis_reply_arr()));

            // move next_parser task( mainly reset the reply type)
            move_next_parse_task();

            return redis_ok;
        }
        else{
            // array size > 0
            // to array stack
            read_ctx_.array_size_stack.push(array_size);
            read_ctx_.array_stack.push(std::move(redis_reply_arr()));

            // move next_parser task( mainly reset the reply type)
            move_next_parse_task();

            return process_item();
        }
    }

    void move_next_parse_task(){
        // reset the reply type
        read_ctx_.reply_type = -1;
    }
};
} // end namespace detial
} // end namespace redis_cpp

#endif