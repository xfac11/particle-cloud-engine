cmake_minimum_required(VERSION 3.16)
project(Engine.Compute)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CSHARP_BIN_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../Engine.API/Engine.API/bin/x64/Debug/net8.0)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CSHARP_BIN_DIR})



add_library(Engine.Compute SHARED
    src/test.cpp
    src/particle.cpp
    include/test.hpp
    include/pch.hpp
    include/particle.hpp
)

target_include_directories(Engine.Compute PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

set_target_properties(Engine.Compute PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${CSHARP_BIN_DIR}
)

target_precompile_headers(Engine.Compute PRIVATE include/pch.hpp)

