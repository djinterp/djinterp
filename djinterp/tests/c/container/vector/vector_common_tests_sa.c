#include "./vector_common_tests_sa.h"


/*
d_tests_sa_vector_common_run_all
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
bool
d_tests_sa_vector_common_run_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    // run all test categories
    result = d_tests_sa_vector_common_init_all(_counter) && result;
    result = d_tests_sa_vector_common_capacity_all(_counter) && result;
    result = d_tests_sa_vector_common_element_all(_counter) && result;
    result = d_tests_sa_vector_common_append_all(_counter) && result;
    result = d_tests_sa_vector_common_resize_all(_counter) && result;
    result = d_tests_sa_vector_common_access_all(_counter) && result;
    result = d_tests_sa_vector_common_query_all(_counter) && result;
    result = d_tests_sa_vector_common_utility_all(_counter) && result;
    result = d_tests_sa_vector_common_cleanup_all(_counter) && result;

    return result;
}
