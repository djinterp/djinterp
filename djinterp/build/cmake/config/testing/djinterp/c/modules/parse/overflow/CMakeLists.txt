#*******************************************************************************
# djinterp [parse]                                                CMakeLists.txt
#
# Test leaf for the upgrade audit's allocation sites in parse and parsegen.
#   Builds tests/djinterp/c/modules/parse/t_parse_overflow.c, which drives a
# count too large at each site that takes one from a caller, with the parse
# and parsegen units those sites are in and the units they stand on, in
# testing mode, once per C level from the C floor (C99) up, and runs each as
# a test. The suite is a plain main: it prints "[PASS]" or "[FAIL]" for each
# section and returns 0 only when every section passed, so a test fails on
# its exit status or on a "[FAIL]" line. A level the compiler cannot select
# is left out, and the configure step says so. The refusals that exist only
# where size_t is 32 bits run in a 32-bit build: configure one (-m32, or
# -A Win32 with Visual Studio) to reach them.
#   Plain CMake, as the foundation leaf beside parsegen is, so that the leaf
# works both ways as it stands:
#     cmake -S <this directory> -B <out> && cmake --build <out> && \
#         ctest --test-dir <out>
#     add_subdirectory(<this directory> <binary directory>)
#
#   cache:  DJINTERP_PARSE_OVERFLOW_C_LEVELS  C levels to build the suite at
#           DJINTERP_PARSE_OVERFLOW_STRICT    the ladder's diagnostics, as
#                                             errors, where the compiler has
#                                             them
#
#
# path:      /build/cmake/config/testing/djinterp/c/modules/parse/overflow/CMakeLists.txt
# link(s):   TBA
# author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.05
#                                                            revised: 2026.11.05
#*******************************************************************************
cmake_minimum_required(VERSION 3.16)

# standing alone, this leaf is the project; added from another, it is not
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(djinterp_parse_overflow_tests LANGUAGES C)
    enable_testing()
endif()

# a second add_subdirectory() of this leaf would define every target twice
get_property(_seen GLOBAL PROPERTY DJINTERP_PARSE_OVERFLOW_LEAF)
if(_seen)
    return()
endif()
set_property(GLOBAL PROPERTY DJINTERP_PARSE_OVERFLOW_LEAF ON)

get_property(_languages GLOBAL PROPERTY ENABLED_LANGUAGES)
if(NOT "C" IN_LIST _languages)
    message(FATAL_ERROR
            "the parse overflow leaf needs C: the enclosing project() must "
            "enable it")
endif()

# the repository root, nine levels above this leaf
get_filename_component(_root
                       "${CMAKE_CURRENT_LIST_DIR}/../../../../../../../../.."
                       ABSOLUTE)

set(DJINTERP_PARSE_OVERFLOW_C_LEVELS "99;11;17;23" CACHE STRING
    "C levels the overflow suite is built and run at")
option(DJINTERP_PARSE_OVERFLOW_STRICT
       "Build with the ladder's diagnostics as errors, where there are any"
       ON)

set(_parse    "${_root}/src/djinterp/c/modules/parse")
set(_parsegen "${_root}/src/djinterp/c/modules/parsegen")
file(GLOB _arg_parser CONFIGURE_DEPENDS "${_parse}/cli/arg_parser/*.c")
set(_sources
    "${_root}/tests/djinterp/c/modules/parse/t_parse_overflow.c"
    "${_parse}/charset.c"
    "${_parse}/diagnostic.c"
    "${_parse}/machine.c"
    "${_parse}/pool.c"
    "${_parse}/program.c"
    "${_parse}/storage.c"
    "${_parse}/syntax/syntax_group.c"
    "${_parse}/syntax/syntax_item.c"
    ${_arg_parser}
    "${_parsegen}/feature.c"
    "${_parsegen}/grammar.c"
    "${_parsegen}/registry.c"
    "${_root}/src/djinterp/c/util/sink_common.c")

foreach(_level IN LISTS DJINTERP_PARSE_OVERFLOW_C_LEVELS)
    if(NOT "c_std_${_level}" IN_LIST CMAKE_C_COMPILE_FEATURES)
        message(STATUS "djinterp parse overflow: C${_level} left out -- "
                       "${CMAKE_C_COMPILER_ID} ${CMAKE_C_COMPILER_VERSION} "
                       "cannot select it")
        continue()
    endif()

    # a cl-style driver's switch for C23 is /std:clatest, its latest draft
    # rather than the level
    if("${CMAKE_C${_level}_STANDARD_COMPILE_OPTION}" MATCHES "latest")
        message(STATUS "djinterp parse overflow: C${_level} left out -- the "
                       "compiler's switch for it is "
                       "${CMAKE_C${_level}_STANDARD_COMPILE_OPTION}, its "
                       "latest draft and not the level")
        continue()
    endif()

    set(_target djinterp_parse_overflow_tests_c${_level})
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
    elseif( (DJINTERP_PARSE_OVERFLOW_STRICT) AND
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
