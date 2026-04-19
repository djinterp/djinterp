#include "./ptr_array_tests_sa.h"


/******************************************************************************
 * HELPER FUNCTIONS
 *****************************************************************************/

// helper comparator for int pointer values
static int
int_ptr_comparator
(
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
d_tests_sa_ptr_array_new
  Tests the d_ptr_array_new function.
  Tests the following:
  - creation with zero initial size
  - creation with positive initial size
  - array structure is properly initialized
*/
bool
d_tests_sa_ptr_array_new
(
    struct d_test_counter* _counter
)
{
    bool                 result;
    struct d_ptr_array*  arr;

    result = true;

    // test 1: creation with zero initial size
    arr = d_ptr_array_new(0);
    result = d_assert_standalone(
        arr != NULL,
        "new_zero_size",
        "Should create array with zero initial size",
        _counter) && result;

    if (arr)
    {
        result = d_assert_standalone(
            arr->count == 0,
            "new_zero_count",
            "New array count should be 0",
            _counter) && result;

        d_ptr_array_free(arr);
    }

    // test 2: creation with positive initial size
    arr = d_ptr_array_new(10);
    result = d_assert_standalone(
        arr != NULL,
        "new_positive_size",
        "Should create array with positive initial size",
        _counter) && result;

    if (arr)
    {
        result = d_assert_standalone(
            arr->count == 0,
            "new_positive_count_zero",
            "New array count should still be 0 (capacity != count)",
            _counter) && result;

        result = d_assert_standalone(
            arr->elements != NULL,
            "new_elements_allocated",
            "Elements array should be allocated",
            _counter) && result;

        d_ptr_array_free(arr);
    }

    // test 3: creation with large initial size
    arr = d_ptr_array_new(1000);
    result = d_assert_standalone(
        arr != NULL,
        "new_large_size",
        "Should create array with large initial size",
        _counter) && result;

    if (arr)
    {
        d_ptr_array_free(arr);
    }

    return result;
}


/*
d_tests_sa_ptr_array_new_default_size
  Tests the d_ptr_array_new_default_size function.
  Tests the following:
  - creation with default capacity
  - array structure is properly initialized
*/
bool
d_tests_sa_ptr_array_new_default_size
(
    struct d_test_counter* _counter
)
{
    bool                 result;
    struct d_ptr_array*  arr;

    result = true;

    // test 1: creation with default capacity
    arr = d_ptr_array_new_default_size();
    result = d_assert_standalone(
        arr != NULL,
        "new_default_size",
        "Should create array with default capacity",
        _counter) && result;

    if (arr)
    {
        result = d_assert_standalone(
            arr->count == 0,
            "new_default_count",
            "New array count should be 0",
            _counter) && result;

        result = d_assert_standalone(
            arr->elements != NULL,
            "new_default_elements",
            "Elements array should be allocated",
            _counter) && result;

        d_ptr_array_free(arr);
    }

    return result;
}


/*
d_tests_sa_ptr_array_new_from_arr
  Tests the d_ptr_array_new_from_arr function.
  Tests the following:
  - NULL source with count > 0 returns NULL
  - NULL source with count == 0 returns valid empty array
  - valid source array is copied correctly
  - element pointers are preserved (shallow copy)
*/
bool
d_tests_sa_ptr_array_new_from_arr
(
    struct d_test_counter* _counter
)
{
    bool                 result;
    struct d_ptr_array*  arr;
    int                  values[3];
    const void*          source[3];

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
    result = d_assert_standalone(
        arr == NULL,
        "new_from_arr_null_source_nonzero",
        "NULL source with count > 0 should return NULL",
        _counter) && result;

    // test 2: NULL source with count == 0 returns valid array
    arr = d_ptr_array_new_from_arr(NULL, 0);
    result = d_assert_standalone(
        arr != NULL,
        "new_from_arr_null_source_zero",
        "NULL source with count == 0 should return valid array",
        _counter) && result;

    if (arr)
    {
        result = d_assert_standalone(
            arr->count == 0,
            "new_from_arr_null_empty",
            "Array should be empty",
            _counter) && result;

        d_ptr_array_free(arr);
    }

    // test 3: valid source array is copied correctly
    arr = d_ptr_array_new_from_arr(source, 3);
    result = d_assert_standalone(
        arr != NULL,
        "new_from_arr_valid",
        "Should create array from valid source",
        _counter) && result;

    if (arr)
    {
        result = d_assert_standalone(
            arr->count == 3,
            "new_from_arr_count",
            "Array should have 3 elements",
            _counter) && result;

        // test 4: element pointers are preserved (shallow copy)
        result = d_assert_standalone(
            arr->elements[0] == source[0] &&
            arr->elements[1] == source[1] &&
            arr->elements[2] == source[2],
            "new_from_arr_pointers",
            "Element pointers should match source pointers",
            _counter) && result;

        // verify values through pointers
        result = d_assert_standalone(
            *(int*)arr->elements[0] == 100 &&
            *(int*)arr->elements[1] == 200 &&
            *(int*)arr->elements[2] == 300,
            "new_from_arr_values",
            "Values through pointers should be correct",
            _counter) && result;

        d_ptr_array_free(arr);
    }

    return result;
}


/*
d_tests_sa_ptr_array_new_from_args
  Tests the d_ptr_array_new_from_args function.
  Tests the following:
  - creation with zero arguments
  - creation with multiple arguments
  - element pointers are correctly stored
*/
bool
d_tests_sa_ptr_array_new_from_args
(
    struct d_test_counter* _counter
)
{
    bool                 result;
    struct d_ptr_array*  arr;
    int                  values[4];

    result = true;

    // initialize test values
    values[0] = 10;
    values[1] = 20;
    values[2] = 30;
    values[3] = 40;

    // test 1: creation with zero arguments
    arr = d_ptr_array_new_from_args(0);
    result = d_assert_standalone(
        arr != NULL,
        "new_from_args_zero",
        "Should create array with zero arguments",
        _counter) && result;

    if (arr)
    {
        result = d_assert_standalone(
            arr->count == 0,
            "new_from_args_zero_count",
            "Array should be empty",
            _counter) && result;

        d_ptr_array_free(arr);
    }

    // test 2: creation with multiple arguments
    arr = d_ptr_array_new_from_args(4,
                                    &values[0],
                                    &values[1],
                                    &values[2],
                                    &values[3]);
    result = d_assert_standalone(
        arr != NULL,
        "new_from_args_multiple",
        "Should create array with multiple arguments",
        _counter) && result;

    if (arr)
    {
        result = d_assert_standalone(
            arr->count == 4,
            "new_from_args_count",
            "Array should have 4 elements",
            _counter) && result;

        // test 3: element pointers are correctly stored
        result = d_assert_standalone(
            arr->elements[0] == &values[0] &&
            arr->elements[1] == &values[1] &&
            arr->elements[2] == &values[2] &&
            arr->elements[3] == &values[3],
            "new_from_args_pointers",
            "Element pointers should match argument pointers",
            _counter) && result;

        // verify values
        result = d_assert_standalone(
            *(int*)arr->elements[0] == 10 &&
            *(int*)arr->elements[1] == 20 &&
            *(int*)arr->elements[2] == 30 &&
            *(int*)arr->elements[3] == 40,
            "new_from_args_values",
            "Values should be correct",
            _counter) && result;

        d_ptr_array_free(arr);
    }

    return result;
}


/*
d_tests_sa_ptr_array_new_copy
  Tests the d_ptr_array_new_copy function.
  Tests the following:
  - NULL input returns NULL
  - valid input creates a proper copy
  - copy has same count as original
  - copy has same element pointers (shallow copy)
*/
bool
d_tests_sa_ptr_array_new_copy
(
    struct d_test_counter* _counter
)
{
    bool                 result;
    struct d_ptr_array*  original;
    struct d_ptr_array*  copy;
    int                  values[3];

    result = true;

    // initialize test values
    values[0] = 111;
    values[1] = 222;
    values[2] = 333;

    // test 1: NULL input returns NULL
    copy = d_ptr_array_new_copy(NULL);
    result = d_assert_standalone(
        copy == NULL,
        "new_copy_null",
        "NULL input should return NULL",
        _counter) && result;

    // test 2: valid input creates a proper copy
    original = d_ptr_array_new_from_args(3,
                                         &values[0],
                                         &values[1],
                                         &values[2]);

    if (original)
    {
        copy = d_ptr_array_new_copy(original);
        result = d_assert_standalone(
            copy != NULL,
            "new_copy_valid",
            "Should create copy from valid original",
            _counter) && result;

        if (copy)
        {
            // test 3: copy has same count as original
            result = d_assert_standalone(
                copy->count == original->count,
                "new_copy_count",
                "Copy count should match original",
                _counter) && result;

            // test 4: copy has same element pointers (shallow copy)
            result = d_assert_standalone(
                copy->elements[0] == original->elements[0] &&
                copy->elements[1] == original->elements[1] &&
                copy->elements[2] == original->elements[2],
                "new_copy_pointers",
                "Copy should have same element pointers",
                _counter) && result;

            // verify arrays are independent
            result = d_assert_standalone(
                copy->elements != original->elements,
                "new_copy_independent",
                "Copy elements array should be independent",
                _counter) && result;

            d_ptr_array_free(copy);
        }

        d_ptr_array_free(original);
    }

    return result;
}


/*
d_tests_sa_ptr_array_new_merge
  Tests the d_ptr_array_new_merge function.
  Tests the following:
  - merge with zero count returns empty array
  - merge with single array
  - merge with multiple arrays
  - merge with NULL array in list (should be skipped)
*/
bool
d_tests_sa_ptr_array_new_merge
(
    struct d_test_counter* _counter
)
{
    bool                 result;
    struct d_ptr_array*  arr1;
    struct d_ptr_array*  arr2;
    struct d_ptr_array*  merged;
    int                  values1[2];
    int                  values2[3];

    result = true;

    // initialize test values
    values1[0] = 10;
    values1[1] = 20;
    values2[0] = 30;
    values2[1] = 40;
    values2[2] = 50;

    // test 1: merge with zero count returns empty array
    merged = d_ptr_array_new_merge(0);
    result = d_assert_standalone(
        merged != NULL,
        "new_merge_zero",
        "Merge with zero count should return valid array",
        _counter) && result;

    if (merged)
    {
        result = d_assert_standalone(
            merged->count == 0,
            "new_merge_zero_empty",
            "Merged array should be empty",
            _counter) && result;

        d_ptr_array_free(merged);
    }

    // create test arrays
    arr1 = d_ptr_array_new_from_args(2, &values1[0], &values1[1]);
    arr2 = d_ptr_array_new_from_args(3, &values2[0], &values2[1], &values2[2]);

    if (arr1 && arr2)
    {
        // test 2: merge with single array
        merged = d_ptr_array_new_merge(1, arr1);
        result = d_assert_standalone(
            merged != NULL,
            "new_merge_single",
            "Should merge single array",
            _counter) && result;

        if (merged)
        {
            result = d_assert_standalone(
                merged->count == 2,
                "new_merge_single_count",
                "Merged array should have 2 elements",
                _counter) && result;

            d_ptr_array_free(merged);
        }

        // test 3: merge with multiple arrays
        merged = d_ptr_array_new_merge(2, arr1, arr2);
        result = d_assert_standalone(
            merged != NULL,
            "new_merge_multiple",
            "Should merge multiple arrays",
            _counter) && result;

        if (merged)
        {
            result = d_assert_standalone(
                merged->count == 5,
                "new_merge_multiple_count",
                "Merged array should have 5 elements",
                _counter) && result;

            // verify order: arr1 elements first, then arr2
            result = d_assert_standalone(
                merged->elements[0] == &values1[0] &&
                merged->elements[1] == &values1[1] &&
                merged->elements[2] == &values2[0] &&
                merged->elements[3] == &values2[1] &&
                merged->elements[4] == &values2[2],
                "new_merge_order",
                "Merged elements should be in correct order",
                _counter) && result;

            d_ptr_array_free(merged);
        }

        // test 4: merge with NULL array in list (should be skipped)
        merged = d_ptr_array_new_merge(3, arr1, NULL, arr2);
        result = d_assert_standalone(
            merged != NULL,
            "new_merge_with_null",
            "Should merge arrays with NULL in list",
            _counter) && result;

        if (merged)
        {
            result = d_assert_standalone(
                merged->count == 5,
                "new_merge_with_null_count",
                "Merged array should skip NULL and have 5 elements",
                _counter) && result;

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
d_tests_sa_ptr_array_new_slice
  Tests the d_ptr_array_new_slice function.
  Tests the following:
  - NULL input returns NULL
  - valid positive start index
  - valid negative start index
  - out-of-bounds start index
  - start at end returns empty array
*/
bool
d_tests_sa_ptr_array_new_slice
(
    struct d_test_counter* _counter
)
{
    bool                 result;
    struct d_ptr_array*  original;
    struct d_ptr_array*  slice;
    int                  values[5];

    result = true;

    // initialize test values
    values[0] = 1;
    values[1] = 2;
    values[2] = 3;
    values[3] = 4;
    values[4] = 5;

    // test 1: NULL input returns NULL
    slice = d_ptr_array_new_slice(NULL, 0);
    result = d_assert_standalone(
        slice == NULL,
        "new_slice_null",
        "NULL input should return NULL",
        _counter) && result;

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
        result = d_assert_standalone(
            slice != NULL,
            "new_slice_positive",
            "Should create slice with positive start",
            _counter) && result;

        if (slice)
        {
            result = d_assert_standalone(
                slice->count == 3,
                "new_slice_positive_count",
                "Slice should have 3 elements",
                _counter) && result;

            result = d_assert_standalone(
                slice->elements[0] == &values[2] &&
                slice->elements[1] == &values[3] &&
                slice->elements[2] == &values[4],
                "new_slice_positive_elements",
                "Slice should contain elements from index 2 onwards",
                _counter) && result;

            d_ptr_array_free(slice);
        }

        // test 3: valid negative start index
        slice = d_ptr_array_new_slice(original, -2);
        result = d_assert_standalone(
            slice != NULL,
            "new_slice_negative",
            "Should create slice with negative start",
            _counter) && result;

        if (slice)
        {
            result = d_assert_standalone(
                slice->count == 2,
                "new_slice_negative_count",
                "Slice should have 2 elements (last 2)",
                _counter) && result;

            result = d_assert_standalone(
                slice->elements[0] == &values[3] &&
                slice->elements[1] == &values[4],
                "new_slice_negative_elements",
                "Slice should contain last 2 elements",
                _counter) && result;

            d_ptr_array_free(slice);
        }

        // test 4: start at 0 returns full copy
        slice = d_ptr_array_new_slice(original, 0);
        result = d_assert_standalone(
            slice != NULL && slice->count == 5,
            "new_slice_from_start",
            "Slice from 0 should have all elements",
            _counter) && result;

        if (slice)
        {
            d_ptr_array_free(slice);
        }

        d_ptr_array_free(original);
    }

    return result;
}


/*
d_tests_sa_ptr_array_new_slice_range
  Tests the d_ptr_array_new_slice_range function.
  Tests the following:
  - NULL input returns NULL
  - valid positive range
  - valid negative range
  - mixed positive and negative indices
  - single element range
  - invalid range (end before start)
*/
bool
d_tests_sa_ptr_array_new_slice_range
(
    struct d_test_counter* _counter
)
{
    bool                 result;
    struct d_ptr_array*  original;
    struct d_ptr_array*  slice;
    int                  values[5];

    result = true;

    // initialize test values
    values[0] = 10;
    values[1] = 20;
    values[2] = 30;
    values[3] = 40;
    values[4] = 50;

    // test 1: NULL input returns NULL
    slice = d_ptr_array_new_slice_range(NULL, 0, 2);
    result = d_assert_standalone(
        slice == NULL,
        "new_slice_range_null",
        "NULL input should return NULL",
        _counter) && result;

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
        result = d_assert_standalone(
            slice != NULL,
            "new_slice_range_positive",
            "Should create slice with positive range",
            _counter) && result;

        if (slice)
        {
            result = d_assert_standalone(
                slice->count == 3,
                "new_slice_range_positive_count",
                "Slice should have 3 elements (indices 1, 2, 3)",
                _counter) && result;

            result = d_assert_standalone(
                slice->elements[0] == &values[1] &&
                slice->elements[1] == &values[2] &&
                slice->elements[2] == &values[3],
                "new_slice_range_positive_elements",
                "Slice should contain elements 1-3",
                _counter) && result;

            d_ptr_array_free(slice);
        }

        // test 3: valid negative range
        slice = d_ptr_array_new_slice_range(original, -3, -1);
        result = d_assert_standalone(
            slice != NULL,
            "new_slice_range_negative",
            "Should create slice with negative range",
            _counter) && result;

        if (slice)
        {
            result = d_assert_standalone(
                slice->count == 3,
                "new_slice_range_negative_count",
                "Slice should have 3 elements (last 3)",
                _counter) && result;

            result = d_assert_standalone(
                slice->elements[0] == &values[2] &&
                slice->elements[1] == &values[3] &&
                slice->elements[2] == &values[4],
                "new_slice_range_negative_elements",
                "Slice should contain last 3 elements",
                _counter) && result;

            d_ptr_array_free(slice);
        }

        // test 4: mixed positive and negative indices
        slice = d_ptr_array_new_slice_range(original, 1, -1);
        result = d_assert_standalone(
            slice != NULL,
            "new_slice_range_mixed",
            "Should create slice with mixed indices",
            _counter) && result;

        if (slice)
        {
            result = d_assert_standalone(
                slice->count == 4,
                "new_slice_range_mixed_count",
                "Slice from index 1 to -1 should have 4 elements",
                _counter) && result;

            d_ptr_array_free(slice);
        }

        // test 5: single element range
        slice = d_ptr_array_new_slice_range(original, 2, 2);
        result = d_assert_standalone(
            slice != NULL,
            "new_slice_range_single",
            "Should create single-element slice",
            _counter) && result;

        if (slice)
        {
            result = d_assert_standalone(
                slice->count == 1 && slice->elements[0] == &values[2],
                "new_slice_range_single_element",
                "Single-element slice should have element at index 2",
                _counter) && result;

            d_ptr_array_free(slice);
        }

        d_ptr_array_free(original);
    }

    return result;
}


/*
d_tests_sa_ptr_array_constructor_all
  Aggregation function that runs all constructor tests.
*/
bool
d_tests_sa_ptr_array_constructor_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Constructor Functions\n");
    printf("  --------------------------------\n");

    result = d_tests_sa_ptr_array_new(_counter) && result;
    result = d_tests_sa_ptr_array_new_default_size(_counter) && result;
    result = d_tests_sa_ptr_array_new_from_arr(_counter) && result;
    result = d_tests_sa_ptr_array_new_from_args(_counter) && result;
    result = d_tests_sa_ptr_array_new_copy(_counter) && result;
    result = d_tests_sa_ptr_array_new_merge(_counter) && result;
    result = d_tests_sa_ptr_array_new_slice(_counter) && result;
    result = d_tests_sa_ptr_array_new_slice_range(_counter) && result;

    return result;
}
