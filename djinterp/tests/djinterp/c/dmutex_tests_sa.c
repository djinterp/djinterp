#include ".\dmutex_tests_sa.h"


/*
d_tests_sa_dmutex_run_all
  Module-level aggregation function that runs all dmutex tests.
  Executes tests for all categories:
  - Basic mutex operations
  - Recursive mutex operations
  - Thread operations
  - Thread-specific storage
  - Condition variables
  - Read-write locks
  - Synchronization primitives
  - Utility functions
*/
bool
d_tests_sa_dmutex_run_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    // run all test categories
    result = d_tests_sa_mutex_basic_all(_counter) && result;
    result = d_tests_sa_mutex_recursive_all(_counter) && result;
    result = d_tests_sa_thread_ops_all(_counter) && result;
    result = d_tests_sa_tss_all(_counter) && result;
    result = d_tests_sa_cond_all(_counter) && result;
    result = d_tests_sa_rwlock_all(_counter) && result;
    result = d_tests_sa_sync_all(_counter) && result;
    result = d_tests_sa_utility_all(_counter) && result;

    return result;
}
