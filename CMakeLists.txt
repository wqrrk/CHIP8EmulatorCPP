cmake_minimum_required(VERSION 3.20)

project(CHIP8Emulator LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(CHIP8Emulator
        main.cpp
        Chip8.cpp
        Chip8.h
        Chip8.h
        Chip8.cpp
        RuntimeFaults.h
        RuntimeFaults.cpp
        StaticFaults.cpp
)

target_compile_options(CHIP8Emulator PRIVATE
        -Wall
        -Wextra
        -Wpedantic
)