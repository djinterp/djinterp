/*******************************************************************************
* djinterp [test]                                          ptr_vector_tests_sa.c
*
*   Module-level aggregation for ptr_vector unit tests.
*
*
* path:      /tests/djinterp/c/container/vector/ptr_vector_tests_sa.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.01.29
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "./ptr_vector_tests_sa.h"
#include "../../../../../inc/djinterp/test/c/test_counter.h"


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
