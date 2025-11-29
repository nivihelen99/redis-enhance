#include "rollback_manager.h"
#include <iostream>
#include <vector>

int main() {
    std::vector<std::string> hosts = {"127.0.0.1"};
    auto pool_manager = std::make_shared<ConnectionPoolManager>(hosts, 5);
    RollbackManager rollback_manager(pool_manager);

    std::string config_name = "vlan_config";
    json config_data = {
        {"vlan_id", 100},
        {"ports", {"eth0", "eth1"}}
    };

    // Save a snapshot
    std::string timestamp = rollback_manager.saveSnapshot(config_name, config_data);
    std::cout << "Saved snapshot with timestamp: " << timestamp << std::endl;

    // List snapshots
    std::vector<std::string> snapshots = rollback_manager.listSnapshots(config_name);
    std::cout << "Available snapshots:" << std::endl;
    for (const auto& ts : snapshots) {
        std::cout << " - " << ts << std::endl;
    }

    // Get a snapshot
    json snapshot_data = rollback_manager.getSnapshot(config_name, timestamp);
    std::cout << "Retrieved snapshot data: " << snapshot_data.dump(4) << std::endl;

    // Delete a snapshot
    rollback_manager.deleteSnapshot(config_name, timestamp);
    std::cout << "Deleted snapshot with timestamp: " << timestamp << std::endl;

    return 0;
}
