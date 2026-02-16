#include ".\dmutex_tests_sa.h"


/*
d_tests_sa_hardware_concurrency
  Tests the d_thread_hardware_concurrency function.
  Tests the following:
  - returns a positive value (at least 1)
  - calling twice gives the same result
  - result is a reasonable value (< 4096 cores)
*/
bool
d_tests_sa_hardware_concurrency
(
    struct d_test_counter* _counter
)
{
    bool result;
    int  cores;
    int  cores2;

    result = true;

    // test 1: returns at least 1
    cores  = d_thread_hardware_concurrency();
    result = d_assert_standalone(
        cores >= 1,
        "hw_concurrency_positive",
        "d_thread_hardware_concurrency should return >= 1",
        _counter) && result;

    // test 2: consistent across calls
    cores2 = d_thread_hardware_concurrency();
    result = d_assert_standalone(
        cores == cores2,
        "hw_concurrency_consistent",
        "Two calls should return the same value",
        _counter) && result;

    // test 3: reasonable upper bound
    result = d_assert_standalone(
        cores < 4096,
        "hw_concurrency_reasonable",
        "Core count should be < 4096 (sanity check)",
        _counter) && result;

    return result;
}


/*
d_tests_sa_utility_all
  Aggregation function that runs all utility function tests.
*/
bool
d_tests_sa_utility_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Utility Functions\n");
    printf("  ----------------------------\n");

    result = d_tests_sa_hardware_concurrency(_counter) && result;

    return result;
}
