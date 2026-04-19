/******************************************************************************
* djinterp [test]                                   ptr_vector_tests_sa_capacity.c
*
*   Unit tests for ptr_vector capacity management functions.
*
* path:      \tests\container\vector\ptr_vector_tests_sa_capacity.c
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.01.29
******************************************************************************/

#include "./ptr_vector_tests_sa.h"

static int g_cap_test_values[] = {10, 20, 30, 40, 50, 60, 70, 80, 90, 100};


bool
d_tests_sa_ptr_vector_reserve
(
    struct d_test_counter* _counter
)
{
    bool                 result;
    struct d_ptr_vector* vec;
    size_t               original_capacity;

    result = true;

    /* test 1: NULL vector should fail */
    result = d_assert_standalone(
        d_ptr_vector_reserve(NULL, 10) == D_FAILURE,
        "reserve_null_vector",
        "NULL vector should return D_FAILURE",
        _counter) && result;

    /* test 2: reserve smaller than current capacity (no-op) */
    vec = d_ptr_vector_new(20);
    if (vec)
    {
        original_capacity = vec->capacity;

        result = d_assert_standalone(
            d_ptr_vector_reserve(vec, 5) == D_SUCCESS &&
            vec->capacity == original_capacity,
            "reserve_smaller",
            "Reserve smaller should succeed, capacity unchanged",
            _counter) && result;

        d_ptr_vector_free(vec);
    }

    /* test 3: reserve larger than current capacity */
    vec = d_ptr_vector_new(5);
    if (vec)
    {
        d_ptr_vector_push_back(vec, &g_cap_test_values[0]);
        d_ptr_vector_push_back(vec, &g_cap_test_values[1]);
        d_ptr_vector_push_back(vec, &g_cap_test_values[2]);

        result = d_assert_standalone(
            d_ptr_vector_reserve(vec, 50) == D_SUCCESS &&
            vec->capacity >= 50 &&
            vec->count == 3 &&
            vec->elements[0] == &g_cap_test_values[0],
            "reserve_larger",
            "Reserve larger should grow capacity, preserve data",
            _counter) && result;

        d_ptr_vector_free(vec);
    }

    /* test 4: reserve on empty vector with zero capacity */
    vec = d_ptr_vector_new(0);
    if (vec)
    {
        result = d_assert_standalone(
            d_ptr_vector_reserve(vec, 100) == D_SUCCESS &&
            vec->capacity >= 100,
            "reserve_from_zero",
            "Reserve on zero-capacity vector should succeed",
            _counter) && result;

        d_ptr_vector_free(vec);
    }

    return result;
}


bool
d_tests_sa_ptr_vector_shrink_to_fit
(
    struct d_test_counter* _counter
)
{
    bool                 result;
    struct d_ptr_vector* vec;

    result = true;

    /* test 1: NULL vector should fail */
    result = d_assert_standalone(
        d_ptr_vector_shrink_to_fit(NULL) == D_FAILURE,
        "shrink_null_vector",
        "NULL vector should return D_FAILURE",
        _counter) && result;

    /* test 2: shrink when count < capacity */
    vec = d_ptr_vector_new(100);
    if (vec)
    {
        d_ptr_vector_push_back(vec, &g_cap_test_values[0]);
        d_ptr_vector_push_back(vec, &g_cap_test_values[1]);
        d_ptr_vector_push_back(vec, &g_cap_test_values[2]);

        result = d_assert_standalone(
            d_ptr_vector_shrink_to_fit(vec) == D_SUCCESS &&
            vec->capacity == 3 &&
            vec->count == 3 &&
            vec->elements[0] == &g_cap_test_values[0],
            "shrink_success",
            "Shrink should reduce capacity to count, preserve data",
            _counter) && result;

        d_ptr_vector_free(vec);
    }

    /* test 3: shrink empty vector */
    vec = d_ptr_vector_new(50);
    if (vec)
    {
        result = d_assert_standalone(
            d_ptr_vector_shrink_to_fit(vec) == D_SUCCESS &&
            vec->capacity == 0 && vec->count == 0,
            "shrink_empty",
            "Shrink empty vector should have capacity=0",
            _counter) && result;

        d_ptr_vector_free(vec);
    }

    return result;
}


bool
d_tests_sa_ptr_vector_ensure_capacity
(
    struct d_test_counter* _counter
)
{
    bool                 result;
    struct d_ptr_vector* vec;
    size_t               original_capacity;

    result = true;

    /* test 1: NULL vector should fail */
    result = d_assert_standalone(
        d_ptr_vector_ensure_capacity(NULL, 10) == D_FAILURE,
        "ensure_null_vector",
        "NULL vector should return D_FAILURE",
        _counter) && result;

    /* test 2: ensure when capacity already sufficient */
    vec = d_ptr_vector_new(50);
    if (vec)
    {
        original_capacity = vec->capacity;

        result = d_assert_standalone(
            d_ptr_vector_ensure_capacity(vec, 30) == D_SUCCESS &&
            vec->capacity == original_capacity,
            "ensure_sufficient",
            "Ensure with sufficient capacity should not change it",
            _counter) && result;

        d_ptr_vector_free(vec);
    }

    /* test 3: ensure when capacity needs to grow */
    vec = d_ptr_vector_new(5);
    if (vec)
    {
        d_ptr_vector_push_back(vec, &g_cap_test_values[0]);
        d_ptr_vector_push_back(vec, &g_cap_test_values[1]);

        result = d_assert_standalone(
            d_ptr_vector_ensure_capacity(vec, 100) == D_SUCCESS &&
            vec->capacity >= 100 &&
            vec->count == 2 &&
            vec->elements[0] == &g_cap_test_values[0],
            "ensure_grow",
            "Ensure should grow capacity, preserve data",
            _counter) && result;

        d_ptr_vector_free(vec);
    }

    return result;
}


bool
d_tests_sa_ptr_vector_available
(
    struct d_test_counter* _counter
)
{
    bool                 result;
    struct d_ptr_vector* vec;

    result = true;

    /* test 1: NULL vector should return 0 */
    result = d_assert_standalone(
        d_ptr_vector_available(NULL) == 0,
        "available_null_vector",
        "NULL vector should return 0 available",
        _counter) && result;

    /* test 2: newly created vector */
    vec = d_ptr_vector_new(10);
    if (vec)
    {
        result = d_assert_standalone(
            d_ptr_vector_available(vec) == 10,
            "available_new_vector",
            "New vector should have full capacity available",
            _counter) && result;

        d_ptr_vector_free(vec);
    }

    /* test 3: available after adding elements */
    vec = d_ptr_vector_new(10);
    if (vec)
    {
        d_ptr_vector_push_back(vec, &g_cap_test_values[0]);
        d_ptr_vector_push_back(vec, &g_cap_test_values[1]);
        d_ptr_vector_push_back(vec, &g_cap_test_values[2]);

        result = d_assert_standalone(
            d_ptr_vector_available(vec) == 7,
            "available_after_push",
            "Available should be capacity - count (7)",
            _counter) && result;

        d_ptr_vector_free(vec);
    }

    /* test 4: available when full */
    vec = d_ptr_vector_new(3);
    if (vec)
    {
        d_ptr_vector_push_back(vec, &g_cap_test_values[0]);
        d_ptr_vector_push_back(vec, &g_cap_test_values[1]);
        d_ptr_vector_push_back(vec, &g_cap_test_values[2]);

        result = d_assert_standalone(
            d_ptr_vector_available(vec) == 0,
            "available_when_full",
            "Full vector should have 0 available",
            _counter) && result;

        d_ptr_vector_free(vec);
    }

    return result;
}


bool
d_tests_sa_ptr_vector_capacity_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Capacity Management Functions\n");
    printf("  ----------------------------------------\n");

    result = d_tests_sa_ptr_vector_reserve(_counter) && result;
    result = d_tests_sa_ptr_vector_shrink_to_fit(_counter) && result;
    result = d_tests_sa_ptr_vector_ensure_capacity(_counter) && result;
    result = d_tests_sa_ptr_vector_available(_counter) && result;

    return result;
}
