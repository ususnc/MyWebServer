cmake_minimum_required(VERSION 3.0.0)
project(webserver VERSION 0.1.0)

include(CTest)
enable_testing()

set(CMAKE_CXX_STANDARD 20)

include_directories(include)

add_executable(webserver main.cpp src/config.cpp src/heap_timer.cpp src/http_conn.cpp
src/log.cpp src/sql_connection_pool.cpp src/utils.cpp src/webserver.cpp)

target_link_libraries(webserver mysqlclient)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
