cmake_minimum_required(VERSION 3.10)
project(coilasm VERSION 0.1.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler options
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic")
endif()

# Include directories
include_directories(
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_SOURCE_DIR}/include
)

# Source files
set(SOURCES
    src/main.cpp
    src/core/instruction.cpp
    src/core/operand.cpp
    src/core/register.cpp
    src/core/type.cpp
    src/core/variable.cpp
    src/parser/lexer.cpp
    src/parser/parser.cpp
    src/binary/cof.cpp
    src/binary/section.cpp
    src/binary/symbol.cpp
    src/target/target.cpp
    src/target/x86_64.cpp
    src/util/logger.cpp
    src/util/diagnostic.cpp
    src/util/source_location.cpp
)

# Create the executable
add_executable(coilasm ${SOURCES})

# Installation
install(TARGETS coilasm DESTINATION bin)

# Testing
enable_testing()
add_subdirectory(test)