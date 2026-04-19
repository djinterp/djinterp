#include "./ptr_array_tests_sa.h"


/*
d_tests_sa_ptr_array_run_all
  Module-level aggregation function that runs all ptr_array tests.
  Executes tests for all categories:
  - Constructor functions
  - Element manipulation functions
  - Destructor functions
*/
bool
d_tests_sa_ptr_array_run_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    // run all test categories
    result = d_tests_sa_ptr_array_constructor_all(_counter) && result;
    result = d_tests_sa_ptr_array_element_all(_counter) && result;
    result = d_tests_sa_ptr_array_destructor_all(_counter) && result;

    return result;
}
