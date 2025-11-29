#include "ttl_manager.h"
#include <connection_pool_manager/redis_connection_guard.h>
#include <hiredis/hiredis.h>
#include <stdexcept>

TtlManager::TtlManager(std::shared_ptr<ConnectionPoolManager> pool_manager)
    : pool_manager_(pool_manager) {
}

TtlManager::~TtlManager() {
}

void TtlManager::addKey(const std::string& key, int ttl_seconds) {
    RedisConnectionGuard conn(pool_manager_);

    redisReply* reply = (redisReply*)redisCommand(conn.get(), "EXPIRE %s %d", key.c_str(), ttl_seconds);
    if (reply) {
        freeReplyObject(reply);
    }
}
