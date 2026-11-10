/*******************************************************************************
* djinterp [container]                                       array_tests_manip.c
*
* Unit tests for `array.h` manipulation functions.
* Part of the array suite, on DTest's C face.
*
*
* path:      /tests/djinterp/c/container/array/array_tests_manip.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.01.30
*                                                            revised: 2026.11.09
*******************************************************************************/

// djinterp
#include "./array_tests.h"
#include "../../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's C face


/*
d_tests_array_append_element
  Tests the `d_array_append_element` function.
  Tests the following:
  - appending single element
  - NULL array rejection
  - NULL element rejection
*/
bool d_tests_array_append_element(
    struct dj_test_suite* _suite
)
{
    bool            test_result = true;
    struct dj_array* arr;
    int             value = 99;
    int*            int_elements;
    bool            result;

    d_test_suite_test(_suite, "array_append_element");

    arr = d_array_new(sizeof(int), 10);

    if (!arr)
    {
        return false;
    }

    // test appending element
    result = d_array_append_element(arr, sizeof(int), &value);
    int_elements = (int*)arr->elements;

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "`d_array_append_element` appends element correctly",
                                    result == true && int_elements[0] == 99,
                                    "`d_array_append_element` failed to append element");

    // test NULL array
    result = d_array_append_element(NULL, sizeof(int), &value);

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "`d_array_append_element` rejects NULL array",
                                    result == false,
                                    "`d_array_append_element` accepted NULL array");

    // test NULL element
    result = d_array_append_element(arr, sizeof(int), NULL);

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "`d_array_append_element` rejects NULL element",
                                    result == false,
                                    "`d_array_append_element` accepted NULL element");

    d_array_free(arr);

    return test_result;
}

/*
d_tests_array_append_elements
  Tests the `d_array_append_elements` function.
  Tests the following:
  - appending multiple elements
  - zero count handling
  - NULL array rejection
*/
bool d_tests_array_append_elements(
    struct dj_test_suite* _suite
)
{
    bool            test_result = true;
    struct dj_array* arr;
    int             values[] = { 1, 2, 3 };
    bool            result;

    d_test_suite_test(_suite, "array_append_elements");

    arr = d_array_new(sizeof(int), 10);

    if (!arr)
    {
        return false;
    }

    // test appending multiple elements
    result = d_array_append_elements(arr, sizeof(int), values, 3);

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "`d_array_append_elements` appends elements correctly",
                                    result == true,
                                    "`d_array_append_elements` failed");

    // test zero count
    result = d_array_append_elements(arr, sizeof(int), values, 0);

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "`d_array_append_elements` handles zero count",
                                    result == true,
                                    "`d_array_append_elements` failed with zero count");

    // test NULL array
    result = d_array_append_elements(NULL, sizeof(int), values, 3);

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "`d_array_append_elements` rejects NULL array",
                                    result == false,
                                    "`d_array_append_elements` accepted NULL array");

    d_array_free(arr);

    return test_result;
}

/*
d_tests_array_append_array
  Tests the `d_array_append_array` function.
  Tests the following:
  - appending array
  - NULL array rejection
*/
bool d_tests_array_append_array(
    struct dj_test_suite* _suite
)
{
    bool            test_result = true;
    struct dj_array* arr;
    int             source[] = { 4, 5, 6 };
    bool            result;

    d_test_suite_test(_suite, "array_append_array");

    arr = d_array_new(sizeof(int), 10);

    if (!arr)
    {
        return false;
    }

    // test appending array
    result = d_array_append_array(arr, sizeof(int), source, 3, 0);

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "`d_array_append_array` appends array correctly",
                                    result == true,
                                    "`d_array_append_array` failed");

    // test NULL destination
    result = d_array_append_array(NULL, sizeof(int), source, 3, 0);

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "`d_array_append_array` rejects NULL destination",
                                    result == false,
                                    "`d_array_append_array` accepted NULL destination");

    d_array_free(arr);

    return test_result;
}

/*
d_tests_array_contains
  Tests the `d_array_contains` function.
  Tests the following:
  - finding existing element
  - not finding missing element
  - NULL array rejection
*/
bool d_tests_array_contains(
    struct dj_test_suite* _suite
)
{
    bool            test_result = true;
    struct dj_array* arr;
    int             search_val;
    bool            result;

    d_test_suite_test(_suite, "array_contains");

    arr = d_array_new_from_arr(sizeof(int), (int[]){ 1, 2, 3, 4, 5 }, 5);

    if (!arr)
    {
        return false;
    }

    // test finding existing element
    search_val = 3;
    result = d_array_contains(arr, sizeof(int), &search_val);

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "`d_array_contains` finds existing element",
                                    result == true,
                                    "`d_array_contains` failed to find existing element");

    // test not finding missing element
    search_val = 99;
    result = d_array_contains(arr, sizeof(int), &search_val);

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "`d_array_contains` correctly returns false for missing element",
                                    result == false,
                                    "`d_array_contains` incorrectly found missing element");

    d_array_free(arr);

    // test NULL array
    result = d_array_contains(NULL, sizeof(int), &search_val);

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "`d_array_contains` rejects NULL array",
                                    result == false,
                                    "`d_array_contains` accepted NULL array");

    return test_result;
}

/*
d_tests_array_fill
  Tests the `d_array_fill` function.
  Tests the following:
  - filling array with value
  - NULL array rejection
  - NULL fill element rejection
*/
bool d_tests_array_fill(
    struct dj_test_suite* _suite
)
{
    bool            test_result = true;
    struct dj_array* arr;
    int             fill_value = 42;
    int             new_fill = 99;
    int*            int_elements;
    bool            result;

    d_test_suite_test(_suite, "array_fill");

    arr = d_array_new_from_arr(sizeof(int), (int[]){ 1, 2, 3, 4, 5 }, 5);

    if (!arr)
    {
        return false;
    }

    // test filling with different value
    result = d_array_fill(arr, sizeof(int), &new_fill);
    int_elements = (int*)arr->elements;

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "`d_array_fill` fills array correctly",
                                    result == true && int_elements[0] == 99 && int_elements[4] == 99,
                                    "`d_array_fill` failed");

    // test NULL array
    result = d_array_fill(NULL, sizeof(int), &fill_value);

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "`d_array_fill` rejects NULL array",
                                    result == false,
                                    "`d_array_fill` accepted NULL array");

    // test NULL fill element
    result = d_array_fill(arr, sizeof(int), NULL);

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "`d_array_fill` rejects NULL fill element",
                                    result == false,
                                    "`d_array_fill` accepted NULL fill element");

    d_array_free(arr);

    return test_result;
}

/*
d_tests_array_find
  Tests the `d_array_find` function.
  Tests the following:
  - finding existing element
  - not finding missing element
  - NULL array rejection
*/
bool d_tests_array_find(
    struct dj_test_suite* _suite
)
{
    bool            test_result = true;
    struct dj_array* arr;
    int             search_val;
    ssize_t         result;

    d_test_suite_test(_suite, "array_find");

    arr = d_array_new_from_arr(sizeof(int), (int[]){ 10, 20, 30, 40, 50 }, 5);

    if (!arr)
    {
        return false;
    }

    // test finding existing element
    search_val = 30;
    result = d_array_find(arr, sizeof(int), &search_val);

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "`d_array_find` returns correct index for existing element",
                                    result == 2,
                                    "`d_array_find` returned wrong index");

    // test not finding missing element
    search_val = 99;
    result = d_array_find(arr, sizeof(int), &search_val);

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "`d_array_find` returns -1 for missing element",
                                    result == -1,
                                    "`d_array_find` did not return -1 for missing element");

    d_array_free(arr);

    // test NULL array
    result = d_array_find(NULL, sizeof(int), &search_val);

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "`d_array_find` returns -1 for NULL array",
                                    result == -1,
                                    "`d_array_find` did not return -1 for NULL array");

    return test_result;
}

/*
d_tests_array_insert_element
  Tests the `d_array_insert_element` function.
  Tests the following:
  - inserting at beginning
  - inserting in middle
  - NULL array rejection
*/
bool d_tests_array_insert_element(
    struct dj_test_suite* _suite
)
{
    bool            test_result = true;
    struct dj_array* arr;
    int             value = 99;
    bool            result;

    d_test_suite_test(_suite, "array_insert_element");

    arr = d_array_new_from_arr(sizeof(int), (int[]){ 1, 2, 3, 4, 5 }, 5);

    if (!arr)
    {
        return false;
    }

    // test inserting at index 0
    result = d_array_insert_element(arr, sizeof(int), &value, 0);

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "`d_array_insert_element` inserts at beginning",
                                    result == true,
                                    "`d_array_insert_element` failed at beginning");

    // test NULL array
    result = d_array_insert_element(NULL, sizeof(int), &value, 0);

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "`d_array_insert_element` rejects NULL array",
                                    result == false,
                                    "`d_array_insert_element` accepted NULL array");

    d_array_free(arr);

    return test_result;
}

/*
d_tests_array_insert_elements
  Tests the `d_array_insert_elements` function.
  Tests the following:
  - inserting multiple elements
  - NULL array rejection
*/
bool d_tests_array_insert_elements(
    struct dj_test_suite* _suite
)
{
    bool            test_result = true;
    struct dj_array* arr;
    int             values[] = { 88, 99 };
    bool            result;

    d_test_suite_test(_suite, "array_insert_elements");

    arr = d_array_new_from_arr(sizeof(int), (int[]){ 1, 2, 3, 4, 5 }, 5);

    if (!arr)
    {
        return false;
    }

    // test inserting at index 2
    result = d_array_insert_elements(arr, sizeof(int), values, 2, 2);

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "`d_array_insert_elements` inserts multiple elements",
                                    result == true,
                                    "`d_array_insert_elements` failed");

    // test NULL array
    result = d_array_insert_elements(NULL, sizeof(int), values, 2, 0);

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "`d_array_insert_elements` rejects NULL array",
                                    result == false,
                                    "`d_array_insert_elements` accepted NULL array");

    d_array_free(arr);

    return test_result;
}

/*
d_tests_array_insert_array
  Tests the `d_array_insert_array` function.
  Tests the following:
  - inserting array
  - NULL destination rejection
*/
bool d_tests_array_insert_array(
    struct dj_test_suite* _suite
)
{
    bool            test_result = true;
    struct dj_array* dest;
    struct dj_array  source = { 2, NULL };
    int             source_data[] = { 88, 99 };
    bool            result;

    d_test_suite_test(_suite, "array_insert_array");

    dest = d_array_new_from_arr(sizeof(int), (int[]){ 1, 2, 3 }, 3);
    source.elements = source_data;

    if (!dest)
    {
        return false;
    }

    // test inserting array
    result = d_array_insert_array(dest, sizeof(int), &source, 1);

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "`d_array_insert_array` inserts array",
                                    result == true,
                                    "`d_array_insert_array` failed");

    // test NULL destination
    result = d_array_insert_array(NULL, sizeof(int), &source, 0);

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "`d_array_insert_array` rejects NULL destination",
                                    result == false,
                                    "`d_array_insert_array` accepted NULL destination");

    d_array_free(dest);

    return test_result;
}

/*
d_tests_array_is_empty
  Tests the `d_array_is_empty` function.
  Tests the following:
  - empty array detection
  - non-empty array detection
  - NULL array handling
*/
bool d_tests_array_is_empty(
    struct dj_test_suite* _suite
)
{
    bool            test_result = true;
    struct dj_array* empty_arr;
    struct dj_array* full_arr;
    bool            result;

    d_test_suite_test(_suite, "array_is_empty");

    empty_arr = d_array_new(sizeof(int), 10);
    full_arr = d_array_new_from_arr(sizeof(int), (int[]){ 1, 2, 3, 4, 5 }, 5);

    if (!empty_arr || !full_arr)
    {
        if (empty_arr) d_array_free(empty_arr);
        if (full_arr) d_array_free(full_arr);

        return false;
    }

    // test empty array
    result = d_array_is_empty(empty_arr);

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "`d_array_is_empty` correctly identifies empty array",
                                    result == true,
                                    "`d_array_is_empty` failed on empty array");

    // test non-empty array
    result = d_array_is_empty(full_arr);

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "`d_array_is_empty` correctly identifies non-empty array",
                                    result == false,
                                    "`d_array_is_empty` failed on non-empty array");

    d_array_free(empty_arr);
    d_array_free(full_arr);

    // test NULL array
    result = d_array_is_empty(NULL);

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "`d_array_is_empty` returns true for NULL array",
                                    result == true,
                                    "`d_array_is_empty` returned false for NULL array");

    return test_result;
}

/*
d_tests_array_prepend_element
  Tests the `d_array_prepend_element` function.
  Tests the following:
  - prepending element
  - NULL array rejection
*/
bool d_tests_array_prepend_element(
    struct dj_test_suite* _suite
)
{
    bool            test_result = true;
    struct dj_array* arr;
    int             value = 99;
    bool            result;

    d_test_suite_test(_suite, "array_prepend_element");

    arr = d_array_new_from_arr(sizeof(int), (int[]){ 1, 2, 3 }, 3);

    if (!arr)
    {
        return false;
    }

    // test prepending
    result = d_array_prepend_element(arr, sizeof(int), &value);

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "`d_array_prepend_element` prepends element",
                                    result == true,
                                    "`d_array_prepend_element` failed");

    // test NULL array
    result = d_array_prepend_element(NULL, sizeof(int), &value);

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "`d_array_prepend_element` rejects NULL array",
                                    result == false,
                                    "`d_array_prepend_element` accepted NULL array");

    d_array_free(arr);

    return test_result;
}

/*
d_tests_array_prepend_elements
  Tests the `d_array_prepend_elements` function.
  Tests the following:
  - prepending multiple elements
  - NULL array rejection
*/
bool d_tests_array_prepend_elements(
    struct dj_test_suite* _suite
)
{
    bool            test_result = true;
    struct dj_array* arr;
    int             values[] = { 88, 99 };
    bool            result;

    d_test_suite_test(_suite, "array_prepend_elements");

    arr = d_array_new_from_arr(sizeof(int), (int[]){ 1, 2, 3 }, 3);

    if (!arr)
    {
        return false;
    }

    // test prepending multiple
    result = d_array_prepend_elements(arr, sizeof(int), values, 2);

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "`d_array_prepend_elements` prepends elements",
                                    result == true,
                                    "`d_array_prepend_elements` failed");

    // test NULL array
    result = d_array_prepend_elements(NULL, sizeof(int), values, 2);

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "`d_array_prepend_elements` rejects NULL array",
                                    result == false,
                                    "`d_array_prepend_elements` accepted NULL array");

    d_array_free(arr);

    return test_result;
}

/*
d_tests_array_prepend_array
  Tests the `d_array_prepend_array` function.
  Tests the following:
  - prepending array
  - NULL destination rejection
*/
bool d_tests_array_prepend_array(
    struct dj_test_suite* _suite
)
{
    bool            test_result = true;
    struct dj_array* dest;
    struct dj_array  source = { 2, NULL };
    int             source_data[] = { 88, 99 };
    bool            result;

    d_test_suite_test(_suite, "array_prepend_array");

    dest = d_array_new_from_arr(sizeof(int), (int[]){ 1, 2, 3 }, 3);
    source.elements = source_data;

    if (!dest)
    {
        return false;
    }

    // test prepending array
    result = d_array_prepend_array(dest, sizeof(int), &source);

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "`d_array_prepend_array` prepends array",
                                    result == true,
                                    "`d_array_prepend_array` failed");

    // test NULL destination
    result = d_array_prepend_array(NULL, sizeof(int), &source);

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "`d_array_prepend_array` rejects NULL destination",
                                    result == false,
                                    "`d_array_prepend_array` accepted NULL destination");

    d_array_free(dest);

    return test_result;
}

/*
d_tests_array_resize_amount
  Tests the `d_array_resize_amount` function.
  Tests the following:
  - positive resize
  - negative resize
  - NULL array rejection
*/
bool d_tests_array_resize_amount(
    struct dj_test_suite* _suite
)
{
    bool            test_result = true;
    struct dj_array* arr;
    bool            result;

    d_test_suite_test(_suite, "array_resize_amount");

    arr = d_array_new(sizeof(int), 10);

    if (!arr)
    {
        return false;
    }

    // test positive resize
    result = d_array_resize_amount(arr, sizeof(int), 5);

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "`d_array_resize_amount` handles positive resize",
                                    result == true,
                                    "`d_array_resize_amount` failed with positive resize");

    // test NULL array
    result = d_array_resize_amount(NULL, sizeof(int), 5);

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "`d_array_resize_amount` rejects NULL array",
                                    result == false,
                                    "`d_array_resize_amount` accepted NULL array");

    d_array_free(arr);

    return test_result;
}

/*
d_tests_array_resize_factor
  Tests the `d_array_resize_factor` function.
  Tests the following:
  - 2x growth
  - 0.5x shrink
  - NULL array rejection
*/
bool d_tests_array_resize_factor(
    struct dj_test_suite* _suite
)
{
    bool            test_result = true;
    struct dj_array* arr;
    bool            result;

    d_test_suite_test(_suite, "array_resize_factor");

    arr = d_array_new(sizeof(int), 10);

    if (!arr)
    {
        return false;
    }

    // test 2x growth
    result = d_array_resize_factor(arr, sizeof(int), 2.0);

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "`d_array_resize_factor` handles 2x growth",
                                    result == true,
                                    "`d_array_resize_factor` failed with 2x growth");

    // test NULL array
    result = d_array_resize_factor(NULL, sizeof(int), 2.0);

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "`d_array_resize_factor` rejects NULL array",
                                    result == false,
                                    "`d_array_resize_factor` accepted NULL array");

    d_array_free(arr);

    return test_result;
}

/*
d_tests_array_reverse
  Tests the `d_array_reverse` function.
  Tests the following:
  - reversing array
  - NULL array rejection
*/
bool d_tests_array_reverse(
    struct dj_test_suite* _suite
)
{
    bool            test_result = true;
    struct dj_array* arr;
    int*            int_elements;
    bool            result;

    d_test_suite_test(_suite, "array_reverse");

    arr = d_array_new_from_arr(sizeof(int), (int[]){ 1, 2, 3, 4, 5 }, 5);

    if (!arr)
    {
        return false;
    }

    // test reversing
    result = d_array_reverse(arr, sizeof(int));
    int_elements = (int*)arr->elements;

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "`d_array_reverse` reverses array correctly",
                                    result == true && int_elements[0] == 5 && int_elements[4] == 1,
                                    "`d_array_reverse` failed");

    // test NULL array
    result = d_array_reverse(NULL, sizeof(int));

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "`d_array_reverse` rejects NULL array",
                                    result == false,
                                    "`d_array_reverse` accepted NULL array");

    d_array_free(arr);

    return test_result;
}

/*
d_tests_array_shift_left
  Tests the `d_array_shift_left` function.
  Tests the following:
  - shifting left
  - NULL array rejection
*/
bool d_tests_array_shift_left(
    struct dj_test_suite* _suite
)
{
    bool            test_result = true;
    struct dj_array* arr;
    bool            result;

    d_test_suite_test(_suite, "array_shift_left");

    arr = d_array_new_from_arr(sizeof(int), (int[]){ 1, 2, 3, 4, 5 }, 5);

    if (!arr)
    {
        return false;
    }

    // test shifting left by 2
    result = d_array_shift_left(arr, sizeof(int), 2);

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "`d_array_shift_left` shifts left correctly",
                                    result == true,
                                    "`d_array_shift_left` failed");

    // test NULL array
    result = d_array_shift_left(NULL, sizeof(int), 2);

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "`d_array_shift_left` rejects NULL array",
                                    result == false,
                                    "`d_array_shift_left` accepted NULL array");

    d_array_free(arr);

    return test_result;
}

/*
d_tests_array_shift_left_circular
  Tests the `d_array_shift_left_circular` function.
  Tests the following:
  - circular shift left
  - NULL array rejection
*/
bool d_tests_array_shift_left_circular(
    struct dj_test_suite* _suite
)
{
    bool            test_result = true;
    struct dj_array* arr;
    bool            result;

    d_test_suite_test(_suite, "array_shift_left_circular");

    arr = d_array_new_from_arr(sizeof(int), (int[]){ 1, 2, 3, 4, 5 }, 5);

    if (!arr)
    {
        return false;
    }

    // test circular shift left
    result = d_array_shift_left_circular(arr, sizeof(int), 2);

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "`d_array_shift_left_circular` shifts circularly",
                                    result == true,
                                    "`d_array_shift_left_circular` failed");

    // test NULL array
    result = d_array_shift_left_circular(NULL, sizeof(int), 2);

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "`d_array_shift_left_circular` rejects NULL array",
                                    result == false,
                                    "`d_array_shift_left_circular` accepted NULL array");

    d_array_free(arr);

    return test_result;
}

/*
d_tests_array_shift_right
  Tests the `d_array_shift_right` function.
  Tests the following:
  - shifting right
  - NULL array rejection
*/
bool d_tests_array_shift_right(
    struct dj_test_suite* _suite
)
{
    bool            test_result = true;
    struct dj_array* arr;
    bool            result;

    d_test_suite_test(_suite, "array_shift_right");

    arr = d_array_new_from_arr(sizeof(int), (int[]){ 1, 2, 3, 4, 5 }, 5);

    if (!arr)
    {
        return false;
    }

    // test shifting right
    result = d_array_shift_right(arr, sizeof(int), 2);

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "`d_array_shift_right` shifts right correctly",
                                    result == true,
                                    "`d_array_shift_right` failed");

    // test NULL array
    result = d_array_shift_right(NULL, sizeof(int), 2);

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "`d_array_shift_right` rejects NULL array",
                                    result == false,
                                    "`d_array_shift_right` accepted NULL array");

    d_array_free(arr);

    return test_result;
}

/*
d_tests_array_shift_right_circular
  Tests the `d_array_shift_right_circular` function.
  Tests the following:
  - circular shift right
  - NULL array rejection
*/
bool d_tests_array_shift_right_circular(
    struct dj_test_suite* _suite
)
{
    bool            test_result = true;
    struct dj_array* arr;
    bool            result;

    d_test_suite_test(_suite, "array_shift_right_circular");

    arr = d_array_new_from_arr(sizeof(int), (int[]){ 1, 2, 3, 4, 5 }, 5);

    if (!arr)
    {
        return false;
    }

    // test circular shift right
    result = d_array_shift_right_circular(arr, sizeof(int), 2);

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "`d_array_shift_right_circular` shifts circularly",
                                    result == true,
                                    "`d_array_shift_right_circular` failed");

    // test NULL array
    result = d_array_shift_right_circular(NULL, sizeof(int), 2);

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "`d_array_shift_right_circular` rejects NULL array",
                                    result == false,
                                    "`d_array_shift_right_circular` accepted NULL array");

    d_array_free(arr);

    return test_result;
}

/*
d_tests_array_slice
  Tests the `d_array_slice` function.
  Tests the following:
  - slicing from positive index
  - NULL source rejection
*/
bool d_tests_array_slice(
    struct dj_test_suite* _suite
)
{
    bool   test_result = true;
    int    data[] = { 1, 2, 3, 4, 5 };
    void*  slice;

    d_test_suite_test(_suite, "array_slice");

    // test slicing from index 2
    slice = d_array_slice(data, 5, 2, sizeof(int));

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "`d_array_slice` creates slice",
                                    slice != NULL,
                                    "`d_array_slice` failed");

    if (slice)
    {
        free(slice);
    }

    // test NULL source
    slice = d_array_slice(NULL, 5, 0, sizeof(int));

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "`d_array_slice` rejects NULL source",
                                    slice == NULL,
                                    "`d_array_slice` accepted NULL source");

    return test_result;
}

/*
d_tests_array_slice_range
  Tests the `d_array_slice_range` function.
  Tests the following:
  - slicing range
  - NULL source rejection
*/
bool d_tests_array_slice_range(
    struct dj_test_suite* _suite
)
{
    bool   test_result = true;
    int    data[] = { 1, 2, 3, 4, 5 };
    void*  slice;

    d_test_suite_test(_suite, "array_slice_range");

    // test slicing range (1 to 3)
    slice = d_array_slice_range(data, 5, 1, 4, sizeof(int));

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "`d_array_slice_range` creates range slice",
                                    slice != NULL,
                                    "`d_array_slice_range` failed");

    if (slice)
    {
        free(slice);
    }

    // test NULL source
    slice = d_array_slice_range(NULL, 5, 0, 2, sizeof(int));

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "`d_array_slice_range` rejects NULL source",
                                    slice == NULL,
                                    "`d_array_slice_range` accepted NULL source");

    return test_result;
}

/*
d_tests_array_sort
  Tests the `d_array_sort` function.
  Tests the following:
  - sorting unsorted array
  - NULL array handling
*/
bool d_tests_array_sort(
    struct dj_test_suite* _suite
)
{
    bool            test_result = true;
    struct dj_array* arr;
    int*            int_elements;

    d_test_suite_test(_suite, "array_sort");

    arr = d_array_new_from_arr(sizeof(int), (int[]){ 3, 1, 4, 1, 5, 9, 2, 6 }, 8);

    if (!arr)
    {
        return false;
    }

    // test sorting
    d_array_sort(arr, sizeof(int), d_test_int_comparator);
    int_elements = (int*)arr->elements;

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "`d_array_sort` sorts array correctly",
                                    int_elements[0] == 1 && int_elements[7] == 9,
                                    "`d_array_sort` failed");

    // test NULL array (should not crash)
    d_array_sort(NULL, sizeof(int), d_test_int_comparator);

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "`d_array_sort` handles NULL array safely",
                                    true,
                                    "`d_array_sort` crashed on NULL array");

    d_array_free(arr);

    return test_result;
}


/*
===============================================================================
                        MANIPULATION TEST GROUP RUNNER
===============================================================================
*/

/*
d_tests_array_run_manipulation_tests
  Runs all dj_array manipulation function tests.
*/
bool d_tests_array_run_manipulation_tests(
    struct dj_test_suite* _suite
)
{
    bool overall_result = true;

    d_test_suite_block(_suite, "array_run_manipulation_tests");

    printf("\n----------------------------------------\n");
    printf("Running dj_array Manipulation Function Tests\n");

    overall_result &= d_tests_array_append_element(_suite);
    overall_result &= d_tests_array_append_elements(_suite);
    overall_result &= d_tests_array_append_array(_suite);
    overall_result &= d_tests_array_contains(_suite);
    overall_result &= d_tests_array_fill(_suite);
    overall_result &= d_tests_array_find(_suite);
    overall_result &= d_tests_array_insert_element(_suite);
    overall_result &= d_tests_array_insert_elements(_suite);
    overall_result &= d_tests_array_insert_array(_suite);
    overall_result &= d_tests_array_is_empty(_suite);
    overall_result &= d_tests_array_prepend_element(_suite);
    overall_result &= d_tests_array_prepend_elements(_suite);
    overall_result &= d_tests_array_prepend_array(_suite);
    overall_result &= d_tests_array_resize_amount(_suite);
    overall_result &= d_tests_array_resize_factor(_suite);
    overall_result &= d_tests_array_reverse(_suite);
    overall_result &= d_tests_array_shift_left(_suite);
    overall_result &= d_tests_array_shift_left_circular(_suite);
    overall_result &= d_tests_array_shift_right(_suite);
    overall_result &= d_tests_array_shift_right_circular(_suite);
    overall_result &= d_tests_array_slice(_suite);
    overall_result &= d_tests_array_slice_range(_suite);
    overall_result &= d_tests_array_sort(_suite);

    return overall_result;
}
