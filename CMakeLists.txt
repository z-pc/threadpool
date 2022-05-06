set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

project(thread_pool C CXX)

add_executable(thread_pool 
     example.cpp 
     src/noncopyable.h
     src/threadpool.h
     src/threadpool.cpp)