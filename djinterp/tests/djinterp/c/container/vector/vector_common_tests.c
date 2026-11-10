/*******************************************************************************
* djinterp [container]                                     vector_common_tests.c
*
*
* path:      /tests/djinterp/c/container/vector/vector_common_tests.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.02.25
*                                                            revised: 2026.11.09
*******************************************************************************/
// djinterp
#include "./vector_common_tests.h"
#include "../../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's C face


/*
d_tests_vector_common_run_all
  Module-level aggregation function that runs all vector_common tests.
  Executes tests for all categories:
  - Initialization functions
  - Capacity management functions
  - Element manipulation functions
  - Append/extend functions
  - Resize functions
  - Access functions
  - Query functions
  - Utility functions
  - Cleanup functions
*/
bool d_tests_vector_common_run_all(
    struct dj_test_suite* _suite
)
{
    bool result;

    result = true;

    // run all test categories
    result = d_tests_vector_common_init_all(_suite) && result;
    result = d_tests_vector_common_capacity_all(_suite) && result;
    result = d_tests_vector_common_element_all(_suite) && result;
    result = d_tests_vector_common_append_all(_suite) && result;
    result = d_tests_vector_common_resize_all(_suite) && result;
    result = d_tests_vector_common_access_all(_suite) && result;
    result = d_tests_vector_common_query_all(_suite) && result;
    result = d_tests_vector_common_utility_all(_suite) && result;
    result = d_tests_vector_common_cleanup_all(_suite) && result;

    return result;
}
