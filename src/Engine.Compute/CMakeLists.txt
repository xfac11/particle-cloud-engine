cmake_minimum_required(VERSION 3.16)
project(Engine.Compute)

add_library(Engine.Compute SHARED
    src/test.cpp
    include/test.hpp
)

target_include_directories(Engine.Compute PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

