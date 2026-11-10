/*******************************************************************************
* djinterp [test]                                      ptr_vector_tests_search.c
*
*   Unit tests for ptr_vector search functions.
*
*
* path:      /tests/djinterp/c/container/vector/ptr_vector_tests_search.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.01.29
*                                                            revised: 2026.11.09
*******************************************************************************/

// djinterp
#include "./ptr_vector_tests.h"
#include "../../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's C face

static int g_search_test_values[] = {10, 20, 30, 20, 50, 20};

static
int int_ptr_comparator(
    const void* _a,
    const void* _b
)
{
    int val_a, val_b;
    if (!_a && !_b) return 0;
    if (!_a) return -1;
    if (!_b) return 1;
    val_a = *(const int*)_a;
    val_b = *(const int*)_b;
    return (val_a > val_b) - (val_a < val_b);
}


bool d_tests_ptr_vector_find(
    struct dj_test_suite* _suite
)
{
    bool                 result;
    struct dj_ptr_vector* vec;
    int                  search_val;

    d_test_suite_test(_suite, "ptr_vector_find");

    result = true;

    /* test 1: NULL vector should return -1 */
    search_val = 20;
    result = D_TEST_CHECK_MSG(_suite,
                              "find_null_vector",
                              d_ptr_vector_find(NULL, &search_val, int_ptr_comparator) == -1,
                              "NULL vector should return -1") && result;

    /* test 2: find existing value */
    vec = d_ptr_vector_new_from_args(6,
        &g_search_test_values[0], &g_search_test_values[1], &g_search_test_values[2],
        &g_search_test_values[3], &g_search_test_values[4], &g_search_test_values[5]);
    if (vec)
    {
        search_val = 20;
        result = D_TEST_CHECK_MSG(_suite,
                                  "find_first_occurrence",
                                  d_ptr_vector_find(vec, &search_val, int_ptr_comparator) == 1,
                                  "Find 20 should return index 1") && result;

        search_val = 999;
        result = D_TEST_CHECK_MSG(_suite,
                                  "find_not_found",
                                  d_ptr_vector_find(vec, &search_val, int_ptr_comparator) == -1,
                                  "Find non-existent should return -1") && result;

        d_ptr_vector_free(vec);
    }

    return result;
}


bool d_tests_ptr_vector_find_last(
    struct dj_test_suite* _suite
)
{
    bool                 result;
    struct dj_ptr_vector* vec;
    int                  search_val;

    d_test_suite_test(_suite, "ptr_vector_find_last");

    result = true;

    /* test 1: NULL vector should return -1 */
    search_val = 20;
    result = D_TEST_CHECK_MSG(_suite,
                              "find_last_null_vector",
                              d_ptr_vector_find_last(NULL, &search_val, int_ptr_comparator) == -1,
                              "NULL vector should return -1") && result;

    /* test 2: find last occurrence */
    vec = d_ptr_vector_new_from_args(6,
        &g_search_test_values[0], &g_search_test_values[1], &g_search_test_values[2],
        &g_search_test_values[3], &g_search_test_values[4], &g_search_test_values[5]);
    if (vec)
    {
        search_val = 20;
        result = D_TEST_CHECK_MSG(_suite,
                                  "find_last_occurrence",
                                  d_ptr_vector_find_last(vec, &search_val, int_ptr_comparator) == 5,
                                  "Find last 20 should return index 5") && result;

        search_val = 10;
        result = D_TEST_CHECK_MSG(_suite,
                                  "find_last_single",
                                  d_ptr_vector_find_last(vec, &search_val, int_ptr_comparator) == 0,
                                  "Find last 10 should return index 0") && result;

        d_ptr_vector_free(vec);
    }

    return result;
}


bool d_tests_ptr_vector_find_ptr(
    struct dj_test_suite* _suite
)
{
    bool                 result;
    struct dj_ptr_vector* vec;
    int                  other_val;

    d_test_suite_test(_suite, "ptr_vector_find_ptr");

    result = true;
    other_val = 20;

    /* test 1: NULL vector should return -1 */
    result = D_TEST_CHECK_MSG(_suite,
                              "find_ptr_null_vector",
                              d_ptr_vector_find_ptr(NULL, &g_search_test_values[0]) == -1,
                              "NULL vector should return -1") && result;

    /* test 2: find by pointer identity */
    vec = d_ptr_vector_new_from_args(3,
        &g_search_test_values[0], &g_search_test_values[1], &g_search_test_values[2]);
    if (vec)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "find_ptr_found",
                                  d_ptr_vector_find_ptr(vec, &g_search_test_values[1]) == 1,
                                  "Find by ptr should return correct index") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "find_ptr_not_found",
                                  d_ptr_vector_find_ptr(vec, &other_val) == -1,
                                  "Different ptr with same value should return -1") && result;

        d_ptr_vector_free(vec);
    }

    return result;
}


bool d_tests_ptr_vector_contains(
    struct dj_test_suite* _suite
)
{
    bool                 result;
    struct dj_ptr_vector* vec;
    int                  search_val;

    d_test_suite_test(_suite, "ptr_vector_contains");

    result = true;

    /* test 1: NULL vector should return false */
    search_val = 20;
    result = D_TEST_CHECK_MSG(_suite,
                              "contains_null_vector",
                              d_ptr_vector_contains(NULL, &search_val, int_ptr_comparator) == false,
                              "NULL vector should return false") && result;

    /* test 2: contains tests */
    vec = d_ptr_vector_new_from_args(3,
        &g_search_test_values[0], &g_search_test_values[1], &g_search_test_values[2]);
    if (vec)
    {
        search_val = 20;
        result = D_TEST_CHECK_MSG(_suite,
                                  "contains_found",
                                  d_ptr_vector_contains(vec, &search_val, int_ptr_comparator) == true,
                                  "Contains existing should return true") && result;

        search_val = 999;
        result = D_TEST_CHECK_MSG(_suite,
                                  "contains_not_found",
                                  d_ptr_vector_contains(vec, &search_val, int_ptr_comparator) == false,
                                  "Contains non-existent should return false") && result;

        d_ptr_vector_free(vec);
    }

    return result;
}


bool d_tests_ptr_vector_contains_ptr(
    struct dj_test_suite* _suite
)
{
    bool                 result;
    struct dj_ptr_vector* vec;
    int                  other_val;

    d_test_suite_test(_suite, "ptr_vector_contains_ptr");

    result = true;
    other_val = 10;

    /* test 1: contains by pointer identity */
    vec = d_ptr_vector_new_from_args(3,
        &g_search_test_values[0], &g_search_test_values[1], &g_search_test_values[2]);
    if (vec)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "contains_ptr_found",
                                  d_ptr_vector_contains_ptr(vec, &g_search_test_values[0]) == true,
                                  "Contains ptr should return true") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "contains_ptr_not_found",
                                  d_ptr_vector_contains_ptr(vec, &other_val) == false,
                                  "Different ptr should return false") && result;

        d_ptr_vector_free(vec);
    }

    return result;
}


bool d_tests_ptr_vector_count_value(
    struct dj_test_suite* _suite
)
{
    bool                 result;
    struct dj_ptr_vector* vec;
    int                  search_val;

    d_test_suite_test(_suite, "ptr_vector_count_value");

    result = true;

    /* test 1: NULL vector should return 0 */
    search_val = 20;
    result = D_TEST_CHECK_MSG(_suite,
                              "count_value_null_vector",
                              d_ptr_vector_count_value(NULL, &search_val, int_ptr_comparator) == 0,
                              "NULL vector should return 0") && result;

    /* test 2: count occurrences */
    vec = d_ptr_vector_new_from_args(6,
        &g_search_test_values[0], &g_search_test_values[1], &g_search_test_values[2],
        &g_search_test_values[3], &g_search_test_values[4], &g_search_test_values[5]);
    if (vec)
    {
        search_val = 20;
        result = D_TEST_CHECK_MSG(_suite,
                                  "count_value_multiple",
                                  d_ptr_vector_count_value(vec, &search_val, int_ptr_comparator) == 3,
                                  "Count of 20 should be 3") && result;

        search_val = 10;
        result = D_TEST_CHECK_MSG(_suite,
                                  "count_value_single",
                                  d_ptr_vector_count_value(vec, &search_val, int_ptr_comparator) == 1,
                                  "Count of 10 should be 1") && result;

        search_val = 999;
        result = D_TEST_CHECK_MSG(_suite,
                                  "count_value_none",
                                  d_ptr_vector_count_value(vec, &search_val, int_ptr_comparator) == 0,
                                  "Count of non-existent should be 0") && result;

        d_ptr_vector_free(vec);
    }

    return result;
}


bool d_tests_ptr_vector_search_all(
    struct dj_test_suite* _suite
)
{
    bool result;

    d_test_suite_block(_suite, "ptr_vector_search_all");

    result = true;

    result = d_tests_ptr_vector_find(_suite) && result;
    result = d_tests_ptr_vector_find_last(_suite) && result;
    result = d_tests_ptr_vector_find_ptr(_suite) && result;
    result = d_tests_ptr_vector_contains(_suite) && result;
    result = d_tests_ptr_vector_contains_ptr(_suite) && result;
    result = d_tests_ptr_vector_count_value(_suite) && result;

    return result;
}
