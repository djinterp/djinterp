/*******************************************************************************
* djinterp [container]                               buffer_common_tests_state.c
*
*
* path:      /tests/djinterp/c/container/buffer/buffer_common_tests_state.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.15
*                                                            revised: 2026.11.09
*******************************************************************************/
// djinterp
#include "./buffer_common_tests.h"
#include "../../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's C face


/*
d_tests_buffer_common_clear
  Tests the d_buffer_common_clear function.
  Tests the following:
  - NULL count does not crash
  - count is set to zero
*/
bool d_tests_buffer_common_clear(
    struct dj_test_suite* _suite
)
{
    bool   result;
    size_t count;

    d_test_suite_test(_suite, "buffer_common_clear");

    result = true;

    // test 1: NULL count should not crash
    d_buffer_common_clear(NULL);
    result = D_TEST_CHECK_MSG(_suite,
                              "clear_null_count",
                              true,
                              "NULL count should not crash") && result;

    // test 2: count set to zero
    count = 42;
    d_buffer_common_clear(&count);
    result = D_TEST_CHECK_MSG(_suite,
                              "clear_zeroed",
                              count == 0,
                              "Count should be 0 after clear") && result;

    return result;
}


/*
d_tests_buffer_common_is_empty
  Tests the d_buffer_common_is_empty function.
  Tests the following:
  - zero count returns true
  - non-zero count returns false
*/
bool d_tests_buffer_common_is_empty(
    struct dj_test_suite* _suite
)
{
    bool result;

    d_test_suite_test(_suite, "buffer_common_is_empty");

    result = true;

    // test 1: zero count is empty
    result = D_TEST_CHECK_MSG(_suite,
                              "is_empty_zero",
                              d_buffer_common_is_empty(0) == true,
                              "Zero count should return true") && result;

    // test 2: non-zero count is not empty
    result = D_TEST_CHECK_MSG(_suite,
                              "is_empty_nonzero",
                              d_buffer_common_is_empty(5) == false,
                              "Non-zero count should return false") && result;

    return result;
}


/*
d_tests_buffer_common_is_full
  Tests the d_buffer_common_is_full function.
  Tests the following:
  - count < capacity returns false
  - count == capacity returns true
  - count > capacity returns true
*/
bool d_tests_buffer_common_is_full(
    struct dj_test_suite* _suite
)
{
    bool result;

    d_test_suite_test(_suite, "buffer_common_is_full");

    result = true;

    // test 1: count < capacity
    result = D_TEST_CHECK_MSG(_suite,
                              "is_full_under",
                              d_buffer_common_is_full(3, 10) == false,
                              "count < capacity should return false") && result;

    // test 2: count == capacity
    result = D_TEST_CHECK_MSG(_suite,
                              "is_full_equal",
                              d_buffer_common_is_full(10, 10) == true,
                              "count == capacity should return true") && result;

    // test 3: count > capacity (edge case)
    result = D_TEST_CHECK_MSG(_suite,
                              "is_full_over",
                              d_buffer_common_is_full(15, 10) == true,
                              "count > capacity should return true") && result;

    return result;
}


/*
d_tests_buffer_common_utilization
  Tests the d_buffer_common_utilization function.
  Tests the following:
  - zero capacity returns 0.0
  - zero count returns 0.0
  - half-full returns approximately 0.5
  - full returns 1.0
*/
bool d_tests_buffer_common_utilization(
    struct dj_test_suite* _suite
)
{
    bool   result;
    double util;

    d_test_suite_test(_suite, "buffer_common_utilization");

    result = true;

    // test 1: zero capacity
    result = D_TEST_CHECK_MSG(_suite,
                              "utilization_zero_cap",
                              d_buffer_common_utilization(5, 0) == 0.0,
                              "Zero capacity should return 0.0") && result;

    // test 2: zero count
    util   = d_buffer_common_utilization(0, 10);
    result = D_TEST_CHECK_MSG(_suite,
                              "utilization_zero_count",
                              util == 0.0,
                              "Zero count should return 0.0") && result;

    // test 3: half-full
    util   = d_buffer_common_utilization(5, 10);
    result = D_TEST_CHECK_MSG(_suite,
                              "utilization_half",
                              util > 0.49 && util < 0.51,
                              "5/10 should be approximately 0.5") && result;

    // test 4: full
    util   = d_buffer_common_utilization(10, 10);
    result = D_TEST_CHECK_MSG(_suite,
                              "utilization_full",
                              util == 1.0,
                              "10/10 should return 1.0") && result;

    return result;
}


/*
d_tests_buffer_common_state_all
  Aggregation function that runs all state query tests.
*/
bool d_tests_buffer_common_state_all(
    struct dj_test_suite* _suite
)
{
    bool result;

    d_test_suite_block(_suite, "buffer_common_state_all");

    result = true;

    result = d_tests_buffer_common_clear(_suite) && result;
    result = d_tests_buffer_common_is_empty(_suite) && result;
    result = d_tests_buffer_common_is_full(_suite) && result;
    result = d_tests_buffer_common_utilization(_suite) && result;

    return result;
}
