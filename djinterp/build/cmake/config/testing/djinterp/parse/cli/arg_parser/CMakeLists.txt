#*******************************************************************************
# djinterp [parse]                                                CMakeLists.txt
#
# Test leaf for the argument parser.
#   Builds the parser's C sources as C, at the C floor, into one static
# library in the testing configuration, then links the C suites against it
# once per C level, from the floor (C99) up. A level the compiler cannot
# select is left out, and the configure step says so.
#   A suite is a table of sections, and a section is what a configuration
# knob removes, so the number of sections run differs by configuration, by
# design. expected_sections.txt holds the count each level has in the
# default configuration, and every test here must print exactly that: a
# section that silently stopped being compiled is a failure, not a smaller
# pass.
#   The suites and the runner reach every header by a path relative to
# themselves, so no target here has an include directory but inc/.
#
#   PLAIN CMAKE, ON PURPOSE, as the parse/parsegen foundation's leaf is: the
# framework's own CMake helpers are being rewritten, and everything here is
# standard CMake, so the leaf works both ways as it stands:
#     cmake -S <this directory> -B <out> && cmake --build <out> && \
#         ctest --test-dir <out>
#     add_subdirectory(<this directory> <binary directory>)
#
#   cache:  DJINTERP_ARG_PARSER_C_LEVELS       C levels to build the suites at
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
#                                                            revised: 2026.10.13
#*******************************************************************************
cmake_minimum_required(VERSION 3.16)

# standing alone, this leaf is the project; added from another, it is not
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(djinterp_arg_parser_tests LANGUAGES C)
    enable_testing()
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
# the diagnostics the CI ladder builds with.
#   Under a cl-style driver (MSVC, clang-cl) that is /utf-8 and nothing more:
# the sources are UTF-8 with no byte-order mark, which cl reads in the system
# code page unless told, and the ladder's flags mean other things there.
# Under GCC and Clang it is the ladder's flags, with the usual warnings
# beside them, unless DJINTERP_ARG_PARSER_STRICT is off.
function(djinterp_arg_parser_flags _target)
    if(MSVC)
        target_compile_options(${_target} PRIVATE /utf-8)

        return()
    endif()

    if(NOT DJINTERP_ARG_PARSER_STRICT)
        return()
    endif()

    if( (NOT CMAKE_C_COMPILER_ID MATCHES "^(GNU|Clang|AppleClang)$") OR
        (CMAKE_C_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC") )
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
#   function: holds a test to the number of sections its level has in the
# default configuration, as expected_sections.txt records it. A test with a
# pass expression is judged by its output alone, so the fail expression
# beside it catches a failed section whatever the total printed.
function(djinterp_arg_parser_expect _test _level)
    if(NOT DJINTERP_ARG_PARSER_EXPECT_COUNTS)
        return()
    endif()

    file(STRINGS "${CMAKE_CURRENT_LIST_DIR}/expected_sections.txt" _lines
         REGEX "^[a-z+]+[0-9]+ +[0-9]+$")

    foreach(_line IN LISTS _lines)
        if(_line MATCHES "^([a-z+]+[0-9]+) +([0-9]+)$")
            if(CMAKE_MATCH_1 STREQUAL "${_level}")
                set_tests_properties(${_test} PROPERTIES
                    PASS_REGULAR_EXPRESSION
                        "passed: ${CMAKE_MATCH_2}   failed: 0"
                    FAIL_REGULAR_EXPRESSION
                        "\\[FAIL\\]")

                return()
            endif()
        endif()
    endforeach()

    message(FATAL_ERROR
            "expected_sections.txt has no count for ${_level}; add the "
            "level's line, or set DJINTERP_ARG_PARSER_EXPECT_COUNTS=OFF")
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

djinterp_arg_parser_flags(djinterp_arg_parser_testing)
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
        "${_tests}/c/arg_smoke_tests.c"
        "${CMAKE_CURRENT_LIST_DIR}/arg_parser_c_tests_runner.c")

    set_target_properties(${_target} PROPERTIES
        C_STANDARD          ${_level}
        C_STANDARD_REQUIRED ON
        C_EXTENSIONS        OFF)

    target_link_libraries(${_target} PRIVATE djinterp_arg_parser_testing)
    djinterp_arg_parser_flags(${_target})
    djinterp_arg_parser_sanitize(${_target})

    add_test(NAME ${_target} COMMAND ${_target})
    djinterp_arg_parser_expect(${_target} "c${_level}")
endforeach()
