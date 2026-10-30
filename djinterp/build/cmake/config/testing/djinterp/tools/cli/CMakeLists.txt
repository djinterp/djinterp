#*******************************************************************************
# djinterp [tools]                                                CMakeLists.txt
#
# Test leaf for the command-line kit.
#   Builds the argument parser's C sources and the kit's -- with filesys,
# which the kit reads and writes files through -- as C, at the C floor,
# position-independent, into two static libraries in the testing
# configuration, from the lists the kit's CMake module takes them from
# (DjinterpCliSources.cmake). The probe modules the plugin suites load are
# made as a program's modules are made, by d_cli_add_plugin, linking those
# libraries: `prog-KIND` for each kind cli_tests_probe.c builds. The C
# suites link against the libraries once per C level, from the floor (C99)
# up; the C++ suite -- the C++ face, called from C++ across a real language
# boundary -- once per C++ level, from the face's floor (C++98, in strict
# ISO) up, and again without exceptions at the lowest and the highest level
# built. Where the linker wraps symbols (--wrap), an allocation sweep runs
# the kit's cases with every allocation refused in turn. Each run is given
# the directory the probes are in. A level the compiler cannot select is
# left out, and the configure step says so; so is the C++ suite when there
# is no C++ compiler, and the sweep when the linker cannot wrap.
#   A suite is a table of sections, and a section is what a configuration
# knob removes -- or a language level, or exceptions turned off -- so the
# number of sections run differs by configuration, by design.
# expected_sections.txt holds the count each build has in the default
# configuration, and every test here must print exactly that.
#   PLAIN CMAKE, as the argument parser's leaf is: the leaf stands alone or
# is added from another project.
#     cmake -S <this directory> -B <out> && cmake --build <out> && \
#         ctest --test-dir <out>
#     add_subdirectory(<this directory> <binary directory>)
#
#   cache:  DJINTERP_CLI_C_LEVELS       C levels to build the C suites at
#           DJINTERP_CLI_CXX_LEVELS     C++ levels to build the C++ suite at
#           DJINTERP_CLI_NO_EXCEPTIONS  the C++ suite without exceptions too,
#                                       where the compiler turns them off
#           DJINTERP_CLI_EXPECT_COUNTS  hold each test to its count; OFF
#                                       under a configuration knob
#           DJINTERP_CLI_STRICT         the ladder's own diagnostics, as
#                                       errors, where the compiler has them
#           DJINTERP_CLI_SANITIZE       AddressSanitizer and
#                                       UndefinedBehaviorSanitizer, where the
#                                       compiler has them
#
# path:      /build/cmake/config/testing/djinterp/tools/cli/CMakeLists.txt
# link(s):   TBA
# author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.23
#                                                            revised: 2026.10.25
#*******************************************************************************
cmake_minimum_required(VERSION 3.16)

# standing alone, this leaf is the project; added from another, it is not.
# C++ is enabled when there is a compiler for it: without one the C suites
# still run, and the C++ one is left out
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(djinterp_cli_tests LANGUAGES C)
    enable_testing()

    include(CheckLanguage)
    check_language(CXX)

    if(CMAKE_CXX_COMPILER)
        enable_language(CXX)
    endif()
endif()

# a second add_subdirectory() of this leaf would define every target twice
if(TARGET djinterp_cli_testing)
    return()
endif()

# the libraries and the suites are C: an enclosing project that did not
# enable it would otherwise fail far from the cause
get_property(_languages GLOBAL PROPERTY ENABLED_LANGUAGES)

if(NOT "C" IN_LIST _languages)
    message(FATAL_ERROR
            "the command-line kit's leaf needs C: the enclosing project() "
            "must enable it (LANGUAGES C)")
endif()

# the repository root, seven levels above this leaf
get_filename_component(DJINTERP_CLI_ROOT
                       "${CMAKE_CURRENT_LIST_DIR}/../../../../../../.."
                       ABSOLUTE)

set(DJINTERP_CLI_C_LEVELS "99;11;17;23" CACHE STRING
    "C levels the command-line kit's C suites are built and run at")
set(DJINTERP_CLI_CXX_LEVELS "98;11;14;17;20;23" CACHE STRING
    "C++ levels the command-line kit's C++ suite is built and run at")
option(DJINTERP_CLI_NO_EXCEPTIONS
       "Also build the C++ suite without exceptions, lowest and highest"
       ON)
option(DJINTERP_CLI_EXPECT_COUNTS
       "Hold each test to the section count expected_sections.txt gives it"
       ON)
option(DJINTERP_CLI_STRICT
       "Build with the ladder's diagnostics as errors, where there are any"
       ON)
option(DJINTERP_CLI_SANITIZE
       "Build with AddressSanitizer and UndefinedBehaviorSanitizer"
       OFF)

# where the suites are
set(_tests "${DJINTERP_CLI_ROOT}/tests/djinterp/tools/cli")
set(_parser_tests "${DJINTERP_CLI_ROOT}/tests/djinterp/c/modules/parse/cli/arg_parser")


#------------------------------------------------------------------------------
# helpers
#------------------------------------------------------------------------------

# djinterp_cli_flags
#   function: gives a target what the tree's sources need of a compiler, and
# the diagnostics the CI ladder builds with, judged by the compiler of the
# language given -- C, or CXX for the C++ suite, whose C sources take the
# same. Under a cl-style driver that is /utf-8 alone.
function(djinterp_cli_flags _target _language)
    if(MSVC)
        target_compile_options(${_target} PRIVATE /utf-8)

        return()
    endif()

    if(NOT DJINTERP_CLI_STRICT)
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

# djinterp_cli_sanitize
#   function: builds a target with AddressSanitizer and, under GCC and Clang,
# UndefinedBehaviorSanitizer, which then ends the run at its first finding,
# when DJINTERP_CLI_SANITIZE is on.
function(djinterp_cli_sanitize _target)
    if(NOT DJINTERP_CLI_SANITIZE)
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

# djinterp_cli_expect
#   function: holds a test to the number of sections its build has in the
# default configuration, as expected_sections.txt records it under the
# build's name: its level, and `-noexc` for a C++ build without exceptions.
function(djinterp_cli_expect _test _level)
    if(NOT DJINTERP_CLI_EXPECT_COUNTS)
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
            "build's line, or set DJINTERP_CLI_EXPECT_COUNTS=OFF")
endfunction()

# djinterp_cli_run
#   function: runs a suite's executable as a test, given the directory the
# probe modules are in, built before it.
function(djinterp_cli_run _target _key)
    add_dependencies(${_target} ${_probes})
    add_test(NAME ${_target}
             COMMAND ${_target} "$<TARGET_FILE_DIR:prog-probe>")
    djinterp_cli_expect(${_target} "${_key}")
endfunction()

# djinterp_cli_cpp_test
#   function: one build of the C++ suite at a C++ level, run as a test held
# to the count expected_sections.txt gives `_key`; anything after the key
# is a compile option of the C++ sources -- the one that turns exceptions
# off. C++98 without extensions is the framework's strict ISO mode. The
# suite's C sources, the C suites' support and the scratch directories, are
# C99, as the libraries are.
function(djinterp_cli_cpp_test _target _level _key)
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

    target_link_libraries(${_target} PRIVATE djinterp::cli)
    djinterp_cli_flags(${_target} CXX)
    djinterp_cli_sanitize(${_target})
    djinterp_cli_run(${_target} "${_key}")
endfunction()


#------------------------------------------------------------------------------
# the libraries -- C sources, compiled as C at the C floor, position-
# independent so that a module links them; in testing mode, which the
# libraries and every consumer must agree on, so the knobs are PUBLIC
#------------------------------------------------------------------------------
include("${DJINTERP_CLI_ROOT}/build/cmake/config/djinterp/tools/cli/DjinterpCliSources.cmake")

add_library(djinterp_cli_parser_testing STATIC ${DJINTERP_ARG_PARSER_SOURCES})
add_library(djinterp_cli_testing STATIC ${DJINTERP_CLI_SOURCES})

foreach(_library IN ITEMS djinterp_cli_parser_testing djinterp_cli_testing)
    set_target_properties(${_library} PROPERTIES
        C_STANDARD                99
        C_STANDARD_REQUIRED       ON
        C_EXTENSIONS              OFF
        POSITION_INDEPENDENT_CODE ON)

    target_include_directories(${_library}
        PUBLIC "${DJINTERP_CLI_ROOT}/inc")
    target_compile_definitions(${_library}
        PUBLIC D_TESTING=1
               RE_STD_CFG_TESTING=1)

    djinterp_cli_flags(${_library} C)
    djinterp_cli_sanitize(${_library})
endforeach()

# the loader of the modules, where the platform keeps it apart
target_link_libraries(djinterp_cli_testing PUBLIC
    djinterp_cli_parser_testing
    ${CMAKE_DL_LIBS})

# the libraries as a program's modules link them, so that the kit's own
# function makes the probes
add_library(djinterp::arg_parser ALIAS djinterp_cli_parser_testing)
add_library(djinterp::cli ALIAS djinterp_cli_testing)

include("${DJINTERP_CLI_ROOT}/build/cmake/config/djinterp/tools/cli/DjinterpCli.cmake")


#------------------------------------------------------------------------------
# the probe modules -- one source, built once for each kind it names, in the
# order of its D_TESTS_CLI_PROBE_* values
#------------------------------------------------------------------------------
set(_probes "")
set(_kind 1)

foreach(_name IN ITEMS probe other old abi short idle null none kit)
    d_cli_add_plugin(prog-${_name}
                     SOURCES "${_tests}/c/probe/cli_tests_probe.c")

    set_target_properties(prog-${_name} PROPERTIES
        C_STANDARD          99
        C_STANDARD_REQUIRED ON
        C_EXTENSIONS        OFF)

    target_compile_definitions(prog-${_name} PRIVATE
        D_TESTS_CLI_PROBE=${_kind})
    djinterp_cli_flags(prog-${_name} C)
    djinterp_cli_sanitize(prog-${_name})

    list(APPEND _probes prog-${_name})
    math(EXPR _kind "${_kind} + 1")
endforeach()


#------------------------------------------------------------------------------
# the C suites -- the C API called from C, one executable per level, against
# the same libraries
#------------------------------------------------------------------------------
foreach(_level IN LISTS DJINTERP_CLI_C_LEVELS)
    if(NOT "c_std_${_level}" IN_LIST CMAKE_C_COMPILE_FEATURES)
        message(STATUS "djinterp cli: C${_level} left out -- "
                       "${CMAKE_C_COMPILER_ID} ${CMAKE_C_COMPILER_VERSION} "
                       "cannot select it")

        continue()
    endif()

    # a switch for the compiler's latest draft is not a level a count can be
    # held to
    if("${CMAKE_C${_level}_STANDARD_COMPILE_OPTION}" MATCHES "latest")
        message(STATUS "djinterp cli: C${_level} left out -- the "
                       "compiler's switch for it is "
                       "${CMAKE_C${_level}_STANDARD_COMPILE_OPTION}, its "
                       "latest draft and not the level")

        continue()
    endif()

    set(_target djinterp_cli_c_tests_c${_level})

    add_executable(${_target}
        "${_tests}/c/cli_tests_support.c"
        "${_parser_tests}/arg_tests_files.c"
        "${_tests}/c/cli_io_tests.c"
        "${_tests}/c/cli_format_tests.c"
        "${_tests}/c/cli_program_tests.c"
        "${_tests}/c/cli_edit_tests.c"
        "${_tests}/c/cli_operand_tests.c"
        "${_tests}/c/cli_plugin_tests.c"
        "${CMAKE_CURRENT_LIST_DIR}/cli_c_tests_runner.c")

    set_target_properties(${_target} PROPERTIES
        C_STANDARD          ${_level}
        C_STANDARD_REQUIRED ON
        C_EXTENSIONS        OFF)

    target_link_libraries(${_target} PRIVATE djinterp::cli)
    djinterp_cli_flags(${_target} C)
    djinterp_cli_sanitize(${_target})
    djinterp_cli_run(${_target} "c${_level}")
endforeach()


#------------------------------------------------------------------------------
# the allocation sweep -- every allocation of the kit's cases refused in
# turn, at the C floor, where the linker wraps malloc and kin (GNU ld's,
# lld's and MinGW's --wrap): the sweep's wrappers then stand in front of
# every allocation the libraries make
#------------------------------------------------------------------------------
include(CheckCSourceCompiles)

set(CMAKE_REQUIRED_LINK_OPTIONS "-Wl,--wrap=malloc")
check_c_source_compiles("
#include <stddef.h>
void* __real_malloc(size_t _size);
void* __wrap_malloc(size_t _size) { return __real_malloc(_size); }
int main(void) { return 0; }
" DJINTERP_CLI_LINKER_WRAPS)
unset(CMAKE_REQUIRED_LINK_OPTIONS)

if(DJINTERP_CLI_LINKER_WRAPS)
    set(_target djinterp_cli_c_sweep)

    add_executable(${_target}
        "${_tests}/c/cli_sweep_tests.c"
        "${_tests}/c/cli_tests_support.c"
        "${_parser_tests}/arg_tests_files.c"
        "${CMAKE_CURRENT_LIST_DIR}/cli_c_sweep_runner.c")

    set_target_properties(${_target} PROPERTIES
        C_STANDARD          99
        C_STANDARD_REQUIRED ON
        C_EXTENSIONS        OFF)

    target_link_libraries(${_target} PRIVATE djinterp::cli)
    target_link_options(${_target} PRIVATE
        -Wl,--wrap=malloc
        -Wl,--wrap=calloc
        -Wl,--wrap=realloc
        -Wl,--wrap=free)
    djinterp_cli_flags(${_target} C)
    djinterp_cli_sanitize(${_target})
    djinterp_cli_run(${_target} "c99-sweep")
else()
    message(STATUS "djinterp cli: the allocation sweep left out -- the "
                   "linker has no --wrap")
endif()


#------------------------------------------------------------------------------
# the C++ suite -- the C++ face called from C++, one executable per level,
# against the same libraries, across a real language boundary
#------------------------------------------------------------------------------
get_property(_languages GLOBAL PROPERTY ENABLED_LANGUAGES)

if(NOT "CXX" IN_LIST _languages)
    message(STATUS "djinterp cli: the C++ suite left out -- C++ is not "
                   "enabled: there is no C++ compiler, or the enclosing "
                   "project() did not enable it (LANGUAGES C CXX)")

    return()
endif()

set(_cpp_sources
    "${_tests}/cli_tests.cpp"
    # the C suites' support and the scratch directories, which are C
    "${_tests}/c/cli_tests_support.c"
    "${_parser_tests}/arg_tests_files.c"
    "${CMAKE_CURRENT_LIST_DIR}/cli_cpp_tests_runner.cpp")

set(_cpp_built "")

foreach(_level IN LISTS DJINTERP_CLI_CXX_LEVELS)
    if(NOT "cxx_std_${_level}" IN_LIST CMAKE_CXX_COMPILE_FEATURES)
        message(STATUS "djinterp cli: C++${_level} left out -- "
                       "${CMAKE_CXX_COMPILER_ID} "
                       "${CMAKE_CXX_COMPILER_VERSION} cannot select it")

        continue()
    endif()

    # cl's lowest level is C++14, so its C++98 and C++11 builds would be the
    # C++14 one under other names
    if( (MSVC) AND
        (_level LESS 14) )
        message(STATUS "djinterp cli: C++${_level} left out -- the lowest "
                       "level a cl-style driver selects is C++14")

        continue()
    endif()

    if("${CMAKE_CXX${_level}_STANDARD_COMPILE_OPTION}" MATCHES "latest")
        message(STATUS "djinterp cli: C++${_level} left out -- the "
                       "compiler's switch for it is "
                       "${CMAKE_CXX${_level}_STANDARD_COMPILE_OPTION}, its "
                       "latest draft and not the level")

        continue()
    endif()

    djinterp_cli_cpp_test(djinterp_cli_cpp_tests_cxx${_level}
                          ${_level}
                          "c++${_level}")
    list(APPEND _cpp_built ${_level})
endforeach()

# without exceptions: the face's guard compiled out, at the lowest and the
# highest level built, where the compiler has GCC's switch for it
if( (DJINTERP_CLI_NO_EXCEPTIONS) AND
    (_cpp_built) )
    if( (NOT CMAKE_CXX_COMPILER_ID MATCHES "^(GNU|Clang|AppleClang)$") OR
        (CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC") )
        message(STATUS "djinterp cli: the C++ suite without exceptions left "
                       "out -- ${CMAKE_CXX_COMPILER_ID} has no "
                       "-fno-exceptions")
    else()
        list(GET _cpp_built 0 _lowest)
        list(GET _cpp_built -1 _highest)

        foreach(_level IN ITEMS ${_lowest} ${_highest})
            if(TARGET djinterp_cli_cpp_tests_cxx${_level}_noexc)
                continue()
            endif()

            djinterp_cli_cpp_test(djinterp_cli_cpp_tests_cxx${_level}_noexc
                                  ${_level}
                                  "c++${_level}-noexc"
                                  -fno-exceptions)
        endforeach()
    endif()
endif()
