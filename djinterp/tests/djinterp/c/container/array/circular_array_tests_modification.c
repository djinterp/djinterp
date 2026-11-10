/*******************************************************************************
* djinterp [container]                       circular_array_tests_modification.c
*
*
* path:      /tests/djinterp/c/container/array/circular_array_tests_modification.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.02.25
*                                                            revised: 2026.11.09
*******************************************************************************/
// djinterp
#include "./circular_array_tests.h"
#include "../../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's C face


/******************************************************************************
 * III. MODIFICATION FUNCTION TESTS - PUSH/POP OPERATIONS
 *****************************************************************************/

/*
d_tests_circular_array_push
  Tests the d_circular_array_push function.
  Tests the following:
  - push to empty array
  - push multiple elements
  - push to full array fails
  - push updates count and tail
*/
bool d_tests_circular_array_push(
    struct dj_test_suite* _suite
)
{
    bool                      result;
    struct dj_circular_array*  arr;
    int                       values[] = {10, 20, 30, 40, 50};
    int*                      elem;

    d_test_suite_test(_suite, "circular_array_push");

    result = true;

    arr = d_circular_array_new(3, sizeof(int));

    if (arr)
    {
        // test 1: push to empty array
        result = D_TEST_CHECK_MSG(_suite,
                                  "push_empty",
                                  d_circular_array_push(arr, &values[0]) == true,
                                  "Push to empty array should succeed") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "push_empty_count",
                                  arr->count == 1,
                                  "Count should be 1 after push") && result;

        // test 2: push multiple elements
        d_circular_array_push(arr, &values[1]);
        d_circular_array_push(arr, &values[2]);

        result = D_TEST_CHECK_MSG(_suite,
                                  "push_multiple_count",
                                  arr->count == 3,
                                  "Count should be 3 after three pushes") && result;

        elem = (int*)d_circular_array_get(arr, 0);
        result = D_TEST_CHECK_MSG(_suite,
                                  "push_order_first",
                                  elem != NULL && *elem == 10,
                                  "First element should be 10") && result;

        elem = (int*)d_circular_array_get(arr, 2);
        result = D_TEST_CHECK_MSG(_suite,
                                  "push_order_last",
                                  elem != NULL && *elem == 30,
                                  "Last element should be 30") && result;

        // test 3: push to full array fails
        result = D_TEST_CHECK_MSG(_suite,
                                  "push_full",
                                  d_circular_array_push(arr, &values[3]) == false,
                                  "Push to full array should fail") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "push_full_count_unchanged",
                                  arr->count == 3,
                                  "Count should remain 3") && result;

        d_circular_array_free(arr);
    }

    // test 4: NULL array
    result = D_TEST_CHECK_MSG(_suite,
                              "push_null_array",
                              d_circular_array_push(NULL, &values[0]) == false,
                              "Push to NULL array should fail") && result;

    return result;
}


/*
d_tests_circular_array_push_front
  Tests the d_circular_array_push_front function.
  Tests the following:
  - push_front to empty array
  - push_front adds to beginning
  - push_front to full array fails
*/
bool d_tests_circular_array_push_front(
    struct dj_test_suite* _suite
)
{
    bool                      result;
    struct dj_circular_array*  arr;
    int                       values[] = {10, 20, 30};
    int*                      elem;

    d_test_suite_test(_suite, "circular_array_push_front");

    result = true;

    arr = d_circular_array_new(3, sizeof(int));

    if (arr)
    {
        // test 1: push_front to empty array
        result = D_TEST_CHECK_MSG(_suite,
                                  "push_front_empty",
                                  d_circular_array_push_front(arr, &values[0]) == true,
                                  "Push front to empty should succeed") && result;

        // test 2: push_front adds to beginning
        d_circular_array_push_front(arr, &values[1]);

        elem = (int*)d_circular_array_get(arr, 0);
        result = D_TEST_CHECK_MSG(_suite,
                                  "push_front_order",
                                  elem != NULL && *elem == 20,
                                  "Push front should add to beginning") && result;

        elem = (int*)d_circular_array_get(arr, 1);
        result = D_TEST_CHECK_MSG(_suite,
                                  "push_front_second",
                                  elem != NULL && *elem == 10,
                                  "Original element should be second") && result;

        // test 3: push_front to full array fails
        d_circular_array_push_front(arr, &values[2]);  // fill it
        result = D_TEST_CHECK_MSG(_suite,
                                  "push_front_full",
                                  d_circular_array_push_front(arr, &values[0]) == false,
                                  "Push front to full should fail") && result;

        d_circular_array_free(arr);
    }

    // test 4: NULL array
    result = D_TEST_CHECK_MSG(_suite,
                              "push_front_null",
                              d_circular_array_push_front(NULL, &values[0]) == false,
                              "Push front to NULL should fail") && result;

    return result;
}


/*
d_tests_circular_array_push_back
  Tests the d_circular_array_push_back function (alias for push).
  Tests the following:
  - push_back adds to end
  - same behavior as push
*/
bool d_tests_circular_array_push_back(
    struct dj_test_suite* _suite
)
{
    bool                      result;
    struct dj_circular_array*  arr;
    int                       values[] = {10, 20};
    int*                      elem;

    d_test_suite_test(_suite, "circular_array_push_back");

    result = true;

    arr = d_circular_array_new(5, sizeof(int));

    if (arr)
    {
        d_circular_array_push_back(arr, &values[0]);
        d_circular_array_push_back(arr, &values[1]);

        elem = (int*)d_circular_array_get(arr, 0);
        result = D_TEST_CHECK_MSG(_suite,
                                  "push_back_first",
                                  elem != NULL && *elem == 10,
                                  "First element should be 10") && result;

        elem = (int*)d_circular_array_get(arr, 1);
        result = D_TEST_CHECK_MSG(_suite,
                                  "push_back_second",
                                  elem != NULL && *elem == 20,
                                  "Second element should be 20") && result;

        d_circular_array_free(arr);
    }

    return result;
}


/*
d_tests_circular_array_push_all
  Tests the d_circular_array_push_all function.
  Tests the following:
  - push_all multiple elements
  - push_all partial when not enough space
  - push_all to full array fails
*/
bool d_tests_circular_array_push_all(
    struct dj_test_suite* _suite
)
{
    bool                      result;
    struct dj_circular_array*  arr;
    int                       values[] = {10, 20, 30, 40, 50};
    int*                      elem;

    d_test_suite_test(_suite, "circular_array_push_all");

    result = true;

    arr = d_circular_array_new(5, sizeof(int));

    if (arr)
    {
        // test 1: push_all multiple elements
        result = D_TEST_CHECK_MSG(_suite,
                                  "push_all_valid",
                                  d_circular_array_push_all(arr, values, 3) == true,
                                  "Push all should succeed") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "push_all_count",
                                  arr->count == 3,
                                  "Count should be 3") && result;

        elem = (int*)d_circular_array_get(arr, 0);
        result = D_TEST_CHECK_MSG(_suite,
                                  "push_all_first",
                                  elem != NULL && *elem == 10,
                                  "First element should be 10") && result;

        elem = (int*)d_circular_array_get(arr, 2);
        result = D_TEST_CHECK_MSG(_suite,
                                  "push_all_last",
                                  elem != NULL && *elem == 30,
                                  "Last element should be 30") && result;

        // test 2: push_all when not enough space
        result = D_TEST_CHECK_MSG(_suite,
                                  "push_all_overflow",
                                  d_circular_array_push_all(arr, &values[3], 3) == false,
                                  "Push all exceeding capacity should fail") && result;

        d_circular_array_free(arr);
    }

    // test 3: NULL array
    result = D_TEST_CHECK_MSG(_suite,
                              "push_all_null",
                              d_circular_array_push_all(NULL, values, 3) == false,
                              "Push all to NULL should fail") && result;

    // test 4: NULL elements
    arr = d_circular_array_new(5, sizeof(int));

    if (arr)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "push_all_null_elements",
                                  d_circular_array_push_all(arr, NULL, 3) == false,
                                  "Push all with NULL elements should fail") && result;

        d_circular_array_free(arr);
    }

    return result;
}


/*
d_tests_circular_array_push_all_front
  Tests the d_circular_array_push_all_front function.
  Tests the following:
  - push_all_front adds elements to beginning
  - order preserved
*/
bool d_tests_circular_array_push_all_front(
    struct dj_test_suite* _suite
)
{
    bool                      result;
    struct dj_circular_array*  arr;
    int                       initial = 100;
    int                       values[] = {10, 20, 30};
    int*                      elem;

    d_test_suite_test(_suite, "circular_array_push_all_front");

    result = true;

    arr = d_circular_array_new(5, sizeof(int));

    if (arr)
    {
        d_circular_array_push(arr, &initial);

        result = D_TEST_CHECK_MSG(_suite,
                                  "push_all_front_valid",
                                  d_circular_array_push_all_front(arr, values, 3) == true,
                                  "Push all front should succeed") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "push_all_front_count",
                                  arr->count == 4,
                                  "Count should be 4") && result;

        // order should be: 10, 20, 30, 100
        elem = (int*)d_circular_array_get(arr, 0);
        result = D_TEST_CHECK_MSG(_suite,
                                  "push_all_front_first",
                                  elem != NULL && *elem == 10,
                                  "First element should be 10") && result;

        elem = (int*)d_circular_array_get(arr, 3);
        result = D_TEST_CHECK_MSG(_suite,
                                  "push_all_front_last",
                                  elem != NULL && *elem == 100,
                                  "Last element should be original 100") && result;

        d_circular_array_free(arr);
    }

    return result;
}


/*
d_tests_circular_array_pop
  Tests the d_circular_array_pop function.
  Tests the following:
  - pop from non-empty array
  - pop returns elements in FIFO order
  - pop from empty array returns NULL
  - pop updates count and head
*/
bool d_tests_circular_array_pop(
    struct dj_test_suite* _suite
)
{
    bool                      result;
    struct dj_circular_array*  arr;
    int                       values[] = {10, 20, 30};
    int*                      elem;

    d_test_suite_test(_suite, "circular_array_pop");

    result = true;

    arr = d_circular_array_new(5, sizeof(int));

    if (arr)
    {
        // test 1: pop from empty array
        elem = (int*)d_circular_array_pop(arr);
        result = D_TEST_CHECK_MSG(_suite,
                                  "pop_empty",
                                  elem == NULL,
                                  "Pop from empty should return NULL") && result;

        // add elements
        for (int i = 0; i < 3; i++)
        {
            d_circular_array_push(arr, &values[i]);
        }

        // test 2: pop returns first element (FIFO)
        elem = (int*)d_circular_array_pop(arr);
        result = D_TEST_CHECK_MSG(_suite,
                                  "pop_first",
                                  elem != NULL && *elem == 10,
                                  "Pop should return first element") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "pop_count",
                                  arr->count == 2,
                                  "Count should decrease after pop") && result;

        // test 3: pop sequence maintains FIFO
        elem = (int*)d_circular_array_pop(arr);
        result = D_TEST_CHECK_MSG(_suite,
                                  "pop_second",
                                  elem != NULL && *elem == 20,
                                  "Second pop should return 20") && result;

        elem = (int*)d_circular_array_pop(arr);
        result = D_TEST_CHECK_MSG(_suite,
                                  "pop_third",
                                  elem != NULL && *elem == 30,
                                  "Third pop should return 30") && result;

        // test 4: array now empty
        result = D_TEST_CHECK_MSG(_suite,
                                  "pop_now_empty",
                                  d_circular_array_is_empty(arr) == true,
                                  "Array should be empty after all pops") && result;

        d_circular_array_free(arr);
    }

    // test 5: NULL array
    elem = (int*)d_circular_array_pop(NULL);
    result = D_TEST_CHECK_MSG(_suite,
                              "pop_null",
                              elem == NULL,
                              "Pop from NULL should return NULL") && result;

    return result;
}


/*
d_tests_circular_array_pop_front
  Tests the d_circular_array_pop_front function (alias for pop).
*/
bool d_tests_circular_array_pop_front(
    struct dj_test_suite* _suite
)
{
    bool                      result;
    struct dj_circular_array*  arr;
    int                       values[] = {10, 20, 30};
    int*                      elem;

    d_test_suite_test(_suite, "circular_array_pop_front");

    result = true;

    arr = d_circular_array_new(5, sizeof(int));

    if (arr)
    {
        for (int i = 0; i < 3; i++)
        {
            d_circular_array_push(arr, &values[i]);
        }

        elem = (int*)d_circular_array_pop_front(arr);
        result = D_TEST_CHECK_MSG(_suite,
                                  "pop_front_value",
                                  elem != NULL && *elem == 10,
                                  "Pop front should return first element") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "pop_front_count",
                                  arr->count == 2,
                                  "Count should decrease") && result;

        d_circular_array_free(arr);
    }

    return result;
}


/*
d_tests_circular_array_pop_back
  Tests the d_circular_array_pop_back function.
  Tests the following:
  - pop_back removes last element
  - pop_back from empty returns NULL
*/
bool d_tests_circular_array_pop_back(
    struct dj_test_suite* _suite
)
{
    bool                      result;
    struct dj_circular_array*  arr;
    int                       values[] = {10, 20, 30};
    int*                      elem;

    d_test_suite_test(_suite, "circular_array_pop_back");

    result = true;

    arr = d_circular_array_new(5, sizeof(int));

    if (arr)
    {
        // test 1: pop_back from empty
        elem = (int*)d_circular_array_pop_back(arr);
        result = D_TEST_CHECK_MSG(_suite,
                                  "pop_back_empty",
                                  elem == NULL,
                                  "Pop back from empty should return NULL") && result;

        // add elements
        for (int i = 0; i < 3; i++)
        {
            d_circular_array_push(arr, &values[i]);
        }

        // test 2: pop_back returns last element
        elem = (int*)d_circular_array_pop_back(arr);
        result = D_TEST_CHECK_MSG(_suite,
                                  "pop_back_last",
                                  elem != NULL && *elem == 30,
                                  "Pop back should return last element") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "pop_back_count",
                                  arr->count == 2,
                                  "Count should decrease") && result;

        // test 3: verify remaining elements
        elem = (int*)d_circular_array_back(arr);
        result = D_TEST_CHECK_MSG(_suite,
                                  "pop_back_new_last",
                                  elem != NULL && *elem == 20,
                                  "New last element should be 20") && result;

        d_circular_array_free(arr);
    }

    // test 4: NULL array
    elem = (int*)d_circular_array_pop_back(NULL);
    result = D_TEST_CHECK_MSG(_suite,
                              "pop_back_null",
                              elem == NULL,
                              "Pop back from NULL should return NULL") && result;

    return result;
}


/*
d_tests_circular_array_pop_to
  Tests the d_circular_array_pop_to function.
  Tests the following:
  - pop_to copies value to output
  - pop_to on empty returns false
*/
bool d_tests_circular_array_pop_to(
    struct dj_test_suite* _suite
)
{
    bool                      result;
    struct dj_circular_array*  arr;
    int                       values[] = {10, 20, 30};
    int                       output;

    d_test_suite_test(_suite, "circular_array_pop_to");

    result = true;

    arr = d_circular_array_new(5, sizeof(int));

    if (arr)
    {
        // test 1: pop_to on empty
        result = D_TEST_CHECK_MSG(_suite,
                                  "pop_to_empty",
                                  d_circular_array_pop_to(arr, &output) == false,
                                  "Pop to on empty should fail") && result;

        // add elements
        for (int i = 0; i < 3; i++)
        {
            d_circular_array_push(arr, &values[i]);
        }

        // test 2: pop_to copies value
        result = D_TEST_CHECK_MSG(_suite,
                                  "pop_to_success",
                                  d_circular_array_pop_to(arr, &output) == true,
                                  "Pop to should succeed") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "pop_to_value",
                                  output == 10,
                                  "Output should be 10") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "pop_to_count",
                                  arr->count == 2,
                                  "Count should decrease") && result;

        d_circular_array_free(arr);
    }

    // test 3: NULL array
    result = D_TEST_CHECK_MSG(_suite,
                              "pop_to_null_array",
                              d_circular_array_pop_to(NULL, &output) == false,
                              "Pop to with NULL array should fail") && result;

    return result;
}


/*
d_tests_circular_array_push_overwrite
  Tests the d_circular_array_push_overwrite function.
  Tests the following:
  - push_overwrite to non-full array
  - push_overwrite to full array overwrites oldest
*/
bool d_tests_circular_array_push_overwrite(
    struct dj_test_suite* _suite
)
{
    bool                      result;
    struct dj_circular_array*  arr;
    int                       values[] = {10, 20, 30, 40};
    int*                      elem;

    d_test_suite_test(_suite, "circular_array_push_overwrite");

    result = true;

    arr = d_circular_array_new(3, sizeof(int));

    if (arr)
    {
        // test 1: push_overwrite to non-full array
        result = D_TEST_CHECK_MSG(_suite,
                                  "push_overwrite_empty",
                                  d_circular_array_push_overwrite(arr, &values[0]) == true,
                                  "Push overwrite to empty should succeed") && result;

        d_circular_array_push_overwrite(arr, &values[1]);
        d_circular_array_push_overwrite(arr, &values[2]);

        result = D_TEST_CHECK_MSG(_suite,
                                  "push_overwrite_full",
                                  arr->count == 3,
                                  "Array should be full") && result;

        // test 2: push_overwrite to full array overwrites oldest
        result = D_TEST_CHECK_MSG(_suite,
                                  "push_overwrite_overflow",
                                  d_circular_array_push_overwrite(arr, &values[3]) == true,
                                  "Push overwrite to full should succeed") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "push_overwrite_count_same",
                                  arr->count == 3,
                                  "Count should still be 3") && result;

        // oldest (10) should be overwritten, now 20, 30, 40
        elem = (int*)d_circular_array_get(arr, 0);
        result = D_TEST_CHECK_MSG(_suite,
                                  "push_overwrite_new_first",
                                  elem != NULL && *elem == 20,
                                  "First element should now be 20") && result;

        elem = (int*)d_circular_array_get(arr, 2);
        result = D_TEST_CHECK_MSG(_suite,
                                  "push_overwrite_new_last",
                                  elem != NULL && *elem == 40,
                                  "Last element should be 40") && result;

        d_circular_array_free(arr);
    }

    // test 3: NULL array
    result = D_TEST_CHECK_MSG(_suite,
                              "push_overwrite_null",
                              d_circular_array_push_overwrite(NULL, &values[0]) == false,
                              "Push overwrite to NULL should fail") && result;

    return result;
}


/*
d_tests_circular_array_modification_all
  Aggregation function that runs all modification tests.
*/
bool d_tests_circular_array_modification_all(
    struct dj_test_suite* _suite
)
{
    bool result;

    d_test_suite_block(_suite, "circular_array_modification_all");

    result = true;

    result = d_tests_circular_array_push(_suite) && result;
    result = d_tests_circular_array_push_front(_suite) && result;
    result = d_tests_circular_array_push_back(_suite) && result;
    result = d_tests_circular_array_push_all(_suite) && result;
    result = d_tests_circular_array_push_all_front(_suite) && result;
    result = d_tests_circular_array_pop(_suite) && result;
    result = d_tests_circular_array_pop_front(_suite) && result;
    result = d_tests_circular_array_pop_back(_suite) && result;
    result = d_tests_circular_array_pop_to(_suite) && result;
    result = d_tests_circular_array_push_overwrite(_suite) && result;

    return result;
}
