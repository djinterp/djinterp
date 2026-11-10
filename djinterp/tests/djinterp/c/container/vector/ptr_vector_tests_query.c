/*******************************************************************************
* djinterp [test]                                       ptr_vector_tests_query.c
*
*   Unit tests for ptr_vector query functions.
*
*
* path:      /tests/djinterp/c/container/vector/ptr_vector_tests_query.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.01.29
*                                                            revised: 2026.11.09
*******************************************************************************/

// djinterp
#include "./ptr_vector_tests.h"
#include "../../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's C face

static int g_query_test_values[] = {10, 20, 30, 40, 50};


bool d_tests_ptr_vector_is_empty(
    struct dj_test_suite* _suite
)
{
    bool                 result;
    struct dj_ptr_vector* vec;

    d_test_suite_test(_suite, "ptr_vector_is_empty");

    result = true;

    /* test 1: NULL vector should return true */
    result = D_TEST_CHECK_MSG(_suite,
                              "is_empty_null_vector",
                              d_ptr_vector_is_empty(NULL) == true,
                              "NULL vector should be considered empty") && result;

    /* test 2: newly created empty vector */
    vec = d_ptr_vector_new(10);
    if (vec)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "is_empty_new_vector",
                                  d_ptr_vector_is_empty(vec) == true,
                                  "New vector should be empty") && result;

        d_ptr_vector_free(vec);
    }

    /* test 3: non-empty vector */
    vec = d_ptr_vector_new_from_args(2,
                                     &g_query_test_values[0],
                                     &g_query_test_values[1]);
    if (vec)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "is_empty_nonempty",
                                  d_ptr_vector_is_empty(vec) == false,
                                  "Non-empty vector should not be empty") && result;

        d_ptr_vector_free(vec);
    }

    /* test 4: after clearing */
    vec = d_ptr_vector_new_from_args(2,
                                     &g_query_test_values[0],
                                     &g_query_test_values[1]);
    if (vec)
    {
        d_ptr_vector_clear(vec);
        result = D_TEST_CHECK_MSG(_suite,
                                  "is_empty_after_clear",
                                  d_ptr_vector_is_empty(vec) == true,
                                  "Cleared vector should be empty") && result;

        d_ptr_vector_free(vec);
    }

    return result;
}


bool d_tests_ptr_vector_is_full(
    struct dj_test_suite* _suite
)
{
    bool                 result;
    struct dj_ptr_vector* vec;

    d_test_suite_test(_suite, "ptr_vector_is_full");

    result = true;

    /* test 1: NULL vector should return false */
    result = D_TEST_CHECK_MSG(_suite,
                              "is_full_null_vector",
                              d_ptr_vector_is_full(NULL) == false,
                              "NULL vector should not be full") && result;

    /* test 2: empty vector with capacity should not be full */
    vec = d_ptr_vector_new(10);
    if (vec)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "is_full_empty",
                                  d_ptr_vector_is_full(vec) == false,
                                  "Empty vector should not be full") && result;

        d_ptr_vector_free(vec);
    }

    /* test 3: full vector (count == capacity) */
    vec = d_ptr_vector_new(3);
    if (vec)
    {
        d_ptr_vector_push_back(vec, &g_query_test_values[0]);
        d_ptr_vector_push_back(vec, &g_query_test_values[1]);
        d_ptr_vector_push_back(vec, &g_query_test_values[2]);

        result = D_TEST_CHECK_MSG(_suite,
                                  "is_full_full",
                                  d_ptr_vector_is_full(vec) == true,
                                  "Full vector should be full") && result;

        d_ptr_vector_free(vec);
    }

    return result;
}


bool d_tests_ptr_vector_size(
    struct dj_test_suite* _suite
)
{
    bool                 result;
    struct dj_ptr_vector* vec;

    d_test_suite_test(_suite, "ptr_vector_size");

    result = true;

    /* test 1: NULL vector should return 0 */
    result = D_TEST_CHECK_MSG(_suite,
                              "size_null_vector",
                              d_ptr_vector_size(NULL) == 0,
                              "NULL vector should have size 0") && result;

    /* test 2: empty vector */
    vec = d_ptr_vector_new(10);
    if (vec)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "size_empty",
                                  d_ptr_vector_size(vec) == 0,
                                  "Empty vector should have size 0") && result;

        d_ptr_vector_free(vec);
    }

    /* test 3: after adding elements */
    vec = d_ptr_vector_new(10);
    if (vec)
    {
        d_ptr_vector_push_back(vec, &g_query_test_values[0]);
        d_ptr_vector_push_back(vec, &g_query_test_values[1]);
        d_ptr_vector_push_back(vec, &g_query_test_values[2]);
        result = D_TEST_CHECK_MSG(_suite,
                                  "size_three",
                                  d_ptr_vector_size(vec) == 3,
                                  "Size should be 3 after three pushes") && result;

        d_ptr_vector_free(vec);
    }

    return result;
}


bool d_tests_ptr_vector_capacity_query(
    struct dj_test_suite* _suite
)
{
    bool                 result;
    struct dj_ptr_vector* vec;

    d_test_suite_test(_suite, "ptr_vector_capacity_query");

    result = true;

    /* test 1: NULL vector should return 0 */
    result = D_TEST_CHECK_MSG(_suite,
                              "capacity_null_vector",
                              d_ptr_vector_capacity(NULL) == 0,
                              "NULL vector should have capacity 0") && result;

    /* test 2: specified initial capacity */
    vec = d_ptr_vector_new(50);
    if (vec)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "capacity_initial",
                                  d_ptr_vector_capacity(vec) == 50,
                                  "Capacity should match initial value") && result;

        d_ptr_vector_free(vec);
    }

    /* test 3: capacity after reserve */
    vec = d_ptr_vector_new(10);
    if (vec)
    {
        d_ptr_vector_reserve(vec, 100);
        result = D_TEST_CHECK_MSG(_suite,
                                  "capacity_after_reserve",
                                  d_ptr_vector_capacity(vec) >= 100,
                                  "Capacity should be at least reserved amount") && result;

        d_ptr_vector_free(vec);
    }

    return result;
}


bool d_tests_ptr_vector_query_all(
    struct dj_test_suite* _suite
)
{
    bool result;

    d_test_suite_block(_suite, "ptr_vector_query_all");

    result = true;

    result = d_tests_ptr_vector_is_empty(_suite) && result;
    result = d_tests_ptr_vector_is_full(_suite) && result;
    result = d_tests_ptr_vector_size(_suite) && result;
    result = d_tests_ptr_vector_capacity_query(_suite) && result;

    return result;
}
