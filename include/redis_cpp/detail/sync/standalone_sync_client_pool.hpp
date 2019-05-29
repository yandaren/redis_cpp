/**
 *
 * standalone_sync_client_pool.hpp
 *
 * standalone_sync_client_pool, means there is more than one sync client
 *
 * @author  :   yandaren1220@126.com
 * @date    :   2017-04-26
 */

#ifndef __ydk_rediscpp_detail_standalone_sync_client_pool_hpp__
#define __ydk_rediscpp_detail_standalone_sync_client_pool_hpp__


#include <redis_cpp/detail/config.hpp>
#include <redis_cpp/detail/sync/standalone_sync_client.hpp>
#include <redis_cpp/detail/sync/base_standalone_sync_client_pool.hpp>
#include <utility/asio_base/thread_pool.hpp>
#include <utility/asio_base/timer.hpp>
#include <queue>
#include <vector>
#include <mutex>

namespace redis_cpp
{
namespace detail
{

/* the interval to check sync clients available in milliseconds */
static const int32_t check_sync_client_valid_interval = 5000;

/** auto extand pool max size uplimit */
static const int32_t auto_extand_pool_size_uplimit = 25;

class standalone_sync_client_pool : 
    public base_standalone_sync_client_pool,
    public base_sync_client
{
protected:
    utility::asio_base::thread_pool*     thread_pool_;
    bool                                 thread_pool_self_maintain_;
    utility::asio_base::timer::ptr       sync_client_valid_check_timer_;
    std::queue<standalone_sync_client*>  free_sync_client_queue_;
    std::vector<standalone_sync_client*> total_sync_client_list_;
    std::mutex                           list_mtx_;
    int32_t                              pool_max_size_;
    int32_t                              pool_min_size_;
    redis_uri                            redis_uri_;
    bool                                 auto_extand_pool_max_size_;
    std::mutex                           uri_mtx_;

public:
    standalone_sync_client_pool(
        const char* uri, 
        int32_t pool_init_size,
        int32_t pool_max_size,
        utility::asio_base::thread_pool* thread_pool = nullptr) 
            : redis_uri_(uri), auto_extand_pool_max_size_(true){
        if (thread_pool){
            thread_pool_ = thread_pool;
            thread_pool_self_maintain_ = false;
        }
        else{
            thread_pool_ = new utility::asio_base::thread_pool(2);
            thread_pool_self_maintain_ = true;
            thread_pool_->start();
        }

        initialize(pool_init_size, pool_max_size);

        sync_client_valid_check_timer_ = 
            utility::asio_base::timer::create(thread_pool_->io_service());
        sync_client_valid_check_timer_->register_handler(
            std::bind(&standalone_sync_client_pool::check_sync_clients_available_timer_handler, 
            this, std::placeholders::_1, std::placeholders::_2));
        sync_client_valid_check_timer_->start(check_sync_client_valid_interval);
    }

    virtual ~standalone_sync_client_pool(){
        sync_client_valid_check_timer_->cancel();

        if (thread_pool_self_maintain_){
            thread_pool_->stop();
            thread_pool_->wait_for_stop();
        }

        // free the connections
        std::lock_guard<std::mutex> locker(list_mtx_);
        while (!free_sync_client_queue_.empty()){
            free_sync_client_queue_.pop();
        }
        for (auto client : total_sync_client_list_){
            client->destroy();
        }
        total_sync_client_list_.clear();

        // free the thread pool
        if (thread_pool_self_maintain_){
            delete thread_pool_;
            thread_pool_ = nullptr;
        }
    }

    void stop(){
        if (thread_pool_self_maintain_){
            thread_pool_->stop();
        }
    }

    std::string uri_string(){
        std::lock_guard<std::mutex> locker(uri_mtx_);
        return std::move(redis_uri_.to_string());
    }

    redis_uri   get_uri()
    {
        std::lock_guard<std::mutex> locker(uri_mtx_);
        return redis_uri_;
    }

    void        reset_uri_string(const std::string& ip, int32_t port){
        std::lock_guard<std::mutex> locker(uri_mtx_);

        std::string old_uri = redis_uri_.to_string();
        redis_uri_.set_ip(ip.c_str());
        redis_uri_.set_port(port);
        std::string new_uri = redis_uri_.to_string();

        rds_log_info("standalone_sync_client_pool[%p] uri[%s] change to uri[%s].",
            this, old_uri.c_str(), new_uri.c_str());
    }

    bool empty(){
        std::lock_guard<std::mutex> locker(list_mtx_);
        return total_sync_client_list_.empty();
    }

public:

    /** implement of base_sync_client* /
    /** do command */
    virtual redis_reply_ptr do_command(const redis_command& cmd, int32_t hash_slot) override
    {
        standalone_sync_client* client = get_client();
        if (!client){
            rds_log_error("pool[%p] can't find available client to do cmd, uri[%s].", 
                this, uri_string().c_str());
            return nullptr;
        }

        redis_reply_ptr reply = client->do_command(cmd, hash_slot);
        if (reply){
            client->close();
        }
        else{
            client->free();
        }

        return reply;
    }

    /** is cluster mode */
    virtual bool         cluster_mode() override
    {
        return false;
    }

    /** implement of base_standalone_sync_client_pool */
    /** interfaces */

    /** reclaim the client to pool */
    virtual void reclaim_to_pool(standalone_sync_client* client) override
    {
        if (!client)
            return;

        std::lock_guard<std::mutex> locker(list_mtx_);

        // todo, check the client vaild

        free_sync_client_queue_.push(client);
        assert(free_sync_client_queue_.size() <= total_sync_client_list_.size());
    }

    /** remove and free the client from pool */
    virtual void free_client(standalone_sync_client* client) override
    {
        if (!client)
            return;

        // try remove the client from this pool
        {
            std::lock_guard<std::mutex> locker(list_mtx_);

            auto iter = std::find(total_sync_client_list_.begin(), 
                total_sync_client_list_.end(), client);
            if (iter != total_sync_client_list_.end()){
                total_sync_client_list_.erase(iter);
            }

            rds_log_info("remove cli[%p] from pool[%p], cur[%d] free[%d] max[%d] uri[%s].",
                client, this, 
                total_sync_client_list_.size(), 
                free_sync_client_queue_.size(), 
                pool_min_size_,
                uri_string().c_str());
        }

        client->destroy();
    }

    /** get a client from the pool */
    standalone_sync_client* get_client(){
        std::lock_guard<std::mutex> locker(list_mtx_);

        if (free_sync_client_queue_.empty()){
            try_allocate_a_client();
        }

        if (free_sync_client_queue_.empty()){
            return nullptr;
        }

        standalone_sync_client* client = free_sync_client_queue_.front();
        free_sync_client_queue_.pop();
        return client;
    }

protected:

    /** 
     * @brief check_client_available_one_time
     */
    virtual bool check_client_available_one_time(standalone_sync_client* client)
    {
        redis_sync_operator redis_op(client);
        bool ret = redis_op.ping();
        if (!ret){
            rds_log_error("client[%p] ping failed, uri[%s].",
                client, uri_string().c_str());
        }

        return ret;
    }

    /** 
     * @brief check sync clients available
     */
    void check_sync_clients_available(){
        std::size_t test_count = 0;
        {
            std::lock_guard<std::mutex> locker(list_mtx_);
            test_count = free_sync_client_queue_.size();
        }

        for (std::size_t i = 0; i < test_count; ++i){
            standalone_sync_client* client = get_client();
            if (!client){
                break;
            }

            if (!check_client_available_one_time(client)){
                rds_log_info("client[%p] check available, remove from pool, uri[%s].",
                    client, uri_string().c_str());
                free_client(client);
            }
            else
            {
                reclaim_to_pool(client);
            }
        }

        // make sure there has min client
        ensure_min_client_count();
    }

    void ensure_min_client_count(){
        int32_t current_total_count = 0;
        {
            std::lock_guard<std::mutex> locker(list_mtx_);
            current_total_count = (int32_t)total_sync_client_list_.size();
        }

        if (current_total_count >= pool_min_size_){
            return;
        }

        int32_t to_increase_count = pool_min_size_ - current_total_count;
        for (int32_t i = 0; i < to_increase_count; ++i){
            std::lock_guard<std::mutex> locker(list_mtx_);
            try_allocate_a_client();
        }
    }

    void check_sync_clients_available_timer_handler(
        utility::asio_base::timer::ptr timer_ptr, const asio::error_code& error){
        if (!error){
            check_sync_clients_available();

            timer_ptr->start(check_sync_client_valid_interval);
        }
        else{
            rds_log_error("pool[%p] check_clients_avalialbe_timer error, %s:%d.", 
                this, error.message().c_str(), error.value());
        }
    }

    void    initialize(int32_t pool_init_size, int32_t pool_max_size){

        pool_max_size_ = pool_max_size;

        if (pool_init_size > pool_max_size_){
            pool_init_size = pool_max_size_;
        }

        pool_min_size_ = pool_init_size;

        rds_log_info("try init pool[%p] min[%d] max[%d].", 
            this, pool_min_size_, pool_max_size_);

        for (int32_t i = 0; i < pool_min_size_; ++i){
            try_allocate_a_client();
        }

        rds_log_info("pool[%p] init finished, cur[%d] free[%d] max[%d], uri[%s].",
            this, total_sync_client_list_.size(), 
            free_sync_client_queue_.size(), 
            pool_max_size_, uri_string().c_str());
    }

    void    try_allocate_a_client(){

        if ((int32_t)total_sync_client_list_.size() >= pool_max_size_ &&
            auto_extand_pool_max_size_ &&
            pool_max_size_ < auto_extand_pool_size_uplimit){

            rds_log_info("pool[%p] uri[%s] cur poolsize[%d] reach max[%d], "
                "try extand max pool size from [%d] to [%d], uplimit[%d].",
                this, uri_string().c_str(),
                total_sync_client_list_.size(), 
                pool_max_size_, pool_max_size_, 
                pool_max_size_ + 1, auto_extand_pool_size_uplimit);

            pool_max_size_++;
        }

        if ((int32_t)total_sync_client_list_.size() >= pool_max_size_)
            return;

        standalone_sync_client* new_client = create_client();
        if (!new_client){
            return;
        }

        if (!add_client(new_client)){
            new_client->destroy();
        }
    }

    standalone_sync_client* create_client(){
        std::string uri(std::move(uri_string()));
        standalone_sync_client* client =
            new standalone_sync_client(thread_pool_->io_service(), uri.c_str(), this);

        if (!client->connect()){
            client->destroy();
            return nullptr;
        }

        // todo: check the role, such as check role is master, otherwise close the client
        // 
        return client;
    }

    bool    add_client(standalone_sync_client* client){
        if (!client){
            return false;
        }

        if ((int32_t)total_sync_client_list_.size() >= pool_max_size_){
            return false;
        }

        free_sync_client_queue_.push(client);
        total_sync_client_list_.push_back(client);

        rds_log_info("add new client to pool[%p], cur[%d] free[%d] max[%d] uri[%s].",
            this, total_sync_client_list_.size(), 
            free_sync_client_queue_.size(), 
            pool_max_size_, uri_string().c_str());

        return true;
    }

};
}
}

#endif