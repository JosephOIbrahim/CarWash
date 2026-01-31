# HdCarWash — Cognitively-Aware AI Render Delegate
# Root CMakeLists.txt
#
# Build Requirements:
#   - Houdini 21.x with HDK
#   - CMake 3.20+
#   - C++17 compiler
#
# Build Commands:
#   cmake -B build -S . -DCMAKE_PREFIX_PATH="$HFS/toolkit/cmake"
#   cmake --build build --config Release --parallel
#   cmake --install build --prefix "$HOUDINI_USER_PREF_DIR"

cmake_minimum_required(VERSION 3.20)

project(HdCarWash
    VERSION 0.1.0
    DESCRIPTION "Cognitively-Aware AI Render Delegate for Houdini Solaris"
    LANGUAGES CXX
)

# ==============================================================================
# Configuration Options
# ==============================================================================

option(HDCARWASH_BUILD_TESTS "Build unit tests" ON)
option(HDCARWASH_BUILD_DOCS "Build documentation" OFF)
option(HDCARWASH_ENABLE_LTX2 "Enable LTX-2 as primary target" ON)
option(HDCARWASH_ENABLE_COSMOS "Enable Cosmos WFM integration hooks" ON)
option(HDCARWASH_ENABLE_DETERMINISM "Enable batch-invariant determinism" ON)

# ==============================================================================
# Find Houdini HDK
# ==============================================================================

# Houdini provides its own CMake configuration
list(APPEND CMAKE_PREFIX_PATH "$ENV{HFS}/toolkit/cmake")

find_package(Houdini REQUIRED)

# Houdini sets these automatically, but we document them
message(STATUS "Houdini version: ${Houdini_VERSION}")
message(STATUS "Houdini install: $ENV{HFS}")
message(STATUS "USD version: ${USD_VERSION}")

# ==============================================================================
# C++ Standard
# ==============================================================================

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ==============================================================================
# Compiler Flags
# ==============================================================================

if(MSVC)
    add_compile_options(/W4 /WX-)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# ==============================================================================
# USD/Hydra Libraries (via Houdini)
# ==============================================================================

# Houdini bundles USD — all libraries accessed via the Houdini CMake target.
# No need to specify individual USD libs; houdini_configure_target() handles it.

# ==============================================================================
# Build Subdirectories
# ==============================================================================

add_subdirectory(plugin/hdCarWash)

if(HDCARWASH_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests/cpp)
endif()

# ==============================================================================
# Installation
# ==============================================================================

# Install plugin info for USD registry
install(
    FILES plugin/plugInfo.json
    DESTINATION plugin/usd/hdCarWash/resources
)

# Install schemas
install(
    DIRECTORY schema/
    DESTINATION plugin/usd/hdCarWash/resources/schema
    FILES_MATCHING PATTERN "*.usda"
)

# Install Houdini integration
install(
    DIRECTORY houdini/
    DESTINATION houdini
    FILES_MATCHING
        PATTERN "*.py"
        PATTERN "*.pypanel"
        PATTERN "*.shelf"
        PATTERN "*.hda"
)

# ==============================================================================
# Package Info
# ==============================================================================

message(STATUS "")
message(STATUS "HdCarWash Configuration:")
message(STATUS "  Version:      ${PROJECT_VERSION}")
message(STATUS "  LTX-2:        ${HDCARWASH_ENABLE_LTX2}")
message(STATUS "  Cosmos:       ${HDCARWASH_ENABLE_COSMOS}")
message(STATUS "  Determinism:  ${HDCARWASH_ENABLE_DETERMINISM}")
message(STATUS "  Tests:        ${HDCARWASH_BUILD_TESTS}")
message(STATUS "")
