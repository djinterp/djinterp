#*******************************************************************************
# djinterp [tools]                                               run_agree.cmake
#
# The driver lists, of every fixture, what its golden holds:
# tests/dawk/agree.sh as a CMake script, for a system with no shell.
#   run_rules.cmake reads a fixture through the harness, which builds the
# tokens of one file and nothing more. The gates read the tree through
# dcheck, which hangs every builder's node under the file beside the tokens
# -- banner, guard, include-block, delimiter, text -- and runs the cascade.
# A selector can tell the two trees apart, and one that does passes its
# fixture and fails in the gate. So dcheck is run over each sheet's fixtures
# as the gate runs it over the tree, and what it lists, with the severity
# taken out of each line, must be what the goldens hold, line for line.
#   The lines are compared sorted, and without their carriage returns, which
# a program's output has on Windows and a golden does not.
#
#   usage:  cmake -DDCHECK=<the checker> -DROOT=<repository root>
#                 [-DLAUNCHER=<a program to run it with>] -P run_agree.cmake
#           LAUNCHER is for a build made for another system: what runs its
#           programs on this one
#   exit:   0 when the driver and the goldens agree for every sheet;
#           otherwise an error, after every difference has been printed
#
# path:      /build/cmake/config/testing/djinterp/tools/dawk/run_agree.cmake
# link(s):   TBA
# author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.10
#                                                            revised: 2026.10.10
#*******************************************************************************
cmake_minimum_required(VERSION 3.16)

if( (NOT DEFINED DCHECK)  OR
    (NOT DEFINED ROOT) )
    message(FATAL_ERROR
            "usage: cmake -DDCHECK=<the checker> -DROOT=<repository root> "
            "-P run_agree.cmake")
endif()

if(NOT EXISTS "${DCHECK}")
    message(FATAL_ERROR "run_agree: no program at ${DCHECK}")
endif()

# a build for another system is run through what runs it here
set(_run "${DCHECK}")

if( (DEFINED LAUNCHER)  AND
    (NOT "${LAUNCHER}" STREQUAL "") )
    set(_run "${LAUNCHER}" "${DCHECK}")
endif()

# three characters no finding holds stand in for the three a CMake list
# reads as its own, so that a text can be sorted a line at a time
string(ASCII 1 _semicolon)
string(ASCII 2 _open)
string(ASCII 3 _close)

# djinterp_dawk_sorted
#   function: a text's lines, sorted, as one text again, with every
# carriage return and empty line taken out.
function(djinterp_dawk_sorted _text _out)
    string(REPLACE "\r" "" _text "${_text}")
    string(REPLACE ";" "${_semicolon}" _text "${_text}")
    string(REPLACE "[" "${_open}"      _text "${_text}")
    string(REPLACE "]" "${_close}"     _text "${_text}")
    string(REPLACE "\n" ";"            _lines "${_text}")

    list(REMOVE_ITEM _lines "")
    list(SORT _lines)

    string(REPLACE ";" "\n"            _text "${_lines}")
    string(REPLACE "${_semicolon}" ";" _text "${_text}")
    string(REPLACE "${_open}" "["      _text "${_text}")
    string(REPLACE "${_close}" "]"     _text "${_text}")

    set(${_out} "${_text}" PARENT_SCOPE)
endfunction()

file(GLOB _directories
     RELATIVE "${ROOT}/tests/dawk/fixtures"
     "${ROOT}/tests/dawk/fixtures/*")
list(SORT _directories)

set(_sheets 0)
set(_failed 0)

foreach(_sheet IN LISTS _directories)
    set(_root "tests/dawk/fixtures/${_sheet}")

    if(NOT IS_DIRECTORY "${ROOT}/${_root}")
        continue()
    endif()

    execute_process(
        COMMAND ${_run} --source.files=all --report.findings=1000000
                "sheets/${_sheet}.dss" "${_root}"
        WORKING_DIRECTORY "${ROOT}"
        OUTPUT_VARIABLE   _printed
        ERROR_VARIABLE    _errors
        RESULT_VARIABLE   _status)

    # the checker exits 1 when it has findings: only a status that is no
    # number means it did not run
    if(NOT _status MATCHES "^[0-9]+$")
        message(FATAL_ERROR "run_agree: ${DCHECK} did not run: ${_status}")
    endif()

    # the driver's findings: the lines that begin with the fixtures' path,
    # each with its severity taken out, taken a line at a time by position
    string(REPLACE "\r" "" _rest "${_printed}")

    set(_driver "")

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

        if("${_line}" MATCHES
           "^(${_root}/[^:]+:[0-9]+:[0-9]+): (error|warn|advise): ")
            string(LENGTH "${CMAKE_MATCH_0}" _head)
            string(SUBSTRING "${_line}" ${_head} -1 _message)
            string(APPEND _driver "${CMAKE_MATCH_1}: ${_message}\n")
        endif()
    endwhile()

    # the goldens of the sheet, every one of them
    file(GLOB_RECURSE _expected_files "${ROOT}/${_root}/*.expected")

    set(_goldens "")

    foreach(_file IN LISTS _expected_files)
        file(READ "${_file}" _text)
        string(APPEND _goldens "${_text}\n")
    endforeach()

    djinterp_dawk_sorted("${_driver}"  _driver)
    djinterp_dawk_sorted("${_goldens}" _goldens)

    math(EXPR _sheets "${_sheets} + 1")

    if(NOT "${_driver}" STREQUAL "${_goldens}")
        message("FAIL sheets/${_sheet}.dss: the driver and the goldens "
                "differ")
        message("  the goldens hold:\n${_goldens}")
        message("  the driver lists:\n${_driver}")

        math(EXPR _failed "${_failed} + 1")
    endif()
endforeach()

if(_sheets EQUAL 0)
    message(FATAL_ERROR
            "run_agree: no fixtures under ${ROOT}/tests/dawk/fixtures")
endif()

if(NOT _failed EQUAL 0)
    message(FATAL_ERROR "agree: the driver and the goldens differ for "
                        "${_failed} of ${_sheets} sheets")
endif()

message("agree: the driver lists what the goldens hold, for ${_sheets} "
        "sheets")
