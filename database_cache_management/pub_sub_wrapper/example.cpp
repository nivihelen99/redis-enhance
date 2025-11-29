#include <pub_sub_wrapper/pub_sub_wrapper.h>
#include <iostream>
#include <chrono>
#include <thread>

int main() {
    auto pool_manager = std::make_shared<ConnectionPoolManager>(std::vector<std::string>{"127.0.0.1"}, 5);
    PubSubWrapper<std::string> pub_sub(pool_manager);

    pub_sub.subscribe("test_channel", [](const std::string& msg) {
        std::cout << "Received message: " << msg << std::endl;
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    pub_sub.publish("test_channel", "hello world");

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    pub_sub.unsubscribe("test_channel");

    return 0;
}
