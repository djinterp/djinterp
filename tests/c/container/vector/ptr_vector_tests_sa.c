/******************************************************************************
* djinterp [test]                                          ptr_vector_tests_sa.c
*
*   Module-level aggregation for ptr_vector unit tests.
*
* path:      \tests\container\vector\ptr_vector_tests_sa.c
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.01.29
******************************************************************************/

#include "./ptr_vector_tests_sa.h"


/*
d_tests_sa_ptr_vector_run_all
  Module-level aggregation function that runs all ptr_vector tests.
*/
bool
d_tests_sa_ptr_vector_run_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    result = d_tests_sa_ptr_vector_constructor_all(_counter) && result;
    result = d_tests_sa_ptr_vector_capacity_all(_counter) && result;
    result = d_tests_sa_ptr_vector_element_all(_counter) && result;
    result = d_tests_sa_ptr_vector_append_all(_counter) && result;
    result = d_tests_sa_ptr_vector_resize_all(_counter) && result;
    result = d_tests_sa_ptr_vector_access_all(_counter) && result;
    result = d_tests_sa_ptr_vector_query_all(_counter) && result;
    result = d_tests_sa_ptr_vector_search_all(_counter) && result;
    result = d_tests_sa_ptr_vector_utility_all(_counter) && result;
    result = d_tests_sa_ptr_vector_iteration_all(_counter) && result;
    result = d_tests_sa_ptr_vector_destructor_all(_counter) && result;

    return result;
}
