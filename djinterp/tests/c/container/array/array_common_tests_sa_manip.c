/******************************************************************************
* djinterp [container]                            array_common_tests_sa_manip.c
*
*   Unit tests for `array_common.h` manipulation functions.
*   Part of the array_common standalone test suite.
*
*   IMPORTANT: Functions that call realloc (insert, prepend) require
*   dynamically allocated arrays, not stack-allocated arrays.
*
*
* path:      \test\container\array\array_common_tests_sa_manip.c
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.10.25
******************************************************************************/

#include ".\array_common_tests_sa.h"


/******************************************************************************
 * INSERT/PREPEND FUNCTION TESTS
 *****************************************************************************/

/*
d_tests_sa_array_common_insert_element
  Tests the `d_array_common_insert_element` function.
  Tests the following:
  - inserting at beginning, middle, end
  - negative index support
  - NULL parameter handling
*/
bool
d_tests_sa_array_common_insert_element
(
    struct d_test_counter* _test_info
)
{
    bool   test_result;
    bool   result;
    int*   elements;
    size_t count;
    int    value;

    if (!_test_info)
    {
        return false;
    }

    test_result = true;
    elements    = NULL;
    value       = 99;

    printf("Testing `d_array_common_insert_element`...\n");

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

        test_result &= d_assert_standalone(
            ((result == true) && (count == 6) && (elements[0] == 99) && (elements[1] == 1)),
            "inserts at beginning correctly",
            "failed to insert at beginning",
            _test_info
        );
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

        test_result &= d_assert_standalone(
            ((result == true) && (count == 6) && (elements[2] == 99) && (elements[3] == 3)),
            "inserts in middle correctly",
            "failed to insert in middle",
            _test_info
        );
        free(elements);
    }

    //=========================================================================
    // Test 3: NULL elements pointer - Expected: failure
    //=========================================================================
    count = 5;
    result = d_array_common_insert_element(NULL, &count, sizeof(int), &value, 0);
    test_result &= d_assert_standalone(
        (result == false),
        "handles NULL elements correctly",
        "failed with NULL elements",
        _test_info
    );

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

        test_result &= d_assert_standalone(
            ((result == true) && (count == 6) && (elements[4] == 99)),
            "handles negative index (-1) correctly",
            "failed with negative index",
            _test_info
        );
        free(elements);
    }

    _test_info->tests_total++;
    if (test_result) { _test_info->tests_passed++; }

    return test_result;
}

/*
d_tests_sa_array_common_insert_elements
  Tests the `d_array_common_insert_elements` function.
*/
bool
d_tests_sa_array_common_insert_elements
(
    struct d_test_counter* _test_info
)
{
    bool   test_result;
    bool   result;
    int*   elements;
    size_t count;
    int    values[2];

    if (!_test_info)
    {
        return false;
    }

    test_result = true;
    elements    = NULL;
    values[0]   = 88;
    values[1]   = 99;

    printf("Testing `d_array_common_insert_elements`...\n");

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

        test_result &= d_assert_standalone(
            ((result == true) && (count == 7) && (elements[0] == 88) && (elements[1] == 99)),
            "inserts multiple at beginning correctly",
            "failed to insert multiple at beginning",
            _test_info
        );
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

        test_result &= d_assert_standalone(
            ((result == true) && (count == 5)),
            "handles zero count correctly",
            "failed with zero count",
            _test_info
        );
        free(elements);
    }

    _test_info->tests_total++;
    if (test_result) { _test_info->tests_passed++; }

    return test_result;
}

/*
d_tests_sa_array_common_prepend_element
  Tests the `d_array_common_prepend_element` function.
*/
bool
d_tests_sa_array_common_prepend_element
(
    struct d_test_counter* _test_info
)
{
    bool   test_result;
    bool   result;
    int*   elements;
    size_t count;
    int    value;

    if (!_test_info)
    {
        return false;
    }

    test_result = true;
    elements    = NULL;
    value       = 99;

    printf("Testing `d_array_common_prepend_element`...\n");

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

        test_result &= d_assert_standalone(
            ((result == true) && (count == 6) && (elements[0] == 99) && (elements[1] == 1)),
            "prepends element correctly",
            "failed to prepend element",
            _test_info
        );
        free(elements);
    }

    //=========================================================================
    // Test 2: NULL parameters - Expected: failure
    //=========================================================================
    count = 5;
    result = d_array_common_prepend_element(NULL, &count, sizeof(int), &value);
    test_result &= d_assert_standalone(
        (result == false),
        "handles NULL elements correctly",
        "failed with NULL elements",
        _test_info
    );

    _test_info->tests_total++;
    if (test_result) { _test_info->tests_passed++; }

    return test_result;
}

/*
d_tests_sa_array_common_prepend_elements
  Tests the `d_array_common_prepend_elements` function.
*/
bool
d_tests_sa_array_common_prepend_elements
(
    struct d_test_counter* _test_info
)
{
    bool   test_result;
    bool   result;
    int*   elements;
    size_t count;
    int    values[2];

    if (!_test_info)
    {
        return false;
    }

    test_result = true;
    elements    = NULL;
    values[0]   = 88;
    values[1]   = 99;

    printf("Testing `d_array_common_prepend_elements`...\n");

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

        test_result &= d_assert_standalone(
            ((result == true) && (count == 5) && (elements[0] == 88) && (elements[2] == 1)),
            "prepends multiple elements correctly",
            "failed to prepend multiple elements",
            _test_info
        );
        free(elements);
    }

    _test_info->tests_total++;
    if (test_result) { _test_info->tests_passed++; }

    return test_result;
}


/******************************************************************************
 * RESIZE VALIDATION FUNCTION TESTS
 *****************************************************************************/

/*
d_tests_sa_array_common_is_valid_resize_amount
  Tests the `d_array_common_is_valid_resize_amount` function.
*/
bool
d_tests_sa_array_common_is_valid_resize_amount
(
    struct d_test_counter* _test_info
)
{
    bool   test_result;
    bool   result;
    size_t result_size;

    if (!_test_info)
    {
        return false;
    }

    test_result = true;
    result_size = 0;

    printf("Testing `d_array_common_is_valid_resize_amount`...\n");

    //=========================================================================
    // Test 1: Valid positive resize (10 + 5 = 15)
    //=========================================================================
    result = d_array_common_is_valid_resize_amount(10, 5, &result_size);
    test_result &= d_assert_standalone(
        ((result == true) && (result_size == 15)),
        "handles positive resize correctly",
        "failed with positive resize",
        _test_info
    );

    //=========================================================================
    // Test 2: Valid negative resize (10 - 3 = 7)
    //=========================================================================
    result = d_array_common_is_valid_resize_amount(10, -3, &result_size);
    test_result &= d_assert_standalone(
        ((result == true) && (result_size == 7)),
        "handles negative resize correctly",
        "failed with negative resize",
        _test_info
    );

    //=========================================================================
    // Test 3: Underflow detection (5 - 10 < 0) - Expected: failure
    //=========================================================================
    result = d_array_common_is_valid_resize_amount(5, -10, &result_size);
    test_result &= d_assert_standalone(
        (result == false),
        "detects underflow correctly",
        "failed to detect underflow",
        _test_info
    );

    //=========================================================================
    // Test 4: NULL result pointer - Expected: failure
    //=========================================================================
    result = d_array_common_is_valid_resize_amount(10, 5, NULL);
    test_result &= d_assert_standalone(
        (result == false),
        "handles NULL result pointer",
        "failed with NULL result pointer",
        _test_info
    );

    //=========================================================================
    // Test 5: Resize by 0 (no change)
    //=========================================================================
    result = d_array_common_is_valid_resize_amount(10, 0, &result_size);
    test_result &= d_assert_standalone(
        ((result == true) && (result_size == 10)),
        "handles zero resize correctly",
        "failed with zero resize",
        _test_info
    );

    _test_info->tests_total++;
    if (test_result) { _test_info->tests_passed++; }

    return test_result;
}

/*
d_tests_sa_array_common_is_valid_resize_factor
  Tests the `d_array_common_is_valid_resize_factor` function.
*/
bool
d_tests_sa_array_common_is_valid_resize_factor
(
    struct d_test_counter* _test_info
)
{
    bool   test_result;
    bool   result;
    double result_size;

    if (!_test_info)
    {
        return false;
    }

    test_result = true;
    result_size = 0.0;

    printf("Testing `d_array_common_is_valid_resize_factor`...\n");

    //=========================================================================
    // Test 1: Growth factor (2x)
    //=========================================================================
    result = d_array_common_is_valid_resize_factor(10, 2.0, &result_size, false);
    test_result &= d_assert_standalone(
        ((result == true) && (result_size == 20.0)),
        "handles 2x growth factor",
        "failed with 2x factor",
        _test_info
    );

    //=========================================================================
    // Test 2: Shrink factor (0.5x)
    //=========================================================================
    result = d_array_common_is_valid_resize_factor(10, 0.5, &result_size, false);
    test_result &= d_assert_standalone(
        ((result == true) && (result_size == 5.0)),
        "handles 0.5x shrink factor",
        "failed with 0.5x factor",
        _test_info
    );

    //=========================================================================
    // Test 3: Negative factor - Expected: failure
    //=========================================================================
    result = d_array_common_is_valid_resize_factor(10, -1.0, &result_size, false);
    test_result &= d_assert_standalone(
        (result == false),
        "rejects negative factor",
        "accepted negative factor",
        _test_info
    );

    //=========================================================================
    // Test 4: NULL result pointer - Expected: failure
    //=========================================================================
    result = d_array_common_is_valid_resize_factor(10, 2.0, NULL, false);
    test_result &= d_assert_standalone(
        (result == false),
        "handles NULL result pointer",
        "failed with NULL result pointer",
        _test_info
    );

    _test_info->tests_total++;
    if (test_result) { _test_info->tests_passed++; }

    return test_result;
}

/*
d_tests_sa_array_common_resize_amount
  Tests the `d_array_common_resize_amount` function.
*/
bool
d_tests_sa_array_common_resize_amount
(
    struct d_test_counter* _test_info
)
{
    bool    test_result;
    ssize_t result;
    int     elements[5];

    if (!_test_info)
    {
        return false;
    }

    test_result = true;

    printf("Testing `d_array_common_resize_amount`...\n");

    //=========================================================================
    // Test 1: Positive resize (10 + 5 = 15)
    //=========================================================================
    result = d_array_common_resize_amount(elements, 10, sizeof(int), 5);
    test_result &= d_assert_standalone(
        (result == 15),
        "calculates positive resize correctly",
        "failed with positive resize",
        _test_info
    );

    //=========================================================================
    // Test 2: Negative resize (10 - 3 = 7)
    //=========================================================================
    result = d_array_common_resize_amount(elements, 10, sizeof(int), -3);
    test_result &= d_assert_standalone(
        (result == 7),
        "calculates negative resize correctly",
        "failed with negative resize",
        _test_info
    );

    //=========================================================================
    // Test 3: NULL elements - Expected: -1
    //=========================================================================
    result = d_array_common_resize_amount(NULL, 10, sizeof(int), 5);
    test_result &= d_assert_standalone(
        (result == -1),
        "handles NULL elements correctly",
        "failed with NULL elements",
        _test_info
    );

    //=========================================================================
    // Test 4: Underflow (5 - 10) - Expected: -1
    //=========================================================================
    result = d_array_common_resize_amount(elements, 5, sizeof(int), -10);
    test_result &= d_assert_standalone(
        (result == -1),
        "detects underflow correctly",
        "failed to detect underflow",
        _test_info
    );

    _test_info->tests_total++;
    if (test_result) { _test_info->tests_passed++; }

    return test_result;
}

/*
d_tests_sa_array_common_resize_factor
  Tests the `d_array_common_resize_factor` function.
*/
bool
d_tests_sa_array_common_resize_factor
(
    struct d_test_counter* _test_info
)
{
    bool    test_result;
    ssize_t result;
    int     elements[5];

    if (!_test_info)
    {
        return false;
    }

    test_result = true;

    printf("Testing `d_array_common_resize_factor`...\n");

    //=========================================================================
    // Test 1: Double factor (2x): 10 * 2.0 = 20
    //=========================================================================
    result = d_array_common_resize_factor(elements, 10, sizeof(int), 2.0);
    test_result &= d_assert_standalone(
        (result == 20),
        "calculates 2x factor correctly",
        "failed with 2x factor",
        _test_info
    );

    //=========================================================================
    // Test 2: Half factor (0.5x): 10 * 0.5 = 5
    //=========================================================================
    result = d_array_common_resize_factor(elements, 10, sizeof(int), 0.5);
    test_result &= d_assert_standalone(
        (result == 5),
        "calculates 0.5x factor correctly",
        "failed with 0.5x factor",
        _test_info
    );

    //=========================================================================
    // Test 3: NULL elements - Expected: -1
    //=========================================================================
    result = d_array_common_resize_factor(NULL, 10, sizeof(int), 2.0);
    test_result &= d_assert_standalone(
        (result == -1),
        "handles NULL elements correctly",
        "failed with NULL elements",
        _test_info
    );

    //=========================================================================
    // Test 4: Negative factor - Expected: -1
    //=========================================================================
    result = d_array_common_resize_factor(elements, 10, sizeof(int), -1.0);
    test_result &= d_assert_standalone(
        (result == -1),
        "rejects negative factor",
        "accepted negative factor",
        _test_info
    );

    _test_info->tests_total++;
    if (test_result) { _test_info->tests_passed++; }

    return test_result;
}


/******************************************************************************
 * ARRAY TRANSFORMATION FUNCTION TESTS
 *****************************************************************************/

/*
d_tests_sa_array_common_reverse
  Tests the `d_array_common_reverse` function.
*/
bool
d_tests_sa_array_common_reverse
(
    struct d_test_counter* _test_info
)
{
    bool test_result;
    bool result;
    int  arr[6];

    if (!_test_info)
    {
        return false;
    }

    test_result = true;

    printf("Testing `d_array_common_reverse`...\n");

    //=========================================================================
    // Test 1: Reverse 5-element array (odd)
    //=========================================================================
    arr[0] = 1; arr[1] = 2; arr[2] = 3; arr[3] = 4; arr[4] = 5;
    result = d_array_common_reverse(arr, 5, sizeof(int));
    test_result &= d_assert_standalone(
        ((result == true) && (arr[0] == 5) && (arr[2] == 3) && (arr[4] == 1)),
        "reverses odd-length array correctly",
        "failed to reverse odd-length array",
        _test_info
    );

    //=========================================================================
    // Test 2: Reverse 6-element array (even)
    //=========================================================================
    arr[0] = 1; arr[1] = 2; arr[2] = 3; arr[3] = 4; arr[4] = 5; arr[5] = 6;
    result = d_array_common_reverse(arr, 6, sizeof(int));
    test_result &= d_assert_standalone(
        ((result == true) && (arr[0] == 6) && (arr[5] == 1)),
        "reverses even-length array correctly",
        "failed to reverse even-length array",
        _test_info
    );

    //=========================================================================
    // Test 3: Single element (trivially successful)
    //=========================================================================
    arr[0] = 42;
    result = d_array_common_reverse(arr, 1, sizeof(int));
    test_result &= d_assert_standalone(
        ((result == true) && (arr[0] == 42)),
        "handles single element (no-op)",
        "failed with single element",
        _test_info
    );

    //=========================================================================
    // Test 4: NULL elements - Expected: failure
    //=========================================================================
    result = d_array_common_reverse(NULL, 5, sizeof(int));
    test_result &= d_assert_standalone(
        (result == false),
        "handles NULL elements correctly",
        "failed with NULL elements",
        _test_info
    );

    _test_info->tests_total++;
    if (test_result) { _test_info->tests_passed++; }

    return test_result;
}

/*
d_tests_sa_array_common_shift_left
  Tests the `d_array_common_shift_left` function.
*/
bool
d_tests_sa_array_common_shift_left
(
    struct d_test_counter* _test_info
)
{
    bool test_result;
    bool result;
    int  arr[5];

    if (!_test_info)
    {
        return false;
    }

    test_result = true;

    printf("Testing `d_array_common_shift_left`...\n");

    //=========================================================================
    // Test 1: Shift left by 2
    //=========================================================================
    arr[0] = 1; arr[1] = 2; arr[2] = 3; arr[3] = 4; arr[4] = 5;
    result = d_array_common_shift_left(arr, 5, sizeof(int), 2);
    test_result &= d_assert_standalone(
        ((result == true) && (arr[0] == 3) && (arr[1] == 4) && (arr[2] == 5)),
        "shifts left by 2 correctly",
        "failed shifting left by 2",
        _test_info
    );

    //=========================================================================
    // Test 2: Shift by amount >= count - Expected: failure
    //=========================================================================
    arr[0] = 1; arr[1] = 2; arr[2] = 3;
    result = d_array_common_shift_left(arr, 3, sizeof(int), 5);
    test_result &= d_assert_standalone(
        (result == false),
        "rejects shift >= count",
        "accepted invalid shift amount",
        _test_info
    );

    //=========================================================================
    // Test 3: Zero shift (no-op)
    //=========================================================================
    arr[0] = 1; arr[1] = 2; arr[2] = 3;
    result = d_array_common_shift_left(arr, 3, sizeof(int), 0);
    test_result &= d_assert_standalone(
        ((result == true) && (arr[0] == 1)),
        "handles zero shift (no-op)",
        "failed with zero shift",
        _test_info
    );

    _test_info->tests_total++;
    if (test_result) { _test_info->tests_passed++; }

    return test_result;
}

/*
d_tests_sa_array_common_shift_left_circular
  Tests the `d_array_common_shift_left_circular` function.
*/
bool
d_tests_sa_array_common_shift_left_circular
(
    struct d_test_counter* _test_info
)
{
    bool test_result;
    bool result;
    int  arr[5];

    if (!_test_info)
    {
        return false;
    }

    test_result = true;

    printf("Testing `d_array_common_shift_left_circular`...\n");

    //=========================================================================
    // Test 1: Circular shift left by 2
    //   [1, 2, 3, 4, 5] -> [3, 4, 5, 1, 2]
    //=========================================================================
    arr[0] = 1; arr[1] = 2; arr[2] = 3; arr[3] = 4; arr[4] = 5;
    result = d_array_common_shift_left_circular(arr, 5, sizeof(int), 2);
    test_result &= d_assert_standalone(
        ((result == true) && (arr[0] == 3) && (arr[3] == 1) && (arr[4] == 2)),
        "circular shifts left correctly",
        "failed circular shift",
        _test_info
    );

    //=========================================================================
    // Test 2: Shift by exact count (returns to original)
    //=========================================================================
    arr[0] = 10; arr[1] = 20; arr[2] = 30;
    result = d_array_common_shift_left_circular(arr, 3, sizeof(int), 3);
    test_result &= d_assert_standalone(
        ((result == true) && (arr[0] == 10) && (arr[2] == 30)),
        "handles shift by count (returns to original)",
        "failed with shift by count",
        _test_info
    );

    _test_info->tests_total++;
    if (test_result) { _test_info->tests_passed++; }

    return test_result;
}

/*
d_tests_sa_array_common_shift_right
  Tests the `d_array_common_shift_right` function.
*/
bool
d_tests_sa_array_common_shift_right
(
    struct d_test_counter* _test_info
)
{
    bool test_result;
    bool result;
    int  arr[5];

    if (!_test_info)
    {
        return false;
    }

    test_result = true;

    printf("Testing `d_array_common_shift_right`...\n");

    //=========================================================================
    // Test 1: Shift right by 2
    //=========================================================================
    arr[0] = 1; arr[1] = 2; arr[2] = 3; arr[3] = 4; arr[4] = 5;
    result = d_array_common_shift_right(arr, 5, sizeof(int), 2);
    test_result &= d_assert_standalone(
        ((result == true) && (arr[2] == 1) && (arr[3] == 2) && (arr[4] == 3)),
        "shifts right by 2 correctly",
        "failed shifting right by 2",
        _test_info
    );

    //=========================================================================
    // Test 2: Shift by amount >= count - Expected: failure
    //=========================================================================
    arr[0] = 1; arr[1] = 2; arr[2] = 3;
    result = d_array_common_shift_right(arr, 3, sizeof(int), 5);
    test_result &= d_assert_standalone(
        (result == false),
        "rejects shift >= count",
        "accepted invalid shift amount",
        _test_info
    );

    _test_info->tests_total++;
    if (test_result) { _test_info->tests_passed++; }

    return test_result;
}

/*
d_tests_sa_array_common_shift_right_circular
  Tests the `d_array_common_shift_right_circular` function.
*/
bool
d_tests_sa_array_common_shift_right_circular
(
    struct d_test_counter* _test_info
)
{
    bool test_result;
    bool result;
    int  arr[5];

    if (!_test_info)
    {
        return false;
    }

    test_result = true;

    printf("Testing `d_array_common_shift_right_circular`...\n");

    //=========================================================================
    // Test 1: Circular shift right by 2
    //   [1, 2, 3, 4, 5] -> [4, 5, 1, 2, 3]
    //=========================================================================
    arr[0] = 1; arr[1] = 2; arr[2] = 3; arr[3] = 4; arr[4] = 5;
    result = d_array_common_shift_right_circular(arr, 5, sizeof(int), 2);
    test_result &= d_assert_standalone(
        ((result == true) && (arr[0] == 4) && (arr[1] == 5) && (arr[2] == 1)),
        "circular shifts right correctly",
        "failed circular shift",
        _test_info
    );

    //=========================================================================
    // Test 2: Shift by exact count (returns to original)
    //=========================================================================
    arr[0] = 10; arr[1] = 20; arr[2] = 30;
    result = d_array_common_shift_right_circular(arr, 3, sizeof(int), 3);
    test_result &= d_assert_standalone(
        ((result == true) && (arr[0] == 10) && (arr[2] == 30)),
        "handles shift by count (returns to original)",
        "failed with shift by count",
        _test_info
    );

    _test_info->tests_total++;
    if (test_result) { _test_info->tests_passed++; }

    return test_result;
}

/*
d_tests_sa_array_common_sort
  Tests the `d_array_common_sort` function.
*/
bool
d_tests_sa_array_common_sort
(
    struct d_test_counter* _test_info
)
{
    bool test_result;
    int  arr[8];

    if (!_test_info)
    {
        return false;
    }

    test_result = true;

    printf("Testing `d_array_common_sort`...\n");

    //=========================================================================
    // Test 1: Sort unsorted array
    //=========================================================================
    arr[0] = 3; arr[1] = 1; arr[2] = 4; arr[3] = 1;
    arr[4] = 5; arr[5] = 9; arr[6] = 2; arr[7] = 6;
    d_array_common_sort(arr, 8, sizeof(int), d_test_int_comparator);
    test_result &= d_assert_standalone(
        ((arr[0] == 1) && (arr[1] == 1) && (arr[7] == 9)),
        "sorts unsorted array correctly",
        "failed to sort unsorted array",
        _test_info
    );

    //=========================================================================
    // Test 2: Sort already sorted array
    //=========================================================================
    arr[0] = 1; arr[1] = 2; arr[2] = 3; arr[3] = 4; arr[4] = 5;
    d_array_common_sort(arr, 5, sizeof(int), d_test_int_comparator);
    test_result &= d_assert_standalone(
        ((arr[0] == 1) && (arr[4] == 5)),
        "handles already sorted array",
        "failed with sorted array",
        _test_info
    );

    //=========================================================================
    // Test 3: Sort reverse-sorted array
    //=========================================================================
    arr[0] = 5; arr[1] = 4; arr[2] = 3; arr[3] = 2; arr[4] = 1;
    d_array_common_sort(arr, 5, sizeof(int), d_test_int_comparator);
    test_result &= d_assert_standalone(
        ((arr[0] == 1) && (arr[4] == 5)),
        "sorts reverse-sorted array correctly",
        "failed with reverse-sorted array",
        _test_info
    );

    //=========================================================================
    // Test 4: Single element (no-op)
    //=========================================================================
    arr[0] = 42;
    d_array_common_sort(arr, 1, sizeof(int), d_test_int_comparator);
    test_result &= d_assert_standalone(
        (arr[0] == 42),
        "handles single element (no-op)",
        "failed with single element",
        _test_info
    );

    _test_info->tests_total++;
    if (test_result) { _test_info->tests_passed++; }

    return test_result;
}


/******************************************************************************
 * MANIPULATION TEST GROUP RUNNER
 *****************************************************************************/

/*
d_tests_sa_array_common_run_manip_tests
  Runs all manipulation function tests.
*/
bool
d_tests_sa_array_common_run_manip_tests
(
    struct d_test_counter* _test_info
)
{
    bool overall_result;

    if (!_test_info)
    {
        return false;
    }

    overall_result = true;

    printf("\n----------------------------------------\n");
    printf("Running Manipulation Function Tests\n");
    printf("----------------------------------------\n");

    overall_result &= d_tests_sa_array_common_insert_element(_test_info);
    overall_result &= d_tests_sa_array_common_insert_elements(_test_info);
    overall_result &= d_tests_sa_array_common_is_valid_resize_amount(_test_info);
    overall_result &= d_tests_sa_array_common_is_valid_resize_factor(_test_info);
    overall_result &= d_tests_sa_array_common_prepend_element(_test_info);
    overall_result &= d_tests_sa_array_common_prepend_elements(_test_info);
    overall_result &= d_tests_sa_array_common_resize_amount(_test_info);
    overall_result &= d_tests_sa_array_common_resize_factor(_test_info);
    overall_result &= d_tests_sa_array_common_reverse(_test_info);
    overall_result &= d_tests_sa_array_common_shift_left(_test_info);
    overall_result &= d_tests_sa_array_common_shift_left_circular(_test_info);
    overall_result &= d_tests_sa_array_common_shift_right(_test_info);
    overall_result &= d_tests_sa_array_common_shift_right_circular(_test_info);
    overall_result &= d_tests_sa_array_common_sort(_test_info);

    return overall_result;
}
