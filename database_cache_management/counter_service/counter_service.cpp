#include "counter_service.h"
#include <connection_pool_manager/redis_connection_guard.h>
#include <hiredis/hiredis.h>
#include <stdexcept>
#include <string>

CounterService::CounterService(std::shared_ptr<ConnectionPoolManager> pool_manager)
    : pool_manager_(pool_manager) {
}

CounterService::~CounterService() {
}

long long CounterService::increment(const std::string& counter_key, long long amount) {
    RedisConnectionGuard conn(pool_manager_);

    redisReply* reply = (redisReply*)redisCommand(conn.get(), "INCRBY %s %lld", counter_key.c_str(), amount);
    if (!reply || reply->type != REDIS_REPLY_INTEGER) {
        if (reply) freeReplyObject(reply);
        throw std::runtime_error("Failed to increment counter in Redis");
    }

    long long result = reply->integer;
    freeReplyObject(reply);
    return result;
}

long long CounterService::decrement(const std::string& counter_key, long long amount) {
    RedisConnectionGuard conn(pool_manager_);

    redisReply* reply = (redisReply*)redisCommand(conn.get(), "DECRBY %s %lld", counter_key.c_str(), amount);
    if (!reply || reply->type != REDIS_REPLY_INTEGER) {
        if (reply) freeReplyObject(reply);
        throw std::runtime_error("Failed to decrement counter in Redis");
    }

    long long result = reply->integer;
    freeReplyObject(reply);
    return result;
}

long long CounterService::getValue(const std::string& counter_key) {
    RedisConnectionGuard conn(pool_manager_);

    redisReply* reply = (redisReply*)redisCommand(conn.get(), "GET %s", counter_key.c_str());
    if (!reply) {
        throw std::runtime_error("Failed to get counter value from Redis");
    }

    long long result = 0; // Default to 0 if key doesn't exist
    if (reply->type == REDIS_REPLY_STRING) {
        result = std::stoll(reply->str);
    } else if (reply->type != REDIS_REPLY_NIL) {
        freeReplyObject(reply);
        throw std::runtime_error("Unexpected reply type when getting counter value");
    }

    freeReplyObject(reply);
    return result;
}

void CounterService::deleteCounter(const std::string& counter_key) {
    RedisConnectionGuard conn(pool_manager_);

    redisReply* reply = (redisReply*)redisCommand(conn.get(), "DEL %s", counter_key.c_str());
    if (reply) {
        freeReplyObject(reply);
    }
}
