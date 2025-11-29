#ifndef PUB_SUB_WRAPPER_H
#define PUB_SUB_WRAPPER_H

#include <string>
#include <functional>
#include <thread>
#include <nlohmann/json.hpp>
#include <connection_pool_manager/connection_pool_manager.h>
#include <connection_pool_manager/redis_connection_guard.h>
#include <hiredis/hiredis.h>
#include <iostream>
#include <future>
#include <random>

using json = nlohmann::json;

template<typename T>
class PubSubWrapper {
public:
    PubSubWrapper(std::shared_ptr<ConnectionPoolManager> pool_manager);
    ~PubSubWrapper();

    std::future<void> subscribe(const std::string& channel, std::function<void(const T&)> callback);
    void publish(const std::string& channel, const T& message);
    void unsubscribe(const std::string& channel);

private:
    void listener_thread(const std::string& channel, ConnectionPoolManager* pool_manager, std::shared_ptr<std::promise<void>> promise);

    std::shared_ptr<ConnectionPoolManager> m_pool_manager;
    std::map<std::string, std::thread> m_listener_threads;
    std::map<std::string, bool> m_listening;
    std::map<std::string, std::function<void(const T&)>> m_callbacks;
    std::mutex m_mutex;
    std::string m_quit_message;
};

template<typename T>
PubSubWrapper<T>::PubSubWrapper(std::shared_ptr<ConnectionPoolManager> pool_manager)
    : m_pool_manager(pool_manager) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distrib(0, 255);
    for (int i = 0; i < 16; ++i) {
        m_quit_message += std::to_string(distrib(gen));
    }
}

template<typename T>
PubSubWrapper<T>::~PubSubWrapper() {
    std::vector<std::string> channels;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        for (auto const& [channel, thread] : m_listener_threads) {
            channels.push_back(channel);
        }
    }
    for (const auto& channel : channels) {
        unsubscribe(channel);
    }
}

#include <future>

template<typename T>
std::future<void> PubSubWrapper<T>::subscribe(const std::string& channel, std::function<void(const T&)> callback) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_listening.find(channel) != m_listening.end()) {
        auto promise = std::make_shared<std::promise<void>>();
        promise->set_value();
        return promise->get_future();
    }

    m_listening[channel] = true;
    m_callbacks[channel] = callback;

    auto promise = std::make_shared<std::promise<void>>();
    m_listener_threads[channel] = std::thread(&PubSubWrapper<T>::listener_thread, this, channel, m_pool_manager.get(), promise);
    return promise->get_future();
}

template<typename T>
void PubSubWrapper<T>::publish(const std::string& channel, const T& message) {
    RedisConnectionGuard guard(m_pool_manager.get());
    json j = message;
    std::string message_str = j.dump();
    redisReply* reply = (redisReply*)redisCommand(guard.getContext(), "PUBLISH %s %s", channel.c_str(), message_str.c_str());
    if (reply == nullptr) {
        throw std::runtime_error("Failed to publish message");
    }
    freeReplyObject(reply);
}

template<typename T>
void PubSubWrapper<T>::unsubscribe(const std::string& channel) {
    std::thread thread_to_join;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_listening.find(channel);
        if (it == m_listening.end() || !it->second) {
            return;
        }
        it->second = false;

        RedisConnectionGuard guard(m_pool_manager.get());
        redisReply* reply = (redisReply*)redisCommand(guard.getContext(), "PUBLISH %s %s", channel.c_str(), m_quit_message.c_str());
        freeReplyObject(reply);

        auto thread_it = m_listener_threads.find(channel);
        if (thread_it != m_listener_threads.end()) {
            thread_to_join = std::move(thread_it->second);
            m_listener_threads.erase(thread_it);
        }
        m_callbacks.erase(channel);
        m_listening.erase(channel);
    }

    if (thread_to_join.joinable()) {
        thread_to_join.join();
    }
}

template<typename T>
void PubSubWrapper<T>::listener_thread(const std::string& channel, ConnectionPoolManager* pool_manager, std::shared_ptr<std::promise<void>> promise) {
    RedisConnectionGuard guard(pool_manager);
    redisContext* context = guard.getContext();

    redisReply* reply = (redisReply*)redisCommand(context, "SUBSCRIBE %s", channel.c_str());
    freeReplyObject(reply);
    promise->set_value();

    while (true) {
        bool listening;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            auto it = m_listening.find(channel);
            if (it == m_listening.end()) {
                break;
            }
            listening = it->second;
        }
        if (!listening) break;

        if (redisGetReply(context, (void**)&reply) == REDIS_OK) {
            if (reply->type == REDIS_REPLY_ARRAY && reply->elements == 3) {
                if (strcmp(reply->element[0]->str, "message") == 0) {
                    if (strcmp(reply->element[2]->str, m_quit_message.c_str()) == 0) {
                        break;
                    }
                    try {
                        T message = json::parse(reply->element[2]->str);
                        std::function<void(const T&)> callback;
                        {
                            std::lock_guard<std::mutex> lock(m_mutex);
                            auto it = m_callbacks.find(channel);
                            if (it != m_callbacks.end()) {
                                callback = it->second;
                            }
                        }
                        if (callback) {
                            callback(message);
                        }
                    } catch (const std::exception& e) {
                        std::cerr << "Error parsing message: " << e.what() << std::endl;
                    }
                }
            }
            freeReplyObject(reply);
        } else {
            std::cerr << "Redis connection error in listener thread for channel " << channel << std::endl;
            break;
        }
    }

    reply = (redisReply*)redisCommand(context, "UNSUBSCRIBE %s", channel.c_str());
    freeReplyObject(reply);
}

#endif // PUB_SUB_WRAPPER_H
