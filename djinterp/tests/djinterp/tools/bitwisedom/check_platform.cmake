#*******************************************************************************
# bitwisedom [tests]                                        check_platform.cmake
#
# One platform example, proved against the platform: bitwisedom-gen writes the
# example's header, and the compiler compiles tests/platform/<example>.c,
# which includes that header beside the platform's own and compares every
# value at compile time. It fails to compile unless each agrees. Run by CTest
# with cmake -P; prints "<example> agrees with the platform's headers" when
# it did.
#
#     GEN EXAMPLE CHECK (the .c) COMPILER STYLE (gnu|msvc) WORK, and EMULATOR:
#     what runs GEN where this machine cannot -- in a cross build,
#     CMAKE_CROSSCOMPILING_EMULATOR; empty otherwise
#
# The platform's headers are not this project's, so the check is compiled
# without -Werror or /WX: what fails it is a value that disagrees. The
# example's header is held to every warning by the header tests.
#
# path:      /tests/djinterp/tools/bitwisedom/check_platform.cmake
# link(s):   TBA
# author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.10
#                                                            revised: 2026.10.10
#*******************************************************************************
file(MAKE_DIRECTORY "${WORK}")

# the check includes "<example>.h"
execute_process(
    COMMAND ${EMULATOR} "${GEN}" --example "${EXAMPLE}" --level c11
                                 --form macros -o "${WORK}/${EXAMPLE}.h"
    RESULT_VARIABLE _result
    OUTPUT_VARIABLE _output
    ERROR_VARIABLE  _errors)

if(NOT _result EQUAL 0)
    message(FATAL_ERROR "bitwisedom-gen failed (${_result}):\n${_errors}")
endif()

if(STYLE STREQUAL "msvc")
    set(_command "${COMPILER}" /nologo /std:c11 /W3 "/I${WORK}" /c "${CHECK}"
                 "/Fo${WORK}/${EXAMPLE}.obj")
else()
    set(_command "${COMPILER}" -std=gnu11 -Wall -Wextra "-I${WORK}" -c
                 "${CHECK}" -o "${WORK}/${EXAMPLE}.o")
endif()

execute_process(
    COMMAND ${_command}
    WORKING_DIRECTORY "${WORK}"
    RESULT_VARIABLE   _result
    OUTPUT_VARIABLE   _output
    ERROR_VARIABLE    _errors)

if(NOT _result EQUAL 0)
    message(FATAL_ERROR
            "${EXAMPLE} disagrees with the platform's headers, or did not "
            "compile:\n${_output}${_errors}")
endif()

message("${EXAMPLE} agrees with the platform's headers")
