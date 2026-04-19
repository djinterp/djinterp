/******************************************************************************
* djinterp [test]                                     ptr_vector_tests_sa_append.c
*
*   Unit tests for ptr_vector append/extend functions.
*
* path:      \tests\container\vector\ptr_vector_tests_sa_append.c
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.01.29
******************************************************************************/

#include "./ptr_vector_tests_sa.h"

static int g_append_test_values[] = {10, 20, 30, 40, 50, 60, 70, 80, 90, 100};


bool
d_tests_sa_ptr_vector_append
(
    struct d_test_counter* _counter
)
{
    bool                 result;
    struct d_ptr_vector* vec;
    void*                source[3];

    result = true;

    source[0] = &g_append_test_values[3];
    source[1] = &g_append_test_values[4];
    source[2] = &g_append_test_values[5];

    /* test 1: NULL vector should fail */
    result = d_assert_standalone(
        d_ptr_vector_append(NULL, (const void**)source, 3) == D_FAILURE,
        "append_null_vector",
        "NULL vector should return D_FAILURE",
        _counter) && result;

    /* test 2: NULL source with non-zero count should fail */
    vec = d_ptr_vector_new(10);
    if (vec)
    {
        result = d_assert_standalone(
            d_ptr_vector_append(vec, NULL, 3) == D_FAILURE,
            "append_null_source",
            "NULL source with non-zero count should fail",
            _counter) && result;

        d_ptr_vector_free(vec);
    }

    /* test 3: zero count should succeed (no-op) */
    vec = d_ptr_vector_new_from_args(2,
                                     &g_append_test_values[0],
                                     &g_append_test_values[1]);
    if (vec)
    {
        result = d_assert_standalone(
            d_ptr_vector_append(vec, (const void**)source, 0) == D_SUCCESS &&
            vec->count == 2,
            "append_zero_count",
            "Zero count should be no-op",
            _counter) && result;

        d_ptr_vector_free(vec);
    }

    /* test 4: append to empty vector */
    vec = d_ptr_vector_new(10);
    if (vec)
    {
        result = d_assert_standalone(
            d_ptr_vector_append(vec, (const void**)source, 3) == D_SUCCESS &&
            vec->count == 3 &&
            vec->elements[0] == &g_append_test_values[3],
            "append_empty",
            "Append to empty vector should succeed",
            _counter) && result;

        d_ptr_vector_free(vec);
    }

    /* test 5: append to non-empty vector */
    vec = d_ptr_vector_new_from_args(2,
                                     &g_append_test_values[0],
                                     &g_append_test_values[1]);
    if (vec)
    {
        result = d_assert_standalone(
            d_ptr_vector_append(vec, (const void**)source, 3) == D_SUCCESS &&
            vec->count == 5 &&
            vec->elements[0] == &g_append_test_values[0] &&
            vec->elements[2] == &g_append_test_values[3],
            "append_nonempty",
            "Original preserved, new elements appended",
            _counter) && result;

        d_ptr_vector_free(vec);
    }

    return result;
}


bool
d_tests_sa_ptr_vector_append_vector
(
    struct d_test_counter* _counter
)
{
    bool                 result;
    struct d_ptr_vector* dest;
    struct d_ptr_vector* src;

    result = true;

    /* test 1: NULL destination should fail */
    src = d_ptr_vector_new_from_args(2,
                                     &g_append_test_values[0],
                                     &g_append_test_values[1]);
    if (src)
    {
        result = d_assert_standalone(
            d_ptr_vector_append_vector(NULL, src) == D_FAILURE,
            "append_vector_null_dest",
            "NULL destination should return D_FAILURE",
            _counter) && result;

        d_ptr_vector_free(src);
    }

    /* test 2: NULL source should fail */
    dest = d_ptr_vector_new(10);
    if (dest)
    {
        result = d_assert_standalone(
            d_ptr_vector_append_vector(dest, NULL) == D_FAILURE,
            "append_vector_null_src",
            "NULL source should return D_FAILURE",
            _counter) && result;

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
        result = d_assert_standalone(
            d_ptr_vector_append_vector(dest, src) == D_SUCCESS &&
            dest->count == 4 &&
            dest->elements[2] == &g_append_test_values[2] &&
            src->count == 2,
            "append_vector_success",
            "Append vector should succeed, src unchanged",
            _counter) && result;

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


bool
d_tests_sa_ptr_vector_prepend
(
    struct d_test_counter* _counter
)
{
    bool                 result;
    struct d_ptr_vector* vec;
    void*                source[3];

    result = true;

    source[0] = &g_append_test_values[0];
    source[1] = &g_append_test_values[1];
    source[2] = &g_append_test_values[2];

    /* test 1: NULL vector should fail */
    result = d_assert_standalone(
        d_ptr_vector_prepend(NULL, (const void**)source, 3) == D_FAILURE,
        "prepend_null_vector",
        "NULL vector should return D_FAILURE",
        _counter) && result;

    /* test 2: prepend to non-empty vector (shifts existing) */
    vec = d_ptr_vector_new_from_args(2,
                                     &g_append_test_values[5],
                                     &g_append_test_values[6]);
    if (vec)
    {
        result = d_assert_standalone(
            d_ptr_vector_prepend(vec, (const void**)source, 3) == D_SUCCESS &&
            vec->count == 5 &&
            vec->elements[0] == &g_append_test_values[0] &&
            vec->elements[3] == &g_append_test_values[5],
            "prepend_nonempty",
            "New elements prepended, original shifted",
            _counter) && result;

        d_ptr_vector_free(vec);
    }

    return result;
}


bool
d_tests_sa_ptr_vector_prepend_vector
(
    struct d_test_counter* _counter
)
{
    bool                 result;
    struct d_ptr_vector* dest;
    struct d_ptr_vector* src;

    result = true;

    /* test 1: NULL destination should fail */
    src = d_ptr_vector_new_from_args(2,
                                     &g_append_test_values[0],
                                     &g_append_test_values[1]);
    if (src)
    {
        result = d_assert_standalone(
            d_ptr_vector_prepend_vector(NULL, src) == D_FAILURE,
            "prepend_vector_null_dest",
            "NULL destination should return D_FAILURE",
            _counter) && result;

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
        result = d_assert_standalone(
            d_ptr_vector_prepend_vector(dest, src) == D_SUCCESS &&
            dest->count == 4 &&
            dest->elements[0] == &g_append_test_values[0] &&
            dest->elements[2] == &g_append_test_values[2],
            "prepend_vector_success",
            "Source prepended, destination shifted",
            _counter) && result;

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


bool
d_tests_sa_ptr_vector_append_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Append/Extend Functions\n");
    printf("  ----------------------------------\n");

    result = d_tests_sa_ptr_vector_append(_counter) && result;
    result = d_tests_sa_ptr_vector_append_vector(_counter) && result;
    result = d_tests_sa_ptr_vector_prepend(_counter) && result;
    result = d_tests_sa_ptr_vector_prepend_vector(_counter) && result;

    return result;
}
