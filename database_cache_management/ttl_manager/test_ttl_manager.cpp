#include "ttl_manager.h"
#include <connection_pool_manager/connection_pool_manager.h>
#include <gtest/gtest.h>
#include <vector>
#include <string>
#include <thread>
#include <hiredis/hiredis.h>
#include <chrono>

TEST(TtlManagerTest, AddAndExpire) {
    std::vector<std::string> hosts = {"127.0.0.1"};
    auto pool_manager = std::make_shared<ConnectionPoolManager>(hosts, 1);
    TtlManager ttl_manager(pool_manager);

    // Get a connection for setup
    auto conn_setup = pool_manager->getConnection();
    ASSERT_NE(conn_setup, nullptr);

    // Set a key
    redisReply* reply = (redisReply*)redisCommand(conn_setup, "SET %s %s", "mykey", "myvalue");
    ASSERT_NE(reply, nullptr);
    freeReplyObject(reply);
    pool_manager->returnConnection(conn_setup); // Return connection immediately

    // Now call addKey. TtlManager will acquire and release a connection.
    ttl_manager.addKey("mykey", 1); // 1 second TTL

    // Get a connection to verify
    auto conn_verify = pool_manager->getConnection();
    ASSERT_NE(conn_verify, nullptr);

    // Key should exist immediately
    reply = (redisReply*)redisCommand(conn_verify, "GET %s", "mykey");
    ASSERT_NE(reply, nullptr);
    ASSERT_STREQ(reply->str, "myvalue");
    freeReplyObject(reply);

    // Wait for the key to expire
    std::this_thread::sleep_for(std::chrono::seconds(2));

    // Key should be gone
    reply = (redisReply*)redisCommand(conn_verify, "GET %s", "mykey");
    ASSERT_EQ(reply->type, REDIS_REPLY_NIL);
    freeReplyObject(reply);

    pool_manager->returnConnection(conn_verify);
}
