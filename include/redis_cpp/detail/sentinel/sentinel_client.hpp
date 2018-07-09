/**
 *
 * sentinel_client.hpp
 *
 * sentinel_client
 *
 * @author  :   yandaren1220@126.com
 * @date    :   2017-06-03
 */

#ifndef __ydk_rediscpp_detail_sentinel_client_hpp__
#define __ydk_rediscpp_detail_sentinel_client_hpp__

#include <redis_cpp/detail/config.hpp>
#include <redis_cpp/detail/sentinel/base_sentinel_client.hpp>
#include <redis_cpp/detail/async/standalone_async_client.hpp>
#include <utility/sync/promise_once.hpp>
#include <utility/str.hpp>
#include <unordered_map>
#include <unordered_set>
#include <atomic>

namespace redis_cpp
{
namespace detail
{

class sentinel_client :
    public base_sentinel_client,
    public standalone_async_client
{
protected:
    std::atomic_bool    first_channel_open_;
    bool                subscribe_switchmaster_channel_;
public:
    sentinel_client(asio::io_service& io_service, const char* uri, bool subscribe_switchmaster_channel = true)
        : standalone_async_client(io_service, uri)
        , subscribe_switchmaster_channel_(subscribe_switchmaster_channel)
    {
        first_channel_open_ = true;
    }

    ~sentinel_client()
    {

    }

protected:
    void get_master_address_reply_handler(
        std::string master_name,
        std::weak_ptr<utility::sync::promise_once<std::tuple<bool, std::string, int32_t>>> proms,
        redis_cpp::redis_reply* reply)
    {
        std::string uri_str = uri_string();
        rds_log_info("sentinel_client[%p] uri[%s] get master[%s] address reply[%p]",
            this, uri_str.c_str(), master_name.c_str(), reply);

        bool success = false;
        std::string ip;
        int32_t port = 0;
        do{
            if (!reply){
                rds_log_error("sentinel_client[%p] uri[%s] get master[%s] address reply pointer is null",
                    this, uri_str.c_str(), master_name.c_str());
                break;
            }

            if (reply->is_nil()){
                rds_log_warn("sentinel_client[%p] uri[%s] get master[%s] address reply is nil",
                    this, uri_str.c_str(), master_name.c_str());
                break;
            }

            if (!reply->is_array())
            {
                rds_log_error("sentinel_client[%p] uri[%s] get master[%s] address reply is not array",
                    this, uri_str.c_str(), master_name.c_str());
                break;
            }

            redis_reply_arr& arry = reply->to_array();
            if (arry.size() != 2){
                rds_log_error("sentinel_client[%p] uri[%s] get master[%s] address reply array size is not 2",
                    this, uri_str.c_str(), master_name.c_str());
                break;
            }

            ip = arry[0].to_string();
            std::string port_str = arry[1].to_string();
            sscanf(port_str.c_str(), "%d", &port);
            success = true;

        } while (0);

        rds_log_info("sentinel_client[%p] uri[%s] get master[%s] address reply, success[%d] master_address[%s:%d]",
            this, uri_str.c_str(), master_name.c_str(), success, ip.c_str(), port);

        std::shared_ptr < utility::sync::promise_once<std::tuple<bool, std::string, int32_t>> >
            proms_ptr = proms.lock();

        if (proms_ptr){
            proms_ptr->set_value(std::make_tuple(success, ip, port));
        }
    }

public:

   /** implements of base_sentinel_client */

   /**
    * @brief get master address by master name
    */
    virtual bool    get_master_address_by_name(const char* master_name,
        std::pair<std::string, int32_t>& out_master_address) override
    {
        redis_command cmd("sentinel");
        cmd.add_param("get-master-addr-by-name");
        cmd.add_param(master_name);

        std::shared_ptr<utility::sync::promise_once<std::tuple<bool, std::string, int32_t>>>
            proms = std::make_shared<utility::sync::promise_once<std::tuple<bool, std::string, int32_t>>>();

        std::string master_name_str(master_name);

        do_command(cmd, std::bind(&sentinel_client::get_master_address_reply_handler, 
            this, master_name_str, proms, std::placeholders::_1));

        std::future<std::tuple<bool, std::string, int32_t>> future = proms->get_future();
        std::chrono::milliseconds span(1000);
        std::future_status status = future.wait_for(span);
        if (status != std::future_status::ready)
        {
            rds_log_error("sentinel_client[%p] uri[%s] wait for get_master_address_by_name[%s] time out, status[%d].",
                this, uri_string().c_str(), master_name, status);
            return false;
        }

        std::tuple<bool, std::string, int32_t> ret = future.get();
        if (!std::get<0>(ret)){
            return false;
        }

        out_master_address.first = std::get<1>(ret);
        out_master_address.second = std::get<2>(ret);
        return true;
    }

    /**
    * @brief start
    */
    virtual void    start() override
    {
        connect();
    }

    /** override the implement of interface of sentinel_client_pool*/
    /**
    * connection to remote address opened
    */
    virtual void channel_open(const char* ip, int32_t port) override{
        standalone_async_client::channel_open(ip, port);
        if (subscribe_switchmaster_channel_ && first_channel_open_.exchange(false)){
            subscribe_switchmaster_channel();
        }
    }

protected:
    /** 
     * @brief sentinel_default_reply_handler
     */
    void    sentinel_default_reply_handler(redis_reply* reply)
    {
        rds_log_info("sentinel_client[%p] uri[%s] sentinel_default_reply_handler.",
            this, uri_string().c_str());
    }

    void    sentinel_switchmaster_channel_msg_handler(
        const std::string& channel_name, const std::string& message)
    {
        std::string uri_str = uri_string();
        rds_log_info("sentinel_client[%p] uri[%s] switchmaster_channel_msg, channel[%s] message[%s].",
            this, uri_str.c_str(), channel_name.c_str(), message.c_str());

        if (channel_name != "+switch-master")
        {
            rds_log_error("sentinel_client[%p] uri[%s] switchmaster_channel_msg, channel[%s] name not fit[+switch-master].",
                this, uri_str.c_str(), channel_name.c_str());
            return;
        }

        std::vector<std::string> splits;
        utility::str::string_splits(message.c_str(), " ", splits);
        if (splits.size() != 5)
        {
            rds_log_error("sentinel_client[%p] uri[%s] switchmaster_channel_msg, but message[%s] not valid.",
                this, uri_str.c_str(), message.c_str());
            return;
        }

        master_address_change_event_t event;
        event.master_name = splits[0];
        event.old_ip = splits[1];
        std::string old_port_str = splits[2];
        event.old_port = 0;
        sscanf(old_port_str.c_str(), "%d", &event.old_port);
        event.new_ip = splits[3];
        std::string new_port_str = splits[4];
        event.new_port = 0;
        sscanf(new_port_str.c_str(), "%d", &event.new_port);

        rds_log_info("recv msater switch notify, master[%s] old_addr[%s:%d] swith to new_address[%s:%d].",
            event.master_name.c_str(), event.old_ip.c_str(), event.old_port, event.new_ip.c_str(), event.new_port);

        // publish the event
        publish_event(event_master_address_change, &event);
    }

    /** 
     * @brief subscrie +switch-master channel
     */
    void    subscribe_switchmaster_channel()
    {
        subscribe("+switch-master",
            std::bind(&sentinel_client::sentinel_switchmaster_channel_msg_handler, this, std::placeholders::_1, std::placeholders::_2),
            std::bind(&sentinel_client::sentinel_default_reply_handler, this, std::placeholders::_1));
    }
};

}
}

#endif