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
#   The checker reads CMake files too, with dawk's CMake reader, so the leaf
# also tests the reader: alone, over every CMake file under build/cmake, and
# against the CMake running the tests, on every form an argument takes. The
# Visual Studio reader and the documents over it, which dawk's msvs extension
# reads and writes with, are tested here the same way: alone, over every
# Visual Studio file under build/cmake, and with random changes to some. So
# is the wiki's reader, which dawk's wiki extension reads and writes pages
# with: in a library of its own, as it uses no other extension's units, and
# tested alone, over every page the repository holds, and with random
# changes to each -- and, where a MediaWiki is named, against MediaWiki. And
# so are git's units, which dawk's git extension reads git's files and
# repositories with: a library of their own, tested alone, over every file
# of git's and every repository under the tree, with random changes to the
# files -- and, where git and a shell are found, against git.
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
#                                                            revised: 2026.10.20
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
set(_ext   "${_tool}/ext")
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
     "${_tool}/ext/check/*.c")
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
    # the CMake reader dext_cmake reads with, as dext_c reads with the lexer,
    # and the documents over it, tested here
    "${_ext}/cmake/dcmake.c"
    "${_ext}/cmake/dcmake_doc.c"
    "${_tool}/dedit.c"
    # what the walk asks of the file system: the one unit that is not ISO C
    "${_tool}/dfs.c"
    "${_tool}/dgit.c"
    "${_tool}/dguard.c"
    "${_tool}/dinclude.c"
    "${_tool}/dlayout.c"
    "${_tool}/dline.c"
    "${_tool}/dmatch.c"
    # the Visual Studio reader and the documents over it, with dsplice and
    # dxml below: no checker's yet, and tested here as dcmake is
    "${_ext}/msvs/dmsvs.c"
    "${_ext}/msvs/dmsvs_doc.c"
    "${_tool}/dnode.c"
    "${_tool}/dproperty.c"
    "${_tool}/drepair.c"
    "${_tool}/dschema.c"
    "${_tool}/dsection.c"
    "${_tool}/dsettings.c"
    "${_tool}/dsplice.c"
    "${_tool}/dss.c"
    "${_tool}/dsymbol.c"
    "${_tool}/dtree.c"
    "${_tool}/dxml.c"
    ${_extensions}
    ${_lexer}
    "${_parse}/c/diagnostic.c"
    "${_parse}/c/storage.c")

djinterp_dawk_flags(djinterp_dawk_checker)


#------------------------------------------------------------------------------
# the wiki's units -- C, at C11, in a library of their own
#------------------------------------------------------------------------------

# the reader, its two dialects, and the documents over it: no unit of the
# checker's or of another extension's is among them, nor needed by them
add_library(djinterp_dawk_wiki STATIC
    "${_ext}/wiki/dwiki.c"
    "${_ext}/wiki/dwiki_markdown.c"
    "${_ext}/wiki/dwiki_mediawiki.c"
    "${_ext}/wiki/dwiki_doc.c")

djinterp_dawk_flags(djinterp_dawk_wiki)


#------------------------------------------------------------------------------
# Ninja's units -- C, at C11, in a library of their own
#------------------------------------------------------------------------------

# the reader, the loader that makes a build of a manifest as ninja does, and
# the documents over the reader, with the splice and the walk they save
# with; no unit of the checker's or of another extension's is among them
add_library(djinterp_dawk_ninja STATIC
    "${_ext}/ninja/dninja.c"
    "${_ext}/ninja/dninja_eval.c"
    "${_ext}/ninja/dninja_doc.c"
    "${_tool}/dsplice.c"
    "${_tool}/dfs.c")

djinterp_dawk_flags(djinterp_dawk_ninja)


#------------------------------------------------------------------------------
# git's units -- C, at C11, in a library of their own
#------------------------------------------------------------------------------

# git's files read and changed, and a repository read: its own file access,
# hash, inflate, objects, refs, history and index; no unit of the checker's
# or of another extension's is among them, but the walk dfs.c gives
add_library(djinterp_dawk_git STATIC
    "${_ext}/git/dgit_text.c"
    "${_ext}/git/dgit_doc.c"
    "${_ext}/git/dgit_file.c"
    "${_ext}/git/dgit_hash.c"
    "${_ext}/git/dgit_inflate.c"
    "${_ext}/git/dgit_object.c"
    "${_ext}/git/dgit_repo.c"
    "${_ext}/git/dgit_index.c"
    "${_tool}/dfs.c")

djinterp_dawk_flags(djinterp_dawk_git)


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
djinterp_dawk_program(test_dcmake    "${_unit}/ext/cmake/test_dcmake.c")
djinterp_dawk_program(test_dcmake_doc "${_unit}/ext/cmake/test_dcmake_doc.c")
djinterp_dawk_program(test_dext_cmake "${_unit}/ext/check/test_dext_cmake.c")
djinterp_dawk_program(test_dmsvs     "${_unit}/ext/msvs/test_dmsvs.c")
djinterp_dawk_program(test_dmsvs_doc "${_unit}/ext/msvs/test_dmsvs_doc.c")

# the wiki's test, with the wiki's library and the walk dfs.c gives it
add_executable(djinterp_dawk_test_dwiki
    "${_unit}/ext/wiki/test_dwiki.c"
    "${_tool}/dfs.c")

set_target_properties(djinterp_dawk_test_dwiki PROPERTIES
    OUTPUT_NAME test_dwiki)

target_link_libraries(djinterp_dawk_test_dwiki PRIVATE djinterp_dawk_wiki)
djinterp_dawk_flags(djinterp_dawk_test_dwiki)

# Ninja's test, with Ninja's library
add_executable(djinterp_dawk_test_dninja "${_unit}/ext/ninja/test_dninja.c")

set_target_properties(djinterp_dawk_test_dninja PROPERTIES
    OUTPUT_NAME test_dninja)

target_link_libraries(djinterp_dawk_test_dninja PRIVATE djinterp_dawk_ninja)
djinterp_dawk_flags(djinterp_dawk_test_dninja)

# git's test, with git's library
add_executable(djinterp_dawk_test_dgit "${_unit}/ext/git/test_dgit.c")

set_target_properties(djinterp_dawk_test_dgit PROPERTIES
    OUTPUT_NAME test_dgit)

target_link_libraries(djinterp_dawk_test_dgit PRIVATE djinterp_dawk_git)
djinterp_dawk_flags(djinterp_dawk_test_dgit)


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

# the CMake reader and the extension over it; the extension's test writes
# a small tree of its own beneath the build directory
add_test(NAME djinterp_dawk_test_dcmake
         COMMAND djinterp_dawk_test_dcmake)
add_test(NAME djinterp_dawk_test_dext_cmake
         COMMAND djinterp_dawk_test_dext_cmake "${CMAKE_CURRENT_BINARY_DIR}")
set_tests_properties(djinterp_dawk_test_dcmake djinterp_dawk_test_dext_cmake
    PROPERTIES
    PASS_REGULAR_EXPRESSION "[1-9][0-9]* checks, 0 failed"
    FAIL_REGULAR_EXPRESSION "FAIL ")

# the documents over the reader; the test saves beneath the build directory
add_test(NAME djinterp_dawk_test_dcmake_doc
         COMMAND djinterp_dawk_test_dcmake_doc "${CMAKE_CURRENT_BINARY_DIR}")
set_tests_properties(djinterp_dawk_test_dcmake_doc PROPERTIES
    PASS_REGULAR_EXPRESSION "[1-9][0-9]* checks, 0 failed"
    FAIL_REGULAR_EXPRESSION "FAIL ")

# the reader over every CMake file the repository has under build/cmake, each
# read whole, and against CMake itself on every form an argument takes; and
# random changes to each, made in memory and each proved to touch only what
# it said
add_test(NAME djinterp_dawk_cmake_corpus
         COMMAND djinterp_dawk_test_dcmake --corpus
                 "${DJINTERP_DAWK_ROOT}/build/cmake")
add_test(NAME djinterp_dawk_cmake_fuzz
         COMMAND djinterp_dawk_test_dcmake_doc --fuzz
                 "${DJINTERP_DAWK_ROOT}/build/cmake" 1 40)
set_tests_properties(djinterp_dawk_cmake_corpus djinterp_dawk_cmake_fuzz
    PROPERTIES
    PASS_REGULAR_EXPRESSION " 0 failed"
    FAIL_REGULAR_EXPRESSION "FAIL ")

# the Visual Studio reader and its documents; the documents' test writes
# beneath the build directory
add_test(NAME djinterp_dawk_test_dmsvs
         COMMAND djinterp_dawk_test_dmsvs)
add_test(NAME djinterp_dawk_test_dmsvs_doc
         COMMAND djinterp_dawk_test_dmsvs_doc "${CMAKE_CURRENT_BINARY_DIR}")
set_tests_properties(djinterp_dawk_test_dmsvs djinterp_dawk_test_dmsvs_doc
    PROPERTIES
    PASS_REGULAR_EXPRESSION "[1-9][0-9]* checks, 0 failed"
    FAIL_REGULAR_EXPRESSION "FAIL ")

# the reader over every Visual Studio file under build/cmake, each read
# whole; and, where CMake has generated a Visual Studio build there, random
# changes to its tests' projects, each proved to touch only what it said
add_test(NAME djinterp_dawk_msvs_corpus
         COMMAND djinterp_dawk_test_dmsvs --corpus
                 "${DJINTERP_DAWK_ROOT}/build/cmake")
set_tests_properties(djinterp_dawk_msvs_corpus PROPERTIES
    PASS_REGULAR_EXPRESSION " 0 failed"
    FAIL_REGULAR_EXPRESSION "FAIL ")

if(IS_DIRECTORY "${DJINTERP_DAWK_ROOT}/build/cmake/msvs")
    add_test(NAME djinterp_dawk_msvs_fuzz
             COMMAND djinterp_dawk_test_dmsvs_doc --fuzz 1
                     "${DJINTERP_DAWK_ROOT}/build/cmake/msvs")
    set_tests_properties(djinterp_dawk_msvs_fuzz PROPERTIES
        PASS_REGULAR_EXPRESSION " 0 failed"
        FAIL_REGULAR_EXPRESSION "FAIL ")
endif()

# the wiki's reader and documents; the test saves beneath the build directory
add_test(NAME djinterp_dawk_test_dwiki
         COMMAND djinterp_dawk_test_dwiki "${CMAKE_CURRENT_BINARY_DIR}")
set_tests_properties(djinterp_dawk_test_dwiki PROPERTIES
    PASS_REGULAR_EXPRESSION "[1-9][0-9]* checks, 0 failed"
    FAIL_REGULAR_EXPRESSION "FAIL ")

# every page the repository holds, read whole and proved sound; and random
# changes to each, made in memory and each proved to touch only what it said
add_test(NAME djinterp_dawk_wiki_corpus
         COMMAND djinterp_dawk_test_dwiki --corpus "${DJINTERP_DAWK_ROOT}")
add_test(NAME djinterp_dawk_wiki_fuzz
         COMMAND djinterp_dawk_test_dwiki --fuzz 1 "${DJINTERP_DAWK_ROOT}")
set_tests_properties(djinterp_dawk_wiki_corpus djinterp_dawk_wiki_fuzz
    PROPERTIES
    PASS_REGULAR_EXPRESSION " 0 failed"
    FAIL_REGULAR_EXPRESSION "FAIL ")

# Ninja's units; the test loads builds and saves beneath the build directory
add_test(NAME djinterp_dawk_test_dninja
         COMMAND djinterp_dawk_test_dninja "${CMAKE_CURRENT_BINARY_DIR}")
set_tests_properties(djinterp_dawk_test_dninja PROPERTIES
    PASS_REGULAR_EXPRESSION "[1-9][0-9]* checks, 0 failed"
    FAIL_REGULAR_EXPRESSION "FAIL ")

# random changes to every manifest among ninja.sh's cases, made in memory
# and each proved to touch only what it said; ninja.sh itself needs dawk,
# which is not built here
add_test(NAME djinterp_dawk_ninja_fuzz
         COMMAND djinterp_dawk_test_dninja --fuzz
                 "${_unit}/ext/ninja/cases" 1 200)
set_tests_properties(djinterp_dawk_ninja_fuzz PROPERTIES
    PASS_REGULAR_EXPRESSION " 0 failed"
    FAIL_REGULAR_EXPRESSION "FAIL ")

# git's units; the test builds its repositories beneath the build directory
add_test(NAME djinterp_dawk_test_dgit
         COMMAND djinterp_dawk_test_dgit "${CMAKE_CURRENT_BINARY_DIR}")
set_tests_properties(djinterp_dawk_test_dgit PROPERTIES
    PASS_REGULAR_EXPRESSION "[1-9][0-9]* checks, 0 failed"
    FAIL_REGULAR_EXPRESSION "FAIL ")

# every file of git's the tree holds, and the repository it is, if it is
# one, read and proved sound; and random changes to the files, in memory
add_test(NAME djinterp_dawk_git_corpus
         COMMAND djinterp_dawk_test_dgit --corpus "${DJINTERP_DAWK_ROOT}")
add_test(NAME djinterp_dawk_git_fuzz
         COMMAND djinterp_dawk_test_dgit --fuzz 1 "${DJINTERP_DAWK_ROOT}")
set_tests_properties(djinterp_dawk_git_corpus djinterp_dawk_git_fuzz
    PROPERTIES
    PASS_REGULAR_EXPRESSION " 0 failed"
    FAIL_REGULAR_EXPRESSION "FAIL ")

# git's units against git itself, over repositories git makes: only where
# git and a shell to run git.sh are found; dawk is not built here, so its
# extension's part of the suite is not run
find_program(DJINTERP_DAWK_GIT git)
find_program(DJINTERP_DAWK_SH sh)

if(DJINTERP_DAWK_GIT AND DJINTERP_DAWK_SH)
    add_test(NAME djinterp_dawk_git_oracle
             COMMAND "${DJINTERP_DAWK_SH}"
                     "${_unit}/ext/git/git.sh"
                     none "$<TARGET_FILE:djinterp_dawk_test_dgit>"
             WORKING_DIRECTORY "${DJINTERP_DAWK_ROOT}")
    set_tests_properties(djinterp_dawk_git_oracle PROPERTIES
        PASS_REGULAR_EXPRESSION "git: [0-9]+ passed, 0 failed"
        FAIL_REGULAR_EXPRESSION "FAIL ")
endif()

# the MediaWiki reader against MediaWiki itself, over MediaWiki's own parser
# tests: only where a MediaWiki 1.43 is named, set up as
# tests/.../wiki_mw_records.php says, with php and python3 to run it
set(DJINTERP_DAWK_MEDIAWIKI "$ENV{MW_INSTALL_PATH}" CACHE PATH
    "a MediaWiki 1.43 to read the wiki's MediaWiki pages against")
find_program(DJINTERP_DAWK_PHP php)
find_package(Python3 COMPONENTS Interpreter QUIET)

if(DJINTERP_DAWK_MEDIAWIKI AND DJINTERP_DAWK_PHP AND Python3_Interpreter_FOUND)
    add_test(NAME djinterp_dawk_wiki_mediawiki
             COMMAND "${Python3_EXECUTABLE}" -I
                     "${_unit}/ext/wiki/wiki_oracle.py"
                     mediawiki "$<TARGET_FILE:djinterp_dawk_test_dwiki>"
                     -parser-tests)
    set_tests_properties(djinterp_dawk_wiki_mediawiki PROPERTIES
        ENVIRONMENT "MW_INSTALL_PATH=${DJINTERP_DAWK_MEDIAWIKI}"
        FAIL_REGULAR_EXPRESSION "DIFFERENT |NOT READ ")
endif()

djinterp_dawk_script(djinterp_dawk_cmake_values run_cmake_values.cmake
    "-DTEST_DCMAKE=$<TARGET_FILE:djinterp_dawk_test_dcmake>"
    "-DWORK=${CMAKE_CURRENT_BINARY_DIR}/djinterp_dawk_cmake_values")

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
