#*******************************************************************************
# djinterp [parsegen]                                             CMakeLists.txt
#
# Test leaf for the parse/parsegen foundation.
#   Builds the foundation's C sources as C, at the C floor, into one static
# library in the testing configuration, then links the suites against it once
# per language level: the C++ suites at every level from the module floor
# (C++11) up, and the C suites at every level from the C floor (C99) up. The
# library is C whatever its consumer is, which is the boundary the foundation
# exists to serve. A level the compiler cannot select is left out, and the
# configure step says so.
#   A suite is a table of sections, and a section is what a level or a knob
# removes, so the number of sections run differs by level, by design.
# expected_sections.txt holds the count each level has in the default
# configuration, and every test here must print exactly that: a section that
# silently stopped being compiled is a failure, not a smaller pass.
#   The suites and the runners reach every header by a path relative to
# themselves, so no target here has an include directory but inc/.
#
#   PLAIN CMAKE, ON PURPOSE. This does not call djinterp_add_test_executable:
# that helper belongs to the framework's CMake, which is being rewritten, and
# a leaf that guessed its signature would fail on first configure. Everything
# here is standard CMake, so the leaf works both ways as it stands:
#     cmake -S <this directory> -B <out> && cmake --build <out> && \
#         ctest --test-dir <out>
#     add_subdirectory(<this directory> <binary directory>)
#
#   cache:  DJINTERP_FOUNDATION_CXX_LEVELS    C++ levels to build the suites at
#           DJINTERP_FOUNDATION_C_LEVELS      C levels to build the suites at
#           DJINTERP_FOUNDATION_EXPECT_COUNTS hold each test to its count; OFF
#                                             under a configuration knob
#           DJINTERP_FOUNDATION_STRICT        the ladder's own diagnostics, as
#                                             errors, where the compiler has
#                                             them
#
# path:      /build/cmake/config/testing/djinterp/parsegen/foundation/CMakeLists.txt
# link(s):   TBA
# author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.19
#                                                            revised: 2026.10.06
#*******************************************************************************
cmake_minimum_required(VERSION 3.16)

# standing alone, this leaf is the project; added from another, it is not
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(djinterp_foundation_tests LANGUAGES C CXX)
    enable_testing()
endif()

# a second add_subdirectory() of this leaf would define every target twice
if(TARGET djinterp_foundation_testing)
    return()
endif()

# the library is C and the suites are both: an enclosing project that enabled
# only one of them would otherwise fail far from the cause
get_property(_languages GLOBAL PROPERTY ENABLED_LANGUAGES)

if( (NOT "C" IN_LIST _languages)    OR
    (NOT "CXX" IN_LIST _languages) )
    message(FATAL_ERROR
            "the parse/parsegen foundation leaf needs both C and CXX: the "
            "enclosing project() must enable the two (LANGUAGES C CXX)")
endif()

# the repository root, seven levels above this leaf
get_filename_component(DJINTERP_FOUNDATION_ROOT
                       "${CMAKE_CURRENT_LIST_DIR}/../../../../../../.."
                       ABSOLUTE)

set(DJINTERP_FOUNDATION_CXX_LEVELS "11;14;17;20;23" CACHE STRING
    "C++ levels the foundation's C++ suites are built and run at")
set(DJINTERP_FOUNDATION_C_LEVELS "99;11;17;23" CACHE STRING
    "C levels the foundation's C suites are built and run at")
option(DJINTERP_FOUNDATION_EXPECT_COUNTS
       "Hold each test to the section count expected_sections.txt gives it"
       ON)
option(DJINTERP_FOUNDATION_STRICT
       "Build with the ladder's diagnostics as errors, where there are any"
       ON)

# where the sources and the suites are
set(_src   "${DJINTERP_FOUNDATION_ROOT}/src/djinterp")
set(_tests "${DJINTERP_FOUNDATION_ROOT}/tests/djinterp")


#------------------------------------------------------------------------------
# helpers
#------------------------------------------------------------------------------

# djinterp_foundation_flags
#   function: gives a target what the tree's sources need of a compiler, and
# the diagnostics the CI ladder builds with.
#   Under a cl-style driver (MSVC, clang-cl) that is /utf-8 and nothing more:
# the sources are UTF-8 with no byte-order mark, which cl reads in the system
# code page unless told, and the ladder's flags mean other things there --
# /Wall is every warning the compiler has -- so those builds otherwise keep
# their defaults. Under GCC and Clang it is the ladder's flags, with the
# usual warnings beside them, unless DJINTERP_FOUNDATION_STRICT is off.
function(djinterp_foundation_flags _target _language)
    if(MSVC)
        target_compile_options(${_target} PRIVATE /utf-8)

        return()
    endif()

    if(NOT DJINTERP_FOUNDATION_STRICT)
        return()
    endif()

    set(_compiler "${CMAKE_${_language}_COMPILER_ID}")
    set(_frontend "${CMAKE_${_language}_COMPILER_FRONTEND_VARIANT}")

    if( (NOT _compiler MATCHES "^(GNU|Clang|AppleClang)$")  OR
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

# djinterp_foundation_expect
#   function: holds a test to the number of sections its level has in the
# default configuration, as expected_sections.txt records it. A test with a
# pass expression is judged by its output alone, so the fail expression
# beside it catches a failed section whatever the total printed.
function(djinterp_foundation_expect _test _level)
    if(NOT DJINTERP_FOUNDATION_EXPECT_COUNTS)
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
            "level's line, or set DJINTERP_FOUNDATION_EXPECT_COUNTS=OFF")
endfunction()


#------------------------------------------------------------------------------
# the foundation library -- C sources, compiled as C at the C floor
#------------------------------------------------------------------------------
add_library(djinterp_foundation_testing STATIC
    "${_src}/parse/c/charset.c"
    "${_src}/parse/c/diagnostic.c"
    "${_src}/parse/c/machine.c"
    "${_src}/parse/c/pool.c"
    "${_src}/parse/c/program.c"
    "${_src}/parse/c/storage.c"
    "${_src}/parsegen/c/analysis.c"
    "${_src}/parsegen/c/feature.c"
    "${_src}/parsegen/c/grammar.c"
    "${_src}/parsegen/c/registry.c"
    # re_std's one C source: what the fixed-width integer header declares
    "${_src}/c/re_std/dstdint.c")

set_target_properties(djinterp_foundation_testing PROPERTIES
    C_STANDARD          99
    C_STANDARD_REQUIRED ON
    C_EXTENSIONS        OFF)

target_include_directories(djinterp_foundation_testing
    PUBLIC "${DJINTERP_FOUNDATION_ROOT}/inc")

# PUBLIC, not PRIVATE: testing mode switches the machine's trace hook on,
# which changes the layout of struct d_parse_machine. The library and every
# consumer must agree on that layout, so they must agree on these. re_std's
# knob is set beside djinterp's so that neither depends on include order.
target_compile_definitions(djinterp_foundation_testing
    PUBLIC D_TESTING=1
           RE_STD_CFG_TESTING=1)

djinterp_foundation_flags(djinterp_foundation_testing C)


#------------------------------------------------------------------------------
# the C++ suites -- one executable per level, linked against the C library
# across a real language boundary
#------------------------------------------------------------------------------
foreach(_level IN LISTS DJINTERP_FOUNDATION_CXX_LEVELS)
    if(NOT "cxx_std_${_level}" IN_LIST CMAKE_CXX_COMPILE_FEATURES)
        message(STATUS "djinterp foundation: C++${_level} left out -- "
                       "${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION} "
                       "cannot select it")

        continue()
    endif()

    # cl's lowest level is C++14, so its C++11 build would be the C++14 one
    # under another name
    if( (MSVC)             AND
        (_level EQUAL 11) )
        message(STATUS "djinterp foundation: C++11 left out -- the lowest "
                       "level a cl-style driver selects is C++14")

        continue()
    endif()

    set(_target djinterp_foundation_tests_cxx${_level})

    add_executable(${_target}
        "${_tests}/parse/parse_substrate_tests.cpp"
        "${_tests}/parse/parse_program_tests.cpp"
        "${_tests}/parsegen/parsegen_registry_tests.cpp"
        "${_tests}/parsegen/parsegen_grammar_tests.cpp"
        "${_tests}/parsegen/parsegen_analysis_tests.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/foundation_tests_runner.cpp")

    set_target_properties(${_target} PROPERTIES
        CXX_STANDARD          ${_level}
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS        OFF)

    target_link_libraries(${_target} PRIVATE djinterp_foundation_testing)
    djinterp_foundation_flags(${_target} CXX)

    add_test(NAME ${_target} COMMAND ${_target})
    djinterp_foundation_expect(${_target} "c++${_level}")
endforeach()


#------------------------------------------------------------------------------
# the C suites -- the C API called from C, one executable per level, against
# the same library
#------------------------------------------------------------------------------
foreach(_level IN LISTS DJINTERP_FOUNDATION_C_LEVELS)
    if(NOT "c_std_${_level}" IN_LIST CMAKE_C_COMPILE_FEATURES)
        message(STATUS "djinterp foundation: C${_level} left out -- "
                       "${CMAKE_C_COMPILER_ID} ${CMAKE_C_COMPILER_VERSION} "
                       "cannot select it")

        continue()
    endif()

    # cl has no switch for a C level past C17. Where CMake gives it one for
    # C23 the switch is /std:clatest -- whatever draft that compiler has,
    # which moves with the compiler -- and a draft is not a level a count
    # can be held to
    if("${CMAKE_C${_level}_STANDARD_COMPILE_OPTION}" MATCHES "latest")
        message(STATUS "djinterp foundation: C${_level} left out -- the "
                       "compiler's switch for it is "
                       "${CMAKE_C${_level}_STANDARD_COMPILE_OPTION}, its "
                       "latest draft and not the level")

        continue()
    endif()

    set(_target djinterp_foundation_c_tests_c${_level})

    add_executable(${_target}
        "${_tests}/parse/c/parse_c_tests.c"
        "${_tests}/parsegen/c/parsegen_c_tests.c"
        "${CMAKE_CURRENT_LIST_DIR}/foundation_c_tests_runner.c")

    set_target_properties(${_target} PROPERTIES
        C_STANDARD          ${_level}
        C_STANDARD_REQUIRED ON
        C_EXTENSIONS        OFF)

    target_link_libraries(${_target} PRIVATE djinterp_foundation_testing)
    djinterp_foundation_flags(${_target} C)

    add_test(NAME ${_target} COMMAND ${_target})
    djinterp_foundation_expect(${_target} "c${_level}")
endforeach()
