#include ".\registry_tests_sa.h"


/*
d_tests_sa_registry_run_all
  Module-level aggregation function that runs all registry tests.
  Executes tests for all categories:
  - Constructor functions
  - Primary lookup function
  - Row manipulation functions
  - Alias functions
  - Query functions
  - Lookup maintenance functions
  - Iterator functions
  - Utility functions
  - Destructor functions
  - Internal comparison functions
  - Registry common functions
*/
bool
d_tests_sa_registry_run_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    // run all test categories
    result = d_tests_sa_registry_constructors_all(_counter) && result;
    result = d_tests_sa_registry_lookup_all(_counter) && result;
    result = d_tests_sa_registry_row_manipulation_all(_counter) && result;
    result = d_tests_sa_registry_aliases_all(_counter) && result;
    result = d_tests_sa_registry_queries_all(_counter) && result;
    result = d_tests_sa_registry_lookup_maint_all(_counter) && result;
    result = d_tests_sa_registry_iterators_all(_counter) && result;
    result = d_tests_sa_registry_utility_all(_counter) && result;
    result = d_tests_sa_registry_destructor_all(_counter) && result;
    result = d_tests_sa_registry_comparison_all(_counter) && result;
    result = d_tests_sa_registry_common_all(_counter) && result;

    return result;
}
