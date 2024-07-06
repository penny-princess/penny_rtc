cmake_minimum_required(VERSION 3.16)
project(core)

set(CMAKE_CXX_STANDARD 17)

add_definitions("-g -pipe -W -Wall")

add_subdirectory(deps/libev)

add_library(core
        event_loop/event_loop.cpp
)

target_include_directories(core PUBLIC .)
target_link_libraries(core PRIVATE ev)


