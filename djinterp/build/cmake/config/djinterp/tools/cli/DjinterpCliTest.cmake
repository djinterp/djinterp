#*******************************************************************************
# djinterp [tools]                                         DjinterpCliTest.cmake
#
# Run in script mode by a test d_cli_add_test made (DjinterpCli.cmake): runs
# D_CLI_TEST_COMMAND, a program and its arguments, and fails unless it ends
# with the status D_CLI_TEST_STATUS and, where D_CLI_TEST_MATCHES is not
# empty, what it says -- its output and its errors together -- matches that
# expression. What it said is printed either way, for the test's log.
#
#     cmake -DD_CLI_TEST_COMMAND=<program>;<argument>...
#           -DD_CLI_TEST_STATUS=<status> [-DD_CLI_TEST_MATCHES=<regex>]
#           -P DjinterpCliTest.cmake
#
# path:      /build/cmake/config/djinterp/tools/cli/DjinterpCliTest.cmake
# link(s):   TBA
# author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.26
#                                                            revised: 2026.10.26
#*******************************************************************************
execute_process(COMMAND ${D_CLI_TEST_COMMAND}
                RESULT_VARIABLE _status
                OUTPUT_VARIABLE _output
                ERROR_VARIABLE  _errors)

string(CONCAT _said "${_output}" "${_errors}")

# what it said, in the test's log
message("${_said}")

if(NOT "${_status}" STREQUAL "${D_CLI_TEST_STATUS}")
    message(FATAL_ERROR
            "it ended with ${_status}, not ${D_CLI_TEST_STATUS}")
endif()

if( (NOT "${D_CLI_TEST_MATCHES}" STREQUAL "") AND
    (NOT _said MATCHES "${D_CLI_TEST_MATCHES}") )
    message(FATAL_ERROR
            "nothing it said matches: ${D_CLI_TEST_MATCHES}")
endif()
