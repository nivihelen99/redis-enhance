#include "ttl_manager.h"
#include <connection_pool_manager/connection_pool_manager.h>
#include <iostream>
#include <vector>
#include <string>
#include <hiredis/hiredis.h>
#include <thread>
#include <chrono>

int main() {
    try {
        std::vector<std::string> hosts = {"127.0.0.1"};
        auto pool_manager = std::make_shared<ConnectionPoolManager>(hosts, 5);
        TtlManager ttl_manager(pool_manager);

        auto conn = pool_manager->getConnection();
        if (!conn) {
            std::cerr << "Failed to get a connection." << std::endl;
            return 1;
        }

        // Set a key
        redisReply* reply = (redisReply*)redisCommand(conn, "SET %s %s", "tempkey", "some temporary data");
        if (reply) freeReplyObject(reply);

        std::cout << "Setting a 5-second TTL on 'tempkey'." << std::endl;
        ttl_manager.addKey("tempkey", 5);

        for (int i = 0; i < 7; ++i) {
            reply = (redisReply*)redisCommand(conn, "GET %s", "tempkey");
            if (reply && reply->type != REDIS_REPLY_NIL) {
                std::cout << "Waited " << i << " seconds. Key 'tempkey' still exists." << std::endl;
                freeReplyObject(reply);
            } else {
                std::cout << "Waited " << i << " seconds. Key 'tempkey' has expired." << std::endl;
                if(reply) freeReplyObject(reply);
                break;
            }
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

        pool_manager->returnConnection(conn);

    } catch (const std::exception& e) {
        std::cerr << "An exception occurred: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
