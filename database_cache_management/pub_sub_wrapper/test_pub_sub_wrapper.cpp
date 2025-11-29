#include <gtest/gtest.h>
#include <pub_sub_wrapper/pub_sub_wrapper.h>
#include <connection_pool_manager/connection_pool_manager.h>
#include <memory>
#include <string>
#include <chrono>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>

class PubSubWrapperTest : public ::testing::Test {
protected:
    void SetUp() override {
        pool_manager = std::make_shared<ConnectionPoolManager>(std::vector<std::string>{"127.0.0.1"}, 5);
    }

    std::shared_ptr<ConnectionPoolManager> pool_manager;
};

TEST_F(PubSubWrapperTest, PublishAndSubscribe) {
    PubSubWrapper<std::string> pub_sub(pool_manager);
    std::string received_message;
    bool message_received = false;
    std::mutex m;
    std::condition_variable cv;

    auto future = pub_sub.subscribe("test_channel", [&](const std::string& msg) {
        std::lock_guard<std::mutex> lock(m);
        received_message = msg;
        message_received = true;
        cv.notify_one();
    });

    future.get();

    pub_sub.publish("test_channel", "hello world");

    std::unique_lock<std::mutex> lock(m);
    cv.wait(lock, [&] { return message_received; });

    ASSERT_TRUE(message_received);
    ASSERT_EQ(received_message, "hello world");

    pub_sub.unsubscribe("test_channel");
}

TEST_F(PubSubWrapperTest, Unsubscribe) {
    PubSubWrapper<std::string> pub_sub(pool_manager);
    bool message_received = false;
    std::mutex m;
    std::condition_variable cv;

    auto future = pub_sub.subscribe("test_channel", [&](const std::string& msg) {
        std::lock_guard<std::mutex> lock(m);
        message_received = true;
        cv.notify_one();
    });

    future.get();

    pub_sub.unsubscribe("test_channel");

    pub_sub.publish("test_channel", "hello world");

    std::unique_lock<std::mutex> lock(m);
    ASSERT_FALSE(cv.wait_for(lock, std::chrono::milliseconds(100), [&] { return message_received; }));
}

TEST_F(PubSubWrapperTest, MalformedJSON) {
    PubSubWrapper<int> pub_sub(pool_manager);
    bool message_received = false;
    std::mutex m;
    std::condition_variable cv;

    auto future = pub_sub.subscribe("test_channel", [&](const int& msg) {
        std::lock_guard<std::mutex> lock(m);
        message_received = true;
        cv.notify_one();
    });

    future.get();

    // Publish a string to a channel that expects an int
    RedisConnectionGuard guard(pool_manager.get());
    redisReply* reply = (redisReply*)redisCommand(guard.getContext(), "PUBLISH test_channel \"hello world\"");
    freeReplyObject(reply);

    std::unique_lock<std::mutex> lock(m);
    ASSERT_FALSE(cv.wait_for(lock, std::chrono::milliseconds(100), [&] { return message_received; }));
}
