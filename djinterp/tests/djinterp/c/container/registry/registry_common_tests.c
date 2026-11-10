/*******************************************************************************
* djinterp [container]                                   registry_common_tests.c
*
*
* path:      /tests/djinterp/c/container/registry/registry_common_tests.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.02.19
*                                                            revised: 2026.11.09
*******************************************************************************/
// djinterp
#include "./registry_common_tests.h"
#include "../../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's C face


/*
d_tests_registry_common_run_all
  Module-level aggregation function that runs all registry_common tests.
  Executes tests for all categories:
  - String comparison functions (d_registry_strcmp)
  - Schema max enum key functions (d_registry_schema_max_enum_key)
*/
bool d_tests_registry_common_run_all(
    struct dj_test_suite* _suite
)
{
    bool result;

    result = true;

    // run all test categories
    result = d_tests_registry_strcmp_all(_suite) && result;
    result = d_tests_registry_schema_max_enum_key_all(_suite) && result;

    return result;
}
