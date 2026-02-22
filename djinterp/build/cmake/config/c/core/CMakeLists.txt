###############################################################################
# djinterp - C / core component
# 
# Build configuration for the core C modules and their standalone tests.
#
# Modules built here:
#   djinterp   — core types and index utilities
#   env        — environment / platform abstraction
#   dmacro     — preprocessor macro library (header-only)
#   dmemory    — memory management wrappers
#   string_fn  — string utility functions
#   dfile      — file I/O abstraction
#   dstring    — dynamic string type
#   dtime      — time / duration utilities
#
# Location: <root>/build/cmake/config/c/core/CMakeLists.txt
#
# author(s): Samuel 'teer' Neal-Blim                          date: 2023.03.01
###############################################################################
cmake_minimum_required(VERSION 3.20)

# Project name
project(djinterp-c-core VERSION 0.1 LANGUAGES C CXX)

###############################################################################
# DIRECTORY SETUP
#
# Core source/header files live flat under src/c/ and inc/c/.
# Tests live under tests/c/core/.
#
# Parent scope (c/CMakeLists.txt) provides:
#   C_SOURCE_DIR, C_INCLUDE_DIR, C_TEST_DIR,
#   C_TEST_FRAMEWORK_SRC_DIR, C_TEST_FRAMEWORK_INC_DIR,
#   C_CONFIG_TEST_DIR, DJINTERP_ROOT, PLATFORM_ARCH
#
# We set local aliases (SOURCE_DIR, INCLUDE_DIR, TEST_DIR, etc.) so that
# the shared cmake utilities (tests_standalone.cmake, tests_common.cmake)
# work unchanged.
###############################################################################

# Determine the project root if not inherited (standalone invocation)
# This file is at: <root>/build/cmake/config/c/core/CMakeLists.txt
#   core -> c -> config -> cmake -> build -> <root>  (5 levels)
if(NOT DEFINED DJINTERP_ROOT)
    get_filename_component(DJINTERP_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../../../.." ABSOLUTE)
endif()

# Source and header directories (flat under src/c/, inc/c/)
if(DEFINED C_SOURCE_DIR)
    set(SOURCE_DIR "${C_SOURCE_DIR}")
else()
    set(SOURCE_DIR "${DJINTERP_ROOT}/src/c")
endif()

if(DEFINED C_INCLUDE_DIR)
    set(INCLUDE_DIR "${C_INCLUDE_DIR}")
else()
    set(INCLUDE_DIR "${DJINTERP_ROOT}/inc/c")
endif()

# Test directories — core tests live under tests/c/core/
if(DEFINED C_TEST_DIR)
    set(TEST_DIR "${C_TEST_DIR}/core")
else()
    set(TEST_DIR "${DJINTERP_ROOT}/tests/c/core")
endif()

# Also expose the parent test dir for shared test headers
set(C_TEST_BASE_DIR "${DJINTERP_ROOT}/tests/c")

# Test framework paths
if(DEFINED C_TEST_FRAMEWORK_SRC_DIR)
    set(TEST_FRAMEWORK_SRC_DIR "${C_TEST_FRAMEWORK_SRC_DIR}")
else()
    set(TEST_FRAMEWORK_SRC_DIR "${DJINTERP_ROOT}/src/c/test")
endif()

if(DEFINED C_TEST_FRAMEWORK_INC_DIR)
    set(TEST_FRAMEWORK_INC_DIR "${C_TEST_FRAMEWORK_INC_DIR}")
else()
    set(TEST_FRAMEWORK_INC_DIR "${DJINTERP_ROOT}/inc/c/test")
endif()

# .config directory for auto-generated main.c files
if(DEFINED C_CONFIG_TEST_DIR)
    set(CONFIG_TEST_DIR "${C_CONFIG_TEST_DIR}/core")
else()
    set(CONFIG_TEST_DIR "${DJINTERP_ROOT}/.config/.msvs/testing/c/core")
endif()

# Detect platform if not inherited
if(NOT DEFINED PLATFORM_ARCH)
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(PLATFORM_ARCH "x64")
    else()
        set(PLATFORM_ARCH "x86")
    endif()
endif()

# Set output directories
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${DJINTERP_ROOT}/lib/c/core")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${DJINTERP_ROOT}/lib/c/core")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${DJINTERP_ROOT}/bin/tests/c/core")

foreach(CONFIG ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER ${CONFIG} CONFIG_UPPER)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${CONFIG_UPPER} "${DJINTERP_ROOT}/bin/tests/c/core")
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${CONFIG_UPPER} "${DJINTERP_ROOT}/lib/c/core")
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${CONFIG_UPPER} "${DJINTERP_ROOT}/lib/c/core")
endforeach()

# Include shared cmake utilities
# From config/c/core/ we go up 2 levels to reach config/
#   core -> c -> config
include(${CMAKE_CURRENT_SOURCE_DIR}/../../common.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/../../tests_standalone.cmake)

# Print configuration
message(STATUS "  ---- Core Component ----")
message(STATUS "  Source dir:        ${SOURCE_DIR}")
message(STATUS "  Include dir:       ${INCLUDE_DIR}")
message(STATUS "  Test dir:          ${TEST_DIR}")
message(STATUS "  Test FW src:       ${TEST_FRAMEWORK_SRC_DIR}")
message(STATUS "  Test FW inc:       ${TEST_FRAMEWORK_INC_DIR}")
message(STATUS "  Config test dir:   ${CONFIG_TEST_DIR}")
message(STATUS "  Bin dir:           ${DJINTERP_ROOT}/bin/tests/c/core")
message(STATUS "  Lib dir:           ${DJINTERP_ROOT}/lib/c/core")

###############################################################################
# CORE LIBRARIES
###############################################################################

# djinterp module (core types, index utilities)
# NOTE: target name "djinterp" is referenced by tests_common.cmake dependency
# resolution and the test-standalone-support library exclusion list. Do not
# rename without updating those files.
if(NOT TARGET djinterp)
    add_library(djinterp STATIC "${SOURCE_DIR}/djinterp.c")
    target_include_directories(djinterp PUBLIC ${INCLUDE_DIR})
endif()

# env module (environment / platform abstraction)
if(NOT TARGET env)
    add_library(env STATIC "${SOURCE_DIR}/env.c")
    target_include_directories(env PUBLIC ${INCLUDE_DIR})
    target_link_libraries(env PUBLIC djinterp)
endif()

# dmacro module (header-only preprocessor utilities)
if(NOT TARGET dmacro)
    add_library(dmacro INTERFACE)
    target_include_directories(dmacro INTERFACE ${INCLUDE_DIR})
endif()

# dmemory module (memory management wrappers)
if(NOT TARGET dmemory)
    add_library(dmemory STATIC "${SOURCE_DIR}/dmemory.c")
    target_include_directories(dmemory PUBLIC ${INCLUDE_DIR})
    target_link_libraries(dmemory PUBLIC djinterp)
endif()

# string_fn module (string utility functions)
if(NOT TARGET string_fn)
    add_library(string_fn STATIC "${SOURCE_DIR}/string_fn.c")
    target_include_directories(string_fn PUBLIC ${INCLUDE_DIR})
    target_link_libraries(string_fn PUBLIC dmemory djinterp)
endif()

# dfile module (file I/O abstraction)
if(NOT TARGET dfile)
    add_library(dfile STATIC "${SOURCE_DIR}/dfile.c")
    target_include_directories(dfile PUBLIC ${INCLUDE_DIR})
    target_link_libraries(dfile PUBLIC djinterp dmemory string_fn)
endif()

# dstring module (dynamic string type)
if(NOT TARGET dstring)
    add_library(dstring STATIC "${SOURCE_DIR}/dstring.c")
    target_include_directories(dstring PUBLIC ${INCLUDE_DIR})
    target_link_libraries(dstring PUBLIC djinterp dmemory)
endif()

# dtime module (time / duration utilities)
if(NOT TARGET dtime)
    add_library(dtime STATIC "${SOURCE_DIR}/dtime.c")
    target_include_directories(dtime PUBLIC ${INCLUDE_DIR})
    target_link_libraries(dtime PUBLIC djinterp dmemory)
endif()

###############################################################################
# COMPILER FLAGS
###############################################################################

if(MSVC)
    add_compile_options(/FS)
endif()

###############################################################################
# TEST SUPPORT LIBRARY
#
# Compiled once and linked to all test executables for efficiency.
# Includes dfile since test_standalone uses d_fopen.
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
# TEST EXECUTABLES
#
# Each core module gets a standalone test executable.
# Naming convention: djinterp-c-<module>-tests-sa-<arch>
###############################################################################

# --- helper macro to reduce repetition ---
macro(_core_add_test MODULE_NAME)
    set(_extra_args ${ARGN})

    # Convert underscores to hyphens for the .config directory name
    djinterp_module_name_to_path("${MODULE_NAME}" _path_name)
    set(_main "${CONFIG_TEST_DIR}/djinterp-c-${_path_name}-tests-sa/main.c")

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

# djinterp core tests (special .config name: "header-tests")
set(_djinterp_main "${CONFIG_TEST_DIR}/djinterp-c-header-tests-sa/main.c")
if(EXISTS "${_djinterp_main}")
    djinterp_add_standalone_test(MODULE_NAME djinterp MAIN_FILE "${_djinterp_main}")
else()
    _core_add_test(djinterp)
endif()

# env tests
_core_add_test(env        EXTRA_LIBS env)

# dmacro tests
_core_add_test(dmacro     EXTRA_LIBS dmacro)

# dmemory tests
_core_add_test(dmemory    EXTRA_LIBS dmemory)

# string_fn tests
_core_add_test(string_fn  EXTRA_LIBS string_fn)

# dfile tests
_core_add_test(dfile      EXTRA_LIBS dfile)

# dstring tests
_core_add_test(dstring    EXTRA_LIBS dstring)

# dtime tests
_core_add_test(dtime      EXTRA_LIBS dtime)

###############################################################################
# SUMMARY
###############################################################################

message(STATUS "")
message(STATUS "  Core Build Summary:")
message(STATUS "    Libraries:        djinterp, env, dmacro, dmemory,")
message(STATUS "                      string_fn, dfile, dstring, dtime")
message(STATUS "    Test executables:  8")
message(STATUS "    Test framework:    Standalone (library-based)")
message(STATUS "")