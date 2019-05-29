/**
 *
 * standalone_sync_client.hpp
 * 
 * sync client while the redis server run in standalone mode
 *
 * @author  :   yandaren1220@126.com
 * @date    :   2017-04-26
 */

#ifndef __ydk_rediscpp_detail_standalone_sync_client_hpp__
#define __ydk_rediscpp_detail_standalone_sync_client_hpp__

#include <redis_cpp/detail/config.hpp>
#include <redis_cpp/detail/sync/base_sync_client.hpp>
#include <redis_cpp/detail/sync/base_standalone_sync_client_pool.hpp>
#include <redis_cpp/detail/sync/redis_connection.hpp>
#include <redis_cpp/detail/sync/redis_sync_operator.hpp>
#include <redis_cpp/detail/tcp_channel.hpp>
#include <redis_cpp/detail/redis_parser.hpp>
#include <redis_cpp/detail/redis_buffer.hpp>
#include <redis_cpp/internal/logger_handler.hpp>
#include <redis_cpp/redis_uri.hpp>

namespace redis_cpp
{
namespace detail
{
 
class standalone_sync_client;
typedef std::shared_ptr<standalone_sync_client> standalone_sync_client_ptr;

class standalone_sync_client : 
    public base_sync_client
{
protected:
    tcp_sync_channel_ptr                connection_;
    asio::ip::tcp::endpoint*            remote_endpoint_;
    redis_parser                        parser_;
    redis_uri                           uri_;
    base_standalone_sync_client_pool*   client_pool_;
    bool                                cluster_enabled_;

public:
    /** 
     * @brief 
     * @param uri like "redis://foobared@localhost:6380/2"
     * format is "redis://passwd@ip:port/dbnum"
     * @param pool - the client pool
     */
    standalone_sync_client(asio::io_service& io_service, 
        const char* uri, 
        base_standalone_sync_client_pool* pool = nullptr)
        : uri_(uri)
        , client_pool_(pool)
        , cluster_enabled_(false)
    {
        remote_endpoint_ = 
            new asio::ip::tcp::endpoint(
            asio::ip::address::from_string(uri_.get_ip()), uri_.get_port());
        connection_ = std::make_shared<tcp_sync_channel>(io_service);
    }


protected:
    /** void object create on stack, only provice the destroy api to release the object */
    ~standalone_sync_client(){
        if (connection_){
            connection_->close();
        }

        if (remote_endpoint_){
            delete remote_endpoint_;
            remote_endpoint_ = nullptr;
        }
    }

public:
    /** interface **/
    /** do command */
    virtual redis_reply_ptr do_command(const redis_command& cmd, int32_t hash_slot) override
    {
        std::string str(std::move(cmd.to_string()));
        asio::error_code ec;
        connection_->write(str, ec);

        if (ec){
            error_handler(ec.message().c_str());
            return nullptr;
        }

        for (;;){
            int32_t size = connection_->read();

            if (size < 0){
                return nullptr;
            }

            parser_.push_bytes(connection_->get_receive_buffer(), size);
            parse_result result = parser_.parse();

            if (result == redis_ok){
                return parser_.transfer_reply();
            }
            else if (result == redis_incomplete){
                // wait for remain content
            }
            else if (result == redis_error){
                // error 

                error_handler("paser redis content failed");

                return nullptr;
            }
        }

        return nullptr;
    }

    /** is cluster mode */
    virtual bool         cluster_mode() override
    {
        return false;
    }

public:

    /** try connect to the remote address */
    bool connect(){
        if (!remote_endpoint_)
            return false;

        if (!connection_->connect(*remote_endpoint_))
            return false;

        redis_sync_operator redis_op(this);

        // auth
        if (uri_.get_passwd() != ""){
            redis_op.auth(uri_.get_passwd().c_str());
        }

        // select db num
        if (!cluster_enabled_ && uri_.get_dbnum() != 0){
            redis_op.select(uri_.get_dbnum());
        }

        return true;
    }

    /** check the connection is connected */
    bool is_connected(){
        return connection_->is_connected();
    }

    /** close the connection */
    void close(){
        if (client_pool_){
            client_pool_->reclaim_to_pool(this);
        }
    }

    /** free the client */
    void free(){
        if (client_pool_){
            client_pool_->free_client(this);
        }
        else{
            destroy();
        }
    }

    /** destroy the client */
    void destroy(){
        delete this;
    }

    /** get uri string */
    std::string get_uri_string(){
        return std::move(uri_.to_string());
    }

    /** check uri change */
    bool check_address_change(const redis_uri& uri){
        return uri.get_ip() != uri_.get_ip() || uri.get_port() != uri_.get_port();
    }

protected:

    void error_handler(const char* error){

        rds_log_error("standalone sync client error[%s].", error);

        // close the connection
        connection_->close();

        // reset the parser state
        parser_.reset();
    }
};
}
}

#endif