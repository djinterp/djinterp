/*******************************************************************************
* djinterp [container]                                 array_common_tests_util.c
*
*   Unit tests for `array_common.h` utility functions.
*   Part of the array_common suite, on DTest's C face.
*
*   This file tests the following utility functions:
*   - d_array_common_alloc
*   - d_array_common_append_element
*   - d_array_common_append_elements
*   - d_array_common_calc_capacity
*   - d_array_common_contains
*   - d_array_common_fill
*   - d_array_common_find
*   - d_array_common_find_closest
*
*   Each test function covers:
*   - Normal operation with valid inputs
*   - Edge cases (zero size, empty arrays, boundary conditions)
*   - Invalid input handling (NULL pointers, invalid parameters)
*
*   IMPORTANT: Functions that call realloc (append_element, append_elements)
*   require dynamically allocated arrays, not stack-allocated arrays.
*
*
* path:      /tests/djinterp/c/container/array/array_common_tests_util.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.10.25
*                                                            revised: 2026.11.09
*******************************************************************************/

// djinterp
#include "./array_common_tests.h"
#include "../../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's C face


/******************************************************************************
 * UTILITY FUNCTION TESTS
 *****************************************************************************/

/*
d_tests_array_common_alloc
  Tests the `d_array_common_alloc` function.
  Tests the following:
  - basic allocation with valid size
  - zero size handling (should return NULL)
  - various element sizes
*/
bool d_tests_array_common_alloc(
    struct dj_test_suite* _suite
)
{
    bool  test_result;
    void* result;

    // validate parameters
    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "array_common_alloc");

    test_result = true;
    result      = NULL;

    //=========================================================================
    // Test 1: Allocate with valid size (int)
    //   Expected: non-NULL pointer
    //=========================================================================
    result = d_array_common_alloc(sizeof(int));

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "allocates memory for valid element size (int)",
                                    (result != NULL),
                                    "failed with valid element size");

    // cleanup
    if (result)
    {
        free(result);
        result = NULL;
    }

    //=========================================================================
    // Test 2: Allocate with zero size
    //   Expected: NULL pointer (zero-size allocation is invalid)
    //=========================================================================
    result = d_array_common_alloc(0);

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "returns NULL for zero element size",
                                    (result == NULL),
                                    "failed with zero element size");

    //=========================================================================
    // Test 3: Allocate with larger element size (double)
    //   Expected: non-NULL pointer
    //=========================================================================
    result = d_array_common_alloc(sizeof(double));

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "allocates memory for larger element size (double)",
                                    (result != NULL),
                                    "failed with double element size");

    // cleanup
    if (result)
    {
        free(result);
        result = NULL;
    }

    //=========================================================================
    // Test 4: Allocate with struct size
    //   Expected: non-NULL pointer for compound types
    //=========================================================================
    result = d_array_common_alloc(D_TEST_AC_STRUCT_SIZE);

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "allocates memory for struct-sized elements",
                                    (result != NULL),
                                    "failed with struct size");

    // cleanup
    if (result)
    {
        free(result);
        result = NULL;
    }

    return test_result;
}

/*
d_tests_array_common_append_element
  Tests the `d_array_common_append_element` function.
  Tests the following:
  - appending a single element to dynamically allocated array
  - NULL parameter handling
  - zero element size handling
  Note: This function calls realloc, so arrays MUST be malloc'd.
*/
bool d_tests_array_common_append_element(
    struct dj_test_suite* _suite
)
{
    bool   test_result;
    bool   result;
    int*   elements;
    size_t count;
    int    value;

    // validate parameters
    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "array_common_append_element");

    test_result = true;
    result      = false;
    elements    = NULL;
    count       = 0;
    value       = 42;

    //=========================================================================
    // Test 1: Append element to dynamically allocated array
    //   Array must be malloc'd for realloc to work
    //=========================================================================
    elements = (int*)malloc(3 * sizeof(int));

    if (elements)
    {
        elements[0] = 1;
        elements[1] = 2;
        elements[2] = 3;
        count       = 3;

        result = d_array_common_append_element((void**)&elements,
                                               &count,
                                               sizeof(int),
                                               &value);

        test_result &= D_TEST_CHECK_MSG(_suite,
                                        "appends single element correctly",
                                        ( (result == true) && (count == 4) && (elements[3] == 42) ),
                                        "failed to append element");

        // cleanup
        free(elements);
        elements = NULL;
    }
    else
    {
        test_result &= D_TEST_CHECK_MSG(_suite,
                                        "test allocation succeeded",
                                        false,
                                        "test allocation failed");
    }

    //=========================================================================
    // Test 2: Test with NULL elements pointer
    //   Expected: failure
    //=========================================================================
    count  = 3;
    result = d_array_common_append_element(NULL,
                                           &count,
                                           sizeof(int),
                                           &value);

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "handles NULL elements pointer correctly",
                                    (result == false),
                                    "failed with NULL elements pointer");

    //=========================================================================
    // Test 3: Test with NULL value
    //   Expected: failure
    //=========================================================================
    elements = (int*)malloc(3 * sizeof(int));

    if (elements)
    {
        count  = 3;
        result = d_array_common_append_element((void**)&elements,
                                               &count,
                                               sizeof(int),
                                               NULL);

        test_result &= D_TEST_CHECK_MSG(_suite,
                                        "handles NULL value correctly",
                                        (result == false),
                                        "failed with NULL value");

        // cleanup
        free(elements);
        elements = NULL;
    }

    //=========================================================================
    // Test 4: Test with zero element size
    //   Expected: failure
    //=========================================================================
    elements = (int*)malloc(3 * sizeof(int));

    if (elements)
    {
        count  = 3;
        result = d_array_common_append_element((void**)&elements,
                                               &count,
                                               0,
                                               &value);

        test_result &= D_TEST_CHECK_MSG(_suite,
                                        "handles zero element size correctly",
                                        (result == false),
                                        "failed with zero element size");

        // cleanup
        free(elements);
        elements = NULL;
    }

    //=========================================================================
    // Test 5: Append multiple times to verify growth
    //=========================================================================
    elements = (int*)malloc(1 * sizeof(int));

    if (elements)
    {
        elements[0] = 100;
        count       = 1;

        value  = 200;
        result = d_array_common_append_element((void**)&elements,
                                               &count,
                                               sizeof(int),
                                               &value);

        value = 300;
        result &= d_array_common_append_element((void**)&elements,
                                                &count,
                                                sizeof(int),
                                                &value);

        test_result &= D_TEST_CHECK_MSG(_suite,
                                        "handles multiple appends correctly",
                                        ( (result == true) && (count == 3) && (elements[0] == 100) && (elements[1] == 200) && (elements[2] == 300) ),
                                        "failed with multiple appends");

        // cleanup
        free(elements);
        elements = NULL;
    }

    return test_result;
}

/**
 * @brief Part 1 of d_tests_array_common_append_elements.
 *
 * @param[in,out] _suite  the suite the checks are recorded in.
 * @return true if every check of the part passed.
 *
 * The test runs its parts in order, so that no function body is over
 * 100 lines. A part declares its own locals: no value crosses from one
 * part to the next.
 *
 * Sections:
 *   - setup source values
 *   - Test 1: Append multiple elements
 *   - Test 2: Append zero elements
 *   - Test 3: Test with NULL elements pointer
 */
static bool
d_internal_tests_array_common_append_elements_part1(
    struct dj_test_suite* _suite
)
{
    bool   test_result;
    bool   result;
    int*   elements;
    size_t count;
    int    values[3];

    test_result = true;
    result      = false;
    elements    = NULL;
    count       = 0;

    // setup source values
    values[0] = 10;
    values[1] = 20;
    values[2] = 30;

    //=========================================================================
    // Test 1: Append multiple elements
    //   Array: [1, 2, 3] -> append [10, 20, 30] -> [1, 2, 3, 10, 20, 30]
    //=========================================================================
    elements = (int*)malloc(3 * sizeof(int));

    if (elements)
    {
        elements[0] = 1;
        elements[1] = 2;
        elements[2] = 3;
        count       = 3;

        result = d_array_common_append_elements((void**)&elements,
                                                &count,
                                                sizeof(int),
                                                values,
                                                3);

        test_result &= D_TEST_CHECK_MSG(_suite,
                                        "appends multiple elements correctly",
                                        ( (result == true) && (count == 6) && (elements[3] == 10) && (elements[4] == 20) && (elements[5] == 30) ),
                                        "failed to append multiple elements");

        // cleanup
        free(elements);
        elements = NULL;
    }

    //=========================================================================
    // Test 2: Append zero elements
    //   Expected: success (no-op), count unchanged
    //=========================================================================
    elements = (int*)malloc(3 * sizeof(int));

    if (elements)
    {
        elements[0] = 1;
        elements[1] = 2;
        elements[2] = 3;
        count       = 3;

        result = d_array_common_append_elements((void**)&elements,
                                                &count,
                                                sizeof(int),
                                                values,
                                                0);

        test_result &= D_TEST_CHECK_MSG(_suite,
                                        "handles zero count correctly (returns true)",
                                        ( (result == true) && (count == 3) ),
                                        "failed with zero count");

        // cleanup
        free(elements);
        elements = NULL;
    }

    //=========================================================================
    // Test 3: Test with NULL elements pointer
    //   Expected: failure
    //=========================================================================
    count  = 3;
    result = d_array_common_append_elements(NULL,
                                            &count,
                                            sizeof(int),
                                            values,
                                            3);

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "handles NULL elements pointer correctly",
                                    (result == false),
                                    "failed with NULL elements pointer");

    return test_result;
}


/**
 * @brief Part 2 of d_tests_array_common_append_elements.
 *
 * @param[in,out] _suite  the suite the checks are recorded in.
 * @return true if every check of the part passed.
 *
 * The test runs its parts in order, so that no function body is over
 * 100 lines. A part declares its own locals: no value crosses from one
 * part to the next.
 *
 * Sections:
 *   - Test 4: Test with NULL value source (when count > 0)
 */
static bool
d_internal_tests_array_common_append_elements_part2(
    struct dj_test_suite* _suite
)
{
    bool   test_result;
    bool   result;
    int*   elements;
    size_t count;

    test_result = true;
    result      = false;
    elements    = NULL;
    count       = 0;

    //=========================================================================
    // Test 4: Test with NULL value source (when count > 0)
    //   Expected: failure
    //=========================================================================
    elements = (int*)malloc(3 * sizeof(int));

    if (elements)
    {
        count  = 3;
        result = d_array_common_append_elements((void**)&elements,
                                                &count,
                                                sizeof(int),
                                                NULL,
                                                3);

        test_result &= D_TEST_CHECK_MSG(_suite,
                                        "handles NULL value correctly",
                                        (result == false),
                                        "failed with NULL value");

        // cleanup
        free(elements);
        elements = NULL;
    }

    return test_result;
}


/*
d_tests_array_common_append_elements
  Tests the `d_array_common_append_elements` function.
  Tests the following:
  - appending multiple elements to dynamically allocated array
  - appending zero elements (should return true as no-op)
  - NULL parameter handling
  Note: This function calls realloc, so arrays MUST be malloc'd.
*/
bool d_tests_array_common_append_elements(
    struct dj_test_suite* _suite
)
{
    bool test_result;

    // validate parameters
    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "array_common_append_elements");

    test_result = true;
    test_result &= d_internal_tests_array_common_append_elements_part1(_suite);
    test_result &= d_internal_tests_array_common_append_elements_part2(_suite);

    return test_result;
}

/*
d_tests_array_common_calc_capacity
  Tests the `d_array_common_calc_capacity` function.
  Tests the following:
  - calculating capacity for various sizes
  - power-of-2 growth behavior
  - zero size handling
  - large size handling (overflow protection)
*/
bool d_tests_array_common_calc_capacity(
    struct dj_test_suite* _suite
)
{
    bool   test_result;
    size_t result;

    // validate parameters
    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "array_common_calc_capacity");

    test_result = true;
    result      = 0;

    //=========================================================================
    // Test 1: Calculate capacity for zero
    //   Expected: returns 1 (minimum capacity)
    //=========================================================================
    result = d_array_common_calc_capacity(0);

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "returns 1 for zero request",
                                    (result == 1),
                                    "failed with zero request");

    //=========================================================================
    // Test 2: Calculate capacity for small size (5)
    //   Expected: returns power of 2 >= 5 (likely 8)
    //=========================================================================
    result = d_array_common_calc_capacity(5);

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "returns power of 2 >= requested for small size",
                                    // is power of 2
                                    ( (result >= 5) && ((result & (result - 1)) == 0) ),
                                    "failed with small size");

    //=========================================================================
    // Test 3: Calculate capacity for exact power of 2 (16)
    //   Expected: returns 16 (no growth needed)
    //=========================================================================
    result = d_array_common_calc_capacity(16);

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "returns exact value for power of 2 request",
                                    (result == 16),
                                    "failed with power of 2 request");

    //=========================================================================
    // Test 4: Calculate capacity for size just over power of 2 (17)
    //   Expected: returns 32 (next power of 2)
    //=========================================================================
    result = d_array_common_calc_capacity(17);

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "returns next power of 2 for size just over",
                                    (result == 32),
                                    "failed for size just over power of 2");

    //=========================================================================
    // Test 5: Calculate capacity for 1
    //   Expected: returns 1
    //=========================================================================
    result = d_array_common_calc_capacity(1);

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "returns 1 for size 1",
                                    (result == 1),
                                    "failed with size 1");

    //=========================================================================
    // Test 6: Verify power-of-2 property for various sizes
    //=========================================================================
    result = d_array_common_calc_capacity(100);

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "maintains power-of-2 property for larger requests",
                                    ( (result >= 100) && ((result & (result - 1)) == 0) ),
                                    "failed power-of-2 property check");

    return test_result;
}

/*
d_tests_array_common_contains
  Tests the `d_array_common_contains` function.
  Tests the following:
  - finding existing element
  - not finding non-existent element
  - NULL parameter handling
  - empty array handling
*/
bool d_tests_array_common_contains(
    struct dj_test_suite* _suite
)
{
    bool test_result;
    bool result;
    int  elements[5];
    int  search_val;
    int  not_found;

    // validate parameters
    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "array_common_contains");

    test_result = true;
    result      = false;

    // setup test array (stack allocation is fine - no realloc)
    elements[0] = 10;
    elements[1] = 20;
    elements[2] = 30;
    elements[3] = 40;
    elements[4] = 50;

    //=========================================================================
    // Test 1: Find existing element
    //   Expected: returns true
    //=========================================================================
    search_val = 30;

    result = d_array_common_contains(elements,
                                     5,
                                     sizeof(int),
                                     &search_val,
                                     d_test_int_comparator);

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "returns true for existing element",
                                    (result == true),
                                    "failed to find existing element");

    //=========================================================================
    // Test 2: Not find non-existent element
    //   Expected: returns false
    //=========================================================================
    not_found = 99;

    result = d_array_common_contains(elements,
                                     5,
                                     sizeof(int),
                                     &not_found,
                                     d_test_int_comparator);

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "returns false for non-existent element",
                                    (result == false),
                                    "incorrectly found non-existent element");

    //=========================================================================
    // Test 3: First element
    //   Expected: returns true
    //=========================================================================
    search_val = 10;

    result = d_array_common_contains(elements,
                                     5,
                                     sizeof(int),
                                     &search_val,
                                     d_test_int_comparator);

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "finds first element",
                                    (result == true),
                                    "failed to find first element");

    //=========================================================================
    // Test 4: Last element
    //   Expected: returns true
    //=========================================================================
    search_val = 50;

    result = d_array_common_contains(elements,
                                     5,
                                     sizeof(int),
                                     &search_val,
                                     d_test_int_comparator);

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "finds last element",
                                    (result == true),
                                    "failed to find last element");

    //=========================================================================
    // Test 5: Empty array
    //   Expected: returns false
    //=========================================================================
    result = d_array_common_contains(elements,
                                     0,
                                     sizeof(int),
                                     &search_val,
                                     d_test_int_comparator);

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "returns false for empty array",
                                    (result == false),
                                    "incorrectly found element in empty array");

    return test_result;
}

/*
d_tests_array_common_fill
  Tests the `d_array_common_fill` function.
  Tests the following:
  - filling array with value
  - zero count handling
  - NULL parameter handling
  - return value (number of elements filled)
*/
bool d_tests_array_common_fill(
    struct dj_test_suite* _suite
)
{
    bool test_result;
    int  result;
    int  elements[5];
    int  fill_value;

    // validate parameters
    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "array_common_fill");

    test_result = true;
    result      = 0;
    fill_value  = 99;

    //=========================================================================
    // Test 1: Fill array with value
    //   Expected: all elements set to fill_value, returns count
    //=========================================================================
    elements[0] = 0;
    elements[1] = 0;
    elements[2] = 0;
    elements[3] = 0;
    elements[4] = 0;

    result = d_array_common_fill(elements,
                                 5,
                                 sizeof(int),
                                 &fill_value);

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "fills array and returns correct count",
                                    ( (result == 5) && (elements[0] == 99) && (elements[2] == 99) && (elements[4] == 99) ),
                                    "failed to fill array correctly");

    //=========================================================================
    // Test 2: Fill with zero count
    //   Expected: returns 0 (no elements filled)
    //=========================================================================
    result = d_array_common_fill(elements,
                                 0,
                                 sizeof(int),
                                 &fill_value);

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "returns 0 for zero count",
                                    (result == 0),
                                    "failed with zero count");

    //=========================================================================
    // Test 3: Fill with NULL elements
    //   Expected: returns -1 (error)
    //=========================================================================
    result = d_array_common_fill(NULL,
                                 5,
                                 sizeof(int),
                                 &fill_value);

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "returns -1 for NULL elements",
                                    (result == -1),
                                    "failed with NULL elements");

    //=========================================================================
    // Test 4: Fill with NULL value
    //   Expected: returns -1 (error)
    //=========================================================================
    result = d_array_common_fill(elements,
                                 5,
                                 sizeof(int),
                                 NULL);

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "returns -1 for NULL value",
                                    (result == -1),
                                    "failed with NULL value");

    //=========================================================================
    // Test 5: Fill with zero element size
    //   Expected: returns -1 (error)
    //=========================================================================
    result = d_array_common_fill(elements,
                                 5,
                                 0,
                                 &fill_value);

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "returns -1 for zero element size",
                                    (result == -1),
                                    "failed with zero element size");

    return test_result;
}

/*
d_tests_array_common_find
  Tests the `d_array_common_find` function.
  Tests the following:
  - finding existing element (returns index)
  - not finding non-existent element (returns -1)
  - finding first occurrence with duplicates
  - NULL parameter handling
*/
bool d_tests_array_common_find(
    struct dj_test_suite* _suite
)
{
    bool    test_result;
    ssize_t result;
    int     elements[5];
    int     dup_elements[5];
    int     search_val;
    int     not_found;

    // validate parameters
    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "array_common_find");

    test_result = true;
    result      = 0;

    // setup test arrays
    elements[0] = 10;
    elements[1] = 20;
    elements[2] = 30;
    elements[3] = 40;
    elements[4] = 50;

    dup_elements[0] = 1;
    dup_elements[1] = 2;
    dup_elements[2] = 3;
    dup_elements[3] = 2;
    dup_elements[4] = 4;

    //=========================================================================
    // Test 1: Find existing element
    //   Expected: returns index 2 for value 30
    //=========================================================================
    search_val = 30;

    result = d_array_common_find(elements,
                                 5,
                                 sizeof(int),
                                 &search_val,
                                 d_test_int_comparator);

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "returns correct index for existing element",
                                    (result == 2),
                                    "returned wrong index");

    //=========================================================================
    // Test 2: Not find non-existent element
    //   Expected: returns -1
    //=========================================================================
    not_found = 99;

    result = d_array_common_find(elements,
                                 5,
                                 sizeof(int),
                                 &not_found,
                                 d_test_int_comparator);

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "returns -1 for missing element",
                                    (result == -1),
                                    "failed to return -1 for missing element");

    //=========================================================================
    // Test 3: Find first occurrence with duplicates
    //   Expected: returns index 1 (first occurrence of 2)
    //=========================================================================
    search_val = 2;

    result = d_array_common_find(dup_elements,
                                 5,
                                 sizeof(int),
                                 &search_val,
                                 d_test_int_comparator);

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "returns first occurrence with duplicates",
                                    (result == 1),
                                    "failed to return first occurrence");

    //=========================================================================
    // Test 4: Test with NULL comparator
    //   Expected: returns -1
    //=========================================================================
    result = d_array_common_find(elements,
                                 5,
                                 sizeof(int),
                                 &search_val,
                                 NULL);

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "returns -1 for NULL comparator",
                                    (result == -1),
                                    "failed with NULL comparator");

    //=========================================================================
    // Test 5: Test with NULL elements
    //   Expected: returns -1
    //=========================================================================
    result = d_array_common_find(NULL,
                                 5,
                                 sizeof(int),
                                 &search_val,
                                 d_test_int_comparator);

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "returns -1 for NULL elements",
                                    (result == -1),
                                    "failed with NULL elements");

    //=========================================================================
    // Test 6: Find first element
    //   Expected: returns index 0
    //=========================================================================
    search_val = 10;

    result = d_array_common_find(elements,
                                 5,
                                 sizeof(int),
                                 &search_val,
                                 d_test_int_comparator);

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "returns 0 for first element",
                                    (result == 0),
                                    "failed to find first element");

    //=========================================================================
    // Test 7: Find last element
    //   Expected: returns index 4
    //=========================================================================
    search_val = 50;

    result = d_array_common_find(elements,
                                 5,
                                 sizeof(int),
                                 &search_val,
                                 d_test_int_comparator);

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "returns correct index for last element",
                                    (result == 4),
                                    "failed to find last element");

    return test_result;
}

/*
d_tests_array_common_find_closest
  Tests the `d_array_common_find_closest` function.
  Tests the following:
  - finding exact match
  - finding closest element in sorted array
  - boundary cases (below/above range)
  - NULL parameter handling
  - empty array handling
  Note: Array must be sorted for meaningful results.
*/
bool d_tests_array_common_find_closest(
    struct dj_test_suite* _suite
)
{
    bool    test_result;
    ssize_t result;
    int     elements[5];
    int     exact;
    int     between;
    int     below;
    int     above;

    // validate parameters
    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "array_common_find_closest");

    test_result = true;
    result      = 0;

    // setup sorted test array
    elements[0] = 10;
    elements[1] = 20;
    elements[2] = 30;
    elements[3] = 40;
    elements[4] = 50;

    //=========================================================================
    // Test 1: Find exact match
    //   Expected: returns index 2 for value 30
    //=========================================================================
    exact = 30;

    result = d_array_common_find_closest(elements,
                                         5,
                                         sizeof(int),
                                         &exact,
                                         d_test_int_comparator);

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "finds exact match",
                                    (result == 2),
                                    "failed to find exact match");

    //=========================================================================
    // Test 2: Find closest for value between elements (25)
    //   Expected: returns index of 20 or 30 (index 1 or 2)
    //=========================================================================
    between = 25;

    result = d_array_common_find_closest(elements,
                                         5,
                                         sizeof(int),
                                         &between,
                                         d_test_int_comparator);

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "returns closest index for between value",
                                    ( (result == 1) || (result == 2) ),
                                    "failed for between value");

    //=========================================================================
    // Test 3: Find closest for value below all (5)
    //   Expected: returns first element index (0)
    //=========================================================================
    below = 5;

    result = d_array_common_find_closest(elements,
                                         5,
                                         sizeof(int),
                                         &below,
                                         d_test_int_comparator);

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "handles below-range value correctly",
                                    (result == 0),
                                    "failed for below-range value");

    //=========================================================================
    // Test 4: Find closest for value above all (55)
    //   Expected: returns last element index (4)
    //=========================================================================
    above = 55;

    result = d_array_common_find_closest(elements,
                                         5,
                                         sizeof(int),
                                         &above,
                                         d_test_int_comparator);

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "handles above-range value correctly",
                                    (result == 4),
                                    "failed for above-range value");

    //=========================================================================
    // Test 5: Test with empty array
    //   Expected: returns -1
    //=========================================================================
    result = d_array_common_find_closest(elements,
                                         0,
                                         sizeof(int),
                                         &exact,
                                         d_test_int_comparator);

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "returns -1 for empty array",
                                    (result == -1),
                                    "failed with empty array");

    //=========================================================================
    // Test 6: Test with NULL elements
    //   Expected: returns -1
    //=========================================================================
    result = d_array_common_find_closest(NULL,
                                         5,
                                         sizeof(int),
                                         &exact,
                                         d_test_int_comparator);

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "returns -1 for NULL elements",
                                    (result == -1),
                                    "failed with NULL elements");

    //=========================================================================
    // Test 7: Test with single element array
    //   Expected: returns 0 (the only option)
    //=========================================================================
    result = d_array_common_find_closest(elements,
                                         1,
                                         sizeof(int),
                                         &above,
                                         d_test_int_comparator);

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "handles single element array",
                                    (result == 0),
                                    "failed with single element array");

    return test_result;
}


/******************************************************************************
 * UTILITY TEST GROUP RUNNER
 *****************************************************************************/

/*
d_tests_array_common_run_util_tests
  Runs all utility function tests and returns overall pass/fail status.

Parameter(s):
  _suite: pointer to test counter for tracking results
Return:
  A boolean value corresponding to either:
  - true, if all utility tests passed, or
  - false, if any test failed.
*/
bool d_tests_array_common_run_util_tests(
    struct dj_test_suite* _suite
)
{
    bool overall_result;

    // validate parameters
    if (!_suite)
    {
        return false;
    }

    d_test_suite_block(_suite, "array_common_run_util_tests");

    overall_result = true;

    printf("\n----------------------------------------\n");
    printf("Running Utility Function Tests\n");

    // run all utility tests in header order
    overall_result &= d_tests_array_common_alloc(_suite);
    overall_result &= d_tests_array_common_append_element(_suite);
    overall_result &= d_tests_array_common_append_elements(_suite);
    overall_result &= d_tests_array_common_calc_capacity(_suite);
    overall_result &= d_tests_array_common_contains(_suite);
    overall_result &= d_tests_array_common_fill(_suite);
    overall_result &= d_tests_array_common_find(_suite);
    overall_result &= d_tests_array_common_find_closest(_suite);

    return overall_result;
}
