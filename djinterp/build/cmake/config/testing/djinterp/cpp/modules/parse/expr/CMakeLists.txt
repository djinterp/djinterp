#*******************************************************************************
# djinterp [parse]                                                CMakeLists.txt
#
# Test leaf for expression core v2's C++ face (cpp/modules/parse/expr/).
#   Builds the suite in DTest 2.0's C face: the runner in this directory and
# the section translation units of tests/djinterp/cpp/modules/parse/expr/
# (expr_face_tests_*.cpp), as C++, once per C++ level from C++98 up, over
# one C99 build of what they stand on: expression core v2, the two parse
# units under it, and DTest's one source, test_suite.c, in testing mode. Each
# level runs as a test. The runner returns 0 only when every test passed or
# was skipped (the bridge's are, below C++11), so a test fails on its exit
# status. A level the compiler cannot select is left out, and the configure
# step says so.
#   C++98 is built twice, as the ladder builds it (decision 4.5): `98`, where
# the framework may use variadic macros, which GCC and Clang take in C++98
# as an extension, and `98s`, ISO strict (D_CFG_ENV_ISO_STRICT and
# RE_STD_CFG_ISO_STRICT 1), held to the standard alone.
#   Plain CMake, as the foundation leaf beside parsegen is, so that the leaf
# works both ways as it stands:
#     cmake -S <this directory> -B <out> && cmake --build <out> && \
#         ctest --test-dir <out>
#     add_subdirectory(<this directory> <binary directory>)
#
#   cache:  DJINTERP_PARSE_EXPR_FACE_CXX_LEVELS  C++ levels to build the
#                                                suite at
#           DJINTERP_PARSE_EXPR_FACE_STRICT      the ladder's diagnostics, as
#                                                errors, where the compiler
#                                                has them
#
#
# path:      /build/cmake/config/testing/djinterp/cpp/modules/parse/expr/CMakeLists.txt
# link(s):   /docs/djinterp/parse/expression_core_v2.md
# author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.12
#                                                            revised: 2026.11.12
#*******************************************************************************
cmake_minimum_required(VERSION 3.16)

# standing alone, this leaf is the project; added from another, it is not
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(djinterp_parse_expr_face_tests LANGUAGES C CXX)
    enable_testing()
endif()

# a second add_subdirectory() of this leaf would define every target twice
get_property(_seen GLOBAL PROPERTY DJINTERP_PARSE_EXPR_FACE_LEAF)
if(_seen)
    return()
endif()
set_property(GLOBAL PROPERTY DJINTERP_PARSE_EXPR_FACE_LEAF ON)

get_property(_languages GLOBAL PROPERTY ENABLED_LANGUAGES)
if( (NOT "C" IN_LIST _languages) OR (NOT "CXX" IN_LIST _languages) )
    message(FATAL_ERROR
            "the parse expr face leaf needs C and C++: the enclosing "
            "project() must enable both")
endif()

# the repository root, nine levels above this leaf
get_filename_component(_root
                       "${CMAKE_CURRENT_LIST_DIR}/../../../../../../../../.."
                       ABSOLUTE)

set(DJINTERP_PARSE_EXPR_FACE_CXX_LEVELS "98s;98;11;14;17;20;23;26" CACHE
    STRING "C++ levels the face suite is built and run at")
option(DJINTERP_PARSE_EXPR_FACE_STRICT
       "Build with the ladder's diagnostics as errors, where there are any"
       ON)

set(_parse "${_root}/src/djinterp/c/modules/parse")
set(_tests "${_root}/tests/djinterp/cpp/modules/parse/expr")
file(GLOB _core CONFIGURE_DEPENDS "${_parse}/expr/*.c")
file(GLOB _sections CONFIGURE_DEPENDS "${_tests}/expr_face_tests_*.cpp")

# GCC or Clang with their own driver: the ladder's flags apply
set(_gnu OFF)
if( (CMAKE_CXX_COMPILER_ID MATCHES "^(GNU|Clang|AppleClang)$") AND
    (NOT CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC") )
    set(_gnu ON)
endif()

# what the suite stands on, in C, built once
set(_c_target djinterp_parse_expr_face_core)
add_library(${_c_target} OBJECT
    ${_core}
    "${_parse}/diagnostic.c"
    "${_parse}/storage.c"
    "${_root}/src/djinterp/c/modules/test/test_suite.c")
set_target_properties(${_c_target} PROPERTIES
    C_STANDARD          99
    C_STANDARD_REQUIRED ON
    C_EXTENSIONS        OFF)
target_include_directories(${_c_target} PRIVATE "${_root}/inc")
target_compile_definitions(${_c_target} PRIVATE
    D_TESTING=1
    RE_STD_CFG_TESTING=1)

if(MSVC)
    target_compile_options(${_c_target} PRIVATE /utf-8)
elseif( (DJINTERP_PARSE_EXPR_FACE_STRICT) AND
        (CMAKE_C_COMPILER_ID MATCHES "^(GNU|Clang|AppleClang)$") AND
        (NOT CMAKE_C_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC") )
    target_compile_options(${_c_target} PRIVATE
        -Wall -Wextra -pedantic-errors -Werror=undef)
    if(UNIX AND NOT APPLE)
        target_compile_definitions(${_c_target} PRIVATE _XOPEN_SOURCE=700)
    endif()
endif()

foreach(_rung IN LISTS DJINTERP_PARSE_EXPR_FACE_CXX_LEVELS)
    # `98s` is C++98, ISO strict
    string(REGEX REPLACE "s$" "" _level "${_rung}")

    if(NOT "cxx_std_${_level}" IN_LIST CMAKE_CXX_COMPILE_FEATURES)
        message(STATUS "djinterp parse expr face: C++${_rung} left out -- "
                       "${CMAKE_CXX_COMPILER_ID} "
                       "${CMAKE_CXX_COMPILER_VERSION} cannot select it")
        continue()
    endif()

    # a cl-style driver's switch for the newest level is /std:c++latest, its
    # latest draft rather than the level
    if("${CMAKE_CXX${_level}_STANDARD_COMPILE_OPTION}" MATCHES "latest")
        message(STATUS "djinterp parse expr face: C++${_rung} left out -- "
                       "the compiler's switch for it is "
                       "${CMAKE_CXX${_level}_STANDARD_COMPILE_OPTION}, its "
                       "latest draft and not the level")
        continue()
    endif()

    set(_target djinterp_parse_expr_face_tests_cxx${_rung})
    add_executable(${_target}
        "${CMAKE_CURRENT_LIST_DIR}/expr_face_tests_runner.cpp"
        ${_sections}
        $<TARGET_OBJECTS:${_c_target}>)
    set_target_properties(${_target} PROPERTIES
        CXX_STANDARD          ${_level}
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS        OFF)
    target_include_directories(${_target} PRIVATE "${_root}/inc" "${_tests}")
    target_compile_definitions(${_target} PRIVATE
        D_TESTING=1
        RE_STD_CFG_TESTING=1)

    if("${_rung}" STREQUAL "98s")
        target_compile_definitions(${_target} PRIVATE
            D_CFG_ENV_ISO_STRICT=1
            RE_STD_CFG_ISO_STRICT=1)
    endif()

    # the sources are UTF-8 without a byte-order mark, which cl reads in the
    # system code page unless told; under GCC and Clang, the ladder's flags
    if(MSVC)
        target_compile_options(${_target} PRIVATE /utf-8 /EHsc)
    elseif( (DJINTERP_PARSE_EXPR_FACE_STRICT) AND (_gnu) )
        target_compile_options(${_target} PRIVATE
            -Wall -Wextra -pedantic-errors -Werror=undef)
        if("${_rung}" STREQUAL "98")
            target_compile_options(${_target} PRIVATE -Wno-variadic-macros)
        endif()
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

    # join the framework's aggregates (djinterp_add_test_subtree), which build
    # the test targets registered below them; standing alone, nothing reads
    # the registry
    set_property(GLOBAL APPEND PROPERTY DJINTERP_TEST_TARGETS ${_target})
    set_property(GLOBAL APPEND PROPERTY DJINTERP_TEST_TARGET_DIRS
                 "${CMAKE_CURRENT_SOURCE_DIR}")
endforeach()
