#*******************************************************************************
# djinterp [tools]                                          run_gate_fault.cmake
#
# Proves the fixer's repair audit is load-bearing: tests/djinterp/tools/dawk/
# gate_fault.sh as a CMake script, for a system with no shell.
#   The leaf builds the checker twice with a deliberately wrong repair
# compiled in (D_AUDIT_FAULT_CORRUPT): a trailing comment, as the sheets add
# one to a corresponding header's #include and to an #endif, is added with
# the last character of the code before it turned into a `#`. One build
# keeps the repair audit and one compiles it out (D_AUDIT_DISABLED). Each is
# run to a fixpoint over a copy of tests/dawk/fix/before, whose files have
# findings whose rules state a repair. The gate passes only if the fault
# damages lines without the audit, none with it, and the audit refused
# something: a gate that never fires against a fault that never happens
# proves nothing, so the first condition is checked as strictly as the
# second.
#
#   usage:  cmake -DAUDITED=<the checker, with the audit>
#                 -DUNAUDITED=<the checker, without it>
#                 -DROOT=<repository root> -DWORK=<a directory to work in>
#                 [-DLAUNCHER=<a program to run them with>]
#                 -P run_gate_fault.cmake
#   exit:   0 when the audit stopped every damaged line, and there were some
#           to stop; otherwise an error, after every failure has been printed
#
# path:      /build/cmake/config/testing/djinterp/tools/dawk/run_gate_fault.cmake
# link(s):   TBA
# author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.16
#                                                            revised: 2026.11.16
#*******************************************************************************
cmake_minimum_required(VERSION 3.16)

if( (NOT DEFINED AUDITED)    OR
    (NOT DEFINED UNAUDITED)  OR
    (NOT DEFINED ROOT)       OR
    (NOT DEFINED WORK) )
    message(FATAL_ERROR
            "usage: cmake -DAUDITED=<checker> -DUNAUDITED=<checker> "
            "-DROOT=<repository root> -DWORK=<a directory to work in> "
            "-P run_gate_fault.cmake")
endif()

foreach(_program IN ITEMS "${AUDITED}" "${UNAUDITED}")
    if(NOT EXISTS "${_program}")
        message(FATAL_ERROR "run_gate_fault: no program at ${_program}")
    endif()
endforeach()

set(_before "${ROOT}/tests/dawk/fix/before")
set(_sheets c_style cpp_style)
set(_failed 0)

# gate_marks
#   function: how many lines under _dir hold the fault's mark: a `#` just
# before the gap ahead of a `//` comment.
function(gate_marks _dir _out)
    file(GLOB_RECURSE _files "${_dir}/*")
    set(_count 0)

    foreach(_file IN LISTS _files)
        file(STRINGS "${_file}" _marked REGEX "#[ ]+// ")
        list(LENGTH _marked _n)
        math(EXPR _count "${_count} + ${_n}")
    endforeach()

    set(${_out} ${_count} PARENT_SCOPE)
endfunction()

# gate_fix
#   function: runs a checker's fixer over _tree to a fixpoint, or for eight
# runs, and gives the repairs it refused.
function(gate_fix _program _tree _out)
    set(_run "${_program}")

    if( (DEFINED LAUNCHER)  AND
        (NOT "${LAUNCHER}" STREQUAL "") )
        set(_run "${LAUNCHER}" "${_program}")
    endif()

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

            # the checker exits 1 when it has findings: only a status that
            # is no number means it did not run
            if(NOT _status MATCHES "^[0-9]+$")
                message(FATAL_ERROR
                        "run_gate_fault: ${_program} did not run: ${_status}")
            endif()

            if(_printed MATCHES "(^|\n) *([0-9]+) lines repaired")
                math(EXPR _last "${_last} + ${CMAKE_MATCH_2}")
            endif()

            if(_printed MATCHES "(^|\n) *([0-9]+) repairs refused")
                math(EXPR _refused "${_refused} + ${CMAKE_MATCH_2}")
            endif()
        endforeach()

        math(EXPR _runs "${_runs} + 1")
    endwhile()

    set(${_out} ${_refused} PARENT_SCOPE)
endfunction()

gate_marks("${_before}" _base)

foreach(_variant IN ITEMS UNAUDITED AUDITED)
    set(_tree "${WORK}/${_variant}")

    # the copy, from nothing: a file left by an earlier run would count too
    file(REMOVE_RECURSE "${_tree}")
    file(MAKE_DIRECTORY "${_tree}")
    file(COPY "${_before}/" DESTINATION "${_tree}")

    gate_fix("${${_variant}}" "${_tree}" _refused_${_variant})
    gate_marks("${_tree}" _marks)
    math(EXPR _damaged_${_variant} "${_marks} - ${_base}")
endforeach()

message("fault marks without the audit: ${_damaged_UNAUDITED}")
message("fault marks with the audit:    ${_damaged_AUDITED}, "
        "${_refused_AUDITED} repair(s) refused")

if(NOT _damaged_UNAUDITED GREATER 0)
    message("FAIL gate_fault: the fault never fired, so this proves nothing")

    math(EXPR _failed "${_failed} + 1")
endif()

if(NOT _damaged_AUDITED EQUAL 0)
    message("FAIL gate_fault: damaged lines passed the audit")

    math(EXPR _failed "${_failed} + 1")
endif()

if(NOT _refused_AUDITED GREATER 0)
    message("FAIL gate_fault: the audit refused nothing")

    math(EXPR _failed "${_failed} + 1")
endif()

if(NOT _failed EQUAL 0)
    message(FATAL_ERROR "gate_fault: ${_failed} failure(s)")
endif()

message("gate_fault: the audit stopped every damaged line: "
        "${_damaged_UNAUDITED} without it, none with it")
