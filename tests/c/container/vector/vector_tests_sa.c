#include "./vector_tests_sa.h"


/*
d_tests_sa_vector_run_all
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
bool
d_tests_sa_vector_run_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    // run all test categories
    result = d_tests_sa_vector_constructor_all(_counter) && result;
    result = d_tests_sa_vector_capacity_all(_counter) && result;
    result = d_tests_sa_vector_element_all(_counter) && result;
    result = d_tests_sa_vector_append_all(_counter) && result;
    result = d_tests_sa_vector_resize_all(_counter) && result;
    result = d_tests_sa_vector_access_all(_counter) && result;
    result = d_tests_sa_vector_query_all(_counter) && result;
    result = d_tests_sa_vector_search_all(_counter) && result;
    result = d_tests_sa_vector_utility_all(_counter) && result;
    result = d_tests_sa_vector_destructor_all(_counter) && result;

    return result;
}
