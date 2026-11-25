#*******************************************************************************
# djinterp [tools]                                            run_renumber.cmake
#
# dcheck --renumber, which makes headings decimal, against the files it should
# leave: tests/dawk/renumber.sh as a CMake script, for a system with no shell.
#   tests/dawk/renumber/before holds headers with Roman headings, in each
# shape, and tests/dawk/renumber/after the same as they should come out:
# seven converted, six left exactly as they were. The tool runs over a copy
# of before/ and the copy is compared with after/, byte for byte. A second
# run must convert nothing; each file must be the same to the compiler as it
# was, since only comments may change (dcheck --same-tokens); and nothing in
# it may have moved (dcheck --moved-lines).
#
#   usage:  cmake -DDCHECK=<the checker> -DROOT=<repository root>
#                 -DWORK=<a directory to make the copy in>
#                 [-DLAUNCHER=<a program to run it with>]
#                 -P run_renumber.cmake
#           LAUNCHER is for a build made for another system: what runs its
#           programs on this one
#   exit:   0 when every check passes; otherwise an error, after every
#           failure has been printed
#
# path:      /build/cmake/config/testing/djinterp/tools/dawk/run_renumber.cmake
# link(s):   TBA
# author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.20
#                                                            revised: 2026.11.20
#*******************************************************************************
cmake_minimum_required(VERSION 3.16)

if( (NOT DEFINED DCHECK)  OR
    (NOT DEFINED ROOT)    OR
    (NOT DEFINED WORK) )
    message(FATAL_ERROR
            "usage: cmake -DDCHECK=<the checker> -DROOT=<repository root> "
            "-DWORK=<a directory to make the copy in> -P run_renumber.cmake")
endif()

if(NOT EXISTS "${DCHECK}")
    message(FATAL_ERROR "run_renumber: no program at ${DCHECK}")
endif()

# a build for another system is run through what runs it here
set(_run "${DCHECK}")

if( (DEFINED LAUNCHER)  AND
    (NOT "${LAUNCHER}" STREQUAL "") )
    set(_run "${LAUNCHER}" "${DCHECK}")
endif()

set(_before "${ROOT}/tests/dawk/renumber/before")
set(_after  "${ROOT}/tests/dawk/renumber/after")
set(_tree   "${WORK}/inc/djinterp")
set(_failed 0)

# the copy, from nothing: a file left by an earlier run would be compared too
file(REMOVE_RECURSE "${WORK}")
file(MAKE_DIRECTORY "${WORK}/src/djinterp")
file(COPY "${_before}/" DESTINATION "${_tree}")

# two runs: the first converts, the second must find nothing
foreach(_pass IN ITEMS first second)
    execute_process(
        COMMAND ${_run} --renumber --base "${WORK}"
        WORKING_DIRECTORY "${ROOT}"
        OUTPUT_VARIABLE   _said_${_pass}
        ERROR_VARIABLE    _errors
        RESULT_VARIABLE   _status)

    if(NOT _status EQUAL 0)
        message("FAIL renumber: the ${_pass} run failed (${_status})\n"
                "${_said_${_pass}}${_errors}")

        math(EXPR _failed "${_failed} + 1")
    endif()
endforeach()

if(NOT _said_second MATCHES "^converted: 0 file")
    message("FAIL renumber: a second run found more to convert\n"
            "${_said_second}")

    math(EXPR _failed "${_failed} + 1")
endif()

# each file against what it should be, then against its original
file(GLOB _names RELATIVE "${_before}" "${_before}/*")

foreach(_name IN LISTS _names)
    file(READ "${_tree}/${_name}" _got HEX)
    file(READ "${_after}/${_name}" _want HEX)

    if(NOT _got STREQUAL _want)
        message("FAIL renumber: ${_name} is not as it should come out")

        math(EXPR _failed "${_failed} + 1")
    endif()

    foreach(_check IN ITEMS same-tokens moved-lines)
        execute_process(
            COMMAND ${_run} "--${_check}" "${_before}/${_name}"
                    "${_tree}/${_name}"
            WORKING_DIRECTORY "${ROOT}"
            OUTPUT_VARIABLE   _printed
            ERROR_VARIABLE    _printed
            RESULT_VARIABLE   _status)

        if(NOT _status EQUAL 0)
            message("FAIL renumber: ${_name} fails dcheck --${_check}\n"
                    "${_printed}")

            math(EXPR _failed "${_failed} + 1")
        endif()
    endforeach()
endforeach()

if(NOT _failed EQUAL 0)
    message(FATAL_ERROR "renumber: ${_failed} check(s) did not pass")
endif()

string(REGEX REPLACE "\n.*$" "" _summary "${_said_first}")
list(LENGTH _names _count)
message("renumber: ${_summary} of ${_count}; nothing for a second run, the "
        "tokens unchanged, nothing moved")
