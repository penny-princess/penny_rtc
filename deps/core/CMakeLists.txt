cmake_minimum_required(VERSION 3.16)
project(core)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_BUILD_TYPE Release)

add_definitions("-g -pipe -W -Wall")

add_library(core
        common/event_loop.cpp
        net/socket.cpp
        common/unlocked_queues.h
        logger/logger.cpp
)

target_precompile_headers(core PUBLIC pch/pch.h)
target_include_directories(core PUBLIC event_loop net pch common logger include)
