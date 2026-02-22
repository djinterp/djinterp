###############################################################################
# djinterp - C language module
# 
# Build configuration for all C-based djinterp submodules.
# Builds djinterpc.lib — the combined C library — and delegates to individual
# component CMakeLists (core, container, functional, test).
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
#   c -> config -> cmake -> build -> <root>
get_filename_component(DJINTERP_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../../.." ABSOLUTE)

# Set the C language directory paths (shared across all C submodules)
set(C_SOURCE_DIR              "${DJINTERP_ROOT}/src/c")
set(C_INCLUDE_DIR             "${DJINTERP_ROOT}/inc/c")
set(C_TEST_DIR                "${DJINTERP_ROOT}/tests/c")
set(C_TEST_FRAMEWORK_SRC_DIR  "${DJINTERP_ROOT}/src/c/test")
set(C_TEST_FRAMEWORK_INC_DIR  "${DJINTERP_ROOT}/inc/c/test")

# .config directory root for auto-generated main.c files
set(C_CONFIG_TEST_DIR         "${DJINTERP_ROOT}/.config/.msvs/testing/c")

# Detect platform architecture
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(PLATFORM_ARCH "x64")
else()
    set(PLATFORM_ARCH "x86")
endif()

# Include common CMake utilities
# common.cmake lives alongside the master at config/common.cmake
# From config/c/ we go up one level: ../common.cmake -> config/common.cmake
include(${CMAKE_CURRENT_SOURCE_DIR}/../common.cmake)

# Set C/C++ standards
set(CMAKE_C_STANDARD 17 CACHE INTERNAL "")
set(CMAKE_C_STANDARD_REQUIRED ON CACHE INTERNAL "")
set(CMAKE_CXX_STANDARD 23 CACHE INTERNAL "")
set(CMAKE_CXX_STANDARD_REQUIRED ON CACHE INTERNAL "")

# Print configuration
message(STATUS "========================================")
message(STATUS "djinterp C Module Configuration")
message(STATUS "========================================")
message(STATUS "Root:              ${DJINTERP_ROOT}")
message(STATUS "C Source dir:      ${C_SOURCE_DIR}")
message(STATUS "C Include dir:     ${C_INCLUDE_DIR}")
message(STATUS "C Test dir:        ${C_TEST_DIR}")
message(STATUS "C Test FW src:     ${C_TEST_FRAMEWORK_SRC_DIR}")
message(STATUS "C Test FW inc:     ${C_TEST_FRAMEWORK_INC_DIR}")
message(STATUS "Platform:          ${PLATFORM_ARCH}")
message(STATUS "========================================")

###############################################################################
# COMPILER FLAGS
###############################################################################

if(MSVC)
    add_compile_options(/FS)
endif()

###############################################################################
# SUBMODULES
#
# Each submodule defines its own libraries, test support, and test executables.
# Variables prefixed with C_ are available in child scopes.
###############################################################################

# Core C module — fundamental types and utilities
#   Libraries: djinterp, env, dmacro, dmemory, string_fn, dfile, dstring, dtime
add_subdirectory(core)

# Container module — data structure implementations
#   Libraries: container (enum_map_entry, min_enum_map, etc.)
add_subdirectory(container)

# Functional module — functional programming primitives
#   Libraries: functional, functional_common, compose, filter, fn_builder,
#              pipeline, predicate
add_subdirectory(functional)

# Test framework module — standalone test infrastructure self-tests
#   Libraries: test-standalone-support (if not already created by core)
add_subdirectory(test)

###############################################################################
# COMBINED LIBRARY: djinterpc.lib
#
# Aggregates all C submodule libraries into a single djinterpc.lib.
# Downstream targets can link against djinterpc to get everything.
###############################################################################

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${DJINTERP_ROOT}/lib/c")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${DJINTERP_ROOT}/lib/c")

foreach(CONFIG ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER ${CONFIG} CONFIG_UPPER)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${CONFIG_UPPER} "${DJINTERP_ROOT}/lib/c")
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${CONFIG_UPPER} "${DJINTERP_ROOT}/lib/c")
endforeach()

# Collect all core module source files
set(DJINTERPC_CORE_SOURCES
    "${C_SOURCE_DIR}/djinterp.c"
    "${C_SOURCE_DIR}/env.c"
    "${C_SOURCE_DIR}/dmemory.c"
    "${C_SOURCE_DIR}/string_fn.c"
    "${C_SOURCE_DIR}/dfile.c"
    "${C_SOURCE_DIR}/dstring.c"
    "${C_SOURCE_DIR}/dtime.c"
)

# Collect container module sources
set(DJINTERPC_CONTAINER_SOURCES "")
if(EXISTS "${C_SOURCE_DIR}/container")
    file(GLOB_RECURSE CONTAINER_C_FILES "${C_SOURCE_DIR}/container/*.c")
    list(APPEND DJINTERPC_CONTAINER_SOURCES ${CONTAINER_C_FILES})
endif()

# Collect functional module sources
set(DJINTERPC_FUNCTIONAL_SOURCES
    "${C_SOURCE_DIR}/functional/functional.c"
    "${C_SOURCE_DIR}/functional/functional_common.c"
    "${C_SOURCE_DIR}/functional/compose.c"
    "${C_SOURCE_DIR}/functional/filter.c"
    "${C_SOURCE_DIR}/functional/fn_builder.c"
    "${C_SOURCE_DIR}/functional/pipeline.c"
    "${C_SOURCE_DIR}/functional/predicate.c"
)

# Object library for master-level aggregation
add_library(djinterpc-objects OBJECT
    ${DJINTERPC_CORE_SOURCES}
    ${DJINTERPC_CONTAINER_SOURCES}
    ${DJINTERPC_FUNCTIONAL_SOURCES}
)
target_include_directories(djinterpc-objects PUBLIC
    ${C_INCLUDE_DIR}
)

# Static library: djinterpc.lib
add_library(djinterpc STATIC $<TARGET_OBJECTS:djinterpc-objects>)
set_target_properties(djinterpc PROPERTIES OUTPUT_NAME "djinterpc")
target_include_directories(djinterpc PUBLIC
    ${C_INCLUDE_DIR}
)

if(MSVC)
    set_target_properties(djinterpc PROPERTIES
        COMPILE_PDB_NAME "djinterpc"
        COMPILE_PDB_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/djinterpc.dir"
    )
endif()

message(STATUS "Created combined C library: djinterpc.lib")

###############################################################################
# SUMMARY
###############################################################################

message(STATUS "")
message(STATUS "djinterp C Build Summary:")
message(STATUS "  Submodules:       core, container, functional, test")
message(STATUS "  Combined library: djinterpc.lib")
message(STATUS "")