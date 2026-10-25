#*******************************************************************************
# djinterp [parse]                                                CMakeLists.txt
#
# Test leaf for the argument parser.
#   Builds the parser's C sources as C, at the C floor, into one static
# library in the testing configuration, then links the C suites against it
# once per C level, from the floor (C99) up, and the C++ suites -- the C++
# face, called from C++ across a real language boundary -- once per C++
# level, from the face's floor (C++98, in strict ISO) up, and again without
# exceptions at the lowest and the highest level built. Where the linker
# wraps symbols (--wrap), an allocation sweep runs the C API's scenarios
# with every allocation refused in turn. A level the compiler cannot select
# is left out, and the configure step says so; so are the C++ suites when
# there is no C++ compiler, and the sweep when the linker cannot wrap.
#   A suite is a table of sections, and a section is what a configuration
# knob removes -- or a language level, or exceptions turned off -- so the
# number of sections run differs by configuration, by design.
# expected_sections.txt holds the count each build has in the default
# configuration, and every test here must print exactly that: a section
# that silently stopped being compiled is a failure, not a smaller pass.
#   The suites and the runners reach every header by a path relative to
# themselves, so no target here has an include directory but inc/.
#
#   PLAIN CMAKE, ON PURPOSE, as the parse/parsegen foundation's leaf is: the
# framework's own CMake helpers are being rewritten, and everything here is
# standard CMake, so the leaf works both ways as it stands:
#     cmake -S <this directory> -B <out> && cmake --build <out> && \
#         ctest --test-dir <out>
#     add_subdirectory(<this directory> <binary directory>)
#
#   cache:  DJINTERP_ARG_PARSER_C_LEVELS       C levels to build the C suites
#                                              at
#           DJINTERP_ARG_PARSER_CXX_LEVELS     C++ levels to build the C++
#                                              suites at
#           DJINTERP_ARG_PARSER_NO_EXCEPTIONS  the C++ suites without
#                                              exceptions too, where the
#                                              compiler turns them off
#           DJINTERP_ARG_PARSER_EXPECT_COUNTS  hold each test to its count;
#                                              OFF under a configuration knob
#           DJINTERP_ARG_PARSER_STRICT         the ladder's own diagnostics,
#                                              as errors, where the compiler
#                                              has them
#           DJINTERP_ARG_PARSER_SANITIZE       AddressSanitizer and
#                                              UndefinedBehaviorSanitizer,
#                                              where the compiler has them
#
# path:      /build/cmake/config/testing/djinterp/parse/cli/arg_parser/CMakeLists.txt
# link(s):   TBA
# author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.13
#                                                            revised: 2026.10.20
#*******************************************************************************
cmake_minimum_required(VERSION 3.16)

# standing alone, this leaf is the project; added from another, it is not.
# C++ is enabled when there is a compiler for it: without one the C suites
# still run, and the C++ ones are left out
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(djinterp_arg_parser_tests LANGUAGES C)
    enable_testing()

    include(CheckLanguage)
    check_language(CXX)

    if(CMAKE_CXX_COMPILER)
        enable_language(CXX)
    endif()
endif()

# a second add_subdirectory() of this leaf would define every target twice
if(TARGET djinterp_arg_parser_testing)
    return()
endif()

# the library and the suites are C: an enclosing project that did not enable
# it would otherwise fail far from the cause
get_property(_languages GLOBAL PROPERTY ENABLED_LANGUAGES)

if(NOT "C" IN_LIST _languages)
    message(FATAL_ERROR
            "the argument parser's leaf needs C: the enclosing project() "
            "must enable it (LANGUAGES C)")
endif()

# the repository root, eight levels above this leaf
get_filename_component(DJINTERP_ARG_PARSER_ROOT
                       "${CMAKE_CURRENT_LIST_DIR}/../../../../../../../.."
                       ABSOLUTE)

set(DJINTERP_ARG_PARSER_C_LEVELS "99;11;17;23" CACHE STRING
    "C levels the argument parser's C suites are built and run at")
set(DJINTERP_ARG_PARSER_CXX_LEVELS "98;11;14;17;20;23" CACHE STRING
    "C++ levels the argument parser's C++ suites are built and run at")
option(DJINTERP_ARG_PARSER_NO_EXCEPTIONS
       "Also build the C++ suites without exceptions, lowest and highest"
       ON)
option(DJINTERP_ARG_PARSER_EXPECT_COUNTS
       "Hold each test to the section count expected_sections.txt gives it"
       ON)
option(DJINTERP_ARG_PARSER_STRICT
       "Build with the ladder's diagnostics as errors, where there are any"
       ON)
option(DJINTERP_ARG_PARSER_SANITIZE
       "Build with AddressSanitizer and UndefinedBehaviorSanitizer"
       OFF)

# where the sources and the suites are
set(_src   "${DJINTERP_ARG_PARSER_ROOT}/src/djinterp")
set(_tests "${DJINTERP_ARG_PARSER_ROOT}/tests/djinterp/parse/cli/arg_parser")


#------------------------------------------------------------------------------
# helpers
#------------------------------------------------------------------------------

# djinterp_arg_parser_flags
#   function: gives a target what the tree's sources need of a compiler, and
# the diagnostics the CI ladder builds with, judged by the compiler of the
# language given -- C, or CXX for a C++ suite, whose one C source takes the
# same.
#   Under a cl-style driver (MSVC, clang-cl) that is /utf-8 and nothing more:
# the sources are UTF-8 with no byte-order mark, which cl reads in the system
# code page unless told, and the ladder's flags mean other things there.
# Under GCC and Clang it is the ladder's flags, with the usual warnings
# beside them, unless DJINTERP_ARG_PARSER_STRICT is off.
function(djinterp_arg_parser_flags _target _language)
    if(MSVC)
        target_compile_options(${_target} PRIVATE /utf-8)

        return()
    endif()

    if(NOT DJINTERP_ARG_PARSER_STRICT)
        return()
    endif()

    set(_compiler "${CMAKE_${_language}_COMPILER_ID}")
    set(_frontend "${CMAKE_${_language}_COMPILER_FRONTEND_VARIANT}")

    if( (NOT _compiler MATCHES "^(GNU|Clang|AppleClang)$") OR
        (_frontend STREQUAL "MSVC") )
        return()
    endif()

    target_compile_options(${_target} PRIVATE
        -Wall -Wextra -pedantic-errors -Werror=undef)

    # a strict ISO mode hides the POSIX and XSI names the framework uses on
    # POSIX systems; the ladder defines this for the same reason
    if(UNIX AND NOT APPLE)
        target_compile_definitions(${_target} PRIVATE _XOPEN_SOURCE=700)
    endif()
endfunction()

# djinterp_arg_parser_sanitize
#   function: builds a target with AddressSanitizer and, under GCC and Clang,
# UndefinedBehaviorSanitizer, which then ends the run at its first finding,
# when DJINTERP_ARG_PARSER_SANITIZE is on. A leak AddressSanitizer finds at
# exit fails the test as an error would.
function(djinterp_arg_parser_sanitize _target)
    if(NOT DJINTERP_ARG_PARSER_SANITIZE)
        return()
    endif()

    if(MSVC)
        target_compile_options(${_target} PRIVATE /fsanitize=address)

        return()
    endif()

    target_compile_options(${_target} PRIVATE
        -fsanitize=address,undefined
        -fno-sanitize-recover=undefined
        -fno-omit-frame-pointer)
    target_link_options(${_target} PRIVATE
        -fsanitize=address,undefined)
endfunction()

# djinterp_arg_parser_expect
#   function: holds a test to the number of sections its build has in the
# default configuration, as expected_sections.txt records it under the
# build's name: its level, and `-noexc` for a C++ build without exceptions.
# A test with a pass expression is judged by its output alone, so the fail
# expression beside it catches a failed section whatever the total printed.
function(djinterp_arg_parser_expect _test _level)
    if(NOT DJINTERP_ARG_PARSER_EXPECT_COUNTS)
        return()
    endif()

    file(STRINGS "${CMAKE_CURRENT_LIST_DIR}/expected_sections.txt" _lines
         REGEX "^[a-z+]+[0-9]+(-[a-z]+)? +[0-9]+$")

    foreach(_line IN LISTS _lines)
        if(_line MATCHES "^([a-z+]+[0-9]+(-[a-z]+)?) +([0-9]+)$")
            if(CMAKE_MATCH_1 STREQUAL "${_level}")
                set_tests_properties(${_test} PROPERTIES
                    PASS_REGULAR_EXPRESSION
                        "passed: ${CMAKE_MATCH_3}   failed: 0"
                    FAIL_REGULAR_EXPRESSION
                        "\\[FAIL\\]")

                return()
            endif()
        endif()
    endforeach()

    message(FATAL_ERROR
            "expected_sections.txt has no count for ${_level}; add the "
            "build's line, or set DJINTERP_ARG_PARSER_EXPECT_COUNTS=OFF")
endfunction()

# djinterp_arg_parser_cpp_test
#   function: one build of the C++ suites at a C++ level, against the
# library, run as a test held to the count expected_sections.txt gives
# `_key`; anything after the key is a compile option of the C++ sources --
# the one that turns exceptions off. C++98 without extensions is the
# framework's strict ISO mode, in which its variadic macros, an extension
# there, give way to ISO forms. The suites' one C source, the scratch
# directories their cases make files in, is C99, as the library is.
function(djinterp_arg_parser_cpp_test _target _level _key)
    add_executable(${_target} ${_cpp_sources})

    set_target_properties(${_target} PROPERTIES
        CXX_STANDARD          ${_level}
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS        OFF
        C_STANDARD            99
        C_STANDARD_REQUIRED   ON
        C_EXTENSIONS          OFF)

    if(_level EQUAL 98)
        target_compile_definitions(${_target} PRIVATE
            $<$<COMPILE_LANGUAGE:CXX>:D_CFG_ENV_ISO_STRICT=1>
            $<$<COMPILE_LANGUAGE:CXX>:RE_STD_CFG_ISO_STRICT=1>)
    endif()

    foreach(_option IN LISTS ARGN)
        target_compile_options(${_target} PRIVATE
            $<$<COMPILE_LANGUAGE:CXX>:${_option}>)
    endforeach()

    target_link_libraries(${_target} PRIVATE djinterp_arg_parser_testing)
    djinterp_arg_parser_flags(${_target} CXX)
    djinterp_arg_parser_sanitize(${_target})

    add_test(NAME ${_target} COMMAND ${_target})
    djinterp_arg_parser_expect(${_target} "${_key}")
endfunction()


#------------------------------------------------------------------------------
# the parser's library -- C sources, compiled as C at the C floor
#------------------------------------------------------------------------------
set(_parser "${_src}/parse/cli/arg_parser/c")

add_library(djinterp_arg_parser_testing STATIC
    "${_parser}/arg_argv.c"
    "${_parser}/arg_argv_source.c"
    "${_parser}/arg_command.c"
    "${_parser}/arg_command_option.c"
    "${_parser}/arg_command_query.c"
    "${_parser}/arg_command_set.c"
    "${_parser}/arg_command_tree.c"
    "${_parser}/arg_command_util.c"
    "${_parser}/arg_common.c"
    "${_parser}/arg_completion.c"
    "${_parser}/arg_completion_fish.c"
    "${_parser}/arg_completion_pwsh.c"
    "${_parser}/arg_completion_sh.c"
    "${_parser}/arg_config.c"
    "${_parser}/arg_config_read.c"
    "${_parser}/arg_config_write.c"
    "${_parser}/arg_escape.c"
    "${_parser}/arg_formatter.c"
    "${_parser}/arg_formatter_command.c"
    "${_parser}/arg_formatter_option.c"
    "${_parser}/arg_internal.c"
    "${_parser}/arg_list.c"
    "${_parser}/arg_option.c"
    "${_parser}/arg_option_link.c"
    "${_parser}/arg_option_result.c"
    "${_parser}/arg_option_set.c"
    "${_parser}/arg_option_value.c"
    "${_parser}/arg_parse.c"
    "${_parser}/arg_parse_option.c"
    "${_parser}/arg_parse_positional.c"
    "${_parser}/arg_parse_text.c"
    "${_parser}/arg_parser.c"
    "${_parser}/arg_process.c"
    "${_parser}/arg_process_config.c"
    "${_parser}/arg_suggest.c"
    "${_parser}/arg_system.c"
    "${_parser}/arg_type.c"
    "${_parser}/arg_validator.c"
    "${_parser}/arg_validator_extra.c"
    "${_parser}/arg_validator_member.c"
    "${_parser}/arg_validator_path.c"
    "${_parser}/arg_validator_unit.c"
    # the sinks the parser writes through
    "${_src}/c/util/sink_common.c"
    # re_std's one C source: what the fixed-width integer header declares
    "${_src}/c/re_std/dstdint.c")

set_target_properties(djinterp_arg_parser_testing PROPERTIES
    C_STANDARD          99
    C_STANDARD_REQUIRED ON
    C_EXTENSIONS        OFF)

target_include_directories(djinterp_arg_parser_testing
    PUBLIC "${DJINTERP_ARG_PARSER_ROOT}/inc")

# PUBLIC, not PRIVATE: testing mode changes knobs across the tree, which the
# library and every consumer must agree on. re_std's knob is set beside
# djinterp's so that neither depends on include order.
target_compile_definitions(djinterp_arg_parser_testing
    PUBLIC D_TESTING=1
           RE_STD_CFG_TESTING=1)

djinterp_arg_parser_flags(djinterp_arg_parser_testing C)
djinterp_arg_parser_sanitize(djinterp_arg_parser_testing)


#------------------------------------------------------------------------------
# the C suites -- the C API called from C, one executable per level, against
# the same library
#------------------------------------------------------------------------------
foreach(_level IN LISTS DJINTERP_ARG_PARSER_C_LEVELS)
    if(NOT "c_std_${_level}" IN_LIST CMAKE_C_COMPILE_FEATURES)
        message(STATUS "djinterp arg_parser: C${_level} left out -- "
                       "${CMAKE_C_COMPILER_ID} ${CMAKE_C_COMPILER_VERSION} "
                       "cannot select it")

        continue()
    endif()

    # cl has no switch for a C level past C17. Where CMake gives it one for
    # C23 the switch is /std:clatest -- whatever draft that compiler has,
    # which moves with the compiler -- and a draft is not a level a count
    # can be held to
    if("${CMAKE_C${_level}_STANDARD_COMPILE_OPTION}" MATCHES "latest")
        message(STATUS "djinterp arg_parser: C${_level} left out -- the "
                       "compiler's switch for it is "
                       "${CMAKE_C${_level}_STANDARD_COMPILE_OPTION}, its "
                       "latest draft and not the level")

        continue()
    endif()

    set(_target djinterp_arg_parser_c_tests_c${_level})

    add_executable(${_target}
        "${_tests}/c/arg_tests_support.c"
        "${_tests}/c/arg_tests_files.c"
        "${_tests}/c/arg_common_tests.c"
        "${_tests}/c/arg_type_tests.c"
        "${_tests}/c/arg_validator_tests.c"
        "${_tests}/c/arg_validator_extra_tests.c"
        "${_tests}/c/arg_validator_member_tests.c"
        "${_tests}/c/arg_validator_unit_tests.c"
        "${_tests}/c/arg_validator_path_tests.c"
        "${_tests}/c/arg_option_tests.c"
        "${_tests}/c/arg_command_tests.c"
        "${_tests}/c/arg_formatter_tests.c"
        "${_tests}/c/arg_parse_tests.c"
        "${_tests}/c/arg_process_tests.c"
        "${_tests}/c/arg_config_tests.c"
        "${_tests}/c/arg_argv_tests.c"
        "${_tests}/c/arg_parser_tests.c"
        "${_tests}/c/arg_suggest_tests.c"
        "${_tests}/c/arg_completion_tests.c"
        "${_tests}/c/arg_smoke_tests.c"
        "${CMAKE_CURRENT_LIST_DIR}/arg_parser_c_tests_runner.c")

    set_target_properties(${_target} PROPERTIES
        C_STANDARD          ${_level}
        C_STANDARD_REQUIRED ON
        C_EXTENSIONS        OFF)

    target_link_libraries(${_target} PRIVATE djinterp_arg_parser_testing)
    djinterp_arg_parser_flags(${_target} C)
    djinterp_arg_parser_sanitize(${_target})

    add_test(NAME ${_target} COMMAND ${_target})
    djinterp_arg_parser_expect(${_target} "c${_level}")
endforeach()


#------------------------------------------------------------------------------
# the allocation sweep -- the C API's scenarios with every allocation refused
# in turn, at the C floor, where the linker wraps malloc and kin (GNU ld's
# and lld's --wrap): the sweep's wrappers then stand in front of every
# allocation the library makes
#------------------------------------------------------------------------------
include(CheckCSourceCompiles)

set(CMAKE_REQUIRED_LINK_OPTIONS "-Wl,--wrap=malloc")
check_c_source_compiles("
#include <stddef.h>
void* __real_malloc(size_t _size);
void* __wrap_malloc(size_t _size) { return __real_malloc(_size); }
int main(void) { return 0; }
" DJINTERP_ARG_PARSER_LINKER_WRAPS)
unset(CMAKE_REQUIRED_LINK_OPTIONS)

if(DJINTERP_ARG_PARSER_LINKER_WRAPS)
    set(_target djinterp_arg_parser_c_sweep)

    add_executable(${_target}
        "${_tests}/c/arg_sweep_tests.c"
        "${CMAKE_CURRENT_LIST_DIR}/arg_parser_c_sweep_runner.c")

    set_target_properties(${_target} PROPERTIES
        C_STANDARD          99
        C_STANDARD_REQUIRED ON
        C_EXTENSIONS        OFF)

    target_link_libraries(${_target} PRIVATE djinterp_arg_parser_testing)
    target_link_options(${_target} PRIVATE
        -Wl,--wrap=malloc
        -Wl,--wrap=calloc
        -Wl,--wrap=realloc
        -Wl,--wrap=free)
    djinterp_arg_parser_flags(${_target} C)
    djinterp_arg_parser_sanitize(${_target})

    add_test(NAME ${_target} COMMAND ${_target})
    djinterp_arg_parser_expect(${_target} "c99-sweep")
else()
    message(STATUS "djinterp arg_parser: the allocation sweep left out -- "
                   "the linker has no --wrap")
endif()


#------------------------------------------------------------------------------
# the C++ suites -- the C++ face called from C++, one executable per level,
# against the same library, across a real language boundary
#------------------------------------------------------------------------------
get_property(_languages GLOBAL PROPERTY ENABLED_LANGUAGES)

if(NOT "CXX" IN_LIST _languages)
    message(STATUS "djinterp arg_parser: the C++ suites left out -- C++ is "
                   "not enabled: there is no C++ compiler, or the enclosing "
                   "project() did not enable it (LANGUAGES C CXX)")

    return()
endif()

set(_cpp_sources
    "${_tests}/arg_tests_support.cpp"
    "${_tests}/arg_common_tests.cpp"
    "${_tests}/arg_type_tests.cpp"
    "${_tests}/arg_validator_tests.cpp"
    "${_tests}/arg_option_tests.cpp"
    "${_tests}/arg_command_tests.cpp"
    "${_tests}/arg_formatter_tests.cpp"
    "${_tests}/arg_config_tests.cpp"
    "${_tests}/arg_argv_tests.cpp"
    "${_tests}/arg_parser_tests.cpp"
    "${_tests}/arg_completion_tests.cpp"
    "${_tests}/arg_boundary_tests.cpp"
    "${_tests}/arg_examples_tests.cpp"
    # the scratch directories the suites' cases make files in
    "${_tests}/c/arg_tests_files.c"
    "${CMAKE_CURRENT_LIST_DIR}/arg_parser_cpp_tests_runner.cpp")

set(_cpp_built "")

foreach(_level IN LISTS DJINTERP_ARG_PARSER_CXX_LEVELS)
    if(NOT "cxx_std_${_level}" IN_LIST CMAKE_CXX_COMPILE_FEATURES)
        message(STATUS "djinterp arg_parser: C++${_level} left out -- "
                       "${CMAKE_CXX_COMPILER_ID} "
                       "${CMAKE_CXX_COMPILER_VERSION} cannot select it")

        continue()
    endif()

    # cl's lowest level is C++14, so its C++98 and C++11 builds would be the
    # C++14 one under other names
    if( (MSVC) AND
        (_level LESS 14) )
        message(STATUS "djinterp arg_parser: C++${_level} left out -- the "
                       "lowest level a cl-style driver selects is C++14")

        continue()
    endif()

    # a switch for the compiler's latest draft is not a level a count can be
    # held to, as for C23 above
    if("${CMAKE_CXX${_level}_STANDARD_COMPILE_OPTION}" MATCHES "latest")
        message(STATUS "djinterp arg_parser: C++${_level} left out -- the "
                       "compiler's switch for it is "
                       "${CMAKE_CXX${_level}_STANDARD_COMPILE_OPTION}, its "
                       "latest draft and not the level")

        continue()
    endif()

    djinterp_arg_parser_cpp_test(djinterp_arg_parser_cpp_tests_cxx${_level}
                                 ${_level}
                                 "c++${_level}")
    list(APPEND _cpp_built ${_level})
endforeach()

# without exceptions: the face's guard compiled out, at the lowest and the
# highest level built, where the compiler has GCC's switch for it
if( (DJINTERP_ARG_PARSER_NO_EXCEPTIONS) AND
    (_cpp_built) )
    if( (NOT CMAKE_CXX_COMPILER_ID MATCHES "^(GNU|Clang|AppleClang)$") OR
        (CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC") )
        message(STATUS "djinterp arg_parser: the C++ suites without "
                       "exceptions left out -- ${CMAKE_CXX_COMPILER_ID} has "
                       "no -fno-exceptions")
    else()
        list(GET _cpp_built 0 _lowest)
        list(GET _cpp_built -1 _highest)

        foreach(_level IN ITEMS ${_lowest} ${_highest})
            if(TARGET djinterp_arg_parser_cpp_tests_cxx${_level}_noexc)
                continue()
            endif()

            djinterp_arg_parser_cpp_test(
                djinterp_arg_parser_cpp_tests_cxx${_level}_noexc
                ${_level}
                "c++${_level}-noexc"
                -fno-exceptions)
        endforeach()
    endif()
endif()
