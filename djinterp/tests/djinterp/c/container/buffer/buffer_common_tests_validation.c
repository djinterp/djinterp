/*******************************************************************************
* djinterp [container]                          buffer_common_tests_validation.c
*
*
* path:      /tests/djinterp/c/container/buffer/buffer_common_tests_validation.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.15
*                                                            revised: 2026.11.09
*******************************************************************************/
// djinterp
#include "./buffer_common_tests.h"
#include "../../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's C face


/*
d_tests_buffer_common_validate_params
  Tests the d_buffer_common_validate_params function.
  Tests the following:
  - zero element_size returns false
  - positive element_size returns true
  - various valid sizes return true
*/
bool d_tests_buffer_common_validate_params(
    struct dj_test_suite* _suite
)
{
    bool result;

    d_test_suite_test(_suite, "buffer_common_validate_params");

    result = true;

    // test 1: zero element_size
    result = D_TEST_CHECK_MSG(_suite,
                              "validate_params_zero",
                              d_buffer_common_validate_params(0) == false,
                              "Zero element_size should return false") && result;

    // test 2: sizeof(int) is valid
    result = D_TEST_CHECK_MSG(_suite,
                              "validate_params_int",
                              d_buffer_common_validate_params(sizeof(int)) == true,
                              "sizeof(int) should return true") && result;

    // test 3: sizeof(char) is valid
    result = D_TEST_CHECK_MSG(_suite,
                              "validate_params_char",
                              d_buffer_common_validate_params(sizeof(char)) == true,
                              "sizeof(char) should return true") && result;

    // test 4: large size is valid
    result = D_TEST_CHECK_MSG(_suite,
                              "validate_params_large",
                              d_buffer_common_validate_params(1024) == true,
                              "Large element_size should return true") && result;

    return result;
}


/*
d_tests_buffer_common_validate_capacity
  Tests the d_buffer_common_validate_capacity function.
  Tests the following:
  - zero capacity returns false
  - positive capacity returns true
*/
bool d_tests_buffer_common_validate_capacity(
    struct dj_test_suite* _suite
)
{
    bool result;

    d_test_suite_test(_suite, "buffer_common_validate_capacity");

    result = true;

    // test 1: zero capacity
    result = D_TEST_CHECK_MSG(_suite,
                              "validate_capacity_zero",
                              d_buffer_common_validate_capacity(0) == false,
                              "Zero capacity should return false") && result;

    // test 2: positive capacity
    result = D_TEST_CHECK_MSG(_suite,
                              "validate_capacity_one",
                              d_buffer_common_validate_capacity(1) == true,
                              "Capacity=1 should return true") && result;

    // test 3: large capacity
    result = D_TEST_CHECK_MSG(_suite,
                              "validate_capacity_large",
                              d_buffer_common_validate_capacity(100000) == true,
                              "Large capacity should return true") && result;

    return result;
}


/*
d_tests_buffer_common_validate_index
  Tests the d_buffer_common_validate_index function.
  Tests the following:
  - index within bounds returns true
  - index at boundary returns false
  - index beyond bounds returns false
  - zero count always returns false
  - first and last valid indices
*/
bool d_tests_buffer_common_validate_index(
    struct dj_test_suite* _suite
)
{
    bool result;

    d_test_suite_test(_suite, "buffer_common_validate_index");

    result = true;

    // test 1: index 0 with count 5 is valid
    result = D_TEST_CHECK_MSG(_suite,
                              "validate_index_first",
                              d_buffer_common_validate_index(0, 5) == true,
                              "Index 0, count 5 should return true") && result;

    // test 2: last valid index
    result = D_TEST_CHECK_MSG(_suite,
                              "validate_index_last",
                              d_buffer_common_validate_index(4, 5) == true,
                              "Index 4, count 5 should return true") && result;

    // test 3: index at boundary (equal to count)
    result = D_TEST_CHECK_MSG(_suite,
                              "validate_index_boundary",
                              d_buffer_common_validate_index(5, 5) == false,
                              "Index 5, count 5 should return false") && result;

    // test 4: index beyond bounds
    result = D_TEST_CHECK_MSG(_suite,
                              "validate_index_beyond",
                              d_buffer_common_validate_index(10, 5) == false,
                              "Index 10, count 5 should return false") && result;

    // test 5: zero count
    result = D_TEST_CHECK_MSG(_suite,
                              "validate_index_zero_count",
                              d_buffer_common_validate_index(0, 0) == false,
                              "Any index with count 0 should return false") && result;

    return result;
}


/*
d_tests_buffer_common_validation_all
  Aggregation function that runs all validation tests.
*/
bool d_tests_buffer_common_validation_all(
    struct dj_test_suite* _suite
)
{
    bool result;

    d_test_suite_block(_suite, "buffer_common_validation_all");

    result = true;

    result = d_tests_buffer_common_validate_params(_suite) && result;
    result = d_tests_buffer_common_validate_capacity(_suite) && result;
    result = d_tests_buffer_common_validate_index(_suite) && result;

    return result;
}
