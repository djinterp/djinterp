#*******************************************************************************
# djinterp [tools]                                               run_rules.cmake
#
# Every rule fixture against its sheet's golden findings: tests/dawk/rules.sh
# as a CMake script, for a system with no shell.
#   tests/dawk/fixtures/<sheet>/.../<name>.<c|h|cpp|hpp> is a fixture, and
# the file of the same name with .expected after it holds its findings,
# exactly. The sheet is res/sheets/<sheet>.dss. A fixture with an empty golden
# is a negative case: the rules must stay silent on it. A fixture that does not
# match is named, with what was expected and what came; and so is a sheet
# with a rule that fires on none of its fixtures, since a selector naming
# what nothing emits matches nothing, silently.
#   The harness names a finding by the path it was given, so it is given the
# path from the repository root, with `/`, and run from there. What it
# prints is compared without its carriage returns: on Windows a program's
# output has them and a golden does not.
#   This script checks and does not rewrite: rules.sh --bless is the one way
# a golden is written.
#
#   usage:  cmake -DRULES=<the rules program> -DROOT=<repository root>
#                 [-DLAUNCHER=<a program to run it with>] -P run_rules.cmake
#           LAUNCHER is for a build made for another system: what runs its
#           programs on this one
#   exit:   0 when every fixture matches and every rule is exercised;
#           otherwise an error, after every failure has been printed
#
# path:      /build/cmake/config/testing/djinterp/tools/dawk/run_rules.cmake
# link(s):   TBA
# author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.10
#                                                            revised: 2026.10.10
#*******************************************************************************
cmake_minimum_required(VERSION 3.16)

if( (NOT DEFINED RULES)  OR
    (NOT DEFINED ROOT) )
    message(FATAL_ERROR
            "usage: cmake -DRULES=<the rules program> -DROOT=<repository "
            "root> -P run_rules.cmake")
endif()

if(NOT EXISTS "${RULES}")
    message(FATAL_ERROR "run_rules: no program at ${RULES}")
endif()

# a build for another system is run through what runs it here
set(_run "${RULES}")

if( (DEFINED LAUNCHER)  AND
    (NOT "${LAUNCHER}" STREQUAL "") )
    set(_run "${LAUNCHER}" "${RULES}")
endif()

# djinterp_dawk_trimmed
#   function: a program's output or a golden's text as the two are compared:
# with no carriage return, and with no newline at its end, however many it
# had, as a shell's $( ) leaves it.
function(djinterp_dawk_trimmed _text _out)
    string(REPLACE "\r" "" _text "${_text}")
    string(REGEX REPLACE "\n+$" "" _text "${_text}")

    set(${_out} "${_text}" PARENT_SCOPE)
endfunction()

file(GLOB_RECURSE _fixtures
     RELATIVE "${ROOT}"
     "${ROOT}/tests/dawk/fixtures/*.c"
     "${ROOT}/tests/dawk/fixtures/*.h"
     "${ROOT}/tests/dawk/fixtures/*.cpp"
     "${ROOT}/tests/dawk/fixtures/*.hpp")
list(SORT _fixtures)

set(_count  0)
set(_failed 0)
set(_sheets "")

foreach(_fixture IN LISTS _fixtures)
    # tests/dawk/fixtures/<sheet>/...: the fourth part names the sheet
    string(REGEX REPLACE "^tests/dawk/fixtures/([^/]+)/.*$" "\\1"
           _sheet "${_fixture}")

    if(NOT _sheet IN_LIST _sheets)
        list(APPEND _sheets "${_sheet}")
    endif()

    list(APPEND _fixtures_of_${_sheet} "${_fixture}")

    # the harness exits 1 when it has findings, so a run is judged by what
    # it printed; a status that is no number means it did not run at all
    execute_process(
        COMMAND ${_run} "res/sheets/${_sheet}.dss" "${_fixture}"
        WORKING_DIRECTORY "${ROOT}"
        OUTPUT_VARIABLE   _actual
        ERROR_VARIABLE    _errors
        RESULT_VARIABLE   _status)

    if(NOT _status MATCHES "^[0-9]+$")
        message(FATAL_ERROR "run_rules: ${RULES} did not run: ${_status}")
    endif()

    set(_expected "")

    if(EXISTS "${ROOT}/${_fixture}.expected")
        file(READ "${ROOT}/${_fixture}.expected" _expected)
    endif()

    djinterp_dawk_trimmed("${_actual}"   _actual)
    djinterp_dawk_trimmed("${_expected}" _expected)

    math(EXPR _count "${_count} + 1")

    if(NOT "${_actual}" STREQUAL "${_expected}")
        message("FAIL ${_fixture}")
        message("  expected:\n${_expected}")
        message("  the harness printed:\n${_actual}")

        if(NOT "${_errors}" STREQUAL "")
            message("  and, as errors:\n${_errors}")
        endif()

        math(EXPR _failed "${_failed} + 1")
    endif()
endforeach()

if(_count EQUAL 0)
    message(FATAL_ERROR
            "run_rules: no fixture under ${ROOT}/tests/dawk/fixtures")
endif()

# every rule of a sheet must fire on some fixture of that sheet
foreach(_sheet IN LISTS _sheets)
    execute_process(
        COMMAND ${_run} --uncovered "res/sheets/${_sheet}.dss"
                ${_fixtures_of_${_sheet}}
        WORKING_DIRECTORY "${ROOT}"
        OUTPUT_VARIABLE   _actual
        ERROR_VARIABLE    _errors
        RESULT_VARIABLE   _status)

    if(NOT _status EQUAL 0)
        message("FAIL res/sheets/${_sheet}.dss has rules no fixture exercises")
        message("${_actual}${_errors}")

        math(EXPR _failed "${_failed} + 1")
    endif()
endforeach()

if(NOT _failed EQUAL 0)
    message(FATAL_ERROR "rules: ${_failed} of ${_count} fixtures or their "
                        "sheets did not pass")
endif()

message("rules: ${_count} fixtures, all match their goldens")
