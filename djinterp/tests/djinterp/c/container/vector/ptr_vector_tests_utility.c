/*******************************************************************************
* djinterp [test]                                     ptr_vector_tests_utility.c
*
*   Unit tests for ptr_vector utility functions.
*
*
* path:      /tests/djinterp/c/container/vector/ptr_vector_tests_utility.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.01.29
*                                                            revised: 2026.11.09
*******************************************************************************/

// djinterp
#include "./ptr_vector_tests.h"
#include "../../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's C face

static int g_util_test_values[] = {50, 10, 40, 20, 30};

/*
 * int_ptr_comparator
 *   Comparator for sorting an array of void* pointers to integers.
 *   When qsort operates on void*[] array, the comparator receives void**
 *   (pointers to elements of the array, which are themselves void*).
 */
static
int int_ptr_comparator(
    const void* _a,
    const void* _b
)
{
    const void* ptr_a;
    const void* ptr_b;
    int         val_a;
    int         val_b;

    /* _a and _b are pointers to elements of the void*[] array */
    ptr_a = *(const void**)_a;
    ptr_b = *(const void**)_b;

    if (!ptr_a && !ptr_b) return 0;
    if (!ptr_a) return -1;
    if (!ptr_b) return 1;

    val_a = *(const int*)ptr_a;
    val_b = *(const int*)ptr_b;

    return (val_a > val_b) - (val_a < val_b);
}


bool d_tests_ptr_vector_swap(
    struct dj_test_suite* _suite
)
{
    bool                 result;
    struct dj_ptr_vector* vec;

    d_test_suite_test(_suite, "ptr_vector_swap");

    result = true;

    /* test 1: NULL vector should fail */
    result = D_TEST_CHECK_MSG(_suite,
                              "swap_null_vector",
                              d_ptr_vector_swap(NULL, 0, 1) == D_FAILURE,
                              "NULL vector should return D_FAILURE") && result;

    /* test 2: swap elements */
    vec = d_ptr_vector_new_from_args(3,
        &g_util_test_values[0], &g_util_test_values[1], &g_util_test_values[2]);
    if (vec)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "swap_success",
                                  d_ptr_vector_swap(vec, 0, 2) == D_SUCCESS && vec->elements[0] == &g_util_test_values[2] && vec->elements[2] == &g_util_test_values[0],
                                  "Swap should exchange elements") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "swap_same_index",
                                  d_ptr_vector_swap(vec, 1, 1) == D_SUCCESS,
                                  "Swap same index should succeed (no-op)") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "swap_negative_index",
                                  d_ptr_vector_swap(vec, -1, 0) == D_SUCCESS,
                                  "Swap with negative index should work") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "swap_oob",
                                  d_ptr_vector_swap(vec, 0, 100) == D_FAILURE,
                                  "Out-of-bounds swap should fail") && result;

        d_ptr_vector_free(vec);
    }

    return result;
}


bool d_tests_ptr_vector_reverse(
    struct dj_test_suite* _suite
)
{
    bool                 result;
    struct dj_ptr_vector* vec;

    d_test_suite_test(_suite, "ptr_vector_reverse");

    result = true;

    /* test 1: NULL vector should fail */
    result = D_TEST_CHECK_MSG(_suite,
                              "reverse_null_vector",
                              d_ptr_vector_reverse(NULL) == D_FAILURE,
                              "NULL vector should return D_FAILURE") && result;

    /* test 2: reverse elements */
    vec = d_ptr_vector_new_from_args(5,
        &g_util_test_values[0], &g_util_test_values[1], &g_util_test_values[2],
        &g_util_test_values[3], &g_util_test_values[4]);
    if (vec)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "reverse_success",
                                  d_ptr_vector_reverse(vec) == D_SUCCESS && vec->elements[0] == &g_util_test_values[4] && vec->elements[1] == &g_util_test_values[3] && vec->elements[2] == &g_util_test_values[2] && vec->elements[3] == &g_util_test_values[1] && vec->elements[4] == &g_util_test_values[0],
                                  "Reverse should reverse element order") && result;

        d_ptr_vector_free(vec);
    }

    /* test 3: single element */
    vec = d_ptr_vector_new_from_args(1, &g_util_test_values[0]);
    if (vec)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "reverse_single",
                                  d_ptr_vector_reverse(vec) == D_SUCCESS && vec->elements[0] == &g_util_test_values[0],
                                  "Reverse single element should succeed") && result;

        d_ptr_vector_free(vec);
    }

    return result;
}


bool d_tests_ptr_vector_sort(
    struct dj_test_suite* _suite
)
{
    bool                 result;
    struct dj_ptr_vector* vec;

    d_test_suite_test(_suite, "ptr_vector_sort");

    result = true;

    /* test 1: NULL checks - sort returns void */
    d_ptr_vector_sort(NULL, int_ptr_comparator);
    result = D_TEST_CHECK_MSG(_suite,
                              "sort_null_vector",
                              true,
                              "Sort NULL vector should not crash") && result;

    /* test 2: sort elements */
    vec = d_ptr_vector_new_from_args(5,
        &g_util_test_values[0], &g_util_test_values[1], &g_util_test_values[2],
        &g_util_test_values[3], &g_util_test_values[4]);  /* 50, 10, 40, 20, 30 */
    if (vec)
    {
        d_ptr_vector_sort(vec, int_ptr_comparator);

        /* After sort: 10, 20, 30, 40, 50 */
        result = D_TEST_CHECK_MSG(_suite,
                                  "sort_success",
                                  *(int*)vec->elements[0] == 10 && *(int*)vec->elements[1] == 20 && *(int*)vec->elements[2] == 30 && *(int*)vec->elements[3] == 40 && *(int*)vec->elements[4] == 50,
                                  "Sort should order elements correctly") && result;

        d_ptr_vector_free(vec);
    }

    /* test 3: empty vector */
    vec = d_ptr_vector_new(10);
    if (vec)
    {
        d_ptr_vector_sort(vec, int_ptr_comparator);
        result = D_TEST_CHECK_MSG(_suite,
                                  "sort_empty",
                                  vec->count == 0,
                                  "Sort empty vector should work") && result;

        d_ptr_vector_free(vec);
    }

    return result;
}


bool d_tests_ptr_vector_copy_to(
    struct dj_test_suite* _suite
)
{
    bool                 result;
    struct dj_ptr_vector* vec;
    void*                dest[10];

    d_test_suite_test(_suite, "ptr_vector_copy_to");

    result = true;

    /* test 1: NULL vector should fail */
    result = D_TEST_CHECK_MSG(_suite,
                              "copy_to_null_vector",
                              d_ptr_vector_copy_to(NULL, dest, 10) == D_FAILURE,
                              "NULL vector should return D_FAILURE") && result;

    /* test 2: copy tests */
    vec = d_ptr_vector_new_from_args(3,
        &g_util_test_values[0], &g_util_test_values[1], &g_util_test_values[2]);
    if (vec)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "copy_to_null_dest",
                                  d_ptr_vector_copy_to(vec, NULL, 10) == D_FAILURE,
                                  "NULL destination should return D_FAILURE") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "copy_to_small_dest",
                                  d_ptr_vector_copy_to(vec, dest, 2) == D_FAILURE,
                                  "Too small destination should fail") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "copy_to_success",
                                  d_ptr_vector_copy_to(vec, dest, 10) == D_SUCCESS && dest[0] == &g_util_test_values[0] && dest[1] == &g_util_test_values[1] && dest[2] == &g_util_test_values[2],
                                  "Copy to should copy all elements") && result;

        d_ptr_vector_free(vec);
    }

    return result;
}


bool d_tests_ptr_vector_utility_all(
    struct dj_test_suite* _suite
)
{
    bool result;

    d_test_suite_block(_suite, "ptr_vector_utility_all");

    result = true;

    result = d_tests_ptr_vector_swap(_suite) && result;
    result = d_tests_ptr_vector_reverse(_suite) && result;
    result = d_tests_ptr_vector_sort(_suite) && result;
    result = d_tests_ptr_vector_copy_to(_suite) && result;

    return result;
}
