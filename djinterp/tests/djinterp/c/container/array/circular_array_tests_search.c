/*******************************************************************************
* djinterp [container]                             circular_array_tests_search.c
*
*
* path:      /tests/djinterp/c/container/array/circular_array_tests_search.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.02.25
*                                                            revised: 2026.11.09
*******************************************************************************/
// djinterp
#include "./circular_array_tests.h"
#include "../../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's C face


/******************************************************************************
 * HELPER FUNCTIONS
 *****************************************************************************/

// helper comparator for int values
static
int int_comparator(
    const void* _a,
    const void* _b
)
{
    int a = *(const int*)_a;
    int b = *(const int*)_b;

    return (a > b) - (a < b);
}


/******************************************************************************
 * VI. SEARCH FUNCTION TESTS
 *****************************************************************************/

/*
d_tests_circular_array_contains
  Tests the d_circular_array_contains function.
  Tests the following:
  - contains returns true for existing value
  - contains returns false for non-existing value
  - contains on empty array returns false
*/
bool d_tests_circular_array_contains(
    struct dj_test_suite* _suite
)
{
    bool                      result;
    struct dj_circular_array*  arr;
    int                       values[] = {10, 20, 30, 40, 50};
    int                       search_value;

    d_test_suite_test(_suite, "circular_array_contains");

    result = true;

    arr = d_circular_array_new(5, sizeof(int));

    if (arr)
    {
        // test 1: contains on empty array
        search_value = 10;
        result = D_TEST_CHECK_MSG(_suite,
                                  "contains_empty",
                                  d_circular_array_contains(arr, &search_value, int_comparator) == false,
                                  "Contains on empty should return false") && result;

        // add elements
        for (int i = 0; i < 5; i++)
        {
            d_circular_array_push(arr, &values[i]);
        }

        // test 2: contains returns true for existing value
        search_value = 30;
        result = D_TEST_CHECK_MSG(_suite,
                                  "contains_found",
                                  d_circular_array_contains(arr, &search_value, int_comparator) == true,
                                  "Contains should find existing value") && result;

        // test 3: contains returns false for non-existing
        search_value = 99;
        result = D_TEST_CHECK_MSG(_suite,
                                  "contains_not_found",
                                  d_circular_array_contains(arr, &search_value, int_comparator) == false,
                                  "Contains should not find non-existing value") && result;

        d_circular_array_free(arr);
    }

    // test 4: NULL array
    search_value = 10;
    result = D_TEST_CHECK_MSG(_suite,
                              "contains_null",
                              d_circular_array_contains(NULL, &search_value, int_comparator) == false,
                              "Contains on NULL should return false") && result;

    return result;
}


/*
d_tests_circular_array_find
  Tests the d_circular_array_find function.
  Tests the following:
  - find returns correct index for existing value
  - find returns -1 for non-existing value
  - find returns first occurrence
*/
bool d_tests_circular_array_find(
    struct dj_test_suite* _suite
)
{
    bool                      result;
    struct dj_circular_array*  arr;
    int                       values[] = {10, 20, 30, 20, 50};
    int                       search_value;

    d_test_suite_test(_suite, "circular_array_find");

    result = true;

    arr = d_circular_array_new(5, sizeof(int));

    if (arr)
    {
        // add elements (note: 20 appears twice)
        for (int i = 0; i < 5; i++)
        {
            d_circular_array_push(arr, &values[i]);
        }

        // test 1: find returns correct index
        search_value = 30;
        result = D_TEST_CHECK_MSG(_suite,
                                  "find_index",
                                  d_circular_array_find(arr, &search_value, int_comparator) == 2,
                                  "Find should return index 2 for value 30") && result;

        // test 2: find returns first occurrence
        search_value = 20;
        result = D_TEST_CHECK_MSG(_suite,
                                  "find_first_occurrence",
                                  d_circular_array_find(arr, &search_value, int_comparator) == 1,
                                  "Find should return first occurrence (index 1)") && result;

        // test 3: find returns -1 for non-existing
        search_value = 99;
        result = D_TEST_CHECK_MSG(_suite,
                                  "find_not_found",
                                  d_circular_array_find(arr, &search_value, int_comparator) == -1,
                                  "Find should return -1 for non-existing") && result;

        d_circular_array_free(arr);
    }

    // test 4: find on empty array
    arr = d_circular_array_new(5, sizeof(int));

    if (arr)
    {
        search_value = 10;
        result = D_TEST_CHECK_MSG(_suite,
                                  "find_empty",
                                  d_circular_array_find(arr, &search_value, int_comparator) == -1,
                                  "Find on empty should return -1") && result;

        d_circular_array_free(arr);
    }

    // test 5: NULL array
    search_value = 10;
    result = D_TEST_CHECK_MSG(_suite,
                              "find_null",
                              d_circular_array_find(NULL, &search_value, int_comparator) == -1,
                              "Find on NULL should return -1") && result;

    return result;
}


/*
d_tests_circular_array_find_last
  Tests the d_circular_array_find_last function.
  Tests the following:
  - find_last returns last occurrence index
  - find_last returns -1 for non-existing value
*/
bool d_tests_circular_array_find_last(
    struct dj_test_suite* _suite
)
{
    bool                      result;
    struct dj_circular_array*  arr;
    int                       values[] = {10, 20, 30, 20, 50};
    int                       search_value;

    d_test_suite_test(_suite, "circular_array_find_last");

    result = true;

    arr = d_circular_array_new(5, sizeof(int));

    if (arr)
    {
        // add elements (note: 20 appears at index 1 and 3)
        for (int i = 0; i < 5; i++)
        {
            d_circular_array_push(arr, &values[i]);
        }

        // test 1: find_last returns last occurrence
        search_value = 20;
        result = D_TEST_CHECK_MSG(_suite,
                                  "find_last_occurrence",
                                  d_circular_array_find_last(arr, &search_value, int_comparator) == 3,
                                  "Find last should return last occurrence (index 3)") && result;

        // test 2: find_last returns -1 for non-existing
        search_value = 99;
        result = D_TEST_CHECK_MSG(_suite,
                                  "find_last_not_found",
                                  d_circular_array_find_last(arr, &search_value, int_comparator) == -1,
                                  "Find last should return -1 for non-existing") && result;

        // test 3: find_last with single occurrence
        search_value = 10;
        result = D_TEST_CHECK_MSG(_suite,
                                  "find_last_single",
                                  d_circular_array_find_last(arr, &search_value, int_comparator) == 0,
                                  "Find last with single occurrence should return that index") && result;

        d_circular_array_free(arr);
    }

    // test 4: NULL array
    search_value = 10;
    result = D_TEST_CHECK_MSG(_suite,
                              "find_last_null",
                              d_circular_array_find_last(NULL, &search_value, int_comparator) == -1,
                              "Find last on NULL should return -1") && result;

    return result;
}


/*
d_tests_circular_array_count_value
  Tests the d_circular_array_count_value function.
  Tests the following:
  - count_value returns correct count for duplicates
  - count_value returns 0 for non-existing
  - count_value on empty array returns 0
*/
bool d_tests_circular_array_count_value(
    struct dj_test_suite* _suite
)
{
    bool                      result;
    struct dj_circular_array*  arr;
    int                       values[] = {10, 20, 30, 20, 20};
    int                       search_value;

    d_test_suite_test(_suite, "circular_array_count_value");

    result = true;

    arr = d_circular_array_new(5, sizeof(int));

    if (arr)
    {
        // add elements (20 appears 3 times)
        for (int i = 0; i < 5; i++)
        {
            d_circular_array_push(arr, &values[i]);
        }

        // test 1: count_value with duplicates
        search_value = 20;
        result = D_TEST_CHECK_MSG(_suite,
                                  "count_value_duplicates",
                                  d_circular_array_count_value(arr, &search_value, int_comparator) == 3,
                                  "Count value should return 3 for value 20") && result;

        // test 2: count_value with single occurrence
        search_value = 10;
        result = D_TEST_CHECK_MSG(_suite,
                                  "count_value_single",
                                  d_circular_array_count_value(arr, &search_value, int_comparator) == 1,
                                  "Count value should return 1 for value 10") && result;

        // test 3: count_value for non-existing
        search_value = 99;
        result = D_TEST_CHECK_MSG(_suite,
                                  "count_value_not_found",
                                  d_circular_array_count_value(arr, &search_value, int_comparator) == 0,
                                  "Count value should return 0 for non-existing") && result;

        d_circular_array_free(arr);
    }

    // test 4: count_value on empty array
    arr = d_circular_array_new(5, sizeof(int));

    if (arr)
    {
        search_value = 10;
        result = D_TEST_CHECK_MSG(_suite,
                                  "count_value_empty",
                                  d_circular_array_count_value(arr, &search_value, int_comparator) == 0,
                                  "Count value on empty should return 0") && result;

        d_circular_array_free(arr);
    }

    // test 5: NULL array
    search_value = 10;
    result = D_TEST_CHECK_MSG(_suite,
                              "count_value_null",
                              d_circular_array_count_value(NULL, &search_value, int_comparator) == 0,
                              "Count value on NULL should return 0") && result;

    return result;
}


/*
d_tests_circular_array_search_all
  Aggregation function that runs all search tests.
*/
bool d_tests_circular_array_search_all(
    struct dj_test_suite* _suite
)
{
    bool result;

    d_test_suite_block(_suite, "circular_array_search_all");

    result = true;

    result = d_tests_circular_array_contains(_suite) && result;
    result = d_tests_circular_array_find(_suite) && result;
    result = d_tests_circular_array_find_last(_suite) && result;
    result = d_tests_circular_array_count_value(_suite) && result;

    return result;
}
