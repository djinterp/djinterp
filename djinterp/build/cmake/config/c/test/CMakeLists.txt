###############################################################################
# djinterp - test framework module
# 
# Build configuration for the djinterp test framework and its own unit tests.
# Tests the standalone test framework (assert, test_common, test_config,
# test_cvar, test_standalone) to ensure the testing infrastructure itself
# is correct.
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

get_filename_component(TEST_FW_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../../../.." ABSOLUTE)

set(SOURCE_DIR              "${TEST_FW_ROOT}/src/c")
set(INCLUDE_DIR             "${TEST_FW_ROOT}/inc/c")
set(TEST_DIR                "${TEST_FW_ROOT}/tests/c/test")
set(CORE_TEST_DIR           "${TEST_FW_ROOT}/tests/c")
set(TEST_FRAMEWORK_SRC_DIR  "${TEST_FW_ROOT}/src/c/test")
set(TEST_FRAMEWORK_INC_DIR  "${TEST_FW_ROOT}/inc/c/test")
set(CONFIG_TEST_DIR         "${TEST_FW_ROOT}/.config/.msvs/testing/c/test")

set(C_TEST_BASE_DIR "${TEST_FW_ROOT}/tests/c")

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${TEST_FW_ROOT}/lib/test/c")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${TEST_FW_ROOT}/lib/test/c")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${TEST_FW_ROOT}/bin/tests/c/test")

foreach(CONFIG ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER ${CONFIG} CONFIG_UPPER)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${CONFIG_UPPER} "${TEST_FW_ROOT}/bin/tests/c/test")
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${CONFIG_UPPER} "${TEST_FW_ROOT}/lib/test/c")
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${CONFIG_UPPER} "${TEST_FW_ROOT}/lib/test/c")
endforeach()

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(PLATFORM_ARCH "x64")
else()
    set(PLATFORM_ARCH "x86")
endif()

include(${CMAKE_CURRENT_SOURCE_DIR}/../../common.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/../../tests_standalone.cmake)

set(CMAKE_C_STANDARD 17 CACHE INTERNAL "")
set(CMAKE_C_STANDARD_REQUIRED ON CACHE INTERNAL "")
set(CMAKE_CXX_STANDARD 23 CACHE INTERNAL "")
set(CMAKE_CXX_STANDARD_REQUIRED ON CACHE INTERNAL "")

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
# TEST FRAMEWORK MODULE LIBRARIES
#
# Only the three library targets needed to support the five kept test
# executables are defined here. test_module (test.c) is not a kept test;
# rather than giving it its own named target, test.c is compiled directly
# into test_config — the only library that calls d_test_registry_* symbols.
###############################################################################

if(NOT TARGET test_assert)
    add_library(test_assert STATIC
        "${TEST_FRAMEWORK_SRC_DIR}/assert.c"
    )
    target_include_directories(test_assert PUBLIC ${INCLUDE_DIR} ${TEST_FRAMEWORK_INC_DIR})
    target_link_libraries(test_assert PUBLIC djinterp dmemory string_fn)
    target_compile_definitions(test_assert PRIVATE D_TESTING=1)
endif()

# test_config calls d_test_registry_init/registry/find (defined in test.c).
# test.c is compiled directly into this library — no separate test_module target needed.
if(NOT TARGET test_config)
    add_library(test_config STATIC
        "${TEST_FRAMEWORK_SRC_DIR}/test_config.c"
        "${TEST_FRAMEWORK_SRC_DIR}/test.c"
    )
    target_include_directories(test_config PUBLIC ${INCLUDE_DIR} ${TEST_FRAMEWORK_INC_DIR})
    target_link_libraries(test_config PUBLIC djinterp dmemory string_fn)
    target_compile_definitions(test_config PRIVATE D_TESTING=1)
endif()

if(NOT TARGET test_cvar)
    add_library(test_cvar STATIC
        "${TEST_FRAMEWORK_SRC_DIR}/test_cvar.c"
    )
    target_include_directories(test_cvar PUBLIC ${INCLUDE_DIR} ${TEST_FRAMEWORK_INC_DIR})
    target_link_libraries(test_cvar PUBLIC djinterp dmemory string_fn)
    target_compile_definitions(test_cvar PRIVATE D_TESTING=1)
endif()

###############################################################################
# HELPER MACRO
###############################################################################

macro(_test_fw_add_test MODULE_NAME CONFIG_NAME)
    set(_extra_args ${ARGN})
    set(_main "${CONFIG_TEST_DIR}/djinterp-c-test-${CONFIG_NAME}-tests-sa/main.c")
    if(EXISTS "${_main}")
        djinterp_add_standalone_test(
            MODULE_NAME ${MODULE_NAME}
            ${_extra_args}
            MAIN_FILE "${_main}"
        )
    else()
        djinterp_add_standalone_test(
            MODULE_NAME ${MODULE_NAME}
            ${_extra_args}
        )
    endif()
endmacro()

###############################################################################
# TEST EXECUTABLES — five kept modules only
###############################################################################

# assert
_test_fw_add_test(test_assert "assert"
    TEST_PATTERN "assert_tests_sa*.c"
    EXTRA_LIBS test_assert)

# test_common
# test_standalone.c and test_common.c are already compiled into
# test-standalone-support, so no extra library is needed here.
_test_fw_add_test(test_common "test-common")

# test_config
# test_config lib already embeds test.c, so all d_test_registry_* symbols
# are satisfied internally — no separate test_module argument needed.
_test_fw_add_test(test_config "test-config"
    EXTRA_LIBS test_config container)

# test_cvar
_test_fw_add_test(test_cvar "test-cvar"
    EXTRA_LIBS test_cvar test_config container)

# test_standalone
_test_fw_add_test(test_standalone "test-standalone")

###############################################################################
# SUMMARY
###############################################################################

message(STATUS "")
message(STATUS "Test Framework Build Summary:")
message(STATUS "  Test executables: assert, test-common, test-config,")
message(STATUS "                    test-cvar, test-standalone")
message(STATUS "")
