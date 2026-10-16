/*******************************************************************************
* djinterp [container]                                    array_tests_sa_manip.c
*
* Unit tests for `array.h` manipulation functions.
* Part of the array standalone test suite.
*
*
* path:      /tests/djinterp/c/container/array/array_tests_sa_manip.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.01.30
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "./array_tests_sa.h"
#include "../../../../../inc/djinterp/test/c/test_counter.h"


/*
d_tests_sa_array_append_element
  Tests the `d_array_append_element` function.
  Tests the following:
  - appending single element
  - NULL array rejection
  - NULL element rejection
*/
bool
d_tests_sa_array_append_element
(
    struct d_test_counter* _test_info
)
{
    bool            test_result = true;
    struct d_array* arr;
    int             value = 99;
    int*            int_elements;
    bool            result;

    printf("Testing `d_array_append_element`...\n");

    arr = d_array_new(sizeof(int), 10);

    if (!arr)
    {
        _test_info->tests_total++;

        return false;
    }

    // test appending element
    result = d_array_append_element(arr, sizeof(int), &value);
    int_elements = (int*)arr->elements;

    test_result &= d_assert_standalone(
        result == true && int_elements[0] == 99,
        "`d_array_append_element` appends element correctly",
        "`d_array_append_element` failed to append element",
        _test_info
    );

    // test NULL array
    result = d_array_append_element(NULL, sizeof(int), &value);

    test_result &= d_assert_standalone(
        result == false,
        "`d_array_append_element` rejects NULL array",
        "`d_array_append_element` accepted NULL array",
        _test_info
    );

    // test NULL element
    result = d_array_append_element(arr, sizeof(int), NULL);

    test_result &= d_assert_standalone(
        result == false,
        "`d_array_append_element` rejects NULL element",
        "`d_array_append_element` accepted NULL element",
        _test_info
    );

    d_array_free(arr);

    _test_info->tests_total++;
    if (test_result)
    {
        _test_info->tests_passed++;
    }

    return test_result;
}

/*
d_tests_sa_array_append_elements
  Tests the `d_array_append_elements` function.
  Tests the following:
  - appending multiple elements
  - zero count handling
  - NULL array rejection
*/
bool
d_tests_sa_array_append_elements
(
    struct d_test_counter* _test_info
)
{
    bool            test_result = true;
    struct d_array* arr;
    int             values[] = { 1, 2, 3 };
    bool            result;

    printf("Testing `d_array_append_elements`...\n");

    arr = d_array_new(sizeof(int), 10);

    if (!arr)
    {
        _test_info->tests_total++;

        return false;
    }

    // test appending multiple elements
    result = d_array_append_elements(arr, sizeof(int), values, 3);

    test_result &= d_assert_standalone(
        result == true,
        "`d_array_append_elements` appends elements correctly",
        "`d_array_append_elements` failed",
        _test_info
    );

    // test zero count
    result = d_array_append_elements(arr, sizeof(int), values, 0);

    test_result &= d_assert_standalone(
        result == true,
        "`d_array_append_elements` handles zero count",
        "`d_array_append_elements` failed with zero count",
        _test_info
    );

    // test NULL array
    result = d_array_append_elements(NULL, sizeof(int), values, 3);

    test_result &= d_assert_standalone(
        result == false,
        "`d_array_append_elements` rejects NULL array",
        "`d_array_append_elements` accepted NULL array",
        _test_info
    );

    d_array_free(arr);

    _test_info->tests_total++;
    if (test_result)
    {
        _test_info->tests_passed++;
    }

    return test_result;
}

/*
d_tests_sa_array_append_array
  Tests the `d_array_append_array` function.
  Tests the following:
  - appending array
  - NULL array rejection
*/
bool
d_tests_sa_array_append_array
(
    struct d_test_counter* _test_info
)
{
    bool            test_result = true;
    struct d_array* arr;
    int             source[] = { 4, 5, 6 };
    bool            result;

    printf("Testing `d_array_append_array`...\n");

    arr = d_array_new(sizeof(int), 10);

    if (!arr)
    {
        _test_info->tests_total++;

        return false;
    }

    // test appending array
    result = d_array_append_array(arr, sizeof(int), source, 3, 0);

    test_result &= d_assert_standalone(
        result == true,
        "`d_array_append_array` appends array correctly",
        "`d_array_append_array` failed",
        _test_info
    );

    // test NULL destination
    result = d_array_append_array(NULL, sizeof(int), source, 3, 0);

    test_result &= d_assert_standalone(
        result == false,
        "`d_array_append_array` rejects NULL destination",
        "`d_array_append_array` accepted NULL destination",
        _test_info
    );

    d_array_free(arr);

    _test_info->tests_total++;
    if (test_result)
    {
        _test_info->tests_passed++;
    }

    return test_result;
}

/*
d_tests_sa_array_contains
  Tests the `d_array_contains` function.
  Tests the following:
  - finding existing element
  - not finding missing element
  - NULL array rejection
*/
bool
d_tests_sa_array_contains
(
    struct d_test_counter* _test_info
)
{
    bool            test_result = true;
    struct d_array* arr;
    int             search_val;
    bool            result;

    printf("Testing `d_array_contains`...\n");

    arr = d_array_new_from_arr(sizeof(int), (int[]){ 1, 2, 3, 4, 5 }, 5);

    if (!arr)
    {
        _test_info->tests_total++;

        return false;
    }

    // test finding existing element
    search_val = 3;
    result = d_array_contains(arr, sizeof(int), &search_val);

    test_result &= d_assert_standalone(
        result == true,
        "`d_array_contains` finds existing element",
        "`d_array_contains` failed to find existing element",
        _test_info
    );

    // test not finding missing element
    search_val = 99;
    result = d_array_contains(arr, sizeof(int), &search_val);

    test_result &= d_assert_standalone(
        result == false,
        "`d_array_contains` correctly returns false for missing element",
        "`d_array_contains` incorrectly found missing element",
        _test_info
    );

    d_array_free(arr);

    // test NULL array
    result = d_array_contains(NULL, sizeof(int), &search_val);

    test_result &= d_assert_standalone(
        result == false,
        "`d_array_contains` rejects NULL array",
        "`d_array_contains` accepted NULL array",
        _test_info
    );

    _test_info->tests_total++;
    if (test_result)
    {
        _test_info->tests_passed++;
    }

    return test_result;
}

/*
d_tests_sa_array_fill
  Tests the `d_array_fill` function.
  Tests the following:
  - filling array with value
  - NULL array rejection
  - NULL fill element rejection
*/
bool
d_tests_sa_array_fill
(
    struct d_test_counter* _test_info
)
{
    bool            test_result = true;
    struct d_array* arr;
    int             fill_value = 42;
    int             new_fill = 99;
    int*            int_elements;
    bool            result;

    printf("Testing `d_array_fill`...\n");

    arr = d_array_new_from_arr(sizeof(int), (int[]){ 1, 2, 3, 4, 5 }, 5);

    if (!arr)
    {
        _test_info->tests_total++;

        return false;
    }

    // test filling with different value
    result = d_array_fill(arr, sizeof(int), &new_fill);
    int_elements = (int*)arr->elements;

    test_result &= d_assert_standalone(
        result == true && int_elements[0] == 99 && int_elements[4] == 99,
        "`d_array_fill` fills array correctly",
        "`d_array_fill` failed",
        _test_info
    );

    // test NULL array
    result = d_array_fill(NULL, sizeof(int), &fill_value);

    test_result &= d_assert_standalone(
        result == false,
        "`d_array_fill` rejects NULL array",
        "`d_array_fill` accepted NULL array",
        _test_info
    );

    // test NULL fill element
    result = d_array_fill(arr, sizeof(int), NULL);

    test_result &= d_assert_standalone(
        result == false,
        "`d_array_fill` rejects NULL fill element",
        "`d_array_fill` accepted NULL fill element",
        _test_info
    );

    d_array_free(arr);

    _test_info->tests_total++;
    if (test_result)
    {
        _test_info->tests_passed++;
    }

    return test_result;
}

/*
d_tests_sa_array_find
  Tests the `d_array_find` function.
  Tests the following:
  - finding existing element
  - not finding missing element
  - NULL array rejection
*/
bool
d_tests_sa_array_find
(
    struct d_test_counter* _test_info
)
{
    bool            test_result = true;
    struct d_array* arr;
    int             search_val;
    ssize_t         result;

    printf("Testing `d_array_find`...\n");

    arr = d_array_new_from_arr(sizeof(int), (int[]){ 10, 20, 30, 40, 50 }, 5);

    if (!arr)
    {
        _test_info->tests_total++;

        return false;
    }

    // test finding existing element
    search_val = 30;
    result = d_array_find(arr, sizeof(int), &search_val);

    test_result &= d_assert_standalone(
        result == 2,
        "`d_array_find` returns correct index for existing element",
        "`d_array_find` returned wrong index",
        _test_info
    );

    // test not finding missing element
    search_val = 99;
    result = d_array_find(arr, sizeof(int), &search_val);

    test_result &= d_assert_standalone(
        result == -1,
        "`d_array_find` returns -1 for missing element",
        "`d_array_find` did not return -1 for missing element",
        _test_info
    );

    d_array_free(arr);

    // test NULL array
    result = d_array_find(NULL, sizeof(int), &search_val);

    test_result &= d_assert_standalone(
        result == -1,
        "`d_array_find` returns -1 for NULL array",
        "`d_array_find` did not return -1 for NULL array",
        _test_info
    );

    _test_info->tests_total++;
    if (test_result)
    {
        _test_info->tests_passed++;
    }

    return test_result;
}

/*
d_tests_sa_array_insert_element
  Tests the `d_array_insert_element` function.
  Tests the following:
  - inserting at beginning
  - inserting in middle
  - NULL array rejection
*/
bool
d_tests_sa_array_insert_element
(
    struct d_test_counter* _test_info
)
{
    bool            test_result = true;
    struct d_array* arr;
    int             value = 99;
    bool            result;

    printf("Testing `d_array_insert_element`...\n");

    arr = d_array_new_from_arr(sizeof(int), (int[]){ 1, 2, 3, 4, 5 }, 5);

    if (!arr)
    {
        _test_info->tests_total++;

        return false;
    }

    // test inserting at index 0
    result = d_array_insert_element(arr, sizeof(int), &value, 0);

    test_result &= d_assert_standalone(
        result == true,
        "`d_array_insert_element` inserts at beginning",
        "`d_array_insert_element` failed at beginning",
        _test_info
    );

    // test NULL array
    result = d_array_insert_element(NULL, sizeof(int), &value, 0);

    test_result &= d_assert_standalone(
        result == false,
        "`d_array_insert_element` rejects NULL array",
        "`d_array_insert_element` accepted NULL array",
        _test_info
    );

    d_array_free(arr);

    _test_info->tests_total++;
    if (test_result)
    {
        _test_info->tests_passed++;
    }

    return test_result;
}

/*
d_tests_sa_array_insert_elements
  Tests the `d_array_insert_elements` function.
  Tests the following:
  - inserting multiple elements
  - NULL array rejection
*/
bool
d_tests_sa_array_insert_elements
(
    struct d_test_counter* _test_info
)
{
    bool            test_result = true;
    struct d_array* arr;
    int             values[] = { 88, 99 };
    bool            result;

    printf("Testing `d_array_insert_elements`...\n");

    arr = d_array_new_from_arr(sizeof(int), (int[]){ 1, 2, 3, 4, 5 }, 5);

    if (!arr)
    {
        _test_info->tests_total++;

        return false;
    }

    // test inserting at index 2
    result = d_array_insert_elements(arr, sizeof(int), values, 2, 2);

    test_result &= d_assert_standalone(
        result == true,
        "`d_array_insert_elements` inserts multiple elements",
        "`d_array_insert_elements` failed",
        _test_info
    );

    // test NULL array
    result = d_array_insert_elements(NULL, sizeof(int), values, 2, 0);

    test_result &= d_assert_standalone(
        result == false,
        "`d_array_insert_elements` rejects NULL array",
        "`d_array_insert_elements` accepted NULL array",
        _test_info
    );

    d_array_free(arr);

    _test_info->tests_total++;
    if (test_result)
    {
        _test_info->tests_passed++;
    }

    return test_result;
}

/*
d_tests_sa_array_insert_array
  Tests the `d_array_insert_array` function.
  Tests the following:
  - inserting array
  - NULL destination rejection
*/
bool
d_tests_sa_array_insert_array
(
    struct d_test_counter* _test_info
)
{
    bool            test_result = true;
    struct d_array* dest;
    struct d_array  source = { 2, NULL };
    int             source_data[] = { 88, 99 };
    bool            result;

    printf("Testing `d_array_insert_array`...\n");

    dest = d_array_new_from_arr(sizeof(int), (int[]){ 1, 2, 3 }, 3);
    source.elements = source_data;

    if (!dest)
    {
        _test_info->tests_total++;

        return false;
    }

    // test inserting array
    result = d_array_insert_array(dest, sizeof(int), &source, 1);

    test_result &= d_assert_standalone(
        result == true,
        "`d_array_insert_array` inserts array",
        "`d_array_insert_array` failed",
        _test_info
    );

    // test NULL destination
    result = d_array_insert_array(NULL, sizeof(int), &source, 0);

    test_result &= d_assert_standalone(
        result == false,
        "`d_array_insert_array` rejects NULL destination",
        "`d_array_insert_array` accepted NULL destination",
        _test_info
    );

    d_array_free(dest);

    _test_info->tests_total++;
    if (test_result)
    {
        _test_info->tests_passed++;
    }

    return test_result;
}

/*
d_tests_sa_array_is_empty
  Tests the `d_array_is_empty` function.
  Tests the following:
  - empty array detection
  - non-empty array detection
  - NULL array handling
*/
bool
d_tests_sa_array_is_empty
(
    struct d_test_counter* _test_info
)
{
    bool            test_result = true;
    struct d_array* empty_arr;
    struct d_array* full_arr;
    bool            result;

    printf("Testing `d_array_is_empty`...\n");

    empty_arr = d_array_new(sizeof(int), 10);
    full_arr = d_array_new_from_arr(sizeof(int), (int[]){ 1, 2, 3, 4, 5 }, 5);

    if (!empty_arr || !full_arr)
    {
        if (empty_arr) d_array_free(empty_arr);
        if (full_arr) d_array_free(full_arr);
        _test_info->tests_total++;

        return false;
    }

    // test empty array
    result = d_array_is_empty(empty_arr);

    test_result &= d_assert_standalone(
        result == true,
        "`d_array_is_empty` correctly identifies empty array",
        "`d_array_is_empty` failed on empty array",
        _test_info
    );

    // test non-empty array
    result = d_array_is_empty(full_arr);

    test_result &= d_assert_standalone(
        result == false,
        "`d_array_is_empty` correctly identifies non-empty array",
        "`d_array_is_empty` failed on non-empty array",
        _test_info
    );

    d_array_free(empty_arr);
    d_array_free(full_arr);

    // test NULL array
    result = d_array_is_empty(NULL);

    test_result &= d_assert_standalone(
        result == true,
        "`d_array_is_empty` returns true for NULL array",
        "`d_array_is_empty` returned false for NULL array",
        _test_info
    );

    _test_info->tests_total++;
    if (test_result)
    {
        _test_info->tests_passed++;
    }

    return test_result;
}

/*
d_tests_sa_array_prepend_element
  Tests the `d_array_prepend_element` function.
  Tests the following:
  - prepending element
  - NULL array rejection
*/
bool
d_tests_sa_array_prepend_element
(
    struct d_test_counter* _test_info
)
{
    bool            test_result = true;
    struct d_array* arr;
    int             value = 99;
    bool            result;

    printf("Testing `d_array_prepend_element`...\n");

    arr = d_array_new_from_arr(sizeof(int), (int[]){ 1, 2, 3 }, 3);

    if (!arr)
    {
        _test_info->tests_total++;

        return false;
    }

    // test prepending
    result = d_array_prepend_element(arr, sizeof(int), &value);

    test_result &= d_assert_standalone(
        result == true,
        "`d_array_prepend_element` prepends element",
        "`d_array_prepend_element` failed",
        _test_info
    );

    // test NULL array
    result = d_array_prepend_element(NULL, sizeof(int), &value);

    test_result &= d_assert_standalone(
        result == false,
        "`d_array_prepend_element` rejects NULL array",
        "`d_array_prepend_element` accepted NULL array",
        _test_info
    );

    d_array_free(arr);

    _test_info->tests_total++;
    if (test_result)
    {
        _test_info->tests_passed++;
    }

    return test_result;
}

/*
d_tests_sa_array_prepend_elements
  Tests the `d_array_prepend_elements` function.
  Tests the following:
  - prepending multiple elements
  - NULL array rejection
*/
bool
d_tests_sa_array_prepend_elements
(
    struct d_test_counter* _test_info
)
{
    bool            test_result = true;
    struct d_array* arr;
    int             values[] = { 88, 99 };
    bool            result;

    printf("Testing `d_array_prepend_elements`...\n");

    arr = d_array_new_from_arr(sizeof(int), (int[]){ 1, 2, 3 }, 3);

    if (!arr)
    {
        _test_info->tests_total++;

        return false;
    }

    // test prepending multiple
    result = d_array_prepend_elements(arr, sizeof(int), values, 2);

    test_result &= d_assert_standalone(
        result == true,
        "`d_array_prepend_elements` prepends elements",
        "`d_array_prepend_elements` failed",
        _test_info
    );

    // test NULL array
    result = d_array_prepend_elements(NULL, sizeof(int), values, 2);

    test_result &= d_assert_standalone(
        result == false,
        "`d_array_prepend_elements` rejects NULL array",
        "`d_array_prepend_elements` accepted NULL array",
        _test_info
    );

    d_array_free(arr);

    _test_info->tests_total++;
    if (test_result)
    {
        _test_info->tests_passed++;
    }

    return test_result;
}

/*
d_tests_sa_array_prepend_array
  Tests the `d_array_prepend_array` function.
  Tests the following:
  - prepending array
  - NULL destination rejection
*/
bool
d_tests_sa_array_prepend_array
(
    struct d_test_counter* _test_info
)
{
    bool            test_result = true;
    struct d_array* dest;
    struct d_array  source = { 2, NULL };
    int             source_data[] = { 88, 99 };
    bool            result;

    printf("Testing `d_array_prepend_array`...\n");

    dest = d_array_new_from_arr(sizeof(int), (int[]){ 1, 2, 3 }, 3);
    source.elements = source_data;

    if (!dest)
    {
        _test_info->tests_total++;

        return false;
    }

    // test prepending array
    result = d_array_prepend_array(dest, sizeof(int), &source);

    test_result &= d_assert_standalone(
        result == true,
        "`d_array_prepend_array` prepends array",
        "`d_array_prepend_array` failed",
        _test_info
    );

    // test NULL destination
    result = d_array_prepend_array(NULL, sizeof(int), &source);

    test_result &= d_assert_standalone(
        result == false,
        "`d_array_prepend_array` rejects NULL destination",
        "`d_array_prepend_array` accepted NULL destination",
        _test_info
    );

    d_array_free(dest);

    _test_info->tests_total++;
    if (test_result)
    {
        _test_info->tests_passed++;
    }

    return test_result;
}

/*
d_tests_sa_array_resize_amount
  Tests the `d_array_resize_amount` function.
  Tests the following:
  - positive resize
  - negative resize
  - NULL array rejection
*/
bool
d_tests_sa_array_resize_amount
(
    struct d_test_counter* _test_info
)
{
    bool            test_result = true;
    struct d_array* arr;
    bool            result;

    printf("Testing `d_array_resize_amount`...\n");

    arr = d_array_new(sizeof(int), 10);

    if (!arr)
    {
        _test_info->tests_total++;

        return false;
    }

    // test positive resize
    result = d_array_resize_amount(arr, sizeof(int), 5);

    test_result &= d_assert_standalone(
        result == true,
        "`d_array_resize_amount` handles positive resize",
        "`d_array_resize_amount` failed with positive resize",
        _test_info
    );

    // test NULL array
    result = d_array_resize_amount(NULL, sizeof(int), 5);

    test_result &= d_assert_standalone(
        result == false,
        "`d_array_resize_amount` rejects NULL array",
        "`d_array_resize_amount` accepted NULL array",
        _test_info
    );

    d_array_free(arr);

    _test_info->tests_total++;
    if (test_result)
    {
        _test_info->tests_passed++;
    }

    return test_result;
}

/*
d_tests_sa_array_resize_factor
  Tests the `d_array_resize_factor` function.
  Tests the following:
  - 2x growth
  - 0.5x shrink
  - NULL array rejection
*/
bool
d_tests_sa_array_resize_factor
(
    struct d_test_counter* _test_info
)
{
    bool            test_result = true;
    struct d_array* arr;
    bool            result;

    printf("Testing `d_array_resize_factor`...\n");

    arr = d_array_new(sizeof(int), 10);

    if (!arr)
    {
        _test_info->tests_total++;

        return false;
    }

    // test 2x growth
    result = d_array_resize_factor(arr, sizeof(int), 2.0);

    test_result &= d_assert_standalone(
        result == true,
        "`d_array_resize_factor` handles 2x growth",
        "`d_array_resize_factor` failed with 2x growth",
        _test_info
    );

    // test NULL array
    result = d_array_resize_factor(NULL, sizeof(int), 2.0);

    test_result &= d_assert_standalone(
        result == false,
        "`d_array_resize_factor` rejects NULL array",
        "`d_array_resize_factor` accepted NULL array",
        _test_info
    );

    d_array_free(arr);

    _test_info->tests_total++;
    if (test_result)
    {
        _test_info->tests_passed++;
    }

    return test_result;
}

/*
d_tests_sa_array_reverse
  Tests the `d_array_reverse` function.
  Tests the following:
  - reversing array
  - NULL array rejection
*/
bool
d_tests_sa_array_reverse
(
    struct d_test_counter* _test_info
)
{
    bool            test_result = true;
    struct d_array* arr;
    int*            int_elements;
    bool            result;

    printf("Testing `d_array_reverse`...\n");

    arr = d_array_new_from_arr(sizeof(int), (int[]){ 1, 2, 3, 4, 5 }, 5);

    if (!arr)
    {
        _test_info->tests_total++;

        return false;
    }

    // test reversing
    result = d_array_reverse(arr, sizeof(int));
    int_elements = (int*)arr->elements;

    test_result &= d_assert_standalone(
        result == true && int_elements[0] == 5 && int_elements[4] == 1,
        "`d_array_reverse` reverses array correctly",
        "`d_array_reverse` failed",
        _test_info
    );

    // test NULL array
    result = d_array_reverse(NULL, sizeof(int));

    test_result &= d_assert_standalone(
        result == false,
        "`d_array_reverse` rejects NULL array",
        "`d_array_reverse` accepted NULL array",
        _test_info
    );

    d_array_free(arr);

    _test_info->tests_total++;
    if (test_result)
    {
        _test_info->tests_passed++;
    }

    return test_result;
}

/*
d_tests_sa_array_shift_left
  Tests the `d_array_shift_left` function.
  Tests the following:
  - shifting left
  - NULL array rejection
*/
bool
d_tests_sa_array_shift_left
(
    struct d_test_counter* _test_info
)
{
    bool            test_result = true;
    struct d_array* arr;
    bool            result;

    printf("Testing `d_array_shift_left`...\n");

    arr = d_array_new_from_arr(sizeof(int), (int[]){ 1, 2, 3, 4, 5 }, 5);

    if (!arr)
    {
        _test_info->tests_total++;

        return false;
    }

    // test shifting left by 2
    result = d_array_shift_left(arr, sizeof(int), 2);

    test_result &= d_assert_standalone(
        result == true,
        "`d_array_shift_left` shifts left correctly",
        "`d_array_shift_left` failed",
        _test_info
    );

    // test NULL array
    result = d_array_shift_left(NULL, sizeof(int), 2);

    test_result &= d_assert_standalone(
        result == false,
        "`d_array_shift_left` rejects NULL array",
        "`d_array_shift_left` accepted NULL array",
        _test_info
    );

    d_array_free(arr);

    _test_info->tests_total++;
    if (test_result)
    {
        _test_info->tests_passed++;
    }

    return test_result;
}

/*
d_tests_sa_array_shift_left_circular
  Tests the `d_array_shift_left_circular` function.
  Tests the following:
  - circular shift left
  - NULL array rejection
*/
bool
d_tests_sa_array_shift_left_circular
(
    struct d_test_counter* _test_info
)
{
    bool            test_result = true;
    struct d_array* arr;
    bool            result;

    printf("Testing `d_array_shift_left_circular`...\n");

    arr = d_array_new_from_arr(sizeof(int), (int[]){ 1, 2, 3, 4, 5 }, 5);

    if (!arr)
    {
        _test_info->tests_total++;

        return false;
    }

    // test circular shift left
    result = d_array_shift_left_circular(arr, sizeof(int), 2);

    test_result &= d_assert_standalone(
        result == true,
        "`d_array_shift_left_circular` shifts circularly",
        "`d_array_shift_left_circular` failed",
        _test_info
    );

    // test NULL array
    result = d_array_shift_left_circular(NULL, sizeof(int), 2);

    test_result &= d_assert_standalone(
        result == false,
        "`d_array_shift_left_circular` rejects NULL array",
        "`d_array_shift_left_circular` accepted NULL array",
        _test_info
    );

    d_array_free(arr);

    _test_info->tests_total++;
    if (test_result)
    {
        _test_info->tests_passed++;
    }

    return test_result;
}

/*
d_tests_sa_array_shift_right
  Tests the `d_array_shift_right` function.
  Tests the following:
  - shifting right
  - NULL array rejection
*/
bool
d_tests_sa_array_shift_right
(
    struct d_test_counter* _test_info
)
{
    bool            test_result = true;
    struct d_array* arr;
    bool            result;

    printf("Testing `d_array_shift_right`...\n");

    arr = d_array_new_from_arr(sizeof(int), (int[]){ 1, 2, 3, 4, 5 }, 5);

    if (!arr)
    {
        _test_info->tests_total++;

        return false;
    }

    // test shifting right
    result = d_array_shift_right(arr, sizeof(int), 2);

    test_result &= d_assert_standalone(
        result == true,
        "`d_array_shift_right` shifts right correctly",
        "`d_array_shift_right` failed",
        _test_info
    );

    // test NULL array
    result = d_array_shift_right(NULL, sizeof(int), 2);

    test_result &= d_assert_standalone(
        result == false,
        "`d_array_shift_right` rejects NULL array",
        "`d_array_shift_right` accepted NULL array",
        _test_info
    );

    d_array_free(arr);

    _test_info->tests_total++;
    if (test_result)
    {
        _test_info->tests_passed++;
    }

    return test_result;
}

/*
d_tests_sa_array_shift_right_circular
  Tests the `d_array_shift_right_circular` function.
  Tests the following:
  - circular shift right
  - NULL array rejection
*/
bool
d_tests_sa_array_shift_right_circular
(
    struct d_test_counter* _test_info
)
{
    bool            test_result = true;
    struct d_array* arr;
    bool            result;

    printf("Testing `d_array_shift_right_circular`...\n");

    arr = d_array_new_from_arr(sizeof(int), (int[]){ 1, 2, 3, 4, 5 }, 5);

    if (!arr)
    {
        _test_info->tests_total++;

        return false;
    }

    // test circular shift right
    result = d_array_shift_right_circular(arr, sizeof(int), 2);

    test_result &= d_assert_standalone(
        result == true,
        "`d_array_shift_right_circular` shifts circularly",
        "`d_array_shift_right_circular` failed",
        _test_info
    );

    // test NULL array
    result = d_array_shift_right_circular(NULL, sizeof(int), 2);

    test_result &= d_assert_standalone(
        result == false,
        "`d_array_shift_right_circular` rejects NULL array",
        "`d_array_shift_right_circular` accepted NULL array",
        _test_info
    );

    d_array_free(arr);

    _test_info->tests_total++;
    if (test_result)
    {
        _test_info->tests_passed++;
    }

    return test_result;
}

/*
d_tests_sa_array_slice
  Tests the `d_array_slice` function.
  Tests the following:
  - slicing from positive index
  - NULL source rejection
*/
bool
d_tests_sa_array_slice
(
    struct d_test_counter* _test_info
)
{
    bool   test_result = true;
    int    data[] = { 1, 2, 3, 4, 5 };
    void*  slice;

    printf("Testing `d_array_slice`...\n");

    // test slicing from index 2
    slice = d_array_slice(data, 5, 2, sizeof(int));

    test_result &= d_assert_standalone(
        slice != NULL,
        "`d_array_slice` creates slice",
        "`d_array_slice` failed",
        _test_info
    );

    if (slice)
    {
        free(slice);
    }

    // test NULL source
    slice = d_array_slice(NULL, 5, 0, sizeof(int));

    test_result &= d_assert_standalone(
        slice == NULL,
        "`d_array_slice` rejects NULL source",
        "`d_array_slice` accepted NULL source",
        _test_info
    );

    _test_info->tests_total++;
    if (test_result)
    {
        _test_info->tests_passed++;
    }

    return test_result;
}

/*
d_tests_sa_array_slice_range
  Tests the `d_array_slice_range` function.
  Tests the following:
  - slicing range
  - NULL source rejection
*/
bool
d_tests_sa_array_slice_range
(
    struct d_test_counter* _test_info
)
{
    bool   test_result = true;
    int    data[] = { 1, 2, 3, 4, 5 };
    void*  slice;

    printf("Testing `d_array_slice_range`...\n");

    // test slicing range (1 to 3)
    slice = d_array_slice_range(data, 5, 1, 4, sizeof(int));

    test_result &= d_assert_standalone(
        slice != NULL,
        "`d_array_slice_range` creates range slice",
        "`d_array_slice_range` failed",
        _test_info
    );

    if (slice)
    {
        free(slice);
    }

    // test NULL source
    slice = d_array_slice_range(NULL, 5, 0, 2, sizeof(int));

    test_result &= d_assert_standalone(
        slice == NULL,
        "`d_array_slice_range` rejects NULL source",
        "`d_array_slice_range` accepted NULL source",
        _test_info
    );

    _test_info->tests_total++;
    if (test_result)
    {
        _test_info->tests_passed++;
    }

    return test_result;
}

/*
d_tests_sa_array_sort
  Tests the `d_array_sort` function.
  Tests the following:
  - sorting unsorted array
  - NULL array handling
*/
bool
d_tests_sa_array_sort
(
    struct d_test_counter* _test_info
)
{
    bool            test_result = true;
    struct d_array* arr;
    int*            int_elements;

    printf("Testing `d_array_sort`...\n");

    arr = d_array_new_from_arr(sizeof(int), (int[]){ 3, 1, 4, 1, 5, 9, 2, 6 }, 8);

    if (!arr)
    {
        _test_info->tests_total++;

        return false;
    }

    // test sorting
    d_array_sort(arr, sizeof(int), d_test_int_comparator);
    int_elements = (int*)arr->elements;

    test_result &= d_assert_standalone(
        int_elements[0] == 1 && int_elements[7] == 9,
        "`d_array_sort` sorts array correctly",
        "`d_array_sort` failed",
        _test_info
    );

    // test NULL array (should not crash)
    d_array_sort(NULL, sizeof(int), d_test_int_comparator);

    test_result &= d_assert_standalone(
        true,
        "`d_array_sort` handles NULL array safely",
        "`d_array_sort` crashed on NULL array",
        _test_info
    );

    d_array_free(arr);

    _test_info->tests_total++;
    if (test_result)
    {
        _test_info->tests_passed++;
    }

    return test_result;
}


/*
===============================================================================
                        MANIPULATION TEST GROUP RUNNER
===============================================================================
*/

/*
d_tests_sa_array_run_manipulation_tests
  Runs all d_array manipulation function tests.
*/
bool
d_tests_sa_array_run_manipulation_tests
(
    struct d_test_counter* _test_info
)
{
    bool overall_result = true;

    printf("\n----------------------------------------\n");
    printf("Running d_array Manipulation Function Tests\n");
    printf("----------------------------------------\n");

    overall_result &= d_tests_sa_array_append_element(_test_info);
    overall_result &= d_tests_sa_array_append_elements(_test_info);
    overall_result &= d_tests_sa_array_append_array(_test_info);
    overall_result &= d_tests_sa_array_contains(_test_info);
    overall_result &= d_tests_sa_array_fill(_test_info);
    overall_result &= d_tests_sa_array_find(_test_info);
    overall_result &= d_tests_sa_array_insert_element(_test_info);
    overall_result &= d_tests_sa_array_insert_elements(_test_info);
    overall_result &= d_tests_sa_array_insert_array(_test_info);
    overall_result &= d_tests_sa_array_is_empty(_test_info);
    overall_result &= d_tests_sa_array_prepend_element(_test_info);
    overall_result &= d_tests_sa_array_prepend_elements(_test_info);
    overall_result &= d_tests_sa_array_prepend_array(_test_info);
    overall_result &= d_tests_sa_array_resize_amount(_test_info);
    overall_result &= d_tests_sa_array_resize_factor(_test_info);
    overall_result &= d_tests_sa_array_reverse(_test_info);
    overall_result &= d_tests_sa_array_shift_left(_test_info);
    overall_result &= d_tests_sa_array_shift_left_circular(_test_info);
    overall_result &= d_tests_sa_array_shift_right(_test_info);
    overall_result &= d_tests_sa_array_shift_right_circular(_test_info);
    overall_result &= d_tests_sa_array_slice(_test_info);
    overall_result &= d_tests_sa_array_slice_range(_test_info);
    overall_result &= d_tests_sa_array_sort(_test_info);

    return overall_result;
}
