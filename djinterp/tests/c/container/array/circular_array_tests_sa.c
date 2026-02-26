#include "./circular_array_tests_sa.h"


/*
d_tests_sa_circular_array_run_all
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
bool
d_tests_sa_circular_array_run_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    // run all test categories
    result = d_tests_sa_circular_array_constructor_all(_counter) && result;
    result = d_tests_sa_circular_array_access_all(_counter) && result;
    result = d_tests_sa_circular_array_modification_all(_counter) && result;
    result = d_tests_sa_circular_array_bulk_all(_counter) && result;
    result = d_tests_sa_circular_array_query_all(_counter) && result;
    result = d_tests_sa_circular_array_search_all(_counter) && result;
    result = d_tests_sa_circular_array_conversion_all(_counter) && result;
    result = d_tests_sa_circular_array_utility_all(_counter) && result;

    return result;
}
