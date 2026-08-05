cmake_minimum_required(VERSION 3.22)

if(NOT CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
endif()

project(EngineCompute LANGUAGES CXX)

add_library(Engine.Compute SHARED
    src/ParticleEngine.cpp
)

add_executable(Engine.Compute.Test 
    src/ParticleEngine.cpp
    src/main.cpp
)

find_package(Vulkan REQUIRED)
find_package(glm CONFIG REQUIRED)

target_link_libraries(Engine.Compute PRIVATE Vulkan::Vulkan glm::glm)
target_include_directories(Engine.Compute PRIVATE include)

target_link_libraries(Engine.Compute.Test PRIVATE Vulkan::Vulkan glm::glm)
target_include_directories(Engine.Compute.Test PRIVATE include)