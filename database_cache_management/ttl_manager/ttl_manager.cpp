#include "ttl_manager.h"
#include <hiredis/hiredis.h>
#include <stdexcept>

TtlManager::TtlManager(std::shared_ptr<ConnectionPoolManager> pool_manager)
    : pool_manager_(pool_manager) {
}

TtlManager::~TtlManager() {
}

void TtlManager::addKey(const std::string& key, int ttl_seconds) {
    auto conn = pool_manager_->getConnection();
    if (!conn) {
        throw std::runtime_error("Failed to get Redis connection");
    }

    redisReply* reply = (redisReply*)redisCommand(conn, "EXPIRE %s %d", key.c_str(), ttl_seconds);
    if (reply) {
        freeReplyObject(reply);
    }

    pool_manager_->returnConnection(conn);
}
