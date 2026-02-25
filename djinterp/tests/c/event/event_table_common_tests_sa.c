#include "./event_table_common_tests_sa.h"


/*
d_tests_sa_event_table_common_all
  Module-level aggregation function that runs all event_table_common tests.
  Executes tests for all categories:
  - Constants and types
  - Hash functions (FNV-1a and simple)
  - Prime number utilities
  - Node management
  - Statistics printing
  - Integration, stress, and edge cases
*/
bool
d_tests_sa_event_table_common_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    // run all test categories
    result = d_tests_sa_event_hash_constant_all(_counter) && result;
    result = d_tests_sa_event_hash_all(_counter)          && result;
    result = d_tests_sa_event_hash_prime_all(_counter)    && result;
    result = d_tests_sa_event_hash_node_all(_counter)     && result;
    result = d_tests_sa_event_hash_stats_all(_counter)    && result;
    result = d_tests_sa_event_hash_advanced_all(_counter) && result;

    return result;
}
