#*******************************************************************************
# djinterp [parsegen]                                             CMakeLists.txt
#
# Test leaf for djinterp/parsegen/vparse.
#   Recipe R2: the module has production sources under
# src/djinterp/parsegen/vparse consumed only by this test exe, so they are
# compiled straight into it. The helper adds inc/ and defines D_TESTING=1,
# and that is every include directory there is: the tests and the runner
# reach each header by a path relative to themselves, so the module include
# directory this leaf once added for the bare-form headers went with the bare
# form, and the tests directory with it.
#   Where the framework's helper is not there -- this leaf configured on its
# own, or added by a project that has no djinterp_add_test_executable -- the
# leaf builds the same executable with plain CMake instead, once per C++
# level from the module floor (C++11) up. Every level must run all the
# suite's sections; a level the compiler cannot select is left out, and the
# configure step says so.
#     cmake -S <this directory> -B <out> && cmake --build <out> && \
#         ctest --test-dir <out>
#
#   cache:  DJINTERP_VPARSE_CXX_LEVELS  C++ levels the plain build runs the
#                                       suite at
#           DJINTERP_VPARSE_STRICT      the ladder's own diagnostics, as
#                                       errors, where the compiler has them
#
# path:      /build/cmake/config/testing/djinterp/parsegen/vparse/CMakeLists.txt
# link(s):   TBA
# author(s): TBA                                                    created: TBA
#                                                            revised: 2026.10.04
#*******************************************************************************
cmake_minimum_required(VERSION 3.16)

# standing alone, this leaf is the project; added from another, it is not
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(djinterp_vparse_tests LANGUAGES CXX)
    enable_testing()
endif()

# a second add_subdirectory() of this leaf would define every target twice
get_property(_added GLOBAL PROPERTY DJINTERP_VPARSE_LEAF_ADDED)

if(_added)
    return()
endif()

set_property(GLOBAL PROPERTY DJINTERP_VPARSE_LEAF_ADDED TRUE)

# the framework's roots, where the framework's own CMake has not said: this
# leaf is seven levels below the repository root
get_filename_component(_root "${CMAKE_CURRENT_LIST_DIR}/../../../../../../.."
                       ABSOLUTE)

if(NOT DEFINED DJINTERP_INC_ROOT)
    set(DJINTERP_INC_ROOT "${_root}/inc")
endif()

if(NOT DEFINED DJINTERP_SRC_ROOT)
    set(DJINTERP_SRC_ROOT "${_root}/src")
endif()

if(NOT DEFINED DJINTERP_TESTS_ROOT)
    set(DJINTERP_TESTS_ROOT "${_root}/tests")
endif()

set(_tests_dir "${DJINTERP_TESTS_ROOT}/djinterp/parsegen/vparse")
set(_src_dir   "${DJINTERP_SRC_ROOT}/djinterp/parsegen/vparse")

file(GLOB _sources CONFIGURE_DEPENDS "${_tests_dir}/vparse_tests_*.cpp")
list(FILTER _sources EXCLUDE REGEX "_main\\.cpp$")

file(GLOB _prod CONFIGURE_DEPENDS "${_src_dir}/*.cpp")


#------------------------------------------------------------------------------
# with the framework's helper
#------------------------------------------------------------------------------
if(COMMAND djinterp_add_test_executable)
    djinterp_add_test_executable(vparse_tests
        SOURCES
            "${CMAKE_CURRENT_SOURCE_DIR}/vparse_tests_runner.cpp"
            ${_sources}
            ${_prod})

    # re_std's knob beside the D_TESTING the helper defines, so that neither
    # depends on include order. It belongs in the helper, with D_TESTING;
    # until it is there, a second definition of it here does no harm
    if(TARGET vparse_tests)
        target_compile_definitions(vparse_tests PRIVATE RE_STD_CFG_TESTING=1)
    endif()

    return()
endif()


#------------------------------------------------------------------------------
# without it -- plain CMake, one executable per level
#------------------------------------------------------------------------------
set(DJINTERP_VPARSE_CXX_LEVELS "11;14;17;20;23" CACHE STRING
    "C++ levels the vparse suite is built and run at")
option(DJINTERP_VPARSE_STRICT
       "Build with the ladder's diagnostics as errors, where there are any"
       ON)

# how many sections the suite has -- every one of them at every level, so a
# level that ran fewer has lost one. vparse-AGENTS.md states the same number
set(_sections 15)

foreach(_level IN LISTS DJINTERP_VPARSE_CXX_LEVELS)
    if(NOT "cxx_std_${_level}" IN_LIST CMAKE_CXX_COMPILE_FEATURES)
        message(STATUS "djinterp vparse: C++${_level} left out -- "
                       "${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION} "
                       "cannot select it")

        continue()
    endif()

    # cl's lowest level is C++14, so its C++11 build would be the C++14 one
    # under another name
    if( (MSVC)             AND
        (_level EQUAL 11) )
        message(STATUS "djinterp vparse: C++11 left out -- the lowest level "
                       "a cl-style driver selects is C++14")

        continue()
    endif()

    set(_target vparse_tests_cxx${_level})

    add_executable(${_target}
        "${CMAKE_CURRENT_LIST_DIR}/vparse_tests_runner.cpp"
        ${_sources}
        ${_prod})

    set_target_properties(${_target} PROPERTIES
        CXX_STANDARD          ${_level}
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS        OFF)

    target_include_directories(${_target} PRIVATE "${DJINTERP_INC_ROOT}")
    target_compile_definitions(${_target} PRIVATE D_TESTING=1
                                                  RE_STD_CFG_TESTING=1)

    # the tree's sources are UTF-8 with no byte-order mark, which a cl-style
    # driver (MSVC, clang-cl) reads in the system code page unless told
    if(MSVC)
        target_compile_options(${_target} PRIVATE /utf-8)
    endif()

    # the diagnostics the CI ladder builds with. GCC and Clang only: under a
    # cl-style driver the same spellings mean other things
    if( (DJINTERP_VPARSE_STRICT)                                       AND
        (CMAKE_CXX_COMPILER_ID MATCHES "^(GNU|Clang|AppleClang)$")     AND
        (NOT CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC") )
        target_compile_options(${_target} PRIVATE
            -Wall -Wextra -pedantic-errors -Werror=undef)

        # a strict ISO mode hides the POSIX and XSI names the framework uses
        # on POSIX systems; the ladder defines this for the same reason
        if(UNIX AND NOT APPLE)
            target_compile_definitions(${_target} PRIVATE _XOPEN_SOURCE=700)
        endif()
    endif()

    # a test with a pass expression is judged by its output alone, so the
    # fail expression beside it catches a failed section whatever the total
    add_test(NAME ${_target} COMMAND ${_target})
    set_tests_properties(${_target} PROPERTIES
        PASS_REGULAR_EXPRESSION "passed: ${_sections}   failed: 0"
        FAIL_REGULAR_EXPRESSION "\\[FAIL\\]")
endforeach()
