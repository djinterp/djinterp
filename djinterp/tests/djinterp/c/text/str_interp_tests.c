/*******************************************************************************
* djinterp [text]                                             str_interp_tests.c
*
*
* path:      /tests/djinterp/c/text/str_interp_tests.c
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "./str_interp_tests.h"
#include "../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's C face


/******************************************************************************
 * TEST UTILITY FUNCTIONS
 *****************************************************************************/

/*
d_tests_str_interp_setup
  Sets up test environment for string interpolation tests.
*/
bool d_tests_str_interp_setup(void)
{
    /* no specific setup needed; present for consistency */

    return true;
}

/*
d_tests_str_interp_teardown
  Cleans up test environment.
*/
bool d_tests_str_interp_teardown(void)
{
    /* no specific teardown needed; present for consistency */

    return true;
}


/******************************************************************************
 * MASTER TEST RUNNER
 *****************************************************************************/

/*
d_tests_str_interp_run_all
  Master test runner for all str_interp module tests.
  Tests the following:
  - Context management (new, free, clear)
  - Specifier management (add, add_n, remove, update, get, has)
  - Interpolation functions (interp, alloc, length)
  - Convenience functions (quick)
  - Utility functions (error_string)
*/
bool d_tests_str_interp_run_all(struct dj_test_suite* _suite)
{
    bool                  setup_ok;

    // setup test environment
    setup_ok = d_tests_str_interp_setup();

    if (!setup_ok)
    {
        printf("Failed to setup str_interp test environment\n");

        return false;
    }

    // create master group
    d_test_suite_block(_suite, "str_interp Module Tests");

    // run all test categories
    d_tests_str_interp_context_all(_suite);
    d_tests_str_interp_specifier_all(_suite);
    d_tests_str_interp_interp_all(_suite);
    d_tests_str_interp_convenience_all(_suite);
    d_tests_str_interp_utility_all(_suite);

    // cleanup
    d_tests_str_interp_teardown();

    return true;
}
