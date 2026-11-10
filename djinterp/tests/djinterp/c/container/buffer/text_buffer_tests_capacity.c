/*******************************************************************************
* djinterp [container]                              text_buffer_tests_capacity.c
*
*
* path:      /tests/djinterp/c/container/buffer/text_buffer_tests_capacity.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.15
*                                                            revised: 2026.11.09
*******************************************************************************/
// djinterp
#include "./text_buffer_tests.h"
#include "../../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's C face


/*
d_tests_text_buffer_ensure_capacity
  Tests the d_text_buffer_ensure_capacity function.
  Tests the following:
  - NULL buffer returns false
  - requesting less than current capacity succeeds without reallocation
  - requesting more than current capacity grows the buffer
  - data integrity preserved after growth
  - capacity is at least the requested amount
*/
bool d_tests_text_buffer_ensure_capacity(
    struct dj_test_suite* _suite
)
{
    struct dj_text_buffer* buffer;
    size_t                old_cap;
    bool                  result = true;

    // test 1: NULL buffer
    d_test_suite_test(_suite, "text_buffer_ensure_capacity");

    result = D_TEST_CHECK_MSG(_suite,
                              "ensure_cap_null",
                              d_text_buffer_ensure_capacity(NULL, 100) == false,
                              "NULL buffer should return false") && result;

    buffer = d_text_buffer_new_from_string("Hello");

    if (buffer)
    {
        old_cap = d_text_buffer_capacity(buffer);

        // test 2: request less than current
        result = D_TEST_CHECK_MSG(_suite,
                                  "ensure_cap_less",
                                  d_text_buffer_ensure_capacity(buffer, 1) == true,
                                  "Requesting less should succeed") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "ensure_cap_less_unchanged",
                                  d_text_buffer_capacity(buffer) == old_cap,
                                  "Capacity should not change for smaller request") && result;

        // test 3: request much more than current
        result = D_TEST_CHECK_MSG(_suite,
                                  "ensure_cap_grow",
                                  d_text_buffer_ensure_capacity(buffer, 1024) == true,
                                  "Growing capacity should succeed") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "ensure_cap_grow_size",
                                  d_text_buffer_capacity(buffer) >= 1024,
                                  "Capacity should be at least 1024") && result;

        // test 4: data integrity
        result = D_TEST_CHECK_MSG(_suite,
                                  "ensure_cap_data_intact",
                                  d_text_buffer_length(buffer) == 5 && d_strcasecmp(d_text_buffer_get_string(buffer), "Hello") == 0,
                                  "Data should be intact after growth") && result;

        d_text_buffer_free(buffer);
    }

    return result;
}

/*
d_tests_text_buffer_resize_to_fit
  Tests the d_text_buffer_resize_to_fit function.
  Tests the following:
  - NULL buffer returns false
  - buffer with excess capacity is shrunk
  - capacity after shrink is approximately count + 1
  - data integrity preserved after shrink
*/
bool d_tests_text_buffer_resize_to_fit(
    struct dj_test_suite* _suite
)
{
    struct dj_text_buffer* buffer;
    bool                  result = true;

    // test 1: NULL buffer
    d_test_suite_test(_suite, "text_buffer_resize_to_fit");

    result = D_TEST_CHECK_MSG(_suite,
                              "resize_fit_null",
                              d_text_buffer_resize_to_fit(NULL) == false,
                              "NULL buffer should return false") && result;

    // create buffer with large initial capacity
    buffer = d_text_buffer_new(1024);

    if (buffer)
    {
        // add small amount of data
        d_text_buffer_append_string(buffer, "Hi");

        result = D_TEST_CHECK_MSG(_suite,
                                  "resize_fit_precondition",
                                  d_text_buffer_capacity(buffer) >= 1024,
                                  "Capacity should be >= 1024 before shrink") && result;

        // test 2: shrink to fit
        result = D_TEST_CHECK_MSG(_suite,
                                  "resize_fit_success",
                                  d_text_buffer_resize_to_fit(buffer) == true,
                                  "Resize to fit should succeed") && result;

        // test 3: capacity should be reduced
        result = D_TEST_CHECK_MSG(_suite,
                                  "resize_fit_smaller",
                                  d_text_buffer_capacity(buffer) < 1024,
                                  "Capacity should be smaller after shrink") && result;

        // test 4: data intact
        result = D_TEST_CHECK_MSG(_suite,
                                  "resize_fit_data",
                                  d_text_buffer_length(buffer) == 2 && d_strcasecmp(d_text_buffer_get_string(buffer), "Hi") == 0,
                                  "Data should be intact after shrink") && result;

        d_text_buffer_free(buffer);
    }

    return result;
}

/*
d_tests_text_buffer_reserve
  Tests the d_text_buffer_reserve function.
  Tests the following:
  - NULL buffer returns false
  - zero additional capacity succeeds
  - reserving additional space grows capacity
  - data integrity preserved
*/
bool d_tests_text_buffer_reserve(
    struct dj_test_suite* _suite
)
{
    struct dj_text_buffer* buffer;
    size_t                old_cap;
    bool                  result = true;

    // test 1: NULL buffer
    d_test_suite_test(_suite, "text_buffer_reserve");

    result = D_TEST_CHECK_MSG(_suite,
                              "reserve_null",
                              d_text_buffer_reserve(NULL, 100) == false,
                              "NULL buffer should return false") && result;

    buffer = d_text_buffer_new_from_string("Test");

    if (buffer)
    {
        old_cap = d_text_buffer_capacity(buffer);

        // test 2: zero additional
        result = D_TEST_CHECK_MSG(_suite,
                                  "reserve_zero",
                                  d_text_buffer_reserve(buffer, 0) == true,
                                  "Zero additional should succeed") && result;

        // test 3: reserve additional space
        result = D_TEST_CHECK_MSG(_suite,
                                  "reserve_grow",
                                  d_text_buffer_reserve(buffer, 500) == true,
                                  "Reserving additional space should succeed") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "reserve_grow_size",
                                  d_text_buffer_capacity(buffer) >= (old_cap + 500),
                                  "Capacity should grow by at least 500") && result;

        // test 4: data intact
        result = D_TEST_CHECK_MSG(_suite,
                                  "reserve_data_intact",
                                  d_text_buffer_length(buffer) == 4 && d_strcasecmp(d_text_buffer_get_string(buffer), "Test") == 0,
                                  "Data should be intact after reserve") && result;

        d_text_buffer_free(buffer);
    }

    return result;
}

/*
d_tests_text_buffer_capacity_all
  Aggregation function that runs all capacity management tests.
*/
bool d_tests_text_buffer_capacity_all(
    struct dj_test_suite* _suite
)
{
    d_test_suite_block(_suite, "text_buffer_capacity_all");

    return d_tests_text_buffer_ensure_capacity(_suite) &&
           d_tests_text_buffer_resize_to_fit(_suite)   &&
           d_tests_text_buffer_reserve(_suite);
}
