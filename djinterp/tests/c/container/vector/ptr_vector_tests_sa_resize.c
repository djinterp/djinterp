/******************************************************************************
* djinterp [test]                                     ptr_vector_tests_sa_resize.c
*
*   Unit tests for ptr_vector resize functions.
*
* path:      \tests\container\vector\ptr_vector_tests_sa_resize.c
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.01.29
******************************************************************************/

#include "./ptr_vector_tests_sa.h"

static int g_resize_test_values[] = {11, 22, 33, 44, 55};


bool
d_tests_sa_ptr_vector_resize
(
    struct d_test_counter* _counter
)
{
    bool                 result;
    struct d_ptr_vector* vec;
    size_t               i;
    bool                 all_null;

    result = true;

    /* test 1: NULL vector should fail */
    result = d_assert_standalone(
        d_ptr_vector_resize(NULL, 10) == D_FAILURE,
        "resize_null_vector",
        "NULL vector should return D_FAILURE",
        _counter) && result;

    /* test 2: resize to larger (new elements NULL) */
    vec = d_ptr_vector_new_from_args(2,
                                     &g_resize_test_values[0],
                                     &g_resize_test_values[1]);
    if (vec)
    {
        result = d_assert_standalone(
            d_ptr_vector_resize(vec, 5) == D_SUCCESS &&
            vec->count == 5 &&
            vec->elements[0] == &g_resize_test_values[0] &&
            vec->elements[1] == &g_resize_test_values[1],
            "resize_larger",
            "Resize larger should preserve original elements",
            _counter) && result;

        all_null = true;
        for (i = 2; i < vec->count; i++)
        {
            if (vec->elements[i] != NULL)
            {
                all_null = false;
                break;
            }
        }

        result = d_assert_standalone(
            all_null,
            "resize_larger_null_fill",
            "New elements should be NULL",
            _counter) && result;

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
        result = d_assert_standalone(
            d_ptr_vector_resize(vec, 2) == D_SUCCESS &&
            vec->count == 2 &&
            vec->elements[0] == &g_resize_test_values[0] &&
            vec->elements[1] == &g_resize_test_values[1],
            "resize_smaller",
            "Resize smaller should truncate",
            _counter) && result;

        d_ptr_vector_free(vec);
    }

    /* test 4: resize to zero */
    vec = d_ptr_vector_new_from_args(3,
                                     &g_resize_test_values[0],
                                     &g_resize_test_values[1],
                                     &g_resize_test_values[2]);
    if (vec)
    {
        result = d_assert_standalone(
            d_ptr_vector_resize(vec, 0) == D_SUCCESS &&
            vec->count == 0,
            "resize_to_zero",
            "Resize to zero should succeed",
            _counter) && result;

        d_ptr_vector_free(vec);
    }

    return result;
}


bool
d_tests_sa_ptr_vector_resize_fill
(
    struct d_test_counter* _counter
)
{
    bool                 result;
    struct d_ptr_vector* vec;
    void*                fill_ptr;
    size_t               i;
    bool                 all_match;

    result   = true;
    fill_ptr = &g_resize_test_values[4];

    /* test 1: NULL vector should fail */
    result = d_assert_standalone(
        d_ptr_vector_resize_fill(NULL, 10, fill_ptr) == D_FAILURE,
        "resize_fill_null_vector",
        "NULL vector should return D_FAILURE",
        _counter) && result;

    /* test 2: resize larger with fill value */
    vec = d_ptr_vector_new_from_args(2,
                                     &g_resize_test_values[0],
                                     &g_resize_test_values[1]);
    if (vec)
    {
        result = d_assert_standalone(
            d_ptr_vector_resize_fill(vec, 5, fill_ptr) == D_SUCCESS &&
            vec->count == 5 &&
            vec->elements[0] == &g_resize_test_values[0] &&
            vec->elements[1] == &g_resize_test_values[1],
            "resize_fill_larger",
            "Resize fill larger should preserve original",
            _counter) && result;

        all_match = true;
        for (i = 2; i < vec->count; i++)
        {
            if (vec->elements[i] != fill_ptr)
            {
                all_match = false;
                break;
            }
        }

        result = d_assert_standalone(
            all_match,
            "resize_fill_larger_fill_value",
            "New elements should match fill value",
            _counter) && result;

        d_ptr_vector_free(vec);
    }

    /* test 3: resize with NULL fill value */
    vec = d_ptr_vector_new_from_args(2,
                                     &g_resize_test_values[0],
                                     &g_resize_test_values[1]);
    if (vec)
    {
        result = d_assert_standalone(
            d_ptr_vector_resize_fill(vec, 5, NULL) == D_SUCCESS &&
            vec->count == 5 &&
            vec->elements[2] == NULL &&
            vec->elements[3] == NULL &&
            vec->elements[4] == NULL,
            "resize_fill_null_fill",
            "Resize with NULL fill should work",
            _counter) && result;

        d_ptr_vector_free(vec);
    }

    return result;
}


bool
d_tests_sa_ptr_vector_resize_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Resize Functions\n");
    printf("  ---------------------------\n");

    result = d_tests_sa_ptr_vector_resize(_counter) && result;
    result = d_tests_sa_ptr_vector_resize_fill(_counter) && result;

    return result;
}
