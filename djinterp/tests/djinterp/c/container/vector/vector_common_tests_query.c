/*******************************************************************************
* djinterp [container]                               vector_common_tests_query.c
*
*
* path:      /tests/djinterp/c/container/vector/vector_common_tests_query.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.02.25
*                                                            revised: 2026.11.09
*******************************************************************************/
// djinterp
#include "./vector_common_tests.h"
#include "../../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's C face


/*
d_tests_vector_common_is_empty
  Tests the d_vector_common_is_empty function.
  Tests the following:
  - zero count returns true
  - non-zero count returns false
*/
bool d_tests_vector_common_is_empty(
    struct dj_test_suite* _suite
)
{
    bool result;

    d_test_suite_test(_suite, "vector_common_is_empty");

    result = true;

    // test 1: zero count should return true
    result = D_TEST_CHECK_MSG(_suite,
                              "is_empty_zero",
                              d_vector_common_is_empty(0) == true,
                              "Zero count should return true (empty)") && result;

    // test 2: non-zero count should return false
    result = D_TEST_CHECK_MSG(_suite,
                              "is_empty_one",
                              d_vector_common_is_empty(1) == false,
                              "Count of 1 should return false (not empty)") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "is_empty_many",
                              d_vector_common_is_empty(100) == false,
                              "Count of 100 should return false (not empty)") && result;

    return result;
}


/*
d_tests_vector_common_is_full
  Tests the d_vector_common_is_full function.
  Tests the following:
  - count < capacity returns false
  - count == capacity returns true
  - count > capacity returns true (edge case)
*/
bool d_tests_vector_common_is_full(
    struct dj_test_suite* _suite
)
{
    bool result;

    d_test_suite_test(_suite, "vector_common_is_full");

    result = true;

    // test 1: count < capacity should return false
    result = D_TEST_CHECK_MSG(_suite,
                              "is_full_not_full",
                              d_vector_common_is_full(5, 10) == false,
                              "Count < capacity should return false (not full)") && result;

    // test 2: count == capacity should return true
    result = D_TEST_CHECK_MSG(_suite,
                              "is_full_equal",
                              d_vector_common_is_full(10, 10) == true,
                              "Count == capacity should return true (full)") && result;

    // test 3: count > capacity (edge case) should return true
    result = D_TEST_CHECK_MSG(_suite,
                              "is_full_overflow",
                              d_vector_common_is_full(15, 10) == true,
                              "Count > capacity should return true") && result;

    // test 4: both zero should return true
    result = D_TEST_CHECK_MSG(_suite,
                              "is_full_both_zero",
                              d_vector_common_is_full(0, 0) == true,
                              "Both zero should return true (0 >= 0)") && result;

    // test 5: count zero, capacity non-zero should return false
    result = D_TEST_CHECK_MSG(_suite,
                              "is_full_empty",
                              d_vector_common_is_full(0, 10) == false,
                              "Empty vector with capacity should return false") && result;

    return result;
}


/*
d_tests_vector_common_size
  Tests the d_vector_common_size function.
  Tests the following:
  - returns count value as-is
*/
bool d_tests_vector_common_size(
    struct dj_test_suite* _suite
)
{
    bool result;

    d_test_suite_test(_suite, "vector_common_size");

    result = true;

    // test 1: zero count
    result = D_TEST_CHECK_MSG(_suite,
                              "size_zero",
                              d_vector_common_size(0) == 0,
                              "Size of 0 should return 0") && result;

    // test 2: non-zero count
    result = D_TEST_CHECK_MSG(_suite,
                              "size_nonzero",
                              d_vector_common_size(42) == 42,
                              "Size of 42 should return 42") && result;

    // test 3: large count
    result = D_TEST_CHECK_MSG(_suite,
                              "size_large",
                              d_vector_common_size(1000000) == 1000000,
                              "Size of 1000000 should return 1000000") && result;

    return result;
}


/*
d_tests_vector_common_capacity_query
  Tests the d_vector_common_capacity function.
  Tests the following:
  - returns capacity value as-is
*/
bool d_tests_vector_common_capacity_query(
    struct dj_test_suite* _suite
)
{
    bool result;

    d_test_suite_test(_suite, "vector_common_capacity_query");

    result = true;

    // test 1: zero capacity
    result = D_TEST_CHECK_MSG(_suite,
                              "capacity_zero",
                              d_vector_common_capacity(0) == 0,
                              "Capacity of 0 should return 0") && result;

    // test 2: non-zero capacity
    result = D_TEST_CHECK_MSG(_suite,
                              "capacity_nonzero",
                              d_vector_common_capacity(64) == 64,
                              "Capacity of 64 should return 64") && result;

    // test 3: large capacity
    result = D_TEST_CHECK_MSG(_suite,
                              "capacity_large",
                              d_vector_common_capacity(1000000) == 1000000,
                              "Capacity of 1000000 should return 1000000") && result;

    return result;
}


/*
d_tests_vector_common_query_all
  Aggregation function that runs all query tests.
*/
bool d_tests_vector_common_query_all(
    struct dj_test_suite* _suite
)
{
    bool result;

    d_test_suite_block(_suite, "vector_common_query_all");

    result = true;

    result = d_tests_vector_common_is_empty(_suite) && result;
    result = d_tests_vector_common_is_full(_suite) && result;
    result = d_tests_vector_common_size(_suite) && result;
    result = d_tests_vector_common_capacity_query(_suite) && result;

    return result;
}
