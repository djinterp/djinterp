/*******************************************************************************
* djinterp [container]                                 array_common_tests_init.c
*
*   Unit tests for `array_common.h` initialization functions.
*   Part of the array_common suite, on DTest's C face.
*
*   This file tests the following initialization functions:
*   - d_array_common_init_sized
*   - d_array_common_init_from_array
*   - d_array_common_init_from_args
*   - d_array_common_init_copy
*   - d_array_common_init_copy_reverse
*   - d_array_common_init_copy_range
*   - d_array_common_init_copy_range_reverse
*   - d_array_common_init_fill
*   - d_array_common_init_slice
*   - d_array_common_init_slice_reverse
*   - d_array_common_init_slice_range
*   - d_array_common_init_slice_range_reverse
*
*   Each test function covers:
*   - Normal operation with valid inputs
*   - Edge cases (zero size, empty arrays, boundary conditions)
*   - Invalid input handling (NULL pointers, invalid parameters)
*   - dj_index negative indexing support where applicable
*
*
* path:      /tests/djinterp/c/container/array/array_common_tests_init.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.10.25
*                                                            revised: 2026.11.09
*******************************************************************************/

// djinterp
#include "./array_common_tests.h"
#include "../../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's C face


/******************************************************************************
 * INITIALIZATION FUNCTION TESTS
 *****************************************************************************/

/**
 * @brief Part 1 of d_tests_array_common_init_sized.
 *
 * @param[in,out] _suite  the suite the checks are recorded in.
 * @return true if every check of the part passed.
 *
 * The test runs its parts in order, so that no function body is over
 * 100 lines. A part declares its own locals: no value crosses from one
 * part to the next.
 *
 * Sections:
 *   - Test 1: Initialize with zero size
 *   - Test 2: Initialize with valid size using integers
 *   - cleanup
 *   - Test 3: Initialize with different element size (double)
 *   - cleanup
 */
static bool
d_internal_tests_array_common_init_sized_part1(
    struct dj_test_suite* _suite
)
{
    bool   test_result;
    bool   result;
    void*  elements;
    size_t count;

    test_result = true;
    result      = false;
    elements    = NULL;
    count       = SIZE_MAX;

    //=========================================================================
    // Test 1: Initialize with zero size
    //   Expected: success with NULL elements and count = 0
    //=========================================================================
    elements = NULL;
    count    = SIZE_MAX;

    result = d_array_common_init_sized(&elements,
                                       &count,
                                       sizeof(int),
                                       0);

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "zero size initialization returns NULL elements and count = 0",
                                    ( (result == true) && (elements == NULL) && (count == 0) ),
                                    "failed to handle zero size correctly");

    //=========================================================================
    // Test 2: Initialize with valid size using integers
    //   Expected: success with non-NULL elements and count = 0 (empty array)
    //   Note: count starts at 0 because no elements are added yet, only space
    //   is allocated
    //=========================================================================
    elements = NULL;
    count    = SIZE_MAX;

    result = d_array_common_init_sized(&elements,
                                       &count,
                                       sizeof(int),
                                       10);

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "valid size allocates memory with count initialized to 0",
                                    ( (result == true) && (elements != NULL) && (count == 0) ),
                                    "failed to allocate memory for valid size");

    // cleanup
    if (elements)
    {
        free(elements);
        elements = NULL;
    }

    //=========================================================================
    // Test 3: Initialize with different element size (double)
    //   Expected: success with proper allocation for larger element size
    //=========================================================================
    elements = NULL;
    count    = SIZE_MAX;

    result = d_array_common_init_sized(&elements,
                                       &count,
                                       sizeof(double),
                                       5);

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "different element sizes (double) are handled correctly",
                                    ( (result == true) && (elements != NULL) && (count == 0) ),
                                    "failed with different element size (double)");

    // cleanup
    if (elements)
    {
        free(elements);
        elements = NULL;
    }

    return test_result;
}


/**
 * @brief Part 2 of d_tests_array_common_init_sized.
 *
 * @param[in,out] _suite  the suite the checks are recorded in.
 * @return true if every check of the part passed.
 *
 * The test runs its parts in order, so that no function body is over
 * 100 lines. A part declares its own locals: no value crosses from one
 * part to the next.
 *
 * Sections:
 *   - Test 4: Initialize with large allocation
 *   - cleanup
 *   - Test 5: Initialize with size of 1
 *   - cleanup
 */
static bool
d_internal_tests_array_common_init_sized_part2(
    struct dj_test_suite* _suite
)
{
    bool   test_result;
    bool   result;
    void*  elements;
    size_t count;

    test_result = true;
    result      = false;
    elements    = NULL;
    count       = SIZE_MAX;

    //=========================================================================
    // Test 4: Initialize with large allocation
    //   Expected: success with large memory block
    //=========================================================================
    elements = NULL;
    count    = SIZE_MAX;

    result = d_array_common_init_sized(&elements,
                                       &count,
                                       sizeof(char),
                                       D_TEST_AC_LARGE_ALLOCATION);

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "large allocations are handled correctly",
                                    ( (result == true) && (elements != NULL) && (count == 0) ),
                                    "failed with large allocation");

    // cleanup
    if (elements)
    {
        free(elements);
        elements = NULL;
    }

    //=========================================================================
    // Test 5: Initialize with size of 1
    //   Expected: success with minimal allocation
    //=========================================================================
    elements = NULL;
    count    = SIZE_MAX;

    result = d_array_common_init_sized(&elements,
                                       &count,
                                       sizeof(int),
                                       1);

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "size of 1 allocates correctly",
                                    ( (result == true) && (elements != NULL) && (count == 0) ),
                                    "failed with size of 1");

    // cleanup
    if (elements)
    {
        free(elements);
        elements = NULL;
    }

    return test_result;
}


/*
d_tests_array_common_init_sized
  Tests the `d_array_common_init_sized` function which allocates and
  initializes an elements array with a given size.
  Tests the following:
  - zero size allocation handling (should succeed with NULL elements)
  - valid size memory allocation
  - different element sizes (int, double, char)
  - proper count initialization (starts at 0, not size)
  - large allocation handling
*/
bool d_tests_array_common_init_sized(
    struct dj_test_suite* _suite
)
{
    bool test_result;

    // validate parameters
    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "array_common_init_sized");

    test_result = true;
    test_result &= d_internal_tests_array_common_init_sized_part1(_suite);
    test_result &= d_internal_tests_array_common_init_sized_part2(_suite);

    return test_result;
}

/*
d_tests_array_common_init_from_array
  Tests the `d_array_common_init_from_array` function.
  Tests the following:
  - copying from valid source array
  - handling zero count (should succeed with NULL)
  - handling NULL source (should succeed with NULL)
  - proper memory allocation and element copying
  - data integrity after copy
*/
bool d_tests_array_common_init_from_array(
    struct dj_test_suite* _suite
)
{
    const int source[] = { 1, 2, 3, 4, 5 };

    bool   test_result;
    bool   result;
    void*  elements;
    size_t count;
    int*   int_elements;

    // validate parameters
    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "array_common_init_from_array");

    test_result  = true;
    result       = false;
    elements     = NULL;
    count        = SIZE_MAX;
    int_elements = NULL;

    //=========================================================================
    // Test 1: Initialize from valid array
    //   Expected: elements copied correctly, count matches source_count
    //=========================================================================
    elements = NULL;
    count    = SIZE_MAX;

    result = d_array_common_init_from_array(&elements,
                                            &count,
                                            sizeof(int),
                                            source,
                                            5);

    int_elements = (int*)elements;

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "copies from source array correctly with proper count",
                                    ( (result == true) && (elements != NULL) && (count == 5) && (int_elements[0] == 1) && (int_elements[2] == 3) && (int_elements[4] == 5) ),
                                    "failed to copy from source array");

    // cleanup
    if (elements)
    {
        free(elements);
        elements = NULL;
    }

    //=========================================================================
    // Test 2: Initialize with zero count
    //   Expected: success with NULL elements and count = 0
    //=========================================================================
    elements = NULL;
    count    = SIZE_MAX;

    result = d_array_common_init_from_array(&elements,
                                            &count,
                                            sizeof(int),
                                            source,
                                            0);

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "zero count returns NULL elements correctly",
                                    ( (result == true) && (elements == NULL) && (count == 0) ),
                                    "failed with zero count");

    //=========================================================================
    // Test 3: Initialize with NULL source
    //   Expected: success with NULL elements and count = 0
    //=========================================================================
    elements = NULL;
    count    = SIZE_MAX;

    result = d_array_common_init_from_array(&elements,
                                            &count,
                                            sizeof(int),
                                            NULL,
                                            5);

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "NULL source returns NULL elements correctly",
                                    ( (result == true) && (elements == NULL) && (count == 0) ),
                                    "failed with NULL source");

    //=========================================================================
    // Test 4: Verify full data integrity (all elements)
    //   Expected: all source elements copied in correct order
    //=========================================================================
    elements = NULL;
    count    = SIZE_MAX;

    result = d_array_common_init_from_array(&elements,
                                            &count,
                                            sizeof(int),
                                            source,
                                            5);

    int_elements = (int*)elements;

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "all elements copied with correct values and order",
                                    ( (result == true) && (elements != NULL) && (d_test_verify_array_contents(int_elements, 5, source)) ),
                                    "data integrity check failed");

    // cleanup
    if (elements)
    {
        free(elements);
        elements = NULL;
    }

    return test_result;
}

/**
 * @brief Part 1 of d_tests_array_common_init_from_args.
 *
 * @param[in,out] _suite  the suite the checks are recorded in.
 * @return true if every check of the part passed.
 *
 * The test runs its parts in order, so that no function body is over
 * 100 lines. A part declares its own locals: no value crosses from one
 * part to the next.
 *
 * Sections:
 *   - Test 1: Initialize from valid arguments (3 ints)
 *   - cleanup
 *   - Test 2: Initialize with zero arguments
 *   - Test 3: Initialize with single argument
 *   - cleanup
 */
static bool
d_internal_tests_array_common_init_from_args_part1(
    struct dj_test_suite* _suite
)
{
    bool   test_result;
    bool   result;
    void*  elements;
    size_t count;
    int*   int_elements;

    test_result  = true;
    result       = false;
    elements     = NULL;
    count        = SIZE_MAX;
    int_elements = NULL;

    //=========================================================================
    // Test 1: Initialize from valid arguments (3 ints)
    //   Expected: elements populated from variadic args
    //=========================================================================
    elements = NULL;
    count    = SIZE_MAX;

    result = d_test_init_from_args_wrapper(&elements,
                                           &count,
                                           sizeof(int),
                                           3,
                                           10,
                                           20,
                                           30);

    int_elements = (int*)elements;

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "initializes from variadic arguments correctly",
                                    ( (result == true) && (elements != NULL) && (count == 3) && (int_elements[0] == 10) && (int_elements[1] == 20) && (int_elements[2] == 30) ),
                                    "failed to initialize from variadic arguments");

    // cleanup
    if (elements)
    {
        free(elements);
        elements = NULL;
    }

    //=========================================================================
    // Test 2: Initialize with zero arguments
    //   Expected: success with NULL elements and count = 0
    //=========================================================================
    elements = NULL;
    count    = SIZE_MAX;

    result = d_test_init_from_args_wrapper(&elements,
                                           &count,
                                           sizeof(int),
                                           0);

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "zero arguments returns NULL elements correctly",
                                    ( (result == true) && (elements == NULL) && (count == 0) ),
                                    "failed with zero arguments");

    //=========================================================================
    // Test 3: Initialize with single argument
    //   Expected: single element array created
    //=========================================================================
    elements = NULL;
    count    = SIZE_MAX;

    result = d_test_init_from_args_wrapper(&elements,
                                           &count,
                                           sizeof(int),
                                           1,
                                           42);

    int_elements = (int*)elements;

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "single argument creates single-element array",
                                    ( (result == true) && (elements != NULL) && (count == 1) && (int_elements[0] == 42) ),
                                    "failed with single argument");

    // cleanup
    if (elements)
    {
        free(elements);
        elements = NULL;
    }

    return test_result;
}


/**
 * @brief Part 2 of d_tests_array_common_init_from_args.
 *
 * @param[in,out] _suite  the suite the checks are recorded in.
 * @return true if every check of the part passed.
 *
 * The test runs its parts in order, so that no function body is over
 * 100 lines. A part declares its own locals: no value crosses from one
 * part to the next.
 *
 * Sections:
 *   - Test 4: Initialize with multiple arguments (5 ints)
 *   - cleanup
 */
static bool
d_internal_tests_array_common_init_from_args_part2(
    struct dj_test_suite* _suite
)
{
    bool   test_result;
    bool   result;
    void*  elements;
    size_t count;
    int*   int_elements;

    test_result  = true;
    result       = false;
    elements     = NULL;
    count        = SIZE_MAX;
    int_elements = NULL;

    //=========================================================================
    // Test 4: Initialize with multiple arguments (5 ints)
    //   Expected: all elements populated correctly
    //=========================================================================
    elements = NULL;
    count    = SIZE_MAX;

    result = d_test_init_from_args_wrapper(&elements,
                                           &count,
                                           sizeof(int),
                                           5,
                                           100, 200, 300, 400, 500);

    int_elements = (int*)elements;

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "multiple arguments populate all elements",
                                    ( (result == true) && (elements != NULL) && (count == 5) && (int_elements[0] == 100) && (int_elements[4] == 500) ),
                                    "failed with multiple arguments");

    // cleanup
    if (elements)
    {
        free(elements);
        elements = NULL;
    }

    return test_result;
}


/*
d_tests_array_common_init_from_args
  Tests the `d_array_common_init_from_args` function.
  Tests the following:
  - initialization from variadic arguments
  - handling zero argument count
  - proper memory allocation and element setting
  - different number of arguments
*/
bool d_tests_array_common_init_from_args(
    struct dj_test_suite* _suite
)
{
    bool test_result;

    // validate parameters
    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "array_common_init_from_args");

    test_result = true;
    test_result &= d_internal_tests_array_common_init_from_args_part1(_suite);
    test_result &= d_internal_tests_array_common_init_from_args_part2(_suite);

    return test_result;
}

/*
d_tests_array_common_init_copy
  Tests the `d_array_common_init_copy` function.
  Tests the following:
  - copying initialization from source data
  - verification that it works as alias to init_from_array
  - data integrity after copy
*/
bool d_tests_array_common_init_copy(
    struct dj_test_suite* _suite
)
{
    const int source[] = { 100, 200, 300 };

    bool   test_result;
    bool   result;
    void*  elements;
    size_t count;
    int*   int_elements;

    // validate parameters
    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "array_common_init_copy");

    test_result  = true;
    result       = false;
    elements     = NULL;
    count        = SIZE_MAX;
    int_elements = NULL;

    //=========================================================================
    // Test 1: Copy initialization from valid source
    //   Expected: all elements copied correctly
    //=========================================================================
    elements = NULL;
    count    = SIZE_MAX;

    result = d_array_common_init_copy(&elements,
                                      &count,
                                      sizeof(int),
                                      source,
                                      3);

    int_elements = (int*)elements;

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "performs copy initialization correctly",
                                    ( (result == true) && (elements != NULL) && (count == 3) && (int_elements[0] == 100) && (int_elements[1] == 200) && (int_elements[2] == 300) ),
                                    "failed to perform copy initialization");

    // cleanup
    if (elements)
    {
        free(elements);
        elements = NULL;
    }

    //=========================================================================
    // Test 2: Verify full data integrity matches init_from_array behavior
    //   Expected: identical behavior to init_from_array
    //=========================================================================
    elements = NULL;
    count    = SIZE_MAX;

    result = d_array_common_init_copy(&elements,
                                      &count,
                                      sizeof(int),
                                      source,
                                      3);

    int_elements = (int*)elements;

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "data integrity matches init_from_array behavior",
                                    ( (result == true) && (d_test_verify_array_contents(int_elements, 3, source)) ),
                                    "data integrity verification failed");

    // cleanup
    if (elements)
    {
        free(elements);
        elements = NULL;
    }

    return test_result;
}

/*
d_tests_array_common_init_copy_reverse
  Tests the `d_array_common_init_copy_reverse` function.
  Tests the following:
  - copying elements in reverse order with positive indices
  - copying elements in reverse order with negative indices
  - proper dj_index validation and conversion
  - invalid index handling (start > end, out of bounds)
  - NULL source handling
*/
bool d_tests_array_common_init_copy_reverse(
    struct dj_test_suite* _suite
)
{
    const int source[] = { 1, 2, 3, 4, 5 };

    bool   test_result;
    bool   result;
    void*  elements;
    size_t count;
    int*   int_elements;

    // validate parameters
    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "array_common_init_copy_reverse");

    test_result  = true;
    result       = false;
    elements     = NULL;
    count        = SIZE_MAX;
    int_elements = NULL;

    //=========================================================================
    // Test 1: Copy reverse with positive indices (1 to 3)
    //   Source indices 1-3: [2, 3, 4] -> reversed: [4, 3, 2]
    //=========================================================================
    elements = NULL;
    count    = SIZE_MAX;

    result = d_array_common_init_copy_reverse(&elements,
                                              &count,
                                              sizeof(int),
                                              source,
                                              5,
                                              1,
                                              3);

    int_elements = (int*)elements;

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "copies range in reverse with positive indices",
                                    ( (result == true) && (elements != NULL) && (count == 3) && (int_elements[0] == 4) && (int_elements[1] == 3) && (int_elements[2] == 2) ),
                                    "failed with positive indices");

    // cleanup
    if (elements)
    {
        free(elements);
        elements = NULL;
    }

    //=========================================================================
    // Test 2: Copy reverse with negative indices (-3 to -1)
    //   Converts to indices 2-4: [3, 4, 5] -> reversed: [5, 4, 3]
    //=========================================================================
    elements = NULL;
    count    = SIZE_MAX;

    result = d_array_common_init_copy_reverse(&elements,
                                              &count,
                                              sizeof(int),
                                              source,
                                              5,
                                              -3,
                                              -1);

    int_elements = (int*)elements;

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "handles negative indices correctly",
                                    ( (result == true) && (elements != NULL) && (count == 3) && (int_elements[0] == 5) && (int_elements[1] == 4) && (int_elements[2] == 3) ),
                                    "failed with negative indices");

    // cleanup
    if (elements)
    {
        free(elements);
        elements = NULL;
    }

    //=========================================================================
    // Test 3: Invalid indices (start > end after conversion)
    //   Expected: failure with NULL elements and count = 0
    //=========================================================================
    elements = NULL;
    count    = SIZE_MAX;

    result = d_array_common_init_copy_reverse(&elements,
                                              &count,
                                              sizeof(int),
                                              source,
                                              5,
                                              3,
                                              1);

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "rejects invalid range (start > end)",
                                    ( (result == false) && (elements == NULL) && (count == 0) ),
                                    "failed to reject invalid range");

    //=========================================================================
    // Test 4: Out-of-bounds indices
    //   Expected: failure with NULL elements
    //=========================================================================
    elements = NULL;
    count    = SIZE_MAX;

    result = d_array_common_init_copy_reverse(&elements,
                                              &count,
                                              sizeof(int),
                                              source,
                                              5,
                                              0,
                                              10);

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "rejects out-of-bounds indices",
                                    ( (result == false) && (elements == NULL) && (count == 0) ),
                                    "failed to reject out-of-bounds indices");

    //=========================================================================
    // Test 5: NULL source
    //   Expected: failure with NULL elements
    //=========================================================================
    elements = NULL;
    count    = SIZE_MAX;

    result = d_array_common_init_copy_reverse(&elements,
                                              &count,
                                              sizeof(int),
                                              NULL,
                                              5,
                                              0,
                                              2);

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "handles NULL source correctly",
                                    ( (result == false) && (elements == NULL) && (count == 0) ),
                                    "failed with NULL source");

    //=========================================================================
    // Test 6: Full array reverse (0 to 4)
    //   [1, 2, 3, 4, 5] -> [5, 4, 3, 2, 1]
    //=========================================================================
    elements = NULL;
    count    = SIZE_MAX;

    result = d_array_common_init_copy_reverse(&elements,
                                              &count,
                                              sizeof(int),
                                              source,
                                              5,
                                              0,
                                              4);

    int_elements = (int*)elements;

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "reverses full array correctly",
                                    ( (result == true) && (elements != NULL) && (count == 5) && (int_elements[0] == 5) && (int_elements[4] == 1) ),
                                    "failed to reverse full array");

    // cleanup
    if (elements)
    {
        free(elements);
        elements = NULL;
    }

    return test_result;
}

/*
d_tests_array_common_init_copy_range
  Tests the `d_array_common_init_copy_range` function.
  Tests the following:
  - copying a range with positive indices
  - copying a range with negative indices
  - proper dj_index validation and conversion
  - invalid index handling
*/
bool d_tests_array_common_init_copy_range(
    struct dj_test_suite* _suite
)
{
    const int source[] = { 10, 20, 30, 40, 50 };

    bool   test_result;
    bool   result;
    void*  elements;
    size_t count;
    int*   int_elements;

    // validate parameters
    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "array_common_init_copy_range");

    test_result  = true;
    result       = false;
    elements     = NULL;
    count        = SIZE_MAX;
    int_elements = NULL;

    //=========================================================================
    // Test 1: Copy range with positive indices (1 to 3) -> [20, 30, 40]
    //=========================================================================
    elements = NULL;
    count    = SIZE_MAX;

    result = d_array_common_init_copy_range(&elements,
                                            &count,
                                            sizeof(int),
                                            source,
                                            5,
                                            1,
                                            3);

    int_elements = (int*)elements;

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "copies range with positive indices",
                                    ( (result == true) && (elements != NULL) && (count == 3) && (int_elements[0] == 20) && (int_elements[1] == 30) && (int_elements[2] == 40) ),
                                    "failed with positive indices");

    // cleanup
    if (elements)
    {
        free(elements);
        elements = NULL;
    }

    //=========================================================================
    // Test 2: Copy range with negative indices (-2 to -1) -> [40, 50]
    //   -2 converts to index 3, -1 converts to index 4
    //=========================================================================
    elements = NULL;
    count    = SIZE_MAX;

    result = d_array_common_init_copy_range(&elements,
                                            &count,
                                            sizeof(int),
                                            source,
                                            5,
                                            -2,
                                            -1);

    int_elements = (int*)elements;

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "handles negative indices correctly",
                                    ( (result == true) && (elements != NULL) && (count == 2) && (int_elements[0] == 40) && (int_elements[1] == 50) ),
                                    "failed with negative indices");

    // cleanup
    if (elements)
    {
        free(elements);
        elements = NULL;
    }

    //=========================================================================
    // Test 3: Invalid range (start > end)
    //   Expected: failure
    //=========================================================================
    elements = NULL;
    count    = SIZE_MAX;

    result = d_array_common_init_copy_range(&elements,
                                            &count,
                                            sizeof(int),
                                            source,
                                            5,
                                            4,
                                            1);

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "rejects invalid range (start > end)",
                                    ( (result == false) && (elements == NULL) && (count == 0) ),
                                    "failed to reject invalid range");

    //=========================================================================
    // Test 4: Single element range (2 to 2) -> [30]
    //=========================================================================
    elements = NULL;
    count    = SIZE_MAX;

    result = d_array_common_init_copy_range(&elements,
                                            &count,
                                            sizeof(int),
                                            source,
                                            5,
                                            2,
                                            2);

    int_elements = (int*)elements;

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "handles single element range correctly",
                                    ( (result == true) && (elements != NULL) && (count == 1) && (int_elements[0] == 30) ),
                                    "failed with single element range");

    // cleanup
    if (elements)
    {
        free(elements);
        elements = NULL;
    }

    //=========================================================================
    // Test 5: Mixed positive/negative indices (1 to -2)
    //   1 stays as 1, -2 converts to 3 -> [20, 30, 40]
    //=========================================================================
    elements = NULL;
    count    = SIZE_MAX;

    result = d_array_common_init_copy_range(&elements,
                                            &count,
                                            sizeof(int),
                                            source,
                                            5,
                                            1,
                                            -2);

    int_elements = (int*)elements;

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "handles mixed positive/negative indices",
                                    ( (result == true) && (elements != NULL) && (count == 3) && (int_elements[0] == 20) && (int_elements[2] == 40) ),
                                    "failed with mixed indices");

    // cleanup
    if (elements)
    {
        free(elements);
        elements = NULL;
    }

    return test_result;
}

/*
d_tests_array_common_init_copy_range_reverse
  Tests the `d_array_common_init_copy_range_reverse` function.
  Tests the following:
  - copying a range in reverse with positive indices
  - copying a range in reverse with negative indices
  - verification as alias to init_copy_reverse
*/
bool d_tests_array_common_init_copy_range_reverse(
    struct dj_test_suite* _suite
)
{
    const int source[] = { 10, 20, 30, 40, 50 };

    bool   test_result;
    bool   result;
    void*  elements;
    size_t count;
    int*   int_elements;

    // validate parameters
    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "array_common_init_copy_range_reverse");

    test_result  = true;
    result       = false;
    elements     = NULL;
    count        = SIZE_MAX;
    int_elements = NULL;

    //=========================================================================
    // Test 1: Copy range reverse with positive indices (1 to 3)
    //   [20, 30, 40] reversed -> [40, 30, 20]
    //=========================================================================
    elements = NULL;
    count    = SIZE_MAX;

    result = d_array_common_init_copy_range_reverse(&elements,
                                                    &count,
                                                    sizeof(int),
                                                    source,
                                                    5,
                                                    1,
                                                    3);

    int_elements = (int*)elements;

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "copies range in reverse with positive indices",
                                    ( (result == true) && (elements != NULL) && (count == 3) && (int_elements[0] == 40) && (int_elements[1] == 30) && (int_elements[2] == 20) ),
                                    "failed with positive indices");

    // cleanup
    if (elements)
    {
        free(elements);
        elements = NULL;
    }

    //=========================================================================
    // Test 2: Copy range reverse with negative indices (-3 to -1)
    //   Indices 2-4: [30, 40, 50] reversed -> [50, 40, 30]
    //=========================================================================
    elements = NULL;
    count    = SIZE_MAX;

    result = d_array_common_init_copy_range_reverse(&elements,
                                                    &count,
                                                    sizeof(int),
                                                    source,
                                                    5,
                                                    -3,
                                                    -1);

    int_elements = (int*)elements;

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "handles negative indices correctly",
                                    ( (result == true) && (elements != NULL) && (count == 3) && (int_elements[0] == 50) && (int_elements[1] == 40) && (int_elements[2] == 30) ),
                                    "failed with negative indices");

    // cleanup
    if (elements)
    {
        free(elements);
        elements = NULL;
    }

    return test_result;
}

/**
 * @brief Part 1 of d_tests_array_common_init_fill.
 *
 * @param[in,out] _suite  the suite the checks are recorded in.
 * @return true if every check of the part passed.
 *
 * The test runs its parts in order, so that no function body is over
 * 100 lines. A part declares its own locals: no value crosses from one
 * part to the next.
 *
 * Sections:
 *   - Test 1: Fill with valid value
 *   - cleanup
 *   - Test 2: Fill with zero size
 *   - Test 3: Fill with NULL value
 */
static bool
d_internal_tests_array_common_init_fill_part1(
    struct dj_test_suite* _suite
)
{
    bool   test_result;
    bool   result;
    void*  elements;
    size_t count;
    int*   int_elements;
    int    fill_value;

    test_result  = true;
    result       = false;
    elements     = NULL;
    count        = SIZE_MAX;
    int_elements = NULL;
    fill_value   = 42;

    //=========================================================================
    // Test 1: Fill with valid value
    //   Expected: all elements set to fill_value
    //=========================================================================
    elements = NULL;
    count    = SIZE_MAX;

    result = d_array_common_init_fill(&elements,
                                      &count,
                                      sizeof(int),
                                      5,
                                      &fill_value);

    int_elements = (int*)elements;

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "fills array with specified value",
                                    ( (result == true) && (elements != NULL) && (count == 5) && (int_elements[0] == 42) && (int_elements[2] == 42) && (int_elements[4] == 42) ),
                                    "failed to fill array");

    // cleanup
    if (elements)
    {
        free(elements);
        elements = NULL;
    }

    //=========================================================================
    // Test 2: Fill with zero size
    //   Expected: success with NULL elements and count = 0
    //=========================================================================
    elements = NULL;
    count    = SIZE_MAX;

    result = d_array_common_init_fill(&elements,
                                      &count,
                                      sizeof(int),
                                      0,
                                      &fill_value);

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "handles zero size correctly",
                                    ( (result == true) && (elements == NULL) && (count == 0) ),
                                    "failed with zero size");

    //=========================================================================
    // Test 3: Fill with NULL value
    //   Expected: failure
    //=========================================================================
    elements = NULL;
    count    = SIZE_MAX;

    result = d_array_common_init_fill(&elements,
                                      &count,
                                      sizeof(int),
                                      5,
                                      NULL);

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "handles NULL value correctly",
                                    ( (result == false) && (elements == NULL) && (count == 0) ),
                                    "failed with NULL value");

    return test_result;
}


/**
 * @brief Part 2 of d_tests_array_common_init_fill.
 *
 * @param[in,out] _suite  the suite the checks are recorded in.
 * @return true if every check of the part passed.
 *
 * The test runs its parts in order, so that no function body is over
 * 100 lines. A part declares its own locals: no value crosses from one
 * part to the next.
 *
 * Sections:
 *   - Test 4: Fill with different value (negative number)
 *   - cleanup
 *   - Test 5: Fill single element
 *   - cleanup
 */
static bool
d_internal_tests_array_common_init_fill_part2(
    struct dj_test_suite* _suite
)
{
    bool   test_result;
    bool   result;
    void*  elements;
    size_t count;
    int*   int_elements;
    int    fill_value;

    test_result  = true;
    result       = false;
    elements     = NULL;
    count        = SIZE_MAX;
    int_elements = NULL;
    fill_value   = 42;

    //=========================================================================
    // Test 4: Fill with different value (negative number)
    //=========================================================================
    elements   = NULL;
    count      = SIZE_MAX;
    fill_value = -999;

    result = d_array_common_init_fill(&elements,
                                      &count,
                                      sizeof(int),
                                      3,
                                      &fill_value);

    int_elements = (int*)elements;

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "fills with negative value correctly",
                                    ( (result == true) && (elements != NULL) && (count == 3) && (int_elements[0] == -999) && (int_elements[1] == -999) && (int_elements[2] == -999) ),
                                    "failed with negative fill value");

    // cleanup
    if (elements)
    {
        free(elements);
        elements = NULL;
    }

    //=========================================================================
    // Test 5: Fill single element
    //=========================================================================
    elements   = NULL;
    count      = SIZE_MAX;
    fill_value = 777;

    result = d_array_common_init_fill(&elements,
                                      &count,
                                      sizeof(int),
                                      1,
                                      &fill_value);

    int_elements = (int*)elements;

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "handles single element fill",
                                    ( (result == true) && (elements != NULL) && (count == 1) && (int_elements[0] == 777) ),
                                    "failed with single element fill");

    // cleanup
    if (elements)
    {
        free(elements);
        elements = NULL;
    }

    return test_result;
}


/*
d_tests_array_common_init_fill
  Tests the `d_array_common_init_fill` function.
  Tests the following:
  - filling array with a single value
  - handling zero size
  - handling NULL value
  - different fill values
*/
bool d_tests_array_common_init_fill(
    struct dj_test_suite* _suite
)
{
    bool test_result;

    // validate parameters
    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "array_common_init_fill");

    test_result = true;
    test_result &= d_internal_tests_array_common_init_fill_part1(_suite);
    test_result &= d_internal_tests_array_common_init_fill_part2(_suite);

    return test_result;
}

/*
d_tests_array_common_init_slice
  Tests the `d_array_common_init_slice` function.
  Tests the following:
  - slicing from a starting index to end
  - positive index support
  - negative index support
  - invalid parameter handling
  Note: This function operates on arrays of pointers (void**).
*/
bool d_tests_array_common_init_slice(
    struct dj_test_suite* _suite
)
{
    bool   test_result;
    bool   result;
    void*  elements;
    size_t count;

    int         val1, val2, val3, val4, val5;
    void*       ptr_arr[5];
    void**      result_ptrs;
    const void* source;

    // validate parameters
    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "array_common_init_slice");

    test_result = true;
    result      = false;
    elements    = NULL;
    count       = SIZE_MAX;
    result_ptrs = NULL;

    // setup pointer array for testing
    val1 = 10;
    val2 = 20;
    val3 = 30;
    val4 = 40;
    val5 = 50;

    ptr_arr[0] = &val1;
    ptr_arr[1] = &val2;
    ptr_arr[2] = &val3;
    ptr_arr[3] = &val4;
    ptr_arr[4] = &val5;

    // the slices take the address of the elements pointer, as array.c
    // passes &array->elements
    source = (const void*)ptr_arr;

    //=========================================================================
    // Test 1: Slice from index 2 to end -> [&val3, &val4, &val5]
    //=========================================================================
    elements = NULL;
    count    = SIZE_MAX;

    result = d_array_common_init_slice(&elements,
                                       &count,
                                       sizeof(void*),
                                       &source,
                                       5,
                                       2);

    result_ptrs = (void**)elements;

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "slices from positive index to end",
                                    ( (result == true) && (elements != NULL) && (count == 3) && (result_ptrs[0] == &val3) && (result_ptrs[1] == &val4) && (result_ptrs[2] == &val5) ),
                                    "failed with positive start index");

    // cleanup
    if (elements)
    {
        free(elements);
        elements = NULL;
    }

    //=========================================================================
    // Test 2: Slice from negative index -2 to end -> [&val4, &val5]
    //   -2 converts to index 3
    //=========================================================================
    elements = NULL;
    count    = SIZE_MAX;

    result = d_array_common_init_slice(&elements,
                                       &count,
                                       sizeof(void*),
                                       &source,
                                       5,
                                       -2);

    result_ptrs = (void**)elements;

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "handles negative start index correctly",
                                    ( (result == true) && (elements != NULL) && (count == 2) && (result_ptrs[0] == &val4) && (result_ptrs[1] == &val5) ),
                                    "failed with negative start index");

    // cleanup
    if (elements)
    {
        free(elements);
        elements = NULL;
    }

    //=========================================================================
    // Test 3: Slice from index 0 (full array)
    //=========================================================================
    elements = NULL;
    count    = SIZE_MAX;

    result = d_array_common_init_slice(&elements,
                                       &count,
                                       sizeof(void*),
                                       &source,
                                       5,
                                       0);

    result_ptrs = (void**)elements;

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "slices from index 0 (full array)",
                                    ( (result == true) && (elements != NULL) && (count == 5) && (result_ptrs[0] == &val1) && (result_ptrs[4] == &val5) ),
                                    "failed with index 0");

    // cleanup
    if (elements)
    {
        free(elements);
        elements = NULL;
    }

    //=========================================================================
    // Test 4: Slice from last index -1 -> [&val5]
    //=========================================================================
    elements = NULL;
    count    = SIZE_MAX;

    result = d_array_common_init_slice(&elements,
                                       &count,
                                       sizeof(void*),
                                       &source,
                                       5,
                                       -1);

    result_ptrs = (void**)elements;

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "handles slice from last element (-1)",
                                    ( (result == true) && (elements != NULL) && (count == 1) && (result_ptrs[0] == &val5) ),
                                    "failed with -1 index");

    // cleanup
    if (elements)
    {
        free(elements);
        elements = NULL;
    }

    return test_result;
}

/*
d_tests_array_common_init_slice_reverse
  Tests the `d_array_common_init_slice_reverse` function.
  Tests the following:
  - slicing entire array in reverse order
  - verification as wrapper for init_copy_reverse
  Note: This function operates on arrays of pointers (void**).
*/
bool d_tests_array_common_init_slice_reverse(
    struct dj_test_suite* _suite
)
{
    bool   test_result;
    bool   result;
    void*  elements;
    size_t count;

    int         val1, val2, val3;
    void*       ptr_arr[3];
    void**      result_ptrs;
    const void* source;

    // validate parameters
    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "array_common_init_slice_reverse");

    test_result = true;
    result      = false;
    elements    = NULL;
    count       = SIZE_MAX;
    result_ptrs = NULL;

    // setup pointer array for testing
    val1 = 100;
    val2 = 200;
    val3 = 300;

    ptr_arr[0] = &val1;
    ptr_arr[1] = &val2;
    ptr_arr[2] = &val3;

    // the slices take the address of the elements pointer, as array.c
    // passes &array->elements
    source = (const void*)ptr_arr;

    //=========================================================================
    // Test 1: Reverse full array -> [&val3, &val2, &val1]
    //=========================================================================
    elements = NULL;
    count    = SIZE_MAX;

    result = d_array_common_init_slice_reverse(&elements,
                                               &count,
                                               sizeof(void*),
                                               &source,
                                               3);

    result_ptrs = (void**)elements;

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "reverses full array correctly",
                                    ( (result == true) && (elements != NULL) && (count == 3) && (result_ptrs[0] == &val3) && (result_ptrs[1] == &val2) && (result_ptrs[2] == &val1) ),
                                    "failed to reverse full array");

    // cleanup
    if (elements)
    {
        free(elements);
        elements = NULL;
    }

    return test_result;
}

/*
d_tests_array_common_init_slice_range
  Tests the `d_array_common_init_slice_range` function.
  Tests the following:
  - slicing a range with positive indices
  - slicing a range with negative indices
  - proper dj_index validation and conversion
  Note: This function operates on arrays of pointers (void**).
*/
bool d_tests_array_common_init_slice_range(
    struct dj_test_suite* _suite
)
{
    bool   test_result;
    bool   result;
    void*  elements;
    size_t count;

    int         val1, val2, val3, val4, val5;
    void*       ptr_arr[5];
    void**      result_ptrs;
    const void* source;

    // validate parameters
    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "array_common_init_slice_range");

    test_result = true;
    result      = false;
    elements    = NULL;
    count       = SIZE_MAX;
    result_ptrs = NULL;

    // setup pointer array for testing
    val1 = 11;
    val2 = 22;
    val3 = 33;
    val4 = 44;
    val5 = 55;

    ptr_arr[0] = &val1;
    ptr_arr[1] = &val2;
    ptr_arr[2] = &val3;
    ptr_arr[3] = &val4;
    ptr_arr[4] = &val5;

    // the slices take the address of the elements pointer, as array.c
    // passes &array->elements
    source = (const void*)ptr_arr;

    //=========================================================================
    // Test 1: Slice range with indices (1 to 3) -> [&val2, &val3, &val4]
    //=========================================================================
    elements = NULL;
    count    = SIZE_MAX;

    result = d_array_common_init_slice_range(&elements,
                                             &count,
                                             sizeof(void*),
                                             &source,
                                             5,
                                             1,
                                             3);

    result_ptrs = (void**)elements;

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "slices range with positive indices",
                                    ( (result == true) && (elements != NULL) && (count == 3) && (result_ptrs[0] == &val2) && (result_ptrs[1] == &val3) && (result_ptrs[2] == &val4) ),
                                    "failed with positive indices");

    // cleanup
    if (elements)
    {
        free(elements);
        elements = NULL;
    }

    //=========================================================================
    // Test 2: Slice range with negative indices (-2 to -1) -> [&val4, &val5]
    //=========================================================================
    elements = NULL;
    count    = SIZE_MAX;

    result = d_array_common_init_slice_range(&elements,
                                             &count,
                                             sizeof(void*),
                                             &source,
                                             5,
                                             -2,
                                             -1);

    result_ptrs = (void**)elements;

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "handles negative indices correctly",
                                    ( (result == true) && (elements != NULL) && (count == 2) && (result_ptrs[0] == &val4) && (result_ptrs[1] == &val5) ),
                                    "failed with negative indices");

    // cleanup
    if (elements)
    {
        free(elements);
        elements = NULL;
    }

    return test_result;
}

/*
d_tests_array_common_init_slice_range_reverse
  Tests the `d_array_common_init_slice_range_reverse` function.
  Tests the following:
  - slicing a range in reverse order with positive indices
  - slicing a range in reverse order with negative indices
  - proper dj_index validation and conversion
  Note: This function operates on arrays of pointers (void**).
*/
bool d_tests_array_common_init_slice_range_reverse(
    struct dj_test_suite* _suite
)
{
    bool   test_result;
    bool   result;
    void*  elements;
    size_t count;

    int         val1, val2, val3, val4, val5;
    void*       ptr_arr[5];
    void**      result_ptrs;
    const void* source;

    // validate parameters
    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "array_common_init_slice_range_reverse");

    test_result = true;
    result      = false;
    elements    = NULL;
    count       = SIZE_MAX;
    result_ptrs = NULL;

    // setup pointer array for testing
    val1 = 111;
    val2 = 222;
    val3 = 333;
    val4 = 444;
    val5 = 555;

    ptr_arr[0] = &val1;
    ptr_arr[1] = &val2;
    ptr_arr[2] = &val3;
    ptr_arr[3] = &val4;
    ptr_arr[4] = &val5;

    // the slices take the address of the elements pointer, as array.c
    // passes &array->elements
    source = (const void*)ptr_arr;

    //=========================================================================
    // Test 1: Slice range reverse with indices (0 to 2)
    //   [&val1, &val2, &val3] reversed -> [&val3, &val2, &val1]
    //=========================================================================
    elements = NULL;
    count    = SIZE_MAX;

    result = d_array_common_init_slice_range_reverse(&elements,
                                                     &count,
                                                     sizeof(void*),
                                                     &source,
                                                     5,
                                                     0,
                                                     2);

    result_ptrs = (void**)elements;

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "slices range in reverse with positive indices",
                                    ( (result == true) && (elements != NULL) && (count == 3) && (result_ptrs[0] == &val3) && (result_ptrs[1] == &val2) && (result_ptrs[2] == &val1) ),
                                    "failed with positive indices");

    // cleanup
    if (elements)
    {
        free(elements);
        elements = NULL;
    }

    //=========================================================================
    // Test 2: Slice range reverse with negative indices (-3 to -2)
    //   Indices 2 to 3: [&val3, &val4] reversed -> [&val4, &val3]
    //=========================================================================
    elements = NULL;
    count    = SIZE_MAX;

    result = d_array_common_init_slice_range_reverse(&elements,
                                                     &count,
                                                     sizeof(void*),
                                                     &source,
                                                     5,
                                                     -3,
                                                     -2);

    result_ptrs = (void**)elements;

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "handles negative indices correctly",
                                    ( (result == true) && (elements != NULL) && (count == 2) && (result_ptrs[0] == &val4) && (result_ptrs[1] == &val3) ),
                                    "failed with negative indices");

    // cleanup
    if (elements)
    {
        free(elements);
        elements = NULL;
    }

    return test_result;
}


/******************************************************************************
 * INITIALIZATION TEST GROUP RUNNER
 *****************************************************************************/

/*
d_tests_array_common_run_init_tests
  Runs all initialization function tests and returns overall pass/fail status.

Parameter(s):
  _suite: pointer to test counter for tracking results
Return:
  A boolean value corresponding to either:
  - true, if all initialization tests passed, or
  - false, if any test failed.
*/
bool d_tests_array_common_run_init_tests(
    struct dj_test_suite* _suite
)
{
    bool overall_result;

    // validate parameters
    if (!_suite)
    {
        return false;
    }

    d_test_suite_block(_suite, "array_common_run_init_tests");

    overall_result = true;

    printf("\n----------------------------------------\n");
    printf("Running Initialization Function Tests\n");

    // run all initialization tests in header order
    overall_result &= d_tests_array_common_init_sized(_suite);
    overall_result &= d_tests_array_common_init_from_array(_suite);
    overall_result &= d_tests_array_common_init_from_args(_suite);
    overall_result &= d_tests_array_common_init_copy(_suite);
    overall_result &= d_tests_array_common_init_copy_reverse(_suite);
    overall_result &= d_tests_array_common_init_copy_range(_suite);
    overall_result &= d_tests_array_common_init_copy_range_reverse(_suite);
    overall_result &= d_tests_array_common_init_fill(_suite);
    overall_result &= d_tests_array_common_init_slice(_suite);
    overall_result &= d_tests_array_common_init_slice_reverse(_suite);
    overall_result &= d_tests_array_common_init_slice_range(_suite);
    overall_result &= d_tests_array_common_init_slice_range_reverse(_suite);

    return overall_result;
}
