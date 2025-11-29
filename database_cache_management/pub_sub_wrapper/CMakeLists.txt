cmake_minimum_required(VERSION 3.10)
project(PubSubWrapper)

find_package(PkgConfig REQUIRED)
pkg_check_modules(HIREDIS REQUIRED hiredis)

add_library(pub_sub_wrapper INTERFACE)

target_include_directories(pub_sub_wrapper INTERFACE .. ${HIREDIS_INCLUDE_DIRS})
target_link_libraries(pub_sub_wrapper INTERFACE hiredis connection_pool_manager nlohmann_json::nlohmann_json)

find_package(GTest REQUIRED)

add_executable(test_pub_sub_wrapper test_pub_sub_wrapper.cpp)
target_link_libraries(test_pub_sub_wrapper pub_sub_wrapper GTest::GTest GTest::Main)

add_executable(pub_sub_wrapper_example example.cpp)
target_link_libraries(pub_sub_wrapper_example pub_sub_wrapper)
