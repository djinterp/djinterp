#*******************************************************************************
# bitwisedom [tests]                                          check_header.cmake
#
# One generated header, proved: bitwisedom-gen writes an example's header and
# its self-test at a language level, and the compiler compiles the self-test
# at that level. The self-test fails to compile unless every macro has the
# value the sheet gives it. A sheet wider than an integer keeps its bits in
# arrays, which its self-test checks as it runs: with RUN, the self-test is
# linked and run too, and passes only if it returns 0. Run by CTest with
# cmake -P; prints "self-test compiled" when it did, and "self-test compiled
# and ran" when it ran too. A failure never says either: CTest passes a test
# by its output alone.
#
#     GEN EXAMPLE (or SHEET, a sheet file) LEVEL FORM LANGUAGE (c|cxx)
#     COMPILER FLAG STYLE (gnu|msvc) WORK, and EMULATOR: what runs GEN, and
#     a self-test, where this machine cannot -- in a cross build,
#     CMAKE_CROSSCOMPILING_EMULATOR; empty otherwise. RUN (ON|OFF), and WORD:
#     a wide sheet's array words, in bits, when not its own
#
# path:      /tests/djinterp/tools/bitwisedom/check_header.cmake
# link(s):   TBA
# author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.04
#                                                            revised: 2026.10.11
#*******************************************************************************
file(MAKE_DIRECTORY "${WORK}")

if(LANGUAGE STREQUAL "c")
    set(_header_ext ".h")
    set(_test_ext ".c")
else()
    set(_header_ext ".hpp")
    set(_test_ext ".cpp")
endif()

if(DEFINED SHEET AND NOT SHEET STREQUAL "")
    get_filename_component(_name "${SHEET}" NAME)
    string(REGEX REPLACE "[.].*$" "" _name "${_name}")
    set(_source "${SHEET}")
else()
    set(_name "${EXAMPLE}")
    set(_source --example "${EXAMPLE}")
endif()

set(_word_option)

if(DEFINED WORD AND NOT WORD STREQUAL "")
    set(_name "${_name}_w${WORD}")
    set(_word_option --word "${WORD}")
endif()

set(_base "${WORK}/${_name}_${LEVEL}_${FORM}")

execute_process(
    COMMAND ${EMULATOR} "${GEN}" ${_source} --level "${LEVEL}"
                                 --form "${FORM}" ${_word_option}
                                 -o "${_base}${_header_ext}"
                                 --self-test "${_base}${_test_ext}"
    RESULT_VARIABLE _result
    OUTPUT_VARIABLE _output
    ERROR_VARIABLE  _errors)

if(NOT _result EQUAL 0)
    message(FATAL_ERROR "bitwisedom-gen failed (${_result}):\n${_errors}")
endif()

# compiled alone, or linked to run
if(STYLE STREQUAL "msvc")
    if(RUN)
        set(_command "${COMPILER}" /nologo "${FLAG}" /W4 /WX
                     "${_base}${_test_ext}" "/Fo${_base}.obj"
                     "/Fe${_base}.exe")
    else()
        set(_command "${COMPILER}" /nologo "${FLAG}" /W4 /WX /c
                     "${_base}${_test_ext}" "/Fo${_base}.obj")
    endif()
else()
    if(RUN)
        set(_command "${COMPILER}" "${FLAG}" -Wall -Wextra -pedantic-errors
                     -Werror "${_base}${_test_ext}" -o "${_base}.exe")
    else()
        set(_command "${COMPILER}" "${FLAG}" -Wall -Wextra -pedantic-errors
                     -Werror -c "${_base}${_test_ext}" -o "${_base}.o")
    endif()
endif()

execute_process(
    COMMAND ${_command}
    WORKING_DIRECTORY "${WORK}"
    RESULT_VARIABLE   _result
    OUTPUT_VARIABLE   _output
    ERROR_VARIABLE    _errors)

if(NOT _result EQUAL 0)
    message(FATAL_ERROR "the self-test did not compile:\n${_output}${_errors}")
endif()

if(NOT RUN)
    message("self-test compiled: ${_base}${_test_ext}")

    return()
endif()

execute_process(
    COMMAND ${EMULATOR} "${_base}.exe"
    WORKING_DIRECTORY "${WORK}"
    RESULT_VARIABLE   _result
    OUTPUT_VARIABLE   _output
    ERROR_VARIABLE    _errors)

# said without the words a pass is known by
if(NOT _result EQUAL 0)
    message(FATAL_ERROR
            "the self-test's run failed (${_result}):\n${_output}${_errors}")
endif()

message("self-test compiled and ran: ${_base}${_test_ext}")
