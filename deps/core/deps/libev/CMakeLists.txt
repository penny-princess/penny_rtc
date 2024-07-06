cmake_minimum_required(VERSION 3.16)
project(libev C)

if(APPLE)
    set(CMAKE_MACOSX_RPATH 1)
endif()

if(WIN32)
    list(APPEND EV_LIBS Ws2_32)
endif()

set (EV_SOURCE
        ${CMAKE_CURRENT_SOURCE_DIR}/ev.c
        ${CMAKE_CURRENT_SOURCE_DIR}/event.c
)

add_library(ev ${EV_SOURCE})
target_link_libraries(ev ${EV_LIBS})
target_include_directories(ev PUBLIC .)
add_library(ev::ev ALIAS ev)

add_library(ev_static STATIC ${EV_SOURCE})
target_link_libraries(ev_static ${EV_LIBS})
target_include_directories(ev_static PUBLIC .)
add_library(ev::ev_static ALIAS ev_static)