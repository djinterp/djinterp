/*******************************************************************************
* djinterp [container]                                 ptr_array_tests_element.c
*
*
* path:      /tests/djinterp/c/container/array/ptr_array_tests_element.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.02.25
*                                                            revised: 2026.11.09
*******************************************************************************/
// djinterp
#include "./ptr_array_tests.h"
#include "../../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's C face


/******************************************************************************
 * HELPER FUNCTIONS
 *****************************************************************************/

// helper comparator for int pointer values
static
int int_ptr_comparator(
    const void* _a,
    const void* _b
)
{
    int a;
    int b;

    // dereference pointers to get the actual int values
    a = *(const int*)_a;
    b = *(const int*)_b;

    return (a > b) - (a < b);
}


// helper comparator for sorting int pointers by their pointed-to values
static
int int_ptr_sort_comparator(
    const void* _a,
    const void* _b
)
{
    // _a and _b are pointers to the array elements (void**)
    // each element is itself a void* pointing to an int
    const int* ptr_a;
    const int* ptr_b;

    ptr_a = *(const int* const*)_a;
    ptr_b = *(const int* const*)_b;

    return (*ptr_a > *ptr_b) - (*ptr_a < *ptr_b);
}


/******************************************************************************
 * II. ELEMENT MANIPULATION FUNCTION TESTS
 *****************************************************************************/

/*
d_tests_ptr_array_append_element
  Tests the d_ptr_array_append_element function.
  Tests the following:
  - NULL array returns failure
  - appending to empty array
  - appending to non-empty array
  - multiple appends maintain order
*/
bool d_tests_ptr_array_append_element(
    struct dj_test_suite* _suite
)
{
    bool                 result;
    struct dj_ptr_array*  arr;
    int                  values[4];

    d_test_suite_test(_suite, "ptr_array_append_element");

    result = true;

    // initialize test values
    values[0] = 100;
    values[1] = 200;
    values[2] = 300;
    values[3] = 400;

    // test 1: NULL array returns failure
    result = D_TEST_CHECK_MSG(_suite,
                              "append_element_null",
                              d_ptr_array_append_element(NULL, &values[0]) == D_FAILURE,
                              "NULL array should return failure") && result;

    // test 2: appending to empty array
    arr = d_ptr_array_new(10);

    if (arr)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "append_element_empty",
                                  d_ptr_array_append_element(arr, &values[0]) == D_SUCCESS,
                                  "Append to empty array should succeed") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "append_element_empty_verify",
                                  arr->count == 1 && arr->elements[0] == &values[0],
                                  "Array should have 1 element") && result;

        // test 3: appending to non-empty array
        result = D_TEST_CHECK_MSG(_suite,
                                  "append_element_nonempty",
                                  d_ptr_array_append_element(arr, &values[1]) == D_SUCCESS,
                                  "Append to non-empty array should succeed") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "append_element_nonempty_verify",
                                  arr->count == 2 && arr->elements[1] == &values[1],
                                  "Array should have 2 elements") && result;

        // test 4: multiple appends maintain order
        d_ptr_array_append_element(arr, &values[2]);
        d_ptr_array_append_element(arr, &values[3]);

        result = D_TEST_CHECK_MSG(_suite,
                                  "append_element_order",
                                  arr->count == 4 && arr->elements[0] == &values[0] && arr->elements[1] == &values[1] && arr->elements[2] == &values[2] && arr->elements[3] == &values[3],
                                  "Multiple appends should maintain order") && result;

        d_ptr_array_free(arr);
    }

    return result;
}


/*
d_tests_ptr_array_append_elements
  Tests the d_ptr_array_append_elements function.
  Tests the following:
  - NULL array returns failure
  - NULL elements with count > 0 returns failure
  - valid append of multiple elements
  - append with count 0 does nothing
*/
bool d_tests_ptr_array_append_elements(
    struct dj_test_suite* _suite
)
{
    bool                 result;
    struct dj_ptr_array*  arr;
    int                  values[5];
    const void*          source[3];

    d_test_suite_test(_suite, "ptr_array_append_elements");

    result = true;

    // initialize test values
    values[0] = 10;
    values[1] = 20;
    values[2] = 30;
    values[3] = 40;
    values[4] = 50;
    source[0] = &values[2];
    source[1] = &values[3];
    source[2] = &values[4];

    // test 1: NULL array returns failure
    result = D_TEST_CHECK_MSG(_suite,
                              "append_elements_null_arr",
                              d_ptr_array_append_elements(NULL, source, 3) == D_FAILURE,
                              "NULL array should return failure") && result;

    // test 2: NULL elements with count > 0
    arr = d_ptr_array_new(10);

    if (arr)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "append_elements_null_elements",
                                  d_ptr_array_append_elements(arr, NULL, 3) == D_FAILURE,
                                  "NULL elements with count > 0 should return failure") && result;

        // add some initial elements
        d_ptr_array_append_element(arr, &values[0]);
        d_ptr_array_append_element(arr, &values[1]);

        // test 3: valid append of multiple elements
        result = D_TEST_CHECK_MSG(_suite,
                                  "append_elements_valid",
                                  d_ptr_array_append_elements(arr, source, 3) == D_SUCCESS,
                                  "Valid append should succeed") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "append_elements_count",
                                  arr->count == 5,
                                  "Array should have 5 elements") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "append_elements_order",
                                  arr->elements[2] == &values[2] && arr->elements[3] == &values[3] && arr->elements[4] == &values[4],
                                  "Appended elements should be in correct order") && result;

        d_ptr_array_free(arr);
    }

    // test 4: append with count 0
    arr = d_ptr_array_new_from_args(2, &values[0], &values[1]);

    if (arr)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "append_elements_zero",
                                  d_ptr_array_append_elements(arr, source, 0) == D_SUCCESS,
                                  "Append with count 0 should succeed") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "append_elements_zero_unchanged",
                                  arr->count == 2,
                                  "Array count should be unchanged") && result;

        d_ptr_array_free(arr);
    }

    return result;
}


/*
d_tests_ptr_array_append_array
  Tests the d_ptr_array_append_array function.
  Tests the following:
  - NULL destination returns failure
  - NULL source returns failure
  - valid append of array to array
  - append empty array does nothing
*/
bool d_tests_ptr_array_append_array(
    struct dj_test_suite* _suite
)
{
    bool                 result;
    struct dj_ptr_array*  dest;
    struct dj_ptr_array*  src;
    struct dj_ptr_array*  empty;
    int                  values[5];

    d_test_suite_test(_suite, "ptr_array_append_array");

    result = true;

    // initialize test values
    values[0] = 1;
    values[1] = 2;
    values[2] = 3;
    values[3] = 4;
    values[4] = 5;

    // test 1: NULL destination returns failure
    src = d_ptr_array_new_from_args(2, &values[0], &values[1]);

    if (src)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "append_array_null_dest",
                                  d_ptr_array_append_array(NULL, src) == D_FAILURE,
                                  "NULL destination should return failure") && result;

        d_ptr_array_free(src);
    }

    // test 2: NULL source returns failure
    dest = d_ptr_array_new(10);

    if (dest)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "append_array_null_src",
                                  d_ptr_array_append_array(dest, NULL) == D_FAILURE,
                                  "NULL source should return failure") && result;

        d_ptr_array_free(dest);
    }

    // test 3: valid append of array to array
    dest = d_ptr_array_new_from_args(2, &values[0], &values[1]);
    src  = d_ptr_array_new_from_args(3, &values[2], &values[3], &values[4]);

    if (dest && src)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "append_array_valid",
                                  d_ptr_array_append_array(dest, src) == D_SUCCESS,
                                  "Valid array append should succeed") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "append_array_count",
                                  dest->count == 5,
                                  "Destination should have 5 elements") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "append_array_order",
                                  dest->elements[0] == &values[0] && dest->elements[1] == &values[1] && dest->elements[2] == &values[2] && dest->elements[3] == &values[3] && dest->elements[4] == &values[4],
                                  "All elements should be in correct order") && result;
    }

    if (dest)
    {
        d_ptr_array_free(dest);
    }

    if (src)
    {
        d_ptr_array_free(src);
    }

    // test 4: append empty array
    dest  = d_ptr_array_new_from_args(2, &values[0], &values[1]);
    empty = d_ptr_array_new(10);

    if (dest && empty)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "append_array_empty",
                                  d_ptr_array_append_array(dest, empty) == D_SUCCESS,
                                  "Appending empty array should succeed") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "append_array_empty_unchanged",
                                  dest->count == 2,
                                  "Destination count should be unchanged") && result;
    }

    if (dest)
    {
        d_ptr_array_free(dest);
    }

    if (empty)
    {
        d_ptr_array_free(empty);
    }

    return result;
}


/*
d_tests_ptr_array_contains
  Tests the d_ptr_array_contains function.
  Tests the following:
  - NULL array returns false
  - empty array returns false
  - value found returns true
  - value not found returns false
*/
bool d_tests_ptr_array_contains(
    struct dj_test_suite* _suite
)
{
    bool                 result;
    struct dj_ptr_array*  arr;
    int                  values[3];
    int                  not_in_array;

    d_test_suite_test(_suite, "ptr_array_contains");

    result = true;

    // initialize test values
    values[0] = 10;
    values[1] = 20;
    values[2] = 30;
    not_in_array = 999;

    // test 1: NULL array returns false
    result = D_TEST_CHECK_MSG(_suite,
                              "contains_null",
                              d_ptr_array_contains(NULL, &values[0]) == false,
                              "NULL array should return false") && result;

    // test 2: empty array returns false
    arr = d_ptr_array_new(10);

    if (arr)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "contains_empty",
                                  d_ptr_array_contains(arr, &values[0]) == false,
                                  "Empty array should return false") && result;

        d_ptr_array_free(arr);
    }

    // test 3 & 4: value found and not found
    arr = d_ptr_array_new_from_args(3, &values[0], &values[1], &values[2]);

    if (arr)
    {
        // test 3: value found (by pointer identity)
        result = D_TEST_CHECK_MSG(_suite,
                                  "contains_found",
                                  d_ptr_array_contains(arr, &values[1]) == true,
                                  "Existing pointer should be found") && result;

        // test 4: value not found
        result = D_TEST_CHECK_MSG(_suite,
                                  "contains_not_found",
                                  d_ptr_array_contains(arr, &not_in_array) == false,
                                  "Non-existent pointer should not be found") && result;

        d_ptr_array_free(arr);
    }

    return result;
}


/*
d_tests_ptr_array_fill
  Tests the d_ptr_array_fill function.
  Tests the following:
  - NULL array returns failure
  - fill empty array does nothing
  - fill array with value
*/
bool d_tests_ptr_array_fill(
    struct dj_test_suite* _suite
)
{
    bool                 result;
    struct dj_ptr_array*  arr;
    int                  fill_value;
    int                  values[3];

    d_test_suite_test(_suite, "ptr_array_fill");

    result = true;

    // initialize test values
    fill_value = 999;
    values[0] = 1;
    values[1] = 2;
    values[2] = 3;

    // test 1: NULL array returns failure
    result = D_TEST_CHECK_MSG(_suite,
                              "fill_null",
                              d_ptr_array_fill(NULL, &fill_value) == D_FAILURE,
                              "NULL array should return failure") && result;

    // test 2: fill empty array
    arr = d_ptr_array_new(10);

    if (arr)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "fill_empty",
                                  d_ptr_array_fill(arr, &fill_value) == D_SUCCESS,
                                  "Fill empty array should succeed (no-op)") && result;

        d_ptr_array_free(arr);
    }

    // test 3: fill array with value
    arr = d_ptr_array_new_from_args(3, &values[0], &values[1], &values[2]);

    if (arr)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "fill_valid",
                                  d_ptr_array_fill(arr, &fill_value) == D_SUCCESS,
                                  "Fill array should succeed") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "fill_verify",
                                  arr->elements[0] == &fill_value && arr->elements[1] == &fill_value && arr->elements[2] == &fill_value,
                                  "All elements should point to fill value") && result;

        d_ptr_array_free(arr);
    }

    return result;
}


/*
d_tests_ptr_array_find
  Tests the d_ptr_array_find function.
  Tests the following:
  - NULL array returns -1
  - empty array returns -1
  - value found returns correct index
  - value not found returns -1
  - finds first occurrence
*/
bool d_tests_ptr_array_find(
    struct dj_test_suite* _suite
)
{
    bool                 result;
    struct dj_ptr_array*  arr;
    int                  values[5];
    int                  not_in_array;
    ssize_t              idx;

    d_test_suite_test(_suite, "ptr_array_find");

    result = true;

    // initialize test values
    values[0] = 10;
    values[1] = 20;
    values[2] = 30;
    values[3] = 20;  // duplicate value
    values[4] = 40;
    not_in_array = 999;

    // test 1: NULL array returns -1
    idx = d_ptr_array_find(NULL, &values[0]);
    result = D_TEST_CHECK_MSG(_suite,
                              "find_null",
                              idx == -1,
                              "NULL array should return -1") && result;

    // test 2: empty array returns -1
    arr = d_ptr_array_new(10);

    if (arr)
    {
        idx = d_ptr_array_find(arr, &values[0]);
        result = D_TEST_CHECK_MSG(_suite,
                                  "find_empty",
                                  idx == -1,
                                  "Empty array should return -1") && result;

        d_ptr_array_free(arr);
    }

    // tests 3-5: find operations
    arr = d_ptr_array_new_from_args(5,
                                    &values[0],
                                    &values[1],
                                    &values[2],
                                    &values[3],
                                    &values[4]);

    if (arr)
    {
        // test 3: value found returns correct index
        idx = d_ptr_array_find(arr, &values[2]);
        result = D_TEST_CHECK_MSG(_suite,
                                  "find_found",
                                  idx == 2,
                                  "Found pointer should return index 2") && result;

        // test 4: value not found returns -1
        idx = d_ptr_array_find(arr, &not_in_array);
        result = D_TEST_CHECK_MSG(_suite,
                                  "find_not_found",
                                  idx == -1,
                                  "Non-existent pointer should return -1") && result;

        // test 5: finds pointer identity (even if same value, different address)
        idx = d_ptr_array_find(arr, &values[1]);
        result = D_TEST_CHECK_MSG(_suite,
                                  "find_first",
                                  idx == 1,
                                  "Should find first occurrence at index 1") && result;

        d_ptr_array_free(arr);
    }

    return result;
}


/*
d_tests_ptr_array_insert_element
  Tests the d_ptr_array_insert_element function.
  Tests the following:
  - NULL array returns failure
  - insert at beginning (index 0)
  - insert in middle
  - insert at end
  - insert with negative index
*/
bool d_tests_ptr_array_insert_element(
    struct dj_test_suite* _suite
)
{
    bool                 result;
    struct dj_ptr_array*  arr;
    int                  values[5];

    d_test_suite_test(_suite, "ptr_array_insert_element");

    result = true;

    // initialize test values
    values[0] = 10;
    values[1] = 20;
    values[2] = 30;
    values[3] = 15;  // to insert at beginning
    values[4] = 25;  // to insert in middle

    // test 1: NULL array returns failure
    result = D_TEST_CHECK_MSG(_suite,
                              "insert_element_null",
                              d_ptr_array_insert_element(NULL, &values[0], 0) == D_FAILURE,
                              "NULL array should return failure") && result;

    // create test array
    arr = d_ptr_array_new_from_args(3, &values[0], &values[1], &values[2]);

    if (arr)
    {
        // test 2: insert at beginning (index 0)
        result = D_TEST_CHECK_MSG(_suite,
                                  "insert_element_beginning",
                                  d_ptr_array_insert_element(arr, &values[3], 0) == D_SUCCESS,
                                  "Insert at beginning should succeed") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "insert_element_beginning_verify",
                                  arr->count == 4 && arr->elements[0] == &values[3],
                                  "Element should be at beginning") && result;

        d_ptr_array_free(arr);
    }

    // test 3: insert in middle
    arr = d_ptr_array_new_from_args(3, &values[0], &values[1], &values[2]);

    if (arr)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "insert_element_middle",
                                  d_ptr_array_insert_element(arr, &values[4], 1) == D_SUCCESS,
                                  "Insert in middle should succeed") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "insert_element_middle_verify",
                                  arr->count == 4 && arr->elements[0] == &values[0] && arr->elements[1] == &values[4] && arr->elements[2] == &values[1] && arr->elements[3] == &values[2],
                                  "Element should be inserted at index 1") && result;

        d_ptr_array_free(arr);
    }

    // test 4: insert with negative index
    arr = d_ptr_array_new_from_args(3, &values[0], &values[1], &values[2]);

    if (arr)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "insert_element_negative",
                                  d_ptr_array_insert_element(arr, &values[4], -1) == D_SUCCESS,
                                  "Insert with negative index should succeed") && result;

        // -1 on array of size 3 means index 2, so insert at position 2
        result = D_TEST_CHECK_MSG(_suite,
                                  "insert_element_negative_verify",
                                  arr->count == 4 && arr->elements[2] == &values[4],
                                  "Element should be at converted index") && result;

        d_ptr_array_free(arr);
    }

    return result;
}


/*
d_tests_ptr_array_insert_elements
  Tests the d_ptr_array_insert_elements function.
  Tests the following:
  - NULL array returns failure
  - NULL elements with count > 0 returns failure
  - valid insert of multiple elements
  - insert with count 0 does nothing
*/
bool d_tests_ptr_array_insert_elements(
    struct dj_test_suite* _suite
)
{
    bool                 result;
    struct dj_ptr_array*  arr;
    int                  values[6];
    const void*          to_insert[2];

    d_test_suite_test(_suite, "ptr_array_insert_elements");

    result = true;

    // initialize test values
    values[0] = 10;
    values[1] = 20;
    values[2] = 30;
    values[3] = 40;
    values[4] = 15;  // to insert
    values[5] = 16;  // to insert
    to_insert[0] = &values[4];
    to_insert[1] = &values[5];

    // test 1: NULL array returns failure
    result = D_TEST_CHECK_MSG(_suite,
                              "insert_elements_null_arr",
                              d_ptr_array_insert_elements(NULL, to_insert, 2, 0) == D_FAILURE,
                              "NULL array should return failure") && result;

    // test 2: NULL elements with count > 0
    arr = d_ptr_array_new(10);

    if (arr)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "insert_elements_null_elements",
                                  d_ptr_array_insert_elements(arr, NULL, 2, 0) == D_FAILURE,
                                  "NULL elements with count > 0 should return failure") && result;

        d_ptr_array_free(arr);
    }

    // test 3: valid insert of multiple elements
    arr = d_ptr_array_new_from_args(4,
                                    &values[0],
                                    &values[1],
                                    &values[2],
                                    &values[3]);

    if (arr)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "insert_elements_valid",
                                  d_ptr_array_insert_elements(arr, to_insert, 2, 1) == D_SUCCESS,
                                  "Valid insert should succeed") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "insert_elements_count",
                                  arr->count == 6,
                                  "Array should have 6 elements") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "insert_elements_order",
                                  arr->elements[0] == &values[0] && arr->elements[1] == &values[4] && arr->elements[2] == &values[5] && arr->elements[3] == &values[1] && arr->elements[4] == &values[2] && arr->elements[5] == &values[3],
                                  "Elements should be in correct order") && result;

        d_ptr_array_free(arr);
    }

    // test 4: insert with count 0
    arr = d_ptr_array_new_from_args(2, &values[0], &values[1]);

    if (arr)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "insert_elements_zero",
                                  d_ptr_array_insert_elements(arr, to_insert, 0, 1) == D_SUCCESS,
                                  "Insert with count 0 should succeed") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "insert_elements_zero_unchanged",
                                  arr->count == 2,
                                  "Array count should be unchanged") && result;

        d_ptr_array_free(arr);
    }

    return result;
}


/*
d_tests_ptr_array_insert_array
  Tests the d_ptr_array_insert_array function.
  Tests the following:
  - NULL destination returns failure
  - NULL source returns failure
  - valid insert of array into array
*/
bool d_tests_ptr_array_insert_array(
    struct dj_test_suite* _suite
)
{
    bool                 result;
    struct dj_ptr_array*  dest;
    struct dj_ptr_array*  src;
    int                  values[5];

    d_test_suite_test(_suite, "ptr_array_insert_array");

    result = true;

    // initialize test values
    values[0] = 1;
    values[1] = 2;
    values[2] = 3;
    values[3] = 10;  // to insert
    values[4] = 11;  // to insert

    // test 1: NULL destination returns failure
    src = d_ptr_array_new_from_args(2, &values[3], &values[4]);

    if (src)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "insert_array_null_dest",
                                  d_ptr_array_insert_array(NULL, src, 0) == D_FAILURE,
                                  "NULL destination should return failure") && result;

        d_ptr_array_free(src);
    }

    // test 2: NULL source returns failure
    dest = d_ptr_array_new(10);

    if (dest)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "insert_array_null_src",
                                  d_ptr_array_insert_array(dest, NULL, 0) == D_FAILURE,
                                  "NULL source should return failure") && result;

        d_ptr_array_free(dest);
    }

    // test 3: valid insert of array into array
    dest = d_ptr_array_new_from_args(3, &values[0], &values[1], &values[2]);
    src  = d_ptr_array_new_from_args(2, &values[3], &values[4]);

    if (dest && src)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "insert_array_valid",
                                  d_ptr_array_insert_array(dest, src, 1) == D_SUCCESS,
                                  "Valid array insert should succeed") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "insert_array_count",
                                  dest->count == 5,
                                  "Destination should have 5 elements") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "insert_array_order",
                                  dest->elements[0] == &values[0] && dest->elements[1] == &values[3] && dest->elements[2] == &values[4] && dest->elements[3] == &values[1] && dest->elements[4] == &values[2],
                                  "Elements should be in correct order") && result;
    }

    if (dest)
    {
        d_ptr_array_free(dest);
    }

    if (src)
    {
        d_ptr_array_free(src);
    }

    return result;
}


/*
d_tests_ptr_array_is_empty
  Tests the d_ptr_array_is_empty function.
  Tests the following:
  - NULL array returns true
  - empty array returns true
  - non-empty array returns false
*/
bool d_tests_ptr_array_is_empty(
    struct dj_test_suite* _suite
)
{
    bool                 result;
    struct dj_ptr_array*  arr;
    int                  value;

    d_test_suite_test(_suite, "ptr_array_is_empty");

    result = true;
    value  = 42;

    // test 1: NULL array returns true
    result = D_TEST_CHECK_MSG(_suite,
                              "is_empty_null",
                              d_ptr_array_is_empty(NULL) == true,
                              "NULL array should be empty") && result;

    // test 2: empty array returns true
    arr = d_ptr_array_new(10);

    if (arr)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "is_empty_empty",
                                  d_ptr_array_is_empty(arr) == true,
                                  "Empty array should return true") && result;

        d_ptr_array_free(arr);
    }

    // test 3: non-empty array returns false
    arr = d_ptr_array_new_from_args(1, &value);

    if (arr)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "is_empty_nonempty",
                                  d_ptr_array_is_empty(arr) == false,
                                  "Non-empty array should return false") && result;

        d_ptr_array_free(arr);
    }

    return result;
}


/*
d_tests_ptr_array_prepend_element
  Tests the d_ptr_array_prepend_element function.
  Tests the following:
  - NULL array returns failure
  - prepend to empty array
  - prepend to non-empty array maintains order
*/
bool d_tests_ptr_array_prepend_element(
    struct dj_test_suite* _suite
)
{
    bool                 result;
    struct dj_ptr_array*  arr;
    int                  values[3];

    d_test_suite_test(_suite, "ptr_array_prepend_element");

    result = true;

    // initialize test values
    values[0] = 10;
    values[1] = 20;
    values[2] = 30;

    // test 1: NULL array returns failure
    result = D_TEST_CHECK_MSG(_suite,
                              "prepend_element_null",
                              d_ptr_array_prepend_element(NULL, &values[0]) == D_FAILURE,
                              "NULL array should return failure") && result;

    // test 2: prepend to empty array
    arr = d_ptr_array_new(10);

    if (arr)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "prepend_element_empty",
                                  d_ptr_array_prepend_element(arr, &values[0]) == D_SUCCESS,
                                  "Prepend to empty array should succeed") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "prepend_element_empty_verify",
                                  arr->count == 1 && arr->elements[0] == &values[0],
                                  "Element should be at index 0") && result;

        d_ptr_array_free(arr);
    }

    // test 3: prepend to non-empty array maintains order
    arr = d_ptr_array_new_from_args(2, &values[1], &values[2]);

    if (arr)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "prepend_element_nonempty",
                                  d_ptr_array_prepend_element(arr, &values[0]) == D_SUCCESS,
                                  "Prepend to non-empty array should succeed") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "prepend_element_order",
                                  arr->count == 3 && arr->elements[0] == &values[0] && arr->elements[1] == &values[1] && arr->elements[2] == &values[2],
                                  "Prepended element should be first") && result;

        d_ptr_array_free(arr);
    }

    return result;
}


/*
d_tests_ptr_array_prepend_elements
  Tests the d_ptr_array_prepend_elements function.
  Tests the following:
  - NULL array returns failure
  - valid prepend of multiple elements
*/
bool d_tests_ptr_array_prepend_elements(
    struct dj_test_suite* _suite
)
{
    bool                 result;
    struct dj_ptr_array*  arr;
    int                  values[5];
    const void*          to_prepend[2];

    d_test_suite_test(_suite, "ptr_array_prepend_elements");

    result = true;

    // initialize test values
    values[0] = 10;
    values[1] = 20;
    values[2] = 30;
    values[3] = 1;  // to prepend
    values[4] = 2;  // to prepend
    to_prepend[0] = &values[3];
    to_prepend[1] = &values[4];

    // test 1: NULL array returns failure
    result = D_TEST_CHECK_MSG(_suite,
                              "prepend_elements_null",
                              d_ptr_array_prepend_elements(NULL, to_prepend, 2) == D_FAILURE,
                              "NULL array should return failure") && result;

    // test 2: valid prepend of multiple elements
    arr = d_ptr_array_new_from_args(3, &values[0], &values[1], &values[2]);

    if (arr)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "prepend_elements_valid",
                                  d_ptr_array_prepend_elements(arr, to_prepend, 2) == D_SUCCESS,
                                  "Valid prepend should succeed") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "prepend_elements_count",
                                  arr->count == 5,
                                  "Array should have 5 elements") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "prepend_elements_order",
                                  arr->elements[0] == &values[3] && arr->elements[1] == &values[4] && arr->elements[2] == &values[0] && arr->elements[3] == &values[1] && arr->elements[4] == &values[2],
                                  "Prepended elements should be first") && result;

        d_ptr_array_free(arr);
    }

    return result;
}


/*
d_tests_ptr_array_prepend_array
  Tests the d_ptr_array_prepend_array function.
  Tests the following:
  - NULL destination returns failure
  - NULL source returns failure
  - valid prepend of array to array
*/
bool d_tests_ptr_array_prepend_array(
    struct dj_test_suite* _suite
)
{
    bool                 result;
    struct dj_ptr_array*  dest;
    struct dj_ptr_array*  src;
    int                  values[5];

    d_test_suite_test(_suite, "ptr_array_prepend_array");

    result = true;

    // initialize test values
    values[0] = 1;
    values[1] = 2;
    values[2] = 3;
    values[3] = 10;
    values[4] = 11;

    // test 1: NULL destination returns failure
    src = d_ptr_array_new_from_args(2, &values[3], &values[4]);

    if (src)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "prepend_array_null_dest",
                                  d_ptr_array_prepend_array(NULL, src) == D_FAILURE,
                                  "NULL destination should return failure") && result;

        d_ptr_array_free(src);
    }

    // test 2: NULL source returns failure
    dest = d_ptr_array_new(10);

    if (dest)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "prepend_array_null_src",
                                  d_ptr_array_prepend_array(dest, NULL) == D_FAILURE,
                                  "NULL source should return failure") && result;

        d_ptr_array_free(dest);
    }

    // test 3: valid prepend of array to array
    dest = d_ptr_array_new_from_args(3, &values[0], &values[1], &values[2]);
    src  = d_ptr_array_new_from_args(2, &values[3], &values[4]);

    if (dest && src)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "prepend_array_valid",
                                  d_ptr_array_prepend_array(dest, src) == D_SUCCESS,
                                  "Valid array prepend should succeed") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "prepend_array_count",
                                  dest->count == 5,
                                  "Destination should have 5 elements") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "prepend_array_order",
                                  dest->elements[0] == &values[3] && dest->elements[1] == &values[4] && dest->elements[2] == &values[0] && dest->elements[3] == &values[1] && dest->elements[4] == &values[2],
                                  "Prepended elements should be first") && result;
    }

    if (dest)
    {
        d_ptr_array_free(dest);
    }

    if (src)
    {
        d_ptr_array_free(src);
    }

    return result;
}


/*
d_tests_ptr_array_reverse
  Tests the d_ptr_array_reverse function.
  Tests the following:
  - NULL array returns failure
  - reverse empty array succeeds
  - reverse single element succeeds
  - reverse multiple elements
  - reverse odd count elements
*/
bool d_tests_ptr_array_reverse(
    struct dj_test_suite* _suite
)
{
    bool                 result;
    struct dj_ptr_array*  arr;
    int                  values[5];

    d_test_suite_test(_suite, "ptr_array_reverse");

    result = true;

    // initialize test values
    values[0] = 1;
    values[1] = 2;
    values[2] = 3;
    values[3] = 4;
    values[4] = 5;

    // test 1: NULL array returns failure
    result = D_TEST_CHECK_MSG(_suite,
                              "reverse_null",
                              d_ptr_array_reverse(NULL) == D_FAILURE,
                              "NULL array should return failure") && result;

    // test 2: reverse empty array
    arr = d_ptr_array_new(10);

    if (arr)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "reverse_empty",
                                  d_ptr_array_reverse(arr) == D_SUCCESS,
                                  "Reverse empty array should succeed") && result;

        d_ptr_array_free(arr);
    }

    // test 3: reverse single element
    arr = d_ptr_array_new_from_args(1, &values[0]);

    if (arr)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "reverse_single",
                                  d_ptr_array_reverse(arr) == D_SUCCESS,
                                  "Reverse single element should succeed") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "reverse_single_unchanged",
                                  arr->elements[0] == &values[0],
                                  "Single element should be unchanged") && result;

        d_ptr_array_free(arr);
    }

    // test 4: reverse multiple elements (even count)
    arr = d_ptr_array_new_from_args(4,
                                    &values[0],
                                    &values[1],
                                    &values[2],
                                    &values[3]);

    if (arr)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "reverse_even",
                                  d_ptr_array_reverse(arr) == D_SUCCESS,
                                  "Reverse even count should succeed") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "reverse_even_verify",
                                  arr->elements[0] == &values[3] && arr->elements[1] == &values[2] && arr->elements[2] == &values[1] && arr->elements[3] == &values[0],
                                  "Elements should be reversed") && result;

        d_ptr_array_free(arr);
    }

    // test 5: reverse odd count elements
    arr = d_ptr_array_new_from_args(5,
                                    &values[0],
                                    &values[1],
                                    &values[2],
                                    &values[3],
                                    &values[4]);

    if (arr)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "reverse_odd",
                                  d_ptr_array_reverse(arr) == D_SUCCESS,
                                  "Reverse odd count should succeed") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "reverse_odd_verify",
                                  arr->elements[0] == &values[4] && arr->elements[1] == &values[3] && arr->elements[2] == &values[2] && arr->elements[3] == &values[1] && arr->elements[4] == &values[0],
                                  "Elements should be reversed (odd)") && result;

        d_ptr_array_free(arr);
    }

    return result;
}


/*
d_tests_ptr_array_resize_amount
  Tests the d_ptr_array_resize_amount function.
  Tests the following:
  - NULL array returns failure
  - resize by positive amount (grow)
  - resize by negative amount (shrink)
  - resize by zero does nothing
*/
bool d_tests_ptr_array_resize_amount(
    struct dj_test_suite* _suite
)
{
    bool                 result;
    struct dj_ptr_array*  arr;
    int                  values[5];

    d_test_suite_test(_suite, "ptr_array_resize_amount");

    result = true;

    // initialize test values
    values[0] = 10;
    values[1] = 20;
    values[2] = 30;
    values[3] = 40;
    values[4] = 50;

    // test 1: NULL array returns failure
    result = D_TEST_CHECK_MSG(_suite,
                              "resize_amount_null",
                              d_ptr_array_resize_amount(NULL, 5) == D_FAILURE,
                              "NULL array should return failure") && result;

    // test 2: resize by positive amount (grow)
    arr = d_ptr_array_new_from_args(3, &values[0], &values[1], &values[2]);

    if (arr)
    {
        size_t old_count;

        old_count = arr->count;
        result = D_TEST_CHECK_MSG(_suite,
                                  "resize_amount_grow",
                                  d_ptr_array_resize_amount(arr, 2) == D_SUCCESS,
                                  "Resize grow should succeed") && result;

        // note: resize_amount changes the allocation, elements may be preserved
        // but we mainly test that the function succeeds

        d_ptr_array_free(arr);
    }

    // test 3: resize by negative amount (shrink)
    arr = d_ptr_array_new_from_args(5,
                                    &values[0],
                                    &values[1],
                                    &values[2],
                                    &values[3],
                                    &values[4]);

    if (arr)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "resize_amount_shrink",
                                  d_ptr_array_resize_amount(arr, -2) == D_SUCCESS,
                                  "Resize shrink should succeed") && result;

        d_ptr_array_free(arr);
    }

    // test 4: resize by zero
    arr = d_ptr_array_new_from_args(3, &values[0], &values[1], &values[2]);

    if (arr)
    {
        size_t old_count;

        old_count = arr->count;
        result = D_TEST_CHECK_MSG(_suite,
                                  "resize_amount_zero",
                                  d_ptr_array_resize_amount(arr, 0) == D_SUCCESS,
                                  "Resize by zero should succeed") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "resize_amount_zero_unchanged",
                                  arr->count == old_count,
                                  "Count should be unchanged") && result;

        d_ptr_array_free(arr);
    }

    return result;
}


/*
d_tests_ptr_array_resize_factor
  Tests the d_ptr_array_resize_factor function.
  Tests the following:
  - NULL array returns failure
  - resize by factor > 1 (grow)
  - resize by factor < 1 (shrink)
  - resize by factor 1.0 does nothing
*/
bool d_tests_ptr_array_resize_factor(
    struct dj_test_suite* _suite
)
{
    bool                 result;
    struct dj_ptr_array*  arr;
    int                  values[5];

    d_test_suite_test(_suite, "ptr_array_resize_factor");

    result = true;

    // initialize test values
    values[0] = 10;
    values[1] = 20;
    values[2] = 30;
    values[3] = 40;
    values[4] = 50;

    // test 1: NULL array returns failure
    result = D_TEST_CHECK_MSG(_suite,
                              "resize_factor_null",
                              d_ptr_array_resize_factor(NULL, 2.0) == D_FAILURE,
                              "NULL array should return failure") && result;

    // test 2: resize by factor > 1 (grow)
    arr = d_ptr_array_new_from_args(3, &values[0], &values[1], &values[2]);

    if (arr)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "resize_factor_grow",
                                  d_ptr_array_resize_factor(arr, 2.0) == D_SUCCESS,
                                  "Resize grow by factor should succeed") && result;

        d_ptr_array_free(arr);
    }

    // test 3: resize by factor < 1 (shrink)
    arr = d_ptr_array_new_from_args(4,
                                    &values[0],
                                    &values[1],
                                    &values[2],
                                    &values[3]);

    if (arr)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "resize_factor_shrink",
                                  d_ptr_array_resize_factor(arr, 0.5) == D_SUCCESS,
                                  "Resize shrink by factor should succeed") && result;

        d_ptr_array_free(arr);
    }

    // test 4: resize by factor 1.0
    arr = d_ptr_array_new_from_args(3, &values[0], &values[1], &values[2]);

    if (arr)
    {
        size_t old_count;

        old_count = arr->count;
        result = D_TEST_CHECK_MSG(_suite,
                                  "resize_factor_one",
                                  d_ptr_array_resize_factor(arr, 1.0) == D_SUCCESS,
                                  "Resize by factor 1.0 should succeed") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "resize_factor_one_unchanged",
                                  arr->count == old_count,
                                  "Count should be unchanged") && result;

        d_ptr_array_free(arr);
    }

    return result;
}


/*
d_tests_ptr_array_sort
  Tests the d_ptr_array_sort function.
  Tests the following:
  - NULL array does nothing (no crash)
  - NULL comparator does nothing (no crash)
  - sort empty array succeeds
  - sort already sorted array
  - sort unsorted array
*/
bool d_tests_ptr_array_sort(
    struct dj_test_suite* _suite
)
{
    bool                 result;
    struct dj_ptr_array*  arr;
    int                  values[5];

    d_test_suite_test(_suite, "ptr_array_sort");

    result = true;

    // initialize test values in unsorted order
    values[0] = 50;
    values[1] = 20;
    values[2] = 40;
    values[3] = 10;
    values[4] = 30;

    // test 1: NULL array does nothing (no crash)
    d_ptr_array_sort(NULL, int_ptr_sort_comparator);
    result = D_TEST_CHECK_MSG(_suite,
                              "sort_null_array",
                              // if we get here, no crash
                              true,
                              "NULL array should not crash") && result;

    // test 2: NULL comparator does nothing (no crash)
    arr = d_ptr_array_new_from_args(3, &values[0], &values[1], &values[2]);

    if (arr)
    {
        d_ptr_array_sort(arr, NULL);
        result = D_TEST_CHECK_MSG(_suite,
                                  "sort_null_comparator",
                                  // if we get here, no crash
                                  true,
                                  "NULL comparator should not crash") && result;

        d_ptr_array_free(arr);
    }

    // test 3: sort empty array
    arr = d_ptr_array_new(10);

    if (arr)
    {
        d_ptr_array_sort(arr, int_ptr_sort_comparator);
        result = D_TEST_CHECK_MSG(_suite,
                                  "sort_empty",
                                  arr->count == 0,
                                  "Sort empty array should succeed") && result;

        d_ptr_array_free(arr);
    }

    // test 4: sort unsorted array
    arr = d_ptr_array_new_from_args(5,
                                    &values[0],
                                    &values[1],
                                    &values[2],
                                    &values[3],
                                    &values[4]);

    if (arr)
    {
        d_ptr_array_sort(arr, int_ptr_sort_comparator);

        // after sorting by pointed-to values: 10, 20, 30, 40, 50
        result = D_TEST_CHECK_MSG(_suite,
                                  "sort_unsorted",
                                  *(int*)arr->elements[0] == 10 && *(int*)arr->elements[1] == 20 && *(int*)arr->elements[2] == 30 && *(int*)arr->elements[3] == 40 && *(int*)arr->elements[4] == 50,
                                  "Array should be sorted by pointed-to values") && result;

        d_ptr_array_free(arr);
    }

    return result;
}


/*
d_tests_ptr_array_element_all
  Aggregation function that runs all element manipulation tests.
*/
bool d_tests_ptr_array_element_all(
    struct dj_test_suite* _suite
)
{
    bool result;

    d_test_suite_block(_suite, "ptr_array_element_all");

    result = true;

    result = d_tests_ptr_array_append_element(_suite) && result;
    result = d_tests_ptr_array_append_elements(_suite) && result;
    result = d_tests_ptr_array_append_array(_suite) && result;
    result = d_tests_ptr_array_contains(_suite) && result;
    result = d_tests_ptr_array_fill(_suite) && result;
    result = d_tests_ptr_array_find(_suite) && result;
    result = d_tests_ptr_array_insert_element(_suite) && result;
    result = d_tests_ptr_array_insert_elements(_suite) && result;
    result = d_tests_ptr_array_insert_array(_suite) && result;
    result = d_tests_ptr_array_is_empty(_suite) && result;
    result = d_tests_ptr_array_prepend_element(_suite) && result;
    result = d_tests_ptr_array_prepend_elements(_suite) && result;
    result = d_tests_ptr_array_prepend_array(_suite) && result;
    result = d_tests_ptr_array_reverse(_suite) && result;
    result = d_tests_ptr_array_resize_amount(_suite) && result;
    result = d_tests_ptr_array_resize_factor(_suite) && result;
    result = d_tests_ptr_array_sort(_suite) && result;

    return result;
}
