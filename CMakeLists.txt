cmake_minimum_required(VERSION 3.16)
project(penny_rtc)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_BUILD_TYPE Debug)

# cache compiler
#set(CMAKE_C_COMPILER_LAUNCHER ccache)
#set(CMAKE_CXX_COMPILER_LAUNCHER ccache)

add_definitions("-g -pipe -W -Wall")

add_subdirectory(deps/core)

add_executable(penny_rtc
        src/main.cpp
        src/server/server.cpp
        src/server/protocol.h
        src/server/handle.cpp
        src/server/worker.cpp
        src/server/tcp_connection.cpp
)

target_link_libraries(penny_rtc -lpthread core)
set_target_properties(penny_rtc PROPERTIES UNITY_BUILD ON)