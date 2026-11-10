/*******************************************************************************
* djinterp [test]                                      ptr_vector_tests_append.c
*
*   Unit tests for ptr_vector append/extend functions.
*
*
* path:      /tests/djinterp/c/container/vector/ptr_vector_tests_append.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.01.29
*                                                            revised: 2026.11.09
*******************************************************************************/

// djinterp
#include "./ptr_vector_tests.h"
#include "../../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's C face

static int g_append_test_values[] = {10, 20, 30, 40, 50, 60, 70, 80, 90, 100};


bool d_tests_ptr_vector_append(
    struct dj_test_suite* _suite
)
{
    bool                 result;
    struct dj_ptr_vector* vec;
    void*                source[3];

    d_test_suite_test(_suite, "ptr_vector_append");

    result = true;

    source[0] = &g_append_test_values[3];
    source[1] = &g_append_test_values[4];
    source[2] = &g_append_test_values[5];

    /* test 1: NULL vector should fail */
    result = D_TEST_CHECK_MSG(_suite,
                              "append_null_vector",
                              d_ptr_vector_append(NULL, (const void**)source, 3) == D_FAILURE,
                              "NULL vector should return D_FAILURE") && result;

    /* test 2: NULL source with non-zero count should fail */
    vec = d_ptr_vector_new(10);
    if (vec)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "append_null_source",
                                  d_ptr_vector_append(vec, NULL, 3) == D_FAILURE,
                                  "NULL source with non-zero count should fail") && result;

        d_ptr_vector_free(vec);
    }

    /* test 3: zero count should succeed (no-op) */
    vec = d_ptr_vector_new_from_args(2,
                                     &g_append_test_values[0],
                                     &g_append_test_values[1]);
    if (vec)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "append_zero_count",
                                  d_ptr_vector_append(vec, (const void**)source, 0) == D_SUCCESS && vec->count == 2,
                                  "Zero count should be no-op") && result;

        d_ptr_vector_free(vec);
    }

    /* test 4: append to empty vector */
    vec = d_ptr_vector_new(10);
    if (vec)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "append_empty",
                                  d_ptr_vector_append(vec, (const void**)source, 3) == D_SUCCESS && vec->count == 3 && vec->elements[0] == &g_append_test_values[3],
                                  "Append to empty vector should succeed") && result;

        d_ptr_vector_free(vec);
    }

    /* test 5: append to non-empty vector */
    vec = d_ptr_vector_new_from_args(2,
                                     &g_append_test_values[0],
                                     &g_append_test_values[1]);
    if (vec)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "append_nonempty",
                                  d_ptr_vector_append(vec, (const void**)source, 3) == D_SUCCESS && vec->count == 5 && vec->elements[0] == &g_append_test_values[0] && vec->elements[2] == &g_append_test_values[3],
                                  "Original preserved, new elements appended") && result;

        d_ptr_vector_free(vec);
    }

    return result;
}


bool d_tests_ptr_vector_append_vector(
    struct dj_test_suite* _suite
)
{
    bool                 result;
    struct dj_ptr_vector* dest;
    struct dj_ptr_vector* src;

    d_test_suite_test(_suite, "ptr_vector_append_vector");

    result = true;

    /* test 1: NULL destination should fail */
    src = d_ptr_vector_new_from_args(2,
                                     &g_append_test_values[0],
                                     &g_append_test_values[1]);
    if (src)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "append_vector_null_dest",
                                  d_ptr_vector_append_vector(NULL, src) == D_FAILURE,
                                  "NULL destination should return D_FAILURE") && result;

        d_ptr_vector_free(src);
    }

    /* test 2: NULL source should fail */
    dest = d_ptr_vector_new(10);
    if (dest)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "append_vector_null_src",
                                  d_ptr_vector_append_vector(dest, NULL) == D_FAILURE,
                                  "NULL source should return D_FAILURE") && result;

        d_ptr_vector_free(dest);
    }

    /* test 3: successful vector-to-vector append */
    dest = d_ptr_vector_new_from_args(2,
                                      &g_append_test_values[0],
                                      &g_append_test_values[1]);
    src = d_ptr_vector_new_from_args(2,
                                     &g_append_test_values[2],
                                     &g_append_test_values[3]);
    if (dest && src)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "append_vector_success",
                                  d_ptr_vector_append_vector(dest, src) == D_SUCCESS && dest->count == 4 && dest->elements[2] == &g_append_test_values[2] && src->count == 2,
                                  "Append vector should succeed, src unchanged") && result;

        d_ptr_vector_free(src);
        d_ptr_vector_free(dest);
    }
    else
    {
        if (dest) d_ptr_vector_free(dest);
        if (src) d_ptr_vector_free(src);
    }

    return result;
}


bool d_tests_ptr_vector_prepend(
    struct dj_test_suite* _suite
)
{
    bool                 result;
    struct dj_ptr_vector* vec;
    void*                source[3];

    d_test_suite_test(_suite, "ptr_vector_prepend");

    result = true;

    source[0] = &g_append_test_values[0];
    source[1] = &g_append_test_values[1];
    source[2] = &g_append_test_values[2];

    /* test 1: NULL vector should fail */
    result = D_TEST_CHECK_MSG(_suite,
                              "prepend_null_vector",
                              d_ptr_vector_prepend(NULL, (const void**)source, 3) == D_FAILURE,
                              "NULL vector should return D_FAILURE") && result;

    /* test 2: prepend to non-empty vector (shifts existing) */
    vec = d_ptr_vector_new_from_args(2,
                                     &g_append_test_values[5],
                                     &g_append_test_values[6]);
    if (vec)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "prepend_nonempty",
                                  d_ptr_vector_prepend(vec, (const void**)source, 3) == D_SUCCESS && vec->count == 5 && vec->elements[0] == &g_append_test_values[0] && vec->elements[3] == &g_append_test_values[5],
                                  "New elements prepended, original shifted") && result;

        d_ptr_vector_free(vec);
    }

    return result;
}


bool d_tests_ptr_vector_prepend_vector(
    struct dj_test_suite* _suite
)
{
    bool                 result;
    struct dj_ptr_vector* dest;
    struct dj_ptr_vector* src;

    d_test_suite_test(_suite, "ptr_vector_prepend_vector");

    result = true;

    /* test 1: NULL destination should fail */
    src = d_ptr_vector_new_from_args(2,
                                     &g_append_test_values[0],
                                     &g_append_test_values[1]);
    if (src)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "prepend_vector_null_dest",
                                  d_ptr_vector_prepend_vector(NULL, src) == D_FAILURE,
                                  "NULL destination should return D_FAILURE") && result;

        d_ptr_vector_free(src);
    }

    /* test 2: successful vector-to-vector prepend */
    dest = d_ptr_vector_new_from_args(2,
                                      &g_append_test_values[2],
                                      &g_append_test_values[3]);
    src = d_ptr_vector_new_from_args(2,
                                     &g_append_test_values[0],
                                     &g_append_test_values[1]);
    if (dest && src)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "prepend_vector_success",
                                  d_ptr_vector_prepend_vector(dest, src) == D_SUCCESS && dest->count == 4 && dest->elements[0] == &g_append_test_values[0] && dest->elements[2] == &g_append_test_values[2],
                                  "Source prepended, destination shifted") && result;

        d_ptr_vector_free(src);
        d_ptr_vector_free(dest);
    }
    else
    {
        if (dest) d_ptr_vector_free(dest);
        if (src) d_ptr_vector_free(src);
    }

    return result;
}


bool d_tests_ptr_vector_append_all(
    struct dj_test_suite* _suite
)
{
    bool result;

    d_test_suite_block(_suite, "ptr_vector_append_all");

    result = true;

    result = d_tests_ptr_vector_append(_suite) && result;
    result = d_tests_ptr_vector_append_vector(_suite) && result;
    result = d_tests_ptr_vector_prepend(_suite) && result;
    result = d_tests_ptr_vector_prepend_vector(_suite) && result;

    return result;
}
