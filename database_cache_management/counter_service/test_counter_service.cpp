#include "counter_service.h"
#include <connection_pool_manager/connection_pool_manager.h>
#include <gtest/gtest.h>
#include <vector>
#include <string>
#include <thread>
#include <hiredis/hiredis.h>
#include <numeric>

class CounterServiceTest : public ::testing::Test {
protected:
    void SetUp() override {
        CounterService counter(pool_manager);
        counter.deleteCounter(counter_key);
    }

    std::shared_ptr<ConnectionPoolManager> pool_manager = std::make_shared<ConnectionPoolManager>(std::vector<std::string>{"127.0.0.1"}, 10);
    const std::string counter_key = "test_counter";
};


TEST_F(CounterServiceTest, IncrementAndGetValue) {
    CounterService counter(pool_manager);

    EXPECT_EQ(counter.getValue(counter_key), 0);

    EXPECT_EQ(counter.increment(counter_key), 1);
    EXPECT_EQ(counter.getValue(counter_key), 1);

    EXPECT_EQ(counter.increment(counter_key, 5), 6);
    EXPECT_EQ(counter.getValue(counter_key), 6);
}

TEST_F(CounterServiceTest, DecrementAndGetValue) {
    CounterService counter(pool_manager);

    EXPECT_EQ(counter.increment(counter_key, 10), 10);

    EXPECT_EQ(counter.decrement(counter_key), 9);
    EXPECT_EQ(counter.getValue(counter_key), 9);

    EXPECT_EQ(counter.decrement(counter_key, 4), 5);
    EXPECT_EQ(counter.getValue(counter_key), 5);
}

TEST_F(CounterServiceTest, ThreadSafety) {
    CounterService counter(pool_manager);
    const int num_threads = 10;
    const int increments_per_thread = 100;

    std::vector<std::thread> threads;
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&]() {
            for (int j = 0; j < increments_per_thread; ++j) {
                counter.increment(counter_key);
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    EXPECT_EQ(counter.getValue(counter_key), num_threads * increments_per_thread);
}
