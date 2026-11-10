/*******************************************************************************
* djinterp [test]                                    ptr_vector_tests_capacity.c
*
*   Unit tests for ptr_vector capacity management functions.
*
*
* path:      /tests/djinterp/c/container/vector/ptr_vector_tests_capacity.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.01.29
*                                                            revised: 2026.11.09
*******************************************************************************/

// djinterp
#include "./ptr_vector_tests.h"
#include "../../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's C face

static int g_cap_test_values[] = {10, 20, 30, 40, 50, 60, 70, 80, 90, 100};


bool d_tests_ptr_vector_reserve(
    struct dj_test_suite* _suite
)
{
    bool                 result;
    struct dj_ptr_vector* vec;
    size_t               original_capacity;

    d_test_suite_test(_suite, "ptr_vector_reserve");

    result = true;

    /* test 1: NULL vector should fail */
    result = D_TEST_CHECK_MSG(_suite,
                              "reserve_null_vector",
                              d_ptr_vector_reserve(NULL, 10) == D_FAILURE,
                              "NULL vector should return D_FAILURE") && result;

    /* test 2: reserve smaller than current capacity (no-op) */
    vec = d_ptr_vector_new(20);
    if (vec)
    {
        original_capacity = vec->capacity;

        result = D_TEST_CHECK_MSG(_suite,
                                  "reserve_smaller",
                                  d_ptr_vector_reserve(vec, 5) == D_SUCCESS && vec->capacity == original_capacity,
                                  "Reserve smaller should succeed, capacity unchanged") && result;

        d_ptr_vector_free(vec);
    }

    /* test 3: reserve larger than current capacity */
    vec = d_ptr_vector_new(5);
    if (vec)
    {
        d_ptr_vector_push_back(vec, &g_cap_test_values[0]);
        d_ptr_vector_push_back(vec, &g_cap_test_values[1]);
        d_ptr_vector_push_back(vec, &g_cap_test_values[2]);

        result = D_TEST_CHECK_MSG(_suite,
                                  "reserve_larger",
                                  d_ptr_vector_reserve(vec, 50) == D_SUCCESS && vec->capacity >= 50 && vec->count == 3 && vec->elements[0] == &g_cap_test_values[0],
                                  "Reserve larger should grow capacity, preserve data") && result;

        d_ptr_vector_free(vec);
    }

    /* test 4: reserve on empty vector with zero capacity */
    vec = d_ptr_vector_new(0);
    if (vec)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "reserve_from_zero",
                                  d_ptr_vector_reserve(vec, 100) == D_SUCCESS && vec->capacity >= 100,
                                  "Reserve on zero-capacity vector should succeed") && result;

        d_ptr_vector_free(vec);
    }

    return result;
}


bool d_tests_ptr_vector_shrink_to_fit(
    struct dj_test_suite* _suite
)
{
    bool                 result;
    struct dj_ptr_vector* vec;

    d_test_suite_test(_suite, "ptr_vector_shrink_to_fit");

    result = true;

    /* test 1: NULL vector should fail */
    result = D_TEST_CHECK_MSG(_suite,
                              "shrink_null_vector",
                              d_ptr_vector_shrink_to_fit(NULL) == D_FAILURE,
                              "NULL vector should return D_FAILURE") && result;

    /* test 2: shrink when count < capacity */
    vec = d_ptr_vector_new(100);
    if (vec)
    {
        d_ptr_vector_push_back(vec, &g_cap_test_values[0]);
        d_ptr_vector_push_back(vec, &g_cap_test_values[1]);
        d_ptr_vector_push_back(vec, &g_cap_test_values[2]);

        result = D_TEST_CHECK_MSG(_suite,
                                  "shrink_success",
                                  d_ptr_vector_shrink_to_fit(vec) == D_SUCCESS && vec->capacity == 3 && vec->count == 3 && vec->elements[0] == &g_cap_test_values[0],
                                  "Shrink should reduce capacity to count, preserve data") && result;

        d_ptr_vector_free(vec);
    }

    /* test 3: shrink empty vector */
    vec = d_ptr_vector_new(50);
    if (vec)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "shrink_empty",
                                  d_ptr_vector_shrink_to_fit(vec) == D_SUCCESS && vec->capacity == 0 && vec->count == 0,
                                  "Shrink empty vector should have capacity=0") && result;

        d_ptr_vector_free(vec);
    }

    return result;
}


bool d_tests_ptr_vector_ensure_capacity(
    struct dj_test_suite* _suite
)
{
    bool                 result;
    struct dj_ptr_vector* vec;
    size_t               original_capacity;

    d_test_suite_test(_suite, "ptr_vector_ensure_capacity");

    result = true;

    /* test 1: NULL vector should fail */
    result = D_TEST_CHECK_MSG(_suite,
                              "ensure_null_vector",
                              d_ptr_vector_ensure_capacity(NULL, 10) == D_FAILURE,
                              "NULL vector should return D_FAILURE") && result;

    /* test 2: ensure when capacity already sufficient */
    vec = d_ptr_vector_new(50);
    if (vec)
    {
        original_capacity = vec->capacity;

        result = D_TEST_CHECK_MSG(_suite,
                                  "ensure_sufficient",
                                  d_ptr_vector_ensure_capacity(vec, 30) == D_SUCCESS && vec->capacity == original_capacity,
                                  "Ensure with sufficient capacity should not change it") && result;

        d_ptr_vector_free(vec);
    }

    /* test 3: ensure when capacity needs to grow */
    vec = d_ptr_vector_new(5);
    if (vec)
    {
        d_ptr_vector_push_back(vec, &g_cap_test_values[0]);
        d_ptr_vector_push_back(vec, &g_cap_test_values[1]);

        result = D_TEST_CHECK_MSG(_suite,
                                  "ensure_grow",
                                  d_ptr_vector_ensure_capacity(vec, 100) == D_SUCCESS && vec->capacity >= 100 && vec->count == 2 && vec->elements[0] == &g_cap_test_values[0],
                                  "Ensure should grow capacity, preserve data") && result;

        d_ptr_vector_free(vec);
    }

    return result;
}


bool d_tests_ptr_vector_available(
    struct dj_test_suite* _suite
)
{
    bool                 result;
    struct dj_ptr_vector* vec;

    d_test_suite_test(_suite, "ptr_vector_available");

    result = true;

    /* test 1: NULL vector should return 0 */
    result = D_TEST_CHECK_MSG(_suite,
                              "available_null_vector",
                              d_ptr_vector_available(NULL) == 0,
                              "NULL vector should return 0 available") && result;

    /* test 2: newly created vector */
    vec = d_ptr_vector_new(10);
    if (vec)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "available_new_vector",
                                  d_ptr_vector_available(vec) == 10,
                                  "New vector should have full capacity available") && result;

        d_ptr_vector_free(vec);
    }

    /* test 3: available after adding elements */
    vec = d_ptr_vector_new(10);
    if (vec)
    {
        d_ptr_vector_push_back(vec, &g_cap_test_values[0]);
        d_ptr_vector_push_back(vec, &g_cap_test_values[1]);
        d_ptr_vector_push_back(vec, &g_cap_test_values[2]);

        result = D_TEST_CHECK_MSG(_suite,
                                  "available_after_push",
                                  d_ptr_vector_available(vec) == 7,
                                  "Available should be capacity - count (7)") && result;

        d_ptr_vector_free(vec);
    }

    /* test 4: available when full */
    vec = d_ptr_vector_new(3);
    if (vec)
    {
        d_ptr_vector_push_back(vec, &g_cap_test_values[0]);
        d_ptr_vector_push_back(vec, &g_cap_test_values[1]);
        d_ptr_vector_push_back(vec, &g_cap_test_values[2]);

        result = D_TEST_CHECK_MSG(_suite,
                                  "available_when_full",
                                  d_ptr_vector_available(vec) == 0,
                                  "Full vector should have 0 available") && result;

        d_ptr_vector_free(vec);
    }

    return result;
}


bool d_tests_ptr_vector_capacity_all(
    struct dj_test_suite* _suite
)
{
    bool result;

    d_test_suite_block(_suite, "ptr_vector_capacity_all");

    result = true;

    result = d_tests_ptr_vector_reserve(_suite) && result;
    result = d_tests_ptr_vector_shrink_to_fit(_suite) && result;
    result = d_tests_ptr_vector_ensure_capacity(_suite) && result;
    result = d_tests_ptr_vector_available(_suite) && result;

    return result;
}
