###############################################################################
# djinterp - test framework module
# 
# Build configuration for the djinterp test framework and its own unit tests.
# Tests the standalone test framework (test_standalone, test_common) to ensure
# the testing infrastructure itself is correct.
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
set(TEST_DIR                "${TEST_FW_ROOT}/tests/c/test")
set(CORE_TEST_DIR           "${TEST_FW_ROOT}/tests/c")
set(TEST_FRAMEWORK_SRC_DIR  "${TEST_FW_ROOT}/src/c/test")
set(TEST_FRAMEWORK_INC_DIR  "${TEST_FW_ROOT}/inc/c/test")

# Set output directories
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${TEST_FW_ROOT}/lib/test/c")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${TEST_FW_ROOT}/lib/test/c")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${TEST_FW_ROOT}/bin/tests/c/test")

# For multi-config generators (Visual Studio)
foreach(CONFIG ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER ${CONFIG} CONFIG_UPPER)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${CONFIG_UPPER} "${TEST_FW_ROOT}/bin/tests/c/test")
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
include(${CMAKE_CURRENT_SOURCE_DIR}/../../tests_standalone.cmake)

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
message(STATUS "Test dir:           ${TEST_DIR}")
message(STATUS "Platform:           ${PLATFORM_ARCH}")
message(STATUS "========================================")

###############################################################################
# COMPILER FLAGS
###############################################################################

if(MSVC)
    add_compile_options(/FS)
endif()

###############################################################################
# TEST SUPPORT LIBRARY
###############################################################################

# Create the test support library if not already provided by core/
if(NOT TARGET test-standalone-support)
    add_library(test-standalone-support STATIC
        "${SOURCE_DIR}/djinterp.c"
        "${TEST_FRAMEWORK_SRC_DIR}/test_standalone.c"
        "${TEST_FRAMEWORK_SRC_DIR}/test_common.c"
        "${SOURCE_DIR}/dmemory.c"
        "${SOURCE_DIR}/string_fn.c"
        "${SOURCE_DIR}/dfile.c"
        "${SOURCE_DIR}/dstring.c"
    )
    target_include_directories(test-standalone-support PUBLIC
        ${INCLUDE_DIR}
        ${TEST_FRAMEWORK_INC_DIR}
        ${CORE_TEST_DIR}
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
endif()

###############################################################################
# .config directory for main.c files
###############################################################################

set(CONFIG_TEST_DIR "${TEST_FW_ROOT}/.config/.msvs/testing/c/test")

###############################################################################
# test_common tests
###############################################################################

set(TEST_COMMON_MAIN "${CONFIG_TEST_DIR}/djinterp-c-test-common-tests-sa/main.c")
if(EXISTS "${TEST_COMMON_MAIN}")
    # Gather test_common test files
    file(GLOB TEST_COMMON_TEST_FILES "${TEST_DIR}/test_common_tests_sa*.c")

    # Filter out _main.c files
    set(TEST_COMMON_FILTERED "")
    foreach(FILE ${TEST_COMMON_TEST_FILES})
        get_filename_component(FILENAME "${FILE}" NAME)
        if(NOT FILENAME MATCHES "_main\\.c$")
            list(APPEND TEST_COMMON_FILTERED "${FILE}")
        endif()
    endforeach()

    set(TEST_COMMON_TARGET "djinterp-c-test-common-tests-sa-${PLATFORM_ARCH}")

    add_executable(${TEST_COMMON_TARGET}
        "${TEST_COMMON_MAIN}"
        ${TEST_COMMON_FILTERED}
    )

    target_include_directories(${TEST_COMMON_TARGET} PRIVATE
        ${INCLUDE_DIR}
        ${TEST_FRAMEWORK_INC_DIR}
        ${CORE_TEST_DIR}
        ${TEST_DIR}
    )

    target_compile_definitions(${TEST_COMMON_TARGET} PRIVATE D_TESTING=1)

    if(MSVC)
        target_compile_options(${TEST_COMMON_TARGET} PRIVATE /FS)
        set_target_properties(${TEST_COMMON_TARGET} PROPERTIES
            COMPILE_PDB_NAME "${TEST_COMMON_TARGET}"
            COMPILE_PDB_OUTPUT_DIRECTORY
                "${CMAKE_CURRENT_BINARY_DIR}/${TEST_COMMON_TARGET}.dir"
        )
    endif()

    target_link_libraries(${TEST_COMMON_TARGET} PRIVATE
        test-standalone-support
    )

    message(STATUS "Created test executable: ${TEST_COMMON_TARGET}")
else()
    message(STATUS "Skipping test_common tests (no main.c at ${TEST_COMMON_MAIN})")
endif()

###############################################################################
# test_standalone tests
###############################################################################

set(TEST_STANDALONE_MAIN "${CONFIG_TEST_DIR}/djinterp-c-test-standalone-tests-sa/main.c")
if(EXISTS "${TEST_STANDALONE_MAIN}")
    # Gather test_standalone test files
    file(GLOB TEST_STANDALONE_TEST_FILES "${TEST_DIR}/test_standalone_tests_sa*.c")

    # Filter out _main.c files
    set(TEST_STANDALONE_FILTERED "")
    foreach(FILE ${TEST_STANDALONE_TEST_FILES})
        get_filename_component(FILENAME "${FILE}" NAME)
        if(NOT FILENAME MATCHES "_main\\.c$")
            list(APPEND TEST_STANDALONE_FILTERED "${FILE}")
        endif()
    endforeach()

    set(TEST_STANDALONE_TARGET "djinterp-c-test-standalone-tests-sa-${PLATFORM_ARCH}")

    add_executable(${TEST_STANDALONE_TARGET}
        "${TEST_STANDALONE_MAIN}"
        ${TEST_STANDALONE_FILTERED}
    )

    target_include_directories(${TEST_STANDALONE_TARGET} PRIVATE
        ${INCLUDE_DIR}
        ${TEST_FRAMEWORK_INC_DIR}
        ${CORE_TEST_DIR}
        ${TEST_DIR}
    )

    target_compile_definitions(${TEST_STANDALONE_TARGET} PRIVATE D_TESTING=1)

    if(MSVC)
        target_compile_options(${TEST_STANDALONE_TARGET} PRIVATE /FS)
        set_target_properties(${TEST_STANDALONE_TARGET} PROPERTIES
            COMPILE_PDB_NAME "${TEST_STANDALONE_TARGET}"
            COMPILE_PDB_OUTPUT_DIRECTORY
                "${CMAKE_CURRENT_BINARY_DIR}/${TEST_STANDALONE_TARGET}.dir"
        )
    endif()

    target_link_libraries(${TEST_STANDALONE_TARGET} PRIVATE
        test-standalone-support
    )

    message(STATUS "Created test executable: ${TEST_STANDALONE_TARGET}")
else()
    message(STATUS "Skipping test_standalone tests (no main.c at ${TEST_STANDALONE_MAIN})")
endif()

###############################################################################
# SUMMARY
###############################################################################

message(STATUS "")
message(STATUS "Test Framework Build Summary:")
message(STATUS "  Test executables: test-common-tests-sa, test-standalone-tests-sa")
message(STATUS "")