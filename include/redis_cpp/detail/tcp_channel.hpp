/**
 *
 * tcp_channel.hpp
 *
 * tcp channel( connection with redis server)
 *
 * @author  :   yandaren1220@126.com
 * @date    :   2017-04-22
 */

#ifndef __ydk_rediscpp_detial_tcp_channel_hpp__
#define __ydk_rediscpp_detial_tcp_channel_hpp__

#include <redis_cpp/detail/config.hpp>
#include <redis_cpp/internal/logger_handler.hpp>
#include <redis_cpp/detail/redis_buffer.hpp>
#include <redis_cpp/detail/tcp_channel_event.hpp>
#include <utility/noncopyable.hpp>
#include <asio.hpp>
#include <atomic>
#include <memory>
#include <mutex>
#include <queue>
#include <future>

namespace redis_cpp
{
namespace detail
{

static const int32_t max_receiver_buffer_len = 0xffff;
static const int32_t max_write_queue_size = 0x1000;

enum channel_state{
    connecting = 0,
    connected = 1,
    closing = 2,
    closed = 3,
};

class base_tcp_channel : public utility::noncopyable
{
protected:
    asio::io_service&       io_service_;
    asio::ip::tcp::socket   socket_;
    char                    recv_buffer[max_receiver_buffer_len];
    channel_state           con_state_;
    std::string             remote_ip_;
    int32_t                 remote_port_;
    std::mutex              mtx_;

public:
    base_tcp_channel(asio::io_service& io_service)
        : io_service_(io_service)
        , socket_(io_service)
        , con_state_(closed)
        , remote_port_(0)
        , remote_ip_("127.0.0.1")
    {
    }

    virtual ~base_tcp_channel(){}

public:
    bool is_connected(){
        return con_state_ == connected;
    }

    void set_state(channel_state state){
        con_state_ = state;
    }

    void set_address(const asio::ip::tcp::endpoint& end_point){
        remote_ip_ = end_point.address().to_v4().to_string();
        remote_port_ = end_point.port();
    }
};

class tcp_sync_channel;
typedef std::shared_ptr<tcp_sync_channel> tcp_sync_channel_ptr;
class tcp_sync_channel :
    public base_tcp_channel,
    public std::enable_shared_from_this<tcp_sync_channel>
{
public:
    tcp_sync_channel(asio::io_service& io_service)
        :base_tcp_channel(io_service)
    {
    }

    ~tcp_sync_channel()
    {
        close();
    }

public:
    bool connect(const asio::ip::tcp::endpoint& end_point)
    {
        std::lock_guard<std::mutex> locker(mtx_);

        asio::error_code ec;
        socket_.connect(end_point, ec);
        if (!ec)
        {
            set_address(end_point);
            set_state(connected);

            return true;
        }
        else
        {
            rds_log_error("redis sync_channel[%p] connect to address[%s:%d] failed, error:%s:%d.",
                this, end_point.address().to_v4().to_string().c_str(), end_point.port(), ec.message().c_str(), ec.value());
            return false;
        }
    }

    void close()
    {
        std::lock_guard<std::mutex> locker(mtx_);

        if (con_state_ != closed)
        {
            asio::error_code error;
            socket_.close(error);

            if (error)
                rds_log_error("close sync_channe[%p], error[%s:%d].", this, error.message().c_str(), error.value());

            set_state(closed);
        }
    }

    int32_t read(redis_buffer& buffer)
    {
        asio::error_code err;
        std::size_t len = socket_.read_some(asio::buffer(recv_buffer, max_receiver_buffer_len), err);
        if (!err){
            buffer = redis_buffer(recv_buffer, len);
            return len;
        }
        else{
            return -1;
        }
    }

    int32_t read(){
        asio::error_code err;
        std::size_t len = socket_.read_some(asio::buffer(recv_buffer, max_receiver_buffer_len), err);
        if (!err){
            return len;
        }
        else{
            return -1;
        }
    }

    void write(redis_buffer_ptr buffer, asio::error_code& error)
    {
        asio::write(socket_, 
            asio::buffer(buffer->data(), buffer->readable_bytes()), asio::transfer_all(), error);
    }

    void write(const char* buffer, std::size_t len, asio::error_code& error){
        asio::write(socket_,
            asio::buffer(buffer, len), asio::transfer_all(), error);
    }

    void write(std::string& str, asio::error_code& error){
        asio::write(socket_,
            asio::buffer(str.data(), str.length()), asio::transfer_all(), error);
    }

    char* get_receive_buffer(){
        return recv_buffer;
    }
};

class tcp_async_channel;
typedef std::shared_ptr<tcp_async_channel> tcp_async_channel_ptr;
class tcp_async_channel :
    public base_tcp_channel,
    public std::enable_shared_from_this < tcp_async_channel >
{
protected:
    asio::io_service::strand        strand_;
    tcp_channel_event*              event_handler_;
    std::queue<redis_buffer_ptr>    send_queue_;
    std::atomic_bool                shutdown_;

public:
    ~tcp_async_channel()
    {
        close();
    }

    static tcp_async_channel_ptr create(asio::io_service& io_service){
        return tcp_async_channel_ptr(new tcp_async_channel(io_service));
    }

protected:
    tcp_async_channel(asio::io_service& io_service)
        : base_tcp_channel(io_service)
        , strand_(io_service)
        , event_handler_(nullptr)
    {
        shutdown_ = false;
    }

public:

    void shutdown(){
        if (!shutdown_.exchange(true)){
            close();
        }
    }

    void close(){
        internal_close();
    }

    void set_event_handler(tcp_channel_event* event_hander){
        event_handler_ = event_hander;
    }

    bool connect(const asio::ip::tcp::endpoint& end_point, bool use_promise = false){
        if (shutdown_)
            return false;

        std::shared_ptr<std::promise<bool>> proms;
        {
            std::lock_guard<std::mutex> locker(mtx_);
            if (con_state_ != closed){
                return false;
            }

            set_address(end_point);
            set_state(connecting);
            asio::ip::tcp::resolver resolver(io_service_);
            auto endpoint_iterator = resolver.resolve(end_point);

            if (use_promise){
                proms = std::make_shared<std::promise<bool>>();
            }

            asio::async_connect(socket_, endpoint_iterator,
                                        strand_.wrap(std::bind(&tcp_async_channel::async_connect_handler,
                                        shared_from_this(),
                                        proms,
                                        std::placeholders::_1,
                                        std::placeholders::_2)));
        }

        if (proms){
            std::future<bool> future = proms->get_future();
            std::chrono::milliseconds span(1000);
            std::future_status status = future.wait_for(span);
            if (status != std::future_status::ready){
                rds_log_error("tcp_channel[%p] remote_address[%s:%d] wait connect promise time out, status[%d].",
                    this, end_point.address().to_string().c_str(), end_point.port(), status);
                return false;
            }

            bool success = future.get();

            rds_log_info("tcp_channel[%p] remote_address[%s:%d] wait connect promise returned, success[%d].",
                this, end_point.address().to_string().c_str(), end_point.port(), success);

            return success;
        }

        return true;
    }

    bool send(redis_buffer_ptr buffer){
        std::lock_guard<std::mutex> locker(mtx_);

        if (send_queue_.size() > max_write_queue_size){
            rds_log_error("async_channel[%p] current send queue too long, give up.", this);
            return false;
        }

        send_queue_.push(buffer);
        if (send_queue_.size() == 1){
            async_write(buffer);
        }

        return true;
    }

    void clear_send_queue_locked()
    {
        std::lock_guard<std::mutex> locker(mtx_);

        clear_send_queue();
    }

protected:

    void start_receive(){
        socket_.async_read_some(asio::buffer(recv_buffer, max_receiver_buffer_len),
                                strand_.wrap(std::bind(&tcp_async_channel::async_receive_handler,
                                                        shared_from_this(),
                                                        recv_buffer,
                                                        std::placeholders::_1,
                                                        std::placeholders::_2)));
    }

    void async_receive_handler(char* dst, std::error_code error, std::size_t length){
        if (shutdown_){
            return;
        }

        if (error){
            async_failed_handler(error);
            return;
        }

        if (!shutdown_ && event_handler_)
            event_handler_->message_received(remote_ip_.c_str(), remote_port_, dst, length);

        start_receive();
    }


    void async_write(redis_buffer_ptr buffer){
        asio::async_write(socket_, asio::buffer(buffer->data(), buffer->readable_bytes()),
                          strand_.wrap(std::bind(&tcp_async_channel::async_write_handler,
                                                 shared_from_this(),
                                                 buffer,
                                                 std::placeholders::_1,
                                                 std::placeholders::_2)));
    }

    void async_write_handler(redis_buffer_ptr buffer, std::error_code error, std::size_t length){

        if (shutdown_){
            return;
        }

        if (error){
            async_failed_handler(error);
            return;
        }

        std::lock_guard<std::mutex> locker(mtx_);

        if ( !send_queue_.empty())
            send_queue_.pop();

        if (!send_queue_.empty() && con_state_ == connected){
            async_write(send_queue_.front());
        }
    }


    void async_connect_handler(std::weak_ptr<std::promise<bool>> proms, std::error_code error, asio::ip::tcp::resolver::iterator it){

        if (shutdown_){
            return;
        }

        std::shared_ptr<std::promise<bool>> proms_ptr = proms.lock();

        if (error){
            async_failed_handler(error);

            if (proms_ptr){
                proms_ptr->set_value(false);
            }

            return;
        }

        // change the connection state
        {
            std::lock_guard<std::mutex> locker(mtx_);
            set_state(connected);

            // clear the send queue
            clear_send_queue();
        }

        // start receive
        start_receive();

        if (!shutdown_ && event_handler_)
            event_handler_->channel_open(remote_ip_.c_str(), remote_port_);

        if (proms_ptr){
            proms_ptr->set_value(true);
        }
    }

    void async_failed_handler(const std::error_code& error){
        if (!shutdown_ && event_handler_)
            event_handler_->channel_exception(remote_ip_.c_str(), remote_port_, error);

        internal_close();
    }

    void internal_close(){
        // close the socket
        {
            std::lock_guard<std::mutex> locker(mtx_);

            if (con_state_ == closed || con_state_ == closing)
                return;

            con_state_ = closed;

            asio::error_code err;
            socket_.close(err);

            if (err){
                rds_log_error("internal_close socket error, %s, %d", 
                    err.message().c_str(), err.value());
            }
        }

        if (!shutdown_ && event_handler_)
            event_handler_->channel_closed(remote_ip_.c_str(), remote_port_);
    }

    void clear_send_queue(){
        while (!send_queue_.empty()){
            send_queue_.pop();
        }
    }
};

}
}

#endif