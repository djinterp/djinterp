/******************************************************************************
* djinterp [test]                                                       main.c
*
*   Test runner for test_block module unit tests.
*   Executes comprehensive standalone tests for test block construction,
* child management, stage hooks, execution, and utility functions including
* d_test_block_new, d_test_block_add_child, d_test_block_run,
* d_test_block_count_tests, and d_test_block_free.
*
*
* path:      /.config/.msvs/testing/c/test/
*                djinterp-c-test-test-block-tests-sa/main.c
* link(s)    TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.12.16
******************************************************************************/
#include "../../../../../../inc/c/test/test_standalone.h"
#include "../../../../../../tests/c/test/test_block_tests_sa.h"


/*
main
  Entry point for the test_block test suite.
  Sets up the test runner, registers the test_block module, executes all
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
                          "Test Block Module Test Suite",
                          "Comprehensive unit tests for test_block.h");

    // register the test_block module
    d_test_sa_runner_add_module_counter(
        &runner,
        "Test Block Construction, Management & Execution",
        "Tests for d_test_block_new, child mgmt, hooks, run, and utilities",
        d_tests_sa_test_block_all,
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