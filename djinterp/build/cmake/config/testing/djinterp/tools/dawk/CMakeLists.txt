#*******************************************************************************
# djinterp [tools]                                                CMakeLists.txt
#
# Test leaf for the checker under tools/dawk: dcheck, which the style gates
# run, and the programs that test it.
#   The CMake twin of tools/build.sh. Builds the checker's units as C, at
# C11, into one static library, links dcheck and the test programs against
# it, and gives CTest the suites: the unit tests, every rule fixture against
# its golden, the fixer against the files it should leave, the driver's
# resolution of a sheet's claims, and the driver over the fixtures against
# the harness's goldens. The four that are scripts in tests/dawk are CMake
# scripts here, beside this file, so that a suite needs CMake and the
# compiler and nothing else: no shell, on any system.
#   Every unit reaches its headers by a path relative to itself, so no target
# here has an include directory at all.
#   NOT BUILT HERE: dawk itself, the interpreter (main.c, dinterp.c and the
# units only it uses). It is not the checker's, and dinterp.c opens a pipe
# with POSIX's popen.
#
#   PLAIN CMAKE, ON PURPOSE, as the parsegen leaves are: the framework's own
# CMake is being rewritten, and a leaf that guessed its helper's signature
# would fail on first configure. The leaf works both ways as it stands:
#     cmake -S <this directory> -B <out> && cmake --build <out> && \
#         ctest --test-dir <out>
#     add_subdirectory(<this directory> <binary directory>)
# Under a generator with several configurations (Visual Studio), the build
# and the tests name one: cmake --build <out> --config Release, and
# ctest --test-dir <out> -C Release.
#   Standing alone, the leaf also installs: cmake --install <out> --prefix
# <root>/build puts dcheck where ci/check_style.sh looks for it.
#
#   cache:  DJINTERP_DAWK_WARNINGS  the warnings tools/build.sh builds with,
#                                   where the compiler has them
#
# path:      /build/cmake/config/testing/djinterp/tools/dawk/CMakeLists.txt
# link(s):   TBA
# author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.10
#                                                            revised: 2026.10.10
#*******************************************************************************
cmake_minimum_required(VERSION 3.16)

# standing alone, this leaf is the project; added from another, it is not
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(djinterp_dawk_tests LANGUAGES C)
    enable_testing()

    set(_standalone TRUE)
else()
    set(_standalone FALSE)
endif()

# a second add_subdirectory() of this leaf would define every target twice
if(TARGET djinterp_dawk_checker)
    return()
endif()

# every unit here is C: an enclosing project that enabled only C++ would
# otherwise fail far from the cause
get_property(_languages GLOBAL PROPERTY ENABLED_LANGUAGES)

if(NOT "C" IN_LIST _languages)
    message(FATAL_ERROR
            "the tools/dawk leaf needs C: the enclosing project() must "
            "enable it (LANGUAGES C)")
endif()

# the repository root, seven levels above this leaf
get_filename_component(DJINTERP_DAWK_ROOT
                       "${CMAKE_CURRENT_LIST_DIR}/../../../../../../.."
                       ABSOLUTE)

option(DJINTERP_DAWK_WARNINGS
       "Build with the warnings tools/build.sh builds with, where there are any"
       ON)

# where the sources, the suites and the sheets are
set(_tool  "${DJINTERP_DAWK_ROOT}/src/djinterp/tools/dawk")
set(_parse "${DJINTERP_DAWK_ROOT}/src/djinterp/parse")
set(_mine  "${DJINTERP_DAWK_ROOT}/tests/dawk")
set(_unit  "${DJINTERP_DAWK_ROOT}/tests/djinterp/tools/dawk")


#------------------------------------------------------------------------------
# helpers
#------------------------------------------------------------------------------

# djinterp_dawk_flags
#   function: gives a target the level its units are written at, and what
# they need of a compiler.
#   The level is C11 with no extensions, as tools/build.sh has it. Under a
# cl-style driver (MSVC, clang-cl) the rest is /utf-8 -- the sources are
# UTF-8 with no byte-order mark, which cl reads in the system code page
# unless told -- and _CRT_SECURE_NO_WARNINGS, without which every fopen and
# getenv is a C4996 and the warnings worth reading are lost among them.
# Under GCC and Clang it is the script's warnings, unless
# DJINTERP_DAWK_WARNINGS is off. On a POSIX system whose C library hides
# POSIX in a strict ISO mode it is _XOPEN_SOURCE=700 as well, as the style
# guide has every build define it: dfs.c reads directories.
function(djinterp_dawk_flags _target)
    set_target_properties(${_target} PROPERTIES
        C_STANDARD          11
        C_STANDARD_REQUIRED ON
        C_EXTENSIONS        OFF)

    if(MSVC)
        target_compile_options(${_target} PRIVATE /utf-8)
        target_compile_definitions(${_target}
            PRIVATE _CRT_SECURE_NO_WARNINGS)

        return()
    endif()

    if(UNIX AND NOT APPLE)
        target_compile_definitions(${_target} PRIVATE _XOPEN_SOURCE=700)
    endif()

    if(NOT DJINTERP_DAWK_WARNINGS)
        return()
    endif()

    if( (NOT CMAKE_C_COMPILER_ID MATCHES "^(GNU|Clang|AppleClang)$")  OR
        (CMAKE_C_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC") )
        return()
    endif()

    target_compile_options(${_target} PRIVATE
        -Wall -Wextra -Wshadow -pedantic)
endfunction()

# djinterp_dawk_program
#   function: one executable of the leaf: its own source, the checker's
# library, and the flags. The target is named for the leaf, so that it
# cannot meet another project's, and the file for the program.
function(djinterp_dawk_program _name _source)
    add_executable(djinterp_dawk_${_name} "${_source}")

    set_target_properties(djinterp_dawk_${_name} PROPERTIES
        OUTPUT_NAME ${_name})

    target_link_libraries(djinterp_dawk_${_name}
        PRIVATE djinterp_dawk_checker)
    djinterp_dawk_flags(djinterp_dawk_${_name})
endfunction()

# djinterp_dawk_script
#   function: one suite that is a CMake script beside this file. It is run
# from the repository root, where a fixture's path is what its golden names
# it by. The script fails with FATAL_ERROR, which CTest reads as its status;
# the fail expression beside it catches one that printed FAIL and went on.
#   CTest runs a target built for another system through the emulator CMake
# was given; a script runs its program itself, so it is told the emulator.
# One program, with no arguments of its own, is what is passed on.
function(djinterp_dawk_script _test _script)
    set(_launcher "")

    if( (CMAKE_CROSSCOMPILING)  AND
        (CMAKE_CROSSCOMPILING_EMULATOR) )
        list(GET CMAKE_CROSSCOMPILING_EMULATOR 0 _launcher)
    endif()

    add_test(NAME ${_test}
             COMMAND "${CMAKE_COMMAND}"
                     "-DROOT=${DJINTERP_DAWK_ROOT}"
                     "-DLAUNCHER=${_launcher}"
                     ${ARGN}
                     -P "${CMAKE_CURRENT_LIST_DIR}/${_script}"
             WORKING_DIRECTORY "${DJINTERP_DAWK_ROOT}")

    set_tests_properties(${_test} PROPERTIES
        FAIL_REGULAR_EXPRESSION "FAIL ")
endfunction()


#------------------------------------------------------------------------------
# the checker's units -- C, at C11, in one library
#------------------------------------------------------------------------------

# the extensions and the lexer they read with, as tools/build.sh takes them:
# whatever those directories hold
file(GLOB _extensions CONFIGURE_DEPENDS
     "${_tool}/ext/*.c")
file(GLOB _lexer CONFIGURE_DEPENDS
     "${_parse}/source/*.c"
     "${_parse}/lex/*.c"
     "${_parse}/lang/c/*.c"
     "${_parse}/lang/cpp/*.c"
     "${_parse}/syntax/*.c")

add_library(djinterp_dawk_checker STATIC
    "${_tool}/daudit.c"
    "${_tool}/dbanner.c"
    "${_tool}/dcascade.c"
    "${_tool}/dedit.c"
    # what the walk asks of the file system: the one unit that is not ISO C
    "${_tool}/dfs.c"
    "${_tool}/dgit.c"
    "${_tool}/dguard.c"
    "${_tool}/dinclude.c"
    "${_tool}/dlayout.c"
    "${_tool}/dline.c"
    "${_tool}/dmatch.c"
    "${_tool}/dnode.c"
    "${_tool}/dproperty.c"
    "${_tool}/drepair.c"
    "${_tool}/dschema.c"
    "${_tool}/dsection.c"
    "${_tool}/dsettings.c"
    "${_tool}/dss.c"
    "${_tool}/dsymbol.c"
    "${_tool}/dtree.c"
    ${_extensions}
    ${_lexer}
    "${_parse}/c/diagnostic.c"
    "${_parse}/c/storage.c")

djinterp_dawk_flags(djinterp_dawk_checker)


#------------------------------------------------------------------------------
# the programs
#------------------------------------------------------------------------------

# the checker
djinterp_dawk_program(dcheck "${_tool}/dcheck.c")

# the harnesses: a sheet over one file, with and without the cascade
djinterp_dawk_program(rules "${_mine}/rules.c")
djinterp_dawk_program(lint  "${_mine}/lint.c")

# the unit tests
djinterp_dawk_program(test_dext      "${_mine}/test_dext.c")
djinterp_dawk_program(test_dfs       "${_mine}/test_dfs.c")
djinterp_dawk_program(test_daudit    "${_unit}/test_daudit.c")
djinterp_dawk_program(test_dselect   "${_unit}/test_dselect.c")
djinterp_dawk_program(test_dsettings "${_unit}/test_dsettings.c")
djinterp_dawk_program(test_dss       "${_unit}/test_dss.c")


#------------------------------------------------------------------------------
# the suites
#------------------------------------------------------------------------------

# a test with a pass expression is judged by its output alone, so the fail
# expression beside it catches a failed check whatever the last line says
foreach(_name IN ITEMS test_dext test_dfs test_dselect)
    add_test(NAME djinterp_dawk_${_name}
             COMMAND djinterp_dawk_${_name}
             WORKING_DIRECTORY "${DJINTERP_DAWK_ROOT}")
    set_tests_properties(djinterp_dawk_${_name} PROPERTIES
        PASS_REGULAR_EXPRESSION "all checks passed"
        FAIL_REGULAR_EXPRESSION "FAIL ")
endforeach()

add_test(NAME djinterp_dawk_test_daudit
         COMMAND djinterp_dawk_test_daudit)
set_tests_properties(djinterp_dawk_test_daudit PROPERTIES
    PASS_REGULAR_EXPRESSION "(^|\n)0 case\\(s\\) reached the wrong verdict"
    FAIL_REGULAR_EXPRESSION "FAIL ")

add_test(NAME djinterp_dawk_test_dsettings
         COMMAND djinterp_dawk_test_dsettings)
set_tests_properties(djinterp_dawk_test_dsettings PROPERTIES
    PASS_REGULAR_EXPRESSION "[1-9][0-9]* checks, 0 failed"
    FAIL_REGULAR_EXPRESSION "FAIL ")

# the sheet parser over every sheet the tree has: each must parse
file(GLOB _sheets CONFIGURE_DEPENDS
     "${DJINTERP_DAWK_ROOT}/sheets/*.dss")

foreach(_sheet IN LISTS _sheets)
    get_filename_component(_stem "${_sheet}" NAME_WE)

    add_test(NAME djinterp_dawk_test_dss_${_stem}
             COMMAND djinterp_dawk_test_dss "${_sheet}")
endforeach()

# every rule fixture against its golden, and every rule against a fixture
djinterp_dawk_script(djinterp_dawk_rules run_rules.cmake
    "-DRULES=$<TARGET_FILE:djinterp_dawk_rules>")

# the fixer against the files it should leave
djinterp_dawk_script(djinterp_dawk_fix run_fix.cmake
    "-DDCHECK=$<TARGET_FILE:djinterp_dawk_dcheck>"
    "-DWORK=${CMAKE_CURRENT_BINARY_DIR}/djinterp_dawk_fix")

# the driver's resolution of a sheet's claims
djinterp_dawk_script(djinterp_dawk_claims run_claims.cmake
    "-DDCHECK=$<TARGET_FILE:djinterp_dawk_dcheck>")

# the driver over the fixtures, against the harness's goldens
djinterp_dawk_script(djinterp_dawk_agree run_agree.cmake
    "-DDCHECK=$<TARGET_FILE:djinterp_dawk_dcheck>")


#------------------------------------------------------------------------------
# installing, standing alone only: an enclosing project has its own rules
#------------------------------------------------------------------------------
if(_standalone)
    install(TARGETS djinterp_dawk_dcheck
                    djinterp_dawk_rules
                    djinterp_dawk_lint
            RUNTIME DESTINATION ".")
endif()
