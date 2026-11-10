/*******************************************************************************
* djinterp [container]                                            vector_tests.c
*
*
* path:      /tests/djinterp/c/container/vector/vector_tests.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.02.25
*                                                            revised: 2026.11.09
*******************************************************************************/
// djinterp
#include "./vector_tests.h"
#include "../../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's C face


/*
d_tests_vector_run_all
  Module-level aggregation function that runs all vector tests.
  Executes tests for all categories:
  - Constructor functions
  - Capacity management functions
  - Element manipulation functions
  - Append/prepend functions
  - Resize functions
  - Access functions
  - Query functions
  - Search functions
  - Utility functions
  - Destructor functions
*/
bool d_tests_vector_run_all(
    struct dj_test_suite* _suite
)
{
    bool result;

    result = true;

    // run all test categories
    result = d_tests_vector_constructor_all(_suite) && result;
    result = d_tests_vector_capacity_all(_suite) && result;
    result = d_tests_vector_element_all(_suite) && result;
    result = d_tests_vector_append_all(_suite) && result;
    result = d_tests_vector_resize_all(_suite) && result;
    result = d_tests_vector_access_all(_suite) && result;
    result = d_tests_vector_query_all(_suite) && result;
    result = d_tests_vector_search_all(_suite) && result;
    result = d_tests_vector_utility_all(_suite) && result;
    result = d_tests_vector_destructor_all(_suite) && result;

    return result;
}
