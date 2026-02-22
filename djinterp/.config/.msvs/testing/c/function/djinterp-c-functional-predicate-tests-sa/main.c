/******************************************************************************
* djinterp [test]                                                       main.c
*
*   Test runner for predicate module unit tests.
*   Executes comprehensive standalone tests for all predicate combinator
* functions including AND, OR, XOR, and NOT operations with both constructor
* and evaluation functions.
*
*
* path:      \tests\functional\predicate\main.c
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2026.02.09
******************************************************************************/
#include "../../../../../../inc/c/test/test_standalone.h"
#include "../../../../../../tests/c/functional/predicate_tests_sa.h"


/*
main
  Entry point for the predicate test suite.
  Sets up the test runner, registers the predicate module, executes all tests,
and displays comprehensive results.

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
                          "Predicate Module Test Suite",
                          "Comprehensive unit tests for predicate.h");

    // register the predicate module
    d_test_sa_runner_add_module_counter(
        &runner,
        "Predicate Combinators",
        "Tests for AND, OR, XOR, and NOT predicate combinators",
        d_tests_sa_predicate_run_all,
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