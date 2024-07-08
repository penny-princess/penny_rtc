cmake_minimum_required(VERSION 3.16)
project(core)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_BUILD_TYPE Release)

add_definitions("-g -pipe -W -Wall")

add_library(core
    event_loop/event_loop.cpp
    net/socket.cpp
    common/unlocked_queues.h
    common/buffer.cpp
    logger/logger.cpp
)

target_precompile_headers(core PUBLIC pch/pch.h)
set_target_properties(core PROPERTIES UNITY_BUILD ON)
target_include_directories(core PUBLIC event_loop net pch common logger include)
