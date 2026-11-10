/*******************************************************************************
* djinterp [container]                                          registry_tests.c
*
*
* path:      /tests/djinterp/c/container/registry/registry_tests.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.02.19
*                                                            revised: 2026.11.09
*******************************************************************************/
// djinterp
#include "./registry_tests.h"
#include "../../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's C face


/*
d_tests_registry_run_all
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
bool d_tests_registry_run_all(
    struct dj_test_suite* _suite
)
{
    bool result;

    result = true;

    // run all test categories
    result = d_tests_registry_constructors_all(_suite) && result;
    result = d_tests_registry_lookup_all(_suite) && result;
    result = d_tests_registry_row_manipulation_all(_suite) && result;
    result = d_tests_registry_aliases_all(_suite) && result;
    result = d_tests_registry_queries_all(_suite) && result;
    result = d_tests_registry_lookup_maint_all(_suite) && result;
    result = d_tests_registry_iterators_all(_suite) && result;
    result = d_tests_registry_utility_all(_suite) && result;
    result = d_tests_registry_destructor_all(_suite) && result;
    result = d_tests_registry_comparison_all(_suite) && result;
    result = d_tests_registry_common_all(_suite) && result;

    return result;
}
