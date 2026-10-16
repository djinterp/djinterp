/******************************************************************************
* djinterp [test]                                                       main.c
*
*   Test runner for cli_unix module unit tests.
*   Executes comprehensive standalone tests for Unix CLI convention parsing
* including long options (--key, --key=val), short flags (-v), combined
* short flags (-xvf), next-arg consumption, stop token (--), and edge cases.
*
*
* path:      /tests/util/cli_unix/main.c
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2026.02.26
******************************************************************************/
#include "../../../../../../inc/c/test/test_standalone.h"
#include "../../../../../../tests/c/util/test_cli_unix_sa.h"


/*
main
  Entry point for the cli_unix test suite.
  Sets up the test runner, registers the cli_unix module, executes all
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
                          "CLI Unix Module Test Suite",
                          "Comprehensive unit tests for cli_unix.h");

    // register the cli_unix module
    d_test_sa_runner_add_module_counter(
        &runner,
        "Unix CLI Convention Parsing",
        "Tests for --long, -short, -combined, --key=val, and -- stop token",
        d_tests_sa_test_cli_unix_all,
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
