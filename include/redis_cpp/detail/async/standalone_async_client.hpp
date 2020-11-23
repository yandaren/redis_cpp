/**
 *
 * standalone_async_client.hpp
 *
 * standalone async client implement
 *
 * @author  :   yandaren1220@126.com
 * @date    :   2017-05-03
 */

#ifndef __ydk_rediscpp_detail_standalone_async_client_hpp__
#define __ydk_rediscpp_detail_standalone_async_client_hpp__

#include <redis_cpp/detail/config.hpp>
#include <redis_cpp/detail/async/base_async_client.hpp>
#include <redis_cpp/detail/tcp_channel.hpp>
#include <redis_cpp/detail/redis_parser.hpp>
#include <redis_cpp/detail/redis_buffer.hpp>
#include <redis_cpp/detail/redis_command.hpp>
#include <redis_cpp/internal/logger_handler.hpp>
#include <redis_cpp/redis_uri.hpp>
#include <utility/asio_base/timer.hpp>
#include <mutex>
#include <queue>

namespace redis_cpp
{
namespace detail
{

/** reconnect interval in millisecond */
static const int32_t reconnect_interval = 5000;

/** check client available interval */
static const int32_t check_client_available_interval = 5000;

class standalone_async_client :
    public base_async_client,
    public tcp_channel_event
{
protected:
    channel_message_handler_map_type subscribe_msg_handlers_;
    tcp_async_channel_ptr            connection_;
    asio::ip::tcp::endpoint*         endpoint_;
    redis_parser                     parser_;
    redis_uri                        redis_uri_;
    bool                             cluster_enabled_;
    std::mutex                       mtx_;
    std::mutex                       uri_mtx_;
    std::queue<reply_handler_ptr>    handler_queue_;
    utility::asio_base::timer::ptr   reconnect_timer_;
    utility::asio_base::timer::ptr   check_client_available_timer_;
public:
    standalone_async_client(asio::io_service& io_service, const char* uri)
        : redis_uri_(uri)
        , cluster_enabled_(false){
        endpoint_ = new asio::ip::tcp::endpoint(
            asio::ip::address::from_string(redis_uri_.get_ip()),
            redis_uri_.get_port());
        connection_ = tcp_async_channel::create(io_service);
        connection_->set_event_handler(this);

        reconnect_timer_ = utility::asio_base::timer::create(io_service);
        reconnect_timer_->register_handler(std::bind(
            &standalone_async_client::reconnect_timer_handler,
            this,
            std::placeholders::_1,
            std::placeholders::_2));

        check_client_available_timer_ =
            utility::asio_base::timer::create(io_service);
        check_client_available_timer_->register_handler(std::bind(
            &standalone_async_client::check_client_available_timer_handler,
            this,
            std::placeholders::_1,
            std::placeholders::_2));
    }

    virtual ~standalone_async_client(){
        shutdown();

        if (check_client_available_timer_){
            check_client_available_timer_->cancel();
        }

        if (endpoint_){
            delete endpoint_;
            endpoint_ = nullptr;
        }
    }

    void    shutdown(){
        connection_->shutdown();
    }

    bool    is_connected(){
        return connection_->is_connected();
    }

    void try_connect(bool use_promise = false){
        if (endpoint_){
            connection_->connect(*endpoint_, use_promise);
        }
    }

    std::string uri_string(){
        std::lock_guard<std::mutex> locker(uri_mtx_);
        return std::move(redis_uri_.to_string());
    }

    void reset_uri_string(const std::string& ip, int32_t port)
    {
        bool need_reset = false;
        {
            std::lock_guard<std::mutex> locker(uri_mtx_);

            if (redis_uri_.get_ip() != ip || redis_uri_.get_port() != port)
            {
                std::string old_uri = redis_uri_.to_string();
                redis_uri_.set_ip(ip.c_str());
                redis_uri_.set_port(port);
                std::string new_uri = redis_uri_.to_string();

                if (endpoint_)
                {
                    delete endpoint_;
                    endpoint_ = new asio::ip::tcp::endpoint(
                        asio::ip::address::from_string(redis_uri_.get_ip()),
                        redis_uri_.get_port());
                }

                rds_log_info("sentinel_async_client[%p] uri[%s] change to uri[%s].",
                    this, old_uri.c_str(), new_uri.c_str());

                need_reset = true;
            }
        }

        if (need_reset)
        {
            session_reset();
        }
    }

    /** do command */
    void    do_command(const redis_command& cmd, const reply_handler& handler){
        std::string str(cmd.to_string());
        redis_buffer_ptr buffer = redis_buffer::create((int32_t)str.size(), true);
        buffer->write_bytes(str.data(), (int32_t)str.length());

        std::lock_guard<std::mutex> locker(mtx_);
        if (connection_->send(buffer)){
            handler_queue_.push(reply_handler_ptr(new reply_handler(handler)));
        }
    }

public:
    /** implement of interface of base_async_client */
    /** try connect to server */
    virtual void connect() override
    {
        try_connect(true);
    }

    /**
    * @brief subscribe specific channel with specific message handler
    */
    virtual void subscribe(const std::string& channel_name,
        const channel_message_handler& message_handler,
        const reply_handler& reply_handler) override{
        // record the channel msg handlers first, whether the connection is
        // connected or not
        {
            std::lock_guard<std::mutex> locker(mtx_);
            subscribe_msg_handlers_.insert(
                std::make_pair(channel_name,
                channel_message_handler_ptr(new channel_message_handler(message_handler))));
        }

        if (connection_->is_connected()){
            redis_command cmd("subscribe");
            cmd.add_param(channel_name);

            do_command(cmd, reply_handler);
        }
        else{
            rds_log_error("async_client[%p] uri[%s] not connected yet, subscribe channel[%s] failed.",
                this, uri_string().c_str(), channel_name.c_str());
        }
    }

    /**
    * @brief unsubscribe specific channel
    */
    virtual void unsubscribe(const std::string& channel_name,
        const reply_handler& reply_handler) override{
        // remove the channel msg handler first, whether the connection is 
        // connected or not
        {
            std::lock_guard<std::mutex> locker(mtx_);
            subscribe_msg_handlers_.erase(channel_name);
        }

        if (connection_->is_connected()){
            redis_command cmd("unsubscribe");
            cmd.add_param(channel_name);

            do_command(cmd, reply_handler);
        }
        else{
            rds_log_error("async_client[%p] uri[%s] not connected yet, unsubscribe channel[%s] failed.",
                this, uri_string().c_str(), channel_name.c_str());
        }
    }

    /**
    * @brief publish message to specific channel
    */
    virtual void publish(const std::string& channel_name,
        const std::string& message,
        const reply_handler& reply_handler) override{
        if (connection_->is_connected()){
            redis_command cmd("publish");
            cmd.add_param(channel_name);
            cmd.add_param(message);

            do_command(cmd, reply_handler);
        }
        else{
            rds_log_error("async_client[%p] uri[%s] not connected yet, publish msg to channel[%s] failed.",
                this, uri_string().c_str(), channel_name.c_str());
        }
    }

public:
    /** implement of interface of tcp_channel_event */
    /**
    * connection to remote address opened
    */
    virtual void channel_open(const char* ip, int32_t port) override{
        rds_log_info("[%p] async_client to server[%s:%d] channel opend.",
            this, ip, port);

        // clear old handler queue
        clear_handler_queue();

        // auth
        if (redis_uri_.get_passwd() != ""){
            auth();
        }

        // select db
        if (!cluster_enabled_ && redis_uri_.get_dbnum() != 0){
            select_db();
        }

        // check role
        check_role();

        // try recover subscribe channels
        recover_subscribe_channels();

        // start the check client available timer
        check_client_available_timer_->start(check_client_available_interval);
    }

    /*
    * connection to remote address closed
    */
    virtual void channel_closed(const char* ip, int32_t port) override{
        rds_log_error("[%p] async_client to server[%s:%d] channel closed.",
            this, ip, port);

        // cancel the check client available timer
        check_client_available_timer_->cancel();

        // start reconnect timer
        reconnect_timer_->start(reconnect_interval);
    }

    /**
    * connection to remote address exception
    */
    virtual void channel_exception(const char* ip, int32_t port,
        const std::error_code& err) override{
        rds_log_error("[%p] async_client to server[%s:%d] channel exception[%s:%d].",
            this, ip, port, err.message().c_str(), err.value());
    }

    /**
    * received msg from the connection
    */
    virtual void message_received(const char* ip, int32_t port,
        const char* message, int32_t size) override{

        parser_.push_bytes((char*)message, size);
        for (;;){
            parse_result result = parser_.parse();
            if (result == redis_ok){
                process_message();
            }
            else if (result == redis_incomplete){
                // wait for remain content
                return;
            }
            else if (result == redis_error){
                // parse error
                rds_log_error("[%p] uri[%s] parser redis content failed.",
                    this, redis_uri_.to_string().c_str());

                parser_.reset();
                connection_->close();

                return;
            }
        }

    }

protected:
    /**
    * @brief connection reconnect timer handler
    */
    void    reconnect_timer_handler(utility::asio_base::timer::ptr timer_ptr,
        const asio::error_code& error){
        if (!error){
            try_connect();
        }
        else{
            rds_log_error("async_client[%p] reconnect timer error, %s:%d.",
                this, error.message().c_str(), error.value());
        }
    }

    /**
    * @brief check connection available
    */
    void    check_client_available(){
        ping();
    }

    /**
    * @brief check connection available timer handler
    */
    void    check_client_available_timer_handler(
        utility::asio_base::timer::ptr timer_ptr,
        const asio::error_code& error){
        if (!error){
            check_client_available();

            timer_ptr->start(check_client_available_interval);
        }
        else{
            rds_log_error("async_client[%p] check available timer error, %s:%d.",
                this, error.message().c_str(), error.value());
        }
    }

    /** clear old request handler queue */
    void    clear_handler_queue(){

        std::lock_guard<std::mutex> locker(mtx_);
        while (!handler_queue_.empty()){
            handler_queue_.pop();
        }
    }

    /** default reply handler */
    void default_reply_handler(redis_reply_ptr reply){

    }

    /** auth */
    void    auth(){
        redis_command cmd("auth");
        cmd.add_param(redis_uri_.get_passwd());

        do_command(cmd,
            std::bind(
            &standalone_async_client::default_reply_handler,
            this,
            std::placeholders::_1));
    }

    /** select db */
    void    select_db(){
        redis_command cmd("select");
        cmd.add_param(redis_uri_.get_dbnum());

        do_command(cmd,
            std::bind(
            &standalone_async_client::default_reply_handler,
            this,
            std::placeholders::_1));
    }

    /** role reply handler */
    void role_reply_handler(redis_reply_ptr reply){

    }

    /** check role */
    void    check_role(){
        redis_command cmd("role");

        do_command(cmd,
            std::bind(
            &standalone_async_client::role_reply_handler,
            this,
            std::placeholders::_1));
    }

    /** ping */
    void    ping(){
        if (!connection_->is_connected()){
            rds_log_error("async_client[%p] uri[%s] not connected yet, ping failed.",
                this, redis_uri_.to_string().c_str());
            return;
        }

        redis_command cmd("ping");

        do_command(cmd,
            std::bind(
            &standalone_async_client::default_reply_handler,
            this,
            std::placeholders::_1));
    }

    /** check is channel message */
    bool    check_ischannel_message(redis_reply_ptr reply){
        if (!reply->is_array()){
            return false;
        }

        redis_reply_arr& arr = reply->to_array();
        if (arr.size() != 3){
            return false;
        }

        if (!arr[0].is_string() || !arr[1].is_string())
            return false;

        std::string& cmd = arr[0].to_string();
        std::string& channel_name = arr[1].to_string();
        if (cmd == "message" && arr[2].is_string()){
            std::string& msg = arr[2].to_string();
            process_channel_message(channel_name, msg);
            return true;
        }
        else if (cmd == "subscribe" && arr[2].is_integer()){
            int32_t channel_count = arr[2].to_integer_32();
            rds_log_info("async_client[%p] cur channel:%d after subscribe[%s].",
                this, channel_count, channel_name.c_str());
        }
        else if (cmd == "unsubscribe" && arr[2].is_integer()){
            int32_t channel_count = arr[2].to_integer_32();
            rds_log_info("async_client[%p] cur channel:%d after usubscribe[%s].",
                this, channel_count, channel_name.c_str());
        }

        return false;
    }

    /** pop reply handler */
    reply_handler_ptr pop_reply_handler(){
        std::lock_guard<std::mutex> locker(mtx_);
        if (!handler_queue_.empty()){
            reply_handler_ptr ret = handler_queue_.front();
            handler_queue_.pop();
            return ret;
        }
        return reply_handler_ptr();
    }

    /** process message */
    void    process_message(){
        redis_reply_ptr reply = parser_.transfer_reply();

        if (!reply){
            rds_log_error("[%p] uri[%s] process message reply pointer is null.",
                this, redis_uri_.to_string().c_str());
            return;
        }

        // check if channel message
        if (check_ischannel_message(reply)){
            return;
        }

        reply_handler_ptr reply_handle = pop_reply_handler();
        if (!reply_handle){
            std::string message;
            rds_log_error("unexcepted message[%s].", message.c_str());
            return;
        }

        (*reply_handle)(reply);
    }

    /** process channel message */
    void    process_channel_message(const std::string& channel_name,
        const std::string& msg){
        auto msg_handler = get_message_handler(channel_name);
        if (msg_handler){
            (*msg_handler)(channel_name, msg);
        }
    }

    /** try recover subscribe channels */
    void    recover_subscribe_channels(){
        channel_message_handler_map_type subcribe_list;
        {
            std::lock_guard<std::mutex> locker(mtx_);
            subcribe_list = subscribe_msg_handlers_;
            subscribe_msg_handlers_.clear();
        }

        // remove the subscribe
        for (auto iter = subcribe_list.begin();
            iter != subcribe_list.end(); ++iter){
            subscribe(iter->first,
                *iter->second,
                std::bind(
                &standalone_async_client::default_reply_handler,
                this,
                std::placeholders::_1));
        }
    }

    /** get channel msg handler */
    channel_message_handler_ptr get_message_handler(const std::string& channel){
        std::lock_guard<std::mutex> locker(mtx_);
        auto iter = subscribe_msg_handlers_.find(channel);
        if (iter != subscribe_msg_handlers_.end()){
            return iter->second;
        }
        return channel_message_handler_ptr();
    }

private:
    /** session reset */
    void    session_reset()
    {
        std::string uri_str = uri_string();
        rds_log_info("standalone_async_client[%p] uri[%s] session_reset.",
            this, uri_str.c_str());

        if (connection_){
            connection_->close();
        }

        // clear old handler queue
        clear_handler_queue();

        // clear connection old send queue
        if (connection_){
            connection_->clear_send_queue_locked();
        }
    }
};
}
}

#endif
