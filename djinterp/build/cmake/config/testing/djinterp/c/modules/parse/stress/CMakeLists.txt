#*******************************************************************************
# djinterp [parse]                                                CMakeLists.txt
#
# Test leaf for expression core v2's stress suite (parse/expr): seeded round
# trips and seeded random bytes, in DTest 2.0's C face, with the libFuzzer
# harness beside it.
#   Builds the suite from the runner in this directory, the section
# translation units of tests/djinterp/c/modules/parse/stress/, and the
# expression core's suite fixtures (the languages and the term generator),
# over expression core v2, the two parse units under it and DTest's C face, in testing mode, once per C level. Each level runs as
# a test.
#   Beside it, two programs from the harness, stress_fuzz.c: its corpus
# replay, built on every compiler and run as a test over every file of
# tests/djinterp/c/modules/parse/stress/corpus/; and the harness itself,
# built only where the compiler links -fsanitize=fuzzer, and never run as a
# test: a fuzzer runs on a budget of its own, nightly.
#   Plain CMake, as the other parse leaves are, so that the leaf works both
# ways as it stands:
#     cmake -S <this directory> -B <out> && cmake --build <out> && \
#         ctest --test-dir <out>
#     add_subdirectory(<this directory> <binary directory>)
#
#   DTest's C face is the framework's library, djinterp_c_test, where the
# enclosing project defines it; standing alone, the leaf compiles its one
# source, test_suite.c, at each level itself.
#
#   cache:  DJINTERP_PARSE_STRESS_C_LEVELS  C levels to build the suite at
#           DJINTERP_PARSE_STRESS_STRICT    the ladder's diagnostics, as
#                                           errors, where the compiler has
#                                           them
#           DJINTERP_PARSE_STRESS_FUZZER    build the harness where the
#                                           compiler offers libFuzzer
#
#
# path:      /build/cmake/config/testing/djinterp/c/modules/parse/stress/CMakeLists.txt
# link(s):   /docs/djinterp/parse/expression_core_v2.md
# author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.18
#                                                            revised: 2026.11.23
#*******************************************************************************
cmake_minimum_required(VERSION 3.16)

# standing alone, this leaf is the project; added from another, it is not
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(djinterp_parse_stress_tests LANGUAGES C)
    enable_testing()
endif()

# a second add_subdirectory() of this leaf would define every target twice
get_property(_seen GLOBAL PROPERTY DJINTERP_PARSE_STRESS_LEAF)
if(_seen)
    return()
endif()
set_property(GLOBAL PROPERTY DJINTERP_PARSE_STRESS_LEAF ON)

get_property(_languages GLOBAL PROPERTY ENABLED_LANGUAGES)
if(NOT "C" IN_LIST _languages)
    message(FATAL_ERROR
            "the parse stress leaf needs C: the enclosing project() must "
            "enable it")
endif()

# the repository root, nine levels above this leaf
get_filename_component(_root
                       "${CMAKE_CURRENT_LIST_DIR}/../../../../../../../../.."
                       ABSOLUTE)

set(DJINTERP_PARSE_STRESS_C_LEVELS "99;11;17;23" CACHE STRING
    "C levels the expression core's stress suite is built and run at")
option(DJINTERP_PARSE_STRESS_STRICT
       "Build with the ladder's diagnostics as errors, where there are any"
       ON)
option(DJINTERP_PARSE_STRESS_FUZZER
       "Build the libFuzzer harness where the compiler offers it" ON)

set(_parse "${_root}/src/djinterp/c/modules/parse")
set(_tests "${_root}/tests/djinterp/c/modules/parse/stress")
set(_expr  "${_root}/tests/djinterp/c/modules/parse/expr")
file(GLOB _core CONFIGURE_DEPENDS "${_parse}/expr/*.c")
file(GLOB _sections CONFIGURE_DEPENDS "${_tests}/stress_tests_*.c")
file(GLOB _corpus CONFIGURE_DEPENDS "${_tests}/corpus/*")
# DTest's C face: the framework's library, linked below, where the enclosing
# project defines it; standing alone, its one source
if(TARGET djinterp_c_test)
    set(_dtest "")
else()
    set(_dtest "${_root}/src/djinterp/c/modules/test/test_suite.c")
endif()

set(_common
    ${_sections}
    "${_expr}/expr_tests_fixtures.c"
    ${_core}
    "${_parse}/diagnostic.c"
    "${_parse}/storage.c"
    ${_dtest})

# djinterp_parse_stress_target
#   function: gives a target of this leaf the suite's level, include
# directories, definitions and, where asked, the ladder's diagnostics.
function(djinterp_parse_stress_target _target _level)
    set_target_properties(${_target} PROPERTIES
        C_STANDARD          ${_level}
        C_STANDARD_REQUIRED ON
        C_EXTENSIONS        OFF)
    target_include_directories(${_target} PRIVATE
        "${_root}/inc" "${_tests}" "${_expr}")
    target_compile_definitions(${_target} PRIVATE
        D_TESTING=1
        RE_STD_CFG_TESTING=1)

    # the sources are UTF-8 without a byte-order mark, which cl reads in the
    # system code page unless told; under GCC and Clang, the ladder's flags
    if(MSVC)
        target_compile_options(${_target} PRIVATE /utf-8)
    elseif( (DJINTERP_PARSE_STRESS_STRICT) AND
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

    # DTest's C face, from the framework's library where there is one
    if(TARGET djinterp_c_test)
        target_link_libraries(${_target} PRIVATE djinterp_c_test)
    endif()

    # join the framework's aggregates (djinterp_add_test_subtree), which
    # build the test targets registered below them; standing alone, nothing
    # reads the registry
    set_property(GLOBAL APPEND PROPERTY DJINTERP_TEST_TARGETS ${_target})
    set_property(GLOBAL APPEND PROPERTY DJINTERP_TEST_TARGET_DIRS
                 "${CMAKE_CURRENT_SOURCE_DIR}")
endfunction()

set(_first_level "")

foreach(_level IN LISTS DJINTERP_PARSE_STRESS_C_LEVELS)
    if(NOT "c_std_${_level}" IN_LIST CMAKE_C_COMPILE_FEATURES)
        message(STATUS "djinterp parse stress: C${_level} left out -- "
                       "${CMAKE_C_COMPILER_ID} ${CMAKE_C_COMPILER_VERSION} "
                       "cannot select it")
        continue()
    endif()

    # a cl-style driver's switch for C23 is /std:clatest, its latest draft
    # rather than the level
    if("${CMAKE_C${_level}_STANDARD_COMPILE_OPTION}" MATCHES "latest")
        message(STATUS "djinterp parse stress: C${_level} left out -- the "
                       "compiler's switch for it is "
                       "${CMAKE_C${_level}_STANDARD_COMPILE_OPTION}, its "
                       "latest draft and not the level")
        continue()
    endif()

    if(_first_level STREQUAL "")
        set(_first_level ${_level})
    endif()

    set(_target djinterp_parse_stress_tests_c${_level})
    add_executable(${_target}
        "${CMAKE_CURRENT_LIST_DIR}/stress_tests_runner.c" ${_common})
    djinterp_parse_stress_target(${_target} ${_level})
    add_test(NAME ${_target} COMMAND ${_target})
endforeach()

if(_first_level STREQUAL "")
    return()
endif()

# the corpus replay: the harness over every file of the corpus, at the
# lowest level the suite builds at
set(_replay djinterp_parse_stress_replay)
add_executable(${_replay} "${_tests}/stress_fuzz.c" ${_common})
djinterp_parse_stress_target(${_replay} ${_first_level})
target_compile_definitions(${_replay} PRIVATE D_TESTS_PARSE_STRESS_REPLAY=1)
add_test(NAME ${_replay} COMMAND ${_replay} ${_corpus})

# the harness itself, where the compiler links libFuzzer
if( (DJINTERP_PARSE_STRESS_FUZZER) AND
    (CMAKE_C_COMPILER_ID MATCHES "Clang") )
    include(CheckCSourceCompiles)
    set(CMAKE_REQUIRED_FLAGS "-fsanitize=fuzzer")
    set(CMAKE_REQUIRED_LINK_OPTIONS "-fsanitize=fuzzer")
    check_c_source_compiles(
        "#include <stddef.h>
         #include <stdint.h>
         int LLVMFuzzerTestOneInput(const uint8_t* d, size_t n)
         { (void)d; (void)n; return 0; }"
        DJINTERP_PARSE_STRESS_HAS_LIBFUZZER)
    unset(CMAKE_REQUIRED_FLAGS)
    unset(CMAKE_REQUIRED_LINK_OPTIONS)
endif()

if(DJINTERP_PARSE_STRESS_HAS_LIBFUZZER)
    set(_fuzz djinterp_parse_stress_fuzz)
    add_executable(${_fuzz} "${_tests}/stress_fuzz.c" ${_common})
    djinterp_parse_stress_target(${_fuzz} ${_first_level})
    target_compile_options(${_fuzz} PRIVATE
        -fsanitize=fuzzer,address,undefined)
    target_link_options(${_fuzz} PRIVATE -fsanitize=fuzzer,address,undefined)
    message(STATUS "djinterp parse stress: the libFuzzer harness is built")
else()
    message(STATUS "djinterp parse stress: no libFuzzer here; the harness "
                   "is built as its corpus replay only")
endif()
