#*******************************************************************************
# djinterp [tools]                                              run_claims.cmake
#
# The driver's resolution of a sheet's claims, against a golden:
# tests/dawk/dcheck.sh as a CMake script, for a system with no shell.
#   The rule harness goes through the engine's evaluator and never through
# the driver, so it cannot see which claims the driver lets stand or the
# severity it reports them at. This runs the driver itself, over
# tests/dawk/dcheck/root with tests/dawk/dcheck/claims.dss, once under each
# cascade.resolve, and compares the findings it lists with
# claims.<mode>.expected, exactly.
#   The driver names a finding by the path it was given, so it is given the
# path from the repository root, with `/`, and run from there. Its findings
# are the lines that begin with that path; they are compared without their
# carriage returns, which a program's output has on Windows and a golden
# does not.
#   This script checks and does not rewrite: dcheck.sh --bless is the one way
# a golden is written.
#
#   usage:  cmake -DDCHECK=<the checker> -DROOT=<repository root>
#                 [-DLAUNCHER=<a program to run it with>] -P run_claims.cmake
#           LAUNCHER is for a build made for another system: what runs its
#           programs on this one
#   exit:   0 when both modes list what their goldens hold; otherwise an
#           error, after every failure has been printed
#
# path:      /build/cmake/config/testing/djinterp/tools/dawk/run_claims.cmake
# link(s):   TBA
# author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.10
#                                                            revised: 2026.10.10
#*******************************************************************************
cmake_minimum_required(VERSION 3.16)

if( (NOT DEFINED DCHECK)  OR
    (NOT DEFINED ROOT) )
    message(FATAL_ERROR
            "usage: cmake -DDCHECK=<the checker> -DROOT=<repository root> "
            "-P run_claims.cmake")
endif()

if(NOT EXISTS "${DCHECK}")
    message(FATAL_ERROR "run_claims: no program at ${DCHECK}")
endif()

# a build for another system is run through what runs it here
set(_run "${DCHECK}")

if( (DEFINED LAUNCHER)  AND
    (NOT "${LAUNCHER}" STREQUAL "") )
    set(_run "${LAUNCHER}" "${DCHECK}")
endif()

set(_dir    "tests/dawk/dcheck")
set(_failed 0)

foreach(_mode IN ITEMS first all)
    # both streams into one text, in the order they were written, as the
    # shell script's 2>&1 has them
    execute_process(
        COMMAND ${_run} "--cascade.resolve=${_mode}"
                --report.findings=1000 "${_dir}/claims.dss" "${_dir}/root"
        WORKING_DIRECTORY "${ROOT}"
        OUTPUT_VARIABLE   _printed
        ERROR_VARIABLE    _printed
        RESULT_VARIABLE   _status)

    # the checker exits 1 when it has findings: only a status that is no
    # number means it did not run
    if(NOT _status MATCHES "^[0-9]+$")
        message(FATAL_ERROR "run_claims: ${DCHECK} did not run: ${_status}")
    endif()

    # the findings: each line that begins with the root's path, in order.
    # The text is taken a line at a time by position and never made a CMake
    # list, in which a `;` or a bracket of a message would mean something
    string(REPLACE "\r" "" _rest "${_printed}")

    set(_actual "")

    while(NOT "${_rest}" STREQUAL "")
        string(FIND "${_rest}" "\n" _at)

        if(_at EQUAL -1)
            set(_line "${_rest}")
            set(_rest "")
        else()
            math(EXPR _next "${_at} + 1")
            string(SUBSTRING "${_rest}" 0 ${_at} _line)
            string(SUBSTRING "${_rest}" ${_next} -1 _rest)
        endif()

        if("${_line}" MATCHES "^${_dir}/root/")
            string(APPEND _actual "${_line}\n")
        endif()
    endwhile()

    set(_expected "")

    if(EXISTS "${ROOT}/${_dir}/claims.${_mode}.expected")
        file(READ "${ROOT}/${_dir}/claims.${_mode}.expected" _expected)
        string(REPLACE "\r" "" _expected "${_expected}")
    endif()

    string(REGEX REPLACE "\n+$" "" _actual   "${_actual}")
    string(REGEX REPLACE "\n+$" "" _expected "${_expected}")

    if(NOT "${_actual}" STREQUAL "${_expected}")
        message("FAIL dcheck claims (cascade.resolve=${_mode})")
        message("  expected:\n${_expected}")
        message("  the driver listed:\n${_actual}")

        math(EXPR _failed "${_failed} + 1")
    endif()
endforeach()

if(NOT _failed EQUAL 0)
    message(FATAL_ERROR "dcheck: ${_failed} of 2 modes did not list what "
                        "their goldens hold")
endif()

message("dcheck: claims resolve as the goldens say, under first and all")
