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
#   dio        — formatted I/O abstraction
#   dstring    — dynamic string type
#   dtime      — time / duration utilities
#   datomic    — atomic operations
#   dmutex     — mutex / threading primitives
#   dconfig    — configuration management
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
###############################################################################

if(NOT DEFINED DJINTERP_ROOT)
    get_filename_component(DJINTERP_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../../../.." ABSOLUTE)
endif()

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

if(DEFINED C_TEST_DIR)
    set(TEST_DIR "${C_TEST_DIR}/core")
else()
    set(TEST_DIR "${DJINTERP_ROOT}/tests/c/core")
endif()

set(C_TEST_BASE_DIR "${DJINTERP_ROOT}/tests/c")

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

if(DEFINED C_CONFIG_TEST_DIR)
    set(CONFIG_TEST_DIR "${C_CONFIG_TEST_DIR}/core")
else()
    set(CONFIG_TEST_DIR "${DJINTERP_ROOT}/.config/.msvs/testing/c/core")
endif()

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

include(${CMAKE_CURRENT_SOURCE_DIR}/../../common.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/../../tests_standalone.cmake)

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

if(NOT TARGET djinterp)
    add_library(djinterp STATIC "${SOURCE_DIR}/djinterp.c")
    target_include_directories(djinterp PUBLIC ${INCLUDE_DIR})
    target_compile_definitions(djinterp PRIVATE D_TESTING=1)
endif()

if(NOT TARGET env)
    add_library(env STATIC "${SOURCE_DIR}/env.c")
    target_include_directories(env PUBLIC ${INCLUDE_DIR})
    target_link_libraries(env PUBLIC djinterp)
    target_compile_definitions(env PRIVATE D_TESTING=1)
endif()

if(NOT TARGET dmacro)
    add_library(dmacro INTERFACE)
    target_include_directories(dmacro INTERFACE ${INCLUDE_DIR})
endif()

if(NOT TARGET dmemory)
    add_library(dmemory STATIC "${SOURCE_DIR}/dmemory.c")
    target_include_directories(dmemory PUBLIC ${INCLUDE_DIR})
    target_link_libraries(dmemory PUBLIC djinterp)
    target_compile_definitions(dmemory PRIVATE D_TESTING=1)
endif()

if(NOT TARGET string_fn)
    add_library(string_fn STATIC "${SOURCE_DIR}/string_fn.c")
    target_include_directories(string_fn PUBLIC ${INCLUDE_DIR})
    target_link_libraries(string_fn PUBLIC dmemory djinterp)
    target_compile_definitions(string_fn PRIVATE D_TESTING=1)
endif()

if(NOT TARGET dfile)
    add_library(dfile STATIC "${SOURCE_DIR}/dfile.c")
    target_include_directories(dfile PUBLIC ${INCLUDE_DIR})
    target_link_libraries(dfile PUBLIC djinterp dmemory string_fn)
    target_compile_definitions(dfile PRIVATE D_TESTING=1)
endif()

if(NOT TARGET dio)
    add_library(dio STATIC "${SOURCE_DIR}/dio.c")
    target_include_directories(dio PUBLIC ${INCLUDE_DIR})
    target_link_libraries(dio PUBLIC djinterp)
    target_compile_definitions(dio PRIVATE D_TESTING=1)
endif()

if(NOT TARGET dstring)
    add_library(dstring STATIC "${SOURCE_DIR}/dstring.c")
    target_include_directories(dstring PUBLIC ${INCLUDE_DIR})
    target_link_libraries(dstring PUBLIC djinterp dmemory)
    target_compile_definitions(dstring PRIVATE D_TESTING=1)
endif()

if(NOT TARGET dtime)
    add_library(dtime STATIC "${SOURCE_DIR}/dtime.c")
    target_include_directories(dtime PUBLIC ${INCLUDE_DIR})
    target_link_libraries(dtime PUBLIC djinterp dmemory)
    target_compile_definitions(dtime PRIVATE D_TESTING=1)
endif()

if(NOT TARGET datomic)
    add_library(datomic STATIC "${SOURCE_DIR}/datomic.c")
    target_include_directories(datomic PUBLIC ${INCLUDE_DIR})
    target_link_libraries(datomic PUBLIC djinterp)
    target_compile_definitions(datomic PRIVATE D_TESTING=1)
endif()

if(NOT TARGET dmutex)
    add_library(dmutex STATIC "${SOURCE_DIR}/dmutex.c")
    target_include_directories(dmutex PUBLIC ${INCLUDE_DIR})
    target_link_libraries(dmutex PUBLIC djinterp)
    target_compile_definitions(dmutex PRIVATE D_TESTING=1)
endif()

if(NOT TARGET dconfig)
    add_library(dconfig STATIC "${SOURCE_DIR}/dconfig.c")
    target_include_directories(dconfig PUBLIC ${INCLUDE_DIR})
    target_link_libraries(dconfig PUBLIC djinterp dmemory string_fn)
    target_compile_definitions(dconfig PRIVATE D_TESTING=1)
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
###############################################################################

# --- helper macro ---
macro(_core_add_test MODULE_NAME)
    set(_extra_args ${ARGN})
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

# dio tests
_core_add_test(dio        EXTRA_LIBS dio)

# dstring tests
_core_add_test(dstring    EXTRA_LIBS dstring)

# dtime tests
_core_add_test(dtime      EXTRA_LIBS dtime)

# datomic tests
_core_add_test(datomic    EXTRA_LIBS datomic)

# dmutex tests
_core_add_test(dmutex     EXTRA_LIBS dmutex)

###############################################################################
# COMBINED TEST EXECUTABLE: djinterp-c-tests-all
#
# FIX: The main.c at .config/.msvs/testing/c/core/djinterp-c-tests-all/main.c is a
# true cross-module combined test — it calls runners from ALL modules (core, container,
# functional, event, test framework).  The previous implementation only globbed test files
# from ${TEST_DIR} (tests/c/core/), so 30 runner symbols were unresolved at link time.
#
# The fix gathers test source files from every module's test directory, filters out
# *_helpers.c (which contain shared utilities with symbols already defined in the root
# *_tests_sa.c files), and links all required component libraries.
###############################################################################

set(CORE_ALL_MAIN "${CONFIG_TEST_DIR}/djinterp-c-tests-all/main.c")
if(EXISTS "${CORE_ALL_MAIN}")

    # -------------------------------------------------------------------------
    # Convenience variables for each module's test directory
    # -------------------------------------------------------------------------
    set(_TEST_CONTAINER_ARRAY_DIR   "${C_TEST_BASE_DIR}/container/array")
    set(_TEST_CONTAINER_MAP_DIR     "${C_TEST_BASE_DIR}/container/map")
    set(_TEST_CONTAINER_REGISTRY_DIR "${C_TEST_BASE_DIR}/container/registry")
    set(_TEST_CONTAINER_VECTOR_DIR  "${C_TEST_BASE_DIR}/container/vector")
    set(_TEST_FUNCTIONAL_DIR        "${C_TEST_BASE_DIR}/functional")
    set(_TEST_EVENT_DIR             "${C_TEST_BASE_DIR}/event")
    set(_TEST_TEST_DIR              "${C_TEST_BASE_DIR}/test")

    # -------------------------------------------------------------------------
    # Gather test source files from every module
    # -------------------------------------------------------------------------

    # Core tests (already in TEST_DIR)
    file(GLOB _TESTS_CORE "${TEST_DIR}/*_tests_sa*.c")

    # Container: array
    file(GLOB _TESTS_ARRAY    "${_TEST_CONTAINER_ARRAY_DIR}/*_tests_sa*.c")
    # Container: map
    file(GLOB _TESTS_MAP      "${_TEST_CONTAINER_MAP_DIR}/*_tests_sa*.c")
    # Container: registry
    file(GLOB _TESTS_REGISTRY "${_TEST_CONTAINER_REGISTRY_DIR}/*_tests_sa*.c")
    # Container: vector
    file(GLOB _TESTS_VECTOR   "${_TEST_CONTAINER_VECTOR_DIR}/*_tests_sa*.c")

    # Functional
    file(GLOB _TESTS_FUNCTIONAL "${_TEST_FUNCTIONAL_DIR}/*_tests_sa*.c")

    # Event
    file(GLOB _TESTS_EVENT    "${_TEST_EVENT_DIR}/*_tests_sa*.c")

    # Test framework (test_common and test_standalone runners)
    file(GLOB _TESTS_TEST     "${_TEST_TEST_DIR}/*_tests_sa*.c")

    # Combine all raw lists
    set(_ALL_RAW
        ${_TESTS_CORE}
        ${_TESTS_ARRAY}
        ${_TESTS_MAP}
        ${_TESTS_REGISTRY}
        ${_TESTS_VECTOR}
        ${_TESTS_FUNCTIONAL}
        ${_TESTS_EVENT}
        ${_TESTS_TEST}
    )

    # -------------------------------------------------------------------------
    # Filter: exclude _main.c and *_helpers.c
    #   - _main.c:    avoids duplicate main() definitions
    #   - _helpers.c: contain shared utilities (e.g. d_test_int_comparator) that
    #                 are already defined by the corresponding root *_tests_sa.c,
    #                 causing LNK2005 multiply-defined-symbol errors if included
    # -------------------------------------------------------------------------
    set(CORE_ALL_FILTERED "")
    foreach(FILE ${_ALL_RAW})
        get_filename_component(FILENAME "${FILE}" NAME)
        if(NOT FILENAME MATCHES "_main\\.c$" AND NOT FILENAME MATCHES "_helpers\\.c$")
            list(APPEND CORE_ALL_FILTERED "${FILE}")
        endif()
    endforeach()

    set(CORE_ALL_TARGET "djinterp-c-tests-all-${PLATFORM_ARCH}")

    add_executable(${CORE_ALL_TARGET}
        "${CORE_ALL_MAIN}"
        ${CORE_ALL_FILTERED}
    )

    target_include_directories(${CORE_ALL_TARGET} PRIVATE
        ${INCLUDE_DIR}
        ${TEST_FRAMEWORK_INC_DIR}
        ${C_TEST_BASE_DIR}
        ${TEST_DIR}
        ${_TEST_CONTAINER_ARRAY_DIR}
        ${_TEST_CONTAINER_MAP_DIR}
        ${_TEST_CONTAINER_REGISTRY_DIR}
        ${_TEST_CONTAINER_VECTOR_DIR}
        ${_TEST_FUNCTIONAL_DIR}
        ${_TEST_EVENT_DIR}
        ${_TEST_TEST_DIR}
    )

    target_compile_definitions(${CORE_ALL_TARGET} PRIVATE D_TESTING=1)

    if(MSVC)
        target_compile_options(${CORE_ALL_TARGET} PRIVATE /FS)
        set_target_properties(${CORE_ALL_TARGET} PROPERTIES
            COMPILE_PDB_NAME "${CORE_ALL_TARGET}"
            COMPILE_PDB_OUTPUT_DIRECTORY
                "${CMAKE_CURRENT_BINARY_DIR}/${CORE_ALL_TARGET}.dir"
        )
    endif()

    # Link all modules needed by the cross-module main.c:
    #   core:       env, dtime, dstring, datomic, dmutex, dconfig, dio (dmemory/djinterp/string_fn
    #               already pulled in via test-standalone-support)
    #   container:  container (array, map, registry, ptr_vector, vector, circular_array)
    #   functional: filter, predicate, compose, fn_builder, pipeline, functional, functional_common
    #   event:      event_handler, event_table, event_table_common, event
    #   test fw:    test_module (provides test_common + test_standalone runners)
    target_link_libraries(${CORE_ALL_TARGET} PRIVATE
        test-standalone-support
        # core
        env dtime dstring datomic dmutex dconfig dio
        # container (includes circular_array, ptr_vector, etc.)
        container
        # functional
        filter predicate compose fn_builder pipeline functional functional_common
        # event (event_handler → container via circular_array)
        event_handler event_table event_table_common event
        # test framework
        test_module
    )

    message(STATUS "  Created test executable: ${CORE_ALL_TARGET}")
else()
    message(STATUS "  Skipping tests-all (no main.c at ${CORE_ALL_MAIN})")
endif()

###############################################################################
# SUMMARY
###############################################################################

message(STATUS "")
message(STATUS "  Core Build Summary:")
message(STATUS "    Libraries:        djinterp, env, dmacro, dmemory, string_fn,")
message(STATUS "                      dfile, dio, dstring, dtime, datomic,")
message(STATUS "                      dmutex, dconfig")
message(STATUS "    Test executables:  11 individual + 1 combined")
message(STATUS "    Test framework:    Standalone (library-based)")
message(STATUS "")
