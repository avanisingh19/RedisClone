cmake_minimum_required(VERSION 3.16)
project(RedisClone LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(redisclone_core
    parser/command_parser.cpp
    storage/key_value_store.cpp
    persistence/persistence_manager.cpp
    networking/socket_utils.cpp
    server/redis_server.cpp
)

target_include_directories(redisclone_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_compile_options(redisclone_core PRIVATE -Wall -Wextra -Wpedantic)

add_executable(redis-server server/main.cpp)
target_link_libraries(redis-server PRIVATE redisclone_core pthread)

add_executable(redis-cli client/main.cpp)
target_link_libraries(redis-cli PRIVATE redisclone_core pthread)

add_executable(redisclone_tests tests/test_main.cpp)
target_link_libraries(redisclone_tests PRIVATE redisclone_core pthread)
