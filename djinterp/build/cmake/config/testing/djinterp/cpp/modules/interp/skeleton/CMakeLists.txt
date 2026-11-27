#*******************************************************************************
# djinterp [test]                                                 CMakeLists.txt
#
# Test leaf for the interpreter's skeleton, its second step: language.hpp and
# evaluate.hpp, on expression core v2's C API. Its own leaf, and nothing of
# it is installed: the skeleton is a preview (the interpreter's design,
# decision 1).
#   The core's C sources, with parse's diagnostics and storage and the
# regular expressions `=~` matches with (c/util/dregex.c), are compiled once
# as C at the C floor; the skeleton's sources, its suite and the runner at
# each C++ level from its floor, C++11, each run held to the suite's 11
# tests, every one passed. It stands alone, or below another project that
# enables C and C++.
#   Run it with
#     cmake -S . -B out && cmake --build out && ctest --test-dir out
#
#
# path:      /build/cmake/config/testing/djinterp/cpp/modules/interp/skeleton/CMakeLists.txt
# link(s):   /docs/djinterp/cpp/modules/interp/DESIGN.html
# author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.22
#                                                            revised: 2026.11.25
#*******************************************************************************
cmake_minimum_required(VERSION 3.16)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(djinterp_interp_skeleton_tests LANGUAGES C CXX)
    enable_testing()
endif()

get_property(_seen GLOBAL PROPERTY DJINTERP_INTERP_SKELETON_LEAF)
if(_seen)
    return()
endif()
set_property(GLOBAL PROPERTY DJINTERP_INTERP_SKELETON_LEAF ON)

get_property(_languages GLOBAL PROPERTY ENABLED_LANGUAGES)
if( (NOT "C" IN_LIST _languages) OR (NOT "CXX" IN_LIST _languages) )
    message(FATAL_ERROR
            "the interpreter's skeleton leaf needs C and C++: the enclosing "
            "project() must enable both")
endif()

get_filename_component(_root
                       "${CMAKE_CURRENT_LIST_DIR}/../../../../../../../../.."
                       ABSOLUTE)

# whether the compiler builds a level as itself, asked of every level
# below; the enclosing project has it already
include("${_root}/build/cmake/djinterp/standards.cmake")

set(DJINTERP_INTERP_SKELETON_CXX_LEVELS "11;14;17;20;23" CACHE STRING
    "C++ levels the interpreter's skeleton is built and run at")
option(DJINTERP_INTERP_SKELETON_STRICT
       "Build with the ladder's diagnostics as errors, where there are any"
       ON)

set(_parse "${_root}/src/djinterp/c/modules/parse")
set(_interp "${_root}/src/djinterp/cpp/modules/interp")
set(_tests "${_root}/tests/djinterp/cpp/modules/interp")
file(GLOB _core CONFIGURE_DEPENDS "${_parse}/expr/*.c")

set(_gnu OFF)
if( (CMAKE_CXX_COMPILER_ID MATCHES "^(GNU|Clang|AppleClang)$") AND
    (NOT CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC") )
    set(_gnu ON)
endif()

# the core, once, as C at the C floor
add_library(djinterp_interp_skeleton_core STATIC
    ${_core}
    "${_parse}/diagnostic.c"
    "${_parse}/storage.c"
    "${_root}/src/djinterp/c/util/dregex.c"
    "${_root}/src/djinterp/c/modules/test/test_suite.c")
set_target_properties(djinterp_interp_skeleton_core PROPERTIES
    C_STANDARD          99
    C_STANDARD_REQUIRED ON
    C_EXTENSIONS        OFF)
target_include_directories(djinterp_interp_skeleton_core PUBLIC
                           "${_root}/inc")
target_compile_definitions(djinterp_interp_skeleton_core PUBLIC
    D_TESTING=1
    RE_STD_CFG_TESTING=1)

if(MSVC)
    target_compile_options(djinterp_interp_skeleton_core PRIVATE /utf-8)
elseif(_gnu)
    target_compile_options(djinterp_interp_skeleton_core PRIVATE
                           -Wall -Wextra -pedantic -Werror=undef)
    if(UNIX AND NOT APPLE)
        target_compile_definitions(djinterp_interp_skeleton_core PUBLIC
                                   _XOPEN_SOURCE=700)
    endif()
endif()

if(UNIX)
    target_link_libraries(djinterp_interp_skeleton_core PUBLIC m)
endif()

foreach(_level IN LISTS DJINTERP_INTERP_SKELETON_CXX_LEVELS)
    # a level the compiler cannot build as itself is left out
    djinterp_standard_selectable(CXX ${_level} _selectable _why)
    if(NOT _selectable)
        message(STATUS "djinterp interpreter skeleton: C++${_level} left out - "
                       "${_why}")
        continue()
    endif()

    set(_target djinterp_interp_skeleton_tests_cxx${_level})
    add_executable(${_target}
        "${CMAKE_CURRENT_LIST_DIR}/skeleton_tests_runner.cpp"
        "${_tests}/skeleton_tests.cpp"
        "${_interp}/language.cpp"
        "${_interp}/evaluate.cpp")
    set_target_properties(${_target} PROPERTIES
        CXX_STANDARD          ${_level}
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS        OFF)
    target_link_libraries(${_target} PRIVATE djinterp_interp_skeleton_core)

    if(MSVC)
        target_compile_options(${_target} PRIVATE /utf-8 /W4)
    elseif( (DJINTERP_INTERP_SKELETON_STRICT) AND (_gnu) )
        target_compile_options(${_target} PRIVATE
                               -Wall -Wextra -pedantic-errors -Werror=undef)
    endif()

    # every test passed, and as many as the suite has: a section that
    # stopped being compiled fails the run, not shrinks it
    add_test(NAME ${_target} COMMAND ${_target})
    set_tests_properties(${_target} PROPERTIES
        PASS_REGULAR_EXPRESSION "tests: +11 \\(11 passed, 0 failed, 0 skipped\\)")

    set_property(GLOBAL APPEND PROPERTY DJINTERP_TEST_TARGETS ${_target})
    set_property(GLOBAL APPEND PROPERTY DJINTERP_TEST_TARGET_DIRS
                 "${CMAKE_CURRENT_SOURCE_DIR}")
endforeach()
