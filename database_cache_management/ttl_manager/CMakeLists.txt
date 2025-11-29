cmake_minimum_required(VERSION 3.10)
project(TtlManager)

find_package(PkgConfig REQUIRED)
pkg_check_modules(HIREDIS REQUIRED hiredis)

find_package(GTest REQUIRED)

# Add the library
add_library(ttl_manager
    ttl_manager.cpp
)
target_include_directories(ttl_manager PUBLIC ../)
target_link_libraries(ttl_manager
    connection_pool_manager
    ${HIREDIS_LIBRARIES}
)

# Add the test executable
add_executable(test_ttl_manager
    test_ttl_manager.cpp
)
target_link_libraries(test_ttl_manager
    ttl_manager
    connection_pool_manager
    GTest::GTest
    GTest::Main
)

# Add the example executable
add_executable(ttl_manager_example
    example.cpp
)
target_link_libraries(ttl_manager_example
    ttl_manager
    connection_pool_manager
)
