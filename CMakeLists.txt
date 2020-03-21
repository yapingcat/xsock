cmake_minimum_required(VERSION 2.8)
project(xsock)

add_compile_options(-std=c++11)

message(${POLLER})
if(${POLLER} STREQUAL "POLL")
    add_definitions(-DHAVE_POLL)
endif()

set(HEADER_FILE xsock.h)
set(SOURCE_FILE xsock.cpp)

add_library(xsock_static STATIC ${SOURCE_FILE})
add_library(xsock_shared SHARED ${SOURCE_FILE})

set_target_properties(xsock_static PROPERTIES OUTPUT_NAME xsock)
set_target_properties(xsock_shared PROPERTIES OUTPUT_NAME xsock)

set_target_properties(xsock_static PROPERTIES CLEAN_DIRECT_OUTPUT 1)
set_target_properties(xsock_shared PROPERTIES CLEAN_DIRECT_OUTPUT 1)

#add_subdirectory(example)

