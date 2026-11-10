/*******************************************************************************
* djinterp [container]                               circular_array_tests_bulk.c
*
*
* path:      /tests/djinterp/c/container/array/circular_array_tests_bulk.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.02.25
*                                                            revised: 2026.11.09
*******************************************************************************/
// djinterp
#include "./circular_array_tests.h"
#include "../../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's C face


/******************************************************************************
 * IV. BULK OPERATION FUNCTION TESTS
 *****************************************************************************/

/*
d_tests_circular_array_clear
  Tests the d_circular_array_clear function.
  Tests the following:
  - clear resets count to 0
  - clear resets head and tail
  - clear on empty array
  - clear preserves capacity
*/
bool d_tests_circular_array_clear(
    struct dj_test_suite* _suite
)
{
    bool                      result;
    struct dj_circular_array*  arr;
    int                       values[] = {10, 20, 30};

    d_test_suite_test(_suite, "circular_array_clear");

    result = true;

    arr = d_circular_array_new(5, sizeof(int));

    if (arr)
    {
        // add elements
        for (int i = 0; i < 3; i++)
        {
            d_circular_array_push(arr, &values[i]);
        }

        // test 1: clear populated array
        result = D_TEST_CHECK_MSG(_suite,
                                  "clear_success",
                                  d_circular_array_clear(arr) == true,
                                  "Clear should succeed") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "clear_count_zero",
                                  arr->count == 0,
                                  "Count should be 0 after clear") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "clear_head_tail_zero",
                                  arr->head == 0 && arr->tail == 0,
                                  "Head and tail should be 0 after clear") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "clear_capacity_preserved",
                                  arr->capacity == 5,
                                  "Capacity should be preserved") && result;

        // test 2: clear already empty array
        result = D_TEST_CHECK_MSG(_suite,
                                  "clear_empty",
                                  d_circular_array_clear(arr) == true,
                                  "Clear on empty should succeed") && result;

        d_circular_array_free(arr);
    }

    // test 3: clear NULL array
    result = D_TEST_CHECK_MSG(_suite,
                              "clear_null",
                              d_circular_array_clear(NULL) == false,
                              "Clear on NULL should fail") && result;

    return result;
}


/*
d_tests_circular_array_fill
  Tests the d_circular_array_fill function.
  Tests the following:
  - fill replaces all elements with value
  - fill on empty array
*/
bool d_tests_circular_array_fill(
    struct dj_test_suite* _suite
)
{
    bool                      result;
    struct dj_circular_array*  arr;
    int                       values[] = {10, 20, 30};
    int                       fill_value = 99;
    int*                      elem;

    d_test_suite_test(_suite, "circular_array_fill");

    result = true;

    arr = d_circular_array_new(5, sizeof(int));

    if (arr)
    {
        // add elements
        for (int i = 0; i < 3; i++)
        {
            d_circular_array_push(arr, &values[i]);
        }

        // test 1: fill replaces all elements and fills to capacity
        result = D_TEST_CHECK_MSG(_suite,
                                  "fill_success",
                                  d_circular_array_fill(arr, &fill_value) == true,
                                  "Fill should succeed") && result;

        elem = (int*)d_circular_array_get(arr, 0);
        result = D_TEST_CHECK_MSG(_suite,
                                  "fill_first",
                                  elem != NULL && *elem == 99,
                                  "First element should be 99") && result;

        elem = (int*)d_circular_array_get(arr, 4);
        result = D_TEST_CHECK_MSG(_suite,
                                  "fill_last",
                                  elem != NULL && *elem == 99,
                                  "Last element should be 99") && result;

        // fill() fills to capacity, so count becomes capacity
        result = D_TEST_CHECK_MSG(_suite,
                                  "fill_count_to_capacity",
                                  arr->count == arr->capacity,
                                  "Fill should set count to capacity") && result;

        d_circular_array_free(arr);
    }

    // test 2: fill empty array
    arr = d_circular_array_new(5, sizeof(int));

    if (arr)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "fill_empty",
                                  d_circular_array_fill(arr, &fill_value) == true,
                                  "Fill on empty should succeed (no-op)") && result;

        d_circular_array_free(arr);
    }

    // test 3: fill NULL array
    result = D_TEST_CHECK_MSG(_suite,
                              "fill_null",
                              d_circular_array_fill(NULL, &fill_value) == false,
                              "Fill on NULL should fail") && result;

    return result;
}


/*
d_tests_circular_array_rotate_left
  Tests the d_circular_array_rotate_left function.
  Tests the following:
  - rotate_left shifts elements
  - rotate_left by 0 is no-op
  - rotate_left on empty array
*/
bool d_tests_circular_array_rotate_left(
    struct dj_test_suite* _suite
)
{
    bool                      result;
    struct dj_circular_array*  arr;
    int                       values[] = {10, 20, 30, 40, 50};
    int*                      elem;

    d_test_suite_test(_suite, "circular_array_rotate_left");

    result = true;

    arr = d_circular_array_new(5, sizeof(int));

    if (arr)
    {
        for (int i = 0; i < 5; i++)
        {
            d_circular_array_push(arr, &values[i]);
        }

        // test 1: rotate left by 1
        result = D_TEST_CHECK_MSG(_suite,
                                  "rotate_left_1",
                                  d_circular_array_rotate_left(arr, 1) == true,
                                  "Rotate left by 1 should succeed") && result;

        // after rotate left by 1: 20, 30, 40, 50, 10
        elem = (int*)d_circular_array_get(arr, 0);
        result = D_TEST_CHECK_MSG(_suite,
                                  "rotate_left_first",
                                  elem != NULL && *elem == 20,
                                  "First element should be 20") && result;

        elem = (int*)d_circular_array_get(arr, 4);
        result = D_TEST_CHECK_MSG(_suite,
                                  "rotate_left_last",
                                  elem != NULL && *elem == 10,
                                  "Last element should be 10") && result;

        // test 2: rotate left by 0
        result = D_TEST_CHECK_MSG(_suite,
                                  "rotate_left_0",
                                  d_circular_array_rotate_left(arr, 0) == true,
                                  "Rotate left by 0 should succeed") && result;

        d_circular_array_free(arr);
    }

    // test 3: rotate left on empty array
    arr = d_circular_array_new(5, sizeof(int));

    if (arr)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "rotate_left_empty",
                                  d_circular_array_rotate_left(arr, 1) == true,
                                  "Rotate left on empty should succeed (no-op)") && result;

        d_circular_array_free(arr);
    }

    // test 4: NULL array
    result = D_TEST_CHECK_MSG(_suite,
                              "rotate_left_null",
                              d_circular_array_rotate_left(NULL, 1) == false,
                              "Rotate left on NULL should fail") && result;

    return result;
}


/*
d_tests_circular_array_rotate_right
  Tests the d_circular_array_rotate_right function.
  Tests the following:
  - rotate_right shifts elements
  - rotate_right by 0 is no-op
*/
bool d_tests_circular_array_rotate_right(
    struct dj_test_suite* _suite
)
{
    bool                      result;
    struct dj_circular_array*  arr;
    int                       values[] = {10, 20, 30, 40, 50};
    int*                      elem;

    d_test_suite_test(_suite, "circular_array_rotate_right");

    result = true;

    arr = d_circular_array_new(5, sizeof(int));

    if (arr)
    {
        for (int i = 0; i < 5; i++)
        {
            d_circular_array_push(arr, &values[i]);
        }

        // test 1: rotate right by 1
        result = D_TEST_CHECK_MSG(_suite,
                                  "rotate_right_1",
                                  d_circular_array_rotate_right(arr, 1) == true,
                                  "Rotate right by 1 should succeed") && result;

        // after rotate right by 1: 50, 10, 20, 30, 40
        elem = (int*)d_circular_array_get(arr, 0);
        result = D_TEST_CHECK_MSG(_suite,
                                  "rotate_right_first",
                                  elem != NULL && *elem == 50,
                                  "First element should be 50") && result;

        elem = (int*)d_circular_array_get(arr, 1);
        result = D_TEST_CHECK_MSG(_suite,
                                  "rotate_right_second",
                                  elem != NULL && *elem == 10,
                                  "Second element should be 10") && result;

        d_circular_array_free(arr);
    }

    // test 2: NULL array
    result = D_TEST_CHECK_MSG(_suite,
                              "rotate_right_null",
                              d_circular_array_rotate_right(NULL, 1) == false,
                              "Rotate right on NULL should fail") && result;

    return result;
}


/*
d_tests_circular_array_reverse
  Tests the d_circular_array_reverse function.
  Tests the following:
  - reverse order of elements
  - reverse on single element
  - reverse on empty array
*/
bool d_tests_circular_array_reverse(
    struct dj_test_suite* _suite
)
{
    bool                      result;
    struct dj_circular_array*  arr;
    int                       values[] = {10, 20, 30, 40, 50};
    int*                      elem;

    d_test_suite_test(_suite, "circular_array_reverse");

    result = true;

    arr = d_circular_array_new(5, sizeof(int));

    if (arr)
    {
        for (int i = 0; i < 5; i++)
        {
            d_circular_array_push(arr, &values[i]);
        }

        // test 1: reverse
        result = D_TEST_CHECK_MSG(_suite,
                                  "reverse_success",
                                  d_circular_array_reverse(arr) == true,
                                  "Reverse should succeed") && result;

        // after reverse: 50, 40, 30, 20, 10
        elem = (int*)d_circular_array_get(arr, 0);
        result = D_TEST_CHECK_MSG(_suite,
                                  "reverse_first",
                                  elem != NULL && *elem == 50,
                                  "First element should be 50") && result;

        elem = (int*)d_circular_array_get(arr, 4);
        result = D_TEST_CHECK_MSG(_suite,
                                  "reverse_last",
                                  elem != NULL && *elem == 10,
                                  "Last element should be 10") && result;

        d_circular_array_free(arr);
    }

    // test 2: reverse single element
    arr = d_circular_array_new(5, sizeof(int));

    if (arr)
    {
        d_circular_array_push(arr, &values[0]);

        result = D_TEST_CHECK_MSG(_suite,
                                  "reverse_single",
                                  d_circular_array_reverse(arr) == true,
                                  "Reverse on single element should succeed") && result;

        elem = (int*)d_circular_array_get(arr, 0);
        result = D_TEST_CHECK_MSG(_suite,
                                  "reverse_single_unchanged",
                                  elem != NULL && *elem == 10,
                                  "Single element should be unchanged") && result;

        d_circular_array_free(arr);
    }

    // test 3: reverse empty array
    arr = d_circular_array_new(5, sizeof(int));

    if (arr)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "reverse_empty",
                                  d_circular_array_reverse(arr) == true,
                                  "Reverse on empty should succeed") && result;

        d_circular_array_free(arr);
    }

    // test 4: NULL array
    result = D_TEST_CHECK_MSG(_suite,
                              "reverse_null",
                              d_circular_array_reverse(NULL) == false,
                              "Reverse on NULL should fail") && result;

    return result;
}


/*
d_tests_circular_array_swap
  Tests the d_circular_array_swap function.
  Tests the following:
  - swap two elements
  - swap with negative indices
  - swap same index (no-op)
  - swap out of bounds fails
*/
bool d_tests_circular_array_swap(
    struct dj_test_suite* _suite
)
{
    bool                      result;
    struct dj_circular_array*  arr;
    int                       values[] = {10, 20, 30, 40, 50};
    int*                      elem;

    d_test_suite_test(_suite, "circular_array_swap");

    result = true;

    arr = d_circular_array_new(5, sizeof(int));

    if (arr)
    {
        for (int i = 0; i < 5; i++)
        {
            d_circular_array_push(arr, &values[i]);
        }

        // test 1: swap two elements
        result = D_TEST_CHECK_MSG(_suite,
                                  "swap_success",
                                  d_circular_array_swap(arr, 0, 4) == true,
                                  "Swap should succeed") && result;

        elem = (int*)d_circular_array_get(arr, 0);
        result = D_TEST_CHECK_MSG(_suite,
                                  "swap_first",
                                  elem != NULL && *elem == 50,
                                  "First element should be 50 after swap") && result;

        elem = (int*)d_circular_array_get(arr, 4);
        result = D_TEST_CHECK_MSG(_suite,
                                  "swap_last",
                                  elem != NULL && *elem == 10,
                                  "Last element should be 10 after swap") && result;

        // test 2: swap with negative indices
        result = D_TEST_CHECK_MSG(_suite,
                                  "swap_negative",
                                  d_circular_array_swap(arr, 1, -1) == true,
                                  "Swap with negative index should succeed") && result;

        // test 3: swap same index
        result = D_TEST_CHECK_MSG(_suite,
                                  "swap_same",
                                  d_circular_array_swap(arr, 2, 2) == true,
                                  "Swap same index should succeed") && result;

        // test 4: swap out of bounds
        result = D_TEST_CHECK_MSG(_suite,
                                  "swap_out_of_bounds",
                                  d_circular_array_swap(arr, 0, 10) == false,
                                  "Swap out of bounds should fail") && result;

        d_circular_array_free(arr);
    }

    // test 5: NULL array
    result = D_TEST_CHECK_MSG(_suite,
                              "swap_null",
                              d_circular_array_swap(NULL, 0, 1) == false,
                              "Swap on NULL should fail") && result;

    return result;
}


/*
d_tests_circular_array_bulk_all
  Aggregation function that runs all bulk operation tests.
*/
bool d_tests_circular_array_bulk_all(
    struct dj_test_suite* _suite
)
{
    bool result;

    d_test_suite_block(_suite, "circular_array_bulk_all");

    result = true;

    result = d_tests_circular_array_clear(_suite) && result;
    result = d_tests_circular_array_fill(_suite) && result;
    result = d_tests_circular_array_rotate_left(_suite) && result;
    result = d_tests_circular_array_rotate_right(_suite) && result;
    result = d_tests_circular_array_reverse(_suite) && result;
    result = d_tests_circular_array_swap(_suite) && result;

    return result;
}
