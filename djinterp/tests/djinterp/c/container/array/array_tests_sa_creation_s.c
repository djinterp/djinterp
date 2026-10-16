/*******************************************************************************
* djinterp [container]                               array_tests_sa_creation_s.c
*
* Unit tests for `array.h` d_array_s creation functions.
* Part of the array standalone test suite.
*
*
* path:      /tests/djinterp/c/container/array/array_tests_sa_creation_s.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.01.30
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "./array_tests_sa.h"
#include "../../../../../inc/djinterp/test/c/test_counter.h"


/*
d_tests_sa_array_s_new
  Tests the `d_array_s_new` function.
*/
bool
d_tests_sa_array_s_new
(
    struct d_test_counter* _test_info
)
{
    bool              test_result = true;
    struct d_array_s* arr;

    printf("Testing `d_array_s_new`...\n");

    arr = d_array_s_new(sizeof(int), 10);

    test_result &= d_assert_standalone(
        arr != NULL && arr->count == 0 && arr->element_size == sizeof(int),
        "`d_array_s_new` creates array with correct fields",
        "`d_array_s_new` failed",
        _test_info
    );

    if (arr) d_array_s_free(arr);

    arr = d_array_s_new(0, 10);

    test_result &= d_assert_standalone(
        arr == NULL,
        "`d_array_s_new` rejects zero element size",
        "`d_array_s_new` accepted zero element size",
        _test_info
    );

    _test_info->tests_total++;
    if (test_result) _test_info->tests_passed++;

    return test_result;
}

/*
d_tests_sa_array_s_new_default_size
  Tests the `d_array_s_new_default_size` function.
*/
bool
d_tests_sa_array_s_new_default_size
(
    struct d_test_counter* _test_info
)
{
    bool              test_result = true;
    struct d_array_s* arr;

    printf("Testing `d_array_s_new_default_size`...\n");

    arr = d_array_s_new_default_size(sizeof(int));

    test_result &= d_assert_standalone(
        arr != NULL && arr->element_size == sizeof(int),
        "`d_array_s_new_default_size` creates array correctly",
        "`d_array_s_new_default_size` failed",
        _test_info
    );

    if (arr) d_array_s_free(arr);

    _test_info->tests_total++;
    if (test_result) _test_info->tests_passed++;

    return test_result;
}

/*
d_tests_sa_array_s_new_from_arr
  Tests the `d_array_s_new_from_arr` function.
*/
bool
d_tests_sa_array_s_new_from_arr
(
    struct d_test_counter* _test_info
)
{
    bool              test_result = true;
    struct d_array_s* arr;
    int               data[] = { 1, 2, 3, 4, 5 };

    printf("Testing `d_array_s_new_from_arr`...\n");

    arr = d_array_s_new_from_arr(sizeof(int), data, 5);

    test_result &= d_assert_standalone(
        arr != NULL && arr->count == 5 && arr->element_size == sizeof(int),
        "`d_array_s_new_from_arr` creates array correctly",
        "`d_array_s_new_from_arr` failed",
        _test_info
    );

    if (arr) d_array_s_free(arr);

    _test_info->tests_total++;
    if (test_result) _test_info->tests_passed++;

    return test_result;
}

/*
d_tests_sa_array_s_new_from_args
  Tests the `d_array_s_new_from_args` function.
*/
bool
d_tests_sa_array_s_new_from_args
(
    struct d_test_counter* _test_info
)
{
    bool              test_result = true;
    struct d_array_s* arr;

    printf("Testing `d_array_s_new_from_args`...\n");

    arr = d_array_s_new_from_args(sizeof(int), 5, 1, 2, 3, 4, 5);

    test_result &= d_assert_standalone(
        arr != NULL && arr->count == 5 && arr->element_size == sizeof(int),
        "`d_array_s_new_from_args` creates array correctly",
        "`d_array_s_new_from_args` failed",
        _test_info
    );

    if (arr) d_array_s_free(arr);

    _test_info->tests_total++;
    if (test_result) _test_info->tests_passed++;

    return test_result;
}

/*
d_tests_sa_array_s_new_copy
  Tests the `d_array_s_new_copy` function.
*/
bool
d_tests_sa_array_s_new_copy
(
    struct d_test_counter* _test_info
)
{
    bool              test_result = true;
    struct d_array*   original;
    struct d_array_s* copy;
    int               data[] = { 1, 2, 3, 4, 5 };

    printf("Testing `d_array_s_new_copy`...\n");

    original = d_array_new_from_arr(sizeof(int), data, 5);

    if (!original)
    {
        _test_info->tests_total++;

        return false;
    }

    copy = d_array_s_new_copy(original, sizeof(int));

    test_result &= d_assert_standalone(
        copy != NULL && copy->count == 5 && copy->element_size == sizeof(int),
        "`d_array_s_new_copy` creates copy correctly",
        "`d_array_s_new_copy` failed",
        _test_info
    );

    if (copy) d_array_s_free(copy);
    d_array_free(original);

    _test_info->tests_total++;
    if (test_result) _test_info->tests_passed++;

    return test_result;
}

/*
d_tests_sa_array_s_new_copy_reverse
  Tests the `d_array_s_new_copy_reverse` function.
*/
bool
d_tests_sa_array_s_new_copy_reverse
(
    struct d_test_counter* _test_info
)
{
    bool              test_result = true;
    struct d_array*   original;
    struct d_array_s* copy;
    int               data[] = { 1, 2, 3, 4, 5 };
    int*              int_elements;

    printf("Testing `d_array_s_new_copy_reverse`...\n");

    original = d_array_new_from_arr(sizeof(int), data, 5);

    if (!original)
    {
        _test_info->tests_total++;

        return false;
    }

    copy = d_array_s_new_copy_reverse(original, sizeof(int), 0, 4);
    int_elements = copy ? (int*)copy->elements : NULL;

    test_result &= d_assert_standalone(
        copy != NULL && int_elements[0] == 5 && int_elements[4] == 1,
        "`d_array_s_new_copy_reverse` creates reversed copy",
        "`d_array_s_new_copy_reverse` failed",
        _test_info
    );

    if (copy) d_array_s_free(copy);
    d_array_free(original);

    _test_info->tests_total++;
    if (test_result) _test_info->tests_passed++;

    return test_result;
}

/*
d_tests_sa_array_s_new_copy_range
  Tests the `d_array_s_new_copy_range` function.
*/
bool
d_tests_sa_array_s_new_copy_range
(
    struct d_test_counter* _test_info
)
{
    bool              test_result = true;
    struct d_array*   original;
    struct d_array_s* copy;
    int               data[] = { 1, 2, 3, 4, 5 };
    int*              int_elements;

    printf("Testing `d_array_s_new_copy_range`...\n");

    original = d_array_new_from_arr(sizeof(int), data, 5);

    if (!original)
    {
        _test_info->tests_total++;

        return false;
    }

    copy = d_array_s_new_copy_range(original, sizeof(int), 1, 3);
    int_elements = copy ? (int*)copy->elements : NULL;

    test_result &= d_assert_standalone(
        copy != NULL && copy->count == 3 && int_elements[0] == 2,
        "`d_array_s_new_copy_range` creates range copy",
        "`d_array_s_new_copy_range` failed",
        _test_info
    );

    if (copy) d_array_s_free(copy);
    d_array_free(original);

    _test_info->tests_total++;
    if (test_result) _test_info->tests_passed++;

    return test_result;
}

/*
d_tests_sa_array_s_new_copy_range_reverse
  Tests the `d_array_s_new_copy_range_reverse` function.
*/
bool
d_tests_sa_array_s_new_copy_range_reverse
(
    struct d_test_counter* _test_info
)
{
    bool              test_result = true;
    struct d_array*   original;
    struct d_array_s* copy;
    int               data[] = { 1, 2, 3, 4, 5 };
    int*              int_elements;

    printf("Testing `d_array_s_new_copy_range_reverse`...\n");

    original = d_array_new_from_arr(sizeof(int), data, 5);

    if (!original)
    {
        _test_info->tests_total++;

        return false;
    }

    copy = d_array_s_new_copy_range_reverse(original, sizeof(int), 1, 3);
    int_elements = copy ? (int*)copy->elements : NULL;

    test_result &= d_assert_standalone(
        copy != NULL && copy->count == 3 && int_elements[0] == 4,
        "`d_array_s_new_copy_range_reverse` creates reversed range",
        "`d_array_s_new_copy_range_reverse` failed",
        _test_info
    );

    if (copy) d_array_s_free(copy);
    d_array_free(original);

    _test_info->tests_total++;
    if (test_result) _test_info->tests_passed++;

    return test_result;
}

/*
d_tests_sa_array_s_new_fill
  Tests the `d_array_s_new_fill` function.
*/
bool
d_tests_sa_array_s_new_fill
(
    struct d_test_counter* _test_info
)
{
    bool              test_result = true;
    struct d_array_s* arr;
    int               fill_value = 42;
    int*              int_elements;

    printf("Testing `d_array_s_new_fill`...\n");

    arr = d_array_s_new_fill(sizeof(int), 5, &fill_value);
    int_elements = arr ? (int*)arr->elements : NULL;

    test_result &= d_assert_standalone(
        arr != NULL && arr->count == 5 && int_elements[0] == 42,
        "`d_array_s_new_fill` creates filled array",
        "`d_array_s_new_fill` failed",
        _test_info
    );

    if (arr) d_array_s_free(arr);

    _test_info->tests_total++;
    if (test_result) _test_info->tests_passed++;

    return test_result;
}

/*
d_tests_sa_array_s_new_merge
  Tests the `d_array_s_new_merge` function.
*/
bool
d_tests_sa_array_s_new_merge
(
    struct d_test_counter* _test_info
)
{
    bool              test_result = true;
    struct d_array_s* merged;
    struct d_array_s* arr1;
    struct d_array_s* arr2;
    int               data1[] = { 1, 2, 3 };
    int               data2[] = { 4, 5 };

    printf("Testing `d_array_s_new_merge`...\n");

    arr1 = d_array_s_new_from_arr(sizeof(int), data1, 3);
    arr2 = d_array_s_new_from_arr(sizeof(int), data2, 2);

    if (!arr1 || !arr2)
    {
        if (arr1) d_array_s_free(arr1);
        if (arr2) d_array_s_free(arr2);
        _test_info->tests_total++;

        return false;
    }

    merged = d_array_s_new_merge(sizeof(int), 2, arr1, arr2);

    test_result &= d_assert_standalone(
        merged != NULL && merged->count == 5,
        "`d_array_s_new_merge` creates merged array",
        "`d_array_s_new_merge` failed",
        _test_info
    );

    if (merged) d_array_s_free(merged);
    d_array_s_free(arr1);
    d_array_s_free(arr2);

    _test_info->tests_total++;
    if (test_result) _test_info->tests_passed++;

    return test_result;
}

/*
d_tests_sa_array_s_new_slice
  Tests the `d_array_s_new_slice` function.
*/
bool
d_tests_sa_array_s_new_slice
(
    struct d_test_counter* _test_info
)
{
    bool              test_result = true;
    struct d_array*   original;
    struct d_array_s* slice;
    int               data[] = { 1, 2, 3, 4, 5 };

    printf("Testing `d_array_s_new_slice`...\n");

    original = d_array_new_from_arr(sizeof(int), data, 5);

    if (!original)
    {
        _test_info->tests_total++;

        return false;
    }

    slice = d_array_s_new_slice(original, sizeof(int), 2);

    test_result &= d_assert_standalone(
        slice != NULL && slice->element_size == sizeof(int),
        "`d_array_s_new_slice` creates slice",
        "`d_array_s_new_slice` failed",
        _test_info
    );

    if (slice) d_array_s_free(slice);
    d_array_free(original);

    _test_info->tests_total++;
    if (test_result) _test_info->tests_passed++;

    return test_result;
}

/*
d_tests_sa_array_s_new_slice_reverse
  Tests the `d_array_s_new_slice_reverse` function.
*/
bool
d_tests_sa_array_s_new_slice_reverse
(
    struct d_test_counter* _test_info
)
{
    bool              test_result = true;
    struct d_array*   original;
    struct d_array_s* slice;
    int               data[] = { 1, 2, 3, 4, 5 };

    printf("Testing `d_array_s_new_slice_reverse`...\n");

    original = d_array_new_from_arr(sizeof(int), data, 5);

    if (!original)
    {
        _test_info->tests_total++;

        return false;
    }

    slice = d_array_s_new_slice_reverse(original, sizeof(int), 0);

    test_result &= d_assert_standalone(
        slice != NULL,
        "`d_array_s_new_slice_reverse` creates reversed slice",
        "`d_array_s_new_slice_reverse` failed",
        _test_info
    );

    if (slice) d_array_s_free(slice);
    d_array_free(original);

    _test_info->tests_total++;
    if (test_result) _test_info->tests_passed++;

    return test_result;
}

/*
d_tests_sa_array_s_new_slice_range
  Tests the `d_array_s_new_slice_range` function.
*/
bool
d_tests_sa_array_s_new_slice_range
(
    struct d_test_counter* _test_info
)
{
    bool              test_result = true;
    struct d_array*   original;
    struct d_array_s* slice;
    int               data[] = { 1, 2, 3, 4, 5 };

    printf("Testing `d_array_s_new_slice_range`...\n");

    original = d_array_new_from_arr(sizeof(int), data, 5);

    if (!original)
    {
        _test_info->tests_total++;

        return false;
    }

    slice = d_array_s_new_slice_range(original, sizeof(int), 1, 3);

    test_result &= d_assert_standalone(
        slice != NULL,
        "`d_array_s_new_slice_range` creates range slice",
        "`d_array_s_new_slice_range` failed",
        _test_info
    );

    if (slice) d_array_s_free(slice);
    d_array_free(original);

    _test_info->tests_total++;
    if (test_result) _test_info->tests_passed++;

    return test_result;
}

/*
d_tests_sa_array_s_new_slice_range_reverse
  Tests the `d_array_s_new_slice_range_reverse` function.
*/
bool
d_tests_sa_array_s_new_slice_range_reverse
(
    struct d_test_counter* _test_info
)
{
    bool              test_result = true;
    struct d_array*   original;
    struct d_array_s* slice;
    int               data[] = { 1, 2, 3, 4, 5 };

    printf("Testing `d_array_s_new_slice_range_reverse`...\n");

    original = d_array_new_from_arr(sizeof(int), data, 5);

    if (!original)
    {
        _test_info->tests_total++;

        return false;
    }

    slice = d_array_s_new_slice_range_reverse(original, sizeof(int), 1, 3);

    test_result &= d_assert_standalone(
        slice != NULL,
        "`d_array_s_new_slice_range_reverse` creates reversed range slice",
        "`d_array_s_new_slice_range_reverse` failed",
        _test_info
    );

    if (slice) d_array_s_free(slice);
    d_array_free(original);

    _test_info->tests_total++;
    if (test_result) _test_info->tests_passed++;

    return test_result;
}


/*
===============================================================================
                        CREATION_S TEST GROUP RUNNER
===============================================================================
*/

/*
d_tests_sa_array_run_creation_s_tests
  Runs all d_array_s creation function tests.
*/
bool
d_tests_sa_array_run_creation_s_tests
(
    struct d_test_counter* _test_info
)
{
    bool overall_result = true;

    printf("\n----------------------------------------\n");
    printf("Running d_array_s Creation Function Tests\n");
    printf("----------------------------------------\n");

    overall_result &= d_tests_sa_array_s_new(_test_info);
    overall_result &= d_tests_sa_array_s_new_default_size(_test_info);
    overall_result &= d_tests_sa_array_s_new_from_arr(_test_info);
    overall_result &= d_tests_sa_array_s_new_from_args(_test_info);
    overall_result &= d_tests_sa_array_s_new_copy(_test_info);
    overall_result &= d_tests_sa_array_s_new_copy_reverse(_test_info);
    overall_result &= d_tests_sa_array_s_new_copy_range(_test_info);
    overall_result &= d_tests_sa_array_s_new_copy_range_reverse(_test_info);
    overall_result &= d_tests_sa_array_s_new_fill(_test_info);
    overall_result &= d_tests_sa_array_s_new_merge(_test_info);
    overall_result &= d_tests_sa_array_s_new_slice(_test_info);
    overall_result &= d_tests_sa_array_s_new_slice_reverse(_test_info);
    overall_result &= d_tests_sa_array_s_new_slice_range(_test_info);
    overall_result &= d_tests_sa_array_s_new_slice_range_reverse(_test_info);

    return overall_result;
}
