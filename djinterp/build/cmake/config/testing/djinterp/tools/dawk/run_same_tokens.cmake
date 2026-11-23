#*******************************************************************************
# djinterp [tools]                                         run_same_tokens.cmake
#
# dcheck --same-tokens, the check every fixer's work is put through, against
# pairs it must pass and pairs it must not: tests/dawk/same_tokens.sh as a
# CMake script, for a system with no shell.
#   tests/dawk/same_tokens/same holds <name>.before.* and <name>.after.*,
# what a fixer of layout and comments may do, and tests/dawk/same_tokens/
# differ pairs that differ in meaning, each by what its name says. A check
# that cannot fail checks nothing, so the second directory is the test.
#
#   usage:  cmake -DDCHECK=<the checker> -DROOT=<repository root>
#                 [-DLAUNCHER=<a program to run it with>]
#                 -P run_same_tokens.cmake
#           LAUNCHER is for a build made for another system: what runs its
#           programs on this one
#   exit:   0 when every pair passes or fails as it should; otherwise an error,
#           after every failure has been printed
#
# path:      /build/cmake/config/testing/djinterp/tools/dawk/run_same_tokens.cmake
# link(s):   TBA
# author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.19
#                                                            revised: 2026.11.19
#*******************************************************************************
cmake_minimum_required(VERSION 3.16)

if( (NOT DEFINED DCHECK)  OR
    (NOT DEFINED ROOT) )
    message(FATAL_ERROR
            "usage: cmake -DDCHECK=<the checker> -DROOT=<repository root> "
            "-P run_same_tokens.cmake")
endif()

if(NOT EXISTS "${DCHECK}")
    message(FATAL_ERROR "run_same_tokens: no program at ${DCHECK}")
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

file(GLOB _same RELATIVE "${ROOT}"
     "${ROOT}/tests/dawk/same_tokens/same/*.before.*")
file(GLOB _differ RELATIVE "${ROOT}"
     "${ROOT}/tests/dawk/same_tokens/differ/*.before.*")
set(_caught 0)

foreach(_before IN LISTS _same)
    string(REPLACE ".before." ".after." _after "${_before}")
    d_internal_check(same-tokens "${_before}" "${_after}")

    if(NOT _status EQUAL 0)
        message("FAIL same_tokens: ${_before} and its after differ\n${_said}")

        math(EXPR _failed "${_failed} + 1")
    endif()
endforeach()

foreach(_before IN LISTS _differ)
    string(REPLACE ".before." ".after." _after "${_before}")
    d_internal_check(same-tokens "${_before}" "${_after}")

    if(_status EQUAL 1)
        math(EXPR _caught "${_caught} + 1")
    else()
        message("FAIL same_tokens: ${_before} and its after are not told "
                "apart (${_status})\n${_said}")

        math(EXPR _failed "${_failed} + 1")
    endif()
endforeach()

if(NOT _failed EQUAL 0)
    message(FATAL_ERROR "same_tokens: ${_failed} check(s) did not pass")
endif()

list(LENGTH _differ _pairs)
message("same_tokens: the layout pairs pass; ${_caught} of ${_pairs} "
        "differing pairs are caught")
