#ifndef ROLLBACK_MANAGER_H
#define ROLLBACK_MANAGER_H

#include <string>
#include <vector>
#include <memory>
#include <connection_pool_manager/connection_pool_manager.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

class RollbackManager {
public:
    explicit RollbackManager(std::shared_ptr<ConnectionPoolManager> pool_manager);

    std::string saveSnapshot(const std::string& config_name, const json& config_data);
    json getSnapshot(const std::string& config_name, const std::string& timestamp);
    std::vector<std::string> listSnapshots(const std::string& config_name);
    void deleteSnapshot(const std::string& config_name, const std::string& timestamp);

private:
    std::shared_ptr<ConnectionPoolManager> pool_manager_;
};

#endif // ROLLBACK_MANAGER_H
