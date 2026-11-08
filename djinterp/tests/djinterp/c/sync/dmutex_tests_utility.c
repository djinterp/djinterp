/*******************************************************************************
* djinterp [test]                                         dmutex_tests_utility.c
*
*
* path:      /tests/djinterp/c/sync/dmutex_tests_utility.c
* link(s):   TBA
* author(s): TBA                                             created: 2026.10.15
*                                                            revised: 2026.11.06
*******************************************************************************/
// djinterp
#include "dmutex_tests.h"


/*
d_tests_hardware_concurrency
  Tests the d_thread_hardware_concurrency function.
  Tests the following:
  - returns a positive value (at least 1)
  - calling twice gives the same result
  - result is a reasonable value (< 4096 cores)
*/
bool d_tests_hardware_concurrency(
    struct dj_test_suite* _suite
)
{
    bool result;
    int  cores;
    int  cores2;

    d_test_suite_test(_suite, "hardware_concurrency");

    result = true;

    // test 1: returns at least 1
    cores  = d_thread_hardware_concurrency();
    result = D_TEST_CHECK_MSG(_suite,
                              "hw_concurrency_positive",
                              cores >= 1,
                              "d_thread_hardware_concurrency should return >= 1") && result;

    // test 2: consistent across calls
    cores2 = d_thread_hardware_concurrency();
    result = D_TEST_CHECK_MSG(_suite,
                              "hw_concurrency_consistent",
                              cores == cores2,
                              "Two calls should return the same value") && result;

    // test 3: reasonable upper bound
    result = D_TEST_CHECK_MSG(_suite,
                              "hw_concurrency_reasonable",
                              cores < 4096,
                              "Core count should be < 4096 (sanity check)") && result;

    return result;
}


/*
d_tests_utility_all
  Aggregation function that runs all utility function tests.
*/
bool d_tests_utility_all(
    struct dj_test_suite* _suite
)
{
    bool result;

    d_test_suite_block(_suite, "utility_all");

    result = true;

    result = d_tests_hardware_concurrency(_suite) && result;

    return result;
}
