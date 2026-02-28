###############################################################################
# djinterp - C / container component
# 
# Build configuration for container data structure modules and standalone tests.
#
# Modules built here:
#   container        — base container types
#   array            — fixed-size array (array, array_common, array_filter,
#                      circular_array, ptr_array)
#   map              — map types (enum_map_entry, map, min_enum_map)
#   registry         — registry types (registry, registry_common)
#   vector           — dynamic array (ptr_vector, vector, vector_common)
#
# Location: <root>/build/cmake/config/c/container/CMakeLists.txt
#
# author(s): Samuel 'teer' Neal-Blim                          date: 2026.02.25
###############################################################################
cmake_minimum_required(VERSION 3.20)

# Project name
project(djinterp-c-container VERSION 0.1 LANGUAGES C CXX)

###############################################################################
# DIRECTORY SETUP
###############################################################################

# Determine the project root if not inherited (standalone invocation)
# This file is at: <root>/build/cmake/config/c/container/CMakeLists.txt
#   container -> c -> config -> cmake -> build -> <root>  (5 levels)
if(NOT DEFINED DJINTERP_ROOT)
    get_filename_component(DJINTERP_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../../../.." ABSOLUTE)
endif()

# Parent-level C directories
if(NOT DEFINED C_SOURCE_DIR)
    set(C_SOURCE_DIR "${DJINTERP_ROOT}/src/c")
endif()
if(NOT DEFINED C_INCLUDE_DIR)
    set(C_INCLUDE_DIR "${DJINTERP_ROOT}/inc/c")
endif()
if(NOT DEFINED C_TEST_DIR)
    set(C_TEST_DIR "${DJINTERP_ROOT}/tests/c")
endif()
if(NOT DEFINED C_TEST_FRAMEWORK_SRC_DIR)
    set(C_TEST_FRAMEWORK_SRC_DIR "${DJINTERP_ROOT}/src/c/test")
endif()
if(NOT DEFINED C_TEST_FRAMEWORK_INC_DIR)
    set(C_TEST_FRAMEWORK_INC_DIR "${DJINTERP_ROOT}/inc/c/test")
endif()
if(NOT DEFINED C_CONFIG_TEST_DIR)
    set(C_CONFIG_TEST_DIR "${DJINTERP_ROOT}/.config/.msvs/testing/c")
endif()

# Local aliases expected by tests_standalone.cmake / tests_common.cmake
set(SOURCE_DIR              "${C_SOURCE_DIR}/container")
set(INCLUDE_DIR             "${C_INCLUDE_DIR}")
set(TEST_DIR                "${C_TEST_DIR}/container")
set(TEST_FRAMEWORK_SRC_DIR  "${C_TEST_FRAMEWORK_SRC_DIR}")
set(TEST_FRAMEWORK_INC_DIR  "${C_TEST_FRAMEWORK_INC_DIR}")
set(CONFIG_TEST_DIR         "${C_CONFIG_TEST_DIR}/container")

# Parent-level test dir for shared test headers
set(C_TEST_BASE_DIR "${DJINTERP_ROOT}/tests/c")

# Detect platform if not inherited
if(NOT DEFINED PLATFORM_ARCH)
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(PLATFORM_ARCH "x64")
    else()
        set(PLATFORM_ARCH "x86")
    endif()
endif()

# Set output directories
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${DJINTERP_ROOT}/lib/c/container")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${DJINTERP_ROOT}/lib/c/container")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${DJINTERP_ROOT}/bin/tests/c/container")

foreach(CONFIG ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER ${CONFIG} CONFIG_UPPER)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${CONFIG_UPPER} "${DJINTERP_ROOT}/bin/tests/c/container")
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${CONFIG_UPPER} "${DJINTERP_ROOT}/lib/c/container")
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${CONFIG_UPPER} "${DJINTERP_ROOT}/lib/c/container")
endforeach()

# Include shared cmake utilities
include(${CMAKE_CURRENT_SOURCE_DIR}/../../common.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/../../tests_standalone.cmake)

# Print configuration
message(STATUS "  ---- Container Component ----")
message(STATUS "  Source dir:        ${SOURCE_DIR}")
message(STATUS "  Include dir:       ${INCLUDE_DIR}")
message(STATUS "  Test dir:          ${TEST_DIR}")
message(STATUS "  Config test dir:   ${CONFIG_TEST_DIR}")
message(STATUS "  Bin dir:           ${DJINTERP_ROOT}/bin/tests/c/container")
message(STATUS "  Lib dir:           ${DJINTERP_ROOT}/lib/c/container")

###############################################################################
# CONTAINER LIBRARY
#
# Single static library containing all container sources.
#
# FIX: Added ptr_vector.c — previously missing, causing 51 LNK2019 errors in
#      ptr_vector test executable (all d_ptr_vector_* symbols unresolved).
###############################################################################

if(NOT TARGET container)
    add_library(container STATIC
        "${SOURCE_DIR}/container.c"
        # array
        "${SOURCE_DIR}/array/array.c"
        "${SOURCE_DIR}/array/array_common.c"
        "${SOURCE_DIR}/array/array_filter.c"
        "${SOURCE_DIR}/array/circular_array.c"
        "${SOURCE_DIR}/array/ptr_array.c"
        # map
        "${SOURCE_DIR}/map/enum_map_entry.c"
        "${SOURCE_DIR}/map/map.c"
        "${SOURCE_DIR}/map/min_enum_map.c"
        # registry
        "${SOURCE_DIR}/registry/registry.c"
        "${SOURCE_DIR}/registry/registry_common.c"
        # vector
        "${SOURCE_DIR}/vector/ptr_vector.c"   # FIX: was missing → caused 51 LNK2019 errors
        "${SOURCE_DIR}/vector/vector.c"
        "${SOURCE_DIR}/vector/vector_common.c"
    )
    target_include_directories(container PUBLIC ${C_INCLUDE_DIR})
    target_link_libraries(container PUBLIC djinterp dmemory string_fn)
    target_compile_definitions(container PRIVATE D_TESTING=1)
endif()

###############################################################################
# COMPILER FLAGS
###############################################################################

if(MSVC)
    add_compile_options(/FS)
endif()

###############################################################################
# TEST SUPPORT LIBRARY
###############################################################################

if(NOT TARGET test-standalone-support)
    add_library(test-standalone-support STATIC
        "${C_SOURCE_DIR}/djinterp.c"
        "${C_TEST_FRAMEWORK_SRC_DIR}/test_standalone.c"
        "${C_TEST_FRAMEWORK_SRC_DIR}/test_common.c"
        "${C_SOURCE_DIR}/dmemory.c"
        "${C_SOURCE_DIR}/string_fn.c"
        "${C_SOURCE_DIR}/dfile.c"
        "${C_SOURCE_DIR}/dstring.c"
    )
    target_include_directories(test-standalone-support PUBLIC
        ${C_INCLUDE_DIR}
        ${TEST_FRAMEWORK_INC_DIR}
        ${C_TEST_BASE_DIR}
        ${TEST_DIR}
    )
    target_compile_definitions(test-standalone-support PUBLIC D_TESTING=1)

    if(MSVC)
        target_compile_options(test-standalone-support PRIVATE /FS)
        set_target_properties(test-standalone-support PROPERTIES
            COMPILE_PDB_NAME "test-standalone-support"
            COMPILE_PDB_OUTPUT_DIRECTORY
                "${CMAKE_CURRENT_BINARY_DIR}/test-standalone-support.dir"
        )
    endif()

    message(STATUS "  Created test support library: test-standalone-support")
endif()

###############################################################################
# CONTAINER SUB-COMPONENT TESTS
#
# Each sub-component directory defines its own test executables.
###############################################################################

add_subdirectory(array)
add_subdirectory(map)
add_subdirectory(registry)
add_subdirectory(vector)

###############################################################################
# SUMMARY
###############################################################################

message(STATUS "")
message(STATUS "  Container Build Summary:")
message(STATUS "    Libraries:        container")
message(STATUS "    Sub-components:   array, map, registry, vector")
message(STATUS "    Test framework:   Standalone (library-based)")
message(STATUS "")
