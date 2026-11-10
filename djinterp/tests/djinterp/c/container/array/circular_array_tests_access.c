/*******************************************************************************
* djinterp [container]                             circular_array_tests_access.c
*
*
* path:      /tests/djinterp/c/container/array/circular_array_tests_access.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.02.25
*                                                            revised: 2026.11.09
*******************************************************************************/
// djinterp
#include "./circular_array_tests.h"
#include "../../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's C face


/******************************************************************************
 * II. ELEMENT ACCESS FUNCTION TESTS
 *****************************************************************************/

/*
d_tests_circular_array_get
  Tests the d_circular_array_get function.
  Tests the following:
  - get with valid positive index
  - get with negative index
  - get on empty array returns NULL
  - get with out of bounds index returns NULL
  - get works with wraparound
*/
bool d_tests_circular_array_get(
    struct dj_test_suite* _suite
)
{
    bool                      result;
    struct dj_circular_array*  arr;
    int                       values[] = {10, 20, 30, 40, 50};
    int*                      elem;

    d_test_suite_test(_suite, "circular_array_get");

    result = true;

    arr = d_circular_array_new(5, sizeof(int));

    if (arr)
    {
        // test 1: get on empty array
        elem = (int*)d_circular_array_get(arr, 0);
        result = D_TEST_CHECK_MSG(_suite,
                                  "get_empty",
                                  elem == NULL,
                                  "Get on empty array should return NULL") && result;

        // add elements
        for (int i = 0; i < 3; i++)
        {
            d_circular_array_push(arr, &values[i]);
        }

        // test 2: get with valid positive index
        elem = (int*)d_circular_array_get(arr, 0);
        result = D_TEST_CHECK_MSG(_suite,
                                  "get_index_0",
                                  elem != NULL && *elem == 10,
                                  "Get index 0 should return first element") && result;

        elem = (int*)d_circular_array_get(arr, 2);
        result = D_TEST_CHECK_MSG(_suite,
                                  "get_index_2",
                                  elem != NULL && *elem == 30,
                                  "Get index 2 should return third element") && result;

        // test 3: get with negative index
        elem = (int*)d_circular_array_get(arr, -1);
        result = D_TEST_CHECK_MSG(_suite,
                                  "get_negative_1",
                                  elem != NULL && *elem == 30,
                                  "Get index -1 should return last element") && result;

        elem = (int*)d_circular_array_get(arr, -3);
        result = D_TEST_CHECK_MSG(_suite,
                                  "get_negative_3",
                                  elem != NULL && *elem == 10,
                                  "Get index -3 should return first element") && result;

        // test 4: out of bounds returns NULL
        elem = (int*)d_circular_array_get(arr, 5);
        result = D_TEST_CHECK_MSG(_suite,
                                  "get_out_of_bounds",
                                  elem == NULL,
                                  "Get out of bounds should return NULL") && result;

        elem = (int*)d_circular_array_get(arr, -10);
        result = D_TEST_CHECK_MSG(_suite,
                                  "get_negative_out_of_bounds",
                                  elem == NULL,
                                  "Get negative out of bounds should return NULL") && result;

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
        result = D_TEST_CHECK_MSG(_suite,
                                  "get_wraparound_0",
                                  elem != NULL && *elem == 20,
                                  "Get with wraparound should return correct element") && result;

        elem = (int*)d_circular_array_get(arr, 2);
        result = D_TEST_CHECK_MSG(_suite,
                                  "get_wraparound_2",
                                  elem != NULL && *elem == 40,
                                  "Get with wraparound index 2 correct") && result;

        d_circular_array_free(arr);
    }

    // test 6: NULL array
    elem = (int*)d_circular_array_get(NULL, 0);
    result = D_TEST_CHECK_MSG(_suite,
                              "get_null_array",
                              elem == NULL,
                              "Get on NULL array should return NULL") && result;

    return result;
}


/*
d_tests_circular_array_set
  Tests the d_circular_array_set function.
  Tests the following:
  - set with valid index
  - set with negative index
  - set on empty array returns failure
  - set with out of bounds returns failure
*/
bool d_tests_circular_array_set(
    struct dj_test_suite* _suite
)
{
    bool                      result;
    struct dj_circular_array*  arr;
    int                       values[] = {10, 20, 30};
    int                       new_value = 99;
    int*                      elem;

    d_test_suite_test(_suite, "circular_array_set");

    result = true;

    arr = d_circular_array_new(5, sizeof(int));

    if (arr)
    {
        // test 1: set on empty array
        result = D_TEST_CHECK_MSG(_suite,
                                  "set_empty",
                                  d_circular_array_set(arr, 0, &new_value) == false,
                                  "Set on empty array should fail") && result;

        // add elements
        for (int i = 0; i < 3; i++)
        {
            d_circular_array_push(arr, &values[i]);
        }

        // test 2: set with valid index
        result = D_TEST_CHECK_MSG(_suite,
                                  "set_valid_index",
                                  d_circular_array_set(arr, 1, &new_value) == true,
                                  "Set with valid index should succeed") && result;

        elem = (int*)d_circular_array_get(arr, 1);
        result = D_TEST_CHECK_MSG(_suite,
                                  "set_verify",
                                  elem != NULL && *elem == 99,
                                  "Set value should be retrievable") && result;

        // test 3: set with negative index
        new_value = 77;
        result = D_TEST_CHECK_MSG(_suite,
                                  "set_negative_index",
                                  d_circular_array_set(arr, -1, &new_value) == true,
                                  "Set with negative index should succeed") && result;

        elem = (int*)d_circular_array_get(arr, -1);
        result = D_TEST_CHECK_MSG(_suite,
                                  "set_negative_verify",
                                  elem != NULL && *elem == 77,
                                  "Set with negative index should work") && result;

        // test 4: set out of bounds
        result = D_TEST_CHECK_MSG(_suite,
                                  "set_out_of_bounds",
                                  d_circular_array_set(arr, 10, &new_value) == false,
                                  "Set out of bounds should fail") && result;

        d_circular_array_free(arr);
    }

    // test 5: NULL array
    result = D_TEST_CHECK_MSG(_suite,
                              "set_null_array",
                              d_circular_array_set(NULL, 0, &new_value) == false,
                              "Set on NULL array should fail") && result;

    return result;
}


/*
d_tests_circular_array_front
  Tests the d_circular_array_front function.
  Tests the following:
  - front on empty array returns NULL
  - front returns first element
  - front doesn't modify array
*/
bool d_tests_circular_array_front(
    struct dj_test_suite* _suite
)
{
    bool                      result;
    struct dj_circular_array*  arr;
    int                       values[] = {100, 200, 300};
    int*                      elem;

    d_test_suite_test(_suite, "circular_array_front");

    result = true;

    arr = d_circular_array_new(5, sizeof(int));

    if (arr)
    {
        // test 1: front on empty array
        elem = (int*)d_circular_array_front(arr);
        result = D_TEST_CHECK_MSG(_suite,
                                  "front_empty",
                                  elem == NULL,
                                  "Front on empty array should return NULL") && result;

        // add elements
        for (int i = 0; i < 3; i++)
        {
            d_circular_array_push(arr, &values[i]);
        }

        // test 2: front returns first element
        elem = (int*)d_circular_array_front(arr);
        result = D_TEST_CHECK_MSG(_suite,
                                  "front_value",
                                  elem != NULL && *elem == 100,
                                  "Front should return first element") && result;

        // test 3: front doesn't modify array
        size_t count_before = arr->count;
        d_circular_array_front(arr);
        result = D_TEST_CHECK_MSG(_suite,
                                  "front_no_modify",
                                  arr->count == count_before,
                                  "Front should not modify array") && result;

        d_circular_array_free(arr);
    }

    // test 4: NULL array
    elem = (int*)d_circular_array_front(NULL);
    result = D_TEST_CHECK_MSG(_suite,
                              "front_null",
                              elem == NULL,
                              "Front on NULL should return NULL") && result;

    return result;
}


/*
d_tests_circular_array_back
  Tests the d_circular_array_back function.
  Tests the following:
  - back on empty array returns NULL
  - back returns last element
  - back works with wraparound
*/
bool d_tests_circular_array_back(
    struct dj_test_suite* _suite
)
{
    bool                      result;
    struct dj_circular_array*  arr;
    int                       values[] = {100, 200, 300, 400};
    int*                      elem;

    d_test_suite_test(_suite, "circular_array_back");

    result = true;

    arr = d_circular_array_new(5, sizeof(int));

    if (arr)
    {
        // test 1: back on empty array
        elem = (int*)d_circular_array_back(arr);
        result = D_TEST_CHECK_MSG(_suite,
                                  "back_empty",
                                  elem == NULL,
                                  "Back on empty array should return NULL") && result;

        // add elements
        for (int i = 0; i < 3; i++)
        {
            d_circular_array_push(arr, &values[i]);
        }

        // test 2: back returns last element
        elem = (int*)d_circular_array_back(arr);
        result = D_TEST_CHECK_MSG(_suite,
                                  "back_value",
                                  elem != NULL && *elem == 300,
                                  "Back should return last element") && result;

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
        result = D_TEST_CHECK_MSG(_suite,
                                  "back_wraparound",
                                  elem != NULL && *elem == 400,
                                  "Back should work with wraparound") && result;

        d_circular_array_free(arr);
    }

    // test 4: NULL array
    elem = (int*)d_circular_array_back(NULL);
    result = D_TEST_CHECK_MSG(_suite,
                              "back_null",
                              elem == NULL,
                              "Back on NULL should return NULL") && result;

    return result;
}


/*
d_tests_circular_array_peek
  Tests the d_circular_array_peek function.
  Tests the following:
  - peek on empty array returns NULL
  - peek returns front element (same as front)
  - peek doesn't modify array
*/
bool d_tests_circular_array_peek(
    struct dj_test_suite* _suite
)
{
    bool                      result;
    struct dj_circular_array*  arr;
    int                       values[] = {100, 200, 300};
    int*                      elem;

    d_test_suite_test(_suite, "circular_array_peek");

    result = true;

    arr = d_circular_array_new(5, sizeof(int));

    if (arr)
    {
        // test 1: peek on empty array
        elem = (int*)d_circular_array_peek(arr);
        result = D_TEST_CHECK_MSG(_suite,
                                  "peek_empty",
                                  elem == NULL,
                                  "Peek on empty array should return NULL") && result;

        // add elements
        for (int i = 0; i < 3; i++)
        {
            d_circular_array_push(arr, &values[i]);
        }

        // test 2: peek returns front element
        elem = (int*)d_circular_array_peek(arr);
        result = D_TEST_CHECK_MSG(_suite,
                                  "peek_value",
                                  elem != NULL && *elem == 100,
                                  "Peek should return front element") && result;

        // test 3: peek doesn't modify array
        size_t count_before = arr->count;
        size_t head_before = arr->head;
        d_circular_array_peek(arr);
        result = D_TEST_CHECK_MSG(_suite,
                                  "peek_no_modify",
                                  arr->count == count_before && arr->head == head_before,
                                  "Peek should not modify array") && result;

        d_circular_array_free(arr);
    }

    // test 4: NULL array
    elem = (int*)d_circular_array_peek(NULL);
    result = D_TEST_CHECK_MSG(_suite,
                              "peek_null",
                              elem == NULL,
                              "Peek on NULL should return NULL") && result;

    return result;
}


/*
d_tests_circular_array_peek_back
  Tests the d_circular_array_peek_back function.
  Tests the following:
  - peek_back on empty array returns NULL
  - peek_back returns last element
  - peek_back doesn't modify array
*/
bool d_tests_circular_array_peek_back(
    struct dj_test_suite* _suite
)
{
    bool                      result;
    struct dj_circular_array*  arr;
    int                       values[] = {100, 200, 300};
    int*                      elem;

    d_test_suite_test(_suite, "circular_array_peek_back");

    result = true;

    arr = d_circular_array_new(5, sizeof(int));

    if (arr)
    {
        // test 1: peek_back on empty array
        elem = (int*)d_circular_array_peek_back(arr);
        result = D_TEST_CHECK_MSG(_suite,
                                  "peek_back_empty",
                                  elem == NULL,
                                  "Peek back on empty array should return NULL") && result;

        // add elements
        for (int i = 0; i < 3; i++)
        {
            d_circular_array_push(arr, &values[i]);
        }

        // test 2: peek_back returns last element
        elem = (int*)d_circular_array_peek_back(arr);
        result = D_TEST_CHECK_MSG(_suite,
                                  "peek_back_value",
                                  elem != NULL && *elem == 300,
                                  "Peek back should return last element") && result;

        // test 3: peek_back doesn't modify array
        size_t count_before = arr->count;
        d_circular_array_peek_back(arr);
        result = D_TEST_CHECK_MSG(_suite,
                                  "peek_back_no_modify",
                                  arr->count == count_before,
                                  "Peek back should not modify array") && result;

        d_circular_array_free(arr);
    }

    // test 4: NULL array
    elem = (int*)d_circular_array_peek_back(NULL);
    result = D_TEST_CHECK_MSG(_suite,
                              "peek_back_null",
                              elem == NULL,
                              "Peek back on NULL should return NULL") && result;

    return result;
}


/*
d_tests_circular_array_access_all
  Aggregation function that runs all element access tests.
*/
bool d_tests_circular_array_access_all(
    struct dj_test_suite* _suite
)
{
    bool result;

    d_test_suite_block(_suite, "circular_array_access_all");

    result = true;

    result = d_tests_circular_array_get(_suite) && result;
    result = d_tests_circular_array_set(_suite) && result;
    result = d_tests_circular_array_front(_suite) && result;
    result = d_tests_circular_array_back(_suite) && result;
    result = d_tests_circular_array_peek(_suite) && result;
    result = d_tests_circular_array_peek_back(_suite) && result;

    return result;
}
