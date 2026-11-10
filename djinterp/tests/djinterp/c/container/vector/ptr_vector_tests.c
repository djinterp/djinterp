/*******************************************************************************
* djinterp [test]                                             ptr_vector_tests.c
*
*   Module-level aggregation for ptr_vector unit tests.
*
*
* path:      /tests/djinterp/c/container/vector/ptr_vector_tests.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.01.29
*                                                            revised: 2026.11.09
*******************************************************************************/

// djinterp
#include "./ptr_vector_tests.h"
#include "../../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's C face


/*
d_tests_ptr_vector_run_all
  Module-level aggregation function that runs all ptr_vector tests.
*/
bool d_tests_ptr_vector_run_all(
    struct dj_test_suite* _suite
)
{
    bool result;

    result = true;

    result = d_tests_ptr_vector_constructor_all(_suite) && result;
    result = d_tests_ptr_vector_capacity_all(_suite) && result;
    result = d_tests_ptr_vector_element_all(_suite) && result;
    result = d_tests_ptr_vector_append_all(_suite) && result;
    result = d_tests_ptr_vector_resize_all(_suite) && result;
    result = d_tests_ptr_vector_access_all(_suite) && result;
    result = d_tests_ptr_vector_query_all(_suite) && result;
    result = d_tests_ptr_vector_search_all(_suite) && result;
    result = d_tests_ptr_vector_utility_all(_suite) && result;
    result = d_tests_ptr_vector_iteration_all(_suite) && result;
    result = d_tests_ptr_vector_destructor_all(_suite) && result;

    return result;
}
