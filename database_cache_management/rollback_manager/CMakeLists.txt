cmake_minimum_required(VERSION 3.10)
project(RollbackManager)

add_library(rollback_manager rollback_manager.cpp)

target_include_directories(rollback_manager PUBLIC .
    PUBLIC ${CMAKE_SOURCE_DIR}/database_cache_management
    PUBLIC ${CMAKE_SOURCE_DIR}/third_party/hiredis
    PUBLIC ${CMAKE_SOURCE_DIR}/third_party/json/single_include)

target_link_libraries(rollback_manager connection_pool_manager hiredis)

# Example
add_executable(rollback_manager_example example.cpp)
target_link_libraries(rollback_manager_example rollback_manager)

# Tests
find_package(GTest REQUIRED)
add_executable(test_rollback_manager test_rollback_manager.cpp)
target_link_libraries(test_rollback_manager rollback_manager GTest::GTest GTest::Main)
