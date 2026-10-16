#include "./datomic_tests_sa.h"


/*
d_tests_sa_datomic_run_all
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
bool
d_tests_sa_datomic_run_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    // run all test categories
    result = d_tests_sa_datomic_flag_all(_counter)             && result;
    result = d_tests_sa_datomic_init_all(_counter)             && result;
    result = d_tests_sa_datomic_load_all(_counter)             && result;
    result = d_tests_sa_datomic_store_all(_counter)            && result;
    result = d_tests_sa_datomic_exchange_all(_counter)         && result;
    result = d_tests_sa_datomic_compare_exchange_all(_counter) && result;
    result = d_tests_sa_datomic_fetch_all(_counter)            && result;
    result = d_tests_sa_datomic_fence_all(_counter)            && result;
    result = d_tests_sa_datomic_null_all(_counter)             && result;

    return result;
}
