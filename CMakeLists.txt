cmake_minimum_required(VERSION 3.22)
project(gos)

set(CMAKE_CXX_STANDARD 17)

add_subdirectory(proto)

include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(proto)
include_directories(logs_reader)

add_library(logs_reader_static STATIC logs_reader/logs_graph.cpp)
target_link_libraries(logs_reader_static proto ${PROTOBUF_LIBRARY})

add_executable(serialize bin/serialize.cpp)
add_executable(deserialize bin/deserialize.cpp)
target_link_libraries(main proto ${PROTOBUF_LIBRARY})
