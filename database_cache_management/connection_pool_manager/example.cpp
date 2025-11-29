#include "connection_pool_manager.h"
#include <iostream>
#include <vector>
#include <string>
#include <hiredis/hiredis.h>

int main() {
    try {
        std::vector<std::string> hosts = {"127.0.0.1"};
        ConnectionPoolManager pool(hosts, 5);

        std::cout << "Getting a connection from the pool..." << std::endl;
        redisContext* conn = pool.getConnection();

        if (conn) {
            std::cout << "Successfully got a connection. Sending PING." << std::endl;
            redisReply* reply = (redisReply*)redisCommand(conn, "PING");

            if (reply) {
                std::cout << "Received reply: " << reply->str << std::endl;
                freeReplyObject(reply);
            } else {
                std::cerr << "Command failed: " << conn->errstr << std::endl;
            }

            pool.returnConnection(conn);
            std::cout << "Returned connection to the pool." << std::endl;
        } else {
            std::cerr << "Failed to get a connection from the pool." << std::endl;
        }

    } catch (const std::exception& e) {
        std::cerr << "An exception occurred: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
