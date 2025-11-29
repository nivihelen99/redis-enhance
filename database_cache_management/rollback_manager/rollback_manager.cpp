#include "rollback_manager.h"
#include "connection_pool_manager/redis_connection_guard.h"
#include <hiredis/hiredis.h>
#include <chrono>
#include <stdexcept>

RollbackManager::RollbackManager(std::shared_ptr<ConnectionPoolManager> pool_manager)
    : pool_manager_(std::move(pool_manager)) {}

std::string RollbackManager::saveSnapshot(const std::string& config_name, const json& config_data) {
    RedisConnectionGuard guard(pool_manager_.get());
    redisContext* context = guard.getContext();

    long long timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    std::string timestamp_str = std::to_string(timestamp);

    redisReply* reply = (redisReply*)redisCommand(context, "HSET %s %s %s",
        config_name.c_str(), timestamp_str.c_str(), config_data.dump().c_str());

    if (!reply) {
        throw std::runtime_error("Failed to save snapshot: " + std::string(context->errstr));
    }

    freeReplyObject(reply);
    return timestamp_str;
}

json RollbackManager::getSnapshot(const std::string& config_name, const std::string& timestamp) {
    RedisConnectionGuard guard(pool_manager_.get());
    redisContext* context = guard.getContext();

    redisReply* reply = (redisReply*)redisCommand(context, "HGET %s %s",
        config_name.c_str(), timestamp.c_str());

    if (!reply) {
        throw std::runtime_error("Failed to get snapshot: " + std::string(context->errstr));
    }

    if (reply->type == REDIS_REPLY_NIL) {
        freeReplyObject(reply);
        return json{};
    }

    json snapshot = json::parse(reply->str);
    freeReplyObject(reply);
    return snapshot;
}

std::vector<std::string> RollbackManager::listSnapshots(const std::string& config_name) {
    RedisConnectionGuard guard(pool_manager_.get());
    redisContext* context = guard.getContext();

    redisReply* reply = (redisReply*)redisCommand(context, "HKEYS %s", config_name.c_str());

    if (!reply) {
        throw std::runtime_error("Failed to list snapshots: " + std::string(context->errstr));
    }

    std::vector<std::string> snapshots;
    if (reply->type == REDIS_REPLY_ARRAY) {
        for (size_t i = 0; i < reply->elements; ++i) {
            snapshots.push_back(reply->element[i]->str);
        }
    }

    freeReplyObject(reply);
    return snapshots;
}

void RollbackManager::deleteSnapshot(const std::string& config_name, const std::string& timestamp) {
    RedisConnectionGuard guard(pool_manager_.get());
    redisContext* context = guard.getContext();

    redisReply* reply = (redisReply*)redisCommand(context, "HDEL %s %s",
        config_name.c_str(), timestamp.c_str());

    if (!reply) {
        throw std::runtime_error("Failed to delete snapshot: " + std::string(context->errstr));
    }

    freeReplyObject(reply);
}
