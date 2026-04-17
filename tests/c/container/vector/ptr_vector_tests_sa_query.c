/******************************************************************************
* djinterp [test]                                      ptr_vector_tests_sa_query.c
*
*   Unit tests for ptr_vector query functions.
*
* path:      \tests\container\vector\ptr_vector_tests_sa_query.c
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.01.29
******************************************************************************/

#include "./ptr_vector_tests_sa.h"

static int g_query_test_values[] = {10, 20, 30, 40, 50};


bool
d_tests_sa_ptr_vector_is_empty
(
    struct d_test_counter* _counter
)
{
    bool                 result;
    struct d_ptr_vector* vec;

    result = true;

    /* test 1: NULL vector should return true */
    result = d_assert_standalone(
        d_ptr_vector_is_empty(NULL) == true,
        "is_empty_null_vector",
        "NULL vector should be considered empty",
        _counter) && result;

    /* test 2: newly created empty vector */
    vec = d_ptr_vector_new(10);
    if (vec)
    {
        result = d_assert_standalone(
            d_ptr_vector_is_empty(vec) == true,
            "is_empty_new_vector",
            "New vector should be empty",
            _counter) && result;

        d_ptr_vector_free(vec);
    }

    /* test 3: non-empty vector */
    vec = d_ptr_vector_new_from_args(2,
                                     &g_query_test_values[0],
                                     &g_query_test_values[1]);
    if (vec)
    {
        result = d_assert_standalone(
            d_ptr_vector_is_empty(vec) == false,
            "is_empty_nonempty",
            "Non-empty vector should not be empty",
            _counter) && result;

        d_ptr_vector_free(vec);
    }

    /* test 4: after clearing */
    vec = d_ptr_vector_new_from_args(2,
                                     &g_query_test_values[0],
                                     &g_query_test_values[1]);
    if (vec)
    {
        d_ptr_vector_clear(vec);
        result = d_assert_standalone(
            d_ptr_vector_is_empty(vec) == true,
            "is_empty_after_clear",
            "Cleared vector should be empty",
            _counter) && result;

        d_ptr_vector_free(vec);
    }

    return result;
}


bool
d_tests_sa_ptr_vector_is_full
(
    struct d_test_counter* _counter
)
{
    bool                 result;
    struct d_ptr_vector* vec;

    result = true;

    /* test 1: NULL vector should return false */
    result = d_assert_standalone(
        d_ptr_vector_is_full(NULL) == false,
        "is_full_null_vector",
        "NULL vector should not be full",
        _counter) && result;

    /* test 2: empty vector with capacity should not be full */
    vec = d_ptr_vector_new(10);
    if (vec)
    {
        result = d_assert_standalone(
            d_ptr_vector_is_full(vec) == false,
            "is_full_empty",
            "Empty vector should not be full",
            _counter) && result;

        d_ptr_vector_free(vec);
    }

    /* test 3: full vector (count == capacity) */
    vec = d_ptr_vector_new(3);
    if (vec)
    {
        d_ptr_vector_push_back(vec, &g_query_test_values[0]);
        d_ptr_vector_push_back(vec, &g_query_test_values[1]);
        d_ptr_vector_push_back(vec, &g_query_test_values[2]);

        result = d_assert_standalone(
            d_ptr_vector_is_full(vec) == true,
            "is_full_full",
            "Full vector should be full",
            _counter) && result;

        d_ptr_vector_free(vec);
    }

    return result;
}


bool
d_tests_sa_ptr_vector_size
(
    struct d_test_counter* _counter
)
{
    bool                 result;
    struct d_ptr_vector* vec;

    result = true;

    /* test 1: NULL vector should return 0 */
    result = d_assert_standalone(
        d_ptr_vector_size(NULL) == 0,
        "size_null_vector",
        "NULL vector should have size 0",
        _counter) && result;

    /* test 2: empty vector */
    vec = d_ptr_vector_new(10);
    if (vec)
    {
        result = d_assert_standalone(
            d_ptr_vector_size(vec) == 0,
            "size_empty",
            "Empty vector should have size 0",
            _counter) && result;

        d_ptr_vector_free(vec);
    }

    /* test 3: after adding elements */
    vec = d_ptr_vector_new(10);
    if (vec)
    {
        d_ptr_vector_push_back(vec, &g_query_test_values[0]);
        d_ptr_vector_push_back(vec, &g_query_test_values[1]);
        d_ptr_vector_push_back(vec, &g_query_test_values[2]);
        result = d_assert_standalone(
            d_ptr_vector_size(vec) == 3,
            "size_three",
            "Size should be 3 after three pushes",
            _counter) && result;

        d_ptr_vector_free(vec);
    }

    return result;
}


bool
d_tests_sa_ptr_vector_capacity_query
(
    struct d_test_counter* _counter
)
{
    bool                 result;
    struct d_ptr_vector* vec;

    result = true;

    /* test 1: NULL vector should return 0 */
    result = d_assert_standalone(
        d_ptr_vector_capacity(NULL) == 0,
        "capacity_null_vector",
        "NULL vector should have capacity 0",
        _counter) && result;

    /* test 2: specified initial capacity */
    vec = d_ptr_vector_new(50);
    if (vec)
    {
        result = d_assert_standalone(
            d_ptr_vector_capacity(vec) == 50,
            "capacity_initial",
            "Capacity should match initial value",
            _counter) && result;

        d_ptr_vector_free(vec);
    }

    /* test 3: capacity after reserve */
    vec = d_ptr_vector_new(10);
    if (vec)
    {
        d_ptr_vector_reserve(vec, 100);
        result = d_assert_standalone(
            d_ptr_vector_capacity(vec) >= 100,
            "capacity_after_reserve",
            "Capacity should be at least reserved amount",
            _counter) && result;

        d_ptr_vector_free(vec);
    }

    return result;
}


bool
d_tests_sa_ptr_vector_query_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Query Functions\n");
    printf("  --------------------------\n");

    result = d_tests_sa_ptr_vector_is_empty(_counter) && result;
    result = d_tests_sa_ptr_vector_is_full(_counter) && result;
    result = d_tests_sa_ptr_vector_size(_counter) && result;
    result = d_tests_sa_ptr_vector_capacity_query(_counter) && result;

    return result;
}
