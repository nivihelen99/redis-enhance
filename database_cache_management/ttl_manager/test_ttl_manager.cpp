#include "ttl_manager.h"
#include <connection_pool_manager/connection_pool_manager.h>
#include <connection_pool_manager/redis_connection_guard.h>
#include <gtest/gtest.h>
#include <vector>
#include <string>
#include <thread>
#include <hiredis/hiredis.h>
#include <chrono>

TEST(TtlManagerTest, AddAndExpire) {
    std::vector<std::string> hosts = {"127.0.0.1"};
    auto pool_manager = std::make_shared<ConnectionPoolManager>(hosts, 2); // Increased pool size for test
    TtlManager ttl_manager(pool_manager);

    {
        RedisConnectionGuard conn(pool_manager);
        // Set a key
        redisReply* reply = (redisReply*)redisCommand(conn.get(), "SET %s %s", "mykey", "myvalue");
        ASSERT_NE(reply, nullptr);
        freeReplyObject(reply);
    } // Connection is returned here

    // Now call addKey. TtlManager will acquire and release a connection.
    ttl_manager.addKey("mykey", 1); // 1 second TTL

    {
        RedisConnectionGuard conn(pool_manager);
        // Key should exist immediately
        redisReply* reply = (redisReply*)redisCommand(conn.get(), "GET %s", "mykey");
        ASSERT_NE(reply, nullptr);
        ASSERT_STREQ(reply->str, "myvalue");
        freeReplyObject(reply);
    } // Connection is returned here

    // Wait for the key to expire
    std::this_thread::sleep_for(std::chrono::seconds(2));

    {
        RedisConnectionGuard conn(pool_manager);
        // Key should be gone
        redisReply* reply = (redisReply*)redisCommand(conn.get(), "GET %s", "mykey");
        ASSERT_EQ(reply->type, REDIS_REPLY_NIL);
        freeReplyObject(reply);
    } // Connection is returned here
}
