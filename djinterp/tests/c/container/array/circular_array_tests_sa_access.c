#include "./circular_array_tests_sa.h"


/******************************************************************************
 * II. ELEMENT ACCESS FUNCTION TESTS
 *****************************************************************************/

/*
d_tests_sa_circular_array_get
  Tests the d_circular_array_get function.
  Tests the following:
  - get with valid positive index
  - get with negative index
  - get on empty array returns NULL
  - get with out of bounds index returns NULL
  - get works with wraparound
*/
bool
d_tests_sa_circular_array_get
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
        // test 1: get on empty array
        elem = (int*)d_circular_array_get(arr, 0);
        result = d_assert_standalone(
            elem == NULL,
            "get_empty",
            "Get on empty array should return NULL",
            _counter) && result;

        // add elements
        for (int i = 0; i < 3; i++)
        {
            d_circular_array_push(arr, &values[i]);
        }

        // test 2: get with valid positive index
        elem = (int*)d_circular_array_get(arr, 0);
        result = d_assert_standalone(
            elem != NULL && *elem == 10,
            "get_index_0",
            "Get index 0 should return first element",
            _counter) && result;

        elem = (int*)d_circular_array_get(arr, 2);
        result = d_assert_standalone(
            elem != NULL && *elem == 30,
            "get_index_2",
            "Get index 2 should return third element",
            _counter) && result;

        // test 3: get with negative index
        elem = (int*)d_circular_array_get(arr, -1);
        result = d_assert_standalone(
            elem != NULL && *elem == 30,
            "get_negative_1",
            "Get index -1 should return last element",
            _counter) && result;

        elem = (int*)d_circular_array_get(arr, -3);
        result = d_assert_standalone(
            elem != NULL && *elem == 10,
            "get_negative_3",
            "Get index -3 should return first element",
            _counter) && result;

        // test 4: out of bounds returns NULL
        elem = (int*)d_circular_array_get(arr, 5);
        result = d_assert_standalone(
            elem == NULL,
            "get_out_of_bounds",
            "Get out of bounds should return NULL",
            _counter) && result;

        elem = (int*)d_circular_array_get(arr, -10);
        result = d_assert_standalone(
            elem == NULL,
            "get_negative_out_of_bounds",
            "Get negative out of bounds should return NULL",
            _counter) && result;

        d_circular_array_free(arr);
    }

    // test 5: get with wraparound
    arr = d_circular_array_new(3, sizeof(int));

    if (arr)
    {
        // fill, pop one, push one to create wraparound
        for (int i = 0; i < 3; i++)
        {
            d_circular_array_push(arr, &values[i]);
        }
        // Physical: [10, 20, 30], head=0, tail=0, count=3

        d_circular_array_pop(arr);
        // Physical: [10, 20, 30], head=1, tail=0, count=2
        // Logical: 20, 30

        d_circular_array_push(arr, &values[3]);
        // Physical: [40, 20, 30], head=1, tail=1, count=3
        // Logical: 20, 30, 40

        elem = (int*)d_circular_array_get(arr, 0);
        result = d_assert_standalone(
            elem != NULL && *elem == 20,
            "get_wraparound_0",
            "Get with wraparound should return correct element",
            _counter) && result;

        elem = (int*)d_circular_array_get(arr, 2);
        result = d_assert_standalone(
            elem != NULL && *elem == 40,
            "get_wraparound_2",
            "Get with wraparound index 2 correct",
            _counter) && result;

        d_circular_array_free(arr);
    }

    // test 6: NULL array
    elem = (int*)d_circular_array_get(NULL, 0);
    result = d_assert_standalone(
        elem == NULL,
        "get_null_array",
        "Get on NULL array should return NULL",
        _counter) && result;

    return result;
}


/*
d_tests_sa_circular_array_set
  Tests the d_circular_array_set function.
  Tests the following:
  - set with valid index
  - set with negative index
  - set on empty array returns failure
  - set with out of bounds returns failure
*/
bool
d_tests_sa_circular_array_set
(
    struct d_test_counter* _counter
)
{
    bool                      result;
    struct d_circular_array*  arr;
    int                       values[] = {10, 20, 30};
    int                       new_value = 99;
    int*                      elem;

    result = true;

    arr = d_circular_array_new(5, sizeof(int));

    if (arr)
    {
        // test 1: set on empty array
        result = d_assert_standalone(
            d_circular_array_set(arr, 0, &new_value) == false,
            "set_empty",
            "Set on empty array should fail",
            _counter) && result;

        // add elements
        for (int i = 0; i < 3; i++)
        {
            d_circular_array_push(arr, &values[i]);
        }

        // test 2: set with valid index
        result = d_assert_standalone(
            d_circular_array_set(arr, 1, &new_value) == true,
            "set_valid_index",
            "Set with valid index should succeed",
            _counter) && result;

        elem = (int*)d_circular_array_get(arr, 1);
        result = d_assert_standalone(
            elem != NULL && *elem == 99,
            "set_verify",
            "Set value should be retrievable",
            _counter) && result;

        // test 3: set with negative index
        new_value = 77;
        result = d_assert_standalone(
            d_circular_array_set(arr, -1, &new_value) == true,
            "set_negative_index",
            "Set with negative index should succeed",
            _counter) && result;

        elem = (int*)d_circular_array_get(arr, -1);
        result = d_assert_standalone(
            elem != NULL && *elem == 77,
            "set_negative_verify",
            "Set with negative index should work",
            _counter) && result;

        // test 4: set out of bounds
        result = d_assert_standalone(
            d_circular_array_set(arr, 10, &new_value) == false,
            "set_out_of_bounds",
            "Set out of bounds should fail",
            _counter) && result;

        d_circular_array_free(arr);
    }

    // test 5: NULL array
    result = d_assert_standalone(
        d_circular_array_set(NULL, 0, &new_value) == false,
        "set_null_array",
        "Set on NULL array should fail",
        _counter) && result;

    return result;
}


/*
d_tests_sa_circular_array_front
  Tests the d_circular_array_front function.
  Tests the following:
  - front on empty array returns NULL
  - front returns first element
  - front doesn't modify array
*/
bool
d_tests_sa_circular_array_front
(
    struct d_test_counter* _counter
)
{
    bool                      result;
    struct d_circular_array*  arr;
    int                       values[] = {100, 200, 300};
    int*                      elem;

    result = true;

    arr = d_circular_array_new(5, sizeof(int));

    if (arr)
    {
        // test 1: front on empty array
        elem = (int*)d_circular_array_front(arr);
        result = d_assert_standalone(
            elem == NULL,
            "front_empty",
            "Front on empty array should return NULL",
            _counter) && result;

        // add elements
        for (int i = 0; i < 3; i++)
        {
            d_circular_array_push(arr, &values[i]);
        }

        // test 2: front returns first element
        elem = (int*)d_circular_array_front(arr);
        result = d_assert_standalone(
            elem != NULL && *elem == 100,
            "front_value",
            "Front should return first element",
            _counter) && result;

        // test 3: front doesn't modify array
        size_t count_before = arr->count;
        d_circular_array_front(arr);
        result = d_assert_standalone(
            arr->count == count_before,
            "front_no_modify",
            "Front should not modify array",
            _counter) && result;

        d_circular_array_free(arr);
    }

    // test 4: NULL array
    elem = (int*)d_circular_array_front(NULL);
    result = d_assert_standalone(
        elem == NULL,
        "front_null",
        "Front on NULL should return NULL",
        _counter) && result;

    return result;
}


/*
d_tests_sa_circular_array_back
  Tests the d_circular_array_back function.
  Tests the following:
  - back on empty array returns NULL
  - back returns last element
  - back works with wraparound
*/
bool
d_tests_sa_circular_array_back
(
    struct d_test_counter* _counter
)
{
    bool                      result;
    struct d_circular_array*  arr;
    int                       values[] = {100, 200, 300, 400};
    int*                      elem;

    result = true;

    arr = d_circular_array_new(5, sizeof(int));

    if (arr)
    {
        // test 1: back on empty array
        elem = (int*)d_circular_array_back(arr);
        result = d_assert_standalone(
            elem == NULL,
            "back_empty",
            "Back on empty array should return NULL",
            _counter) && result;

        // add elements
        for (int i = 0; i < 3; i++)
        {
            d_circular_array_push(arr, &values[i]);
        }

        // test 2: back returns last element
        elem = (int*)d_circular_array_back(arr);
        result = d_assert_standalone(
            elem != NULL && *elem == 300,
            "back_value",
            "Back should return last element",
            _counter) && result;

        d_circular_array_free(arr);
    }

    // test 3: back with wraparound
    arr = d_circular_array_new(3, sizeof(int));

    if (arr)
    {
        for (int i = 0; i < 3; i++)
        {
            d_circular_array_push(arr, &values[i]);
        }

        d_circular_array_pop(arr);
        d_circular_array_push(arr, &values[3]);

        elem = (int*)d_circular_array_back(arr);
        result = d_assert_standalone(
            elem != NULL && *elem == 400,
            "back_wraparound",
            "Back should work with wraparound",
            _counter) && result;

        d_circular_array_free(arr);
    }

    // test 4: NULL array
    elem = (int*)d_circular_array_back(NULL);
    result = d_assert_standalone(
        elem == NULL,
        "back_null",
        "Back on NULL should return NULL",
        _counter) && result;

    return result;
}


/*
d_tests_sa_circular_array_peek
  Tests the d_circular_array_peek function.
  Tests the following:
  - peek on empty array returns NULL
  - peek returns front element (same as front)
  - peek doesn't modify array
*/
bool
d_tests_sa_circular_array_peek
(
    struct d_test_counter* _counter
)
{
    bool                      result;
    struct d_circular_array*  arr;
    int                       values[] = {100, 200, 300};
    int*                      elem;

    result = true;

    arr = d_circular_array_new(5, sizeof(int));

    if (arr)
    {
        // test 1: peek on empty array
        elem = (int*)d_circular_array_peek(arr);
        result = d_assert_standalone(
            elem == NULL,
            "peek_empty",
            "Peek on empty array should return NULL",
            _counter) && result;

        // add elements
        for (int i = 0; i < 3; i++)
        {
            d_circular_array_push(arr, &values[i]);
        }

        // test 2: peek returns front element
        elem = (int*)d_circular_array_peek(arr);
        result = d_assert_standalone(
            elem != NULL && *elem == 100,
            "peek_value",
            "Peek should return front element",
            _counter) && result;

        // test 3: peek doesn't modify array
        size_t count_before = arr->count;
        size_t head_before = arr->head;
        d_circular_array_peek(arr);
        result = d_assert_standalone(
            arr->count == count_before && arr->head == head_before,
            "peek_no_modify",
            "Peek should not modify array",
            _counter) && result;

        d_circular_array_free(arr);
    }

    // test 4: NULL array
    elem = (int*)d_circular_array_peek(NULL);
    result = d_assert_standalone(
        elem == NULL,
        "peek_null",
        "Peek on NULL should return NULL",
        _counter) && result;

    return result;
}


/*
d_tests_sa_circular_array_peek_back
  Tests the d_circular_array_peek_back function.
  Tests the following:
  - peek_back on empty array returns NULL
  - peek_back returns last element
  - peek_back doesn't modify array
*/
bool
d_tests_sa_circular_array_peek_back
(
    struct d_test_counter* _counter
)
{
    bool                      result;
    struct d_circular_array*  arr;
    int                       values[] = {100, 200, 300};
    int*                      elem;

    result = true;

    arr = d_circular_array_new(5, sizeof(int));

    if (arr)
    {
        // test 1: peek_back on empty array
        elem = (int*)d_circular_array_peek_back(arr);
        result = d_assert_standalone(
            elem == NULL,
            "peek_back_empty",
            "Peek back on empty array should return NULL",
            _counter) && result;

        // add elements
        for (int i = 0; i < 3; i++)
        {
            d_circular_array_push(arr, &values[i]);
        }

        // test 2: peek_back returns last element
        elem = (int*)d_circular_array_peek_back(arr);
        result = d_assert_standalone(
            elem != NULL && *elem == 300,
            "peek_back_value",
            "Peek back should return last element",
            _counter) && result;

        // test 3: peek_back doesn't modify array
        size_t count_before = arr->count;
        d_circular_array_peek_back(arr);
        result = d_assert_standalone(
            arr->count == count_before,
            "peek_back_no_modify",
            "Peek back should not modify array",
            _counter) && result;

        d_circular_array_free(arr);
    }

    // test 4: NULL array
    elem = (int*)d_circular_array_peek_back(NULL);
    result = d_assert_standalone(
        elem == NULL,
        "peek_back_null",
        "Peek back on NULL should return NULL",
        _counter) && result;

    return result;
}


/*
d_tests_sa_circular_array_access_all
  Aggregation function that runs all element access tests.
*/
bool
d_tests_sa_circular_array_access_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Element Access Functions\n");
    printf("  ------------------------------------\n");

    result = d_tests_sa_circular_array_get(_counter) && result;
    result = d_tests_sa_circular_array_set(_counter) && result;
    result = d_tests_sa_circular_array_front(_counter) && result;
    result = d_tests_sa_circular_array_back(_counter) && result;
    result = d_tests_sa_circular_array_peek(_counter) && result;
    result = d_tests_sa_circular_array_peek_back(_counter) && result;

    return result;
}
