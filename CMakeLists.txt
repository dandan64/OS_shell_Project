cmake_minimum_required(VERSION 3.10)
project(cmake-build-debug/smash VERSION 1.0 LANGUAGES CXX)

# Require C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Warning flags
if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# List your source files here:
add_executable(smash
        smash.cpp
        Commands.cpp
        signals.cpp
        helper.cpp
)

# If you use std::experimental::filesystem (GCC < 9), link this:
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_link_libraries(smash PRIVATE stdc++fs)
endif()
