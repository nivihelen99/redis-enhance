#include "counter_service.h"
#include <connection_pool_manager/connection_pool_manager.h>
#include <iostream>
#include <vector>
#include <string>
#include <thread>

int main() {
    try {
        std::vector<std::string> hosts = {"127.0.0.1"};
        auto pool_manager = std::make_shared<ConnectionPoolManager>(hosts, 10);
        CounterService counter_service(pool_manager);

        const std::string page_views_key = "page_views:homepage";

        // Reset the counter for a clean run
        counter_service.deleteCounter(page_views_key);

        std::cout << "Initial page views: " << counter_service.getValue(page_views_key) << std::endl;

        // Simulate 10 concurrent users viewing the page
        std::vector<std::thread> threads;
        for (int i = 0; i < 10; ++i) {
            threads.emplace_back([&]() {
                long long current_views = counter_service.increment(page_views_key);
                // The returned value may not be the final one, as other threads are running.
            });
        }

        for (auto& t : threads) {
            t.join();
        }

        std::cout << "Final page views after 10 concurrent increments: " << counter_service.getValue(page_views_key) << std::endl;

        counter_service.decrement(page_views_key, 2);
        std::cout << "Page views after 2 decrements: " << counter_service.getValue(page_views_key) << std::endl;


    } catch (const std::exception& e) {
        std::cerr << "An exception occurred: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
