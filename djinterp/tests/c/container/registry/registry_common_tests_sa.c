#include ".\registry_common_tests_sa.h"


/*
d_tests_sa_registry_common_run_all
  Module-level aggregation function that runs all registry_common tests.
  Executes tests for all categories:
  - String comparison functions (d_registry_strcmp)
  - Schema max enum key functions (d_registry_schema_max_enum_key)
*/
bool
d_tests_sa_registry_common_run_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    // run all test categories
    result = d_tests_sa_registry_strcmp_all(_counter) && result;
    result = d_tests_sa_registry_schema_max_enum_key_all(_counter) && result;

    return result;
}
