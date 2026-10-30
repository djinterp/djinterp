#*******************************************************************************
# djinterp [tools]                                                 run_fix.cmake
#
# What `dcheck --fix` does to a file, against the file it should leave:
# tests/dawk/fix.sh as a CMake script, for a system with no shell.
#   tests/dawk/fix/before holds files with findings whose rules state a
# repair, and tests/dawk/fix/after the same files, repaired. The fixer is run
# over a copy of before/ until a run repairs nothing, which it must reach,
# and the copy is then compared with after/, byte for byte and name for name.
# No repair may have been refused.
#   NOT DONE HERE: fix.sh's two checks more, that each repaired file is the
# same to the compiler as its original and that what was moved in it stands
# where its brackets put it. Those are tools/same_tokens.py and
# tools/moved_lines.py, and are the shell script's alone.
#
#   usage:  cmake -DDCHECK=<the checker> -DROOT=<repository root>
#                 -DWORK=<a directory to make the copy in>
#                 [-DLAUNCHER=<a program to run it with>] -P run_fix.cmake
#           LAUNCHER is for a build made for another system: what runs its
#           programs on this one
#   exit:   0 when the repaired copy is after/; otherwise an error, after
#           every failure has been printed
#
# path:      /build/cmake/config/testing/djinterp/tools/dawk/run_fix.cmake
# link(s):   TBA
# author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.10
#                                                            revised: 2026.10.10
#*******************************************************************************
cmake_minimum_required(VERSION 3.16)

if( (NOT DEFINED DCHECK)  OR
    (NOT DEFINED ROOT)    OR
    (NOT DEFINED WORK) )
    message(FATAL_ERROR
            "usage: cmake -DDCHECK=<the checker> -DROOT=<repository root> "
            "-DWORK=<a directory to make the copy in> -P run_fix.cmake")
endif()

if(NOT EXISTS "${DCHECK}")
    message(FATAL_ERROR "run_fix: no program at ${DCHECK}")
endif()

# a build for another system is run through what runs it here
set(_run "${DCHECK}")

if( (DEFINED LAUNCHER)  AND
    (NOT "${LAUNCHER}" STREQUAL "") )
    set(_run "${LAUNCHER}" "${DCHECK}")
endif()

set(_before "${ROOT}/tests/dawk/fix/before")
set(_after  "${ROOT}/tests/dawk/fix/after")
set(_tree   "${WORK}/tree")
set(_sheets c_style cpp_style)
set(_failed 0)

# the copy, from nothing: a file left by an earlier run would be compared too
file(REMOVE_RECURSE "${WORK}")
file(MAKE_DIRECTORY "${WORK}")
file(COPY "${_before}/" DESTINATION "${_tree}")

# until a run repairs nothing: an item moved to its column is moved alone,
# and what else its file needs waits for the next run
set(_total   0)
set(_refused 0)
set(_runs    0)
set(_last    1)

while( (_runs LESS 8)  AND
       (NOT _last EQUAL 0) )
    set(_last 0)

    foreach(_sheet IN LISTS _sheets)
        execute_process(
            COMMAND ${_run} --fix "res/sheets/${_sheet}.dss" "${_tree}"
            WORKING_DIRECTORY "${ROOT}"
            OUTPUT_VARIABLE   _printed
            ERROR_VARIABLE    _errors
            RESULT_VARIABLE   _status)

        # the checker exits 1 when it has findings: only a status that is
        # no number means it did not run
        if(NOT _status MATCHES "^[0-9]+$")
            message(FATAL_ERROR "run_fix: ${DCHECK} did not run: ${_status}")
        endif()

        if(_printed MATCHES "(^|\n) *([0-9]+) lines repaired")
            math(EXPR _last "${_last} + ${CMAKE_MATCH_2}")
        else()
            message("FAIL fix: res/sheets/${_sheet}.dss printed no count of "
                    "repairs")
            message("${_printed}${_errors}")

            math(EXPR _failed "${_failed} + 1")
        endif()

        if(_printed MATCHES "(^|\n) *([0-9]+) repairs refused")
            math(EXPR _refused "${_refused} + ${CMAKE_MATCH_2}")
        endif()
    endforeach()

    math(EXPR _runs  "${_runs} + 1")
    math(EXPR _total "${_total} + ${_last}")
endwhile()

if(NOT _last EQUAL 0)
    message("FAIL fix: still repairing after ${_runs} runs")

    math(EXPR _failed "${_failed} + 1")
endif()

if(NOT _refused EQUAL 0)
    message("FAIL fix: ${_refused} repair(s) refused")

    math(EXPR _failed "${_failed} + 1")
endif()

# name for name, then byte for byte
file(GLOB_RECURSE _wanted RELATIVE "${_after}" "${_after}/*")
file(GLOB_RECURSE _made   RELATIVE "${_tree}"  "${_tree}/*")
list(SORT _wanted)
list(SORT _made)

if(NOT "${_wanted}" STREQUAL "${_made}")
    message("FAIL fix: the repaired files are not the expected ones")
    message("  expected: ${_wanted}")
    message("  made:     ${_made}")

    math(EXPR _failed "${_failed} + 1")
endif()

foreach(_name IN LISTS _wanted)
    if(NOT EXISTS "${_tree}/${_name}")
        continue()
    endif()

    file(SHA256 "${_after}/${_name}" _want)
    file(SHA256 "${_tree}/${_name}"  _have)

    if(NOT _want STREQUAL _have)
        message("FAIL fix: ${_name} is not as tests/dawk/fix/after has it; "
                "the repaired copy is ${_tree}/${_name}")

        math(EXPR _failed "${_failed} + 1")
    endif()
endforeach()

if(NOT _failed EQUAL 0)
    message(FATAL_ERROR "fix: ${_failed} check(s) did not pass")
endif()

math(EXPR _needed "${_runs} - 1")

message("fix: ${_total} repairs as expected in ${_needed} run(s), none "
        "refused, the last run found nothing")
