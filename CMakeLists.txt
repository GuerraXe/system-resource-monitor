cmake_minimum_required(VERSION 3.21)

project(system_resource_monitor
    LANGUAGES CXX
    DESCRIPTION "Cross-platform system resource monitor CLI"
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Emit compile_commands.json for editor tooling (clangd, etc.)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if (MSVC)
    add_compile_options(/W4 /permissive- /Zc:__cplusplus /EHsc)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

option(SRM_BUILD_TESTS "Build the unit/integration test suite" ON)

add_subdirectory(src)

if (SRM_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
