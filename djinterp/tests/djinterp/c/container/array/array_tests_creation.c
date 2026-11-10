/*******************************************************************************
* djinterp [container]                                    array_tests_creation.c
*
* Unit tests for `array.h` dj_array creation functions.
* Part of the array suite, on DTest's C face.
*
*
* path:      /tests/djinterp/c/container/array/array_tests_creation.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.01.30
*                                                            revised: 2026.11.09
*******************************************************************************/

// djinterp
#include "./array_tests.h"
#include "../../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's C face


/*
d_tests_array_new
  Tests the `d_array_new` function.
  Tests the following:
  - normal allocation with valid parameters
  - zero element size rejection
  - zero initial size acceptance
*/
bool d_tests_array_new(
    struct dj_test_suite* _suite
)
{
    bool            test_result = true;
    struct dj_array* arr;

    d_test_suite_test(_suite, "array_new");

    // test normal allocation
    arr = d_array_new(sizeof(int), 10);

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "`d_array_new` creates array with valid parameters",
                                    arr != NULL,
                                    "`d_array_new` failed with valid parameters");

    if (arr)
    {
        test_result &= D_TEST_CHECK_MSG(_suite,
                                        "`d_array_new` initializes count to 0",
                                        arr->count == 0,
                                        "`d_array_new` did not initialize count correctly");

        test_result &= D_TEST_CHECK_MSG(_suite,
                                        "`d_array_new` allocates elements storage",
                                        arr->elements != NULL,
                                        "`d_array_new` did not allocate elements storage");

        d_array_free(arr);
    }

    // test zero element size
    arr = d_array_new(0, 10);

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "`d_array_new` rejects zero element size",
                                    arr == NULL,
                                    "`d_array_new` accepted zero element size");

    // test zero initial size
    arr = d_array_new(sizeof(int), 0);

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "`d_array_new` accepts zero initial size",
                                    arr != NULL,
                                    "`d_array_new` rejected zero initial size");

    if (arr)
    {
        d_array_free(arr);
    }

    return test_result;
}

/*
d_tests_array_new_default_size
  Tests the `d_array_new_default_size` function.
  Tests the following:
  - normal allocation with valid element size
  - zero element size rejection
*/
bool d_tests_array_new_default_size(
    struct dj_test_suite* _suite
)
{
    bool            test_result = true;
    struct dj_array* arr;

    d_test_suite_test(_suite, "array_new_default_size");

    // test normal allocation
    arr = d_array_new_default_size(sizeof(int));

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "`d_array_new_default_size` creates array with valid element size",
                                    arr != NULL,
                                    "`d_array_new_default_size` failed with valid element size");

    if (arr)
    {
        test_result &= D_TEST_CHECK_MSG(_suite,
                                        "`d_array_new_default_size` initializes count to 0",
                                        arr->count == 0,
                                        "`d_array_new_default_size` did not initialize count correctly");

        d_array_free(arr);
    }

    // test zero element size
    arr = d_array_new_default_size(0);

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "`d_array_new_default_size` rejects zero element size",
                                    arr == NULL,
                                    "`d_array_new_default_size` accepted zero element size");

    return test_result;
}

/*
d_tests_array_new_from_arr
  Tests the `d_array_new_from_arr` function.
  Tests the following:
  - normal creation from existing array
  - correct data copying
  - NULL source rejection
  - zero element size rejection
  - zero count acceptance
*/
bool d_tests_array_new_from_arr(
    struct dj_test_suite* _suite
)
{
    bool            test_result = true;
    struct dj_array* arr;
    int*            test_data;
    const size_t    test_count = 5;

    d_test_suite_test(_suite, "array_new_from_arr");

    d_test_create_test_data_int(&test_data, test_count);

    if (!test_data)
    {
        test_result &= D_TEST_CHECK_MSG(_suite,
                                        "test data creation succeeded",
                                        false,
                                        "failed to create test data for `d_array_new_from_arr`");

        return test_result;
    }

    // test normal creation from array
    arr = d_array_new_from_arr(sizeof(int), test_data, test_count);

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "`d_array_new_from_arr` creates array from valid source",
                                    arr != NULL,
                                    "`d_array_new_from_arr` failed with valid source");

    if (arr)
    {
        test_result &= D_TEST_CHECK_MSG(_suite,
                                        "`d_array_new_from_arr` sets correct count",
                                        arr->count == test_count,
                                        "`d_array_new_from_arr` did not set correct count");

        test_result &= D_TEST_CHECK_MSG(_suite,
                                        "`d_array_new_from_arr` copies data correctly",
                                        d_test_arrays_equal_int(arr->elements, test_data, test_count),
                                        "`d_array_new_from_arr` did not copy data correctly");

        d_array_free(arr);
    }

    // test null source data
    arr = d_array_new_from_arr(sizeof(int), NULL, test_count);

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "`d_array_new_from_arr` rejects NULL source data",
                                    arr == NULL,
                                    "`d_array_new_from_arr` accepted NULL source data");

    // test zero element size
    arr = d_array_new_from_arr(0, test_data, test_count);

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "`d_array_new_from_arr` rejects zero element size",
                                    arr == NULL,
                                    "`d_array_new_from_arr` accepted zero element size");

    // test zero count
    arr = d_array_new_from_arr(sizeof(int), test_data, 0);

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "`d_array_new_from_arr` accepts zero count",
                                    arr != NULL,
                                    "`d_array_new_from_arr` rejected zero count");

    if (arr)
    {
        test_result &= D_TEST_CHECK_MSG(_suite,
                                        "`d_array_new_from_arr` with zero count sets count to 0",
                                        arr->count == 0,
                                        "`d_array_new_from_arr` with zero count did not set count to 0");

        d_array_free(arr);
    }

    free(test_data);

    return test_result;
}

/*
d_tests_array_new_from_args
  Tests the `d_array_new_from_args` function.
  Tests the following:
  - normal creation from variadic arguments
  - correct data storage
  - zero element size rejection
  - zero argument count acceptance
*/
bool d_tests_array_new_from_args(
    struct dj_test_suite* _suite
)
{
    bool            test_result = true;
    struct dj_array* arr;
    int             expected[] = { 1, 2, 3, 4, 5 };
    const size_t    expected_count = 5;

    d_test_suite_test(_suite, "array_new_from_args");

    // test normal creation from variadic args
    arr = d_array_new_from_args(sizeof(int), expected_count, 1, 2, 3, 4, 5);

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "`d_array_new_from_args` creates array from variadic arguments",
                                    arr != NULL,
                                    "`d_array_new_from_args` failed with variadic arguments");

    if (arr)
    {
        test_result &= D_TEST_CHECK_MSG(_suite,
                                        "`d_array_new_from_args` sets correct count",
                                        arr->count == expected_count,
                                        "`d_array_new_from_args` did not set correct count");

        test_result &= D_TEST_CHECK_MSG(_suite,
                                        "`d_array_new_from_args` stores arguments correctly",
                                        d_test_arrays_equal_int(arr->elements, expected, expected_count),
                                        "`d_array_new_from_args` did not store arguments correctly");

        d_array_free(arr);
    }

    // test zero element size
    arr = d_array_new_from_args(0, expected_count, 1, 2, 3, 4, 5);

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "`d_array_new_from_args` rejects zero element size",
                                    arr == NULL,
                                    "`d_array_new_from_args` accepted zero element size");

    // test zero argument count
    arr = d_array_new_from_args(sizeof(int), 0);

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "`d_array_new_from_args` accepts zero argument count",
                                    arr != NULL,
                                    "`d_array_new_from_args` rejected zero argument count");

    if (arr)
    {
        test_result &= D_TEST_CHECK_MSG(_suite,
                                        "`d_array_new_from_args` with zero args sets count to 0",
                                        arr->count == 0,
                                        "`d_array_new_from_args` with zero args did not set count to 0");

        d_array_free(arr);
    }

    return test_result;
}

/*
d_tests_array_new_copy
  Tests the `d_array_new_copy` function.
  Tests the following:
  - creating copy of existing array
  - correct data copying
  - independent data storage
  - NULL source rejection
  - zero element size rejection
*/
bool d_tests_array_new_copy(
    struct dj_test_suite* _suite
)
{
    bool            test_result = true;
    struct dj_array* original;
    struct dj_array* copy;
    int*            test_data;
    const size_t    test_count = 5;

    d_test_suite_test(_suite, "array_new_copy");

    d_test_create_test_data_int(&test_data, test_count);

    if (!test_data)
    {
        test_result &= D_TEST_CHECK_MSG(_suite,
                                        "test data creation succeeded",
                                        false,
                                        "failed to create test data for `d_array_new_copy`");

        return test_result;
    }

    original = d_array_new_from_arr(sizeof(int), test_data, test_count);

    if (!original)
    {
        test_result &= D_TEST_CHECK_MSG(_suite,
                                        "original array creation succeeded",
                                        false,
                                        "failed to create original array for `d_array_new_copy` test");

        free(test_data);

        return test_result;
    }

    // test copying
    copy = d_array_new_copy(original, sizeof(int));

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "`d_array_new_copy` creates copy of array",
                                    copy != NULL,
                                    "`d_array_new_copy` failed to create copy");

    if (copy)
    {
        test_result &= D_TEST_CHECK_MSG(_suite,
                                        "`d_array_new_copy` copies count correctly",
                                        copy->count == original->count,
                                        "`d_array_new_copy` did not copy count correctly");

        test_result &= D_TEST_CHECK_MSG(_suite,
                                        "`d_array_new_copy` copies data correctly",
                                        d_test_arrays_equal_int(copy->elements, original->elements, test_count),
                                        "`d_array_new_copy` did not copy data correctly");

        // verify independence of copies
        test_result &= D_TEST_CHECK_MSG(_suite,
                                        "`d_array_new_copy` creates independent data copy",
                                        copy->elements != original->elements,
                                        "`d_array_new_copy` did not create independent data copy");

        d_array_free(copy);
    }

    // test null source
    copy = d_array_new_copy(NULL, sizeof(int));

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "`d_array_new_copy` rejects NULL source array",
                                    copy == NULL,
                                    "`d_array_new_copy` accepted NULL source array");

    // test zero element size
    copy = d_array_new_copy(original, 0);

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "`d_array_new_copy` rejects zero element size",
                                    copy == NULL,
                                    "`d_array_new_copy` accepted zero element size");

    d_array_free(original);
    free(test_data);

    return test_result;
}

/*
d_tests_array_new_copy_reverse
  Tests the `d_array_new_copy_reverse` function.
  Tests the following:
  - creating reversed copy with positive indices
  - creating reversed copy with negative indices
  - NULL source rejection
  - invalid range rejection
*/
bool d_tests_array_new_copy_reverse(
    struct dj_test_suite* _suite
)
{
    bool            test_result = true;
    struct dj_array* original;
    struct dj_array* copy;
    int             test_data[] = { 1, 2, 3, 4, 5 };
    int*            int_elements;

    d_test_suite_test(_suite, "array_new_copy_reverse");

    original = d_array_new_from_arr(sizeof(int), test_data, 5);

    if (!original)
    {
        test_result &= D_TEST_CHECK_MSG(_suite,
                                        "original array creation succeeded",
                                        false,
                                        "failed to create original array");

        return test_result;
    }

    // test reversed copy with full range
    copy = d_array_new_copy_reverse(original, sizeof(int), 0, 4);
    int_elements = copy ? (int*)copy->elements : NULL;

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "`d_array_new_copy_reverse` creates reversed copy",
                                    copy != NULL && copy->count == 5 && int_elements[0] == 5 && int_elements[4] == 1,
                                    "`d_array_new_copy_reverse` failed to create reversed copy");

    if (copy)
    {
        d_array_free(copy);
    }

    // test with negative indices
    copy = d_array_new_copy_reverse(original, sizeof(int), -3, -1);
    int_elements = copy ? (int*)copy->elements : NULL;

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "`d_array_new_copy_reverse` handles negative indices",
                                    copy != NULL && copy->count == 3 && int_elements[0] == 5 && int_elements[2] == 3,
                                    "`d_array_new_copy_reverse` failed with negative indices");

    if (copy)
    {
        d_array_free(copy);
    }

    // test NULL source
    copy = d_array_new_copy_reverse(NULL, sizeof(int), 0, 4);

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "`d_array_new_copy_reverse` rejects NULL source",
                                    copy == NULL,
                                    "`d_array_new_copy_reverse` accepted NULL source");

    d_array_free(original);

    return test_result;
}

/*
d_tests_array_new_copy_range
  Tests the `d_array_new_copy_range` function.
  Tests the following:
  - creating copy of range with positive indices
  - creating copy of range with negative indices
  - NULL source rejection
*/
bool d_tests_array_new_copy_range(
    struct dj_test_suite* _suite
)
{
    bool            test_result = true;
    struct dj_array* original;
    struct dj_array* copy;
    int             test_data[] = { 1, 2, 3, 4, 5 };
    int*            int_elements;

    d_test_suite_test(_suite, "array_new_copy_range");

    original = d_array_new_from_arr(sizeof(int), test_data, 5);

    if (!original)
    {
        test_result &= D_TEST_CHECK_MSG(_suite,
                                        "original array creation succeeded",
                                        false,
                                        "failed to create original array");

        return test_result;
    }

    // test range copy with positive indices (1 to 3 -> [2, 3, 4])
    copy = d_array_new_copy_range(original, sizeof(int), 1, 3);
    int_elements = copy ? (int*)copy->elements : NULL;

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "`d_array_new_copy_range` creates range copy",
                                    copy != NULL && copy->count == 3 && int_elements[0] == 2 && int_elements[2] == 4,
                                    "`d_array_new_copy_range` failed to create range copy");

    if (copy)
    {
        d_array_free(copy);
    }

    // test with negative indices (-3 to -1 -> indices 2 to 4 -> [3, 4, 5])
    copy = d_array_new_copy_range(original, sizeof(int), -3, -1);
    int_elements = copy ? (int*)copy->elements : NULL;

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "`d_array_new_copy_range` handles negative indices",
                                    copy != NULL && copy->count == 3 && int_elements[0] == 3 && int_elements[2] == 5,
                                    "`d_array_new_copy_range` failed with negative indices");

    if (copy)
    {
        d_array_free(copy);
    }

    // test NULL source
    copy = d_array_new_copy_range(NULL, sizeof(int), 0, 2);

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "`d_array_new_copy_range` rejects NULL source",
                                    copy == NULL,
                                    "`d_array_new_copy_range` accepted NULL source");

    d_array_free(original);

    return test_result;
}

/*
d_tests_array_new_copy_range_reverse
  Tests the `d_array_new_copy_range_reverse` function.
  Tests the following:
  - creating reversed range copy
  - handling negative indices
  - NULL source rejection
*/
bool d_tests_array_new_copy_range_reverse(
    struct dj_test_suite* _suite
)
{
    bool            test_result = true;
    struct dj_array* original;
    struct dj_array* copy;
    int             test_data[] = { 1, 2, 3, 4, 5 };
    int*            int_elements;

    d_test_suite_test(_suite, "array_new_copy_range_reverse");

    original = d_array_new_from_arr(sizeof(int), test_data, 5);

    if (!original)
    {
        test_result &= D_TEST_CHECK_MSG(_suite,
                                        "original array creation succeeded",
                                        false,
                                        "failed to create original array");

        return test_result;
    }

    // test range reverse copy (1 to 3 -> [2, 3, 4] reversed -> [4, 3, 2])
    copy = d_array_new_copy_range_reverse(original, sizeof(int), 1, 3);
    int_elements = copy ? (int*)copy->elements : NULL;

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "`d_array_new_copy_range_reverse` creates reversed range copy",
                                    copy != NULL && copy->count == 3 && int_elements[0] == 4 && int_elements[2] == 2,
                                    "`d_array_new_copy_range_reverse` failed");

    if (copy)
    {
        d_array_free(copy);
    }

    // test NULL source
    copy = d_array_new_copy_range_reverse(NULL, sizeof(int), 0, 2);

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "`d_array_new_copy_range_reverse` rejects NULL source",
                                    copy == NULL,
                                    "`d_array_new_copy_range_reverse` accepted NULL source");

    d_array_free(original);

    return test_result;
}

/*
d_tests_array_new_fill
  Tests the `d_array_new_fill` function.
  Tests the following:
  - creating array filled with value
  - zero element size rejection
  - NULL value rejection for non-zero size
  - zero initial size acceptance
*/
bool d_tests_array_new_fill(
    struct dj_test_suite* _suite
)
{
    bool            test_result = true;
    struct dj_array* arr;
    int             fill_value = 42;
    int*            int_elements;

    d_test_suite_test(_suite, "array_new_fill");

    // test normal fill
    arr = d_array_new_fill(sizeof(int), 5, &fill_value);
    int_elements = arr ? (int*)arr->elements : NULL;

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "`d_array_new_fill` creates filled array",
                                    arr != NULL && arr->count == 5,
                                    "`d_array_new_fill` failed to create filled array");

    if (arr)
    {
        bool all_filled = true;

        for (size_t i = 0; i < arr->count; i++)
        {
            if (int_elements[i] != 42)
            {
                all_filled = false;
                break;
            }
        }

        test_result &= D_TEST_CHECK_MSG(_suite,
                                        "`d_array_new_fill` fills all elements correctly",
                                        all_filled,
                                        "`d_array_new_fill` did not fill all elements");

        d_array_free(arr);
    }

    // test zero element size
    arr = d_array_new_fill(0, 5, &fill_value);

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "`d_array_new_fill` rejects zero element size",
                                    arr == NULL,
                                    "`d_array_new_fill` accepted zero element size");

    // test NULL value with non-zero size
    arr = d_array_new_fill(sizeof(int), 5, NULL);

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "`d_array_new_fill` rejects NULL value for non-zero size",
                                    arr == NULL,
                                    "`d_array_new_fill` accepted NULL value for non-zero size");

    return test_result;
}

/*
d_tests_array_new_merge
  Tests the `d_array_new_merge` function.
  Tests the following:
  - merging two arrays
  - correct data ordering
  - zero element size rejection
  - zero count rejection
*/
bool d_tests_array_new_merge(
    struct dj_test_suite* _suite
)
{
    bool            test_result = true;
    struct dj_array* merged;
    struct dj_array  arr1 = { 3, NULL };
    struct dj_array  arr2 = { 2, NULL };
    int             data1[] = { 1, 2, 3 };
    int             data2[] = { 4, 5 };
    int             expected[] = { 1, 2, 3, 4, 5 };
    const size_t    expected_count = 5;

    d_test_suite_test(_suite, "array_new_merge");

    arr1.elements = data1;
    arr2.elements = data2;

    // test normal merge
    merged = d_array_new_merge(sizeof(int), 2, &arr1, &arr2);

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "`d_array_new_merge` creates merged array",
                                    merged != NULL,
                                    "`d_array_new_merge` failed to create merged array");

    if (merged)
    {
        test_result &= D_TEST_CHECK_MSG(_suite,
                                        "`d_array_new_merge` sets correct count",
                                        merged->count == expected_count,
                                        "`d_array_new_merge` did not set correct count");

        test_result &= D_TEST_CHECK_MSG(_suite,
                                        "`d_array_new_merge` merges data correctly",
                                        d_test_arrays_equal_int(merged->elements, expected, expected_count),
                                        "`d_array_new_merge` did not merge data correctly");

        d_array_free(merged);
    }

    // test zero element size
    merged = d_array_new_merge(0, 2, &arr1, &arr2);

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "`d_array_new_merge` rejects zero element size",
                                    merged == NULL,
                                    "`d_array_new_merge` accepted zero element size");

    // test zero count
    merged = d_array_new_merge(sizeof(int), 0);

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "`d_array_new_merge` rejects zero array count",
                                    merged == NULL,
                                    "`d_array_new_merge` accepted zero array count");

    return test_result;
}

/*
d_tests_array_new_slice
  Tests the `d_array_new_slice` function.
  Tests the following:
  - creating slice from positive index
  - creating slice from negative index
  - NULL source rejection
*/
bool d_tests_array_new_slice(
    struct dj_test_suite* _suite
)
{
    bool            test_result = true;
    struct dj_array* original;
    struct dj_array* slice;
    int             test_data[] = { 1, 2, 3, 4, 5 };

    d_test_suite_test(_suite, "array_new_slice");

    original = d_array_new_from_arr(sizeof(int), test_data, 5);

    if (!original)
    {
        test_result &= D_TEST_CHECK_MSG(_suite,
                                        "original array creation succeeded",
                                        false,
                                        "failed to create original array");

        return test_result;
    }

    // test slice from positive index 2 -> should get elements from index 2 onwards
    slice = d_array_new_slice(original, sizeof(int), 2);

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "`d_array_new_slice` creates slice from positive index",
                                    slice != NULL,
                                    "`d_array_new_slice` failed with positive index");

    if (slice)
    {
        d_array_free(slice);
    }

    // test NULL source
    slice = d_array_new_slice(NULL, sizeof(int), 0);

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "`d_array_new_slice` rejects NULL source",
                                    slice == NULL,
                                    "`d_array_new_slice` accepted NULL source");

    d_array_free(original);

    return test_result;
}

/*
d_tests_array_new_slice_reverse
  Tests the `d_array_new_slice_reverse` function.
  Tests the following:
  - creating reversed slice
  - NULL source rejection
*/
bool d_tests_array_new_slice_reverse(
    struct dj_test_suite* _suite
)
{
    bool            test_result = true;
    struct dj_array* original;
    struct dj_array* slice;
    int             test_data[] = { 1, 2, 3, 4, 5 };

    d_test_suite_test(_suite, "array_new_slice_reverse");

    original = d_array_new_from_arr(sizeof(int), test_data, 5);

    if (!original)
    {
        test_result &= D_TEST_CHECK_MSG(_suite,
                                        "original array creation succeeded",
                                        false,
                                        "failed to create original array");

        return test_result;
    }

    // test reversed slice
    slice = d_array_new_slice_reverse(original, sizeof(int), 0);

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "`d_array_new_slice_reverse` creates reversed slice",
                                    slice != NULL,
                                    "`d_array_new_slice_reverse` failed");

    if (slice)
    {
        d_array_free(slice);
    }

    // test NULL source
    slice = d_array_new_slice_reverse(NULL, sizeof(int), 0);

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "`d_array_new_slice_reverse` rejects NULL source",
                                    slice == NULL,
                                    "`d_array_new_slice_reverse` accepted NULL source");

    d_array_free(original);

    return test_result;
}

/*
d_tests_array_new_slice_range
  Tests the `d_array_new_slice_range` function.
  Tests the following:
  - creating range slice
  - NULL source rejection
*/
bool d_tests_array_new_slice_range(
    struct dj_test_suite* _suite
)
{
    bool            test_result = true;
    struct dj_array* original;
    struct dj_array* slice;
    int             test_data[] = { 1, 2, 3, 4, 5 };

    d_test_suite_test(_suite, "array_new_slice_range");

    original = d_array_new_from_arr(sizeof(int), test_data, 5);

    if (!original)
    {
        test_result &= D_TEST_CHECK_MSG(_suite,
                                        "original array creation succeeded",
                                        false,
                                        "failed to create original array");

        return test_result;
    }

    // test range slice (1 to 3)
    slice = d_array_new_slice_range(original, sizeof(int), 1, 3);

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "`d_array_new_slice_range` creates range slice",
                                    slice != NULL,
                                    "`d_array_new_slice_range` failed");

    if (slice)
    {
        d_array_free(slice);
    }

    // test NULL source
    slice = d_array_new_slice_range(NULL, sizeof(int), 0, 2);

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "`d_array_new_slice_range` rejects NULL source",
                                    slice == NULL,
                                    "`d_array_new_slice_range` accepted NULL source");

    d_array_free(original);

    return test_result;
}

/*
d_tests_array_new_slice_range_reverse
  Tests the `d_array_new_slice_range_reverse` function.
  Tests the following:
  - creating reversed range slice
  - NULL source rejection
*/
bool d_tests_array_new_slice_range_reverse(
    struct dj_test_suite* _suite
)
{
    bool            test_result = true;
    struct dj_array* original;
    struct dj_array* slice;
    int             test_data[] = { 1, 2, 3, 4, 5 };

    d_test_suite_test(_suite, "array_new_slice_range_reverse");

    original = d_array_new_from_arr(sizeof(int), test_data, 5);

    if (!original)
    {
        test_result &= D_TEST_CHECK_MSG(_suite,
                                        "original array creation succeeded",
                                        false,
                                        "failed to create original array");

        return test_result;
    }

    // test reversed range slice
    slice = d_array_new_slice_range_reverse(original, sizeof(int), 1, 3);

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "`d_array_new_slice_range_reverse` creates reversed range slice",
                                    slice != NULL,
                                    "`d_array_new_slice_range_reverse` failed");

    if (slice)
    {
        d_array_free(slice);
    }

    // test NULL source
    slice = d_array_new_slice_range_reverse(NULL, sizeof(int), 0, 2);

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "`d_array_new_slice_range_reverse` rejects NULL source",
                                    slice == NULL,
                                    "`d_array_new_slice_range_reverse` accepted NULL source");

    d_array_free(original);

    return test_result;
}


/*
===============================================================================
                        CREATION TEST GROUP RUNNER
===============================================================================
*/

/*
d_tests_array_run_creation_tests
  Runs all dj_array creation function tests.
*/
bool d_tests_array_run_creation_tests(
    struct dj_test_suite* _suite
)
{
    bool overall_result = true;

    d_test_suite_block(_suite, "array_run_creation_tests");

    printf("\n----------------------------------------\n");
    printf("Running dj_array Creation Function Tests\n");

    overall_result &= d_tests_array_new(_suite);
    overall_result &= d_tests_array_new_default_size(_suite);
    overall_result &= d_tests_array_new_from_arr(_suite);
    overall_result &= d_tests_array_new_from_args(_suite);
    overall_result &= d_tests_array_new_copy(_suite);
    overall_result &= d_tests_array_new_copy_reverse(_suite);
    overall_result &= d_tests_array_new_copy_range(_suite);
    overall_result &= d_tests_array_new_copy_range_reverse(_suite);
    overall_result &= d_tests_array_new_fill(_suite);
    overall_result &= d_tests_array_new_merge(_suite);
    overall_result &= d_tests_array_new_slice(_suite);
    overall_result &= d_tests_array_new_slice_reverse(_suite);
    overall_result &= d_tests_array_new_slice_range(_suite);
    overall_result &= d_tests_array_new_slice_range_reverse(_suite);

    return overall_result;
}
