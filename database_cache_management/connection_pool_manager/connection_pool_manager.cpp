#include "connection_pool_manager.h"
#include "redis_connection_guard.h"
#include <hiredis/hiredis.h>
#include <iostream>
#include <stdexcept>
#include <chrono>

ConnectionPoolManager::ConnectionPoolManager(const std::vector<std::string>& hosts, int pool_size)
    : redis_hosts_(hosts), pool_size_(pool_size), in_use_(pool_size, false) {
    if (redis_hosts_.empty() || pool_size_ <= 0) {
        throw std::invalid_argument("Invalid hosts or pool size");
    }

    pool_.resize(pool_size, nullptr);
    for (int i = 0; i < pool_size_; ++i) {
        pool_[i] = connectToRedis(redis_hosts_[i % redis_hosts_.size()]);
    }

    health_check_thread_ = std::thread(&ConnectionPoolManager::healthCheck, this);
}

ConnectionPoolManager::~ConnectionPoolManager() {
    {
        std::unique_lock<std::mutex> lock(mutex_);
        shutting_down_ = true;
    }
    condition_.notify_all();
    health_check_thread_.join();

    for (auto conn : pool_) {
        if (conn) {
            redisFree(conn);
        }
    }
}

redisContext* ConnectionPoolManager::connectToRedis(const std::string& host) {
    redisContext* context = redisConnect(host.c_str(), 6379);
    if (context == nullptr || context->err) {
        if (context) {
            std::cerr << "Redis connection error: " << context->errstr << std::endl;
            redisFree(context);
        } else {
            std::cerr << "Can't allocate redis context" << std::endl;
        }
        return nullptr;
    }
    return context;
}

redisContext* ConnectionPoolManager::getConnection() {
    std::unique_lock<std::mutex> lock(mutex_);
    condition_.wait(lock, [this] {
        if (shutting_down_) return true;
        for (int i = 0; i < pool_size_; ++i) {
            if (!in_use_[i] && pool_[i] != nullptr) {
                return true;
            }
        }
        return false;
    });

    if (shutting_down_) {
        return nullptr;
    }

    for (int i = 0; i < pool_size_; ++i) {
        if (!in_use_[i] && pool_[i] != nullptr) {
            in_use_[i] = true;
            return pool_[i];
        }
    }

    return nullptr; // Should not be reached if wait condition is correct
}

void ConnectionPoolManager::returnConnection(redisContext* context) {
    if (context == nullptr) return;

    std::unique_lock<std::mutex> lock(mutex_);
    for (int i = 0; i < pool_size_; ++i) {
        if (pool_[i] == context) {
            in_use_[i] = false;
            break;
        }
    }
    condition_.notify_one();
}

void ConnectionPoolManager::healthCheck() {
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(5));
        {
            std::unique_lock<std::mutex> lock(mutex_);
            if (shutting_down_) {
                return;
            }

            for (int i = 0; i < pool_size_; ++i) {
                if (in_use_[i]) continue;

                if (pool_[i] == nullptr) {
                    pool_[i] = connectToRedis(redis_hosts_[i % redis_hosts_.size()]);
                    if(pool_[i] != nullptr) condition_.notify_one();
                } else {
                    redisReply* reply = (redisReply*)redisCommand(pool_[i], "PING");
                    if (reply == nullptr || pool_[i]->err) {
                        std::cerr << "Health check failed for connection " << i << ". Reconnecting." << std::endl;
                        redisFree(pool_[i]);
                        pool_[i] = connectToRedis(redis_hosts_[i % redis_hosts_.size()]);
                        if(pool_[i] != nullptr) condition_.notify_one();
                    }
                    if (reply) {
                        freeReplyObject(reply);
                    }
                }
            }
        }
    }
}
