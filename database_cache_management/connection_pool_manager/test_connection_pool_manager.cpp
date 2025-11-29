#include "connection_pool_manager.h"
#include <gtest/gtest.h>
#include <vector>
#include <string>
#include <thread>
#include <hiredis/hiredis.h>

// We will use a live Redis server for integration testing.
// Make sure Redis is running on localhost:6379.

TEST(ConnectionPoolManagerTest, BasicConnection) {
    std::vector<std::string> hosts = {"127.0.0.1"};
    ConnectionPoolManager pool(hosts, 1);

    redisContext* conn = pool.getConnection();
    ASSERT_NE(conn, nullptr);

    redisReply* reply = (redisReply*)redisCommand(conn, "PING");
    ASSERT_NE(reply, nullptr);
    ASSERT_STREQ(reply->str, "PONG");
    freeReplyObject(reply);

    pool.returnConnection(conn);
}

// Test that a thread can acquire a connection after it's returned by another thread.
TEST(ConnectionPoolManagerTest, PoolExhaustionAndReturn) {
    std::vector<std::string> hosts = {"127.0.0.1"};
    ConnectionPoolManager pool(hosts, 1);

    // Take the only available connection
    redisContext* conn1 = pool.getConnection();
    ASSERT_NE(conn1, nullptr);

    // This thread will block waiting for the connection to be returned
    std::thread t([&]() {
        redisContext* conn2 = pool.getConnection();
        // This should succeed now
        ASSERT_NE(conn2, nullptr);
        if(conn2) {
            pool.returnConnection(conn2);
        }
    });

    // Give the thread a moment to start and block
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Return the connection, which should unblock the thread
    pool.returnConnection(conn1);

    t.join();
}

TEST(ConnectionPoolManagerTest, InvalidInitialization) {
    std::vector<std::string> empty_hosts;
    ASSERT_THROW(ConnectionPoolManager(empty_hosts, 1), std::invalid_argument);

    std::vector<std::string> hosts = {"127.0.0.1"};
    ASSERT_THROW(ConnectionPoolManager(hosts, 0), std::invalid_argument);
}

// Test that a blocked getConnection call returns nullptr when the pool is destroyed.
TEST(ConnectionPoolManagerTest, Shutdown) {
    std::vector<std::string> hosts = {"127.0.0.1"};
    auto pool = std::make_unique<ConnectionPoolManager>(hosts, 1);

    // Take the only connection
    redisContext* conn1 = pool->getConnection();
    ASSERT_NE(conn1, nullptr);

    // This thread will block, waiting for a connection
    std::thread t([&]() {
        redisContext* conn2 = pool->getConnection();
        // When the pool is destroyed, getConnection should unblock and return nullptr
        ASSERT_EQ(conn2, nullptr);
    });

    // Give the thread a moment to block
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Destroy the pool. This should unblock the waiting thread.
    pool.reset();

    t.join();
}
