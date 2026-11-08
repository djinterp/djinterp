/*******************************************************************************
* djinterp [test]                                                datomic_tests.c
*
*
* path:      /tests/djinterp/c/sync/datomic_tests.c
* link(s):   TBA
* author(s): TBA                                             created: 2026.10.15
*                                                            revised: 2026.11.06
*******************************************************************************/
// djinterp
#include "datomic_tests.h"


/*
d_tests_datomic_run_all
  Module-level aggregation function that runs all atomic tests.
  Executes tests for all categories:
  - Atomic flag operations
  - Atomic initialization
  - Atomic load operations
  - Atomic store operations
  - Atomic exchange operations
  - Atomic compare-and-exchange
  - Atomic fetch-and-modify operations
  - Memory fences and utilities
  - Null-safety guards
*/
bool d_tests_datomic_run_all(
    struct dj_test_suite* _suite
)
{
    bool result;

    result = true;

    // run all test categories
    result = d_tests_datomic_flag_all(_suite)             && result;
    result = d_tests_datomic_init_all(_suite)             && result;
    result = d_tests_datomic_load_all(_suite)             && result;
    result = d_tests_datomic_store_all(_suite)            && result;
    result = d_tests_datomic_exchange_all(_suite)         && result;
    result = d_tests_datomic_compare_exchange_all(_suite) && result;
    result = d_tests_datomic_fetch_all(_suite)            && result;
    result = d_tests_datomic_fence_all(_suite)            && result;
    result = d_tests_datomic_null_all(_suite)             && result;

    return result;
}
