#ifndef CONNECTION_POOL_MANAGER_H
#define CONNECTION_POOL_MANAGER_H

#include <vector>
#include <string>
#include <memory>
#include <mutex>
#include <condition_variable>
#include <thread>

// Forward declaration for hiredis context
struct redisContext;

class ConnectionPoolManager {
public:
    ConnectionPoolManager(const std::vector<std::string>& hosts, int pool_size);
    ~ConnectionPoolManager();

    // Deleted copy and move constructors/assignments
    ConnectionPoolManager(const ConnectionPoolManager&) = delete;
    ConnectionPoolManager& operator=(const ConnectionPoolManager&) = delete;
    ConnectionPoolManager(ConnectionPoolManager&&) = delete;
    ConnectionPoolManager& operator=(ConnectionPoolManager&&) = delete;

    redisContext* getConnection();
    void returnConnection(redisContext* context);

private:
    redisContext* connectToRedis(const std::string& host);
    void healthCheck();

    const std::vector<std::string> redis_hosts_;
    const int pool_size_;

    std::vector<redisContext*> pool_; // All connections, some can be nullptr
    std::vector<bool> in_use_;      // Flag for each connection

    std::mutex mutex_;
    std::condition_variable condition_;
    bool shutting_down_ = false;
    std::thread health_check_thread_;
};

#endif // CONNECTION_POOL_MANAGER_H
