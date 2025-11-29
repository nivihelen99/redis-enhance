#ifndef REDIS_CONNECTION_GUARD_H
#define REDIS_CONNECTION_GUARD_H

#include "connection_pool_manager.h"
#include <stdexcept>
#include <memory>

class RedisConnectionGuard {
public:
    RedisConnectionGuard(std::shared_ptr<ConnectionPoolManager> pool_manager)
        : pool_manager_(pool_manager), connection_(nullptr) {
        connection_ = pool_manager_->getConnection();
        if (!connection_) {
            throw std::runtime_error("Failed to get Redis connection");
        }
    }

    ~RedisConnectionGuard() {
        if (connection_) {
            pool_manager_->returnConnection(connection_);
        }
    }

    // Deleted copy and move constructors/assignments
    RedisConnectionGuard(const RedisConnectionGuard&) = delete;
    RedisConnectionGuard& operator=(const RedisConnectionGuard&) = delete;
    RedisConnectionGuard(RedisConnectionGuard&&) = delete;
    RedisConnectionGuard& operator=(RedisConnectionGuard&&) = delete;

    redisContext* operator->() const {
        return connection_;
    }

    redisContext* get() const {
        return connection_;
    }

private:
    std::shared_ptr<ConnectionPoolManager> pool_manager_;
    redisContext* connection_;
};

#endif // REDIS_CONNECTION_GUARD_H
