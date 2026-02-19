###############################################################################
# djinterp - container module
# 
# Build configuration for container djinterp modules and tests.
# Includes data structures such as maps, lists, vectors, trees, etc.
#
# Location: <root>/build/cmake/config/c/container/CMakeLists.txt
#
# author(s): Samuel 'teer' Neal-Blim                          date: 2026.02.18
###############################################################################
cmake_minimum_required(VERSION 3.20)

# Project name
project(djinterp-container VERSION 0.1 LANGUAGES C CXX)

###############################################################################
# DIRECTORY SETUP
###############################################################################

# Determine the project root directory
# This CMakeLists.txt is located at: <root>/build/cmake/config/c/container/CMakeLists.txt
# So we need to go up 5 levels to reach the root
get_filename_component(CONTAINER_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../../../.." ABSOLUTE)

# Set directory paths
set(SOURCE_DIR              "${CONTAINER_ROOT}/src/c")
set(INCLUDE_DIR             "${CONTAINER_ROOT}/inc/c")
set(TEST_DIR                "${CONTAINER_ROOT}/tests/c")
set(TEST_FRAMEWORK_SRC_DIR  "${CONTAINER_ROOT}/src/c/test")
set(TEST_FRAMEWORK_INC_DIR  "${CONTAINER_ROOT}/inc/c/test")

# Set output directories
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CONTAINER_ROOT}/lib/container/c")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CONTAINER_ROOT}/lib/container/c")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CONTAINER_ROOT}/bin/tests/c/container")

# For multi-config generators (Visual Studio)
foreach(CONFIG ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER ${CONFIG} CONFIG_UPPER)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${CONFIG_UPPER} "${CONTAINER_ROOT}/bin/tests/c/container")
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${CONFIG_UPPER} "${CONTAINER_ROOT}/lib/container/c")
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${CONFIG_UPPER} "${CONTAINER_ROOT}/lib/container/c")
endforeach()

# Detect platform architecture
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(PLATFORM_ARCH "x64")
else()
    set(PLATFORM_ARCH "x86")
endif()

# Include common CMake utilities
include(${CMAKE_CURRENT_SOURCE_DIR}/../../common.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/../../tests_standalone.cmake)

# Set C/C++ standards
set(CMAKE_C_STANDARD 17 CACHE INTERNAL "")
set(CMAKE_C_STANDARD_REQUIRED ON CACHE INTERNAL "")
set(CMAKE_CXX_STANDARD 23 CACHE INTERNAL "")
set(CMAKE_CXX_STANDARD_REQUIRED ON CACHE INTERNAL "")

# Print configuration
message(STATUS "========================================")
message(STATUS "djinterp Container Module Configuration")
message(STATUS "========================================")
message(STATUS "Root:               ${CONTAINER_ROOT}")
message(STATUS "Source dir:         ${SOURCE_DIR}")
message(STATUS "Include dir:        ${INCLUDE_DIR}")
message(STATUS "Platform:           ${PLATFORM_ARCH}")
message(STATUS "========================================")

###############################################################################
# COMPILER FLAGS
###############################################################################

if(MSVC)
    add_compile_options(/FS)
endif()

###############################################################################
# CONTAINER LIBRARIES
###############################################################################

# TODO: Add container libraries as they are implemented
# Example:
#   add_library(dvector STATIC "${SOURCE_DIR}/dvector.c")
#   target_include_directories(dvector PUBLIC ${INCLUDE_DIR})
#   target_link_libraries(dvector PUBLIC djinterp dmemory)

###############################################################################
# CONTAINER TEST EXECUTABLES
###############################################################################

# TODO: Add container test executables as they are implemented

###############################################################################
# SUMMARY
###############################################################################

message(STATUS "")
message(STATUS "Container Build Summary:")
message(STATUS "  Libraries:        (none yet)")
message(STATUS "  Test executables: (none yet)")
message(STATUS "")