cmake_minimum_required(VERSION 3.20)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

project(thread_pool C CXX)

option(TP_CONSOLE "Write activity of thread pool to console" ON)

if(TP_CONSOLE)
    add_definitions(-DTP_CONSOLE)
endif()

add_executable(thread_pool 
     example.cpp 
     RunnableExample.h
     src/noncopyable.h
     src/threadpool.h
     src/threadpool.cpp)
