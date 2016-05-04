cmake_minimum_required(VERSION 2.8)
project(HTTPServer)
find_package(Threads)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
set(CMAKE_BUILD_TYPE Debug)

set(SOURCE_FILES main.cpp format.cc)
add_executable(HTTPServer ${SOURCE_FILES})
target_link_libraries (HTTPServer ${CMAKE_THREAD_LIBS_INIT})
