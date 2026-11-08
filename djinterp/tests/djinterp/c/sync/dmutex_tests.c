/*******************************************************************************
* djinterp [test]                                                 dmutex_tests.c
*
*
* path:      /tests/djinterp/c/sync/dmutex_tests.c
* link(s):   TBA
* author(s): TBA                                             created: 2026.10.15
*                                                            revised: 2026.11.06
*******************************************************************************/
// djinterp
#include "dmutex_tests.h"


/*
d_tests_dmutex_run_all
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
bool d_tests_dmutex_run_all(
    struct dj_test_suite* _suite
)
{
    bool result;

    result = true;

    // run all test categories
    result = d_tests_mutex_basic_all(_suite) && result;
    result = d_tests_mutex_recursive_all(_suite) && result;
    result = d_tests_thread_ops_all(_suite) && result;
    result = d_tests_tss_all(_suite) && result;
    result = d_tests_cond_all(_suite) && result;
    result = d_tests_rwlock_all(_suite) && result;
    result = d_tests_sync_all(_suite) && result;
    result = d_tests_utility_all(_suite) && result;

    return result;
}
