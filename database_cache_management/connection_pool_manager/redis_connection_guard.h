#ifndef REDIS_CONNECTION_GUARD_H
#define REDIS_CONNECTION_GUARD_H

#include <stdexcept>

// Forward declaration for hiredis context
struct redisContext;
class ConnectionPoolManager;

class RedisConnectionGuard {
public:
    RedisConnectionGuard(ConnectionPoolManager* pool_manager)
        : pool_manager_(pool_manager), context_(pool_manager->getConnection()) {
        if (!context_) {
            throw std::runtime_error("Failed to get Redis connection from pool");
        }
    }

    ~RedisConnectionGuard() {
        if (context_) {
            pool_manager_->returnConnection(context_);
        }
    }

    // Deleted copy and move constructors/assignments
    RedisConnectionGuard(const RedisConnectionGuard&) = delete;
    RedisConnectionGuard& operator=(const RedisConnectionGuard&) = delete;
    RedisConnectionGuard(RedisConnectionGuard&&) = delete;
    RedisConnectionGuard& operator=(RedisConnectionGuard&&) = delete;

    redisContext* getContext() const { return context_; }

private:
    ConnectionPoolManager* pool_manager_;
    redisContext* context_;
};

#endif // REDIS_CONNECTION_GUARD_H
