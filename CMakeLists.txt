cmake_minimum_required(VERSION 3.20)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(UNIX)
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
endif()

project(threadpool C CXX)
option(TP_CONSOLE "Write activity of thread pool to console" ON)

if(TP_CONSOLE)
    add_definitions(-DTP_CONSOLE)
endif()

add_library(threadpool src/threadpool.cpp)

add_executable(ex_simple samples/ex_simple.cpp)

target_link_libraries(ex_simple
               threadpool)
target_include_directories(ex_simple PUBLIC src)

