/*******************************************************************************
* djinterp [test]                                      ptr_vector_tests_resize.c
*
*   Unit tests for ptr_vector resize functions.
*
*
* path:      /tests/djinterp/c/container/vector/ptr_vector_tests_resize.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.01.29
*                                                            revised: 2026.11.09
*******************************************************************************/

// djinterp
#include "./ptr_vector_tests.h"
#include "../../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's C face

static int g_resize_test_values[] = {11, 22, 33, 44, 55};


bool d_tests_ptr_vector_resize(
    struct dj_test_suite* _suite
)
{
    bool                 result;
    struct dj_ptr_vector* vec;
    size_t               i;
    bool                 all_null;

    d_test_suite_test(_suite, "ptr_vector_resize");

    result = true;

    /* test 1: NULL vector should fail */
    result = D_TEST_CHECK_MSG(_suite,
                              "resize_null_vector",
                              d_ptr_vector_resize(NULL, 10) == D_FAILURE,
                              "NULL vector should return D_FAILURE") && result;

    /* test 2: resize to larger (new elements NULL) */
    vec = d_ptr_vector_new_from_args(2,
                                     &g_resize_test_values[0],
                                     &g_resize_test_values[1]);
    if (vec)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "resize_larger",
                                  d_ptr_vector_resize(vec, 5) == D_SUCCESS && vec->count == 5 && vec->elements[0] == &g_resize_test_values[0] && vec->elements[1] == &g_resize_test_values[1],
                                  "Resize larger should preserve original elements") && result;

        all_null = true;
        for (i = 2; i < vec->count; i++)
        {
            if (vec->elements[i] != NULL)
            {
                all_null = false;
                break;
            }
        }

        result = D_TEST_CHECK_MSG(_suite,
                                  "resize_larger_null_fill",
                                  all_null,
                                  "New elements should be NULL") && result;

        d_ptr_vector_free(vec);
    }

    /* test 3: resize to smaller (truncates) */
    vec = d_ptr_vector_new_from_args(5,
                                     &g_resize_test_values[0],
                                     &g_resize_test_values[1],
                                     &g_resize_test_values[2],
                                     &g_resize_test_values[3],
                                     &g_resize_test_values[4]);
    if (vec)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "resize_smaller",
                                  d_ptr_vector_resize(vec, 2) == D_SUCCESS && vec->count == 2 && vec->elements[0] == &g_resize_test_values[0] && vec->elements[1] == &g_resize_test_values[1],
                                  "Resize smaller should truncate") && result;

        d_ptr_vector_free(vec);
    }

    /* test 4: resize to zero */
    vec = d_ptr_vector_new_from_args(3,
                                     &g_resize_test_values[0],
                                     &g_resize_test_values[1],
                                     &g_resize_test_values[2]);
    if (vec)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "resize_to_zero",
                                  d_ptr_vector_resize(vec, 0) == D_SUCCESS && vec->count == 0,
                                  "Resize to zero should succeed") && result;

        d_ptr_vector_free(vec);
    }

    return result;
}


bool d_tests_ptr_vector_resize_fill(
    struct dj_test_suite* _suite
)
{
    bool                 result;
    struct dj_ptr_vector* vec;
    void*                fill_ptr;
    size_t               i;
    bool                 all_match;

    d_test_suite_test(_suite, "ptr_vector_resize_fill");

    result   = true;
    fill_ptr = &g_resize_test_values[4];

    /* test 1: NULL vector should fail */
    result = D_TEST_CHECK_MSG(_suite,
                              "resize_fill_null_vector",
                              d_ptr_vector_resize_fill(NULL, 10, fill_ptr) == D_FAILURE,
                              "NULL vector should return D_FAILURE") && result;

    /* test 2: resize larger with fill value */
    vec = d_ptr_vector_new_from_args(2,
                                     &g_resize_test_values[0],
                                     &g_resize_test_values[1]);
    if (vec)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "resize_fill_larger",
                                  d_ptr_vector_resize_fill(vec, 5, fill_ptr) == D_SUCCESS && vec->count == 5 && vec->elements[0] == &g_resize_test_values[0] && vec->elements[1] == &g_resize_test_values[1],
                                  "Resize fill larger should preserve original") && result;

        all_match = true;
        for (i = 2; i < vec->count; i++)
        {
            if (vec->elements[i] != fill_ptr)
            {
                all_match = false;
                break;
            }
        }

        result = D_TEST_CHECK_MSG(_suite,
                                  "resize_fill_larger_fill_value",
                                  all_match,
                                  "New elements should match fill value") && result;

        d_ptr_vector_free(vec);
    }

    /* test 3: resize with NULL fill value */
    vec = d_ptr_vector_new_from_args(2,
                                     &g_resize_test_values[0],
                                     &g_resize_test_values[1]);
    if (vec)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "resize_fill_null_fill",
                                  d_ptr_vector_resize_fill(vec, 5, NULL) == D_SUCCESS && vec->count == 5 && vec->elements[2] == NULL && vec->elements[3] == NULL && vec->elements[4] == NULL,
                                  "Resize with NULL fill should work") && result;

        d_ptr_vector_free(vec);
    }

    return result;
}


bool d_tests_ptr_vector_resize_all(
    struct dj_test_suite* _suite
)
{
    bool result;

    d_test_suite_block(_suite, "ptr_vector_resize_all");

    result = true;

    result = d_tests_ptr_vector_resize(_suite) && result;
    result = d_tests_ptr_vector_resize_fill(_suite) && result;

    return result;
}
