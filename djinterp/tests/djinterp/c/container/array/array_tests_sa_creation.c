/*******************************************************************************
* djinterp [container]                                 array_tests_sa_creation.c
*
* Unit tests for `array.h` d_array creation functions.
* Part of the array standalone test suite.
*
*
* path:      /tests/djinterp/c/container/array/array_tests_sa_creation.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.01.30
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "./array_tests_sa.h"
#include "../../../../../inc/djinterp/test/c/test_counter.h"


/*
d_tests_sa_array_new
  Tests the `d_array_new` function.
  Tests the following:
  - normal allocation with valid parameters
  - zero element size rejection
  - zero initial size acceptance
*/
bool
d_tests_sa_array_new
(
    struct d_test_counter* _test_info
)
{
    bool            test_result = true;
    struct d_array* arr;

    printf("Testing `d_array_new`...\n");

    // test normal allocation
    arr = d_array_new(sizeof(int), 10);

    test_result &= d_assert_standalone(
        arr != NULL,
        "`d_array_new` creates array with valid parameters",
        "`d_array_new` failed with valid parameters",
        _test_info
    );

    if (arr)
    {
        test_result &= d_assert_standalone(
            arr->count == 0,
            "`d_array_new` initializes count to 0",
            "`d_array_new` did not initialize count correctly",
            _test_info
        );

        test_result &= d_assert_standalone(
            arr->elements != NULL,
            "`d_array_new` allocates elements storage",
            "`d_array_new` did not allocate elements storage",
            _test_info
        );

        d_array_free(arr);
    }

    // test zero element size
    arr = d_array_new(0, 10);

    test_result &= d_assert_standalone(
        arr == NULL,
        "`d_array_new` rejects zero element size",
        "`d_array_new` accepted zero element size",
        _test_info
    );

    // test zero initial size
    arr = d_array_new(sizeof(int), 0);

    test_result &= d_assert_standalone(
        arr != NULL,
        "`d_array_new` accepts zero initial size",
        "`d_array_new` rejected zero initial size",
        _test_info
    );

    if (arr)
    {
        d_array_free(arr);
    }

    _test_info->tests_total++;
    if (test_result)
    {
        _test_info->tests_passed++;
    }

    return test_result;
}

/*
d_tests_sa_array_new_default_size
  Tests the `d_array_new_default_size` function.
  Tests the following:
  - normal allocation with valid element size
  - zero element size rejection
*/
bool
d_tests_sa_array_new_default_size
(
    struct d_test_counter* _test_info
)
{
    bool            test_result = true;
    struct d_array* arr;

    printf("Testing `d_array_new_default_size`...\n");

    // test normal allocation
    arr = d_array_new_default_size(sizeof(int));

    test_result &= d_assert_standalone(
        arr != NULL,
        "`d_array_new_default_size` creates array with valid element size",
        "`d_array_new_default_size` failed with valid element size",
        _test_info
    );

    if (arr)
    {
        test_result &= d_assert_standalone(
            arr->count == 0,
            "`d_array_new_default_size` initializes count to 0",
            "`d_array_new_default_size` did not initialize count correctly",
            _test_info
        );

        d_array_free(arr);
    }

    // test zero element size
    arr = d_array_new_default_size(0);

    test_result &= d_assert_standalone(
        arr == NULL,
        "`d_array_new_default_size` rejects zero element size",
        "`d_array_new_default_size` accepted zero element size",
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
d_tests_sa_array_new_from_arr
  Tests the `d_array_new_from_arr` function.
  Tests the following:
  - normal creation from existing array
  - correct data copying
  - NULL source rejection
  - zero element size rejection
  - zero count acceptance
*/
bool
d_tests_sa_array_new_from_arr
(
    struct d_test_counter* _test_info
)
{
    bool            test_result = true;
    struct d_array* arr;
    int*            test_data;
    const size_t    test_count = 5;

    printf("Testing `d_array_new_from_arr`...\n");

    d_test_create_test_data_int(&test_data, test_count);

    if (!test_data)
    {
        test_result &= d_assert_standalone(
            false,
            "test data creation succeeded",
            "failed to create test data for `d_array_new_from_arr`",
            _test_info
        );

        _test_info->tests_total++;

        return test_result;
    }

    // test normal creation from array
    arr = d_array_new_from_arr(sizeof(int), test_data, test_count);

    test_result &= d_assert_standalone(
        arr != NULL,
        "`d_array_new_from_arr` creates array from valid source",
        "`d_array_new_from_arr` failed with valid source",
        _test_info
    );

    if (arr)
    {
        test_result &= d_assert_standalone(
            arr->count == test_count,
            "`d_array_new_from_arr` sets correct count",
            "`d_array_new_from_arr` did not set correct count",
            _test_info
        );

        test_result &= d_assert_standalone(
            d_test_arrays_equal_int(arr->elements, test_data, test_count),
            "`d_array_new_from_arr` copies data correctly",
            "`d_array_new_from_arr` did not copy data correctly",
            _test_info
        );

        d_array_free(arr);
    }

    // test null source data
    arr = d_array_new_from_arr(sizeof(int), NULL, test_count);

    test_result &= d_assert_standalone(
        arr == NULL,
        "`d_array_new_from_arr` rejects NULL source data",
        "`d_array_new_from_arr` accepted NULL source data",
        _test_info
    );

    // test zero element size
    arr = d_array_new_from_arr(0, test_data, test_count);

    test_result &= d_assert_standalone(
        arr == NULL,
        "`d_array_new_from_arr` rejects zero element size",
        "`d_array_new_from_arr` accepted zero element size",
        _test_info
    );

    // test zero count
    arr = d_array_new_from_arr(sizeof(int), test_data, 0);

    test_result &= d_assert_standalone(
        arr != NULL,
        "`d_array_new_from_arr` accepts zero count",
        "`d_array_new_from_arr` rejected zero count",
        _test_info
    );

    if (arr)
    {
        test_result &= d_assert_standalone(
            arr->count == 0,
            "`d_array_new_from_arr` with zero count sets count to 0",
            "`d_array_new_from_arr` with zero count did not set count to 0",
            _test_info
        );

        d_array_free(arr);
    }

    free(test_data);

    _test_info->tests_total++;
    if (test_result)
    {
        _test_info->tests_passed++;
    }

    return test_result;
}

/*
d_tests_sa_array_new_from_args
  Tests the `d_array_new_from_args` function.
  Tests the following:
  - normal creation from variadic arguments
  - correct data storage
  - zero element size rejection
  - zero argument count acceptance
*/
bool
d_tests_sa_array_new_from_args
(
    struct d_test_counter* _test_info
)
{
    bool            test_result = true;
    struct d_array* arr;
    int             expected[] = { 1, 2, 3, 4, 5 };
    const size_t    expected_count = 5;

    printf("Testing `d_array_new_from_args`...\n");

    // test normal creation from variadic args
    arr = d_array_new_from_args(sizeof(int), expected_count, 1, 2, 3, 4, 5);

    test_result &= d_assert_standalone(
        arr != NULL,
        "`d_array_new_from_args` creates array from variadic arguments",
        "`d_array_new_from_args` failed with variadic arguments",
        _test_info
    );

    if (arr)
    {
        test_result &= d_assert_standalone(
            arr->count == expected_count,
            "`d_array_new_from_args` sets correct count",
            "`d_array_new_from_args` did not set correct count",
            _test_info
        );

        test_result &= d_assert_standalone(
            d_test_arrays_equal_int(arr->elements, expected, expected_count),
            "`d_array_new_from_args` stores arguments correctly",
            "`d_array_new_from_args` did not store arguments correctly",
            _test_info
        );

        d_array_free(arr);
    }

    // test zero element size
    arr = d_array_new_from_args(0, expected_count, 1, 2, 3, 4, 5);

    test_result &= d_assert_standalone(
        arr == NULL,
        "`d_array_new_from_args` rejects zero element size",
        "`d_array_new_from_args` accepted zero element size",
        _test_info
    );

    // test zero argument count
    arr = d_array_new_from_args(sizeof(int), 0);

    test_result &= d_assert_standalone(
        arr != NULL,
        "`d_array_new_from_args` accepts zero argument count",
        "`d_array_new_from_args` rejected zero argument count",
        _test_info
    );

    if (arr)
    {
        test_result &= d_assert_standalone(
            arr->count == 0,
            "`d_array_new_from_args` with zero args sets count to 0",
            "`d_array_new_from_args` with zero args did not set count to 0",
            _test_info
        );

        d_array_free(arr);
    }

    _test_info->tests_total++;
    if (test_result)
    {
        _test_info->tests_passed++;
    }

    return test_result;
}

/*
d_tests_sa_array_new_copy
  Tests the `d_array_new_copy` function.
  Tests the following:
  - creating copy of existing array
  - correct data copying
  - independent data storage
  - NULL source rejection
  - zero element size rejection
*/
bool
d_tests_sa_array_new_copy
(
    struct d_test_counter* _test_info
)
{
    bool            test_result = true;
    struct d_array* original;
    struct d_array* copy;
    int*            test_data;
    const size_t    test_count = 5;

    printf("Testing `d_array_new_copy`...\n");

    d_test_create_test_data_int(&test_data, test_count);

    if (!test_data)
    {
        test_result &= d_assert_standalone(
            false,
            "test data creation succeeded",
            "failed to create test data for `d_array_new_copy`",
            _test_info
        );

        _test_info->tests_total++;

        return test_result;
    }

    original = d_array_new_from_arr(sizeof(int), test_data, test_count);

    if (!original)
    {
        test_result &= d_assert_standalone(
            false,
            "original array creation succeeded",
            "failed to create original array for `d_array_new_copy` test",
            _test_info
        );

        free(test_data);
        _test_info->tests_total++;

        return test_result;
    }

    // test copying
    copy = d_array_new_copy(original, sizeof(int));

    test_result &= d_assert_standalone(
        copy != NULL,
        "`d_array_new_copy` creates copy of array",
        "`d_array_new_copy` failed to create copy",
        _test_info
    );

    if (copy)
    {
        test_result &= d_assert_standalone(
            copy->count == original->count,
            "`d_array_new_copy` copies count correctly",
            "`d_array_new_copy` did not copy count correctly",
            _test_info
        );

        test_result &= d_assert_standalone(
            d_test_arrays_equal_int(copy->elements, original->elements, test_count),
            "`d_array_new_copy` copies data correctly",
            "`d_array_new_copy` did not copy data correctly",
            _test_info
        );

        // verify independence of copies
        test_result &= d_assert_standalone(
            copy->elements != original->elements,
            "`d_array_new_copy` creates independent data copy",
            "`d_array_new_copy` did not create independent data copy",
            _test_info
        );

        d_array_free(copy);
    }

    // test null source
    copy = d_array_new_copy(NULL, sizeof(int));

    test_result &= d_assert_standalone(
        copy == NULL,
        "`d_array_new_copy` rejects NULL source array",
        "`d_array_new_copy` accepted NULL source array",
        _test_info
    );

    // test zero element size
    copy = d_array_new_copy(original, 0);

    test_result &= d_assert_standalone(
        copy == NULL,
        "`d_array_new_copy` rejects zero element size",
        "`d_array_new_copy` accepted zero element size",
        _test_info
    );

    d_array_free(original);
    free(test_data);

    _test_info->tests_total++;
    if (test_result)
    {
        _test_info->tests_passed++;
    }

    return test_result;
}

/*
d_tests_sa_array_new_copy_reverse
  Tests the `d_array_new_copy_reverse` function.
  Tests the following:
  - creating reversed copy with positive indices
  - creating reversed copy with negative indices
  - NULL source rejection
  - invalid range rejection
*/
bool
d_tests_sa_array_new_copy_reverse
(
    struct d_test_counter* _test_info
)
{
    bool            test_result = true;
    struct d_array* original;
    struct d_array* copy;
    int             test_data[] = { 1, 2, 3, 4, 5 };
    int*            int_elements;

    printf("Testing `d_array_new_copy_reverse`...\n");

    original = d_array_new_from_arr(sizeof(int), test_data, 5);

    if (!original)
    {
        test_result &= d_assert_standalone(
            false,
            "original array creation succeeded",
            "failed to create original array",
            _test_info
        );

        _test_info->tests_total++;

        return test_result;
    }

    // test reversed copy with full range
    copy = d_array_new_copy_reverse(original, sizeof(int), 0, 4);
    int_elements = copy ? (int*)copy->elements : NULL;

    test_result &= d_assert_standalone(
        copy != NULL && copy->count == 5 &&
        int_elements[0] == 5 && int_elements[4] == 1,
        "`d_array_new_copy_reverse` creates reversed copy",
        "`d_array_new_copy_reverse` failed to create reversed copy",
        _test_info
    );

    if (copy)
    {
        d_array_free(copy);
    }

    // test with negative indices
    copy = d_array_new_copy_reverse(original, sizeof(int), -3, -1);
    int_elements = copy ? (int*)copy->elements : NULL;

    test_result &= d_assert_standalone(
        copy != NULL && copy->count == 3 &&
        int_elements[0] == 5 && int_elements[2] == 3,
        "`d_array_new_copy_reverse` handles negative indices",
        "`d_array_new_copy_reverse` failed with negative indices",
        _test_info
    );

    if (copy)
    {
        d_array_free(copy);
    }

    // test NULL source
    copy = d_array_new_copy_reverse(NULL, sizeof(int), 0, 4);

    test_result &= d_assert_standalone(
        copy == NULL,
        "`d_array_new_copy_reverse` rejects NULL source",
        "`d_array_new_copy_reverse` accepted NULL source",
        _test_info
    );

    d_array_free(original);

    _test_info->tests_total++;
    if (test_result)
    {
        _test_info->tests_passed++;
    }

    return test_result;
}

/*
d_tests_sa_array_new_copy_range
  Tests the `d_array_new_copy_range` function.
  Tests the following:
  - creating copy of range with positive indices
  - creating copy of range with negative indices
  - NULL source rejection
*/
bool
d_tests_sa_array_new_copy_range
(
    struct d_test_counter* _test_info
)
{
    bool            test_result = true;
    struct d_array* original;
    struct d_array* copy;
    int             test_data[] = { 1, 2, 3, 4, 5 };
    int*            int_elements;

    printf("Testing `d_array_new_copy_range`...\n");

    original = d_array_new_from_arr(sizeof(int), test_data, 5);

    if (!original)
    {
        test_result &= d_assert_standalone(
            false,
            "original array creation succeeded",
            "failed to create original array",
            _test_info
        );

        _test_info->tests_total++;

        return test_result;
    }

    // test range copy with positive indices (1 to 3 -> [2, 3, 4])
    copy = d_array_new_copy_range(original, sizeof(int), 1, 3);
    int_elements = copy ? (int*)copy->elements : NULL;

    test_result &= d_assert_standalone(
        copy != NULL && copy->count == 3 &&
        int_elements[0] == 2 && int_elements[2] == 4,
        "`d_array_new_copy_range` creates range copy",
        "`d_array_new_copy_range` failed to create range copy",
        _test_info
    );

    if (copy)
    {
        d_array_free(copy);
    }

    // test with negative indices (-3 to -1 -> indices 2 to 4 -> [3, 4, 5])
    copy = d_array_new_copy_range(original, sizeof(int), -3, -1);
    int_elements = copy ? (int*)copy->elements : NULL;

    test_result &= d_assert_standalone(
        copy != NULL && copy->count == 3 &&
        int_elements[0] == 3 && int_elements[2] == 5,
        "`d_array_new_copy_range` handles negative indices",
        "`d_array_new_copy_range` failed with negative indices",
        _test_info
    );

    if (copy)
    {
        d_array_free(copy);
    }

    // test NULL source
    copy = d_array_new_copy_range(NULL, sizeof(int), 0, 2);

    test_result &= d_assert_standalone(
        copy == NULL,
        "`d_array_new_copy_range` rejects NULL source",
        "`d_array_new_copy_range` accepted NULL source",
        _test_info
    );

    d_array_free(original);

    _test_info->tests_total++;
    if (test_result)
    {
        _test_info->tests_passed++;
    }

    return test_result;
}

/*
d_tests_sa_array_new_copy_range_reverse
  Tests the `d_array_new_copy_range_reverse` function.
  Tests the following:
  - creating reversed range copy
  - handling negative indices
  - NULL source rejection
*/
bool
d_tests_sa_array_new_copy_range_reverse
(
    struct d_test_counter* _test_info
)
{
    bool            test_result = true;
    struct d_array* original;
    struct d_array* copy;
    int             test_data[] = { 1, 2, 3, 4, 5 };
    int*            int_elements;

    printf("Testing `d_array_new_copy_range_reverse`...\n");

    original = d_array_new_from_arr(sizeof(int), test_data, 5);

    if (!original)
    {
        test_result &= d_assert_standalone(
            false,
            "original array creation succeeded",
            "failed to create original array",
            _test_info
        );

        _test_info->tests_total++;

        return test_result;
    }

    // test range reverse copy (1 to 3 -> [2, 3, 4] reversed -> [4, 3, 2])
    copy = d_array_new_copy_range_reverse(original, sizeof(int), 1, 3);
    int_elements = copy ? (int*)copy->elements : NULL;

    test_result &= d_assert_standalone(
        copy != NULL && copy->count == 3 &&
        int_elements[0] == 4 && int_elements[2] == 2,
        "`d_array_new_copy_range_reverse` creates reversed range copy",
        "`d_array_new_copy_range_reverse` failed",
        _test_info
    );

    if (copy)
    {
        d_array_free(copy);
    }

    // test NULL source
    copy = d_array_new_copy_range_reverse(NULL, sizeof(int), 0, 2);

    test_result &= d_assert_standalone(
        copy == NULL,
        "`d_array_new_copy_range_reverse` rejects NULL source",
        "`d_array_new_copy_range_reverse` accepted NULL source",
        _test_info
    );

    d_array_free(original);

    _test_info->tests_total++;
    if (test_result)
    {
        _test_info->tests_passed++;
    }

    return test_result;
}

/*
d_tests_sa_array_new_fill
  Tests the `d_array_new_fill` function.
  Tests the following:
  - creating array filled with value
  - zero element size rejection
  - NULL value rejection for non-zero size
  - zero initial size acceptance
*/
bool
d_tests_sa_array_new_fill
(
    struct d_test_counter* _test_info
)
{
    bool            test_result = true;
    struct d_array* arr;
    int             fill_value = 42;
    int*            int_elements;

    printf("Testing `d_array_new_fill`...\n");

    // test normal fill
    arr = d_array_new_fill(sizeof(int), 5, &fill_value);
    int_elements = arr ? (int*)arr->elements : NULL;

    test_result &= d_assert_standalone(
        arr != NULL && arr->count == 5,
        "`d_array_new_fill` creates filled array",
        "`d_array_new_fill` failed to create filled array",
        _test_info
    );

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

        test_result &= d_assert_standalone(
            all_filled,
            "`d_array_new_fill` fills all elements correctly",
            "`d_array_new_fill` did not fill all elements",
            _test_info
        );

        d_array_free(arr);
    }

    // test zero element size
    arr = d_array_new_fill(0, 5, &fill_value);

    test_result &= d_assert_standalone(
        arr == NULL,
        "`d_array_new_fill` rejects zero element size",
        "`d_array_new_fill` accepted zero element size",
        _test_info
    );

    // test NULL value with non-zero size
    arr = d_array_new_fill(sizeof(int), 5, NULL);

    test_result &= d_assert_standalone(
        arr == NULL,
        "`d_array_new_fill` rejects NULL value for non-zero size",
        "`d_array_new_fill` accepted NULL value for non-zero size",
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
d_tests_sa_array_new_merge
  Tests the `d_array_new_merge` function.
  Tests the following:
  - merging two arrays
  - correct data ordering
  - zero element size rejection
  - zero count rejection
*/
bool
d_tests_sa_array_new_merge
(
    struct d_test_counter* _test_info
)
{
    bool            test_result = true;
    struct d_array* merged;
    struct d_array  arr1 = { 3, NULL };
    struct d_array  arr2 = { 2, NULL };
    int             data1[] = { 1, 2, 3 };
    int             data2[] = { 4, 5 };
    int             expected[] = { 1, 2, 3, 4, 5 };
    const size_t    expected_count = 5;

    printf("Testing `d_array_new_merge`...\n");

    arr1.elements = data1;
    arr2.elements = data2;

    // test normal merge
    merged = d_array_new_merge(sizeof(int), 2, &arr1, &arr2);

    test_result &= d_assert_standalone(
        merged != NULL,
        "`d_array_new_merge` creates merged array",
        "`d_array_new_merge` failed to create merged array",
        _test_info
    );

    if (merged)
    {
        test_result &= d_assert_standalone(
            merged->count == expected_count,
            "`d_array_new_merge` sets correct count",
            "`d_array_new_merge` did not set correct count",
            _test_info
        );

        test_result &= d_assert_standalone(
            d_test_arrays_equal_int(merged->elements, expected, expected_count),
            "`d_array_new_merge` merges data correctly",
            "`d_array_new_merge` did not merge data correctly",
            _test_info
        );

        d_array_free(merged);
    }

    // test zero element size
    merged = d_array_new_merge(0, 2, &arr1, &arr2);

    test_result &= d_assert_standalone(
        merged == NULL,
        "`d_array_new_merge` rejects zero element size",
        "`d_array_new_merge` accepted zero element size",
        _test_info
    );

    // test zero count
    merged = d_array_new_merge(sizeof(int), 0);

    test_result &= d_assert_standalone(
        merged == NULL,
        "`d_array_new_merge` rejects zero array count",
        "`d_array_new_merge` accepted zero array count",
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
d_tests_sa_array_new_slice
  Tests the `d_array_new_slice` function.
  Tests the following:
  - creating slice from positive index
  - creating slice from negative index
  - NULL source rejection
*/
bool
d_tests_sa_array_new_slice
(
    struct d_test_counter* _test_info
)
{
    bool            test_result = true;
    struct d_array* original;
    struct d_array* slice;
    int             test_data[] = { 1, 2, 3, 4, 5 };

    printf("Testing `d_array_new_slice`...\n");

    original = d_array_new_from_arr(sizeof(int), test_data, 5);

    if (!original)
    {
        test_result &= d_assert_standalone(
            false,
            "original array creation succeeded",
            "failed to create original array",
            _test_info
        );

        _test_info->tests_total++;

        return test_result;
    }

    // test slice from positive index 2 -> should get elements from index 2 onwards
    slice = d_array_new_slice(original, sizeof(int), 2);

    test_result &= d_assert_standalone(
        slice != NULL,
        "`d_array_new_slice` creates slice from positive index",
        "`d_array_new_slice` failed with positive index",
        _test_info
    );

    if (slice)
    {
        d_array_free(slice);
    }

    // test NULL source
    slice = d_array_new_slice(NULL, sizeof(int), 0);

    test_result &= d_assert_standalone(
        slice == NULL,
        "`d_array_new_slice` rejects NULL source",
        "`d_array_new_slice` accepted NULL source",
        _test_info
    );

    d_array_free(original);

    _test_info->tests_total++;
    if (test_result)
    {
        _test_info->tests_passed++;
    }

    return test_result;
}

/*
d_tests_sa_array_new_slice_reverse
  Tests the `d_array_new_slice_reverse` function.
  Tests the following:
  - creating reversed slice
  - NULL source rejection
*/
bool
d_tests_sa_array_new_slice_reverse
(
    struct d_test_counter* _test_info
)
{
    bool            test_result = true;
    struct d_array* original;
    struct d_array* slice;
    int             test_data[] = { 1, 2, 3, 4, 5 };

    printf("Testing `d_array_new_slice_reverse`...\n");

    original = d_array_new_from_arr(sizeof(int), test_data, 5);

    if (!original)
    {
        test_result &= d_assert_standalone(
            false,
            "original array creation succeeded",
            "failed to create original array",
            _test_info
        );

        _test_info->tests_total++;

        return test_result;
    }

    // test reversed slice
    slice = d_array_new_slice_reverse(original, sizeof(int), 0);

    test_result &= d_assert_standalone(
        slice != NULL,
        "`d_array_new_slice_reverse` creates reversed slice",
        "`d_array_new_slice_reverse` failed",
        _test_info
    );

    if (slice)
    {
        d_array_free(slice);
    }

    // test NULL source
    slice = d_array_new_slice_reverse(NULL, sizeof(int), 0);

    test_result &= d_assert_standalone(
        slice == NULL,
        "`d_array_new_slice_reverse` rejects NULL source",
        "`d_array_new_slice_reverse` accepted NULL source",
        _test_info
    );

    d_array_free(original);

    _test_info->tests_total++;
    if (test_result)
    {
        _test_info->tests_passed++;
    }

    return test_result;
}

/*
d_tests_sa_array_new_slice_range
  Tests the `d_array_new_slice_range` function.
  Tests the following:
  - creating range slice
  - NULL source rejection
*/
bool
d_tests_sa_array_new_slice_range
(
    struct d_test_counter* _test_info
)
{
    bool            test_result = true;
    struct d_array* original;
    struct d_array* slice;
    int             test_data[] = { 1, 2, 3, 4, 5 };

    printf("Testing `d_array_new_slice_range`...\n");

    original = d_array_new_from_arr(sizeof(int), test_data, 5);

    if (!original)
    {
        test_result &= d_assert_standalone(
            false,
            "original array creation succeeded",
            "failed to create original array",
            _test_info
        );

        _test_info->tests_total++;

        return test_result;
    }

    // test range slice (1 to 3)
    slice = d_array_new_slice_range(original, sizeof(int), 1, 3);

    test_result &= d_assert_standalone(
        slice != NULL,
        "`d_array_new_slice_range` creates range slice",
        "`d_array_new_slice_range` failed",
        _test_info
    );

    if (slice)
    {
        d_array_free(slice);
    }

    // test NULL source
    slice = d_array_new_slice_range(NULL, sizeof(int), 0, 2);

    test_result &= d_assert_standalone(
        slice == NULL,
        "`d_array_new_slice_range` rejects NULL source",
        "`d_array_new_slice_range` accepted NULL source",
        _test_info
    );

    d_array_free(original);

    _test_info->tests_total++;
    if (test_result)
    {
        _test_info->tests_passed++;
    }

    return test_result;
}

/*
d_tests_sa_array_new_slice_range_reverse
  Tests the `d_array_new_slice_range_reverse` function.
  Tests the following:
  - creating reversed range slice
  - NULL source rejection
*/
bool
d_tests_sa_array_new_slice_range_reverse
(
    struct d_test_counter* _test_info
)
{
    bool            test_result = true;
    struct d_array* original;
    struct d_array* slice;
    int             test_data[] = { 1, 2, 3, 4, 5 };

    printf("Testing `d_array_new_slice_range_reverse`...\n");

    original = d_array_new_from_arr(sizeof(int), test_data, 5);

    if (!original)
    {
        test_result &= d_assert_standalone(
            false,
            "original array creation succeeded",
            "failed to create original array",
            _test_info
        );

        _test_info->tests_total++;

        return test_result;
    }

    // test reversed range slice
    slice = d_array_new_slice_range_reverse(original, sizeof(int), 1, 3);

    test_result &= d_assert_standalone(
        slice != NULL,
        "`d_array_new_slice_range_reverse` creates reversed range slice",
        "`d_array_new_slice_range_reverse` failed",
        _test_info
    );

    if (slice)
    {
        d_array_free(slice);
    }

    // test NULL source
    slice = d_array_new_slice_range_reverse(NULL, sizeof(int), 0, 2);

    test_result &= d_assert_standalone(
        slice == NULL,
        "`d_array_new_slice_range_reverse` rejects NULL source",
        "`d_array_new_slice_range_reverse` accepted NULL source",
        _test_info
    );

    d_array_free(original);

    _test_info->tests_total++;
    if (test_result)
    {
        _test_info->tests_passed++;
    }

    return test_result;
}


/*
===============================================================================
                        CREATION TEST GROUP RUNNER
===============================================================================
*/

/*
d_tests_sa_array_run_creation_tests
  Runs all d_array creation function tests.
*/
bool
d_tests_sa_array_run_creation_tests
(
    struct d_test_counter* _test_info
)
{
    bool overall_result = true;

    printf("\n----------------------------------------\n");
    printf("Running d_array Creation Function Tests\n");
    printf("----------------------------------------\n");

    overall_result &= d_tests_sa_array_new(_test_info);
    overall_result &= d_tests_sa_array_new_default_size(_test_info);
    overall_result &= d_tests_sa_array_new_from_arr(_test_info);
    overall_result &= d_tests_sa_array_new_from_args(_test_info);
    overall_result &= d_tests_sa_array_new_copy(_test_info);
    overall_result &= d_tests_sa_array_new_copy_reverse(_test_info);
    overall_result &= d_tests_sa_array_new_copy_range(_test_info);
    overall_result &= d_tests_sa_array_new_copy_range_reverse(_test_info);
    overall_result &= d_tests_sa_array_new_fill(_test_info);
    overall_result &= d_tests_sa_array_new_merge(_test_info);
    overall_result &= d_tests_sa_array_new_slice(_test_info);
    overall_result &= d_tests_sa_array_new_slice_reverse(_test_info);
    overall_result &= d_tests_sa_array_new_slice_range(_test_info);
    overall_result &= d_tests_sa_array_new_slice_range_reverse(_test_info);

    return overall_result;
}
