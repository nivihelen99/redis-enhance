cmake_minimum_required(VERSION 3.10)
project(CounterService)

find_package(PkgConfig REQUIRED)
pkg_check_modules(HIREDIS REQUIRED hiredis)

find_package(GTest REQUIRED)

# Add the library
add_library(counter_service
    counter_service.cpp
)
target_include_directories(counter_service PUBLIC ../)
target_link_libraries(counter_service
    connection_pool_manager
    ${HIREDIS_LIBRARIES}
)

# Add the test executable
add_executable(test_counter_service
    test_counter_service.cpp
)
target_link_libraries(test_counter_service
    counter_service
    connection_pool_manager
    GTest::GTest
    GTest::Main
)

# Add the example executable
add_executable(counter_service_example
    example.cpp
)
target_link_libraries(counter_service_example
    counter_service
    connection_pool_manager
)
