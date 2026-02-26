#include "./circular_array_tests_sa.h"


/******************************************************************************
 * III. MODIFICATION FUNCTION TESTS - PUSH/POP OPERATIONS
 *****************************************************************************/

/*
d_tests_sa_circular_array_push
  Tests the d_circular_array_push function.
  Tests the following:
  - push to empty array
  - push multiple elements
  - push to full array fails
  - push updates count and tail
*/
bool
d_tests_sa_circular_array_push
(
    struct d_test_counter* _counter
)
{
    bool                      result;
    struct d_circular_array*  arr;
    int                       values[] = {10, 20, 30, 40, 50};
    int*                      elem;

    result = true;

    arr = d_circular_array_new(3, sizeof(int));

    if (arr)
    {
        // test 1: push to empty array
        result = d_assert_standalone(
            d_circular_array_push(arr, &values[0]) == true,
            "push_empty",
            "Push to empty array should succeed",
            _counter) && result;

        result = d_assert_standalone(
            arr->count == 1,
            "push_empty_count",
            "Count should be 1 after push",
            _counter) && result;

        // test 2: push multiple elements
        d_circular_array_push(arr, &values[1]);
        d_circular_array_push(arr, &values[2]);

        result = d_assert_standalone(
            arr->count == 3,
            "push_multiple_count",
            "Count should be 3 after three pushes",
            _counter) && result;

        elem = (int*)d_circular_array_get(arr, 0);
        result = d_assert_standalone(
            elem != NULL && *elem == 10,
            "push_order_first",
            "First element should be 10",
            _counter) && result;

        elem = (int*)d_circular_array_get(arr, 2);
        result = d_assert_standalone(
            elem != NULL && *elem == 30,
            "push_order_last",
            "Last element should be 30",
            _counter) && result;

        // test 3: push to full array fails
        result = d_assert_standalone(
            d_circular_array_push(arr, &values[3]) == false,
            "push_full",
            "Push to full array should fail",
            _counter) && result;

        result = d_assert_standalone(
            arr->count == 3,
            "push_full_count_unchanged",
            "Count should remain 3",
            _counter) && result;

        d_circular_array_free(arr);
    }

    // test 4: NULL array
    result = d_assert_standalone(
        d_circular_array_push(NULL, &values[0]) == false,
        "push_null_array",
        "Push to NULL array should fail",
        _counter) && result;

    return result;
}


/*
d_tests_sa_circular_array_push_front
  Tests the d_circular_array_push_front function.
  Tests the following:
  - push_front to empty array
  - push_front adds to beginning
  - push_front to full array fails
*/
bool
d_tests_sa_circular_array_push_front
(
    struct d_test_counter* _counter
)
{
    bool                      result;
    struct d_circular_array*  arr;
    int                       values[] = {10, 20, 30};
    int*                      elem;

    result = true;

    arr = d_circular_array_new(3, sizeof(int));

    if (arr)
    {
        // test 1: push_front to empty array
        result = d_assert_standalone(
            d_circular_array_push_front(arr, &values[0]) == true,
            "push_front_empty",
            "Push front to empty should succeed",
            _counter) && result;

        // test 2: push_front adds to beginning
        d_circular_array_push_front(arr, &values[1]);

        elem = (int*)d_circular_array_get(arr, 0);
        result = d_assert_standalone(
            elem != NULL && *elem == 20,
            "push_front_order",
            "Push front should add to beginning",
            _counter) && result;

        elem = (int*)d_circular_array_get(arr, 1);
        result = d_assert_standalone(
            elem != NULL && *elem == 10,
            "push_front_second",
            "Original element should be second",
            _counter) && result;

        // test 3: push_front to full array fails
        d_circular_array_push_front(arr, &values[2]);  // fill it
        result = d_assert_standalone(
            d_circular_array_push_front(arr, &values[0]) == false,
            "push_front_full",
            "Push front to full should fail",
            _counter) && result;

        d_circular_array_free(arr);
    }

    // test 4: NULL array
    result = d_assert_standalone(
        d_circular_array_push_front(NULL, &values[0]) == false,
        "push_front_null",
        "Push front to NULL should fail",
        _counter) && result;

    return result;
}


/*
d_tests_sa_circular_array_push_back
  Tests the d_circular_array_push_back function (alias for push).
  Tests the following:
  - push_back adds to end
  - same behavior as push
*/
bool
d_tests_sa_circular_array_push_back
(
    struct d_test_counter* _counter
)
{
    bool                      result;
    struct d_circular_array*  arr;
    int                       values[] = {10, 20};
    int*                      elem;

    result = true;

    arr = d_circular_array_new(5, sizeof(int));

    if (arr)
    {
        d_circular_array_push_back(arr, &values[0]);
        d_circular_array_push_back(arr, &values[1]);

        elem = (int*)d_circular_array_get(arr, 0);
        result = d_assert_standalone(
            elem != NULL && *elem == 10,
            "push_back_first",
            "First element should be 10",
            _counter) && result;

        elem = (int*)d_circular_array_get(arr, 1);
        result = d_assert_standalone(
            elem != NULL && *elem == 20,
            "push_back_second",
            "Second element should be 20",
            _counter) && result;

        d_circular_array_free(arr);
    }

    return result;
}


/*
d_tests_sa_circular_array_push_all
  Tests the d_circular_array_push_all function.
  Tests the following:
  - push_all multiple elements
  - push_all partial when not enough space
  - push_all to full array fails
*/
bool
d_tests_sa_circular_array_push_all
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
        // test 1: push_all multiple elements
        result = d_assert_standalone(
            d_circular_array_push_all(arr, values, 3) == true,
            "push_all_valid",
            "Push all should succeed",
            _counter) && result;

        result = d_assert_standalone(
            arr->count == 3,
            "push_all_count",
            "Count should be 3",
            _counter) && result;

        elem = (int*)d_circular_array_get(arr, 0);
        result = d_assert_standalone(
            elem != NULL && *elem == 10,
            "push_all_first",
            "First element should be 10",
            _counter) && result;

        elem = (int*)d_circular_array_get(arr, 2);
        result = d_assert_standalone(
            elem != NULL && *elem == 30,
            "push_all_last",
            "Last element should be 30",
            _counter) && result;

        // test 2: push_all when not enough space
        result = d_assert_standalone(
            d_circular_array_push_all(arr, &values[3], 3) == false,
            "push_all_overflow",
            "Push all exceeding capacity should fail",
            _counter) && result;

        d_circular_array_free(arr);
    }

    // test 3: NULL array
    result = d_assert_standalone(
        d_circular_array_push_all(NULL, values, 3) == false,
        "push_all_null",
        "Push all to NULL should fail",
        _counter) && result;

    // test 4: NULL elements
    arr = d_circular_array_new(5, sizeof(int));

    if (arr)
    {
        result = d_assert_standalone(
            d_circular_array_push_all(arr, NULL, 3) == false,
            "push_all_null_elements",
            "Push all with NULL elements should fail",
            _counter) && result;

        d_circular_array_free(arr);
    }

    return result;
}


/*
d_tests_sa_circular_array_push_all_front
  Tests the d_circular_array_push_all_front function.
  Tests the following:
  - push_all_front adds elements to beginning
  - order preserved
*/
bool
d_tests_sa_circular_array_push_all_front
(
    struct d_test_counter* _counter
)
{
    bool                      result;
    struct d_circular_array*  arr;
    int                       initial = 100;
    int                       values[] = {10, 20, 30};
    int*                      elem;

    result = true;

    arr = d_circular_array_new(5, sizeof(int));

    if (arr)
    {
        d_circular_array_push(arr, &initial);

        result = d_assert_standalone(
            d_circular_array_push_all_front(arr, values, 3) == true,
            "push_all_front_valid",
            "Push all front should succeed",
            _counter) && result;

        result = d_assert_standalone(
            arr->count == 4,
            "push_all_front_count",
            "Count should be 4",
            _counter) && result;

        // order should be: 10, 20, 30, 100
        elem = (int*)d_circular_array_get(arr, 0);
        result = d_assert_standalone(
            elem != NULL && *elem == 10,
            "push_all_front_first",
            "First element should be 10",
            _counter) && result;

        elem = (int*)d_circular_array_get(arr, 3);
        result = d_assert_standalone(
            elem != NULL && *elem == 100,
            "push_all_front_last",
            "Last element should be original 100",
            _counter) && result;

        d_circular_array_free(arr);
    }

    return result;
}


/*
d_tests_sa_circular_array_pop
  Tests the d_circular_array_pop function.
  Tests the following:
  - pop from non-empty array
  - pop returns elements in FIFO order
  - pop from empty array returns NULL
  - pop updates count and head
*/
bool
d_tests_sa_circular_array_pop
(
    struct d_test_counter* _counter
)
{
    bool                      result;
    struct d_circular_array*  arr;
    int                       values[] = {10, 20, 30};
    int*                      elem;

    result = true;

    arr = d_circular_array_new(5, sizeof(int));

    if (arr)
    {
        // test 1: pop from empty array
        elem = (int*)d_circular_array_pop(arr);
        result = d_assert_standalone(
            elem == NULL,
            "pop_empty",
            "Pop from empty should return NULL",
            _counter) && result;

        // add elements
        for (int i = 0; i < 3; i++)
        {
            d_circular_array_push(arr, &values[i]);
        }

        // test 2: pop returns first element (FIFO)
        elem = (int*)d_circular_array_pop(arr);
        result = d_assert_standalone(
            elem != NULL && *elem == 10,
            "pop_first",
            "Pop should return first element",
            _counter) && result;

        result = d_assert_standalone(
            arr->count == 2,
            "pop_count",
            "Count should decrease after pop",
            _counter) && result;

        // test 3: pop sequence maintains FIFO
        elem = (int*)d_circular_array_pop(arr);
        result = d_assert_standalone(
            elem != NULL && *elem == 20,
            "pop_second",
            "Second pop should return 20",
            _counter) && result;

        elem = (int*)d_circular_array_pop(arr);
        result = d_assert_standalone(
            elem != NULL && *elem == 30,
            "pop_third",
            "Third pop should return 30",
            _counter) && result;

        // test 4: array now empty
        result = d_assert_standalone(
            d_circular_array_is_empty(arr) == true,
            "pop_now_empty",
            "Array should be empty after all pops",
            _counter) && result;

        d_circular_array_free(arr);
    }

    // test 5: NULL array
    elem = (int*)d_circular_array_pop(NULL);
    result = d_assert_standalone(
        elem == NULL,
        "pop_null",
        "Pop from NULL should return NULL",
        _counter) && result;

    return result;
}


/*
d_tests_sa_circular_array_pop_front
  Tests the d_circular_array_pop_front function (alias for pop).
*/
bool
d_tests_sa_circular_array_pop_front
(
    struct d_test_counter* _counter
)
{
    bool                      result;
    struct d_circular_array*  arr;
    int                       values[] = {10, 20, 30};
    int*                      elem;

    result = true;

    arr = d_circular_array_new(5, sizeof(int));

    if (arr)
    {
        for (int i = 0; i < 3; i++)
        {
            d_circular_array_push(arr, &values[i]);
        }

        elem = (int*)d_circular_array_pop_front(arr);
        result = d_assert_standalone(
            elem != NULL && *elem == 10,
            "pop_front_value",
            "Pop front should return first element",
            _counter) && result;

        result = d_assert_standalone(
            arr->count == 2,
            "pop_front_count",
            "Count should decrease",
            _counter) && result;

        d_circular_array_free(arr);
    }

    return result;
}


/*
d_tests_sa_circular_array_pop_back
  Tests the d_circular_array_pop_back function.
  Tests the following:
  - pop_back removes last element
  - pop_back from empty returns NULL
*/
bool
d_tests_sa_circular_array_pop_back
(
    struct d_test_counter* _counter
)
{
    bool                      result;
    struct d_circular_array*  arr;
    int                       values[] = {10, 20, 30};
    int*                      elem;

    result = true;

    arr = d_circular_array_new(5, sizeof(int));

    if (arr)
    {
        // test 1: pop_back from empty
        elem = (int*)d_circular_array_pop_back(arr);
        result = d_assert_standalone(
            elem == NULL,
            "pop_back_empty",
            "Pop back from empty should return NULL",
            _counter) && result;

        // add elements
        for (int i = 0; i < 3; i++)
        {
            d_circular_array_push(arr, &values[i]);
        }

        // test 2: pop_back returns last element
        elem = (int*)d_circular_array_pop_back(arr);
        result = d_assert_standalone(
            elem != NULL && *elem == 30,
            "pop_back_last",
            "Pop back should return last element",
            _counter) && result;

        result = d_assert_standalone(
            arr->count == 2,
            "pop_back_count",
            "Count should decrease",
            _counter) && result;

        // test 3: verify remaining elements
        elem = (int*)d_circular_array_back(arr);
        result = d_assert_standalone(
            elem != NULL && *elem == 20,
            "pop_back_new_last",
            "New last element should be 20",
            _counter) && result;

        d_circular_array_free(arr);
    }

    // test 4: NULL array
    elem = (int*)d_circular_array_pop_back(NULL);
    result = d_assert_standalone(
        elem == NULL,
        "pop_back_null",
        "Pop back from NULL should return NULL",
        _counter) && result;

    return result;
}


/*
d_tests_sa_circular_array_pop_to
  Tests the d_circular_array_pop_to function.
  Tests the following:
  - pop_to copies value to output
  - pop_to on empty returns false
*/
bool
d_tests_sa_circular_array_pop_to
(
    struct d_test_counter* _counter
)
{
    bool                      result;
    struct d_circular_array*  arr;
    int                       values[] = {10, 20, 30};
    int                       output;

    result = true;

    arr = d_circular_array_new(5, sizeof(int));

    if (arr)
    {
        // test 1: pop_to on empty
        result = d_assert_standalone(
            d_circular_array_pop_to(arr, &output) == false,
            "pop_to_empty",
            "Pop to on empty should fail",
            _counter) && result;

        // add elements
        for (int i = 0; i < 3; i++)
        {
            d_circular_array_push(arr, &values[i]);
        }

        // test 2: pop_to copies value
        result = d_assert_standalone(
            d_circular_array_pop_to(arr, &output) == true,
            "pop_to_success",
            "Pop to should succeed",
            _counter) && result;

        result = d_assert_standalone(
            output == 10,
            "pop_to_value",
            "Output should be 10",
            _counter) && result;

        result = d_assert_standalone(
            arr->count == 2,
            "pop_to_count",
            "Count should decrease",
            _counter) && result;

        d_circular_array_free(arr);
    }

    // test 3: NULL array
    result = d_assert_standalone(
        d_circular_array_pop_to(NULL, &output) == false,
        "pop_to_null_array",
        "Pop to with NULL array should fail",
        _counter) && result;

    return result;
}


/*
d_tests_sa_circular_array_push_overwrite
  Tests the d_circular_array_push_overwrite function.
  Tests the following:
  - push_overwrite to non-full array
  - push_overwrite to full array overwrites oldest
*/
bool
d_tests_sa_circular_array_push_overwrite
(
    struct d_test_counter* _counter
)
{
    bool                      result;
    struct d_circular_array*  arr;
    int                       values[] = {10, 20, 30, 40};
    int*                      elem;

    result = true;

    arr = d_circular_array_new(3, sizeof(int));

    if (arr)
    {
        // test 1: push_overwrite to non-full array
        result = d_assert_standalone(
            d_circular_array_push_overwrite(arr, &values[0]) == true,
            "push_overwrite_empty",
            "Push overwrite to empty should succeed",
            _counter) && result;

        d_circular_array_push_overwrite(arr, &values[1]);
        d_circular_array_push_overwrite(arr, &values[2]);

        result = d_assert_standalone(
            arr->count == 3,
            "push_overwrite_full",
            "Array should be full",
            _counter) && result;

        // test 2: push_overwrite to full array overwrites oldest
        result = d_assert_standalone(
            d_circular_array_push_overwrite(arr, &values[3]) == true,
            "push_overwrite_overflow",
            "Push overwrite to full should succeed",
            _counter) && result;

        result = d_assert_standalone(
            arr->count == 3,
            "push_overwrite_count_same",
            "Count should still be 3",
            _counter) && result;

        // oldest (10) should be overwritten, now 20, 30, 40
        elem = (int*)d_circular_array_get(arr, 0);
        result = d_assert_standalone(
            elem != NULL && *elem == 20,
            "push_overwrite_new_first",
            "First element should now be 20",
            _counter) && result;

        elem = (int*)d_circular_array_get(arr, 2);
        result = d_assert_standalone(
            elem != NULL && *elem == 40,
            "push_overwrite_new_last",
            "Last element should be 40",
            _counter) && result;

        d_circular_array_free(arr);
    }

    // test 3: NULL array
    result = d_assert_standalone(
        d_circular_array_push_overwrite(NULL, &values[0]) == false,
        "push_overwrite_null",
        "Push overwrite to NULL should fail",
        _counter) && result;

    return result;
}


/*
d_tests_sa_circular_array_modification_all
  Aggregation function that runs all modification tests.
*/
bool
d_tests_sa_circular_array_modification_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Modification Functions (Push/Pop)\n");
    printf("  ---------------------------------------------\n");

    result = d_tests_sa_circular_array_push(_counter) && result;
    result = d_tests_sa_circular_array_push_front(_counter) && result;
    result = d_tests_sa_circular_array_push_back(_counter) && result;
    result = d_tests_sa_circular_array_push_all(_counter) && result;
    result = d_tests_sa_circular_array_push_all_front(_counter) && result;
    result = d_tests_sa_circular_array_pop(_counter) && result;
    result = d_tests_sa_circular_array_pop_front(_counter) && result;
    result = d_tests_sa_circular_array_pop_back(_counter) && result;
    result = d_tests_sa_circular_array_pop_to(_counter) && result;
    result = d_tests_sa_circular_array_push_overwrite(_counter) && result;

    return result;
}
