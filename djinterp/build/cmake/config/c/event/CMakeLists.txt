###############################################################################
# djinterp - C / event component
# 
# Build configuration for event system modules and standalone tests.
#
# Modules built here:
#   event              — core event types and operations
#   event_handler      — event handler / listener management
#   event_table        — hash-based event dispatch table
#   event_table_common — shared event table helpers (hashing, primes, nodes)
#
# Location: <root>/build/cmake/config/c/event/CMakeLists.txt
#
# author(s): Samuel 'teer' Neal-Blim                          date: 2026.02.25
###############################################################################
cmake_minimum_required(VERSION 3.20)

# Project name
project(djinterp-c-event VERSION 0.1 LANGUAGES C CXX)

###############################################################################
# DIRECTORY SETUP
###############################################################################

if(NOT DEFINED DJINTERP_ROOT)
    get_filename_component(DJINTERP_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../../../.." ABSOLUTE)
endif()

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

set(SOURCE_DIR              "${C_SOURCE_DIR}/event")
set(INCLUDE_DIR             "${C_INCLUDE_DIR}")
set(TEST_DIR                "${C_TEST_DIR}/event")
set(TEST_FRAMEWORK_SRC_DIR  "${C_TEST_FRAMEWORK_SRC_DIR}")
set(TEST_FRAMEWORK_INC_DIR  "${C_TEST_FRAMEWORK_INC_DIR}")
set(CONFIG_TEST_DIR         "${C_CONFIG_TEST_DIR}/event")

set(C_TEST_BASE_DIR "${DJINTERP_ROOT}/tests/c")

if(NOT DEFINED PLATFORM_ARCH)
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(PLATFORM_ARCH "x64")
    else()
        set(PLATFORM_ARCH "x86")
    endif()
endif()

# Set output directories
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${DJINTERP_ROOT}/lib/c/event")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${DJINTERP_ROOT}/lib/c/event")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${DJINTERP_ROOT}/bin/tests/c/event")

foreach(CONFIG ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER ${CONFIG} CONFIG_UPPER)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${CONFIG_UPPER} "${DJINTERP_ROOT}/bin/tests/c/event")
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${CONFIG_UPPER} "${DJINTERP_ROOT}/lib/c/event")
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${CONFIG_UPPER} "${DJINTERP_ROOT}/lib/c/event")
endforeach()

include(${CMAKE_CURRENT_SOURCE_DIR}/../../common.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/../../tests_standalone.cmake)

message(STATUS "  ---- Event Component ----")
message(STATUS "  Source dir:        ${SOURCE_DIR}")
message(STATUS "  Include dir:       ${INCLUDE_DIR}")
message(STATUS "  Test dir:          ${TEST_DIR}")
message(STATUS "  Config test dir:   ${CONFIG_TEST_DIR}")
message(STATUS "  Bin dir:           ${DJINTERP_ROOT}/bin/tests/c/event")
message(STATUS "  Lib dir:           ${DJINTERP_ROOT}/lib/c/event")

###############################################################################
# EVENT LIBRARIES
###############################################################################

# event_table_common (shared helpers — hashing, primes, node management)
if(NOT TARGET event_table_common)
    add_library(event_table_common STATIC "${SOURCE_DIR}/event_table_common.c")
    target_include_directories(event_table_common PUBLIC ${C_INCLUDE_DIR})
    target_link_libraries(event_table_common PUBLIC djinterp dmemory string_fn)
    target_compile_definitions(event_table_common PRIVATE D_TESTING=1)
endif()

# event module (core event types)
if(NOT TARGET event)
    add_library(event STATIC "${SOURCE_DIR}/event.c")
    target_include_directories(event PUBLIC ${C_INCLUDE_DIR})
    target_link_libraries(event PUBLIC djinterp dmemory string_fn)
    target_compile_definitions(event PRIVATE D_TESTING=1)
endif()

# event_table (hash-based event dispatch)
if(NOT TARGET event_table)
    add_library(event_table STATIC "${SOURCE_DIR}/event_table.c")
    target_include_directories(event_table PUBLIC ${C_INCLUDE_DIR})
    target_link_libraries(event_table PUBLIC event_table_common event djinterp dmemory)
    target_compile_definitions(event_table PRIVATE D_TESTING=1)
endif()

# event_handler (listener management)
# FIX: Added 'container' to link libraries — event_handler.c calls d_circular_array_new/push/pop/
#      is_empty/free (from container/array/circular_array.c), causing 5 LNK2019 errors in both
#      djinterp-c-event-handler-tests-sa and djinterp-c-event-tests-sa-all.
if(NOT TARGET event_handler)
    add_library(event_handler STATIC "${SOURCE_DIR}/event_handler.c")
    target_include_directories(event_handler PUBLIC ${C_INCLUDE_DIR})
    target_link_libraries(event_handler PUBLIC event_table event djinterp dmemory container)
    target_compile_definitions(event_handler PRIVATE D_TESTING=1)
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
# TEST EXECUTABLES
###############################################################################

# --- helper macro ---
macro(_event_add_test MODULE_NAME)
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

# event tests
_event_add_test(event
    EXTRA_LIBS event event_table_common)

# event_handler tests
# FIX: Added 'container' to EXTRA_LIBS so the test exe can resolve circular_array symbols
#      that event_handler.lib pulls in transitively.
_event_add_test(event_handler
    EXTRA_LIBS event_handler event_table event_table_common event container)

# event_table tests
_event_add_test(event_table
    EXTRA_LIBS event_table event_table_common event)

# event_table_common tests
_event_add_test(event_table_common
    EXTRA_LIBS event_table_common event)

###############################################################################
# COMBINED TEST EXECUTABLE: event-tests-sa-all
###############################################################################

set(EVENT_ALL_MAIN "${CONFIG_TEST_DIR}/djinterp-c-event-tests-sa-all/main.c")
if(EXISTS "${EVENT_ALL_MAIN}")
    file(GLOB EVENT_ALL_TEST_FILES "${TEST_DIR}/*_tests_sa*.c")

    set(EVENT_ALL_FILTERED "")
    foreach(FILE ${EVENT_ALL_TEST_FILES})
        get_filename_component(FILENAME "${FILE}" NAME)
        if(NOT FILENAME MATCHES "_main\\.c$")
            list(APPEND EVENT_ALL_FILTERED "${FILE}")
        endif()
    endforeach()

    set(EVENT_ALL_TARGET "djinterp-c-event-tests-sa-all-${PLATFORM_ARCH}")

    add_executable(${EVENT_ALL_TARGET}
        "${EVENT_ALL_MAIN}"
        ${EVENT_ALL_FILTERED}
    )

    target_include_directories(${EVENT_ALL_TARGET} PRIVATE
        ${INCLUDE_DIR}
        ${TEST_FRAMEWORK_INC_DIR}
        ${C_TEST_BASE_DIR}
        ${TEST_DIR}
    )

    target_compile_definitions(${EVENT_ALL_TARGET} PRIVATE D_TESTING=1)

    if(MSVC)
        target_compile_options(${EVENT_ALL_TARGET} PRIVATE /FS)
        set_target_properties(${EVENT_ALL_TARGET} PROPERTIES
            COMPILE_PDB_NAME "${EVENT_ALL_TARGET}"
            COMPILE_PDB_OUTPUT_DIRECTORY
                "${CMAKE_CURRENT_BINARY_DIR}/${EVENT_ALL_TARGET}.dir"
        )
    endif()

    # FIX: Added 'container' — event_handler depends on circular_array from container lib
    target_link_libraries(${EVENT_ALL_TARGET} PRIVATE
        test-standalone-support
        event_handler event_table event_table_common event
        container
    )

    message(STATUS "  Created test executable: ${EVENT_ALL_TARGET}")
else()
    message(STATUS "  Skipping event-tests-sa-all (no main.c at ${EVENT_ALL_MAIN})")
endif()

###############################################################################
# SUMMARY
###############################################################################

message(STATUS "")
message(STATUS "  Event Build Summary:")
message(STATUS "    Libraries:        event, event_handler, event_table,")
message(STATUS "                      event_table_common")
message(STATUS "    Test executables:  4 individual + 1 combined")
message(STATUS "    Test framework:    Standalone (library-based)")
message(STATUS "")
