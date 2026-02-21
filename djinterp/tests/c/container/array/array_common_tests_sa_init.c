/******************************************************************************
* djinterp [container]                             array_common_tests_sa_init.c
*
*   Unit tests for `array_common.h` initialization functions.
*   Part of the array_common standalone test suite.
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
*   - d_index negative indexing support where applicable
*
*
* path:      \test\container\array\array_common_tests_sa_init.c
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.10.25
******************************************************************************/

#include ".\array_common_tests_sa.h"


/******************************************************************************
 * INITIALIZATION FUNCTION TESTS
 *****************************************************************************/

/*
d_tests_sa_array_common_init_sized
  Tests the `d_array_common_init_sized` function which allocates and
  initializes an elements array with a given size.
  Tests the following:
  - zero size allocation handling (should succeed with NULL elements)
  - valid size memory allocation
  - different element sizes (int, double, char)
  - proper count initialization (starts at 0, not size)
  - large allocation handling
*/
bool
d_tests_sa_array_common_init_sized
(
    struct d_test_counter* _test_info
)
{
    bool   test_result;
    bool   result;
    void*  elements;
    size_t count;

    // validate parameters
    if (!_test_info)
    {
        return false;
    }

    test_result = true;
    result      = false;
    elements    = NULL;
    count       = SIZE_MAX;

    printf("Testing `d_array_common_init_sized`...\n");

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

    test_result &= d_assert_standalone(
        ( (result == true)   &&
          (elements == NULL) &&
          (count == 0) ),
        "zero size initialization returns NULL elements and count = 0",
        "failed to handle zero size correctly",
        _test_info
    );

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

    test_result &= d_assert_standalone(
        ( (result == true)   &&
          (elements != NULL) &&
          (count == 0) ),
        "valid size allocates memory with count initialized to 0",
        "failed to allocate memory for valid size",
        _test_info
    );

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

    test_result &= d_assert_standalone(
        ( (result == true)   &&
          (elements != NULL) &&
          (count == 0) ),
        "different element sizes (double) are handled correctly",
        "failed with different element size (double)",
        _test_info
    );

    // cleanup
    if (elements)
    {
        free(elements);
        elements = NULL;
    }

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

    test_result &= d_assert_standalone(
        ( (result == true)   &&
          (elements != NULL) &&
          (count == 0) ),
        "large allocations are handled correctly",
        "failed with large allocation",
        _test_info
    );

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

    test_result &= d_assert_standalone(
        ( (result == true)   &&
          (elements != NULL) &&
          (count == 0) ),
        "size of 1 allocates correctly",
        "failed with size of 1",
        _test_info
    );

    // cleanup
    if (elements)
    {
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
d_tests_sa_array_common_init_from_array
  Tests the `d_array_common_init_from_array` function.
  Tests the following:
  - copying from valid source array
  - handling zero count (should succeed with NULL)
  - handling NULL source (should succeed with NULL)
  - proper memory allocation and element copying
  - data integrity after copy
*/
bool
d_tests_sa_array_common_init_from_array
(
    struct d_test_counter* _test_info
)
{
    const int source[] = { 1, 2, 3, 4, 5 };

    bool   test_result;
    bool   result;
    void*  elements;
    size_t count;
    int*   int_elements;

    // validate parameters
    if (!_test_info)
    {
        return false;
    }

    test_result  = true;
    result       = false;
    elements     = NULL;
    count        = SIZE_MAX;
    int_elements = NULL;

    printf("Testing `d_array_common_init_from_array`...\n");

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

    test_result &= d_assert_standalone(
        ( (result == true)       &&
          (elements != NULL)     &&
          (count == 5)           &&
          (int_elements[0] == 1) &&
          (int_elements[2] == 3) &&
          (int_elements[4] == 5) ),
        "copies from source array correctly with proper count",
        "failed to copy from source array",
        _test_info
    );

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

    test_result &= d_assert_standalone(
        ( (result == true)   &&
          (elements == NULL) &&
          (count == 0) ),
        "zero count returns NULL elements correctly",
        "failed with zero count",
        _test_info
    );

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

    test_result &= d_assert_standalone(
        ( (result == true)   &&
          (elements == NULL) &&
          (count == 0) ),
        "NULL source returns NULL elements correctly",
        "failed with NULL source",
        _test_info
    );

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

    test_result &= d_assert_standalone(
        ( (result == true)                                  &&
          (elements != NULL)                                &&
          (d_test_verify_array_contents(int_elements, 5, source)) ),
        "all elements copied with correct values and order",
        "data integrity check failed",
        _test_info
    );

    // cleanup
    if (elements)
    {
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
d_tests_sa_array_common_init_from_args
  Tests the `d_array_common_init_from_args` function.
  Tests the following:
  - initialization from variadic arguments
  - handling zero argument count
  - proper memory allocation and element setting
  - different number of arguments
*/
bool
d_tests_sa_array_common_init_from_args
(
    struct d_test_counter* _test_info
)
{
    bool   test_result;
    bool   result;
    void*  elements;
    size_t count;
    int*   int_elements;

    // validate parameters
    if (!_test_info)
    {
        return false;
    }

    test_result  = true;
    result       = false;
    elements     = NULL;
    count        = SIZE_MAX;
    int_elements = NULL;

    printf("Testing `d_array_common_init_from_args`...\n");

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

    test_result &= d_assert_standalone(
        ( (result == true)        &&
          (elements != NULL)      &&
          (count == 3)            &&
          (int_elements[0] == 10) &&
          (int_elements[1] == 20) &&
          (int_elements[2] == 30) ),
        "initializes from variadic arguments correctly",
        "failed to initialize from variadic arguments",
        _test_info
    );

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

    test_result &= d_assert_standalone(
        ( (result == true)   &&
          (elements == NULL) &&
          (count == 0) ),
        "zero arguments returns NULL elements correctly",
        "failed with zero arguments",
        _test_info
    );

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

    test_result &= d_assert_standalone(
        ( (result == true)        &&
          (elements != NULL)      &&
          (count == 1)            &&
          (int_elements[0] == 42) ),
        "single argument creates single-element array",
        "failed with single argument",
        _test_info
    );

    // cleanup
    if (elements)
    {
        free(elements);
        elements = NULL;
    }

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

    test_result &= d_assert_standalone(
        ( (result == true)          &&
          (elements != NULL)        &&
          (count == 5)              &&
          (int_elements[0] == 100)  &&
          (int_elements[4] == 500) ),
        "multiple arguments populate all elements",
        "failed with multiple arguments",
        _test_info
    );

    // cleanup
    if (elements)
    {
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
d_tests_sa_array_common_init_copy
  Tests the `d_array_common_init_copy` function.
  Tests the following:
  - copying initialization from source data
  - verification that it works as alias to init_from_array
  - data integrity after copy
*/
bool
d_tests_sa_array_common_init_copy
(
    struct d_test_counter* _test_info
)
{
    const int source[] = { 100, 200, 300 };

    bool   test_result;
    bool   result;
    void*  elements;
    size_t count;
    int*   int_elements;

    // validate parameters
    if (!_test_info)
    {
        return false;
    }

    test_result  = true;
    result       = false;
    elements     = NULL;
    count        = SIZE_MAX;
    int_elements = NULL;

    printf("Testing `d_array_common_init_copy`...\n");

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

    test_result &= d_assert_standalone(
        ( (result == true)         &&
          (elements != NULL)       &&
          (count == 3)             &&
          (int_elements[0] == 100) &&
          (int_elements[1] == 200) &&
          (int_elements[2] == 300) ),
        "performs copy initialization correctly",
        "failed to perform copy initialization",
        _test_info
    );

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

    test_result &= d_assert_standalone(
        ( (result == true)                                    &&
          (d_test_verify_array_contents(int_elements, 3, source)) ),
        "data integrity matches init_from_array behavior",
        "data integrity verification failed",
        _test_info
    );

    // cleanup
    if (elements)
    {
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
d_tests_sa_array_common_init_copy_reverse
  Tests the `d_array_common_init_copy_reverse` function.
  Tests the following:
  - copying elements in reverse order with positive indices
  - copying elements in reverse order with negative indices
  - proper d_index validation and conversion
  - invalid index handling (start > end, out of bounds)
  - NULL source handling
*/
bool
d_tests_sa_array_common_init_copy_reverse
(
    struct d_test_counter* _test_info
)
{
    const int source[] = { 1, 2, 3, 4, 5 };

    bool   test_result;
    bool   result;
    void*  elements;
    size_t count;
    int*   int_elements;

    // validate parameters
    if (!_test_info)
    {
        return false;
    }

    test_result  = true;
    result       = false;
    elements     = NULL;
    count        = SIZE_MAX;
    int_elements = NULL;

    printf("Testing `d_array_common_init_copy_reverse`...\n");

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

    test_result &= d_assert_standalone(
        ( (result == true)       &&
          (elements != NULL)     &&
          (count == 3)           &&
          (int_elements[0] == 4) &&
          (int_elements[1] == 3) &&
          (int_elements[2] == 2) ),
        "copies range in reverse with positive indices",
        "failed with positive indices",
        _test_info
    );

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

    test_result &= d_assert_standalone(
        ( (result == true)       &&
          (elements != NULL)     &&
          (count == 3)           &&
          (int_elements[0] == 5) &&
          (int_elements[1] == 4) &&
          (int_elements[2] == 3) ),
        "handles negative indices correctly",
        "failed with negative indices",
        _test_info
    );

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

    test_result &= d_assert_standalone(
        ( (result == false)  &&
          (elements == NULL) &&
          (count == 0) ),
        "rejects invalid range (start > end)",
        "failed to reject invalid range",
        _test_info
    );

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

    test_result &= d_assert_standalone(
        ( (result == false)  &&
          (elements == NULL) &&
          (count == 0) ),
        "rejects out-of-bounds indices",
        "failed to reject out-of-bounds indices",
        _test_info
    );

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

    test_result &= d_assert_standalone(
        ( (result == false)  &&
          (elements == NULL) &&
          (count == 0) ),
        "handles NULL source correctly",
        "failed with NULL source",
        _test_info
    );

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

    test_result &= d_assert_standalone(
        ( (result == true)       &&
          (elements != NULL)     &&
          (count == 5)           &&
          (int_elements[0] == 5) &&
          (int_elements[4] == 1) ),
        "reverses full array correctly",
        "failed to reverse full array",
        _test_info
    );

    // cleanup
    if (elements)
    {
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
d_tests_sa_array_common_init_copy_range
  Tests the `d_array_common_init_copy_range` function.
  Tests the following:
  - copying a range with positive indices
  - copying a range with negative indices
  - proper d_index validation and conversion
  - invalid index handling
*/
bool
d_tests_sa_array_common_init_copy_range
(
    struct d_test_counter* _test_info
)
{
    const int source[] = { 10, 20, 30, 40, 50 };

    bool   test_result;
    bool   result;
    void*  elements;
    size_t count;
    int*   int_elements;

    // validate parameters
    if (!_test_info)
    {
        return false;
    }

    test_result  = true;
    result       = false;
    elements     = NULL;
    count        = SIZE_MAX;
    int_elements = NULL;

    printf("Testing `d_array_common_init_copy_range`...\n");

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

    test_result &= d_assert_standalone(
        ( (result == true)        &&
          (elements != NULL)      &&
          (count == 3)            &&
          (int_elements[0] == 20) &&
          (int_elements[1] == 30) &&
          (int_elements[2] == 40) ),
        "copies range with positive indices",
        "failed with positive indices",
        _test_info
    );

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

    test_result &= d_assert_standalone(
        ( (result == true)        &&
          (elements != NULL)      &&
          (count == 2)            &&
          (int_elements[0] == 40) &&
          (int_elements[1] == 50) ),
        "handles negative indices correctly",
        "failed with negative indices",
        _test_info
    );

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

    test_result &= d_assert_standalone(
        ( (result == false)  &&
          (elements == NULL) &&
          (count == 0) ),
        "rejects invalid range (start > end)",
        "failed to reject invalid range",
        _test_info
    );

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

    test_result &= d_assert_standalone(
        ( (result == true)        &&
          (elements != NULL)      &&
          (count == 1)            &&
          (int_elements[0] == 30) ),
        "handles single element range correctly",
        "failed with single element range",
        _test_info
    );

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

    test_result &= d_assert_standalone(
        ( (result == true)        &&
          (elements != NULL)      &&
          (count == 3)            &&
          (int_elements[0] == 20) &&
          (int_elements[2] == 40) ),
        "handles mixed positive/negative indices",
        "failed with mixed indices",
        _test_info
    );

    // cleanup
    if (elements)
    {
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
d_tests_sa_array_common_init_copy_range_reverse
  Tests the `d_array_common_init_copy_range_reverse` function.
  Tests the following:
  - copying a range in reverse with positive indices
  - copying a range in reverse with negative indices
  - verification as alias to init_copy_reverse
*/
bool
d_tests_sa_array_common_init_copy_range_reverse
(
    struct d_test_counter* _test_info
)
{
    const int source[] = { 10, 20, 30, 40, 50 };

    bool   test_result;
    bool   result;
    void*  elements;
    size_t count;
    int*   int_elements;

    // validate parameters
    if (!_test_info)
    {
        return false;
    }

    test_result  = true;
    result       = false;
    elements     = NULL;
    count        = SIZE_MAX;
    int_elements = NULL;

    printf("Testing `d_array_common_init_copy_range_reverse`...\n");

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

    test_result &= d_assert_standalone(
        ( (result == true)        &&
          (elements != NULL)      &&
          (count == 3)            &&
          (int_elements[0] == 40) &&
          (int_elements[1] == 30) &&
          (int_elements[2] == 20) ),
        "copies range in reverse with positive indices",
        "failed with positive indices",
        _test_info
    );

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

    test_result &= d_assert_standalone(
        ( (result == true)        &&
          (elements != NULL)      &&
          (count == 3)            &&
          (int_elements[0] == 50) &&
          (int_elements[1] == 40) &&
          (int_elements[2] == 30) ),
        "handles negative indices correctly",
        "failed with negative indices",
        _test_info
    );

    // cleanup
    if (elements)
    {
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
d_tests_sa_array_common_init_fill
  Tests the `d_array_common_init_fill` function.
  Tests the following:
  - filling array with a single value
  - handling zero size
  - handling NULL value
  - different fill values
*/
bool
d_tests_sa_array_common_init_fill
(
    struct d_test_counter* _test_info
)
{
    bool   test_result;
    bool   result;
    void*  elements;
    size_t count;
    int*   int_elements;
    int    fill_value;

    // validate parameters
    if (!_test_info)
    {
        return false;
    }

    test_result  = true;
    result       = false;
    elements     = NULL;
    count        = SIZE_MAX;
    int_elements = NULL;
    fill_value   = 42;

    printf("Testing `d_array_common_init_fill`...\n");

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

    test_result &= d_assert_standalone(
        ( (result == true)        &&
          (elements != NULL)      &&
          (count == 5)            &&
          (int_elements[0] == 42) &&
          (int_elements[2] == 42) &&
          (int_elements[4] == 42) ),
        "fills array with specified value",
        "failed to fill array",
        _test_info
    );

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

    test_result &= d_assert_standalone(
        ( (result == true)   &&
          (elements == NULL) &&
          (count == 0) ),
        "handles zero size correctly",
        "failed with zero size",
        _test_info
    );

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

    test_result &= d_assert_standalone(
        ( (result == false)  &&
          (elements == NULL) &&
          (count == 0) ),
        "handles NULL value correctly",
        "failed with NULL value",
        _test_info
    );

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

    test_result &= d_assert_standalone(
        ( (result == true)          &&
          (elements != NULL)        &&
          (count == 3)              &&
          (int_elements[0] == -999) &&
          (int_elements[1] == -999) &&
          (int_elements[2] == -999) ),
        "fills with negative value correctly",
        "failed with negative fill value",
        _test_info
    );

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

    test_result &= d_assert_standalone(
        ( (result == true)         &&
          (elements != NULL)       &&
          (count == 1)             &&
          (int_elements[0] == 777) ),
        "handles single element fill",
        "failed with single element fill",
        _test_info
    );

    // cleanup
    if (elements)
    {
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
d_tests_sa_array_common_init_slice
  Tests the `d_array_common_init_slice` function.
  Tests the following:
  - slicing from a starting index to end
  - positive index support
  - negative index support
  - invalid parameter handling
  Note: This function operates on arrays of pointers (void**).
*/
bool
d_tests_sa_array_common_init_slice
(
    struct d_test_counter* _test_info
)
{
    bool   test_result;
    bool   result;
    void*  elements;
    size_t count;

    int    val1, val2, val3, val4, val5;
    void*  ptr_arr[5];
    void** result_ptrs;

    // validate parameters
    if (!_test_info)
    {
        return false;
    }

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

    printf("Testing `d_array_common_init_slice`...\n");

    //=========================================================================
    // Test 1: Slice from index 2 to end -> [&val3, &val4, &val5]
    //=========================================================================
    elements = NULL;
    count    = SIZE_MAX;

    result = d_array_common_init_slice(&elements,
                                       &count,
                                       sizeof(void*),
                                       (const void**)ptr_arr,
                                       5,
                                       2);

    result_ptrs = (void**)elements;

    test_result &= d_assert_standalone(
        ( (result == true)          &&
          (elements != NULL)        &&
          (count == 3)              &&
          (result_ptrs[0] == &val3) &&
          (result_ptrs[1] == &val4) &&
          (result_ptrs[2] == &val5) ),
        "slices from positive index to end",
        "failed with positive start index",
        _test_info
    );

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
                                       (const void**)ptr_arr,
                                       5,
                                       -2);

    result_ptrs = (void**)elements;

    test_result &= d_assert_standalone(
        ( (result == true)          &&
          (elements != NULL)        &&
          (count == 2)              &&
          (result_ptrs[0] == &val4) &&
          (result_ptrs[1] == &val5) ),
        "handles negative start index correctly",
        "failed with negative start index",
        _test_info
    );

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
                                       (const void**)ptr_arr,
                                       5,
                                       0);

    result_ptrs = (void**)elements;

    test_result &= d_assert_standalone(
        ( (result == true)          &&
          (elements != NULL)        &&
          (count == 5)              &&
          (result_ptrs[0] == &val1) &&
          (result_ptrs[4] == &val5) ),
        "slices from index 0 (full array)",
        "failed with index 0",
        _test_info
    );

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
                                       (const void**)ptr_arr,
                                       5,
                                       -1);

    result_ptrs = (void**)elements;

    test_result &= d_assert_standalone(
        ( (result == true)          &&
          (elements != NULL)        &&
          (count == 1)              &&
          (result_ptrs[0] == &val5) ),
        "handles slice from last element (-1)",
        "failed with -1 index",
        _test_info
    );

    // cleanup
    if (elements)
    {
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
d_tests_sa_array_common_init_slice_reverse
  Tests the `d_array_common_init_slice_reverse` function.
  Tests the following:
  - slicing entire array in reverse order
  - verification as wrapper for init_copy_reverse
  Note: This function operates on arrays of pointers (void**).
*/
bool
d_tests_sa_array_common_init_slice_reverse
(
    struct d_test_counter* _test_info
)
{
    bool   test_result;
    bool   result;
    void*  elements;
    size_t count;

    int    val1, val2, val3;
    void*  ptr_arr[3];
    void** result_ptrs;

    // validate parameters
    if (!_test_info)
    {
        return false;
    }

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

    printf("Testing `d_array_common_init_slice_reverse`...\n");

    //=========================================================================
    // Test 1: Reverse full array -> [&val3, &val2, &val1]
    //=========================================================================
    elements = NULL;
    count    = SIZE_MAX;

    result = d_array_common_init_slice_reverse(&elements,
                                               &count,
                                               sizeof(void*),
                                               (const void**)ptr_arr,
                                               3);

    result_ptrs = (void**)elements;

    test_result &= d_assert_standalone(
        ( (result == true)          &&
          (elements != NULL)        &&
          (count == 3)              &&
          (result_ptrs[0] == &val3) &&
          (result_ptrs[1] == &val2) &&
          (result_ptrs[2] == &val1) ),
        "reverses full array correctly",
        "failed to reverse full array",
        _test_info
    );

    // cleanup
    if (elements)
    {
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
d_tests_sa_array_common_init_slice_range
  Tests the `d_array_common_init_slice_range` function.
  Tests the following:
  - slicing a range with positive indices
  - slicing a range with negative indices
  - proper d_index validation and conversion
  Note: This function operates on arrays of pointers (void**).
*/
bool
d_tests_sa_array_common_init_slice_range
(
    struct d_test_counter* _test_info
)
{
    bool   test_result;
    bool   result;
    void*  elements;
    size_t count;

    int    val1, val2, val3, val4, val5;
    void*  ptr_arr[5];
    void** result_ptrs;

    // validate parameters
    if (!_test_info)
    {
        return false;
    }

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

    printf("Testing `d_array_common_init_slice_range`...\n");

    //=========================================================================
    // Test 1: Slice range with indices (1 to 3) -> [&val2, &val3, &val4]
    //=========================================================================
    elements = NULL;
    count    = SIZE_MAX;

    result = d_array_common_init_slice_range(&elements,
                                             &count,
                                             sizeof(void*),
                                             (const void**)ptr_arr,
                                             5,
                                             1,
                                             3);

    result_ptrs = (void**)elements;

    test_result &= d_assert_standalone(
        ( (result == true)          &&
          (elements != NULL)        &&
          (count == 3)              &&
          (result_ptrs[0] == &val2) &&
          (result_ptrs[1] == &val3) &&
          (result_ptrs[2] == &val4) ),
        "slices range with positive indices",
        "failed with positive indices",
        _test_info
    );

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
                                             (const void**)ptr_arr,
                                             5,
                                             -2,
                                             -1);

    result_ptrs = (void**)elements;

    test_result &= d_assert_standalone(
        ( (result == true)          &&
          (elements != NULL)        &&
          (count == 2)              &&
          (result_ptrs[0] == &val4) &&
          (result_ptrs[1] == &val5) ),
        "handles negative indices correctly",
        "failed with negative indices",
        _test_info
    );

    // cleanup
    if (elements)
    {
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
d_tests_sa_array_common_init_slice_range_reverse
  Tests the `d_array_common_init_slice_range_reverse` function.
  Tests the following:
  - slicing a range in reverse order with positive indices
  - slicing a range in reverse order with negative indices
  - proper d_index validation and conversion
  Note: This function operates on arrays of pointers (void**).
*/
bool
d_tests_sa_array_common_init_slice_range_reverse
(
    struct d_test_counter* _test_info
)
{
    bool   test_result;
    bool   result;
    void*  elements;
    size_t count;

    int    val1, val2, val3, val4, val5;
    void*  ptr_arr[5];
    void** result_ptrs;

    // validate parameters
    if (!_test_info)
    {
        return false;
    }

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

    printf("Testing `d_array_common_init_slice_range_reverse`...\n");

    //=========================================================================
    // Test 1: Slice range reverse with indices (0 to 2)
    //   [&val1, &val2, &val3] reversed -> [&val3, &val2, &val1]
    //=========================================================================
    elements = NULL;
    count    = SIZE_MAX;

    result = d_array_common_init_slice_range_reverse(&elements,
                                                     &count,
                                                     sizeof(void*),
                                                     (const void**)ptr_arr,
                                                     5,
                                                     0,
                                                     2);

    result_ptrs = (void**)elements;

    test_result &= d_assert_standalone(
        ( (result == true)          &&
          (elements != NULL)        &&
          (count == 3)              &&
          (result_ptrs[0] == &val3) &&
          (result_ptrs[1] == &val2) &&
          (result_ptrs[2] == &val1) ),
        "slices range in reverse with positive indices",
        "failed with positive indices",
        _test_info
    );

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
                                                     (const void**)ptr_arr,
                                                     5,
                                                     -3,
                                                     -2);

    result_ptrs = (void**)elements;

    test_result &= d_assert_standalone(
        ( (result == true)          &&
          (elements != NULL)        &&
          (count == 2)              &&
          (result_ptrs[0] == &val4) &&
          (result_ptrs[1] == &val3) ),
        "handles negative indices correctly",
        "failed with negative indices",
        _test_info
    );

    // cleanup
    if (elements)
    {
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


/******************************************************************************
 * INITIALIZATION TEST GROUP RUNNER
 *****************************************************************************/

/*
d_tests_sa_array_common_run_init_tests
  Runs all initialization function tests and returns overall pass/fail status.

Parameter(s):
  _test_info: pointer to test counter for tracking results
Return:
  A boolean value corresponding to either:
  - true, if all initialization tests passed, or
  - false, if any test failed.
*/
bool
d_tests_sa_array_common_run_init_tests
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
    printf("Running Initialization Function Tests\n");
    printf("----------------------------------------\n");

    // run all initialization tests in header order
    overall_result &= d_tests_sa_array_common_init_sized(_test_info);
    overall_result &= d_tests_sa_array_common_init_from_array(_test_info);
    overall_result &= d_tests_sa_array_common_init_from_args(_test_info);
    overall_result &= d_tests_sa_array_common_init_copy(_test_info);
    overall_result &= d_tests_sa_array_common_init_copy_reverse(_test_info);
    overall_result &= d_tests_sa_array_common_init_copy_range(_test_info);
    overall_result &= d_tests_sa_array_common_init_copy_range_reverse(_test_info);
    overall_result &= d_tests_sa_array_common_init_fill(_test_info);
    overall_result &= d_tests_sa_array_common_init_slice(_test_info);
    overall_result &= d_tests_sa_array_common_init_slice_reverse(_test_info);
    overall_result &= d_tests_sa_array_common_init_slice_range(_test_info);
    overall_result &= d_tests_sa_array_common_init_slice_range_reverse(_test_info);

    return overall_result;
}
