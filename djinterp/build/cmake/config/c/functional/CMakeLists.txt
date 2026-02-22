###############################################################################
# djinterp - C / functional component
# 
# Build configuration for functional programming modules and standalone tests.
#
# Modules built here:
#   functional        — base functional types and dispatch
#   functional_common — shared helpers (identity, fold, quantifiers, etc.)
#   compose           — function composition and transformers
#   filter            — filter builders, combinators, and iterators
#   fn_builder        — generic function builder / closure construction
#   pipeline          — staged pipeline creation and execution
#   predicate         — predicate constructors, evaluators, and macros
#
# Location: <root>/build/cmake/config/c/functional/CMakeLists.txt
#
# author(s): Samuel 'teer' Neal-Blim                          date: 2026.02.21
###############################################################################
cmake_minimum_required(VERSION 3.20)

# Project name
project(djinterp-c-functional VERSION 0.1 LANGUAGES C CXX)

###############################################################################
# DIRECTORY SETUP
#
# Functional source/header files live under src/c/functional/ and
# inc/c/functional/.  Tests live under tests/c/functional/.
#
# Parent scope (c/CMakeLists.txt) provides:
#   C_SOURCE_DIR, C_INCLUDE_DIR, C_TEST_DIR,
#   C_TEST_FRAMEWORK_SRC_DIR, C_TEST_FRAMEWORK_INC_DIR,
#   C_CONFIG_TEST_DIR, DJINTERP_ROOT, PLATFORM_ARCH
###############################################################################

# Determine the project root if not inherited (standalone invocation)
# This file is at: <root>/build/cmake/config/c/functional/CMakeLists.txt
#   functional -> c -> config -> cmake -> build -> <root>  (5 levels)
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
# SOURCE_DIR points at the functional subdirectory (where .c files live)
# INCLUDE_DIR points at the C include root (headers are included as
#   "functional/compose.h" etc.)
set(SOURCE_DIR              "${C_SOURCE_DIR}/functional")
set(INCLUDE_DIR             "${C_INCLUDE_DIR}")
set(TEST_DIR                "${C_TEST_DIR}/functional")
set(TEST_FRAMEWORK_SRC_DIR  "${C_TEST_FRAMEWORK_SRC_DIR}")
set(TEST_FRAMEWORK_INC_DIR  "${C_TEST_FRAMEWORK_INC_DIR}")
set(CONFIG_TEST_DIR         "${C_CONFIG_TEST_DIR}/functional")

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
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${DJINTERP_ROOT}/lib/c/functional")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${DJINTERP_ROOT}/lib/c/functional")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${DJINTERP_ROOT}/bin/tests/c/functional")

foreach(CONFIG ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER ${CONFIG} CONFIG_UPPER)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${CONFIG_UPPER} "${DJINTERP_ROOT}/bin/tests/c/functional")
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${CONFIG_UPPER} "${DJINTERP_ROOT}/lib/c/functional")
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${CONFIG_UPPER} "${DJINTERP_ROOT}/lib/c/functional")
endforeach()

# Include shared cmake utilities
# From config/c/functional/ we go up 2 levels to reach config/
#   functional -> c -> config
include(${CMAKE_CURRENT_SOURCE_DIR}/../../common.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/../../tests_standalone.cmake)

# Print configuration
message(STATUS "  ---- Functional Component ----")
message(STATUS "  Source dir:        ${SOURCE_DIR}")
message(STATUS "  Include dir:       ${INCLUDE_DIR}")
message(STATUS "  Test dir:          ${TEST_DIR}")
message(STATUS "  Config test dir:   ${CONFIG_TEST_DIR}")
message(STATUS "  Bin dir:           ${DJINTERP_ROOT}/bin/tests/c/functional")
message(STATUS "  Lib dir:           ${DJINTERP_ROOT}/lib/c/functional")

###############################################################################
# FUNCTIONAL LIBRARIES
###############################################################################

# functional module (base types and dispatch)
if(NOT TARGET functional)
    add_library(functional STATIC "${SOURCE_DIR}/functional.c")
    target_include_directories(functional PUBLIC ${C_INCLUDE_DIR})
    target_link_libraries(functional PUBLIC djinterp dmemory)
endif()

# functional_common module (shared helpers — identity, fold, quantifiers, etc.)
if(NOT TARGET functional_common)
    add_library(functional_common STATIC "${SOURCE_DIR}/functional_common.c")
    target_include_directories(functional_common PUBLIC ${C_INCLUDE_DIR})
    target_link_libraries(functional_common PUBLIC functional djinterp dmemory)
endif()

# compose module (function composition and transformers)
if(NOT TARGET compose)
    add_library(compose STATIC "${SOURCE_DIR}/compose.c")
    target_include_directories(compose PUBLIC ${C_INCLUDE_DIR})
    target_link_libraries(compose PUBLIC functional functional_common djinterp dmemory)
endif()

# filter module (filter builders, combinators, iterators)
if(NOT TARGET filter)
    add_library(filter STATIC "${SOURCE_DIR}/filter.c")
    target_include_directories(filter PUBLIC ${C_INCLUDE_DIR})
    target_link_libraries(filter PUBLIC functional functional_common djinterp dmemory)
endif()

# fn_builder module (generic function builder / closure construction)
if(NOT TARGET fn_builder)
    add_library(fn_builder STATIC "${SOURCE_DIR}/fn_builder.c")
    target_include_directories(fn_builder PUBLIC ${C_INCLUDE_DIR})
    target_link_libraries(fn_builder PUBLIC functional functional_common djinterp dmemory)
endif()

# pipeline module (staged pipeline creation and execution)
if(NOT TARGET pipeline)
    add_library(pipeline STATIC "${SOURCE_DIR}/pipeline.c")
    target_include_directories(pipeline PUBLIC ${C_INCLUDE_DIR})
    target_link_libraries(pipeline PUBLIC functional functional_common compose djinterp dmemory)
endif()

# predicate module (predicate constructors, evaluators, macros)
if(NOT TARGET predicate)
    add_library(predicate STATIC "${SOURCE_DIR}/predicate.c")
    target_include_directories(predicate PUBLIC ${C_INCLUDE_DIR})
    target_link_libraries(predicate PUBLIC functional functional_common djinterp dmemory)
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
# If test-standalone-support was already created by core, reuse it.
# Otherwise create it here (for standalone invocation).
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
# TEST EXECUTABLES
#
# Each functional module gets a standalone test executable.
# Naming convention: djinterp-c-<module>-tests-sa-<arch>
###############################################################################

# --- helper macro to reduce repetition ---
macro(_functional_add_test MODULE_NAME)
    set(_extra_args ${ARGN})

    # Convert underscores to hyphens for the .config directory name
    djinterp_module_name_to_path("${MODULE_NAME}" _path_name)
    set(_main "${CONFIG_TEST_DIR}/djinterp-c-functional-${_path_name}-tests-sa/main.c")

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

# compose tests
_functional_add_test(compose
    EXTRA_LIBS compose functional functional_common)

# filter tests
_functional_add_test(filter
    EXTRA_LIBS filter functional functional_common)

# fn_builder tests
_functional_add_test(fn_builder
    EXTRA_LIBS fn_builder functional functional_common)

# functional_common tests
_functional_add_test(functional_common
    EXTRA_LIBS functional_common functional)

# pipeline tests
_functional_add_test(pipeline
    EXTRA_LIBS pipeline compose functional functional_common)

# predicate tests
_functional_add_test(predicate
    EXTRA_LIBS predicate functional functional_common)

###############################################################################
# SUMMARY
###############################################################################

message(STATUS "")
message(STATUS "  Functional Build Summary:")
message(STATUS "    Libraries:        functional, functional_common, compose,")
message(STATUS "                      filter, fn_builder, pipeline, predicate")
message(STATUS "    Test executables:  6")
message(STATUS "    Test framework:    Standalone (library-based)")
message(STATUS "")