#*******************************************************************************
# djinterp [tools]                                         run_moved_lines.cmake
#
# dcheck --moved-lines, which reads where a fixer put what it moved, against
# pairs it must pass and pairs it must not: tests/dawk/moved_lines.sh as a
# CMake script, for a system with no shell.
#   tests/dawk/moved_lines/right holds each repair of layout made as it
# should be, and tests/dawk/moved_lines/wrong the same made wrongly, each
# with <name>.says, what the check must say of it, line and all. Every pair
# holds the same tokens before and after, which is checked too.
#
#   usage:  cmake -DDCHECK=<the checker> -DROOT=<repository root>
#                 [-DLAUNCHER=<a program to run it with>]
#                 -P run_moved_lines.cmake
#           LAUNCHER is for a build made for another system: what runs its
#           programs on this one
#   exit:   0 when every pair passes or is refused for what it is; otherwise an
#           error, after every failure has been printed
#
# path:      /build/cmake/config/testing/djinterp/tools/dawk/run_moved_lines.cmake
# link(s):   TBA
# author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.19
#                                                            revised: 2026.11.19
#*******************************************************************************
cmake_minimum_required(VERSION 3.16)

if( (NOT DEFINED DCHECK)  OR
    (NOT DEFINED ROOT) )
    message(FATAL_ERROR
            "usage: cmake -DDCHECK=<the checker> -DROOT=<repository root> "
            "-P run_moved_lines.cmake")
endif()

if(NOT EXISTS "${DCHECK}")
    message(FATAL_ERROR "run_moved_lines: no program at ${DCHECK}")
endif()

# a build for another system is run through what runs it here
set(_run "${DCHECK}")

if( (DEFINED LAUNCHER)  AND
    (NOT "${LAUNCHER}" STREQUAL "") )
    set(_run "${LAUNCHER}" "${DCHECK}")
endif()

set(_failed 0)

# d_internal_check
#   macro: runs `dcheck --<check> <before> <after>`, into _status and _said.
macro(d_internal_check _check _before _after)
    execute_process(
        COMMAND ${_run} "--${_check}" "${_before}" "${_after}"
        WORKING_DIRECTORY "${ROOT}"
        OUTPUT_VARIABLE   _said
        ERROR_VARIABLE    _said
        RESULT_VARIABLE   _status)
endmacro()

file(GLOB _right RELATIVE "${ROOT}"
     "${ROOT}/tests/dawk/moved_lines/right/*.before.*")
file(GLOB _wrong RELATIVE "${ROOT}"
     "${ROOT}/tests/dawk/moved_lines/wrong/*.before.*")
set(_caught 0)

foreach(_before IN LISTS _right _wrong)
    string(REPLACE ".before." ".after." _after "${_before}")
    d_internal_check(same-tokens "${_before}" "${_after}")

    if(NOT _status EQUAL 0)
        message("FAIL moved_lines: ${_before} and its after do not hold the "
                "same tokens\n${_said}")

        math(EXPR _failed "${_failed} + 1")
    endif()
endforeach()

foreach(_before IN LISTS _right)
    string(REPLACE ".before." ".after." _after "${_before}")
    d_internal_check(moved-lines "${_before}" "${_after}")

    if(NOT _status EQUAL 0)
        message("FAIL moved_lines: ${_before} was moved rightly, and is not "
                "passed\n${_said}")

        math(EXPR _failed "${_failed} + 1")
    endif()
endforeach()

foreach(_before IN LISTS _wrong)
    string(REPLACE ".before." ".after." _after "${_before}")
    string(REGEX REPLACE "\\.before\\..*$" ".says" _says_file "${_before}")
    file(READ "${ROOT}/${_says_file}" _says)
    string(REPLACE "\r" "" _says "${_says}")
    string(STRIP "${_says}" _says)
    d_internal_check(moved-lines "${_before}" "${_after}")
    string(FIND "${_said}" "${_says}" _found)

    if( (_status EQUAL 1)  AND
        (NOT _found EQUAL -1) )
        math(EXPR _caught "${_caught} + 1")
    else()
        message("FAIL moved_lines: ${_before} is not refused for what it is"
                "\n${_said}    expected: ${_says}")

        math(EXPR _failed "${_failed} + 1")
    endif()
endforeach()

if(NOT _failed EQUAL 0)
    message(FATAL_ERROR "moved_lines: ${_failed} check(s) did not pass")
endif()

list(LENGTH _right _passed)
list(LENGTH _wrong _pairs)
message("moved_lines: ${_passed} pairs moved rightly pass; ${_caught} of "
        "${_pairs} moved wrongly are caught, each for what it is")
