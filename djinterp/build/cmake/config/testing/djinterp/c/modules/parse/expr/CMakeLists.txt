#*******************************************************************************
# djinterp [parse]                                                CMakeLists.txt
#
# Test leaf for expression core v2's C core (c/modules/parse/expr/).
#   Builds tests/djinterp/c/modules/parse/expr/t_parse_expr.c with the core's
# sources and the two parse units it stands on, diagnostic.c and storage.c,
# in testing mode, once per C level from the C floor (C99) up, and runs each
# as a test. The suite is a plain main until lane F's week 2 moves it to
# DTest 2.0: it prints "[PASS]" or "[FAIL]" for each section and returns 0
# only when every section passed, so a test fails on its exit status or on a
# "[FAIL]" line. A level the compiler cannot select is left out, and the
# configure step says so.
#   Plain CMake, as the foundation leaf beside parsegen is, so that the leaf
# works both ways as it stands:
#     cmake -S <this directory> -B <out> && cmake --build <out> && \
#         ctest --test-dir <out>
#     add_subdirectory(<this directory> <binary directory>)
#
#   cache:  DJINTERP_PARSE_EXPR_C_LEVELS  C levels to build the suite at
#           DJINTERP_PARSE_EXPR_STRICT    the ladder's diagnostics, as errors,
#                                         where the compiler has them
#
#
# path:      /build/cmake/config/testing/djinterp/c/modules/parse/expr/CMakeLists.txt
# link(s):   TBA
# author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.05
#                                                            revised: 2026.11.05
#*******************************************************************************
cmake_minimum_required(VERSION 3.16)

# standing alone, this leaf is the project; added from another, it is not
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(djinterp_parse_expr_tests LANGUAGES C)
    enable_testing()
endif()

# a second add_subdirectory() of this leaf would define every target twice
get_property(_seen GLOBAL PROPERTY DJINTERP_PARSE_EXPR_LEAF)
if(_seen)
    return()
endif()
set_property(GLOBAL PROPERTY DJINTERP_PARSE_EXPR_LEAF ON)

get_property(_languages GLOBAL PROPERTY ENABLED_LANGUAGES)
if(NOT "C" IN_LIST _languages)
    message(FATAL_ERROR
            "the parse expression leaf needs C: the enclosing project() must "
            "enable it")
endif()

# the repository root, nine levels above this leaf
get_filename_component(_root
                       "${CMAKE_CURRENT_LIST_DIR}/../../../../../../../../.."
                       ABSOLUTE)

set(DJINTERP_PARSE_EXPR_C_LEVELS "99;11;17;23" CACHE STRING
    "C levels the expression core's suite is built and run at")
option(DJINTERP_PARSE_EXPR_STRICT
       "Build with the ladder's diagnostics as errors, where there are any"
       ON)

set(_parse "${_root}/src/djinterp/c/modules/parse")
file(GLOB _core CONFIGURE_DEPENDS "${_parse}/expr/*.c")
set(_sources
    "${_root}/tests/djinterp/c/modules/parse/expr/t_parse_expr.c"
    ${_core}
    "${_parse}/diagnostic.c"
    "${_parse}/storage.c")

foreach(_level IN LISTS DJINTERP_PARSE_EXPR_C_LEVELS)
    if(NOT "c_std_${_level}" IN_LIST CMAKE_C_COMPILE_FEATURES)
        message(STATUS "djinterp parse expr: C${_level} left out -- "
                       "${CMAKE_C_COMPILER_ID} ${CMAKE_C_COMPILER_VERSION} "
                       "cannot select it")
        continue()
    endif()

    # a cl-style driver's switch for C23 is /std:clatest, its latest draft
    # rather than the level
    if("${CMAKE_C${_level}_STANDARD_COMPILE_OPTION}" MATCHES "latest")
        message(STATUS "djinterp parse expr: C${_level} left out -- the "
                       "compiler's switch for it is "
                       "${CMAKE_C${_level}_STANDARD_COMPILE_OPTION}, its "
                       "latest draft and not the level")
        continue()
    endif()

    set(_target djinterp_parse_expr_tests_c${_level})
    add_executable(${_target} ${_sources})
    set_target_properties(${_target} PROPERTIES
        C_STANDARD          ${_level}
        C_STANDARD_REQUIRED ON
        C_EXTENSIONS        OFF)
    target_include_directories(${_target} PRIVATE "${_root}/inc")
    target_compile_definitions(${_target} PRIVATE
        D_TESTING=1
        RE_STD_CFG_TESTING=1)

    # the sources are UTF-8 without a byte-order mark, which cl reads in the
    # system code page unless told; under GCC and Clang, the ladder's flags
    if(MSVC)
        target_compile_options(${_target} PRIVATE /utf-8)
    elseif( (DJINTERP_PARSE_EXPR_STRICT) AND
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

    if(UNIX)
        target_link_libraries(${_target} PRIVATE m)
    endif()

    add_test(NAME ${_target} COMMAND ${_target})
    set_tests_properties(${_target} PROPERTIES
        FAIL_REGULAR_EXPRESSION "\\[FAIL\\]")

    # join the framework's aggregates (djinterp_add_test_subtree), which build
    # the test targets registered below them; standing alone, nothing reads
    # the registry
    set_property(GLOBAL APPEND PROPERTY DJINTERP_TEST_TARGETS ${_target})
    set_property(GLOBAL APPEND PROPERTY DJINTERP_TEST_TARGET_DIRS
                 "${CMAKE_CURRENT_SOURCE_DIR}")
endforeach()
