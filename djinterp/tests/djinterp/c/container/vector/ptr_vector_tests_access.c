/*******************************************************************************
* djinterp [test]                                      ptr_vector_tests_access.c
*
*   Unit tests for ptr_vector access functions.
*
*
* path:      /tests/djinterp/c/container/vector/ptr_vector_tests_access.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.01.29
*                                                            revised: 2026.11.09
*******************************************************************************/

// djinterp
#include "./ptr_vector_tests.h"
#include "../../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's C face

static int g_access_test_values[] = {100, 200, 300, 400, 500};


bool d_tests_ptr_vector_at(
    struct dj_test_suite* _suite
)
{
    bool                 result;
    struct dj_ptr_vector* vec;

    d_test_suite_test(_suite, "ptr_vector_at");

    result = true;

    /* test 1: NULL vector should return NULL */
    result = D_TEST_CHECK_MSG(_suite,
                              "at_null_vector",
                              d_ptr_vector_at(NULL, 0) == NULL,
                              "NULL vector should return NULL") && result;

    /* test 2: empty vector should return NULL */
    vec = d_ptr_vector_new(10);
    if (vec)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "at_empty_vector",
                                  d_ptr_vector_at(vec, 0) == NULL,
                                  "Empty vector should return NULL") && result;

        d_ptr_vector_free(vec);
    }

    /* test 3: valid positive index access */
    vec = d_ptr_vector_new_from_args(3,
                                     &g_access_test_values[0],
                                     &g_access_test_values[1],
                                     &g_access_test_values[2]);
    if (vec)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "at_positive_index",
                                  d_ptr_vector_at(vec, 0) == &g_access_test_values[0] && d_ptr_vector_at(vec, 1) == &g_access_test_values[1] && d_ptr_vector_at(vec, 2) == &g_access_test_values[2],
                                  "Positive indices should return correct elements") && result;

        d_ptr_vector_free(vec);
    }

    /* test 4: valid negative index access */
    vec = d_ptr_vector_new_from_args(3,
                                     &g_access_test_values[0],
                                     &g_access_test_values[1],
                                     &g_access_test_values[2]);
    if (vec)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "at_negative_index",
                                  d_ptr_vector_at(vec, -1) == &g_access_test_values[2] && d_ptr_vector_at(vec, -2) == &g_access_test_values[1] && d_ptr_vector_at(vec, -3) == &g_access_test_values[0],
                                  "Negative indices should return correct elements") && result;

        d_ptr_vector_free(vec);
    }

    /* test 5: out-of-bounds should return NULL */
    vec = d_ptr_vector_new_from_args(3,
                                     &g_access_test_values[0],
                                     &g_access_test_values[1],
                                     &g_access_test_values[2]);
    if (vec)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "at_oob",
                                  d_ptr_vector_at(vec, 10) == NULL && d_ptr_vector_at(vec, -10) == NULL,
                                  "Out-of-bounds should return NULL") && result;

        d_ptr_vector_free(vec);
    }

    return result;
}


bool d_tests_ptr_vector_front(
    struct dj_test_suite* _suite
)
{
    bool                 result;
    struct dj_ptr_vector* vec;

    d_test_suite_test(_suite, "ptr_vector_front");

    result = true;

    /* test 1: NULL vector should return NULL */
    result = D_TEST_CHECK_MSG(_suite,
                              "front_null_vector",
                              d_ptr_vector_front(NULL) == NULL,
                              "NULL vector should return NULL") && result;

    /* test 2: empty vector should return NULL */
    vec = d_ptr_vector_new(10);
    if (vec)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "front_empty_vector",
                                  d_ptr_vector_front(vec) == NULL,
                                  "Empty vector should return NULL") && result;

        d_ptr_vector_free(vec);
    }

    /* test 3: valid front access */
    vec = d_ptr_vector_new_from_args(3,
                                     &g_access_test_values[0],
                                     &g_access_test_values[1],
                                     &g_access_test_values[2]);
    if (vec)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "front_valid",
                                  d_ptr_vector_front(vec) == &g_access_test_values[0],
                                  "Front should return first element") && result;

        d_ptr_vector_free(vec);
    }

    return result;
}


bool d_tests_ptr_vector_back(
    struct dj_test_suite* _suite
)
{
    bool                 result;
    struct dj_ptr_vector* vec;

    d_test_suite_test(_suite, "ptr_vector_back");

    result = true;

    /* test 1: NULL vector should return NULL */
    result = D_TEST_CHECK_MSG(_suite,
                              "back_null_vector",
                              d_ptr_vector_back(NULL) == NULL,
                              "NULL vector should return NULL") && result;

    /* test 2: empty vector should return NULL */
    vec = d_ptr_vector_new(10);
    if (vec)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "back_empty_vector",
                                  d_ptr_vector_back(vec) == NULL,
                                  "Empty vector should return NULL") && result;

        d_ptr_vector_free(vec);
    }

    /* test 3: valid back access */
    vec = d_ptr_vector_new_from_args(3,
                                     &g_access_test_values[0],
                                     &g_access_test_values[1],
                                     &g_access_test_values[2]);
    if (vec)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "back_valid",
                                  d_ptr_vector_back(vec) == &g_access_test_values[2],
                                  "Back should return last element") && result;

        d_ptr_vector_free(vec);
    }

    return result;
}


bool d_tests_ptr_vector_data(
    struct dj_test_suite* _suite
)
{
    bool                 result;
    struct dj_ptr_vector* vec;
    void**               data;

    d_test_suite_test(_suite, "ptr_vector_data");

    result = true;

    /* test 1: NULL vector should return NULL */
    result = D_TEST_CHECK_MSG(_suite,
                              "data_null_vector",
                              d_ptr_vector_data(NULL) == NULL,
                              "NULL vector should return NULL") && result;

    /* test 2: valid data access */
    vec = d_ptr_vector_new_from_args(3,
                                     &g_access_test_values[0],
                                     &g_access_test_values[1],
                                     &g_access_test_values[2]);
    if (vec)
    {
        data = d_ptr_vector_data(vec);
        result = D_TEST_CHECK_MSG(_suite,
                                  "data_valid",
                                  data == vec->elements && data[0] == &g_access_test_values[0],
                                  "Data should return elements array") && result;

        d_ptr_vector_free(vec);
    }

    return result;
}


bool d_tests_ptr_vector_get(
    struct dj_test_suite* _suite
)
{
    bool                 result;
    struct dj_ptr_vector* vec;

    d_test_suite_test(_suite, "ptr_vector_get");

    result = true;

    /* test 1: NULL vector should return NULL */
    result = D_TEST_CHECK_MSG(_suite,
                              "get_null_vector",
                              d_ptr_vector_get(NULL, 0) == NULL,
                              "NULL vector should return NULL") && result;

    /* test 2: valid get */
    vec = d_ptr_vector_new_from_args(3,
                                     &g_access_test_values[0],
                                     &g_access_test_values[1],
                                     &g_access_test_values[2]);
    if (vec)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "get_valid",
                                  d_ptr_vector_get(vec, 1) == &g_access_test_values[1] && d_ptr_vector_get(vec, -1) == &g_access_test_values[2],
                                  "Get should work like at") && result;

        d_ptr_vector_free(vec);
    }

    return result;
}


bool d_tests_ptr_vector_set(
    struct dj_test_suite* _suite
)
{
    bool                 result;
    struct dj_ptr_vector* vec;

    d_test_suite_test(_suite, "ptr_vector_set");

    result = true;

    /* test 1: NULL vector should fail */
    result = D_TEST_CHECK_MSG(_suite,
                              "set_null_vector",
                              d_ptr_vector_set(NULL, 0, &g_access_test_values[0]) == D_FAILURE,
                              "NULL vector should return D_FAILURE") && result;

    /* test 2: empty vector should fail */
    vec = d_ptr_vector_new(10);
    if (vec)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "set_empty_vector",
                                  d_ptr_vector_set(vec, 0, &g_access_test_values[0]) == D_FAILURE,
                                  "Empty vector should return D_FAILURE") && result;

        d_ptr_vector_free(vec);
    }

    /* test 3: valid set with positive index */
    vec = d_ptr_vector_new_from_args(3,
                                     &g_access_test_values[0],
                                     &g_access_test_values[1],
                                     &g_access_test_values[2]);
    if (vec)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "set_positive",
                                  d_ptr_vector_set(vec, 1, &g_access_test_values[4]) == D_SUCCESS && vec->elements[1] == &g_access_test_values[4],
                                  "Set with positive index should work") && result;

        d_ptr_vector_free(vec);
    }

    /* test 4: valid set with negative index */
    vec = d_ptr_vector_new_from_args(3,
                                     &g_access_test_values[0],
                                     &g_access_test_values[1],
                                     &g_access_test_values[2]);
    if (vec)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "set_negative",
                                  d_ptr_vector_set(vec, -1, &g_access_test_values[4]) == D_SUCCESS && vec->elements[2] == &g_access_test_values[4],
                                  "Set with negative index should work") && result;

        d_ptr_vector_free(vec);
    }

    /* test 5: set to NULL */
    vec = d_ptr_vector_new_from_args(3,
                                     &g_access_test_values[0],
                                     &g_access_test_values[1],
                                     &g_access_test_values[2]);
    if (vec)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "set_null_value",
                                  d_ptr_vector_set(vec, 0, NULL) == D_SUCCESS && vec->elements[0] == NULL,
                                  "Set to NULL should work") && result;

        d_ptr_vector_free(vec);
    }

    /* test 6: out-of-bounds should fail */
    vec = d_ptr_vector_new_from_args(3,
                                     &g_access_test_values[0],
                                     &g_access_test_values[1],
                                     &g_access_test_values[2]);
    if (vec)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "set_oob",
                                  d_ptr_vector_set(vec, 100, &g_access_test_values[4]) == D_FAILURE,
                                  "Out-of-bounds set should fail") && result;

        d_ptr_vector_free(vec);
    }

    return result;
}


bool d_tests_ptr_vector_access_all(
    struct dj_test_suite* _suite
)
{
    bool result;

    d_test_suite_block(_suite, "ptr_vector_access_all");

    result = true;

    result = d_tests_ptr_vector_at(_suite) && result;
    result = d_tests_ptr_vector_front(_suite) && result;
    result = d_tests_ptr_vector_back(_suite) && result;
    result = d_tests_ptr_vector_data(_suite) && result;
    result = d_tests_ptr_vector_get(_suite) && result;
    result = d_tests_ptr_vector_set(_suite) && result;

    return result;
}
