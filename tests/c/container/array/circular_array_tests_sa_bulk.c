#include "./circular_array_tests_sa.h"


/******************************************************************************
 * IV. BULK OPERATION FUNCTION TESTS
 *****************************************************************************/

/*
d_tests_sa_circular_array_clear
  Tests the d_circular_array_clear function.
  Tests the following:
  - clear resets count to 0
  - clear resets head and tail
  - clear on empty array
  - clear preserves capacity
*/
bool
d_tests_sa_circular_array_clear
(
    struct d_test_counter* _counter
)
{
    bool                      result;
    struct d_circular_array*  arr;
    int                       values[] = {10, 20, 30};

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
        result = d_assert_standalone(
            d_circular_array_clear(arr) == true,
            "clear_success",
            "Clear should succeed",
            _counter) && result;

        result = d_assert_standalone(
            arr->count == 0,
            "clear_count_zero",
            "Count should be 0 after clear",
            _counter) && result;

        result = d_assert_standalone(
            arr->head == 0 && arr->tail == 0,
            "clear_head_tail_zero",
            "Head and tail should be 0 after clear",
            _counter) && result;

        result = d_assert_standalone(
            arr->capacity == 5,
            "clear_capacity_preserved",
            "Capacity should be preserved",
            _counter) && result;

        // test 2: clear already empty array
        result = d_assert_standalone(
            d_circular_array_clear(arr) == true,
            "clear_empty",
            "Clear on empty should succeed",
            _counter) && result;

        d_circular_array_free(arr);
    }

    // test 3: clear NULL array
    result = d_assert_standalone(
        d_circular_array_clear(NULL) == false,
        "clear_null",
        "Clear on NULL should fail",
        _counter) && result;

    return result;
}


/*
d_tests_sa_circular_array_fill
  Tests the d_circular_array_fill function.
  Tests the following:
  - fill replaces all elements with value
  - fill on empty array
*/
bool
d_tests_sa_circular_array_fill
(
    struct d_test_counter* _counter
)
{
    bool                      result;
    struct d_circular_array*  arr;
    int                       values[] = {10, 20, 30};
    int                       fill_value = 99;
    int*                      elem;

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
        result = d_assert_standalone(
            d_circular_array_fill(arr, &fill_value) == true,
            "fill_success",
            "Fill should succeed",
            _counter) && result;

        elem = (int*)d_circular_array_get(arr, 0);
        result = d_assert_standalone(
            elem != NULL && *elem == 99,
            "fill_first",
            "First element should be 99",
            _counter) && result;

        elem = (int*)d_circular_array_get(arr, 4);
        result = d_assert_standalone(
            elem != NULL && *elem == 99,
            "fill_last",
            "Last element should be 99",
            _counter) && result;

        // fill() fills to capacity, so count becomes capacity
        result = d_assert_standalone(
            arr->count == arr->capacity,
            "fill_count_to_capacity",
            "Fill should set count to capacity",
            _counter) && result;

        d_circular_array_free(arr);
    }

    // test 2: fill empty array
    arr = d_circular_array_new(5, sizeof(int));

    if (arr)
    {
        result = d_assert_standalone(
            d_circular_array_fill(arr, &fill_value) == true,
            "fill_empty",
            "Fill on empty should succeed (no-op)",
            _counter) && result;

        d_circular_array_free(arr);
    }

    // test 3: fill NULL array
    result = d_assert_standalone(
        d_circular_array_fill(NULL, &fill_value) == false,
        "fill_null",
        "Fill on NULL should fail",
        _counter) && result;

    return result;
}


/*
d_tests_sa_circular_array_rotate_left
  Tests the d_circular_array_rotate_left function.
  Tests the following:
  - rotate_left shifts elements
  - rotate_left by 0 is no-op
  - rotate_left on empty array
*/
bool
d_tests_sa_circular_array_rotate_left
(
    struct d_test_counter* _counter
)
{
    bool                      result;
    struct d_circular_array*  arr;
    int                       values[] = {10, 20, 30, 40, 50};
    int*                      elem;

    result = true;

    arr = d_circular_array_new(5, sizeof(int));

    if (arr)
    {
        for (int i = 0; i < 5; i++)
        {
            d_circular_array_push(arr, &values[i]);
        }

        // test 1: rotate left by 1
        result = d_assert_standalone(
            d_circular_array_rotate_left(arr, 1) == true,
            "rotate_left_1",
            "Rotate left by 1 should succeed",
            _counter) && result;

        // after rotate left by 1: 20, 30, 40, 50, 10
        elem = (int*)d_circular_array_get(arr, 0);
        result = d_assert_standalone(
            elem != NULL && *elem == 20,
            "rotate_left_first",
            "First element should be 20",
            _counter) && result;

        elem = (int*)d_circular_array_get(arr, 4);
        result = d_assert_standalone(
            elem != NULL && *elem == 10,
            "rotate_left_last",
            "Last element should be 10",
            _counter) && result;

        // test 2: rotate left by 0
        result = d_assert_standalone(
            d_circular_array_rotate_left(arr, 0) == true,
            "rotate_left_0",
            "Rotate left by 0 should succeed",
            _counter) && result;

        d_circular_array_free(arr);
    }

    // test 3: rotate left on empty array
    arr = d_circular_array_new(5, sizeof(int));

    if (arr)
    {
        result = d_assert_standalone(
            d_circular_array_rotate_left(arr, 1) == true,
            "rotate_left_empty",
            "Rotate left on empty should succeed (no-op)",
            _counter) && result;

        d_circular_array_free(arr);
    }

    // test 4: NULL array
    result = d_assert_standalone(
        d_circular_array_rotate_left(NULL, 1) == false,
        "rotate_left_null",
        "Rotate left on NULL should fail",
        _counter) && result;

    return result;
}


/*
d_tests_sa_circular_array_rotate_right
  Tests the d_circular_array_rotate_right function.
  Tests the following:
  - rotate_right shifts elements
  - rotate_right by 0 is no-op
*/
bool
d_tests_sa_circular_array_rotate_right
(
    struct d_test_counter* _counter
)
{
    bool                      result;
    struct d_circular_array*  arr;
    int                       values[] = {10, 20, 30, 40, 50};
    int*                      elem;

    result = true;

    arr = d_circular_array_new(5, sizeof(int));

    if (arr)
    {
        for (int i = 0; i < 5; i++)
        {
            d_circular_array_push(arr, &values[i]);
        }

        // test 1: rotate right by 1
        result = d_assert_standalone(
            d_circular_array_rotate_right(arr, 1) == true,
            "rotate_right_1",
            "Rotate right by 1 should succeed",
            _counter) && result;

        // after rotate right by 1: 50, 10, 20, 30, 40
        elem = (int*)d_circular_array_get(arr, 0);
        result = d_assert_standalone(
            elem != NULL && *elem == 50,
            "rotate_right_first",
            "First element should be 50",
            _counter) && result;

        elem = (int*)d_circular_array_get(arr, 1);
        result = d_assert_standalone(
            elem != NULL && *elem == 10,
            "rotate_right_second",
            "Second element should be 10",
            _counter) && result;

        d_circular_array_free(arr);
    }

    // test 2: NULL array
    result = d_assert_standalone(
        d_circular_array_rotate_right(NULL, 1) == false,
        "rotate_right_null",
        "Rotate right on NULL should fail",
        _counter) && result;

    return result;
}


/*
d_tests_sa_circular_array_reverse
  Tests the d_circular_array_reverse function.
  Tests the following:
  - reverse order of elements
  - reverse on single element
  - reverse on empty array
*/
bool
d_tests_sa_circular_array_reverse
(
    struct d_test_counter* _counter
)
{
    bool                      result;
    struct d_circular_array*  arr;
    int                       values[] = {10, 20, 30, 40, 50};
    int*                      elem;

    result = true;

    arr = d_circular_array_new(5, sizeof(int));

    if (arr)
    {
        for (int i = 0; i < 5; i++)
        {
            d_circular_array_push(arr, &values[i]);
        }

        // test 1: reverse
        result = d_assert_standalone(
            d_circular_array_reverse(arr) == true,
            "reverse_success",
            "Reverse should succeed",
            _counter) && result;

        // after reverse: 50, 40, 30, 20, 10
        elem = (int*)d_circular_array_get(arr, 0);
        result = d_assert_standalone(
            elem != NULL && *elem == 50,
            "reverse_first",
            "First element should be 50",
            _counter) && result;

        elem = (int*)d_circular_array_get(arr, 4);
        result = d_assert_standalone(
            elem != NULL && *elem == 10,
            "reverse_last",
            "Last element should be 10",
            _counter) && result;

        d_circular_array_free(arr);
    }

    // test 2: reverse single element
    arr = d_circular_array_new(5, sizeof(int));

    if (arr)
    {
        d_circular_array_push(arr, &values[0]);

        result = d_assert_standalone(
            d_circular_array_reverse(arr) == true,
            "reverse_single",
            "Reverse on single element should succeed",
            _counter) && result;

        elem = (int*)d_circular_array_get(arr, 0);
        result = d_assert_standalone(
            elem != NULL && *elem == 10,
            "reverse_single_unchanged",
            "Single element should be unchanged",
            _counter) && result;

        d_circular_array_free(arr);
    }

    // test 3: reverse empty array
    arr = d_circular_array_new(5, sizeof(int));

    if (arr)
    {
        result = d_assert_standalone(
            d_circular_array_reverse(arr) == true,
            "reverse_empty",
            "Reverse on empty should succeed",
            _counter) && result;

        d_circular_array_free(arr);
    }

    // test 4: NULL array
    result = d_assert_standalone(
        d_circular_array_reverse(NULL) == false,
        "reverse_null",
        "Reverse on NULL should fail",
        _counter) && result;

    return result;
}


/*
d_tests_sa_circular_array_swap
  Tests the d_circular_array_swap function.
  Tests the following:
  - swap two elements
  - swap with negative indices
  - swap same index (no-op)
  - swap out of bounds fails
*/
bool
d_tests_sa_circular_array_swap
(
    struct d_test_counter* _counter
)
{
    bool                      result;
    struct d_circular_array*  arr;
    int                       values[] = {10, 20, 30, 40, 50};
    int*                      elem;

    result = true;

    arr = d_circular_array_new(5, sizeof(int));

    if (arr)
    {
        for (int i = 0; i < 5; i++)
        {
            d_circular_array_push(arr, &values[i]);
        }

        // test 1: swap two elements
        result = d_assert_standalone(
            d_circular_array_swap(arr, 0, 4) == true,
            "swap_success",
            "Swap should succeed",
            _counter) && result;

        elem = (int*)d_circular_array_get(arr, 0);
        result = d_assert_standalone(
            elem != NULL && *elem == 50,
            "swap_first",
            "First element should be 50 after swap",
            _counter) && result;

        elem = (int*)d_circular_array_get(arr, 4);
        result = d_assert_standalone(
            elem != NULL && *elem == 10,
            "swap_last",
            "Last element should be 10 after swap",
            _counter) && result;

        // test 2: swap with negative indices
        result = d_assert_standalone(
            d_circular_array_swap(arr, 1, -1) == true,
            "swap_negative",
            "Swap with negative index should succeed",
            _counter) && result;

        // test 3: swap same index
        result = d_assert_standalone(
            d_circular_array_swap(arr, 2, 2) == true,
            "swap_same",
            "Swap same index should succeed",
            _counter) && result;

        // test 4: swap out of bounds
        result = d_assert_standalone(
            d_circular_array_swap(arr, 0, 10) == false,
            "swap_out_of_bounds",
            "Swap out of bounds should fail",
            _counter) && result;

        d_circular_array_free(arr);
    }

    // test 5: NULL array
    result = d_assert_standalone(
        d_circular_array_swap(NULL, 0, 1) == false,
        "swap_null",
        "Swap on NULL should fail",
        _counter) && result;

    return result;
}


/*
d_tests_sa_circular_array_bulk_all
  Aggregation function that runs all bulk operation tests.
*/
bool
d_tests_sa_circular_array_bulk_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Bulk Operation Functions\n");
    printf("  ------------------------------------\n");

    result = d_tests_sa_circular_array_clear(_counter) && result;
    result = d_tests_sa_circular_array_fill(_counter) && result;
    result = d_tests_sa_circular_array_rotate_left(_counter) && result;
    result = d_tests_sa_circular_array_rotate_right(_counter) && result;
    result = d_tests_sa_circular_array_reverse(_counter) && result;
    result = d_tests_sa_circular_array_swap(_counter) && result;

    return result;
}
