/*******************************************************************************
* djinterp [container]                            buffer_common_tests_sa_state.c
*
*
* path:      /tests/djinterp/c/container/buffer/buffer_common_tests_sa_state.c
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "./buffer_common_tests_sa.h"
#include "../../../../../inc/djinterp/test/c/test_counter.h"


/*
d_tests_sa_buffer_common_clear
  Tests the d_buffer_common_clear function.
  Tests the following:
  - NULL count does not crash
  - count is set to zero
*/
bool
d_tests_sa_buffer_common_clear
(
    struct d_test_counter* _counter
)
{
    bool   result;
    size_t count;

    result = true;

    // test 1: NULL count should not crash
    d_buffer_common_clear(NULL);
    result = d_assert_standalone(
        true,
        "clear_null_count",
        "NULL count should not crash",
        _counter) && result;

    // test 2: count set to zero
    count = 42;
    d_buffer_common_clear(&count);
    result = d_assert_standalone(
        count == 0,
        "clear_zeroed",
        "Count should be 0 after clear",
        _counter) && result;

    return result;
}


/*
d_tests_sa_buffer_common_is_empty
  Tests the d_buffer_common_is_empty function.
  Tests the following:
  - zero count returns true
  - non-zero count returns false
*/
bool
d_tests_sa_buffer_common_is_empty
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    // test 1: zero count is empty
    result = d_assert_standalone(
        d_buffer_common_is_empty(0) == true,
        "is_empty_zero",
        "Zero count should return true",
        _counter) && result;

    // test 2: non-zero count is not empty
    result = d_assert_standalone(
        d_buffer_common_is_empty(5) == false,
        "is_empty_nonzero",
        "Non-zero count should return false",
        _counter) && result;

    return result;
}


/*
d_tests_sa_buffer_common_is_full
  Tests the d_buffer_common_is_full function.
  Tests the following:
  - count < capacity returns false
  - count == capacity returns true
  - count > capacity returns true
*/
bool
d_tests_sa_buffer_common_is_full
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    // test 1: count < capacity
    result = d_assert_standalone(
        d_buffer_common_is_full(3, 10) == false,
        "is_full_under",
        "count < capacity should return false",
        _counter) && result;

    // test 2: count == capacity
    result = d_assert_standalone(
        d_buffer_common_is_full(10, 10) == true,
        "is_full_equal",
        "count == capacity should return true",
        _counter) && result;

    // test 3: count > capacity (edge case)
    result = d_assert_standalone(
        d_buffer_common_is_full(15, 10) == true,
        "is_full_over",
        "count > capacity should return true",
        _counter) && result;

    return result;
}


/*
d_tests_sa_buffer_common_utilization
  Tests the d_buffer_common_utilization function.
  Tests the following:
  - zero capacity returns 0.0
  - zero count returns 0.0
  - half-full returns approximately 0.5
  - full returns 1.0
*/
bool
d_tests_sa_buffer_common_utilization
(
    struct d_test_counter* _counter
)
{
    bool   result;
    double util;

    result = true;

    // test 1: zero capacity
    result = d_assert_standalone(
        d_buffer_common_utilization(5, 0) == 0.0,
        "utilization_zero_cap",
        "Zero capacity should return 0.0",
        _counter) && result;

    // test 2: zero count
    util   = d_buffer_common_utilization(0, 10);
    result = d_assert_standalone(
        util == 0.0,
        "utilization_zero_count",
        "Zero count should return 0.0",
        _counter) && result;

    // test 3: half-full
    util   = d_buffer_common_utilization(5, 10);
    result = d_assert_standalone(
        util > 0.49 && util < 0.51,
        "utilization_half",
        "5/10 should be approximately 0.5",
        _counter) && result;

    // test 4: full
    util   = d_buffer_common_utilization(10, 10);
    result = d_assert_standalone(
        util == 1.0,
        "utilization_full",
        "10/10 should return 1.0",
        _counter) && result;

    return result;
}


/*
d_tests_sa_buffer_common_state_all
  Aggregation function that runs all state query tests.
*/
bool
d_tests_sa_buffer_common_state_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] State Query Functions\n");
    printf("  --------------------------------\n");

    result = d_tests_sa_buffer_common_clear(_counter) && result;
    result = d_tests_sa_buffer_common_is_empty(_counter) && result;
    result = d_tests_sa_buffer_common_is_full(_counter) && result;
    result = d_tests_sa_buffer_common_utilization(_counter) && result;

    return result;
}
