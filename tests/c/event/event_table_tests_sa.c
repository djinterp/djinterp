#include "./event_table_tests_sa.h"


/*
d_tests_sa_event_hash_table_all
  Module-level aggregation function that runs all event_table tests.
  Executes tests for all categories:
  - Creation and destruction
  - Core operations (insert, lookup, remove, contains)
  - Table management (size, count, load factor, resize, clear)
  - Iterator traversal
  - Statistics reporting
  - Integration, stress, and edge cases
*/
bool
d_tests_sa_event_hash_table_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    result = d_tests_sa_et_creation_all(_counter)   && result;
    result = d_tests_sa_et_operations_all(_counter)  && result;
    result = d_tests_sa_et_management_all(_counter)  && result;
    result = d_tests_sa_et_iterator_all(_counter)    && result;
    result = d_tests_sa_et_statistics_all(_counter)  && result;
    result = d_tests_sa_et_advanced_all(_counter)    && result;

    return result;
}
