###############################################################################
# djinterp - top-level build configuration
# 
# Root entry point for the djinterp build system.
# Delegates to language-specific subdirectories.
#
# Location: <root>/build/cmake/config/CMakeLists.txt
#
# author(s): Samuel 'teer' Neal-Blim                          date: 2023.03.01
###############################################################################
cmake_minimum_required(VERSION 3.20)

# Project name
project(djinterp VERSION 0.1 LANGUAGES C CXX)

###############################################################################
# DIRECTORY SETUP
###############################################################################

# Determine the project root directory
# This CMakeLists.txt is located at: <root>/build/cmake/config/CMakeLists.txt
# So we need to go up 3 levels to reach the root
get_filename_component(DJINTERP_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../.." ABSOLUTE)

# Detect platform architecture
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(PLATFORM_ARCH "x64")
else()
    set(PLATFORM_ARCH "x86")
endif()

# Include common CMake utilities
include(${CMAKE_CURRENT_SOURCE_DIR}/common.cmake)

# Print configuration
message(STATUS "========================================")
message(STATUS "djinterp Top-Level Configuration")
message(STATUS "========================================")
message(STATUS "Root:              ${DJINTERP_ROOT}")
message(STATUS "Platform:          ${PLATFORM_ARCH}")
message(STATUS "========================================")

###############################################################################
# LANGUAGE SUBDIRECTORIES
###############################################################################

# C language modules
add_subdirectory(c)