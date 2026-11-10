/*******************************************************************************
* djinterp [container]                                  array_tests_creation_s.c
*
* Unit tests for `array.h` dj_array_s creation functions.
* Part of the array suite, on DTest's C face.
*
*
* path:      /tests/djinterp/c/container/array/array_tests_creation_s.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.01.30
*                                                            revised: 2026.11.09
*******************************************************************************/

// djinterp
#include "./array_tests.h"
#include "../../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's C face


/*
d_tests_array_s_new
  Tests the `d_array_s_new` function.
*/
bool d_tests_array_s_new(
    struct dj_test_suite* _suite
)
{
    bool              test_result = true;
    struct dj_array_s* arr;

    d_test_suite_test(_suite, "array_s_new");

    arr = d_array_s_new(sizeof(int), 10);

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "`d_array_s_new` creates array with correct fields",
                                    arr != NULL && arr->count == 0 && arr->element_size == sizeof(int),
                                    "`d_array_s_new` failed");

    if (arr) d_array_s_free(arr);

    arr = d_array_s_new(0, 10);

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "`d_array_s_new` rejects zero element size",
                                    arr == NULL,
                                    "`d_array_s_new` accepted zero element size");

    return test_result;
}

/*
d_tests_array_s_new_default_size
  Tests the `d_array_s_new_default_size` function.
*/
bool d_tests_array_s_new_default_size(
    struct dj_test_suite* _suite
)
{
    bool              test_result = true;
    struct dj_array_s* arr;

    d_test_suite_test(_suite, "array_s_new_default_size");

    arr = d_array_s_new_default_size(sizeof(int));

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "`d_array_s_new_default_size` creates array correctly",
                                    arr != NULL && arr->element_size == sizeof(int),
                                    "`d_array_s_new_default_size` failed");

    if (arr) d_array_s_free(arr);

    return test_result;
}

/*
d_tests_array_s_new_from_arr
  Tests the `d_array_s_new_from_arr` function.
*/
bool d_tests_array_s_new_from_arr(
    struct dj_test_suite* _suite
)
{
    bool              test_result = true;
    struct dj_array_s* arr;
    int               data[] = { 1, 2, 3, 4, 5 };

    d_test_suite_test(_suite, "array_s_new_from_arr");

    arr = d_array_s_new_from_arr(sizeof(int), data, 5);

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "`d_array_s_new_from_arr` creates array correctly",
                                    arr != NULL && arr->count == 5 && arr->element_size == sizeof(int),
                                    "`d_array_s_new_from_arr` failed");

    if (arr) d_array_s_free(arr);

    return test_result;
}

/*
d_tests_array_s_new_from_args
  Tests the `d_array_s_new_from_args` function.
*/
bool d_tests_array_s_new_from_args(
    struct dj_test_suite* _suite
)
{
    bool              test_result = true;
    struct dj_array_s* arr;

    d_test_suite_test(_suite, "array_s_new_from_args");

    arr = d_array_s_new_from_args(sizeof(int), 5, 1, 2, 3, 4, 5);

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "`d_array_s_new_from_args` creates array correctly",
                                    arr != NULL && arr->count == 5 && arr->element_size == sizeof(int),
                                    "`d_array_s_new_from_args` failed");

    if (arr) d_array_s_free(arr);

    return test_result;
}

/*
d_tests_array_s_new_copy
  Tests the `d_array_s_new_copy` function.
*/
bool d_tests_array_s_new_copy(
    struct dj_test_suite* _suite
)
{
    bool              test_result = true;
    struct dj_array*   original;
    struct dj_array_s* copy;
    int               data[] = { 1, 2, 3, 4, 5 };

    d_test_suite_test(_suite, "array_s_new_copy");

    original = d_array_new_from_arr(sizeof(int), data, 5);

    if (!original)
    {
        return false;
    }

    copy = d_array_s_new_copy(original, sizeof(int));

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "`d_array_s_new_copy` creates copy correctly",
                                    copy != NULL && copy->count == 5 && copy->element_size == sizeof(int),
                                    "`d_array_s_new_copy` failed");

    if (copy) d_array_s_free(copy);
    d_array_free(original);

    return test_result;
}

/*
d_tests_array_s_new_copy_reverse
  Tests the `d_array_s_new_copy_reverse` function.
*/
bool d_tests_array_s_new_copy_reverse(
    struct dj_test_suite* _suite
)
{
    bool              test_result = true;
    struct dj_array*   original;
    struct dj_array_s* copy;
    int               data[] = { 1, 2, 3, 4, 5 };
    int*              int_elements;

    d_test_suite_test(_suite, "array_s_new_copy_reverse");

    original = d_array_new_from_arr(sizeof(int), data, 5);

    if (!original)
    {
        return false;
    }

    copy = d_array_s_new_copy_reverse(original, sizeof(int), 0, 4);
    int_elements = copy ? (int*)copy->elements : NULL;

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "`d_array_s_new_copy_reverse` creates reversed copy",
                                    copy != NULL && int_elements[0] == 5 && int_elements[4] == 1,
                                    "`d_array_s_new_copy_reverse` failed");

    if (copy) d_array_s_free(copy);
    d_array_free(original);

    return test_result;
}

/*
d_tests_array_s_new_copy_range
  Tests the `d_array_s_new_copy_range` function.
*/
bool d_tests_array_s_new_copy_range(
    struct dj_test_suite* _suite
)
{
    bool              test_result = true;
    struct dj_array*   original;
    struct dj_array_s* copy;
    int               data[] = { 1, 2, 3, 4, 5 };
    int*              int_elements;

    d_test_suite_test(_suite, "array_s_new_copy_range");

    original = d_array_new_from_arr(sizeof(int), data, 5);

    if (!original)
    {
        return false;
    }

    copy = d_array_s_new_copy_range(original, sizeof(int), 1, 3);
    int_elements = copy ? (int*)copy->elements : NULL;

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "`d_array_s_new_copy_range` creates range copy",
                                    copy != NULL && copy->count == 3 && int_elements[0] == 2,
                                    "`d_array_s_new_copy_range` failed");

    if (copy) d_array_s_free(copy);
    d_array_free(original);

    return test_result;
}

/*
d_tests_array_s_new_copy_range_reverse
  Tests the `d_array_s_new_copy_range_reverse` function.
*/
bool d_tests_array_s_new_copy_range_reverse(
    struct dj_test_suite* _suite
)
{
    bool              test_result = true;
    struct dj_array*   original;
    struct dj_array_s* copy;
    int               data[] = { 1, 2, 3, 4, 5 };
    int*              int_elements;

    d_test_suite_test(_suite, "array_s_new_copy_range_reverse");

    original = d_array_new_from_arr(sizeof(int), data, 5);

    if (!original)
    {
        return false;
    }

    copy = d_array_s_new_copy_range_reverse(original, sizeof(int), 1, 3);
    int_elements = copy ? (int*)copy->elements : NULL;

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "`d_array_s_new_copy_range_reverse` creates reversed range",
                                    copy != NULL && copy->count == 3 && int_elements[0] == 4,
                                    "`d_array_s_new_copy_range_reverse` failed");

    if (copy) d_array_s_free(copy);
    d_array_free(original);

    return test_result;
}

/*
d_tests_array_s_new_fill
  Tests the `d_array_s_new_fill` function.
*/
bool d_tests_array_s_new_fill(
    struct dj_test_suite* _suite
)
{
    bool              test_result = true;
    struct dj_array_s* arr;
    int               fill_value = 42;
    int*              int_elements;

    d_test_suite_test(_suite, "array_s_new_fill");

    arr = d_array_s_new_fill(sizeof(int), 5, &fill_value);
    int_elements = arr ? (int*)arr->elements : NULL;

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "`d_array_s_new_fill` creates filled array",
                                    arr != NULL && arr->count == 5 && int_elements[0] == 42,
                                    "`d_array_s_new_fill` failed");

    if (arr) d_array_s_free(arr);

    return test_result;
}

/*
d_tests_array_s_new_merge
  Tests the `d_array_s_new_merge` function.
*/
bool d_tests_array_s_new_merge(
    struct dj_test_suite* _suite
)
{
    bool              test_result = true;
    struct dj_array_s* merged;
    struct dj_array_s* arr1;
    struct dj_array_s* arr2;
    int               data1[] = { 1, 2, 3 };
    int               data2[] = { 4, 5 };

    d_test_suite_test(_suite, "array_s_new_merge");

    arr1 = d_array_s_new_from_arr(sizeof(int), data1, 3);
    arr2 = d_array_s_new_from_arr(sizeof(int), data2, 2);

    if (!arr1 || !arr2)
    {
        if (arr1) d_array_s_free(arr1);
        if (arr2) d_array_s_free(arr2);

        return false;
    }

    merged = d_array_s_new_merge(sizeof(int), 2, arr1, arr2);

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "`d_array_s_new_merge` creates merged array",
                                    merged != NULL && merged->count == 5,
                                    "`d_array_s_new_merge` failed");

    if (merged) d_array_s_free(merged);
    d_array_s_free(arr1);
    d_array_s_free(arr2);

    return test_result;
}

/*
d_tests_array_s_new_slice
  Tests the `d_array_s_new_slice` function.
*/
bool d_tests_array_s_new_slice(
    struct dj_test_suite* _suite
)
{
    bool              test_result = true;
    struct dj_array*   original;
    struct dj_array_s* slice;
    int               data[] = { 1, 2, 3, 4, 5 };

    d_test_suite_test(_suite, "array_s_new_slice");

    original = d_array_new_from_arr(sizeof(int), data, 5);

    if (!original)
    {
        return false;
    }

    slice = d_array_s_new_slice(original, sizeof(int), 2);

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "`d_array_s_new_slice` creates slice",
                                    slice != NULL && slice->element_size == sizeof(int),
                                    "`d_array_s_new_slice` failed");

    if (slice) d_array_s_free(slice);
    d_array_free(original);

    return test_result;
}

/*
d_tests_array_s_new_slice_reverse
  Tests the `d_array_s_new_slice_reverse` function.
*/
bool d_tests_array_s_new_slice_reverse(
    struct dj_test_suite* _suite
)
{
    bool              test_result = true;
    struct dj_array*   original;
    struct dj_array_s* slice;
    int               data[] = { 1, 2, 3, 4, 5 };

    d_test_suite_test(_suite, "array_s_new_slice_reverse");

    original = d_array_new_from_arr(sizeof(int), data, 5);

    if (!original)
    {
        return false;
    }

    slice = d_array_s_new_slice_reverse(original, sizeof(int), 0);

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "`d_array_s_new_slice_reverse` creates reversed slice",
                                    slice != NULL,
                                    "`d_array_s_new_slice_reverse` failed");

    if (slice) d_array_s_free(slice);
    d_array_free(original);

    return test_result;
}

/*
d_tests_array_s_new_slice_range
  Tests the `d_array_s_new_slice_range` function.
*/
bool d_tests_array_s_new_slice_range(
    struct dj_test_suite* _suite
)
{
    bool              test_result = true;
    struct dj_array*   original;
    struct dj_array_s* slice;
    int               data[] = { 1, 2, 3, 4, 5 };

    d_test_suite_test(_suite, "array_s_new_slice_range");

    original = d_array_new_from_arr(sizeof(int), data, 5);

    if (!original)
    {
        return false;
    }

    slice = d_array_s_new_slice_range(original, sizeof(int), 1, 3);

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "`d_array_s_new_slice_range` creates range slice",
                                    slice != NULL,
                                    "`d_array_s_new_slice_range` failed");

    if (slice) d_array_s_free(slice);
    d_array_free(original);

    return test_result;
}

/*
d_tests_array_s_new_slice_range_reverse
  Tests the `d_array_s_new_slice_range_reverse` function.
*/
bool d_tests_array_s_new_slice_range_reverse(
    struct dj_test_suite* _suite
)
{
    bool              test_result = true;
    struct dj_array*   original;
    struct dj_array_s* slice;
    int               data[] = { 1, 2, 3, 4, 5 };

    d_test_suite_test(_suite, "array_s_new_slice_range_reverse");

    original = d_array_new_from_arr(sizeof(int), data, 5);

    if (!original)
    {
        return false;
    }

    slice = d_array_s_new_slice_range_reverse(original, sizeof(int), 1, 3);

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "`d_array_s_new_slice_range_reverse` creates reversed range slice",
                                    slice != NULL,
                                    "`d_array_s_new_slice_range_reverse` failed");

    if (slice) d_array_s_free(slice);
    d_array_free(original);

    return test_result;
}


/*
===============================================================================
                        CREATION_S TEST GROUP RUNNER
===============================================================================
*/

/*
d_tests_array_run_creation_s_tests
  Runs all dj_array_s creation function tests.
*/
bool d_tests_array_run_creation_s_tests(
    struct dj_test_suite* _suite
)
{
    bool overall_result = true;

    d_test_suite_block(_suite, "array_run_creation_s_tests");

    printf("\n----------------------------------------\n");
    printf("Running dj_array_s Creation Function Tests\n");

    overall_result &= d_tests_array_s_new(_suite);
    overall_result &= d_tests_array_s_new_default_size(_suite);
    overall_result &= d_tests_array_s_new_from_arr(_suite);
    overall_result &= d_tests_array_s_new_from_args(_suite);
    overall_result &= d_tests_array_s_new_copy(_suite);
    overall_result &= d_tests_array_s_new_copy_reverse(_suite);
    overall_result &= d_tests_array_s_new_copy_range(_suite);
    overall_result &= d_tests_array_s_new_copy_range_reverse(_suite);
    overall_result &= d_tests_array_s_new_fill(_suite);
    overall_result &= d_tests_array_s_new_merge(_suite);
    overall_result &= d_tests_array_s_new_slice(_suite);
    overall_result &= d_tests_array_s_new_slice_reverse(_suite);
    overall_result &= d_tests_array_s_new_slice_range(_suite);
    overall_result &= d_tests_array_s_new_slice_range_reverse(_suite);

    return overall_result;
}
