a c++ redis client based on asio, need c++ 11 support
***
### samples

### 1. custom log handler
我们可以自定义redis_cpp的内部日志处理函数以及设置内部日志级别, 默认是输出到控制台

```
#include <iostream>
#include <strstream>
#include <vector>
#include <redis_cpp.hpp>
#include <utility/asio_base/thread_pool.hpp>

//#define __standalone_sync_client_test__
//#define __standalone_sync_client_pool_test__
#define __cluster_sync_client_test__

//#define __standalone_async_client_test__
#define __cluster_async_client_tet__

void custom_log_handler(redis_cpp::internal::rds_log_level log_level, const char* data, int32_t len){
    printf("custom_log_handler, lvl[%d], msg[%s], len[%d]\n", log_level, data, len);
}

static const char* loglvl_str(redis_cpp::internal::rds_log_level lvl){
    static const char* log_level_strs[] = { "debug", "info", "warn", "error", "fatal" };
    static const char* uknow_level = "unknow";
    if (lvl >= redis_cpp::internal::rds_log_level_debug &&
        lvl <= redis_cpp::internal::rds_log_level_fatal){
        return log_level_strs[lvl];
    }
    return uknow_level;
}

void redis_cpp_log_handler_1(redis_cpp::internal::rds_log_level log_level, const char* data, int32_t len){
    auto tp = std::chrono::system_clock::now();
    auto tt = std::chrono::system_clock::to_time_t(tp);
    auto duration = tp.time_since_epoch();
    int32_t millis = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count() % 1000;

    struct tm* ptm = localtime(&tt);
    char date[64] = { 0 };

    sprintf(date, "%d-%02d-%02d %02d:%02d:%02d,%03d",
        (int32_t)ptm->tm_year + 1900, (int32_t)ptm->tm_mon + 1, (int32_t)ptm->tm_mday,
        (int32_t)ptm->tm_hour, (int32_t)ptm->tm_min, (int32_t)ptm->tm_sec, millis);

    printf("--------[%s][%s][rds] %s\n", date, loglvl_str(log_level), data);
}

int main()
{
    std::cout << "redis_cpp test." << std::endl;

	// set the internal loggel level
	redis_cpp::set_log_lvl(redis_cpp::internal::rds_log_level_debug);
	// set the internal log handler
    redis_cpp::set_log_handler(redis_cpp_log_handler_1);

	return 0
}

```

### 2. redis stanalone, sentinel, cluster 三种模式下的operator

- 单例模式(standalone)
  - 单链接
    ```
    using namespace redis_cpp;
    using namespace redis_cpp::detail;

    utility::asio_base::thread_pool pool(2);
    pool.start();
    standalone_sync_client* sync_client = new standalone_sync_client(pool.io_service(), redis_uri.c_str());
    sync_client->connect();
	
	// operator
    redis_sync_operator client(sync_client);

    // set foo bar
    client.set("foo", "bar");
    
	// get foo
    std::string res_foo;
    if (client.get("foo", res_foo))
    {
        printf("get foo: %s\n", res_foo.c_str());
    }
    else
    {
        printf("get foo: failed\n");
    }
    ```
  - 连接池
    ```
    using namespace redis_cpp;
    using namespace redis_cpp::detail;

    utility::asio_base::thread_pool pool(2);
    pool.start();
	standalone_sync_client_pool client_pool(redis_uri.c_str(), 1, 2, &pool);
    base_sync_client* sync_client = &client_pool;
	
	// operator
    redis_sync_operator client(sync_client);

    // set foo bar
    client.set("foo", "bar");
    
	// get foo
    std::string res_foo;
    if (client.get("foo", res_foo))
    {
        printf("get foo: %s\n", res_foo.c_str());
    }
    else
    {
        printf("get foo: failed\n");
    }
    ```
- 哨兵模式(sentinel)
  ```
	using namespace redis_cpp;
	using namespace redis_cpp::detail;
	
	std::string master_name = "test_master";
	const char* srv_list = "127.0.0.1:26379";
	// sentinel client
	sentinel_client_pool_ptr client_pool = sentinel_client_pool::create(srv_list);
	client_pool->start();
	
	std::string redis_uri_str = "redis://foobared@127.0.0.1:6381/1";
	redis_uri uri(redis_uri_str.c_str());
	
	std::pair<std::string, int32_t> address;
	if (client_pool->get_master_address_by_name(master_name.c_str(), address))
	{
	    uri.set_ip(address.first.c_str());
	    uri.set_port(address.second);
	
	    printf("master_name[%s]'s master address is [%s:%d].\n", 
			master_name.c_str(), address.first.c_str(), address.second);
	
	    redis_uri_str = uri.to_string();
	}
	else
	{
	    printf("try get master_name[%s]' master address failed.\n", master_name.c_str());
	}
	
	// sentinal sync client
	sentinel_sync_client* client = new sentinel_sync_client("acmaster", redis_uri_str.c_str(), 1, 2);
	client->set_sentinel_client_pool(client_pool);
	redis_sync_operator redis_op(client);

	// set foo bar
    client.set("foo", "bar");
    
  ```
- 集群模式
  ```
    using namespace redis_cpp;
    using namespace redis_cpp::detail;

    utility::asio_base::thread_pool pool(2);
    pool.start();
    std::string redis_uri_list = "redis://foobared@127.0.0.1:7000;redis://foobared@127.0.0.1:7001;redis://foobared@127.0.0.1:7002";
    cluster_sync_client cluster(redis_uri_list.c_str(), 1, 2);
    base_sync_client* sync_client = &cluster;
	
	// operator
    redis_sync_operator client(sync_client);

    // set foo bar
    client.set("foo", "bar");
    
	// get foo
    std::string res_foo;
    if (client.get("foo", res_foo))
    {
        printf("get foo: %s\n", res_foo.c_str());
    }
    else
    {
        printf("get foo: failed\n");
    }
  ```

### 2. redis key 相关的操作的接口使用

```
void redis_key_test(){
    using namespace redis_cpp;
    using namespace redis_cpp::detail;

    utility::asio_base::thread_pool pool(2);
    pool.start();

    std::string redis_uri = "redis://foobared@127.0.0.1:6379/1";

#if defined(__standalone_sync_client_test__)
    standalone_sync_client* sync_client = new standalone_sync_client(pool.io_service(), redis_uri.c_str());
    sync_client->connect();
#elif defined(__standalone_sync_client_pool_test__)
    standalone_sync_client_pool client_pool(redis_uri.c_str(), 1, 2, &pool);
    base_sync_client* sync_client = &client_pool;
#elif defined(__cluster_sync_client_test__)
    std::string redis_uri_list = "redis://foobared@127.0.0.1:7000;redis://foobared@127.0.0.1:7001;redis://foobared@127.0.0.1:7002";
    cluster_sync_client cluster(redis_uri_list.c_str(), 1, 2);
    base_sync_client* sync_client = &cluster;
#endif

    redis_sync_operator client(sync_client);

    // set foo bar
    client.set("foo", "bar");
    client.set("foo1", "bar1");
    client.set("foo2", "bar2");

    // get foo
    std::string res_foo;
    if (client.get("foo", res_foo))
    {
        printf("get foo: %s\n", res_foo.c_str());
    }
    else
    {
        printf("get foo: failed\n");
    }


    // exist
    bool f = client.exist("foo");
    bool f1 = client.exist("foo3");

    printf("foo exists:%d, foo3 exist2:%d\n", f, f1);

    // expire and ttl pttl
    f = client.expire("foo", 3000);
    printf("set foo expire result: %d\n", f);

    int32_t lt = client.ttl("foo");
    printf("get foo lifetime in seconds: %ds\n", lt);

    int64_t plt = client.pttl("foo");
    printf("get foo lifetime in milliseconds: %lldms\n", plt);

    lt = client.ttl("foo3");
    printf("get foo3 lifetime in seconds: %ds\n", lt);

    lt = client.ttl("foo2");
    printf("get foo2 lifetime in seconds: %ds\n", lt);

    f = client.pexpire("foo", 400000);
    printf("set foo pexpire result: %d\n", f);

    lt = client.ttl("foo");
    plt = client.pttl("foo");

    printf("get foo lifetime %ds, %lldms\n", lt, plt);

    // key pattern
    std::vector<std::string> keys;
    f = client.keys_pattern("*", keys);
    printf("keys: %d\n", keys.size());
    for (std::size_t i = 0; i < keys.size(); ++i)
    {
        printf("%d) %s\n", i + 1, keys[i].c_str());
    }

    printf("\n");

    // persist
    f = client.persist("foo");
    printf("persist key foo result %d\n", f);

    // random key
    std::string random_key;

    f = client.randomkey(random_key);

    printf("randomkey result: %d, key:%s\n", f, random_key.c_str());

    // rename key
    f = client.rename("aa", "bb");

    printf("raname aa to bb result :%d\n", f);

    f = client.renamenx("bb", "aa");

    printf("renamenx bb to aa reesult: %d\n", f);

    redis_key_type type = client.type("fzt");
    printf("key fzt type: %d\n", type);

    // scan 
    std::vector<std::string> out_elements;
    uint64_t next_cursor = client.scan(0, out_elements, nullptr, nullptr);

    printf("scan result, next_cursor[%llu], out_elements size[%d]\n", next_cursor, out_elements.size());
    for (std::size_t i = 0; i < out_elements.size(); ++i)
    {
        printf("%d) %s\n", i + 1, out_elements[i].c_str());
    }

    // del
    int32_t del_count = client.del("foo2");
    f1 = client.exist("foo2");
    printf("delete foo2 count[%d], exists[%d] now\n", del_count, f1);

    system("pause");

    pool.wait_for_stop();

    //sync_client->destroy();
}

```

### 3. redis string 相关操作的接口使用

```
void redis_string_test(){

    using namespace redis_cpp;
    using namespace redis_cpp::detail;

    utility::asio_base::thread_pool pool(2);
    pool.start();

    std::string redis_uri = "redis://foobared@127.0.0.1:6379/1";

#if defined(__standalone_sync_client_test__)
    standalone_sync_client* sync_client = new standalone_sync_client(pool.io_service(), redis_uri.c_str());
    sync_client->connect();
#elif defined(__standalone_sync_client_pool_test__)
    standalone_sync_client_pool client_pool(redis_uri.c_str(), 1, 2, &pool);
    base_sync_client* sync_client = &client_pool;
#elif defined(__cluster_sync_client_test__)
    std::string redis_uri_list = "redis://foobared@127.0.0.1:7000;redis://foobared@127.0.0.1:7001;redis://foobared@127.0.0.1:7002";
    cluster_sync_client cluster(redis_uri_list.c_str(), 1, 2);
    base_sync_client* sync_client = &cluster;
#endif

    redis_sync_operator client(sync_client);

    // set
    client.set("fuzuotao", "isgod");

    // get
    std::string out_value;
    client.get("fuzuotao", out_value);
    printf("get fuzuotao: %s\n", out_value.c_str());
    out_value.clear();
    client.get("fuzuotao1", out_value);
    printf("get fuzuotao1: %s\n", out_value.c_str());

    // append
    int32_t len = client.append("fuzuotao", "hello", 7);
    printf("fuzuotao' value len :%d after append\n", len);

    client.get("fuzuotao", out_value);
    printf("get fuzuotao: %s\n", out_value.c_str());

    // mset 
    std::unordered_map<std::string, std::string> kv_pairs;
    kv_pairs["s1"] = "sss1";
    kv_pairs["s2"] = "sss2";
    kv_pairs["s3"] = "sss3";
    bool f = client.mset(kv_pairs);
    printf("mset result: %d\n", f);

    // mget
    std::vector<std::string> out_value_list;
    std::vector<std::string> keys;
    keys.push_back("s1");
    keys.push_back("s2");
    keys.push_back("s3");
    f = client.mget(keys, out_value_list);
    printf("mget result: %d, out_value_list_size: %d\n", f, out_value_list.size());
    for (std::size_t i = 0; i < out_value_list.size(); ++i)
    {
        printf("%d)[%s] %s\n", i + 1, keys[i].c_str(), out_value_list[i].c_str());
    }

    // msetnx
    f = client.msetnx(kv_pairs);
    printf("msetnx with exist key result: %d\n", f);

    keys.clear();
    keys.push_back("s4");
    keys.push_back("s5");
    int32_t del_count = client.del(keys);
    printf("del count :%d\n", del_count);

    kv_pairs.clear();
    kv_pairs["s4"] = "sss4";
    kv_pairs["s5"] = "sss5";
    f = client.msetnx(kv_pairs);
    printf("msetnx with not exit key result: %d\n", f);

    out_value_list.clear();
    f = client.mget(keys, out_value_list);
    printf("mget result: %d, out_value_list_size: %d\n", f, out_value_list.size());
    for (std::size_t i = 0; i < out_value_list.size(); ++i)
    {
        printf("%d)[%s] %s\n", i + 1, keys[i].c_str(), out_value_list[i].c_str());
    }

    // setex
    f = client.setex("fuzuotao", "fzt", 3000);
    printf("setex fuzuotao result :%d\n", f);
    int32_t lt = client.ttl("fuzuotao");
    printf("key fuzutoao lifetime :%ds\n", lt);

    // psetex
    f = client.psetex("fuzuotao", "fzt001", 4000000);
    printf("psetex fuzuotao result: %d\n", f);
    int64_t plf = client.pttl("fuzuotao");
    printf("key fuzuotao lifetime: %lldms\n", plf);


    // setnx
    client.del("xiaoming");
    f = client.setnx("xiaoming", "2b");
    out_value.clear();
    bool f1 = client.get("xiaoming", out_value);
    printf("setnx not exist key[xiaoming] result:%d, value: %s\n", f, out_value.c_str());

    f = client.setnx("fuzuotao", "sanhaoqingning");
    printf("setnx exist key[fuzuotao] result: %d.\n", f);


    // set bit 
    int32_t old_bit = client.setbit("bittest", 10, 1);
    printf("setbit [bittest] oldbit: %d\n", old_bit);
    int32_t bit = 0;
    f = client.getbit("bittest", 10, bit);
    printf("get bit result, %d, bit[%d]\n", f, bit);

    // bit count
    int32_t bit_count = 0;
    f = client.bitcount("bittest", bit_count);
    printf("getbitcount [bittest], result[%d], count[%d]\n", f, bit_count);

    // setrange
    client.del("rangetest");
    client.set("rangetest", "rrrrr");
    int32_t new_str_len = client.setrange("rangetest", 7, "tttt");

    out_value.clear();
    client.get("rangetest", out_value);
    printf("setrange [rangetest] result[%d].\n", f);
    printf("new_value:");
    printString(out_value);

    // get range
    std::string sub_str;
    f = client.getrange("rangetest", 6, 10, sub_str);
    printf("getrange [rangetest] 6-10, result[%d].\n", f);
    printf("substr:");
    printString(sub_str);

    // increase decrease
    client.del("numbertest");
    client.set("numbertest", "10");
    int32_t newnumb = client.incr("numbertest");
    printf("incr [numbertest] new value: %d\n", newnumb);
    newnumb = client.incrby("numbertest", 20);
    printf("incrby 20 [numbertest] new value: %d\n", newnumb);
    newnumb = client.decr("numbertest");
    printf("decr [numbertest] new value: %d\n", newnumb);
    newnumb = client.decrby("numbertest", 5);
    printf("decrby 5 [numbertest] new value: %d\n", newnumb);

    double new_float = 0;
    f = client.incrbyfloat("numbertest", 11.5, &new_float);
    printf("decrbyfloat 11.5 [numbertest] new_value: %f\n", new_float);

    system("pause");

    pool.wait_for_stop();

    //sync_client->destroy();
}
```

### 4. redis list 相关操作
```
void redis_list_test(){

    using namespace redis_cpp;
    using namespace redis_cpp::detail;

    utility::asio_base::thread_pool pool(2);
    pool.start();

    std::string redis_uri = "redis://foobared@127.0.0.1:6379/1";

#if defined(__standalone_sync_client_test__)
    standalone_sync_client* sync_client = new standalone_sync_client(pool.io_service(), redis_uri.c_str());
    sync_client->connect();
#elif defined(__standalone_sync_client_pool_test__)
    standalone_sync_client_pool client_pool(redis_uri.c_str(), 1, 2, &pool);
    base_sync_client* sync_client = &client_pool;
#elif defined(__cluster_sync_client_test__)
    std::string redis_uri_list = "redis://foobared@127.0.0.1:7000;redis://foobared@127.0.0.1:7001;redis://foobared@127.0.0.1:7002";
    cluster_sync_client cluster(redis_uri_list.c_str(), 1, 2);
    base_sync_client* sync_client = &cluster;
#endif

    redis_sync_operator client(sync_client);

    // lpush, llen
    std::string tlist = "test_list";
    std::string tlist1 = "test_list1";
    client.del(tlist.c_str());
    client.del(tlist1.c_str());

    int32_t len = client.lpush(tlist.c_str(), "f1");
    printf("[%s] len[%d] after lpush.\n", tlist.c_str(), len);

    std::vector<std::string> values;
    values.push_back("f2");
    values.push_back("f3");
    len = client.lpush(tlist.c_str(), values);
    int32_t len1 = client.llen(tlist.c_str());
    printf("[%s] len[%d] len1[%d] after lpush 2 elements.\n", tlist.c_str(), len, len1);

    // lpushx
    len = client.lpushx(tlist.c_str(), "f4");
    printf("[%s] len[%d] after lpushx.\n", tlist.c_str(), len);

    len = client.lpushx(tlist1.c_str(), "f5");
    printf("not exist list[%s] len[%d] after lpushx.\n", tlist1.c_str(), len);

    // lrange
    std::vector<std::string> out_values;
    bool f = client.lrange(tlist.c_str(), 0, -1, out_values);
    printf("[%s] lrange result[%d], out_ele_count[%d].\n", tlist.c_str(), f, out_values.size());

    for (std::size_t i = 0; i < out_values.size(); ++i)
    {
        printf("%d) %s.\n", i + 1, out_values[i].c_str());
    }

    // rpush
    len = client.rpush(tlist.c_str(), "f10");
    printf("[%s] len[%d] after rpush.\n", tlist.c_str(), len);

    values.clear();
    values.push_back("f11");
    values.push_back("f12");
    len = client.rpush(tlist.c_str(), values);
    printf("[%s] len[%d] after rpush 2 elements.\n", tlist.c_str(), len);

    // rpushx
    len = client.rpushx(tlist.c_str(), "f13");
    printf("[%s] len[%d] after rpushx.\n", tlist.c_str(), len);

    len = client.rpushx(tlist1.c_str(), "f111");
    printf("not exist list[%s] len[%d] after rpushx.\n", tlist1.c_str(), len);

    // lindex
    std::string element;
    int32_t index = 1;
    f = client.lindex(tlist.c_str(), index, element);
    printf("[%s] element at index[%d] is %s, result[%d]\n", tlist.c_str(), index, element.c_str(), f);

    // lset
    f = client.lset(tlist.c_str(), index, "fxxxx");
    client.lindex(tlist.c_str(), index, element);
    printf("[%s] element at index[%d] after reset is [%s], result[%d]\n", tlist.c_str(), index, element.c_str(), f);

    // insert before
    len = client.insert_before(tlist.c_str(), "f1", "bf1");
    printf("[%s] len[%d] after insert before [f1].\n", tlist.c_str(), len);

    // insert after
    len = client.insert_after(tlist.c_str(), "f1", "af1");
    printf("[%s] len[%d] after insert after [f1].\n", tlist.c_str(), len);

    printf("current list dump....\n");
    out_values.clear();
    client.lrange(tlist.c_str(), 0, -1, out_values);
    for (std::size_t i = 0; i < out_values.size(); ++i)
    {
        printf("%d) %s.\n", i + 1, out_values[i].c_str());
    }
    printf("----------------------------\n");

    // lpop
    std::string poped_value;
    f = client.lpop(tlist.c_str(), &poped_value);
    printf("[%s] len[%d] after lpop, poped_value[%s], result[%d]\n", tlist.c_str(), client.llen(tlist.c_str()), poped_value.c_str(), f);
    f = client.lpop(tlist1.c_str(), nullptr);
    printf("[%s] len[%d] after lpop, result[%d].\n", tlist1.c_str(), client.llen(tlist1.c_str()), f);

    // rpop
    f = client.rpop(tlist.c_str(), &poped_value);
    printf("[%s] len[%d] after rpop, poped_value[%s], result[%d]\n", tlist.c_str(), client.llen(tlist.c_str()), poped_value.c_str(), f);
    f = client.rpop(tlist1.c_str(), nullptr);
    printf("[%s] len[%d] after rpop, result[%d].\n", tlist1.c_str(), client.llen(tlist1.c_str()), f);

    printf("current list dump....\n");
    out_values.clear();
    client.lrange(tlist.c_str(), 0, -1, out_values);
    for (std::size_t i = 0; i < out_values.size(); ++i)
    {
        printf("%d) %s.\n", i + 1, out_values[i].c_str());
    }
    printf("----------------------------\n");

    // lrem
    len = client.lrem(tlist.c_str(), 0, "f1");
    printf("[%s] len[%d] after remove [f1].\n", tlist.c_str(), len);

    printf("current list dump....\n");
    out_values.clear();
    client.lrange(tlist.c_str(), 0, -1, out_values);
    for (std::size_t i = 0; i < out_values.size(); ++i)
    {
        printf("%d) %s.\n", i + 1, out_values[i].c_str());
    }
    printf("----------------------------\n");

    // ltrim
    f = client.ltrim(tlist.c_str(), 3, 7);
    printf("[%s] trim result[%d] [3-7].\n", tlist.c_str(), f);

    printf("current list dump....\n");
    out_values.clear();
    client.lrange(tlist.c_str(), 0, -1, out_values);
    for (std::size_t i = 0; i < out_values.size(); ++i)
    {
        printf("%d) %s.\n", i + 1, out_values[i].c_str());
    }
    printf("----------------------------\n");

    // blpop
    std::vector<std::string> keys;
    keys.push_back(tlist);
    std::pair<std::string, std::string> result;
    f = client.blpop(keys, 5, result);
    printf("[%s] blpop result[%d], out[%s:%s]\n", tlist.c_str(), f, result.first.c_str(), result.second.c_str());

    f = client.brpop(keys, 5, result);
    printf("[%s] brpop result[%d], out[%s:%s]\n", tlist.c_str(), f, result.first.c_str(), result.second.c_str());
    printf("current list dump....\n");
    out_values.clear();
    client.lrange(tlist.c_str(), 0, -1, out_values);
    for (std::size_t i = 0; i < out_values.size(); ++i)
    {
        printf("%d) %s.\n", i + 1, out_values[i].c_str());
    }
    printf("----------------------------\n");

    // rpoplpush
    f = client.rpoplpush(tlist.c_str(), tlist1.c_str(), &poped_value);
    printf("[%s] rpoplpush result[%d]\n", tlist.c_str(), f);

    printf("current list dump....\n");
    out_values.clear();
    client.lrange(tlist.c_str(), 0, -1, out_values);
    for (std::size_t i = 0; i < out_values.size(); ++i)
    {
        printf("%d) %s.\n", i + 1, out_values[i].c_str());
    }
    printf("----------------------------\n");

    len = client.llen(tlist1.c_str());
    printf("[%s] len[%d]\n", tlist1.c_str(), len);
    out_values.clear();
    client.lrange(tlist1.c_str(), 0, -1, out_values);
    for (std::size_t i = 0; i < out_values.size(); ++i)
    {
        printf("%d) %s.\n", i + 1, out_values[i].c_str());
    }

    len = client.llen(tlist.c_str());
    for (int32_t i = 0; i < len; ++i)
    {
        client.rpop(tlist.c_str(), nullptr);
    }

    len = client.llen(tlist.c_str());
    printf("[%s] now len[%d]\n", tlist.c_str(), len);
    // brpop, blpop, brpoplpush
    keys.clear();
    keys.push_back(tlist);
    result.first = "";
    result.second = "";
    client.brpop(keys, 5, result);
    printf("[%s] brpop finish, result[%s:%s]\n", tlist.c_str(), result.first.c_str(), result.second.c_str());

    result.first = "";
    result.second = "";
    client.blpop(keys, 5, result);
    printf("[%s] blpop finish, result[%s:%s]\n", tlist.c_str(), result.first.c_str(), result.second.c_str());

    result.first = "";
    result.second = "";
    client.brpoplpush(tlist.c_str(), tlist1.c_str(), 5, &result);
    printf("[%s] brpoplpush finish, result[%s:%s]\n", tlist.c_str(), result.first.c_str(), result.second.c_str());

    system("pause");

    pool.wait_for_stop();

    //sync_client->destroy();
}
```

### 5. redis hash 相关操作
```
void redis_hash_test(){
    using namespace redis_cpp;
    using namespace redis_cpp::detail;

    utility::asio_base::thread_pool pool(2);
    pool.start();

    std::string redis_uri = "redis://foobared@127.0.0.1:6379/1";

#if defined(__standalone_sync_client_test__)
    standalone_sync_client* sync_client = new standalone_sync_client(pool.io_service(), redis_uri.c_str());
    sync_client->connect();
#elif defined(__standalone_sync_client_pool_test__)
    standalone_sync_client_pool client_pool(redis_uri.c_str(), 1, 2, &pool);
    base_sync_client* sync_client = &client_pool;
#elif defined(__cluster_sync_client_test__)
    std::string redis_uri_list = "redis://foobared@127.0.0.1:7000;redis://foobared@127.0.0.1:7001;redis://foobared@127.0.0.1:7002";
    cluster_sync_client cluster(redis_uri_list.c_str(), 1, 2);
    base_sync_client* sync_client = &cluster;
#endif

    redis_sync_operator client(sync_client);

    // del testhash
    std::string thash = "testhash";

    client.del(thash.c_str());

    // hset
    int32_t res = client.hset(thash.c_str(), "f1", "v1");
    int32_t res1 = client.hset(thash.c_str(), "f2", "v2");

    int32_t hlen = client.hlen(thash.c_str());

    printf("[%s] set field[f1], [f2] result[%d], [%d], current_len[%d].\n", thash.c_str(), res, res1, hlen);

    // hsetnx
    bool f = client.hsetnx(thash.c_str(), "f1", "v1_x");
    printf("[%s] set exists field[f1] result[%d], current_len[%d].\n", thash.c_str(), f, client.hlen(thash.c_str()));

    f = client.hsetnx(thash.c_str(), "f3", "v3");
    printf("[%s] set not exists field[f1] result[%d], current_len[%d].\n", thash.c_str(), f, client.hlen(thash.c_str()));

    // hget
    std::string v1;
    std::string v2;

    f = client.hget(thash.c_str(), "f1", v1);
    bool f1 = client.hget(thash.c_str(), "f2", v2);
    printf("[%s] get field[f1],[f2] result[%d], [%d], vaule[%s],[%s].\n", thash.c_str(), f, f1, v1.c_str(), v2.c_str());

    // hmset
    std::unordered_map<std::string, std::string> input_map;
    input_map["f4"] = "v4";
    input_map["f5"] = "v5";
    input_map["f6"] = "v6";

    f = client.hmset(thash.c_str(), input_map);
    printf("[%s] mset field[f4][f5][f6], result[%d], current_len[%d].\n", thash.c_str(), f, client.hlen(thash.c_str()));

    // hmget
    std::vector<std::string> out_values;
    std::vector<std::string> input_fields;
    input_fields.push_back("f4");
    input_fields.push_back("f5");
    input_fields.push_back("f6");

    f = client.hmget(thash.c_str(), input_fields, out_values);
    printf("[%s] mget field[f4][f5][f6], result[%d], out_values_size[%d].\n", thash.c_str(), f, out_values.size());
    for (std::size_t i = 0; i < out_values.size(); ++i)
    {
        printf("<%s> %s.\n", input_fields[i].c_str(), out_values[i].c_str());
    }

    printf("-------------------------------------\n");

    // hkeys
    std::vector <std::string> out_keys;
    f = client.hkeys(thash.c_str(), out_keys);
    printf("[%s] hkeys result[%d], size[%d].\n", thash.c_str(), f, out_keys.size());
    for (std::size_t i = 0; i < out_keys.size(); ++i)
    {
        printf("%d) %s.\n", i + 1, out_keys[i].c_str());
    }

    printf("--------------------------------------\n");

    // hvals
    out_values.clear();
    f = client.hvals(thash.c_str(), out_values);
    printf("[%s] hvals result[%d], size[%d].\n", thash.c_str(), f, out_values.size());
    for (std::size_t i = 0; i < out_values.size(); ++i)
    {
        printf("%d) %s.\n", i + 1, out_values[i].c_str());
    }

    // hgetall
    std::unordered_map<std::string, std::string> out_map;
    f = client.hgetall(thash.c_str(), out_map);
    printf("[%s] hgetall result[%d], size[%d].\n", thash.c_str(), f, out_map.size());
    auto result_iter = out_map.begin();
    for (result_iter; result_iter != out_map.end(); ++result_iter)
    {
        printf("<%s> %s.\n", result_iter->first.c_str(), result_iter->second.c_str());
    }

    // hincryby
    res = client.hset(thash.c_str(), "f10", "10");
    int32_t out_int_new_value;
    f = client.hincrby(thash.c_str(), "f10", 5, &out_int_new_value);
    printf("[%s] field[f10] value[10] hincrby 5, result[%d], new_value[%d].\n", thash.c_str(), f, out_int_new_value);

    // hincrybyfloat
    res = client.hset(thash.c_str(), "f11", "10.3");
    double out_float_new_value;
    f = client.hincrbyfloat(thash.c_str(), "f11", 5.5, &out_float_new_value);
    printf("[%s] field[f11] value[10.3] hincrybyfloat 5.5, result[%d]. new_value[%f].\n", thash.c_str(), f, out_float_new_value);

    // hexists
    f = client.hexists(thash.c_str(), "f10");
    printf("[%s] filed[f10] exists[%d].\n", thash.c_str(), f);


    // hscan
    out_map.clear();
    uint64_t next_cursor = client.hscan(thash.c_str(), 0, out_map, nullptr, nullptr);
    printf("[%s] hscan, next_curor[%llu], out_result_size[%d].\n", thash.c_str(), next_cursor, out_map.size());
    result_iter = out_map.begin();
    for (result_iter; result_iter != out_map.end(); ++result_iter)
    {
        printf("<%s> %s.\n", result_iter->first.c_str(), result_iter->second.c_str());
    }

    system("pause");

    pool.wait_for_stop();

    //sync_client->destroy();
}

```

### 6. redis set 相关操作

```
void redis_set_test(){
    using namespace redis_cpp;
    using namespace redis_cpp::detail;

    utility::asio_base::thread_pool pool(2);
    pool.start();

    std::string redis_uri = "redis://foobared@127.0.0.1:6379/1";

#if defined(__standalone_sync_client_test__)
    standalone_sync_client* sync_client = new standalone_sync_client(pool.io_service(), redis_uri.c_str());
    sync_client->connect();
#elif defined(__standalone_sync_client_pool_test__)
    standalone_sync_client_pool client_pool(redis_uri.c_str(), 1, 2, &pool);
    base_sync_client* sync_client = &client_pool;
#elif defined(__cluster_sync_client_test__)
    std::string redis_uri_list = "redis://foobared@127.0.0.1:7000;redis://foobared@127.0.0.1:7001;redis://foobared@127.0.0.1:7002";
    cluster_sync_client cluster(redis_uri_list.c_str(), 1, 2);
    base_sync_client* sync_client = &cluster;
#endif

    redis_sync_operator client(sync_client);

    // sadd
    std::string tset = "testset";
    std::string tset1 = "testset1";
    std::string diff_dest = "diffdest";
    std::string inter_dest = "interstore";
    std::string union_dest = "union_dest";

    client.del(tset.c_str());
    client.del(tset1.c_str());
    client.del(diff_dest.c_str());
    client.del(inter_dest.c_str());
    client.del(union_dest.c_str());

    int32_t add_count = client.sadd(tset.c_str(), "s1");
    printf("[%s] sadd [s1], add_count[%d].\n", tset.c_str(), add_count);

    std::vector<std::string> add_values;
    add_values.push_back("s2");
    add_values.push_back("s3");
    add_values.push_back("s4");
    add_count = client.sadd(tset.c_str(), add_values);
    printf("[%s] sadd[s2][s3][s4], add_count[%d], current_size[%d].\n", tset.c_str(), add_count, client.scard(tset.c_str()));

    // spop
    std::string poped_value;
    bool f = client.spop(tset.c_str(), &poped_value);
    printf("[%s] spop result[%d], poped_value[%s], current_size[%d].\n", tset.c_str(), f, poped_value.c_str(), client.scard(tset.c_str()));

    // srandmember
    poped_value.clear();
    f = client.srandmember(tset.c_str(), poped_value);
    printf("[%s] srandmember result[%d], poped_value[%s], current_size[%d].\n", tset.c_str(), f, poped_value.c_str(), client.scard(tset.c_str()));

    std::vector<std::string> poped_values;
    f = client.srandmember(tset.c_str(), 2, poped_values);
    printf("[%s] srandmemer 2 member, result[%d], poped ele count[%d], current_size[%d]\n", tset.c_str(), f, poped_values.size(), client.scard(tset.c_str()));
    for (std::size_t i = 0; i < poped_values.size(); ++i)
    {
        printf("%d) %s\n", i + 1, poped_values[i].c_str());
    }

    printf("________________________________\n");

    // smembers
    add_values.push_back("s1");
    add_count = client.sadd(tset.c_str(), add_values);

    std::vector<std::string> out_values;
    f = client.smembers(tset.c_str(), out_values);
    printf("[%s] smember result[%d], out_size[%d]\n", tset.c_str(), f, out_values.size());
    for (std::size_t i = 0; i < out_values.size(); ++i)
    {
        printf("%d) %s\n", i + 1, out_values[i].c_str());
    }

    printf("________________________________\n");

    // sismember
    f = client.sismember(tset.c_str(), "s1");
    printf("[%s] value[s1] exists[%d].\n", tset.c_str(), f);

    f = client.sismember(tset.c_str(), "s11");
    printf("[%s] value[s11] exists[%d].\n", tset.c_str(), f);

    // s2
    std::vector<std::string> add_values2;
    add_values2.push_back("ss1");
    add_values2.push_back("ss2");
    add_values2.push_back("ss5");

    client.sadd(tset1.c_str(), add_values2);

    // sdiff
    std::vector<std::string> diff_set;
    f = client.sdiff(tset.c_str(), tset1.c_str(), diff_set);
    printf("sdiff [%s][%s], result[%d], diff_set_size[%d]\n",
        tset.c_str(), tset1.c_str(), f, diff_set.size());

    printf("diff set size[%d]\n", diff_set.size());
    for (std::size_t i = 0; i < diff_set.size(); ++i)
    {
        printf("%d) %s\n", i + 1, diff_set[i].c_str());
    }
    printf("----------------------------\n");

    // sdiffstore
    int32_t diff_count = client.sdiffstore(diff_dest.c_str(), tset.c_str(), tset1.c_str());
    printf("sdiffstore [%s][%s], diff_count[%d]\n", tset.c_str(), tset1.c_str(), diff_count);
    out_values.clear();
    client.smembers(diff_dest.c_str(), out_values);
    printf("diff set, size[%d]\n", out_values.size());
    for (std::size_t i = 0; i < out_values.size(); ++i)
    {
        printf("%d) %s\n", i + 1, diff_set[i].c_str());
    }
    printf("----------------------------\n");

    // sinter
    std::vector<std::string> inter_set;
    f = client.sinter(tset.c_str(), tset1.c_str(), inter_set);
    printf("sinter [%s][%s], result[%d], inter_set_size[%d]\n",
        tset.c_str(), tset1.c_str(), f, inter_set.size());
    for (std::size_t i = 0; i < inter_set.size(); ++i)
    {
        printf("%d) %s\n", i + 1, inter_set[i].c_str());
    }
    printf("----------------------------\n");

    // sinterstore
    int32_t inter_set_count = client.sinterstore(inter_dest.c_str(), tset.c_str(), tset1.c_str());
    printf("sinterstore [%s][%s], interset_size[%d].\n", tset.c_str(), tset1.c_str(), inter_set_count);

    out_values.clear();
    client.smembers(inter_dest.c_str(), out_values);
    printf("inter set, size[%d].\n", out_values.size());
    for (std::size_t i = 0; i < out_values.size(); ++i)
    {
        printf("%d) %s\n", i + 1, out_values[i].c_str());
    }
    printf("----------------------------\n");

    // sunion
    std::vector<std::string> union_set;
    f = client.sunion(tset.c_str(), tset1.c_str(), union_set);
    printf("sunion [%s][%s], result[%d], union_set_size[%d]\n",
        tset.c_str(), tset1.c_str(), f, union_set.size());

    for (std::size_t i = 0; i < union_set.size(); ++i)
    {
        printf("%d) %s\n", i + 1, union_set[i].c_str());
    }
    printf("----------------------------\n");

    // sunionstore
    out_values.clear();
    int32_t union_set_count = client.sunionstore(union_dest.c_str(), tset.c_str(), tset1.c_str());
    printf("sunionstore [%s][%s], union_set_count[%d].\n", tset.c_str(), tset1.c_str(), union_set_count);

    client.smembers(union_dest.c_str(), out_values);

    printf("union set, size[%d].\n", out_values.size());
    for (std::size_t i = 0; i < out_values.size(); ++i)
    {
        printf("%d) %s\n", i + 1, out_values[i].c_str());
    }
    printf("----------------------------\n");

    system("pause");

    system("pause");

    pool.wait_for_stop();

    //sync_client->destroy();
}
```

### 7. redis zset 相关操作
```
void redis_zset_test(){
    using namespace redis_cpp;
    using namespace redis_cpp::detail;

    utility::asio_base::thread_pool pool(2);
    pool.start();

    std::string redis_uri = "redis://foobared@127.0.0.1:6379/1";

#if defined(__standalone_sync_client_test__)
    standalone_sync_client* sync_client = new standalone_sync_client(pool.io_service(), redis_uri.c_str());
    sync_client->connect();
#elif defined(__standalone_sync_client_pool_test__)
    standalone_sync_client_pool client_pool(redis_uri.c_str(), 1, 2, &pool);
    base_sync_client* sync_client = &client_pool;
#elif defined(__cluster_sync_client_test__)
    std::string redis_uri_list = "redis://foobared@127.0.0.1:7000;redis://foobared@127.0.0.1:7001;redis://foobared@127.0.0.1:7002";
    cluster_sync_client cluster(redis_uri_list.c_str(), 1, 2);
    base_sync_client* sync_client = &cluster;
#endif

    redis_sync_operator client(sync_client);

    // add
    std::string tzset = "testzset";
    std::string tzset1 = "testzset1";
    std::string tzset_union = "testzset_union";
    std::string tzset_inter = "testzset_inter";
    client.del(tzset.c_str());
    client.del(tzset_union.c_str());
    client.del(tzset_inter.c_str());

    int32_t added_count = client.zadd(tzset.c_str(), 100, "zs1");
    printf("[%s] add [zs1], added_count[%d], current_size[%d]\n",
        tzset.c_str(), added_count, client.zcard(tzset.c_str()));

    std::vector<std::pair<double, std::string>> add_values;
    add_values.push_back(std::make_pair(10, "zs10"));
    add_values.push_back(std::make_pair(15, "zs15"));
    add_values.push_back(std::make_pair(5, "zs5"));
    add_values.push_back(std::make_pair(20, "zs20"));
    add_values.push_back(std::make_pair(20, "ab20"));

    added_count = client.zadd(tzset.c_str(), add_values);
    printf("[%s] add [zs5][zs10][zs15], added_cout[%d], current_size[%d]\n",
        tzset.c_str(), added_count, client.zcard(tzset.c_str()));

    std::vector<std::pair<double, std::string>> add_values1;
    add_values1.push_back(std::make_pair(100, "zs100"));
    add_values1.push_back(std::make_pair(105, "zs105"));
    add_values1.push_back(std::make_pair(115, "zs5"));
    client.zadd(tzset1.c_str(), add_values1);

    // zcount
    int32_t count = client.zcount(tzset.c_str(), 10, 1001);
    printf("[%s] score at[10, 1001] element count[%d].\n", tzset.c_str(), count);


    // zrange
    std::vector<std::string> out_result_no_scores;
    bool f = client.zrange(tzset.c_str(), 0, -1, out_result_no_scores);
    printf("[%s] zrange at [0, -1] result[%d], out_result_size[%d]\n", tzset.c_str(), f, out_result_no_scores.size());
    for (std::size_t i = 0; i < out_result_no_scores.size(); ++i)
    {
        printf("%d) %s.\n", i + 1, out_result_no_scores[i].c_str());
    }

    out_result_no_scores.clear();
    f = client.zrange(tzset.c_str(), 0, 1, out_result_no_scores);
    printf("[%s] zrange at [0, 1] result[%d], out_result_size[%d]\n", tzset.c_str(), f, out_result_no_scores.size());
    for (std::size_t i = 0; i < out_result_no_scores.size(); ++i)
    {
        printf("%d) %s.\n", i + 1, out_result_no_scores[i].c_str());
    }

    // zrange with scores
    std::vector<std::pair<std::string, double>> out_result_with_score;
    f = client.zrangewithscores(tzset.c_str(), 0, -1, out_result_with_score);
    printf("[%s] zrange with score at [0, -1] result[%d], out_result_size[%d]\n", tzset.c_str(), f, out_result_with_score.size());
    for (std::size_t i = 0; i < out_result_with_score.size(); ++i)
    {
        printf("%d) %s : %f.\n", i + 1, out_result_with_score[i].first.c_str(), out_result_with_score[i].second);
    }

    out_result_with_score.clear();
    f = client.zrangewithscores(tzset.c_str(), 0, 1, out_result_with_score);
    printf("[%s] zrange with score at [0, 1] result[%d], out_result_size[%d]\n", tzset.c_str(), f, out_result_with_score.size());
    for (std::size_t i = 0; i < out_result_with_score.size(); ++i)
    {
        printf("%d) %s : %f.\n", i + 1, out_result_with_score[i].first.c_str(), out_result_with_score[i].second);
    }

    // zrangebyscore
    out_result_no_scores.clear();
    f = client.zrangebyscore(tzset.c_str(), 0, 50, out_result_no_scores);
    printf("[%s] zrangebyscore at [0, 50] result[%d], out_result_size[%d]\n",
        tzset.c_str(), f, out_result_no_scores.size());
    for (std::size_t i = 0; i < out_result_no_scores.size(); ++i)
    {
        printf("%d) %s.\n", i + 1, out_result_no_scores[i].c_str());
    }

    // zrangebyscore with score
    out_result_with_score.clear();
    f = client.zrangebyscorewithscore(tzset.c_str(), 0, 10000, out_result_with_score);
    printf("[%s] zrangebyscore with scores at [0, 10000] result[%d], out_result_size[%d]\n",
        tzset.c_str(), f, out_result_with_score.size());
    for (std::size_t i = 0; i < out_result_with_score.size(); ++i)
    {
        printf("%d) %s : %f.\n", i + 1, out_result_with_score[i].first.c_str(), out_result_with_score[i].second);
    }

    // zrank
    int32_t out_rank = 0;
    f = client.zrank(tzset.c_str(), "zs10", out_rank);
    printf("[%s] zrank member[zs10] result[%d], out_rank[%d]\n",
        tzset.c_str(), f, out_rank);

    printf("revert........\n");

    // zrev range
    out_result_no_scores.clear();
    f = client.zrevrange(tzset.c_str(), 0, -1, out_result_no_scores);
    printf("[%s] zrevrange at [0, -1] result[%d], out_result_size[%d]\n", tzset.c_str(), f, out_result_no_scores.size());
    for (std::size_t i = 0; i < out_result_no_scores.size(); ++i)
    {
        printf("%d) %s.\n", i + 1, out_result_no_scores[i].c_str());
    }

    out_result_no_scores.clear();
    f = client.zrevrange(tzset.c_str(), 0, 1, out_result_no_scores);
    printf("[%s] zrevrange at [0, 1] result[%d], out_result_size[%d]\n", tzset.c_str(), f, out_result_no_scores.size());
    for (std::size_t i = 0; i < out_result_no_scores.size(); ++i)
    {
        printf("%d) %s.\n", i + 1, out_result_no_scores[i].c_str());
    }

    // zrevrange with scores
    out_result_with_score.clear();
    f = client.zrevrangewithscores(tzset.c_str(), 0, -1, out_result_with_score);
    printf("[%s] zrevrange with score at [0, -1] result[%d], out_result_size[%d]\n", tzset.c_str(), f, out_result_with_score.size());
    for (std::size_t i = 0; i < out_result_with_score.size(); ++i)
    {
        printf("%d) %s : %f.\n", i + 1, out_result_with_score[i].first.c_str(), out_result_with_score[i].second);
    }

    out_result_with_score.clear();
    f = client.zrevrangewithscores(tzset.c_str(), 0, 1, out_result_with_score);
    printf("[%s] zrevrange with score at [0, 1] result[%d], out_result_size[%d]\n", tzset.c_str(), f, out_result_with_score.size());
    for (std::size_t i = 0; i < out_result_with_score.size(); ++i)
    {
        printf("%d) %s : %f.\n", i + 1, out_result_with_score[i].first.c_str(), out_result_with_score[i].second);
    }

    // zrevrangebyscore
    out_result_no_scores.clear();
    f = client.zrevrangebyscore(tzset.c_str(), 50, 0, out_result_no_scores);
    printf("[%s] zrevrangebyscore at [50, 0] result[%d], out_result_size[%d]\n",
        tzset.c_str(), f, out_result_no_scores.size());
    for (std::size_t i = 0; i < out_result_no_scores.size(); ++i)
    {
        printf("%d) %s.\n", i + 1, out_result_no_scores[i].c_str());
    }

    // zrevrangebyscore with score
    out_result_with_score.clear();
    f = client.zrevrangebyscorewithscore(tzset.c_str(), 10000, 0, out_result_with_score);
    printf("[%s] zrevrangebyscore with scores [10000, 0] result[%d], out_result_size[%d]\n",
        tzset.c_str(), f, out_result_with_score.size());
    for (std::size_t i = 0; i < out_result_with_score.size(); ++i)
    {
        printf("%d) %s : %f.\n", i + 1, out_result_with_score[i].first.c_str(), out_result_with_score[i].second);
    }

    // zrevrank
    out_rank = 0;
    f = client.zrevrank(tzset.c_str(), "zs10", out_rank);
    printf("[%s] zrevrank member[zs10] result[%d], out_rank[%d]\n",
        tzset.c_str(), f, out_rank);

    // zscore
    std::string out_score;
    f = client.zscore(tzset.c_str(), "zs1", out_score);
    printf("[%s] zscore [zs1] result[%d], out_score[%s]\n",
        tzset.c_str(), f, out_score.c_str());

    // zincrby
    std::string out_new_score;
    f = client.zincrby(tzset.c_str(), 8, "zs10", &out_new_score);
    printf("[%s] zincrby [zs10] score increase 8, result[%d], new_score[%s]\n",
        tzset.c_str(), f, out_new_score.c_str());

    out_new_score.clear();
    f = client.zincrby(tzset.c_str(), 1.5, "zs10", &out_new_score);
    printf("[%s] zincrby [zs10] score increase 1.5, result[%d], new_score[%s]\n",
        tzset.c_str(), f, out_new_score.c_str());

    // zunionstore
    std::vector<std::string> keys;
    keys.push_back(tzset);
    keys.push_back(tzset1);
    int32_t union_set_size = client.zunionstore(tzset_union.c_str(), keys, nullptr, "SUM");
    printf("[%s][%s] zunionstore, union_set_size[%d]\n", tzset.c_str(), tzset1.c_str(), union_set_size);
    out_result_with_score.clear();
    client.zrangewithscores(tzset_union.c_str(), 0, -1, out_result_with_score);
    for (std::size_t i = 0; i < out_result_with_score.size(); ++i)
    {
        printf("%d) %s : %f.\n", i + 1, out_result_with_score[i].first.c_str(), out_result_with_score[i].second);
    }

    // zinterstore
    int32_t inter_set_size = client.zinterstore(tzset_inter.c_str(), keys, nullptr, "SUM");
    printf("[%s][%s] zinterstore, inter_set_size[%d]\n", tzset.c_str(), tzset1.c_str(), inter_set_size);
    out_result_with_score.clear();
    client.zrangewithscores(tzset_inter.c_str(), 0, -1, out_result_with_score);
    for (std::size_t i = 0; i < out_result_with_score.size(); ++i)
    {
        printf("%d) %s : %f.\n", i + 1, out_result_with_score[i].first.c_str(), out_result_with_score[i].second);
    }

    // zrem
    int32_t removed_count = client.zrem(tzset.c_str(), "zs1");
    printf("[%s] zrem [zs1], removed_count[%d], curent_size[%d]\n",
        tzset.c_str(), removed_count, client.zcard(tzset.c_str()));

    std::vector<std::string> to_remove_list;
    to_remove_list.push_back("zs5");
    to_remove_list.push_back("zs10");
    to_remove_list.push_back("zs15");

    removed_count = client.zrem(tzset.c_str(), to_remove_list);
    printf("[%s] zrem [zs5][zs10][zs15], removed_count[%d], curent_size[%d]\n",
        tzset.c_str(), removed_count, client.zcard(tzset.c_str()));

    printf("dump current set...\n");
    out_result_with_score.clear();
    client.zrangewithscores(tzset.c_str(), 0, -1, out_result_with_score);
    for (std::size_t i = 0; i < out_result_with_score.size(); ++i)
    {
        printf("%d) %s : %f.\n", i + 1, out_result_with_score[i].first.c_str(), out_result_with_score[i].second);
    }

    // zrembyrank
    removed_count = client.zremrangebyrank(tzset.c_str(), 0, 1);
    printf("[%s] zrembyrank from [0, 1], remove_count[%d], current_size[%d]\n",
        tzset.c_str(), removed_count, client.zcard(tzset.c_str()));

    printf("dump current set...\n");
    out_result_with_score.clear();
    client.zrangewithscores(tzset.c_str(), 0, -1, out_result_with_score);
    for (std::size_t i = 0; i < out_result_with_score.size(); ++i)
    {
        printf("%d) %s : %f.\n", i + 1, out_result_with_score[i].first.c_str(), out_result_with_score[i].second);
    }

    // zrembyscore
    removed_count = client.zremrangebyscore(tzset.c_str(), 0, 50);
    printf("[%s] zrembyscore from [0, 50], remove_count[%d], current_size[%d]\n",
        tzset.c_str(), removed_count, client.zcard(tzset.c_str()));

    printf("dump current set...\n");
    out_result_with_score.clear();
    client.zrangewithscores(tzset.c_str(), 0, -1, out_result_with_score);
    for (std::size_t i = 0; i < out_result_with_score.size(); ++i)
    {
        printf("%d) %s : %f.\n", i + 1, out_result_with_score[i].first.c_str(), out_result_with_score[i].second);
    }


    system("pause");

    pool.wait_for_stop();

    //sync_client->destroy();
}
```

### 8. 订阅/发布相关操作
 - 同步接口
 
    ```
	void redis_pubsub_test(){
	    using namespace redis_cpp;
	    using namespace redis_cpp::detail;
	
	    utility::asio_base::thread_pool pool(2);
	    pool.start();
	
	    std::string redis_uri = "redis://foobared@127.0.0.1:6379/1";
	
	#if defined(__standalone_sync_client_test__)
	    standalone_sync_client* sync_client = new standalone_sync_client(pool.io_service(), redis_uri.c_str());
	    sync_client->connect();
	#elif defined(__standalone_sync_client_pool_test__)
	    standalone_sync_client_pool client_pool(redis_uri.c_str(), 1, 2, &pool);
	    base_sync_client* sync_client = &client_pool;
	#elif defined(__cluster_sync_client_test__)
	    std::string redis_uri_list = 
			"redis://foobared@127.0.0.1:7000;redis://foobared@127.0.0.1:7001;redis://foobared@127.0.0.1:7002";
	    cluster_sync_client cluster(redis_uri_list.c_str(), 1, 2);
	    base_sync_client* sync_client = &cluster;
	#endif
	
	    redis_sync_operator client(sync_client);
	
	    std::string cmd;
	    while (true)
	    {
	        std::cout << ">";
	
	        std::cin >> cmd;
	
	        if (cmd == "pubsub")
	        {
	            std::string p;
	            std::cin >> p;
	
	            if (p == "numsub")
	            {
	                std::string channel_name;
	                int32_t sub_count;
	
	                std::cin >> channel_name;
	
	                client.pubsub_numsub(channel_name.c_str(), sub_count);
	
	                printf("channel_name:%s, subed_count: %d\n", channel_name.c_str(), sub_count);
	            }
	            else if (p == "channels")
	            {
	                std::vector<std::string> out_result;
	                client.pubsub_channels(out_result);
	
	                printf("current active channels count:%d\n", out_result.size());
	                for (std::size_t i = 0; i < out_result.size(); ++i)
	                {
	                    printf("%d) %s\n", i + 1, out_result[i].c_str());
	                }
	            }
	            else if (p == "numpat")
	            {
	                int32_t count = client.pubsub_numpat();
	
	                printf("numpat: %d\n", count);
	            }
	
	        }
	        else if (cmd == "publish")
	        {
	            std::string channel_name;
	            std::string message;
	
	            std::cin >> channel_name;
	            std::cin >> message;
	
	            int32_t recv_count = client.publish(channel_name.c_str(), message.c_str());
	
	            printf("recved_count: %d\n", recv_count);
	        }
	    }
	
	    system("pause");
	
	    pool.wait_for_stop();
	
	    //sync_client->destroy();
	}
    ```
 - 异步接口
 
    ```
    void standalone_async_client_test(){
	    using namespace redis_cpp;
	    using namespace redis_cpp::detail;
	
	    utility::asio_base::thread_pool pool(2);
	    pool.start();
	    
		#if defined(__standalone_async_client_test__)
	    std::string redis_uri = "redis://foobared@127.0.0.1:6379/1";
	    standalone_async_client s_client(pool.io_service(), redis_uri.c_str());
	    base_async_client* client = &s_client;
		#elif defined(__cluster_async_client_tet__)
	    std::string redis_uri = 
			"redis://foobared@127.0.0.1:7000;redis://foobared@127.0.0.1:7001;redis://foobared@127.0.0.1:7002";
	    cluster_async_client c_client(redis_uri.c_str());
	    base_async_client* client = &c_client;
		#endif
	    client->connect();
	
	    std::string cmd;
	    while (true){
	        std::cout << "> ";
	        std::cin >> cmd;
	        if (cmd == "subscribe"){
	            std::string channel_name;
	            std::cin >> channel_name;
	            client->subscribe(channel_name,
	                std::bind(test_channel_message_handler, std::placeholders::_1, std::placeholders::_2),
	                std::bind(default_reply_handler, std::placeholders::_1));
	        }
	        else if (cmd == "unsubscribe"){
	            std::string channel_name;
	            std::cin >> channel_name;
	            client->unsubscribe(channel_name,
	                std::bind(default_reply_handler, std::placeholders::_1));
	        }
	        else if (cmd == "publish"){
	            std::string channel_name, message;
	            std::cin >> channel_name >> message;
	            client->publish(channel_name, message, std::bind(default_reply_handler, std::placeholders::_1));
	        }
	    }
	
	    pool.wait_for_stop();
	}
    ```

### 9. 其他命令
 - server相关命令
    
   ```
    void redis_server_cmd_test(){

	    using namespace redis_cpp;
	    using namespace redis_cpp::detail;
	
	    utility::asio_base::thread_pool pool(2);
	    pool.start();
	
	    std::string redis_uri = "redis://foobared@127.0.0.1:6379/1";
	
	#if defined(__standalone_sync_client_test__)
	    standalone_sync_client* sync_client = new standalone_sync_client(pool.io_service(), redis_uri.c_str());
	    sync_client->connect();
	#elif defined(__standalone_sync_client_pool_test__)
	    standalone_sync_client_pool client_pool(redis_uri.c_str(), 1, 2, &pool);
	    base_sync_client* sync_client = &client_pool;
	#elif defined(__cluster_sync_client_test__)
	    std::string redis_uri_list = 
			"redis://foobared@127.0.0.1:7000;redis://foobared@127.0.0.1:7001;redis://foobared@127.0.0.1:7002";
	    cluster_sync_client cluster(redis_uri_list.c_str(), 1, 2);
	    base_sync_client* sync_client = &cluster;
	#endif
	
	    redis_sync_operator client(sync_client);
	
	    while (true)
	    {
	        //system("pause");
	
	        int64_t time_server;
	        int64_t microseconds;
	
	        if (client.time(&time_server, &microseconds))
	        {
	            time_t now = time(NULL);
	            printf("time: %lld, microseconds: %lld, now[%d]\n",
	                time_server, microseconds, (int32_t)now);
	        }
	        else
	        {
	            printf("get server time failed\n");
	        }
	
	        node_role_info role_info;
	
	        if (client.role(role_info))
	        {
	            printf("role: %s\n", role_info.role.c_str());
	
	            if (role_info.role == "master")
	            {
	                std::shared_ptr<master_node_info> master = role_info.master;
	
	                printf("repli_offset: %d\n", master->replication_offset);
	                printf("slave list, count:%d\n", master->slave_list.size());
	                for (std::size_t i = 0; i < master->slave_list.size(); ++i)
	                {
	                    slave_base_info& slave_info = master->slave_list[i];
	                    printf("------------------------------------------\n");
	                    printf("ip: %s\n", slave_info.ip.c_str());
	                    printf("port: %d\n", slave_info.port);
	                    printf("replic_offset: %d\n", slave_info.replication_offset);
	
	                    printf("\n");
	                }
	            }
	            else if (role_info.role == "slave")
	            {
	                std::shared_ptr<slave_node_info> slave = role_info.slave;
	
	                printf("master_ip: %s\n", slave->master_ip.c_str());
	                printf("master_port: %d\n", slave->master_port);
	                printf("state: %s\n", slave->state.c_str());
	                printf("recv_size: %d\n", slave->recved_data_size);
	            }
	            else if (role_info.role == "sentinel")
	            {
	                std::shared_ptr<sentinel_node_info> sentinel = role_info.sentinel;
	
	                printf("monitored master list, count:%d\n", sentinel->master_name_list_monitored.size());
	                for (std::size_t i = 0; i < sentinel->master_name_list_monitored.size(); ++i)
	                {
	                    printf("i: %d, %s\n", i, sentinel->master_name_list_monitored[i].c_str());
	                }
	            }
	            else
	            {
	                printf("unsupport role type:%s\n", role_info.role.c_str());
	            }
	        }
	        else
	        {
	            printf("get role info failed\n");
	        }
	
	        std::this_thread::sleep_for(std::chrono::seconds(1));
	    }
	
	    system("pause");
	
	    pool.wait_for_stop();
	
	    //sync_client->destroy();
	}

   ```
 - cluster相关命令
 
   ```
   void redis_cluster_cmd_test(){
	    using namespace redis_cpp;
	    using namespace redis_cpp::detail;
	
	    utility::asio_base::thread_pool pool(2);
	    pool.start();
	
	    std::string redis_uri = "redis://foobared@127.0.0.1:7000/1";
	
	#if defined(__standalone_sync_client_test__)
	    standalone_sync_client* sync_client = new standalone_sync_client(pool.io_service(), redis_uri.c_str());
	    sync_client->connect();
	#elif defined(__standalone_sync_client_pool_test__)
	    standalone_sync_client_pool client_pool(redis_uri.c_str(), 1, 2, &pool);
	    base_sync_client* sync_client = &client_pool;
	#elif defined(__cluster_sync_client_test__)
	    std::string redis_uri_list = 
			"redis://foobared@127.0.0.1:7000;redis://foobared@127.0.0.1:7001;redis://foobared@127.0.0.1:7002";
	    cluster_sync_client cluster(redis_uri_list.c_str(), 1, 2);
	    base_sync_client* sync_client = &cluster;
	#endif
	
	    redis_sync_operator client(sync_client);
	
	    while (true){
	        slot_range_map_type map;
	        if (client.cluster_slots(map)){
	            printf("slot range count[%d].\n", map.size());
	            int32_t index = 0;
	            for (auto& kv : map){
	                printf("%02d, range[%d~%d].\n", index, kv.first.start_slot, kv.first.end_slot);
	                printf("    master [%s:%d].\n", kv.second.master.ip.c_str(), kv.second.master.port);
	                printf("    slaves, count[%d]\n", kv.second.slave_list.size());
	                int32_t slave_index = 0;
	                for (auto& slave : kv.second.slave_list){
	                    printf("      %02d, %s:%d.\n", slave_index, slave.ip.c_str(), slave.port);
	                    slave_index++;
	                }
	                index++;
	            }
	        }
	        else{
	            printf("parser slot range failed\n");
	        }
	
	        system("pause");
	    }
    }

   ```