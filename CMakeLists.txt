cmake_minimum_required(VERSION 3.10)
project(MultiuserChattingServer)

set(CMAKE_CXX_STANDARD 11)

include_directories(threadpool)

add_executable(MultiuserChattingServer
        threadpool/example.cpp
        threadpool/ThreadPool.h
        server.cpp)
