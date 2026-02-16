/******************************************************************************
* djinterp [test]                                              dtime_tests_sa.c
*
*   Master test runner for the dtime module (cross-platform time utilities).
*   Aggregates all test categories and manages test environment lifecycle.
*
*
* path:      \src\test\dtime_tests_sa.c
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.12.25
******************************************************************************/
#include "dtime_tests_sa.h"


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
bool
d_tests_dtime_setup
(
    void
)
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
bool
d_tests_dtime_teardown
(
    void
)
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
struct d_test_object*
d_tests_dtime_run_all
(
    void
)
{
    struct d_test_object* root;
    size_t                idx;
    size_t                total_tests;

    // setup test environment
    if (!d_tests_dtime_setup())
    {
        return NULL;
    }

    // determine total test count (9 sections)
    total_tests = 9;

    // create root test group
    root = d_test_object_new_interior("dtime Module Tests", total_tests);

    if (!root)
    {
        d_tests_dtime_teardown();

        return NULL;
    }

    // run all test categories
    idx = 0;
    root->elements[idx++] = d_tests_dtime_thread_safe_conversion_all();
    root->elements[idx++] = d_tests_dtime_high_resolution_time_all();
    root->elements[idx++] = d_tests_dtime_sleep_functions_all();
    root->elements[idx++] = d_tests_dtime_timezone_utilities_all();
    root->elements[idx++] = d_tests_dtime_string_parsing_formatting_all();
    root->elements[idx++] = d_tests_dtime_time_arithmetic_all();
    root->elements[idx++] = d_tests_dtime_monotonic_time_all();
    root->elements[idx++] = d_tests_dtime_timespec_normalization_all();
    root->elements[idx++] = d_tests_dtime_null_params_all();

    // teardown test environment
    d_tests_dtime_teardown();

    return root;
}
