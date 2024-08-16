cmake_minimum_required(VERSION 3.16)
project(penny_rtc)

set(CMAKE_CXX_STANDARD 20)

#add_definitions("-g -pipe -W -Wall")

#set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=address")
#set(CMAKE_LINKER_FLAGS_DEBUG "${CMAKE_LINKER_FLAGS_DEBUG} -fsanitize=address")


#set(ENV{ASAN_OPTIONS} "log_path=/tmp/asan.log")


add_subdirectory(deps/core)
include_directories(src)

add_executable(penny_rtc
        src/main.cpp
        src/server/server.cpp
        src/server/protocol.h
        src/server/handle.cpp
        src/server/worker.cpp
        src/server/tcp_connection.cpp
)

target_link_libraries(penny_rtc PRIVATE -lpthread core)
set_target_properties(penny_rtc PROPERTIES UNITY_BUILD ON)