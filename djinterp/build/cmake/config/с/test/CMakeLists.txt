###############################################################################
# djinterp - test framework module
# 
# Build configuration for the djinterp test framework libraries.
# Provides the standalone test framework (test_standalone, test_common)
# as a reusable library that can be consumed by other submodules without
# rebuilding core dependencies.
#
# Location: <root>/build/cmake/config/c/test/CMakeLists.txt
#
# author(s): Samuel 'teer' Neal-Blim                          date: 2026.02.18
###############################################################################
cmake_minimum_required(VERSION 3.20)

# Project name
project(djinterp-test VERSION 0.1 LANGUAGES C CXX)

###############################################################################
# DIRECTORY SETUP
###############################################################################

# Determine the project root directory
# This CMakeLists.txt is located at: <root>/build/cmake/config/c/test/CMakeLists.txt
# So we need to go up 5 levels to reach the root
get_filename_component(TEST_FW_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../../../.." ABSOLUTE)

# Set directory paths
set(SOURCE_DIR              "${TEST_FW_ROOT}/src/c")
set(INCLUDE_DIR             "${TEST_FW_ROOT}/inc/c")
set(TEST_DIR                "${TEST_FW_ROOT}/tests/c")
set(TEST_FRAMEWORK_SRC_DIR  "${TEST_FW_ROOT}/src/c/test")
set(TEST_FRAMEWORK_INC_DIR  "${TEST_FW_ROOT}/inc/c/test")

# Set output directories
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${TEST_FW_ROOT}/lib/test/c")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${TEST_FW_ROOT}/lib/test/c")

# For multi-config generators (Visual Studio)
foreach(CONFIG ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER ${CONFIG} CONFIG_UPPER)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${CONFIG_UPPER} "${TEST_FW_ROOT}/lib/test/c")
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${CONFIG_UPPER} "${TEST_FW_ROOT}/lib/test/c")
endforeach()

# Detect platform architecture
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(PLATFORM_ARCH "x64")
else()
    set(PLATFORM_ARCH "x86")
endif()

# Include common CMake utilities
include(${CMAKE_CURRENT_SOURCE_DIR}/../../common.cmake)

# Set C/C++ standards
set(CMAKE_C_STANDARD 17 CACHE INTERNAL "")
set(CMAKE_C_STANDARD_REQUIRED ON CACHE INTERNAL "")
set(CMAKE_CXX_STANDARD 23 CACHE INTERNAL "")
set(CMAKE_CXX_STANDARD_REQUIRED ON CACHE INTERNAL "")

# Print configuration
message(STATUS "========================================")
message(STATUS "djinterp Test Framework Configuration")
message(STATUS "========================================")
message(STATUS "Root:               ${TEST_FW_ROOT}")
message(STATUS "Framework src:      ${TEST_FRAMEWORK_SRC_DIR}")
message(STATUS "Framework inc:      ${TEST_FRAMEWORK_INC_DIR}")
message(STATUS "Platform:           ${PLATFORM_ARCH}")
message(STATUS "========================================")

###############################################################################
# COMPILER FLAGS
###############################################################################

if(MSVC)
    add_compile_options(/FS)
endif()

###############################################################################
# TEST FRAMEWORK LIBRARIES
###############################################################################

# Standalone test framework (framework files only, no core dependencies)
# This is useful when a consuming submodule wants to provide its own
# core library linkage but needs the test framework sources compiled.
#
# For the fully-bundled support library (framework + djinterp + dmemory +
# string_fn + dfile + dstring), see test-standalone-support in core/.
if(NOT TARGET test-standalone-framework)
    add_library(test-standalone-framework STATIC
        "${TEST_FRAMEWORK_SRC_DIR}/test_standalone.c"
        "${TEST_FRAMEWORK_SRC_DIR}/test_common.c"
    )
    target_include_directories(test-standalone-framework PUBLIC
        ${INCLUDE_DIR}
        ${TEST_FRAMEWORK_INC_DIR}
    )
    target_compile_definitions(test-standalone-framework PUBLIC D_TESTING=1)

    if(MSVC)
        target_compile_options(test-standalone-framework PRIVATE /FS)
        set_target_properties(test-standalone-framework PROPERTIES
            COMPILE_PDB_NAME "test-standalone-framework"
            COMPILE_PDB_OUTPUT_DIRECTORY
                "${CMAKE_CURRENT_BINARY_DIR}/test-standalone-framework.dir"
        )
    endif()

    message(STATUS "Created library: test-standalone-framework")
endif()

# TODO: Add DTest framework library when implemented
# if(NOT TARGET dtest)
#     add_library(dtest STATIC
#         "${TEST_FRAMEWORK_SRC_DIR}/dtest.c"
#         "${TEST_FRAMEWORK_SRC_DIR}/dtest_runner.c"
#     )
#     target_include_directories(dtest PUBLIC
#         ${INCLUDE_DIR}
#         ${TEST_FRAMEWORK_INC_DIR}
#     )
#     target_link_libraries(dtest PUBLIC
#         djinterp dmemory string_fn dfile dstring dtime
#     )
#     message(STATUS "Created library: dtest")
# endif()

###############################################################################
# SUMMARY
###############################################################################

message(STATUS "")
message(STATUS "Test Framework Build Summary:")
message(STATUS "  Libraries:        test-standalone-framework")
message(STATUS "")