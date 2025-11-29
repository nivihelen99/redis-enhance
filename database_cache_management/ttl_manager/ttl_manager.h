#ifndef TTL_MANAGER_H
#define TTL_MANAGER_H

#include <connection_pool_manager/connection_pool_manager.h>
#include <string>
#include <memory>

class TtlManager {
public:
    TtlManager(std::shared_ptr<ConnectionPoolManager> pool_manager);
    ~TtlManager();

    // Deleted copy and move constructors/assignments
    TtlManager(const TtlManager&) = delete;
    TtlManager& operator=(const TtlManager&) = delete;
    TtlManager(TtlManager&&) = delete;
    TtlManager& operator=(TtlManager&&) = delete;

    void addKey(const std::string& key, int ttl_seconds);

private:
    std::shared_ptr<ConnectionPoolManager> pool_manager_;
};

#endif // TTL_MANAGER_H
