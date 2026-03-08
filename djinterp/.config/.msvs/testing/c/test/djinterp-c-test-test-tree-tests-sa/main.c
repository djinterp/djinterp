/******************************************************************************
* djinterp [test]                                                       main.c
*
*   Test runner for cli_option module unit tests.
*   Executes comprehensive standalone tests for convention-agnostic
* string-to-void* lookup and CLI argument parsing including d_string_cli_entry
* lookup, generic field-based search, custom lookup delegation, argv parsing,
* and table macros.
*
*
* path:      /.config/.msvs/testing/c/test/djinterp-c-test-test-tests-sa/main.c
* link(s)    TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2026.02.26
******************************************************************************/
#include "../../../../../../inc/c/test/test_standalone.h"
#include "../../../../../../tests/c/test/test_tests_sa.h"


/*
main
  Entry point for the cli_option test suite.
  Sets up the test runner, registers the cli_option module, executes all
tests, and displays comprehensive results.

Parameter(s):
  _argc: argument count (unused)
  _argv: argument values (unused)
Return:
  0 on success, 1 on failure.
*/
int
main
(
    int   _argc,
    char* _argv[]
)
{
    struct d_test_sa_runner runner;
    int                     exit_code;

    (void)_argc;
    (void)_argv;

    // initialize the test runner
    d_test_sa_runner_init(&runner,
                          "CLI Option Module Test Suite",
                          "Comprehensive unit tests for cli_option.h");

    // register the cli_option module
    d_test_sa_runner_add_module_counter(
        &runner,
        "String-to-Void* Lookup & CLI Parsing",
        "Tests for d_string_cli_entry lookup, field search, and argv parsing",
        d_tests_sa_test_run_all,
        0,
        NULL);

    // configure runner options
    d_test_sa_runner_set_wait_for_input(&runner, false);
    d_test_sa_runner_set_show_notes(&runner, false);

    // execute all tests
    exit_code = d_test_sa_runner_execute(&runner);

    // cleanup
    d_test_sa_runner_cleanup(&runner);

    return exit_code;
}