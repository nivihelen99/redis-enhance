cmake_minimum_required(VERSION 3.10)
project(ConnectionPoolManager)

find_package(PkgConfig REQUIRED)
pkg_check_modules(HIREDIS REQUIRED hiredis)

find_package(GTest REQUIRED)

# Add the library
add_library(connection_pool_manager
    connection_pool_manager.cpp
)
target_include_directories(connection_pool_manager PUBLIC ${HIREDIS_INCLUDE_DIRS})
target_link_libraries(connection_pool_manager PRIVATE ${HIREDIS_LIBRARIES})

# Add the test executable
add_executable(test_connection_pool_manager
    test_connection_pool_manager.cpp
)
target_link_libraries(test_connection_pool_manager
    connection_pool_manager
    ${HIREDIS_LIBRARIES}
    GTest::GTest
    GTest::Main
)

# Add the example executable
add_executable(connection_pool_manager_example
    example.cpp
)
target_link_libraries(connection_pool_manager_example
    connection_pool_manager
    ${HIREDIS_LIBRARIES}
)
