/*******************************************************************************
* djinterp [container]                                    circular_array_tests.c
*
*
* path:      /tests/djinterp/c/container/array/circular_array_tests.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.02.25
*                                                            revised: 2026.11.09
*******************************************************************************/
// djinterp
#include "./circular_array_tests.h"
#include "../../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's C face


/*
d_tests_circular_array_run_all
  Module-level aggregation function that runs all circular_array tests.
  Executes tests for all categories:
  - Constructor functions
  - Element access functions
  - Modification functions (push/pop)
  - Bulk operation functions
  - Query functions
  - Search functions
  - Conversion functions
  - Utility and memory management functions
*/
bool d_tests_circular_array_run_all(
    struct dj_test_suite* _suite
)
{
    bool result;

    result = true;

    // run all test categories
    result = d_tests_circular_array_constructor_all(_suite) && result;
    result = d_tests_circular_array_access_all(_suite) && result;
    result = d_tests_circular_array_modification_all(_suite) && result;
    result = d_tests_circular_array_bulk_all(_suite) && result;
    result = d_tests_circular_array_query_all(_suite) && result;
    result = d_tests_circular_array_search_all(_suite) && result;
    result = d_tests_circular_array_conversion_all(_suite) && result;
    result = d_tests_circular_array_utility_all(_suite) && result;

    return result;
}
