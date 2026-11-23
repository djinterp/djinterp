#*******************************************************************************
# djinterp [tools]                                        run_clang_tokens.cmake
#
# Parse's lexer, which the fixer and its two safety checks read with, against
# Clang's raw lexer, over every C and C++ file under inc, src and tests, the
# checks' own pairs among them (dawk's decision 45a):
# tests/dawk/clang_tokens.sh as a CMake script, for a system with no shell.
#   Each file is lexed both ways -- Clang's `-Xclang -dump-raw-tokens`, then
# `dcheck --clang-tokens=<that dump>`, which lexes it again -- and the two
# must agree token for token: name, spelling, line and column. A file on
# tests/dawk/clang_tokens_known.txt may disagree, and one there that agrees
# must come off it. A generated file is left out, as dcheck leaves it out:
# the fixer never reads one. The leaf registers this suite only where it
# finds Clang.
#
#   usage:  cmake -DDCHECK=<the checker> -DCLANG=<clang> -DROOT=<repository
#                 root> -DWORK=<a directory for the dumps>
#                 [-DLAUNCHER=<a program to run it with>]
#                 -P run_clang_tokens.cmake
#           LAUNCHER is for a build made for another system: what runs its
#           programs on this one; Clang runs here
#   exit:   0 when every file agrees or is listed; otherwise an error, after
#           every failure has been printed
#
# path:      /build/cmake/config/testing/djinterp/tools/dawk/run_clang_tokens.cmake
# link(s):   TBA
# author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.19
#                                                            revised: 2026.11.19
#*******************************************************************************
cmake_minimum_required(VERSION 3.16)

if( (NOT DEFINED DCHECK)  OR
    (NOT DEFINED CLANG)   OR
    (NOT DEFINED ROOT)    OR
    (NOT DEFINED WORK) )
    message(FATAL_ERROR
            "usage: cmake -DDCHECK=<the checker> -DCLANG=<clang> "
            "-DROOT=<repository root> -DWORK=<a directory> "
            "-P run_clang_tokens.cmake")
endif()

if(NOT EXISTS "${DCHECK}")
    message(FATAL_ERROR "run_clang_tokens: no program at ${DCHECK}")
endif()

# a build for another system is run through what runs it here
set(_run "${DCHECK}")

if( (DEFINED LAUNCHER)  AND
    (NOT "${LAUNCHER}" STREQUAL "") )
    set(_run "${LAUNCHER}" "${DCHECK}")
endif()

# the known paths: the first field of each line that is not a comment
file(STRINGS "${ROOT}/tests/dawk/clang_tokens_known.txt" _lines)
set(_known "")

foreach(_line IN LISTS _lines)
    string(REPLACE "\r" "" _line "${_line}")

    if( (_line STREQUAL "")  OR
        (_line MATCHES "^#") )
        continue()
    endif()

    string(REGEX REPLACE "\t.*$" "" _path "${_line}")
    list(APPEND _known "${_path}")
endforeach()

# every C and C++ file, as dcheck's extensions name them
set(_patterns "")

foreach(_top IN ITEMS inc src tests)
    foreach(_ext IN ITEMS c h cpp hpp cc hh cxx hxx ipp tpp)
        list(APPEND _patterns "${ROOT}/${_top}/*.${_ext}")
    endforeach()
endforeach()

file(GLOB_RECURSE _files RELATIVE "${ROOT}" ${_patterns})
list(SORT _files)
file(MAKE_DIRECTORY "${WORK}")

set(_failed    0)
set(_disagree  0)
set(_generated 0)

foreach(_file IN LISTS _files)
    # dcheck's mark of a generator's file, in its first 4 KiB
    file(READ "${ROOT}/${_file}" _head LIMIT 4096)
    string(FIND "${_head}" "Auto-generated. Do not edit by hand." _marked)

    if(NOT _marked EQUAL -1)
        math(EXPR _generated "${_generated} + 1")

        continue()
    endif()

    if(_file MATCHES "\\.(cpp|hpp|cc|hh|cxx|hxx|ipp|tpp)$")
        set(_lang -x c++ -std=c++2b)
    else()
        set(_lang -x c -std=c2x)
    endif()

    execute_process(
        COMMAND "${CLANG}" ${_lang} -fsyntax-only -Xclang -dump-raw-tokens
                "${_file}"
        WORKING_DIRECTORY "${ROOT}"
        OUTPUT_QUIET
        ERROR_FILE        "${WORK}/dump")
    execute_process(
        COMMAND ${_run} "--clang-tokens=${WORK}/dump" "${_file}"
        WORKING_DIRECTORY "${ROOT}"
        OUTPUT_VARIABLE   _said
        ERROR_VARIABLE    _said
        RESULT_VARIABLE   _status)
    list(FIND _known "${_file}" _listed)

    if( (_status EQUAL 1)  AND
        (NOT _listed EQUAL -1) )
        math(EXPR _disagree "${_disagree} + 1")
    elseif(_status EQUAL 1)
        message("FAIL clang_tokens: ${_said}")

        math(EXPR _failed "${_failed} + 1")
    elseif( (_status EQUAL 0)  AND
            (NOT _listed EQUAL -1) )
        message("FAIL clang_tokens: ${_file} agrees now; take it off "
                "tests/dawk/clang_tokens_known.txt")

        math(EXPR _failed "${_failed} + 1")
    elseif(NOT _status EQUAL 0)
        message("FAIL clang_tokens: ${_file}: ${_said}")

        math(EXPR _failed "${_failed} + 1")
    endif()
endforeach()

if(NOT _failed EQUAL 0)
    message(FATAL_ERROR "clang_tokens: ${_failed} check(s) did not pass")
endif()

list(LENGTH _files _checked)
math(EXPR _checked "${_checked} - ${_generated}")
math(EXPR _agree "${_checked} - ${_disagree}")
message("clang_tokens: ${_checked} files lexed both ways (${_generated} "
        "generated left out); ${_agree} agree, and the ${_disagree} that "
        "don't are listed")
