cmake_minimum_required(VERSION 3.20)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")

project(thread_pool C CXX)
find_package (Threads)
option(TP_CONSOLE "Write activity of thread pool to console" ON)

if(TP_CONSOLE)
    add_definitions(-DTP_CONSOLE)
endif()

add_library(threadpool src/threadpool.cpp)

add_executable(run_sample 
     example.cpp)

target_link_libraries(run_sample
               threadpool)