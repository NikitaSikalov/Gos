cmake_minimum_required(VERSION 3.22)
project(gos)

set(CMAKE_CXX_STANDARD 17)

add_subdirectory(proto)

include_directories(logs_reader)
include_directories(proto)

add_library(logs_reader_static STATIC logs_reader/logs_graph.cpp)
target_link_libraries(logs_reader_static proto)

add_executable(serialize bin/serialize.cpp)
add_executable(deserialize bin/deserialize.cpp)

target_link_libraries(serialize logs_reader_static)
target_link_libraries(deserialize logs_reader_static)
