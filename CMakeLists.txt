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
target_include_directories(threadpool PUBLIC src)

list(APPEND ATHREAD_SAMPLES
  ex_auto_shutdown 
  ex_lambda
  ex_simple
)

foreach(sample IN LISTS ATHREAD_SAMPLES)
  add_executable(${sample} samples/${sample}.cpp)
  target_link_libraries(${sample} threadpool)
endforeach()

