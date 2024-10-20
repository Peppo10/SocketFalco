cmake_minimum_required(VERSION 3.10)

# Platform-specific configurations
if (WIN32)
    # Windows-specific settings
    set(CMAKE_CXX_COMPILER "C:/mingw64/bin/g++.exe")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static")
elseif (UNIX)
    # Linux-specific settings
    set(CMAKE_CXX_COMPILER "/usr/bin/g++")
endif()

# Project name
project(SocketFalco CXX)

# Set the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_SOURCE_DIR "src")

# Source files for the project
set(SOURCES
    ${CMAKE_SOURCE_DIR}/service.cpp
    ${CMAKE_SOURCE_DIR}/caching.cpp
    ${CMAKE_SOURCE_DIR}/connection.cpp
    ${CMAKE_SOURCE_DIR}/socket_falco.cpp # main source file
)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g3 -Wall -fdiagnostics-color=always")

add_executable(SocketFalco ${SOURCES})

if (WIN32)
    # Link Windows-specific libraries
    target_link_libraries(SocketFalco ws2_32 pthread uuid)
elseif (UNIX)
    # Link Linux-specific libraries
    target_link_libraries(SocketFalco pthread)
endif()

# Set output executable name
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR})