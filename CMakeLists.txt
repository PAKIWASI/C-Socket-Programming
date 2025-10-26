cmake_minimum_required(VERSION 3.20)
project(C_Socket_Programming LANGUAGES C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_COMPILER clang)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(chatserver src/chatserver.c)
add_executable(chatclient src/chatclient.c)


target_include_directories(chatserver PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_include_directories(chatclient PRIVATE ${CMAKE_SOURCE_DIR}/include)
