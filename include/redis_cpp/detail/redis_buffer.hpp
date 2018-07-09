/**
 *
 * redis_buffer.hpp
 *
 *
 * @author  :   yandaren1220@126.com
 * @date    :   2017-04-22
 */

#ifndef __ydk_rediscpp_detail_redis_buffer_hpp__
#define __ydk_rediscpp_detail_redis_buffer_hpp__

#include <cstdint>
#include <stdexcept>
#include <memory>
#include <algorithm>

namespace redis_cpp
{
namespace detail
{
#define max_buffer_len 1024 * 1024 * 512

class redis_buffer;
typedef std::shared_ptr<redis_buffer> redis_buffer_ptr;
class redis_buffer
{
protected:
    char*   data_;
    int32_t capacity_;
    int32_t reader_index_;
    int32_t writer_index_;
    int32_t reader_index_mark_;
    bool    is_wrappered_;
    bool    is_auto_extend_;
public:
    redis_buffer()
        : data_(0)
        , capacity_(0)
        , reader_index_(0)
        , writer_index_(0)
        , reader_index_mark_(0)
        , is_wrappered_(false)
        , is_auto_extend_(false)
    {}

    redis_buffer(int32_t size, bool auto_extend = false)
        : data_(0)
        , capacity_(0)
        , reader_index_(0)
        , writer_index_(0)
        , reader_index_mark_(0)
        , is_wrappered_(false)
        , is_auto_extend_(auto_extend)
    {
        allocate(size);
    }

    redis_buffer(char* data, int32_t size, bool auto_delete_wrapper_buf = false)
        : data_(0)
        , capacity_(size)
        , reader_index_(0)
        , writer_index_(size)
        , reader_index_mark_(0)
        , is_wrappered_(!auto_delete_wrapper_buf)
        , is_auto_extend_(false)
    {
    }

    static redis_buffer_ptr create(int32_t size, bool auto_extend = false)
    {
        return std::make_shared<redis_buffer>(size, auto_extend);
    }

    virtual ~redis_buffer(){
        if (data_ && !is_wrappered_){
            delete[] data_;
            data_ = nullptr;
        }
    }

public:
    void set_auto_extend(bool f){
        is_auto_extend_ = f;
    }

    char* data(){
        return data_ + reader_index_;
    }

    int32_t  drop_read(int32_t size){
        int32_t to_drop_len = size;
        if (reader_index_ + size > writer_index_){
            to_drop_len = writer_index_ - reader_index_;
        }

        reader_index_ += to_drop_len;
        return to_drop_len;
    }

    int32_t drop_write(int32_t size){
        int32_t to_drop_len = size;
        if (writer_index_ - size < reader_index_){
            to_drop_len = writer_index_ - reader_index_;
        }
        writer_index_ -= to_drop_len;
        return to_drop_len;
    }

    int32_t max_capacity(){
        return capacity_;
    }

    int32_t capacity(){
        return capacity_ - writer_index_;
    }

    int32_t readable_bytes(){
        return writer_index_ - reader_index_;
    }

    bool is_readable(){
        return readable_bytes() > 0;
    }

    bool is_readable(int32_t size){
        return readable_bytes() >= size;
    }

    void mark_reader_index(){
        reader_index_mark_ = reader_index_;
    }

    void reset_reader_index(){
        reader_index_ = reader_index_mark_;
    }

    int32_t reader_index(){
        return reader_index_;
    }

    void    set_reader_index(int32_t index){
        if (index >= 0 && index < capacity_)
            reader_index_ = index;
        else
            throw std::runtime_error("set reader index out of bounds");
    }

    void    clear(){
        reader_index_ = 0;
        writer_index_ = 0;
        reader_index_mark_ = 0;
    }

    bool    is_writable(){
        return capacity() > 0;
    }

    bool    is_writable(int32_t size){
        return capacity() >= size;
    }

    void    set_write_index(int32_t index){
        if (index >= 0 && index < capacity_)
            writer_index_ = index;
        else
            throw std::runtime_error("set write index out of bounds.");
    }

    int32_t writer_index(){
        return writer_index_;
    }

    char get_byte(int32_t index){
        return data_[index];
    }

    void get_bytes(int32_t index, char* dst, int32_t length){
        memcpy(dst, data_ + index, length);
    }

    void set_byte(int32_t index, char b){
        data_[index] = b;
    }

    void set_bytes(int32_t index, char* src, int32_t length){
        memcpy(data_ + index, src, length);
    }

    int32_t read_bytes(char* dst, int32_t length){
        int32_t to_read_len = length;
        if (to_read_len > readable_bytes()){
            to_read_len = readable_bytes();
        }

        memcpy(dst, data(), to_read_len);
        reader_index_ += to_read_len;

        return to_read_len;
    }

    int32_t read_bytes(redis_buffer_ptr dst, int32_t length){
        int32_t to_read_len = length;
        if (to_read_len > readable_bytes()){
            to_read_len = readable_bytes();
        }

        dst->write_bytes(data(), to_read_len);
        reader_index_ += to_read_len;

        return to_read_len;
    }

    int8_t  read_int8(){
        int8_t v;
        read(v);
        return v;
    }

    int16_t read_int16(){
        int16_t v;
        read(v);
        return v;
    }

    int32_t read_int32(){
        int32_t v;
        read(v);
        return v;
    }

    int64_t read_int64(){
        int64_t v;
        read(v);
        return v;
    }

    bool write_bytes(const char* data, int32_t len){
        if (capacity() < len){
            if (!resize(len)){
                return false;
            }
        }

        memcpy(data_ + writer_index_, data, len);
        writer_index_ += len;
        return true;
    }

    bool write_int8(int8_t v){
        return write(v);
    }

    bool write_int16(int16_t v){
        return write(v);
    }

    bool write_int32(int32_t v){
        return write(v);
    }

    bool write_int64(int64_t v){
        return write(v);
    }

    void compact(){
        if (reader_index_ > 0){
            memmove(data_, data(), readable_bytes());
            writer_index_ = readable_bytes();
            reader_index_ = 0;
        }
    }

protected:
    template<typename T>
    void read(T& v){
        int32_t size = sizeof(T);
        char* dest = (char*)&v;
        read_bytes(dest, size);
    }

    template<typename T>
    bool write(T v){
        int32_t size = sizeof(T);
        char* data = (char*)&v;
        return write_bytes(data, size);
    }

    void    allocate(int32_t size)
    {
        if (data_){
            throw std::runtime_error("the buffer can not allocate memory more than once.");
        }
        else{
            data_ = new char[size];
            if (data_ == 0){
                throw std::runtime_error("buffer allocate memory failed.");
            }
            else capacity_ = size;
        }
    }

    bool    resize(int32_t size){

        if (!is_auto_extend_)
            return false;

        uint32_t new_capacity = (std::min<uint32_t>)(capacity_ * 2, max_buffer_len);
        new_capacity = (std::max<uint32_t>)(new_capacity, writer_index_ + size);

        if (new_capacity > max_buffer_len){
            char error[128];
            sprintf(error, "exceeding the maximum allowable memory [%dB].", max_buffer_len);
            throw std::runtime_error(error);
            return false;
        }

        char* new_data = new char[new_capacity];
        int32_t readable_len = readable_bytes();
        memcpy(new_data, data(), readable_len);

        writer_index_ = readable_len;
        reader_index_ = 0;
        capacity_ = new_capacity;

        if (data_)
            delete[] data_;
        data_ = new_data;

        return true;
    }
};

}
}

#endif