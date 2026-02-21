/******************************************************************************
* djinterp [container]                             array_common_tests_sa_util.c
*
*   Unit tests for `array_common.h` utility functions.
*   Part of the array_common standalone test suite.
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
* path:      \test\container\array\array_common_tests_sa_util.c
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.10.25
******************************************************************************/

#include ".\array_common_tests_sa.h"


/******************************************************************************
 * UTILITY FUNCTION TESTS
 *****************************************************************************/

/*
d_tests_sa_array_common_alloc
  Tests the `d_array_common_alloc` function.
  Tests the following:
  - basic allocation with valid size
  - zero size handling (should return NULL)
  - various element sizes
*/
bool
d_tests_sa_array_common_alloc
(
    struct d_test_counter* _test_info
)
{
    bool  test_result;
    void* result;

    // validate parameters
    if (!_test_info)
    {
        return false;
    }

    test_result = true;
    result      = NULL;

    printf("Testing `d_array_common_alloc`...\n");

    //=========================================================================
    // Test 1: Allocate with valid size (int)
    //   Expected: non-NULL pointer
    //=========================================================================
    result = d_array_common_alloc(sizeof(int));

    test_result &= d_assert_standalone(
        (result != NULL),
        "allocates memory for valid element size (int)",
        "failed with valid element size",
        _test_info
    );

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

    test_result &= d_assert_standalone(
        (result == NULL),
        "returns NULL for zero element size",
        "failed with zero element size",
        _test_info
    );

    //=========================================================================
    // Test 3: Allocate with larger element size (double)
    //   Expected: non-NULL pointer
    //=========================================================================
    result = d_array_common_alloc(sizeof(double));

    test_result &= d_assert_standalone(
        (result != NULL),
        "allocates memory for larger element size (double)",
        "failed with double element size",
        _test_info
    );

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

    test_result &= d_assert_standalone(
        (result != NULL),
        "allocates memory for struct-sized elements",
        "failed with struct size",
        _test_info
    );

    // cleanup
    if (result)
    {
        free(result);
        result = NULL;
    }

    // update test counters
    _test_info->tests_total++;

    if (test_result)
    {
        _test_info->tests_passed++;
    }

    return test_result;
}

/*
d_tests_sa_array_common_append_element
  Tests the `d_array_common_append_element` function.
  Tests the following:
  - appending a single element to dynamically allocated array
  - NULL parameter handling
  - zero element size handling
  Note: This function calls realloc, so arrays MUST be malloc'd.
*/
bool
d_tests_sa_array_common_append_element
(
    struct d_test_counter* _test_info
)
{
    bool   test_result;
    bool   result;
    int*   elements;
    size_t count;
    int    value;

    // validate parameters
    if (!_test_info)
    {
        return false;
    }

    test_result = true;
    result      = false;
    elements    = NULL;
    count       = 0;
    value       = 42;

    printf("Testing `d_array_common_append_element`...\n");

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

        test_result &= d_assert_standalone(
            ( (result == true)    &&
              (count == 4)        &&
              (elements[3] == 42) ),
            "appends single element correctly",
            "failed to append element",
            _test_info
        );

        // cleanup
        free(elements);
        elements = NULL;
    }
    else
    {
        test_result &= d_assert_standalone(
            false,
            "test allocation succeeded",
            "test allocation failed",
            _test_info
        );
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

    test_result &= d_assert_standalone(
        (result == false),
        "handles NULL elements pointer correctly",
        "failed with NULL elements pointer",
        _test_info
    );

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

        test_result &= d_assert_standalone(
            (result == false),
            "handles NULL value correctly",
            "failed with NULL value",
            _test_info
        );

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

        test_result &= d_assert_standalone(
            (result == false),
            "handles zero element size correctly",
            "failed with zero element size",
            _test_info
        );

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

        test_result &= d_assert_standalone(
            ( (result == true)     &&
              (count == 3)         &&
              (elements[0] == 100) &&
              (elements[1] == 200) &&
              (elements[2] == 300) ),
            "handles multiple appends correctly",
            "failed with multiple appends",
            _test_info
        );

        // cleanup
        free(elements);
        elements = NULL;
    }

    // update test counters
    _test_info->tests_total++;

    if (test_result)
    {
        _test_info->tests_passed++;
    }

    return test_result;
}

/*
d_tests_sa_array_common_append_elements
  Tests the `d_array_common_append_elements` function.
  Tests the following:
  - appending multiple elements to dynamically allocated array
  - appending zero elements (should return true as no-op)
  - NULL parameter handling
  Note: This function calls realloc, so arrays MUST be malloc'd.
*/
bool
d_tests_sa_array_common_append_elements
(
    struct d_test_counter* _test_info
)
{
    bool   test_result;
    bool   result;
    int*   elements;
    size_t count;
    int    values[3];

    // validate parameters
    if (!_test_info)
    {
        return false;
    }

    test_result = true;
    result      = false;
    elements    = NULL;
    count       = 0;

    // setup source values
    values[0] = 10;
    values[1] = 20;
    values[2] = 30;

    printf("Testing `d_array_common_append_elements`...\n");

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

        test_result &= d_assert_standalone(
            ( (result == true)    &&
              (count == 6)        &&
              (elements[3] == 10) &&
              (elements[4] == 20) &&
              (elements[5] == 30) ),
            "appends multiple elements correctly",
            "failed to append multiple elements",
            _test_info
        );

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

        test_result &= d_assert_standalone(
            ( (result == true) &&
              (count == 3) ),
            "handles zero count correctly (returns true)",
            "failed with zero count",
            _test_info
        );

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

    test_result &= d_assert_standalone(
        (result == false),
        "handles NULL elements pointer correctly",
        "failed with NULL elements pointer",
        _test_info
    );

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

        test_result &= d_assert_standalone(
            (result == false),
            "handles NULL value correctly",
            "failed with NULL value",
            _test_info
        );

        // cleanup
        free(elements);
        elements = NULL;
    }

    // update test counters
    _test_info->tests_total++;

    if (test_result)
    {
        _test_info->tests_passed++;
    }

    return test_result;
}

/*
d_tests_sa_array_common_calc_capacity
  Tests the `d_array_common_calc_capacity` function.
  Tests the following:
  - calculating capacity for various sizes
  - power-of-2 growth behavior
  - zero size handling
  - large size handling (overflow protection)
*/
bool
d_tests_sa_array_common_calc_capacity
(
    struct d_test_counter* _test_info
)
{
    bool   test_result;
    size_t result;

    // validate parameters
    if (!_test_info)
    {
        return false;
    }

    test_result = true;
    result      = 0;

    printf("Testing `d_array_common_calc_capacity`...\n");

    //=========================================================================
    // Test 1: Calculate capacity for zero
    //   Expected: returns 1 (minimum capacity)
    //=========================================================================
    result = d_array_common_calc_capacity(0);

    test_result &= d_assert_standalone(
        (result == 1),
        "returns 1 for zero request",
        "failed with zero request",
        _test_info
    );

    //=========================================================================
    // Test 2: Calculate capacity for small size (5)
    //   Expected: returns power of 2 >= 5 (likely 8)
    //=========================================================================
    result = d_array_common_calc_capacity(5);

    test_result &= d_assert_standalone(
        ( (result >= 5) &&
          ((result & (result - 1)) == 0) ),  // is power of 2
        "returns power of 2 >= requested for small size",
        "failed with small size",
        _test_info
    );

    //=========================================================================
    // Test 3: Calculate capacity for exact power of 2 (16)
    //   Expected: returns 16 (no growth needed)
    //=========================================================================
    result = d_array_common_calc_capacity(16);

    test_result &= d_assert_standalone(
        (result == 16),
        "returns exact value for power of 2 request",
        "failed with power of 2 request",
        _test_info
    );

    //=========================================================================
    // Test 4: Calculate capacity for size just over power of 2 (17)
    //   Expected: returns 32 (next power of 2)
    //=========================================================================
    result = d_array_common_calc_capacity(17);

    test_result &= d_assert_standalone(
        (result == 32),
        "returns next power of 2 for size just over",
        "failed for size just over power of 2",
        _test_info
    );

    //=========================================================================
    // Test 5: Calculate capacity for 1
    //   Expected: returns 1
    //=========================================================================
    result = d_array_common_calc_capacity(1);

    test_result &= d_assert_standalone(
        (result == 1),
        "returns 1 for size 1",
        "failed with size 1",
        _test_info
    );

    //=========================================================================
    // Test 6: Verify power-of-2 property for various sizes
    //=========================================================================
    result = d_array_common_calc_capacity(100);

    test_result &= d_assert_standalone(
        ( (result >= 100) &&
          ((result & (result - 1)) == 0) ),
        "maintains power-of-2 property for larger requests",
        "failed power-of-2 property check",
        _test_info
    );

    // update test counters
    _test_info->tests_total++;

    if (test_result)
    {
        _test_info->tests_passed++;
    }

    return test_result;
}

/*
d_tests_sa_array_common_contains
  Tests the `d_array_common_contains` function.
  Tests the following:
  - finding existing element
  - not finding non-existent element
  - NULL parameter handling
  - empty array handling
*/
bool
d_tests_sa_array_common_contains
(
    struct d_test_counter* _test_info
)
{
    bool test_result;
    bool result;
    int  elements[5];
    int  search_val;
    int  not_found;

    // validate parameters
    if (!_test_info)
    {
        return false;
    }

    test_result = true;
    result      = false;

    // setup test array (stack allocation is fine - no realloc)
    elements[0] = 10;
    elements[1] = 20;
    elements[2] = 30;
    elements[3] = 40;
    elements[4] = 50;

    printf("Testing `d_array_common_contains`...\n");

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

    test_result &= d_assert_standalone(
        (result == true),
        "returns true for existing element",
        "failed to find existing element",
        _test_info
    );

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

    test_result &= d_assert_standalone(
        (result == false),
        "returns false for non-existent element",
        "incorrectly found non-existent element",
        _test_info
    );

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

    test_result &= d_assert_standalone(
        (result == true),
        "finds first element",
        "failed to find first element",
        _test_info
    );

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

    test_result &= d_assert_standalone(
        (result == true),
        "finds last element",
        "failed to find last element",
        _test_info
    );

    //=========================================================================
    // Test 5: Empty array
    //   Expected: returns false
    //=========================================================================
    result = d_array_common_contains(elements,
                                     0,
                                     sizeof(int),
                                     &search_val,
                                     d_test_int_comparator);

    test_result &= d_assert_standalone(
        (result == false),
        "returns false for empty array",
        "incorrectly found element in empty array",
        _test_info
    );

    // update test counters
    _test_info->tests_total++;

    if (test_result)
    {
        _test_info->tests_passed++;
    }

    return test_result;
}

/*
d_tests_sa_array_common_fill
  Tests the `d_array_common_fill` function.
  Tests the following:
  - filling array with value
  - zero count handling
  - NULL parameter handling
  - return value (number of elements filled)
*/
bool
d_tests_sa_array_common_fill
(
    struct d_test_counter* _test_info
)
{
    bool test_result;
    int  result;
    int  elements[5];
    int  fill_value;

    // validate parameters
    if (!_test_info)
    {
        return false;
    }

    test_result = true;
    result      = 0;
    fill_value  = 99;

    printf("Testing `d_array_common_fill`...\n");

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

    test_result &= d_assert_standalone(
        ( (result == 5)        &&
          (elements[0] == 99)  &&
          (elements[2] == 99)  &&
          (elements[4] == 99) ),
        "fills array and returns correct count",
        "failed to fill array correctly",
        _test_info
    );

    //=========================================================================
    // Test 2: Fill with zero count
    //   Expected: returns 0 (no elements filled)
    //=========================================================================
    result = d_array_common_fill(elements,
                                 0,
                                 sizeof(int),
                                 &fill_value);

    test_result &= d_assert_standalone(
        (result == 0),
        "returns 0 for zero count",
        "failed with zero count",
        _test_info
    );

    //=========================================================================
    // Test 3: Fill with NULL elements
    //   Expected: returns -1 (error)
    //=========================================================================
    result = d_array_common_fill(NULL,
                                 5,
                                 sizeof(int),
                                 &fill_value);

    test_result &= d_assert_standalone(
        (result == -1),
        "returns -1 for NULL elements",
        "failed with NULL elements",
        _test_info
    );

    //=========================================================================
    // Test 4: Fill with NULL value
    //   Expected: returns -1 (error)
    //=========================================================================
    result = d_array_common_fill(elements,
                                 5,
                                 sizeof(int),
                                 NULL);

    test_result &= d_assert_standalone(
        (result == -1),
        "returns -1 for NULL value",
        "failed with NULL value",
        _test_info
    );

    //=========================================================================
    // Test 5: Fill with zero element size
    //   Expected: returns -1 (error)
    //=========================================================================
    result = d_array_common_fill(elements,
                                 5,
                                 0,
                                 &fill_value);

    test_result &= d_assert_standalone(
        (result == -1),
        "returns -1 for zero element size",
        "failed with zero element size",
        _test_info
    );

    // update test counters
    _test_info->tests_total++;

    if (test_result)
    {
        _test_info->tests_passed++;
    }

    return test_result;
}

/*
d_tests_sa_array_common_find
  Tests the `d_array_common_find` function.
  Tests the following:
  - finding existing element (returns index)
  - not finding non-existent element (returns -1)
  - finding first occurrence with duplicates
  - NULL parameter handling
*/
bool
d_tests_sa_array_common_find
(
    struct d_test_counter* _test_info
)
{
    bool    test_result;
    ssize_t result;
    int     elements[5];
    int     dup_elements[5];
    int     search_val;
    int     not_found;

    // validate parameters
    if (!_test_info)
    {
        return false;
    }

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

    printf("Testing `d_array_common_find`...\n");

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

    test_result &= d_assert_standalone(
        (result == 2),
        "returns correct index for existing element",
        "returned wrong index",
        _test_info
    );

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

    test_result &= d_assert_standalone(
        (result == -1),
        "returns -1 for missing element",
        "failed to return -1 for missing element",
        _test_info
    );

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

    test_result &= d_assert_standalone(
        (result == 1),
        "returns first occurrence with duplicates",
        "failed to return first occurrence",
        _test_info
    );

    //=========================================================================
    // Test 4: Test with NULL comparator
    //   Expected: returns -1
    //=========================================================================
    result = d_array_common_find(elements,
                                 5,
                                 sizeof(int),
                                 &search_val,
                                 NULL);

    test_result &= d_assert_standalone(
        (result == -1),
        "returns -1 for NULL comparator",
        "failed with NULL comparator",
        _test_info
    );

    //=========================================================================
    // Test 5: Test with NULL elements
    //   Expected: returns -1
    //=========================================================================
    result = d_array_common_find(NULL,
                                 5,
                                 sizeof(int),
                                 &search_val,
                                 d_test_int_comparator);

    test_result &= d_assert_standalone(
        (result == -1),
        "returns -1 for NULL elements",
        "failed with NULL elements",
        _test_info
    );

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

    test_result &= d_assert_standalone(
        (result == 0),
        "returns 0 for first element",
        "failed to find first element",
        _test_info
    );

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

    test_result &= d_assert_standalone(
        (result == 4),
        "returns correct index for last element",
        "failed to find last element",
        _test_info
    );

    // update test counters
    _test_info->tests_total++;

    if (test_result)
    {
        _test_info->tests_passed++;
    }

    return test_result;
}

/*
d_tests_sa_array_common_find_closest
  Tests the `d_array_common_find_closest` function.
  Tests the following:
  - finding exact match
  - finding closest element in sorted array
  - boundary cases (below/above range)
  - NULL parameter handling
  - empty array handling
  Note: Array must be sorted for meaningful results.
*/
bool
d_tests_sa_array_common_find_closest
(
    struct d_test_counter* _test_info
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
    if (!_test_info)
    {
        return false;
    }

    test_result = true;
    result      = 0;

    // setup sorted test array
    elements[0] = 10;
    elements[1] = 20;
    elements[2] = 30;
    elements[3] = 40;
    elements[4] = 50;

    printf("Testing `d_array_common_find_closest`...\n");

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

    test_result &= d_assert_standalone(
        (result == 2),
        "finds exact match",
        "failed to find exact match",
        _test_info
    );

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

    test_result &= d_assert_standalone(
        ( (result == 1) || (result == 2) ),
        "returns closest index for between value",
        "failed for between value",
        _test_info
    );

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

    test_result &= d_assert_standalone(
        (result == 0),
        "handles below-range value correctly",
        "failed for below-range value",
        _test_info
    );

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

    test_result &= d_assert_standalone(
        (result == 4),
        "handles above-range value correctly",
        "failed for above-range value",
        _test_info
    );

    //=========================================================================
    // Test 5: Test with empty array
    //   Expected: returns -1
    //=========================================================================
    result = d_array_common_find_closest(elements,
                                         0,
                                         sizeof(int),
                                         &exact,
                                         d_test_int_comparator);

    test_result &= d_assert_standalone(
        (result == -1),
        "returns -1 for empty array",
        "failed with empty array",
        _test_info
    );

    //=========================================================================
    // Test 6: Test with NULL elements
    //   Expected: returns -1
    //=========================================================================
    result = d_array_common_find_closest(NULL,
                                         5,
                                         sizeof(int),
                                         &exact,
                                         d_test_int_comparator);

    test_result &= d_assert_standalone(
        (result == -1),
        "returns -1 for NULL elements",
        "failed with NULL elements",
        _test_info
    );

    //=========================================================================
    // Test 7: Test with single element array
    //   Expected: returns 0 (the only option)
    //=========================================================================
    result = d_array_common_find_closest(elements,
                                         1,
                                         sizeof(int),
                                         &above,
                                         d_test_int_comparator);

    test_result &= d_assert_standalone(
        (result == 0),
        "handles single element array",
        "failed with single element array",
        _test_info
    );

    // update test counters
    _test_info->tests_total++;

    if (test_result)
    {
        _test_info->tests_passed++;
    }

    return test_result;
}


/******************************************************************************
 * UTILITY TEST GROUP RUNNER
 *****************************************************************************/

/*
d_tests_sa_array_common_run_util_tests
  Runs all utility function tests and returns overall pass/fail status.

Parameter(s):
  _test_info: pointer to test counter for tracking results
Return:
  A boolean value corresponding to either:
  - true, if all utility tests passed, or
  - false, if any test failed.
*/
bool
d_tests_sa_array_common_run_util_tests
(
    struct d_test_counter* _test_info
)
{
    bool overall_result;

    // validate parameters
    if (!_test_info)
    {
        return false;
    }

    overall_result = true;

    printf("\n----------------------------------------\n");
    printf("Running Utility Function Tests\n");
    printf("----------------------------------------\n");

    // run all utility tests in header order
    overall_result &= d_tests_sa_array_common_alloc(_test_info);
    overall_result &= d_tests_sa_array_common_append_element(_test_info);
    overall_result &= d_tests_sa_array_common_append_elements(_test_info);
    overall_result &= d_tests_sa_array_common_calc_capacity(_test_info);
    overall_result &= d_tests_sa_array_common_contains(_test_info);
    overall_result &= d_tests_sa_array_common_fill(_test_info);
    overall_result &= d_tests_sa_array_common_find(_test_info);
    overall_result &= d_tests_sa_array_common_find_closest(_test_info);

    return overall_result;
}
