/*******************************************************************************
* djinterp [container]                                array_common_tests_manip.c
*
*   Unit tests for `array_common.h` manipulation functions.
*   Part of the array_common suite, on DTest's C face.
*
*   IMPORTANT: Functions that call realloc (insert, prepend) require
*   dynamically allocated arrays, not stack-allocated arrays.
*
*
* path:      /tests/djinterp/c/container/array/array_common_tests_manip.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.10.25
*                                                            revised: 2026.11.09
*******************************************************************************/

// djinterp
#include "./array_common_tests.h"
#include "../../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's C face


/******************************************************************************
 * INSERT/PREPEND FUNCTION TESTS
 *****************************************************************************/

/*
d_tests_array_common_insert_element
  Tests the `d_array_common_insert_element` function.
  Tests the following:
  - inserting at beginning, middle, end
  - negative index support
  - NULL parameter handling
*/
bool d_tests_array_common_insert_element(
    struct dj_test_suite* _suite
)
{
    bool   test_result;
    bool   result;
    int*   elements;
    size_t count;
    int    value;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "array_common_insert_element");

    test_result = true;
    elements    = NULL;
    value       = 99;

    //=========================================================================
    // Test 1: Insert at beginning (index 0)
    //=========================================================================
    elements = (int*)malloc(5 * sizeof(int));
    if (elements)
    {
        elements[0] = 1; elements[1] = 2; elements[2] = 3;
        elements[3] = 4; elements[4] = 5;
        count = 5;

        result = d_array_common_insert_element((void**)&elements, &count,
                                               sizeof(int), &value, 0);

        test_result &= D_TEST_CHECK_MSG(_suite,
                                        "inserts at beginning correctly",
                                        ((result == true) && (count == 6) && (elements[0] == 99) && (elements[1] == 1)),
                                        "failed to insert at beginning");
        free(elements);
    }

    //=========================================================================
    // Test 2: Insert in middle (index 2)
    //=========================================================================
    elements = (int*)malloc(5 * sizeof(int));
    if (elements)
    {
        elements[0] = 1; elements[1] = 2; elements[2] = 3;
        elements[3] = 4; elements[4] = 5;
        count = 5;

        result = d_array_common_insert_element((void**)&elements, &count,
                                               sizeof(int), &value, 2);

        test_result &= D_TEST_CHECK_MSG(_suite,
                                        "inserts in middle correctly",
                                        ((result == true) && (count == 6) && (elements[2] == 99) && (elements[3] == 3)),
                                        "failed to insert in middle");
        free(elements);
    }

    //=========================================================================
    // Test 3: NULL elements pointer - Expected: failure
    //=========================================================================
    count = 5;
    result = d_array_common_insert_element(NULL, &count, sizeof(int), &value, 0);
    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "handles NULL elements correctly",
                                    (result == false),
                                    "failed with NULL elements");

    //=========================================================================
    // Test 4: Insert using negative index (-1 = last element)
    //=========================================================================
    elements = (int*)malloc(5 * sizeof(int));
    if (elements)
    {
        elements[0] = 1; elements[1] = 2; elements[2] = 3;
        elements[3] = 4; elements[4] = 5;
        count = 5;

        result = d_array_common_insert_element((void**)&elements, &count,
                                               sizeof(int), &value, -1);

        test_result &= D_TEST_CHECK_MSG(_suite,
                                        "handles negative index (-1) correctly",
                                        ((result == true) && (count == 6) && (elements[4] == 99)),
                                        "failed with negative index");
        free(elements);
    }

    return test_result;
}

/*
d_tests_array_common_insert_elements
  Tests the `d_array_common_insert_elements` function.
*/
bool d_tests_array_common_insert_elements(
    struct dj_test_suite* _suite
)
{
    bool   test_result;
    bool   result;
    int*   elements;
    size_t count;
    int    values[2];

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "array_common_insert_elements");

    test_result = true;
    elements    = NULL;
    values[0]   = 88;
    values[1]   = 99;

    //=========================================================================
    // Test 1: Insert multiple at beginning
    //=========================================================================
    elements = (int*)malloc(5 * sizeof(int));
    if (elements)
    {
        elements[0] = 1; elements[1] = 2; elements[2] = 3;
        elements[3] = 4; elements[4] = 5;
        count = 5;

        result = d_array_common_insert_elements((void**)&elements, &count,
                                                sizeof(int), values, 2, 0);

        test_result &= D_TEST_CHECK_MSG(_suite,
                                        "inserts multiple at beginning correctly",
                                        ((result == true) && (count == 7) && (elements[0] == 88) && (elements[1] == 99)),
                                        "failed to insert multiple at beginning");
        free(elements);
    }

    //=========================================================================
    // Test 2: Insert zero elements (no-op)
    //=========================================================================
    elements = (int*)malloc(5 * sizeof(int));
    if (elements)
    {
        elements[0] = 1; elements[1] = 2; elements[2] = 3;
        elements[3] = 4; elements[4] = 5;
        count = 5;

        result = d_array_common_insert_elements((void**)&elements, &count,
                                                sizeof(int), values, 0, 2);

        test_result &= D_TEST_CHECK_MSG(_suite,
                                        "handles zero count correctly",
                                        ((result == true) && (count == 5)),
                                        "failed with zero count");
        free(elements);
    }

    return test_result;
}

/*
d_tests_array_common_prepend_element
  Tests the `d_array_common_prepend_element` function.
*/
bool d_tests_array_common_prepend_element(
    struct dj_test_suite* _suite
)
{
    bool   test_result;
    bool   result;
    int*   elements;
    size_t count;
    int    value;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "array_common_prepend_element");

    test_result = true;
    elements    = NULL;
    value       = 99;

    //=========================================================================
    // Test 1: Prepend single element
    //=========================================================================
    elements = (int*)malloc(5 * sizeof(int));
    if (elements)
    {
        elements[0] = 1; elements[1] = 2; elements[2] = 3;
        elements[3] = 4; elements[4] = 5;
        count = 5;

        result = d_array_common_prepend_element((void**)&elements, &count,
                                                sizeof(int), &value);

        test_result &= D_TEST_CHECK_MSG(_suite,
                                        "prepends element correctly",
                                        ((result == true) && (count == 6) && (elements[0] == 99) && (elements[1] == 1)),
                                        "failed to prepend element");
        free(elements);
    }

    //=========================================================================
    // Test 2: NULL parameters - Expected: failure
    //=========================================================================
    count = 5;
    result = d_array_common_prepend_element(NULL, &count, sizeof(int), &value);
    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "handles NULL elements correctly",
                                    (result == false),
                                    "failed with NULL elements");

    return test_result;
}

/*
d_tests_array_common_prepend_elements
  Tests the `d_array_common_prepend_elements` function.
*/
bool d_tests_array_common_prepend_elements(
    struct dj_test_suite* _suite
)
{
    bool   test_result;
    bool   result;
    int*   elements;
    size_t count;
    int    values[2];

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "array_common_prepend_elements");

    test_result = true;
    elements    = NULL;
    values[0]   = 88;
    values[1]   = 99;

    //=========================================================================
    // Test 1: Prepend multiple elements
    //=========================================================================
    elements = (int*)malloc(3 * sizeof(int));
    if (elements)
    {
        elements[0] = 1; elements[1] = 2; elements[2] = 3;
        count = 3;

        result = d_array_common_prepend_elements((void**)&elements, &count,
                                                 sizeof(int), values, 2);

        test_result &= D_TEST_CHECK_MSG(_suite,
                                        "prepends multiple elements correctly",
                                        ((result == true) && (count == 5) && (elements[0] == 88) && (elements[2] == 1)),
                                        "failed to prepend multiple elements");
        free(elements);
    }

    return test_result;
}


/******************************************************************************
 * RESIZE VALIDATION FUNCTION TESTS
 *****************************************************************************/

/*
d_tests_array_common_is_valid_resize_amount
  Tests the `d_array_common_is_valid_resize_amount` function.
*/
bool d_tests_array_common_is_valid_resize_amount(
    struct dj_test_suite* _suite
)
{
    bool   test_result;
    bool   result;
    size_t result_size;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "array_common_is_valid_resize_amount");

    test_result = true;
    result_size = 0;

    //=========================================================================
    // Test 1: Valid positive resize (10 + 5 = 15)
    //=========================================================================
    result = d_array_common_is_valid_resize_amount(10, 5, &result_size);
    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "handles positive resize correctly",
                                    ((result == true) && (result_size == 15)),
                                    "failed with positive resize");

    //=========================================================================
    // Test 2: Valid negative resize (10 - 3 = 7)
    //=========================================================================
    result = d_array_common_is_valid_resize_amount(10, -3, &result_size);
    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "handles negative resize correctly",
                                    ((result == true) && (result_size == 7)),
                                    "failed with negative resize");

    //=========================================================================
    // Test 3: Underflow detection (5 - 10 < 0) - Expected: failure
    //=========================================================================
    result = d_array_common_is_valid_resize_amount(5, -10, &result_size);
    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "detects underflow correctly",
                                    (result == false),
                                    "failed to detect underflow");

    //=========================================================================
    // Test 4: NULL result pointer - Expected: failure
    //=========================================================================
    result = d_array_common_is_valid_resize_amount(10, 5, NULL);
    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "handles NULL result pointer",
                                    (result == false),
                                    "failed with NULL result pointer");

    //=========================================================================
    // Test 5: Resize by 0 (no change)
    //=========================================================================
    result = d_array_common_is_valid_resize_amount(10, 0, &result_size);
    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "handles zero resize correctly",
                                    ((result == true) && (result_size == 10)),
                                    "failed with zero resize");

    return test_result;
}

/*
d_tests_array_common_is_valid_resize_factor
  Tests the `d_array_common_is_valid_resize_factor` function.
*/
bool d_tests_array_common_is_valid_resize_factor(
    struct dj_test_suite* _suite
)
{
    bool   test_result;
    bool   result;
    double result_size;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "array_common_is_valid_resize_factor");

    test_result = true;
    result_size = 0.0;

    //=========================================================================
    // Test 1: Growth factor (2x)
    //=========================================================================
    result = d_array_common_is_valid_resize_factor(10, 2.0, &result_size, false);
    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "handles 2x growth factor",
                                    ((result == true) && (result_size == 20.0)),
                                    "failed with 2x factor");

    //=========================================================================
    // Test 2: Shrink factor (0.5x)
    //=========================================================================
    result = d_array_common_is_valid_resize_factor(10, 0.5, &result_size, false);
    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "handles 0.5x shrink factor",
                                    ((result == true) && (result_size == 5.0)),
                                    "failed with 0.5x factor");

    //=========================================================================
    // Test 3: Negative factor - Expected: failure
    //=========================================================================
    result = d_array_common_is_valid_resize_factor(10, -1.0, &result_size, false);
    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "rejects negative factor",
                                    (result == false),
                                    "accepted negative factor");

    //=========================================================================
    // Test 4: NULL result pointer - Expected: failure
    //=========================================================================
    result = d_array_common_is_valid_resize_factor(10, 2.0, NULL, false);
    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "handles NULL result pointer",
                                    (result == false),
                                    "failed with NULL result pointer");

    return test_result;
}

/*
d_tests_array_common_resize_amount
  Tests the `d_array_common_resize_amount` function.
*/
bool d_tests_array_common_resize_amount(
    struct dj_test_suite* _suite
)
{
    bool    test_result;
    ssize_t result;
    int     elements[5];

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "array_common_resize_amount");

    test_result = true;

    //=========================================================================
    // Test 1: Positive resize (10 + 5 = 15)
    //=========================================================================
    result = d_array_common_resize_amount(elements, 10, sizeof(int), 5);
    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "calculates positive resize correctly",
                                    (result == 15),
                                    "failed with positive resize");

    //=========================================================================
    // Test 2: Negative resize (10 - 3 = 7)
    //=========================================================================
    result = d_array_common_resize_amount(elements, 10, sizeof(int), -3);
    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "calculates negative resize correctly",
                                    (result == 7),
                                    "failed with negative resize");

    //=========================================================================
    // Test 3: NULL elements - Expected: -1
    //=========================================================================
    result = d_array_common_resize_amount(NULL, 10, sizeof(int), 5);
    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "handles NULL elements correctly",
                                    (result == -1),
                                    "failed with NULL elements");

    //=========================================================================
    // Test 4: Underflow (5 - 10) - Expected: -1
    //=========================================================================
    result = d_array_common_resize_amount(elements, 5, sizeof(int), -10);
    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "detects underflow correctly",
                                    (result == -1),
                                    "failed to detect underflow");

    return test_result;
}

/*
d_tests_array_common_resize_factor
  Tests the `d_array_common_resize_factor` function.
*/
bool d_tests_array_common_resize_factor(
    struct dj_test_suite* _suite
)
{
    bool    test_result;
    ssize_t result;
    int     elements[5];

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "array_common_resize_factor");

    test_result = true;

    //=========================================================================
    // Test 1: Double factor (2x): 10 * 2.0 = 20
    //=========================================================================
    result = d_array_common_resize_factor(elements, 10, sizeof(int), 2.0);
    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "calculates 2x factor correctly",
                                    (result == 20),
                                    "failed with 2x factor");

    //=========================================================================
    // Test 2: Half factor (0.5x): 10 * 0.5 = 5
    //=========================================================================
    result = d_array_common_resize_factor(elements, 10, sizeof(int), 0.5);
    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "calculates 0.5x factor correctly",
                                    (result == 5),
                                    "failed with 0.5x factor");

    //=========================================================================
    // Test 3: NULL elements - Expected: -1
    //=========================================================================
    result = d_array_common_resize_factor(NULL, 10, sizeof(int), 2.0);
    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "handles NULL elements correctly",
                                    (result == -1),
                                    "failed with NULL elements");

    //=========================================================================
    // Test 4: Negative factor - Expected: -1
    //=========================================================================
    result = d_array_common_resize_factor(elements, 10, sizeof(int), -1.0);
    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "rejects negative factor",
                                    (result == -1),
                                    "accepted negative factor");

    return test_result;
}


/******************************************************************************
 * ARRAY TRANSFORMATION FUNCTION TESTS
 *****************************************************************************/

/*
d_tests_array_common_reverse
  Tests the `d_array_common_reverse` function.
*/
bool d_tests_array_common_reverse(
    struct dj_test_suite* _suite
)
{
    bool test_result;
    bool result;
    int  arr[6];

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "array_common_reverse");

    test_result = true;

    //=========================================================================
    // Test 1: Reverse 5-element array (odd)
    //=========================================================================
    arr[0] = 1; arr[1] = 2; arr[2] = 3; arr[3] = 4; arr[4] = 5;
    result = d_array_common_reverse(arr, 5, sizeof(int));
    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "reverses odd-length array correctly",
                                    ((result == true) && (arr[0] == 5) && (arr[2] == 3) && (arr[4] == 1)),
                                    "failed to reverse odd-length array");

    //=========================================================================
    // Test 2: Reverse 6-element array (even)
    //=========================================================================
    arr[0] = 1; arr[1] = 2; arr[2] = 3; arr[3] = 4; arr[4] = 5; arr[5] = 6;
    result = d_array_common_reverse(arr, 6, sizeof(int));
    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "reverses even-length array correctly",
                                    ((result == true) && (arr[0] == 6) && (arr[5] == 1)),
                                    "failed to reverse even-length array");

    //=========================================================================
    // Test 3: Single element (trivially successful)
    //=========================================================================
    arr[0] = 42;
    result = d_array_common_reverse(arr, 1, sizeof(int));
    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "handles single element (no-op)",
                                    ((result == true) && (arr[0] == 42)),
                                    "failed with single element");

    //=========================================================================
    // Test 4: NULL elements - Expected: failure
    //=========================================================================
    result = d_array_common_reverse(NULL, 5, sizeof(int));
    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "handles NULL elements correctly",
                                    (result == false),
                                    "failed with NULL elements");

    return test_result;
}

/*
d_tests_array_common_shift_left
  Tests the `d_array_common_shift_left` function.
*/
bool d_tests_array_common_shift_left(
    struct dj_test_suite* _suite
)
{
    bool test_result;
    bool result;
    int  arr[5];

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "array_common_shift_left");

    test_result = true;

    //=========================================================================
    // Test 1: Shift left by 2
    //=========================================================================
    arr[0] = 1; arr[1] = 2; arr[2] = 3; arr[3] = 4; arr[4] = 5;
    result = d_array_common_shift_left(arr, 5, sizeof(int), 2);
    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "shifts left by 2 correctly",
                                    ((result == true) && (arr[0] == 3) && (arr[1] == 4) && (arr[2] == 5)),
                                    "failed shifting left by 2");

    //=========================================================================
    // Test 2: Shift by amount >= count - Expected: failure
    //=========================================================================
    arr[0] = 1; arr[1] = 2; arr[2] = 3;
    result = d_array_common_shift_left(arr, 3, sizeof(int), 5);
    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "rejects shift >= count",
                                    (result == false),
                                    "accepted invalid shift amount");

    //=========================================================================
    // Test 3: Zero shift (no-op)
    //=========================================================================
    arr[0] = 1; arr[1] = 2; arr[2] = 3;
    result = d_array_common_shift_left(arr, 3, sizeof(int), 0);
    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "handles zero shift (no-op)",
                                    ((result == true) && (arr[0] == 1)),
                                    "failed with zero shift");

    return test_result;
}

/*
d_tests_array_common_shift_left_circular
  Tests the `d_array_common_shift_left_circular` function.
*/
bool d_tests_array_common_shift_left_circular(
    struct dj_test_suite* _suite
)
{
    bool test_result;
    bool result;
    int  arr[5];

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "array_common_shift_left_circular");

    test_result = true;

    //=========================================================================
    // Test 1: Circular shift left by 2
    //   [1, 2, 3, 4, 5] -> [3, 4, 5, 1, 2]
    //=========================================================================
    arr[0] = 1; arr[1] = 2; arr[2] = 3; arr[3] = 4; arr[4] = 5;
    result = d_array_common_shift_left_circular(arr, 5, sizeof(int), 2);
    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "circular shifts left correctly",
                                    ((result == true) && (arr[0] == 3) && (arr[3] == 1) && (arr[4] == 2)),
                                    "failed circular shift");

    //=========================================================================
    // Test 2: Shift by exact count (returns to original)
    //=========================================================================
    arr[0] = 10; arr[1] = 20; arr[2] = 30;
    result = d_array_common_shift_left_circular(arr, 3, sizeof(int), 3);
    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "handles shift by count (returns to original)",
                                    ((result == true) && (arr[0] == 10) && (arr[2] == 30)),
                                    "failed with shift by count");

    return test_result;
}

/*
d_tests_array_common_shift_right
  Tests the `d_array_common_shift_right` function.
*/
bool d_tests_array_common_shift_right(
    struct dj_test_suite* _suite
)
{
    bool test_result;
    bool result;
    int  arr[5];

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "array_common_shift_right");

    test_result = true;

    //=========================================================================
    // Test 1: Shift right by 2
    //=========================================================================
    arr[0] = 1; arr[1] = 2; arr[2] = 3; arr[3] = 4; arr[4] = 5;
    result = d_array_common_shift_right(arr, 5, sizeof(int), 2);
    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "shifts right by 2 correctly",
                                    ((result == true) && (arr[2] == 1) && (arr[3] == 2) && (arr[4] == 3)),
                                    "failed shifting right by 2");

    //=========================================================================
    // Test 2: Shift by amount >= count - Expected: failure
    //=========================================================================
    arr[0] = 1; arr[1] = 2; arr[2] = 3;
    result = d_array_common_shift_right(arr, 3, sizeof(int), 5);
    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "rejects shift >= count",
                                    (result == false),
                                    "accepted invalid shift amount");

    return test_result;
}

/*
d_tests_array_common_shift_right_circular
  Tests the `d_array_common_shift_right_circular` function.
*/
bool d_tests_array_common_shift_right_circular(
    struct dj_test_suite* _suite
)
{
    bool test_result;
    bool result;
    int  arr[5];

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "array_common_shift_right_circular");

    test_result = true;

    //=========================================================================
    // Test 1: Circular shift right by 2
    //   [1, 2, 3, 4, 5] -> [4, 5, 1, 2, 3]
    //=========================================================================
    arr[0] = 1; arr[1] = 2; arr[2] = 3; arr[3] = 4; arr[4] = 5;
    result = d_array_common_shift_right_circular(arr, 5, sizeof(int), 2);
    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "circular shifts right correctly",
                                    ((result == true) && (arr[0] == 4) && (arr[1] == 5) && (arr[2] == 1)),
                                    "failed circular shift");

    //=========================================================================
    // Test 2: Shift by exact count (returns to original)
    //=========================================================================
    arr[0] = 10; arr[1] = 20; arr[2] = 30;
    result = d_array_common_shift_right_circular(arr, 3, sizeof(int), 3);
    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "handles shift by count (returns to original)",
                                    ((result == true) && (arr[0] == 10) && (arr[2] == 30)),
                                    "failed with shift by count");

    return test_result;
}

/*
d_tests_array_common_sort
  Tests the `d_array_common_sort` function.
*/
bool d_tests_array_common_sort(
    struct dj_test_suite* _suite
)
{
    bool test_result;
    int  arr[8];

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "array_common_sort");

    test_result = true;

    //=========================================================================
    // Test 1: Sort unsorted array
    //=========================================================================
    arr[0] = 3; arr[1] = 1; arr[2] = 4; arr[3] = 1;
    arr[4] = 5; arr[5] = 9; arr[6] = 2; arr[7] = 6;
    d_array_common_sort(arr, 8, sizeof(int), d_test_int_comparator);
    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "sorts unsorted array correctly",
                                    ((arr[0] == 1) && (arr[1] == 1) && (arr[7] == 9)),
                                    "failed to sort unsorted array");

    //=========================================================================
    // Test 2: Sort already sorted array
    //=========================================================================
    arr[0] = 1; arr[1] = 2; arr[2] = 3; arr[3] = 4; arr[4] = 5;
    d_array_common_sort(arr, 5, sizeof(int), d_test_int_comparator);
    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "handles already sorted array",
                                    ((arr[0] == 1) && (arr[4] == 5)),
                                    "failed with sorted array");

    //=========================================================================
    // Test 3: Sort reverse-sorted array
    //=========================================================================
    arr[0] = 5; arr[1] = 4; arr[2] = 3; arr[3] = 2; arr[4] = 1;
    d_array_common_sort(arr, 5, sizeof(int), d_test_int_comparator);
    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "sorts reverse-sorted array correctly",
                                    ((arr[0] == 1) && (arr[4] == 5)),
                                    "failed with reverse-sorted array");

    //=========================================================================
    // Test 4: Single element (no-op)
    //=========================================================================
    arr[0] = 42;
    d_array_common_sort(arr, 1, sizeof(int), d_test_int_comparator);
    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "handles single element (no-op)",
                                    (arr[0] == 42),
                                    "failed with single element");

    return test_result;
}


/******************************************************************************
 * MANIPULATION TEST GROUP RUNNER
 *****************************************************************************/

/*
d_tests_array_common_run_manip_tests
  Runs all manipulation function tests.
*/
bool d_tests_array_common_run_manip_tests(
    struct dj_test_suite* _suite
)
{
    bool overall_result;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_block(_suite, "array_common_run_manip_tests");

    overall_result = true;

    printf("\n----------------------------------------\n");
    printf("Running Manipulation Function Tests\n");

    overall_result &= d_tests_array_common_insert_element(_suite);
    overall_result &= d_tests_array_common_insert_elements(_suite);
    overall_result &= d_tests_array_common_is_valid_resize_amount(_suite);
    overall_result &= d_tests_array_common_is_valid_resize_factor(_suite);
    overall_result &= d_tests_array_common_prepend_element(_suite);
    overall_result &= d_tests_array_common_prepend_elements(_suite);
    overall_result &= d_tests_array_common_resize_amount(_suite);
    overall_result &= d_tests_array_common_resize_factor(_suite);
    overall_result &= d_tests_array_common_reverse(_suite);
    overall_result &= d_tests_array_common_shift_left(_suite);
    overall_result &= d_tests_array_common_shift_left_circular(_suite);
    overall_result &= d_tests_array_common_shift_right(_suite);
    overall_result &= d_tests_array_common_shift_right_circular(_suite);
    overall_result &= d_tests_array_common_sort(_suite);

    return overall_result;
}
