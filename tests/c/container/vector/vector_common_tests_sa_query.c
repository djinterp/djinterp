#include "./vector_common_tests_sa.h"


/*
d_tests_sa_vector_common_is_empty
  Tests the d_vector_common_is_empty function.
  Tests the following:
  - zero count returns true
  - non-zero count returns false
*/
bool
d_tests_sa_vector_common_is_empty
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    // test 1: zero count should return true
    result = d_assert_standalone(
        d_vector_common_is_empty(0) == true,
        "is_empty_zero",
        "Zero count should return true (empty)",
        _counter) && result;

    // test 2: non-zero count should return false
    result = d_assert_standalone(
        d_vector_common_is_empty(1) == false,
        "is_empty_one",
        "Count of 1 should return false (not empty)",
        _counter) && result;

    result = d_assert_standalone(
        d_vector_common_is_empty(100) == false,
        "is_empty_many",
        "Count of 100 should return false (not empty)",
        _counter) && result;

    return result;
}


/*
d_tests_sa_vector_common_is_full
  Tests the d_vector_common_is_full function.
  Tests the following:
  - count < capacity returns false
  - count == capacity returns true
  - count > capacity returns true (edge case)
*/
bool
d_tests_sa_vector_common_is_full
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    // test 1: count < capacity should return false
    result = d_assert_standalone(
        d_vector_common_is_full(5, 10) == false,
        "is_full_not_full",
        "Count < capacity should return false (not full)",
        _counter) && result;

    // test 2: count == capacity should return true
    result = d_assert_standalone(
        d_vector_common_is_full(10, 10) == true,
        "is_full_equal",
        "Count == capacity should return true (full)",
        _counter) && result;

    // test 3: count > capacity (edge case) should return true
    result = d_assert_standalone(
        d_vector_common_is_full(15, 10) == true,
        "is_full_overflow",
        "Count > capacity should return true",
        _counter) && result;

    // test 4: both zero should return true
    result = d_assert_standalone(
        d_vector_common_is_full(0, 0) == true,
        "is_full_both_zero",
        "Both zero should return true (0 >= 0)",
        _counter) && result;

    // test 5: count zero, capacity non-zero should return false
    result = d_assert_standalone(
        d_vector_common_is_full(0, 10) == false,
        "is_full_empty",
        "Empty vector with capacity should return false",
        _counter) && result;

    return result;
}


/*
d_tests_sa_vector_common_size
  Tests the d_vector_common_size function.
  Tests the following:
  - returns count value as-is
*/
bool
d_tests_sa_vector_common_size
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    // test 1: zero count
    result = d_assert_standalone(
        d_vector_common_size(0) == 0,
        "size_zero",
        "Size of 0 should return 0",
        _counter) && result;

    // test 2: non-zero count
    result = d_assert_standalone(
        d_vector_common_size(42) == 42,
        "size_nonzero",
        "Size of 42 should return 42",
        _counter) && result;

    // test 3: large count
    result = d_assert_standalone(
        d_vector_common_size(1000000) == 1000000,
        "size_large",
        "Size of 1000000 should return 1000000",
        _counter) && result;

    return result;
}


/*
d_tests_sa_vector_common_capacity_query
  Tests the d_vector_common_capacity function.
  Tests the following:
  - returns capacity value as-is
*/
bool
d_tests_sa_vector_common_capacity_query
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    // test 1: zero capacity
    result = d_assert_standalone(
        d_vector_common_capacity(0) == 0,
        "capacity_zero",
        "Capacity of 0 should return 0",
        _counter) && result;

    // test 2: non-zero capacity
    result = d_assert_standalone(
        d_vector_common_capacity(64) == 64,
        "capacity_nonzero",
        "Capacity of 64 should return 64",
        _counter) && result;

    // test 3: large capacity
    result = d_assert_standalone(
        d_vector_common_capacity(1000000) == 1000000,
        "capacity_large",
        "Capacity of 1000000 should return 1000000",
        _counter) && result;

    return result;
}


/*
d_tests_sa_vector_common_query_all
  Aggregation function that runs all query tests.
*/
bool
d_tests_sa_vector_common_query_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Query Functions\n");
    printf("  --------------------------\n");

    result = d_tests_sa_vector_common_is_empty(_counter) && result;
    result = d_tests_sa_vector_common_is_full(_counter) && result;
    result = d_tests_sa_vector_common_size(_counter) && result;
    result = d_tests_sa_vector_common_capacity_query(_counter) && result;

    return result;
}
