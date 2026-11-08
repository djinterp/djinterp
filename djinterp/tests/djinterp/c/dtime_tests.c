/*******************************************************************************
* djinterp [test]                                                  dtime_tests.c
*
*   Master test runner for the dtime module (cross-platform time utilities).
*   Aggregates all test categories and manages test environment lifecycle.
*
*
* path:      /tests/djinterp/c/dtime_tests.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.12.25
*                                                            revised: 2026.11.06
*******************************************************************************/
// djinterp
#include "dtime_tests.h"


/******************************************************************************
 * TEST UTILITY FUNCTIONS
 *****************************************************************************/

/*
d_tests_dtime_setup
  Initializes test environment for dtime tests.
  Ensures timezone is set and performs any necessary initialization.

Return:
  true on success, false on failure.
*/
bool d_tests_dtime_setup(void)
{
    // initialize timezone information
    d_tzset();

    return true;
}


/*
d_tests_dtime_teardown
  Cleans up test environment after dtime tests.
  Currently a no-op as dtime tests don't create persistent resources.

Return:
  true on success, false on failure.
*/
bool d_tests_dtime_teardown(void)
{
    // no cleanup needed for time tests
    return true;
}


/******************************************************************************
 * MASTER TEST RUNNER
 *****************************************************************************/

/*
d_tests_dtime_run_all
  Runs all dtime tests and returns the root test object tree.
  Tests the following:
  - sets up test environment
  - runs all test categories
  - tears down test environment
  - returns comprehensive test results

Return:
  pointer to root test object containing all test results, or NULL on failure.
*/
bool d_tests_dtime_run_all(struct dj_test_suite* _suite)
{
    // setup test environment
    if (!d_tests_dtime_setup())
    {
        return false;
    }

    // run all test categories
    d_tests_dtime_thread_safe_conversion_all(_suite);
    d_tests_dtime_high_resolution_time_all(_suite);
    d_tests_dtime_sleep_functions_all(_suite);
    d_tests_dtime_timezone_utilities_all(_suite);
    d_tests_dtime_string_parsing_formatting_all(_suite);
    d_tests_dtime_time_arithmetic_all(_suite);
    d_tests_dtime_monotonic_time_all(_suite);
    d_tests_dtime_timespec_normalization_all(_suite);
    d_tests_dtime_null_params_all(_suite);

    // teardown test environment
    d_tests_dtime_teardown();

    return true;
}
