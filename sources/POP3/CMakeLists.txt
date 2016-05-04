cmake_minimum_required(VERSION 2.8)
project(POP3Client)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES main.cpp pop3.cpp socket.cpp format.cc)
add_executable(POP3Client ${SOURCE_FILES})