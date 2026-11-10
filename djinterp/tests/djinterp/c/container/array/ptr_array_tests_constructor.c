/*******************************************************************************
* djinterp [container]                             ptr_array_tests_constructor.c
*
*
* path:      /tests/djinterp/c/container/array/ptr_array_tests_constructor.c
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

    // compare the integer values pointed to by the pointers
    a = *(const int*)_a;
    b = *(const int*)_b;

    return (a > b) - (a < b);
}


/******************************************************************************
 * I. CONSTRUCTOR FUNCTION TESTS
 *****************************************************************************/

/*
d_tests_ptr_array_new
  Tests the d_ptr_array_new function.
  Tests the following:
  - creation with zero initial size
  - creation with positive initial size
  - array structure is properly initialized
*/
bool d_tests_ptr_array_new(
    struct dj_test_suite* _suite
)
{
    bool                 result;
    struct dj_ptr_array*  arr;

    d_test_suite_test(_suite, "ptr_array_new");

    result = true;

    // test 1: creation with zero initial size
    arr = d_ptr_array_new(0);
    result = D_TEST_CHECK_MSG(_suite,
                              "new_zero_size",
                              arr != NULL,
                              "Should create array with zero initial size") && result;

    if (arr)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "new_zero_count",
                                  arr->count == 0,
                                  "New array count should be 0") && result;

        d_ptr_array_free(arr);
    }

    // test 2: creation with positive initial size
    arr = d_ptr_array_new(10);
    result = D_TEST_CHECK_MSG(_suite,
                              "new_positive_size",
                              arr != NULL,
                              "Should create array with positive initial size") && result;

    if (arr)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "new_positive_count_zero",
                                  arr->count == 0,
                                  "New array count should still be 0 (capacity != count)") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "new_elements_allocated",
                                  arr->elements != NULL,
                                  "Elements array should be allocated") && result;

        d_ptr_array_free(arr);
    }

    // test 3: creation with large initial size
    arr = d_ptr_array_new(1000);
    result = D_TEST_CHECK_MSG(_suite,
                              "new_large_size",
                              arr != NULL,
                              "Should create array with large initial size") && result;

    if (arr)
    {
        d_ptr_array_free(arr);
    }

    return result;
}


/*
d_tests_ptr_array_new_default_size
  Tests the d_ptr_array_new_default_size function.
  Tests the following:
  - creation with default capacity
  - array structure is properly initialized
*/
bool d_tests_ptr_array_new_default_size(
    struct dj_test_suite* _suite
)
{
    bool                 result;
    struct dj_ptr_array*  arr;

    d_test_suite_test(_suite, "ptr_array_new_default_size");

    result = true;

    // test 1: creation with default capacity
    arr = d_ptr_array_new_default_size();
    result = D_TEST_CHECK_MSG(_suite,
                              "new_default_size",
                              arr != NULL,
                              "Should create array with default capacity") && result;

    if (arr)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "new_default_count",
                                  arr->count == 0,
                                  "New array count should be 0") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "new_default_elements",
                                  arr->elements != NULL,
                                  "Elements array should be allocated") && result;

        d_ptr_array_free(arr);
    }

    return result;
}


/*
d_tests_ptr_array_new_from_arr
  Tests the d_ptr_array_new_from_arr function.
  Tests the following:
  - NULL source with count > 0 returns NULL
  - NULL source with count == 0 returns valid empty array
  - valid source array is copied correctly
  - element pointers are preserved (shallow copy)
*/
bool d_tests_ptr_array_new_from_arr(
    struct dj_test_suite* _suite
)
{
    bool                 result;
    struct dj_ptr_array*  arr;
    int                  values[3];
    const void*          source[3];

    d_test_suite_test(_suite, "ptr_array_new_from_arr");

    result = true;

    // initialize test values
    values[0] = 100;
    values[1] = 200;
    values[2] = 300;
    source[0] = &values[0];
    source[1] = &values[1];
    source[2] = &values[2];

    // test 1: NULL source with count > 0 returns NULL
    arr = d_ptr_array_new_from_arr(NULL, 5);
    result = D_TEST_CHECK_MSG(_suite,
                              "new_from_arr_null_source_nonzero",
                              arr == NULL,
                              "NULL source with count > 0 should return NULL") && result;

    // test 2: NULL source with count == 0 returns valid array
    arr = d_ptr_array_new_from_arr(NULL, 0);
    result = D_TEST_CHECK_MSG(_suite,
                              "new_from_arr_null_source_zero",
                              arr != NULL,
                              "NULL source with count == 0 should return valid array") && result;

    if (arr)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "new_from_arr_null_empty",
                                  arr->count == 0,
                                  "Array should be empty") && result;

        d_ptr_array_free(arr);
    }

    // test 3: valid source array is copied correctly
    arr = d_ptr_array_new_from_arr(source, 3);
    result = D_TEST_CHECK_MSG(_suite,
                              "new_from_arr_valid",
                              arr != NULL,
                              "Should create array from valid source") && result;

    if (arr)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "new_from_arr_count",
                                  arr->count == 3,
                                  "Array should have 3 elements") && result;

        // test 4: element pointers are preserved (shallow copy)
        result = D_TEST_CHECK_MSG(_suite,
                                  "new_from_arr_pointers",
                                  arr->elements[0] == source[0] && arr->elements[1] == source[1] && arr->elements[2] == source[2],
                                  "Element pointers should match source pointers") && result;

        // verify values through pointers
        result = D_TEST_CHECK_MSG(_suite,
                                  "new_from_arr_values",
                                  *(int*)arr->elements[0] == 100 && *(int*)arr->elements[1] == 200 && *(int*)arr->elements[2] == 300,
                                  "Values through pointers should be correct") && result;

        d_ptr_array_free(arr);
    }

    return result;
}


/*
d_tests_ptr_array_new_from_args
  Tests the d_ptr_array_new_from_args function.
  Tests the following:
  - creation with zero arguments
  - creation with multiple arguments
  - element pointers are correctly stored
*/
bool d_tests_ptr_array_new_from_args(
    struct dj_test_suite* _suite
)
{
    bool                 result;
    struct dj_ptr_array*  arr;
    int                  values[4];

    d_test_suite_test(_suite, "ptr_array_new_from_args");

    result = true;

    // initialize test values
    values[0] = 10;
    values[1] = 20;
    values[2] = 30;
    values[3] = 40;

    // test 1: creation with zero arguments
    arr = d_ptr_array_new_from_args(0);
    result = D_TEST_CHECK_MSG(_suite,
                              "new_from_args_zero",
                              arr != NULL,
                              "Should create array with zero arguments") && result;

    if (arr)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "new_from_args_zero_count",
                                  arr->count == 0,
                                  "Array should be empty") && result;

        d_ptr_array_free(arr);
    }

    // test 2: creation with multiple arguments
    arr = d_ptr_array_new_from_args(4,
                                    &values[0],
                                    &values[1],
                                    &values[2],
                                    &values[3]);
    result = D_TEST_CHECK_MSG(_suite,
                              "new_from_args_multiple",
                              arr != NULL,
                              "Should create array with multiple arguments") && result;

    if (arr)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "new_from_args_count",
                                  arr->count == 4,
                                  "Array should have 4 elements") && result;

        // test 3: element pointers are correctly stored
        result = D_TEST_CHECK_MSG(_suite,
                                  "new_from_args_pointers",
                                  arr->elements[0] == &values[0] && arr->elements[1] == &values[1] && arr->elements[2] == &values[2] && arr->elements[3] == &values[3],
                                  "Element pointers should match argument pointers") && result;

        // verify values
        result = D_TEST_CHECK_MSG(_suite,
                                  "new_from_args_values",
                                  *(int*)arr->elements[0] == 10 && *(int*)arr->elements[1] == 20 && *(int*)arr->elements[2] == 30 && *(int*)arr->elements[3] == 40,
                                  "Values should be correct") && result;

        d_ptr_array_free(arr);
    }

    return result;
}


/*
d_tests_ptr_array_new_copy
  Tests the d_ptr_array_new_copy function.
  Tests the following:
  - NULL input returns NULL
  - valid input creates a proper copy
  - copy has same count as original
  - copy has same element pointers (shallow copy)
*/
bool d_tests_ptr_array_new_copy(
    struct dj_test_suite* _suite
)
{
    bool                 result;
    struct dj_ptr_array*  original;
    struct dj_ptr_array*  copy;
    int                  values[3];

    d_test_suite_test(_suite, "ptr_array_new_copy");

    result = true;

    // initialize test values
    values[0] = 111;
    values[1] = 222;
    values[2] = 333;

    // test 1: NULL input returns NULL
    copy = d_ptr_array_new_copy(NULL);
    result = D_TEST_CHECK_MSG(_suite,
                              "new_copy_null",
                              copy == NULL,
                              "NULL input should return NULL") && result;

    // test 2: valid input creates a proper copy
    original = d_ptr_array_new_from_args(3,
                                         &values[0],
                                         &values[1],
                                         &values[2]);

    if (original)
    {
        copy = d_ptr_array_new_copy(original);
        result = D_TEST_CHECK_MSG(_suite,
                                  "new_copy_valid",
                                  copy != NULL,
                                  "Should create copy from valid original") && result;

        if (copy)
        {
            // test 3: copy has same count as original
            result = D_TEST_CHECK_MSG(_suite,
                                      "new_copy_count",
                                      copy->count == original->count,
                                      "Copy count should match original") && result;

            // test 4: copy has same element pointers (shallow copy)
            result = D_TEST_CHECK_MSG(_suite,
                                      "new_copy_pointers",
                                      copy->elements[0] == original->elements[0] && copy->elements[1] == original->elements[1] && copy->elements[2] == original->elements[2],
                                      "Copy should have same element pointers") && result;

            // verify arrays are independent
            result = D_TEST_CHECK_MSG(_suite,
                                      "new_copy_independent",
                                      copy->elements != original->elements,
                                      "Copy elements array should be independent") && result;

            d_ptr_array_free(copy);
        }

        d_ptr_array_free(original);
    }

    return result;
}


/*
d_tests_ptr_array_new_merge
  Tests the d_ptr_array_new_merge function.
  Tests the following:
  - merge with zero count returns empty array
  - merge with single array
  - merge with multiple arrays
  - merge with NULL array in list (should be skipped)
*/
bool d_tests_ptr_array_new_merge(
    struct dj_test_suite* _suite
)
{
    bool                 result;
    struct dj_ptr_array*  arr1;
    struct dj_ptr_array*  arr2;
    struct dj_ptr_array*  merged;
    int                  values1[2];
    int                  values2[3];

    d_test_suite_test(_suite, "ptr_array_new_merge");

    result = true;

    // initialize test values
    values1[0] = 10;
    values1[1] = 20;
    values2[0] = 30;
    values2[1] = 40;
    values2[2] = 50;

    // test 1: merge with zero count returns empty array
    merged = d_ptr_array_new_merge(0);
    result = D_TEST_CHECK_MSG(_suite,
                              "new_merge_zero",
                              merged != NULL,
                              "Merge with zero count should return valid array") && result;

    if (merged)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "new_merge_zero_empty",
                                  merged->count == 0,
                                  "Merged array should be empty") && result;

        d_ptr_array_free(merged);
    }

    // create test arrays
    arr1 = d_ptr_array_new_from_args(2, &values1[0], &values1[1]);
    arr2 = d_ptr_array_new_from_args(3, &values2[0], &values2[1], &values2[2]);

    if (arr1 && arr2)
    {
        // test 2: merge with single array
        merged = d_ptr_array_new_merge(1, arr1);
        result = D_TEST_CHECK_MSG(_suite,
                                  "new_merge_single",
                                  merged != NULL,
                                  "Should merge single array") && result;

        if (merged)
        {
            result = D_TEST_CHECK_MSG(_suite,
                                      "new_merge_single_count",
                                      merged->count == 2,
                                      "Merged array should have 2 elements") && result;

            d_ptr_array_free(merged);
        }

        // test 3: merge with multiple arrays
        merged = d_ptr_array_new_merge(2, arr1, arr2);
        result = D_TEST_CHECK_MSG(_suite,
                                  "new_merge_multiple",
                                  merged != NULL,
                                  "Should merge multiple arrays") && result;

        if (merged)
        {
            result = D_TEST_CHECK_MSG(_suite,
                                      "new_merge_multiple_count",
                                      merged->count == 5,
                                      "Merged array should have 5 elements") && result;

            // verify order: arr1 elements first, then arr2
            result = D_TEST_CHECK_MSG(_suite,
                                      "new_merge_order",
                                      merged->elements[0] == &values1[0] && merged->elements[1] == &values1[1] && merged->elements[2] == &values2[0] && merged->elements[3] == &values2[1] && merged->elements[4] == &values2[2],
                                      "Merged elements should be in correct order") && result;

            d_ptr_array_free(merged);
        }

        // test 4: merge with NULL array in list (should be skipped)
        merged = d_ptr_array_new_merge(3, arr1, NULL, arr2);
        result = D_TEST_CHECK_MSG(_suite,
                                  "new_merge_with_null",
                                  merged != NULL,
                                  "Should merge arrays with NULL in list") && result;

        if (merged)
        {
            result = D_TEST_CHECK_MSG(_suite,
                                      "new_merge_with_null_count",
                                      merged->count == 5,
                                      "Merged array should skip NULL and have 5 elements") && result;

            d_ptr_array_free(merged);
        }
    }

    // cleanup
    if (arr1)
    {
        d_ptr_array_free(arr1);
    }

    if (arr2)
    {
        d_ptr_array_free(arr2);
    }

    return result;
}


/*
d_tests_ptr_array_new_slice
  Tests the d_ptr_array_new_slice function.
  Tests the following:
  - NULL input returns NULL
  - valid positive start index
  - valid negative start index
  - out-of-bounds start index
  - start at end returns empty array
*/
bool d_tests_ptr_array_new_slice(
    struct dj_test_suite* _suite
)
{
    bool                 result;
    struct dj_ptr_array*  original;
    struct dj_ptr_array*  slice;
    int                  values[5];

    d_test_suite_test(_suite, "ptr_array_new_slice");

    result = true;

    // initialize test values
    values[0] = 1;
    values[1] = 2;
    values[2] = 3;
    values[3] = 4;
    values[4] = 5;

    // test 1: NULL input returns NULL
    slice = d_ptr_array_new_slice(NULL, 0);
    result = D_TEST_CHECK_MSG(_suite,
                              "new_slice_null",
                              slice == NULL,
                              "NULL input should return NULL") && result;

    // create test array
    original = d_ptr_array_new_from_args(5,
                                         &values[0],
                                         &values[1],
                                         &values[2],
                                         &values[3],
                                         &values[4]);

    if (original)
    {
        // test 2: valid positive start index
        slice = d_ptr_array_new_slice(original, 2);
        result = D_TEST_CHECK_MSG(_suite,
                                  "new_slice_positive",
                                  slice != NULL,
                                  "Should create slice with positive start") && result;

        if (slice)
        {
            result = D_TEST_CHECK_MSG(_suite,
                                      "new_slice_positive_count",
                                      slice->count == 3,
                                      "Slice should have 3 elements") && result;

            result = D_TEST_CHECK_MSG(_suite,
                                      "new_slice_positive_elements",
                                      slice->elements[0] == &values[2] && slice->elements[1] == &values[3] && slice->elements[2] == &values[4],
                                      "Slice should contain elements from index 2 onwards") && result;

            d_ptr_array_free(slice);
        }

        // test 3: valid negative start index
        slice = d_ptr_array_new_slice(original, -2);
        result = D_TEST_CHECK_MSG(_suite,
                                  "new_slice_negative",
                                  slice != NULL,
                                  "Should create slice with negative start") && result;

        if (slice)
        {
            result = D_TEST_CHECK_MSG(_suite,
                                      "new_slice_negative_count",
                                      slice->count == 2,
                                      "Slice should have 2 elements (last 2)") && result;

            result = D_TEST_CHECK_MSG(_suite,
                                      "new_slice_negative_elements",
                                      slice->elements[0] == &values[3] && slice->elements[1] == &values[4],
                                      "Slice should contain last 2 elements") && result;

            d_ptr_array_free(slice);
        }

        // test 4: start at 0 returns full copy
        slice = d_ptr_array_new_slice(original, 0);
        result = D_TEST_CHECK_MSG(_suite,
                                  "new_slice_from_start",
                                  slice != NULL && slice->count == 5,
                                  "Slice from 0 should have all elements") && result;

        if (slice)
        {
            d_ptr_array_free(slice);
        }

        d_ptr_array_free(original);
    }

    return result;
}


/*
d_tests_ptr_array_new_slice_range
  Tests the d_ptr_array_new_slice_range function.
  Tests the following:
  - NULL input returns NULL
  - valid positive range
  - valid negative range
  - mixed positive and negative indices
  - single element range
  - invalid range (end before start)
*/
bool d_tests_ptr_array_new_slice_range(
    struct dj_test_suite* _suite
)
{
    bool                 result;
    struct dj_ptr_array*  original;
    struct dj_ptr_array*  slice;
    int                  values[5];

    d_test_suite_test(_suite, "ptr_array_new_slice_range");

    result = true;

    // initialize test values
    values[0] = 10;
    values[1] = 20;
    values[2] = 30;
    values[3] = 40;
    values[4] = 50;

    // test 1: NULL input returns NULL
    slice = d_ptr_array_new_slice_range(NULL, 0, 2);
    result = D_TEST_CHECK_MSG(_suite,
                              "new_slice_range_null",
                              slice == NULL,
                              "NULL input should return NULL") && result;

    // create test array
    original = d_ptr_array_new_from_args(5,
                                         &values[0],
                                         &values[1],
                                         &values[2],
                                         &values[3],
                                         &values[4]);

    if (original)
    {
        // test 2: valid positive range
        slice = d_ptr_array_new_slice_range(original, 1, 3);
        result = D_TEST_CHECK_MSG(_suite,
                                  "new_slice_range_positive",
                                  slice != NULL,
                                  "Should create slice with positive range") && result;

        if (slice)
        {
            result = D_TEST_CHECK_MSG(_suite,
                                      "new_slice_range_positive_count",
                                      slice->count == 3,
                                      "Slice should have 3 elements (indices 1, 2, 3)") && result;

            result = D_TEST_CHECK_MSG(_suite,
                                      "new_slice_range_positive_elements",
                                      slice->elements[0] == &values[1] && slice->elements[1] == &values[2] && slice->elements[2] == &values[3],
                                      "Slice should contain elements 1-3") && result;

            d_ptr_array_free(slice);
        }

        // test 3: valid negative range
        slice = d_ptr_array_new_slice_range(original, -3, -1);
        result = D_TEST_CHECK_MSG(_suite,
                                  "new_slice_range_negative",
                                  slice != NULL,
                                  "Should create slice with negative range") && result;

        if (slice)
        {
            result = D_TEST_CHECK_MSG(_suite,
                                      "new_slice_range_negative_count",
                                      slice->count == 3,
                                      "Slice should have 3 elements (last 3)") && result;

            result = D_TEST_CHECK_MSG(_suite,
                                      "new_slice_range_negative_elements",
                                      slice->elements[0] == &values[2] && slice->elements[1] == &values[3] && slice->elements[2] == &values[4],
                                      "Slice should contain last 3 elements") && result;

            d_ptr_array_free(slice);
        }

        // test 4: mixed positive and negative indices
        slice = d_ptr_array_new_slice_range(original, 1, -1);
        result = D_TEST_CHECK_MSG(_suite,
                                  "new_slice_range_mixed",
                                  slice != NULL,
                                  "Should create slice with mixed indices") && result;

        if (slice)
        {
            result = D_TEST_CHECK_MSG(_suite,
                                      "new_slice_range_mixed_count",
                                      slice->count == 4,
                                      "Slice from index 1 to -1 should have 4 elements") && result;

            d_ptr_array_free(slice);
        }

        // test 5: single element range
        slice = d_ptr_array_new_slice_range(original, 2, 2);
        result = D_TEST_CHECK_MSG(_suite,
                                  "new_slice_range_single",
                                  slice != NULL,
                                  "Should create single-element slice") && result;

        if (slice)
        {
            result = D_TEST_CHECK_MSG(_suite,
                                      "new_slice_range_single_element",
                                      slice->count == 1 && slice->elements[0] == &values[2],
                                      "Single-element slice should have element at index 2") && result;

            d_ptr_array_free(slice);
        }

        d_ptr_array_free(original);
    }

    return result;
}


/*
d_tests_ptr_array_constructor_all
  Aggregation function that runs all constructor tests.
*/
bool d_tests_ptr_array_constructor_all(
    struct dj_test_suite* _suite
)
{
    bool result;

    d_test_suite_block(_suite, "ptr_array_constructor_all");

    result = true;

    result = d_tests_ptr_array_new(_suite) && result;
    result = d_tests_ptr_array_new_default_size(_suite) && result;
    result = d_tests_ptr_array_new_from_arr(_suite) && result;
    result = d_tests_ptr_array_new_from_args(_suite) && result;
    result = d_tests_ptr_array_new_copy(_suite) && result;
    result = d_tests_ptr_array_new_merge(_suite) && result;
    result = d_tests_ptr_array_new_slice(_suite) && result;
    result = d_tests_ptr_array_new_slice_range(_suite) && result;

    return result;
}
