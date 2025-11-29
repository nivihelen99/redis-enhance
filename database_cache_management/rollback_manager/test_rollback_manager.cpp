#include "gtest/gtest.h"
#include "rollback_manager.h"
#include "connection_pool_manager/redis_connection_guard.h"
#include <hiredis/hiredis.h>
#include <vector>
#include <thread>
#include <chrono>

class RollbackManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        std::vector<std::string> hosts = {"127.0.0.1"};
        pool_manager = std::make_shared<ConnectionPoolManager>(hosts, 5);
        rollback_manager = std::make_unique<RollbackManager>(pool_manager);

        RedisConnectionGuard guard(pool_manager.get());
        redisContext* context = guard.getContext();
        redisReply* reply = (redisReply*)redisCommand(context, "FLUSHALL");
        freeReplyObject(reply);
    }

    std::shared_ptr<ConnectionPoolManager> pool_manager;
    std::unique_ptr<RollbackManager> rollback_manager;
};

TEST_F(RollbackManagerTest, SaveAndGetSnapshot) {
    std::string config_name = "test_config";
    json config_data = {{"key", "value"}};
    std::string timestamp = rollback_manager->saveSnapshot(config_name, config_data);

    json snapshot = rollback_manager->getSnapshot(config_name, timestamp);
    ASSERT_EQ(snapshot, config_data);
}

TEST_F(RollbackManagerTest, ListSnapshots) {
    std::string config_name = "list_config";
    json config_data = {{"key", "value"}};
    std::string timestamp1 = rollback_manager->saveSnapshot(config_name, config_data);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    std::string timestamp2 = rollback_manager->saveSnapshot(config_name, config_data);

    std::vector<std::string> snapshots = rollback_manager->listSnapshots(config_name);
    ASSERT_EQ(snapshots.size(), 2);
    ASSERT_NE(std::find(snapshots.begin(), snapshots.end(), timestamp1), snapshots.end());
    ASSERT_NE(std::find(snapshots.begin(), snapshots.end(), timestamp2), snapshots.end());
}

TEST_F(RollbackManagerTest, DeleteSnapshot) {
    std::string config_name = "delete_config";
    json config_data = {{"key", "value"}};
    std::string timestamp = rollback_manager->saveSnapshot(config_name, config_data);

    rollback_manager->deleteSnapshot(config_name, timestamp);
    json snapshot = rollback_manager->getSnapshot(config_name, timestamp);
    ASSERT_TRUE(snapshot.is_null());
}

TEST_F(RollbackManagerTest, GetNonExistentSnapshot) {
    std::string config_name = "non_existent_config";
    json snapshot = rollback_manager->getSnapshot(config_name, "12345");
    ASSERT_TRUE(snapshot.is_null());
}
