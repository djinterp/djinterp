#*******************************************************************************
# djinterp [parse]                                                CMakeLists.txt
#
# Test leaf for parse's log unit (c/modules/parse/parse_log.h): its two
# tables of codes, and the subscriber that writes parse's diagnostics as the
# log's records.
#   Builds the suite in DTest 2.0's C face: the runner in this directory, the
# section translation units of tests/djinterp/c/modules/parse/log/
# (parse_log_tests*.c), parse_log.c, expression core v2 and the parse units
# it stands on, parse's settings, and the log with what it stands on (desc,
# the option environment, the streams, the files, the formatter and the
# template writer, a memory source and the mutex), in testing mode, once per C level from the C floor (C99) up,
# and runs each as a test. The runner returns 0 only when every test passed
# or was skipped. A level the compiler cannot select is left out, and the
# configure step says so.
#   Plain CMake, as parse's other leaves are, so that the leaf works both
# ways as it stands:
#     cmake -S <this directory> -B <out> && cmake --build <out> && \
#         ctest --test-dir <out>
#     add_subdirectory(<this directory> <binary directory>)
#
#   DTest's C face is the framework's library, djinterp_c_test, where the
# enclosing project defines it; standing alone, the leaf compiles its one
# source, test_suite.c, at each level itself.
#
#   cache:  DJINTERP_PARSE_LOG_C_LEVELS  C levels to build the suite at
#           DJINTERP_PARSE_LOG_STRICT    the ladder's diagnostics, as errors,
#                                        where the compiler has them
#
#
# path:      /build/cmake/config/testing/djinterp/c/modules/parse/log/CMakeLists.txt
# link(s):   TBA
# author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.23
#                                                            revised: 2026.11.23
#*******************************************************************************
cmake_minimum_required(VERSION 3.16)

# standing alone, this leaf is the project; added from another, it is not
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(djinterp_parse_log_tests LANGUAGES C)
    enable_testing()
endif()

# a second add_subdirectory() of this leaf would define every target twice
get_property(_seen GLOBAL PROPERTY DJINTERP_PARSE_LOG_LEAF)
if(_seen)
    return()
endif()
set_property(GLOBAL PROPERTY DJINTERP_PARSE_LOG_LEAF ON)

get_property(_languages GLOBAL PROPERTY ENABLED_LANGUAGES)
if(NOT "C" IN_LIST _languages)
    message(FATAL_ERROR
            "the parse log leaf needs C: the enclosing project() must enable "
            "it")
endif()

# the repository root, nine levels above this leaf
get_filename_component(_root
                       "${CMAKE_CURRENT_LIST_DIR}/../../../../../../../../.."
                       ABSOLUTE)

# whether the compiler builds a level as itself, asked of every level
# below; the enclosing project has it already
include("${_root}/build/cmake/djinterp/standards.cmake")

set(DJINTERP_PARSE_LOG_C_LEVELS "99;11;17;23" CACHE STRING
    "C levels the log suite is built and run at")
option(DJINTERP_PARSE_LOG_STRICT
       "Build with the ladder's diagnostics as errors, where there are any"
       ON)

find_package(Threads REQUIRED)

set(_c      "${_root}/src/djinterp/c")
set(_parse  "${_c}/modules/parse")
set(_tests  "${_root}/tests/djinterp/c/modules/parse/log")
file(GLOB _core CONFIGURE_DEPENDS "${_parse}/expr/*.c")
file(GLOB _sections CONFIGURE_DEPENDS "${_tests}/parse_log_tests*.c")
# DTest's C face: the framework's library, linked below, where the enclosing
# project defines it; standing alone, its one source
if(TARGET djinterp_c_test)
    set(_dtest "")
else()
    set(_dtest "${_c}/modules/test/test_suite.c")
endif()

set(_sources
    "${CMAKE_CURRENT_LIST_DIR}/parse_log_tests_runner.c"
    ${_sections}
    "${_parse}/parse_log.c"
    "${_parse}/settings.c"
    ${_core}
    "${_parse}/diagnostic.c"
    "${_parse}/storage.c"
    "${_c}/log/log.c"
    "${_c}/log/log_writers.c"
    "${_c}/desc/desc_machines.c"
    "${_c}/desc/desc_units.c"
    "${_c}/desc/desc_options.c"
    "${_c}/option/option_env.c"
    "${_c}/option/option_env_registry.c"
    "${_c}/option/option_common.c"
    "${_c}/option/option_set.c"
    "${_c}/option/option_set_common.c"
    "${_c}/util/ostream/ostream.c"
    "${_c}/util/ostream/ostream_console.c"
    "${_c}/util/ostream/ostream_file.c"
    "${_c}/filesys/file_open.c"
    "${_c}/filesys/file_common.c"
    "${_c}/filesys/file_desc.c"
    "${_c}/filesys/file_io.c"
    "${_c}/filesys/file_seek.c"
    "${_c}/filesys/file_stat.c"
    "${_c}/text/ostream_format.c"
    "${_c}/memory/mem_source.c"
    "${_c}/memory/mem_common.c"
    "${_c}/text/ostream_template.c"
    "${_c}/sync/dmutex.c"
    ${_dtest})

foreach(_level IN LISTS DJINTERP_PARSE_LOG_C_LEVELS)
    # a level the compiler cannot build as itself is left out
    djinterp_standard_selectable(C ${_level} _selectable _why)
    if(NOT _selectable)
        message(STATUS "djinterp parse log: C${_level} left out - ${_why}")
        continue()
    endif()

    set(_target djinterp_parse_log_tests_c${_level})
    add_executable(${_target} ${_sources})
    set_target_properties(${_target} PROPERTIES
        C_STANDARD          ${_level}
        C_STANDARD_REQUIRED ON
        C_EXTENSIONS        OFF)
    target_include_directories(${_target} PRIVATE "${_root}/inc" "${_tests}")
    target_compile_definitions(${_target} PRIVATE
        D_TESTING=1
        RE_STD_CFG_TESTING=1)

    # the sources are UTF-8 without a byte-order mark, which cl reads in the
    # system code page unless told; under GCC and Clang, the ladder's flags
    if(MSVC)
        target_compile_options(${_target} PRIVATE /utf-8)
    elseif( (DJINTERP_PARSE_LOG_STRICT) AND
            (CMAKE_C_COMPILER_ID MATCHES "^(GNU|Clang|AppleClang)$") AND
            (NOT CMAKE_C_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC") )
        target_compile_options(${_target} PRIVATE
            -Wall -Wextra -pedantic-errors -Werror=undef)
        # a strict ISO mode hides the POSIX names the framework uses on
        # POSIX systems; the ladder defines this for the same reason
        if(UNIX AND NOT APPLE)
            target_compile_definitions(${_target} PRIVATE _XOPEN_SOURCE=700)
        endif()
    endif()

    target_link_libraries(${_target} PRIVATE Threads::Threads)

    if(UNIX)
        target_link_libraries(${_target} PRIVATE m)
    endif()

    # DTest's C face, from the framework's library where there is one
    if(TARGET djinterp_c_test)
        target_link_libraries(${_target} PRIVATE djinterp_c_test)
    endif()

    add_test(NAME ${_target} COMMAND ${_target})

    # join the framework's aggregates (djinterp_add_test_subtree), which build
    # the test targets registered below them; standing alone, nothing reads
    # the registry
    set_property(GLOBAL APPEND PROPERTY DJINTERP_TEST_TARGETS ${_target})
    set_property(GLOBAL APPEND PROPERTY DJINTERP_TEST_TARGET_DIRS
                 "${CMAKE_CURRENT_SOURCE_DIR}")
endforeach()
