#ifndef COUNTER_SERVICE_H
#define COUNTER_SERVICE_H

#include <connection_pool_manager/connection_pool_manager.h>
#include <string>
#include <memory>

class CounterService {
public:
    CounterService(std::shared_ptr<ConnectionPoolManager> pool_manager);
    ~CounterService();

    // Deleted copy and move constructors/assignments
    CounterService(const CounterService&) = delete;
    CounterService& operator=(const CounterService&) = delete;
    CounterService(CounterService&&) = delete;
    CounterService& operator=(CounterService&&) = delete;

    long long increment(const std::string& counter_key, long long amount = 1);
    long long decrement(const std::string& counter_key, long long amount = 1);
    long long getValue(const std::string& counter_key);
    void deleteCounter(const std::string& counter_key);

private:
    std::shared_ptr<ConnectionPoolManager> pool_manager_;
};

#endif // COUNTER_SERVICE_H
