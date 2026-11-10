/*******************************************************************************
* djinterp [container]                              circular_array_tests_query.c
*
*
* path:      /tests/djinterp/c/container/array/circular_array_tests_query.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.02.25
*                                                            revised: 2026.11.09
*******************************************************************************/
// djinterp
#include "./circular_array_tests.h"
#include "../../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's C face


/******************************************************************************
 * V. QUERY FUNCTION TESTS
 *****************************************************************************/

/*
d_tests_circular_array_is_empty
  Tests the d_circular_array_is_empty function.
  Tests the following:
  - is_empty on empty array returns true
  - is_empty on non-empty array returns false
  - is_empty on NULL returns true
*/
bool d_tests_circular_array_is_empty(
    struct dj_test_suite* _suite
)
{
    bool                      result;
    struct dj_circular_array*  arr;
    int                       value = 42;

    d_test_suite_test(_suite, "circular_array_is_empty");

    result = true;

    arr = d_circular_array_new(5, sizeof(int));

    if (arr)
    {
        // test 1: is_empty on empty array
        result = D_TEST_CHECK_MSG(_suite,
                                  "is_empty_true",
                                  d_circular_array_is_empty(arr) == true,
                                  "Empty array should return true") && result;

        // add element
        d_circular_array_push(arr, &value);

        // test 2: is_empty on non-empty array
        result = D_TEST_CHECK_MSG(_suite,
                                  "is_empty_false",
                                  d_circular_array_is_empty(arr) == false,
                                  "Non-empty array should return false") && result;

        d_circular_array_free(arr);
    }

    // test 3: is_empty on NULL returns false (per implementation)
    result = D_TEST_CHECK_MSG(_suite,
                              "is_empty_null",
                              d_circular_array_is_empty(NULL) == false,
                              "NULL array should return false") && result;

    return result;
}


/*
d_tests_circular_array_is_full
  Tests the d_circular_array_is_full function.
  Tests the following:
  - is_full on empty array returns false
  - is_full on partially filled returns false
  - is_full on full array returns true
*/
bool d_tests_circular_array_is_full(
    struct dj_test_suite* _suite
)
{
    bool                      result;
    struct dj_circular_array*  arr;
    int                       values[] = {10, 20, 30};

    d_test_suite_test(_suite, "circular_array_is_full");

    result = true;

    arr = d_circular_array_new(3, sizeof(int));

    if (arr)
    {
        // test 1: is_full on empty array
        result = D_TEST_CHECK_MSG(_suite,
                                  "is_full_empty",
                                  d_circular_array_is_full(arr) == false,
                                  "Empty array should not be full") && result;

        // add one element
        d_circular_array_push(arr, &values[0]);

        // test 2: is_full on partially filled
        result = D_TEST_CHECK_MSG(_suite,
                                  "is_full_partial",
                                  d_circular_array_is_full(arr) == false,
                                  "Partially filled array should not be full") && result;

        // fill it up
        d_circular_array_push(arr, &values[1]);
        d_circular_array_push(arr, &values[2]);

        // test 3: is_full on full array
        result = D_TEST_CHECK_MSG(_suite,
                                  "is_full_true",
                                  d_circular_array_is_full(arr) == true,
                                  "Full array should return true") && result;

        d_circular_array_free(arr);
    }

    // test 4: is_full on NULL
    result = D_TEST_CHECK_MSG(_suite,
                              "is_full_null",
                              d_circular_array_is_full(NULL) == false,
                              "NULL array should return false") && result;

    return result;
}


/*
d_tests_circular_array_count
  Tests the d_circular_array_count function.
  Tests the following:
  - count on empty array returns 0
  - count after push operations
  - count after pop operations
*/
bool d_tests_circular_array_count(
    struct dj_test_suite* _suite
)
{
    bool                      result;
    struct dj_circular_array*  arr;
    int                       values[] = {10, 20, 30};

    d_test_suite_test(_suite, "circular_array_count");

    result = true;

    arr = d_circular_array_new(5, sizeof(int));

    if (arr)
    {
        // test 1: count on empty array
        result = D_TEST_CHECK_MSG(_suite,
                                  "count_empty",
                                  d_circular_array_count(arr) == 0,
                                  "Empty array count should be 0") && result;

        // add elements
        d_circular_array_push(arr, &values[0]);
        d_circular_array_push(arr, &values[1]);
        d_circular_array_push(arr, &values[2]);

        // test 2: count after push
        result = D_TEST_CHECK_MSG(_suite,
                                  "count_after_push",
                                  d_circular_array_count(arr) == 3,
                                  "Count should be 3 after 3 pushes") && result;

        // pop one element
        d_circular_array_pop(arr);

        // test 3: count after pop
        result = D_TEST_CHECK_MSG(_suite,
                                  "count_after_pop",
                                  d_circular_array_count(arr) == 2,
                                  "Count should be 2 after pop") && result;

        d_circular_array_free(arr);
    }

    // test 4: count on NULL
    result = D_TEST_CHECK_MSG(_suite,
                              "count_null",
                              d_circular_array_count(NULL) == 0,
                              "NULL array count should be 0") && result;

    return result;
}


/*
d_tests_circular_array_capacity
  Tests the d_circular_array_capacity function.
  Tests the following:
  - capacity returns correct value
  - capacity unchanged after operations
*/
bool d_tests_circular_array_capacity(
    struct dj_test_suite* _suite
)
{
    bool                      result;
    struct dj_circular_array*  arr;
    int                       value = 42;

    d_test_suite_test(_suite, "circular_array_capacity");

    result = true;

    arr = d_circular_array_new(10, sizeof(int));

    if (arr)
    {
        // test 1: capacity returns correct value
        result = D_TEST_CHECK_MSG(_suite,
                                  "capacity_value",
                                  d_circular_array_capacity(arr) == 10,
                                  "Capacity should be 10") && result;

        // add element
        d_circular_array_push(arr, &value);

        // test 2: capacity unchanged after operations
        result = D_TEST_CHECK_MSG(_suite,
                                  "capacity_unchanged",
                                  d_circular_array_capacity(arr) == 10,
                                  "Capacity should remain 10") && result;

        d_circular_array_free(arr);
    }

    // test 3: capacity on NULL
    result = D_TEST_CHECK_MSG(_suite,
                              "capacity_null",
                              d_circular_array_capacity(NULL) == 0,
                              "NULL array capacity should be 0") && result;

    return result;
}


/*
d_tests_circular_array_available_space
  Tests the d_circular_array_available_space function.
  Tests the following:
  - available_space on empty array equals capacity
  - available_space decreases after push
  - available_space on full array is 0
*/
bool d_tests_circular_array_available_space(
    struct dj_test_suite* _suite
)
{
    bool                      result;
    struct dj_circular_array*  arr;
    int                       values[] = {10, 20, 30, 40, 50};

    d_test_suite_test(_suite, "circular_array_available_space");

    result = true;

    arr = d_circular_array_new(5, sizeof(int));

    if (arr)
    {
        // test 1: available_space on empty equals capacity
        result = D_TEST_CHECK_MSG(_suite,
                                  "available_space_empty",
                                  d_circular_array_available_space(arr) == 5,
                                  "Available space should be 5 on empty") && result;

        // add elements
        d_circular_array_push(arr, &values[0]);
        d_circular_array_push(arr, &values[1]);

        // test 2: available_space decreases
        result = D_TEST_CHECK_MSG(_suite,
                                  "available_space_partial",
                                  d_circular_array_available_space(arr) == 3,
                                  "Available space should be 3") && result;

        // fill it up
        d_circular_array_push(arr, &values[2]);
        d_circular_array_push(arr, &values[3]);
        d_circular_array_push(arr, &values[4]);

        // test 3: available_space on full is 0
        result = D_TEST_CHECK_MSG(_suite,
                                  "available_space_full",
                                  d_circular_array_available_space(arr) == 0,
                                  "Available space should be 0 when full") && result;

        d_circular_array_free(arr);
    }

    // test 4: available_space on NULL
    result = D_TEST_CHECK_MSG(_suite,
                              "available_space_null",
                              d_circular_array_available_space(NULL) == 0,
                              "NULL array available space should be 0") && result;

    return result;
}


/*
d_tests_circular_array_element_size
  Tests the d_circular_array_element_size function.
  Tests the following:
  - element_size returns correct value
*/
bool d_tests_circular_array_element_size(
    struct dj_test_suite* _suite
)
{
    bool                      result;
    struct dj_circular_array*  arr;

    d_test_suite_test(_suite, "circular_array_element_size");

    result = true;

    // test 1: element_size for int
    arr = d_circular_array_new(5, sizeof(int));

    if (arr)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "element_size_int",
                                  d_circular_array_element_size(arr) == sizeof(int),
                                  "Element size should be sizeof(int)") && result;

        d_circular_array_free(arr);
    }

    // test 2: element_size for double
    arr = d_circular_array_new(5, sizeof(double));

    if (arr)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "element_size_double",
                                  d_circular_array_element_size(arr) == sizeof(double),
                                  "Element size should be sizeof(double)") && result;

        d_circular_array_free(arr);
    }

    // test 3: element_size on NULL
    result = D_TEST_CHECK_MSG(_suite,
                              "element_size_null",
                              d_circular_array_element_size(NULL) == 0,
                              "NULL array element size should be 0") && result;

    return result;
}


/*
d_tests_circular_array_query_all
  Aggregation function that runs all query tests.
*/
bool d_tests_circular_array_query_all(
    struct dj_test_suite* _suite
)
{
    bool result;

    d_test_suite_block(_suite, "circular_array_query_all");

    result = true;

    result = d_tests_circular_array_is_empty(_suite) && result;
    result = d_tests_circular_array_is_full(_suite) && result;
    result = d_tests_circular_array_count(_suite) && result;
    result = d_tests_circular_array_capacity(_suite) && result;
    result = d_tests_circular_array_available_space(_suite) && result;
    result = d_tests_circular_array_element_size(_suite) && result;

    return result;
}
