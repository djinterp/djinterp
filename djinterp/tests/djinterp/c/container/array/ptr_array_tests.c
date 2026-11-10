/*******************************************************************************
* djinterp [container]                                         ptr_array_tests.c
*
*
* path:      /tests/djinterp/c/container/array/ptr_array_tests.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.02.25
*                                                            revised: 2026.11.09
*******************************************************************************/
// djinterp
#include "./ptr_array_tests.h"
#include "../../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's C face


/*
d_tests_ptr_array_run_all
  Module-level aggregation function that runs all ptr_array tests.
  Executes tests for all categories:
  - Constructor functions
  - Element manipulation functions
  - Destructor functions
*/
bool d_tests_ptr_array_run_all(
    struct dj_test_suite* _suite
)
{
    bool result;

    result = true;

    // run all test categories
    result = d_tests_ptr_array_constructor_all(_suite) && result;
    result = d_tests_ptr_array_element_all(_suite) && result;
    result = d_tests_ptr_array_destructor_all(_suite) && result;

    return result;
}
