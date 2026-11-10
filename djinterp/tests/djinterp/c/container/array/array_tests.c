/*******************************************************************************
* djinterp [container]                                             array_tests.c
*
* Comprehensive test runner for `array.h` unit tests.
* Contains aggregate test suite runners only.
*
*
* path:      /tests/djinterp/c/container/array/array_tests.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.01.30
*                                                            revised: 2026.11.09
*******************************************************************************/

// djinterp
#include "./array_tests.h"
#include "../../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's C face


/*
d_tests_array_all
  Runs all core array tests.
*/
bool d_tests_array_all(
    struct dj_test_suite* _suite
)
{
    d_tests_array_run_creation_tests(_suite);
    d_tests_array_run_creation_s_tests(_suite);
    d_tests_array_run_manipulation_tests(_suite);
    d_tests_array_run_memory_tests(_suite);

    return true;
}

/*
d_tests_array_all_extended
  Runs all array tests including integration and edge cases.
*/
bool d_tests_array_all_extended(
    struct dj_test_suite* _suite
)
{
    d_tests_array_run_creation_tests(_suite);
    d_tests_array_run_creation_s_tests(_suite);
    d_tests_array_run_manipulation_tests(_suite);
    d_tests_array_run_memory_tests(_suite);
    d_tests_array_run_integration_tests(_suite);

    return true;
}

/*
d_tests_array_run_all
  Main entry point for the test module runner.
*/
bool d_tests_array_run_all(
    struct dj_test_suite* _suite
)
{
    return d_tests_array_all_extended(_suite);
}
