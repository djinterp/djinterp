#*******************************************************************************
# djinterp [tools]                                        run_cmake_values.cmake
#
# What dcmake reads each argument as, against what CMake reads it as.
#   tests/djinterp/tools/dawk/ext/cmake/cmake_values.cmake holds an argument
# of every form, each in a call to `show`, which prints what it was given.
# Run under this CMake, the file prints CMake's reading; test_dcmake --values
# prints dcmake's reading of the same file, in the same form -- the arguments
# CMake passes, an unquoted one divided at its `;`. The two are compared
# exactly, once as the file is and once with every line ending made CR LF,
# since a file checked out on Windows has them.
#   Both readings are compared without carriage returns, which a program's
# output has on Windows and CMake's messages do not.
#
#   usage:  cmake -DTEST_DCMAKE=<test_dcmake> -DROOT=<repository root>
#                 -DWORK=<a directory to write in>
#                 [-DLAUNCHER=<a program to run it with>]
#                 -P run_cmake_values.cmake
#           WORK receives the CR LF copy of the cases; LAUNCHER is for a
#           build made for another system: what runs its programs on this one
#   exit:   0 when the readings agree; otherwise an error, after every
#           difference has been printed
#
# path:      /build/cmake/config/testing/djinterp/tools/dawk/run_cmake_values.cmake
# link(s):   TBA
# author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.16
#                                                            revised: 2026.10.20
#*******************************************************************************
cmake_minimum_required(VERSION 3.16)

if( (NOT DEFINED TEST_DCMAKE)  OR
    (NOT DEFINED ROOT)         OR
    (NOT DEFINED WORK) )
    message(FATAL_ERROR
            "usage: cmake -DTEST_DCMAKE=<test_dcmake> -DROOT=<repository "
            "root> -DWORK=<a directory> -P run_cmake_values.cmake")
endif()

if(NOT EXISTS "${TEST_DCMAKE}")
    message(FATAL_ERROR "run_cmake_values: no program at ${TEST_DCMAKE}")
endif()

# a build for another system is run through what runs it here
set(_run "${TEST_DCMAKE}")

if( (DEFINED LAUNCHER)  AND
    (NOT "${LAUNCHER}" STREQUAL "") )
    set(_run "${LAUNCHER}" "${TEST_DCMAKE}")
endif()

set(_cases  "${ROOT}/tests/djinterp/tools/dawk/ext/cmake/cmake_values.cmake")
set(_crlf   "${WORK}/cmake_values_crlf.cmake")
set(_failed 0)

# the same cases with CR LF line endings, written where the run may write
file(MAKE_DIRECTORY "${WORK}")
file(READ "${_cases}" _text)
string(REPLACE "\r\n" "\n" _text "${_text}")
string(REPLACE "\n" "\r\n" _text "${_text}")
file(WRITE "${_crlf}" "${_text}")

foreach(_file IN ITEMS "${_cases}" "${_crlf}")
    # CMake's reading: show() prints with message(), on the error stream
    execute_process(
        COMMAND "${CMAKE_COMMAND}" -P "${_file}"
        OUTPUT_VARIABLE _cmake_out
        ERROR_VARIABLE  _cmake
        RESULT_VARIABLE _cmake_status)

    # dcmake's reading of the same file
    execute_process(
        COMMAND ${_run} --values "${_file}"
        OUTPUT_VARIABLE _dcmake
        ERROR_VARIABLE  _dcmake_error
        RESULT_VARIABLE _dcmake_status)

    string(REPLACE "\r" "" _cmake  "${_cmake}")
    string(REPLACE "\r" "" _dcmake "${_dcmake}")

    # each must have read the file, and the two must agree exactly
    if(NOT _cmake_status EQUAL 0)
        message("FAIL CMake could not run ${_file}: ${_cmake}")
        set(_failed 1)
    elseif(NOT _dcmake_status EQUAL 0)
        message("FAIL test_dcmake could not read ${_file}: ${_dcmake}")
        set(_failed 1)
    elseif(NOT "${_cmake}" STREQUAL "${_dcmake}")
        message("FAIL the readings of ${_file} differ")
        message("CMake:\n${_cmake}")
        message("dcmake:\n${_dcmake}")
        set(_failed 1)
    else()
        message("agree: ${_file}")
    endif()
endforeach()

if(_failed)
    message(FATAL_ERROR "run_cmake_values: dcmake and CMake disagree")
endif()
