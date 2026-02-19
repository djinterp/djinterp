###############################################################################
# djinterp - C language submodule
# 
# Build configuration for C-based djinterp modules
# This is the intermediate CMakeLists.txt that sits between the config root
# and the individual module directories (core, etc.)
#
# Location: <root>/build/cmake/config/c/CMakeLists.txt
#
# author(s): Samuel 'teer' Neal-Blim                          date: 2023.03.01
###############################################################################
cmake_minimum_required(VERSION 3.20)

# Project name
project(djinterp-c VERSION 0.1 LANGUAGES C CXX)

###############################################################################
# DIRECTORY SETUP
###############################################################################

# Determine the project root directory
# This CMakeLists.txt is located at: <root>/build/cmake/config/c/CMakeLists.txt
# So we need to go up 4 levels to reach the root
get_filename_component(DJINTERP_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../../.." ABSOLUTE)

# Set the C language subdirectory paths (shared across all C submodules)
set(C_SOURCE_DIR              "${DJINTERP_ROOT}/src/c")
set(C_INCLUDE_DIR             "${DJINTERP_ROOT}/inc/c")
set(C_TEST_DIR                "${DJINTERP_ROOT}/tests/c")
set(C_TEST_FRAMEWORK_SRC_DIR  "${DJINTERP_ROOT}/src/c/test")
set(C_TEST_FRAMEWORK_INC_DIR  "${DJINTERP_ROOT}/inc/c/test")

# Detect platform architecture
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(PLATFORM_ARCH "x64")
else()
    set(PLATFORM_ARCH "x86")
endif()

# Include common CMake utilities
include(${CMAKE_CURRENT_SOURCE_DIR}/../common.cmake)

# Set C/C++ standards
set(CMAKE_C_STANDARD 17 CACHE INTERNAL "")
set(CMAKE_C_STANDARD_REQUIRED ON CACHE INTERNAL "")
set(CMAKE_CXX_STANDARD 23 CACHE INTERNAL "")
set(CMAKE_CXX_STANDARD_REQUIRED ON CACHE INTERNAL "")

# Print configuration
message(STATUS "========================================")
message(STATUS "djinterp C Submodule Configuration")
message(STATUS "========================================")
message(STATUS "Root:              ${DJINTERP_ROOT}")
message(STATUS "C Source dir:      ${C_SOURCE_DIR}")
message(STATUS "C Include dir:     ${C_INCLUDE_DIR}")
message(STATUS "C Test dir:        ${C_TEST_DIR}")
message(STATUS "Platform:          ${PLATFORM_ARCH}")
message(STATUS "========================================")

###############################################################################
# COMPILER FLAGS
###############################################################################

# Add /FS flag for MSVC to avoid parallel compilation PDB issues
if(MSVC)
    add_compile_options(/FS)
endif()

###############################################################################
# SUBMODULES
###############################################################################

# Core C module
add_subdirectory(core)

# Container module
add_subdirectory(container)

# Test framework module
add_subdirectory(test)