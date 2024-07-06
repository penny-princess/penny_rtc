cmake_minimum_required(VERSION 3.16)
project(penny_rtc)

set(CMAKE_CXX_STANDARD 17)


add_definitions("-g -pipe -W -Wall")
add_definitions("-std=gnu++14")

add_subdirectory(deps/core)

add_executable(penny_rtc src/main.cpp)

target_link_libraries(penny_rtc core)
