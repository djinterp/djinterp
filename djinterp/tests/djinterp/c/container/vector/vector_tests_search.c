/*******************************************************************************
* djinterp [container]                                     vector_tests_search.c
*
*
* path:      /tests/djinterp/c/container/vector/vector_tests_search.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.02.25
*                                                            revised: 2026.11.09
*******************************************************************************/
// djinterp
#include "./vector_tests.h"
#include "../../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's C face


// helper comparator for int values
static
int int_comparator(
    const void* _a,
    const void* _b
)
{
    int a = *(const int*)_a;
    int b = *(const int*)_b;

    return (a > b) - (a < b);
}


/*
d_tests_vector_find
  Tests the d_vector_find function.
  Tests the following:
  - NULL vector returns -1
  - NULL comparator returns -1
  - value not found returns -1
  - value found returns correct index
*/
bool d_tests_vector_find(
    struct dj_test_suite* _suite
)
{
    bool             result;
    struct dj_vector* vec;
    int              search_val;
    ssize_t          idx;

    d_test_suite_test(_suite, "vector_find");

    result = true;

    // test 1: NULL vector returns -1
    search_val = 10;
    idx        = d_vector_find(NULL, &search_val, int_comparator);
    result     = D_TEST_CHECK_MSG(_suite,
                                  "find_null_vector",
                                  idx == -1,
                                  "NULL vector should return -1") && result;

    // test 2: NULL comparator returns -1
    vec = d_vector_new_from_args(sizeof(int), 3, 10, 20, 30);

    if (vec)
    {
        idx    = d_vector_find(vec, &search_val, NULL);
        result = D_TEST_CHECK_MSG(_suite,
                                  "find_null_comparator",
                                  idx == -1,
                                  "NULL comparator should return -1") && result;

        // test 3: value not found returns -1
        search_val = 999;
        idx        = d_vector_find(vec, &search_val, int_comparator);
        result     = D_TEST_CHECK_MSG(_suite,
                                      "find_not_found",
                                      idx == -1,
                                      "Value not found should return -1") && result;

        // test 4: value found returns correct index
        search_val = 20;
        idx        = d_vector_find(vec, &search_val, int_comparator);
        result     = D_TEST_CHECK_MSG(_suite,
                                      "find_found",
                                      idx == 1,
                                      "Value 20 should be at index 1") && result;

        d_vector_free(vec);
    }

    return result;
}


/*
d_tests_vector_find_last
  Tests the d_vector_find_last function.
  Tests the following:
  - NULL vector returns -1
  - NULL comparator returns -1
  - returns last occurrence index
*/
bool d_tests_vector_find_last(
    struct dj_test_suite* _suite
)
{
    bool             result;
    struct dj_vector* vec;
    int              search_val;
    ssize_t          idx;

    d_test_suite_test(_suite, "vector_find_last");

    result = true;

    // test 1: NULL vector returns -1
    search_val = 10;
    idx        = d_vector_find_last(NULL, &search_val, int_comparator);
    result     = D_TEST_CHECK_MSG(_suite,
                                  "find_last_null_vector",
                                  idx == -1,
                                  "NULL vector should return -1") && result;

    // test 2: returns last occurrence
    vec = d_vector_new_from_args(sizeof(int), 5, 10, 20, 10, 30, 10);

    if (vec)
    {
        search_val = 10;
        idx        = d_vector_find_last(vec, &search_val, int_comparator);
        result     = D_TEST_CHECK_MSG(_suite,
                                      "find_last_found",
                                      idx == 4,
                                      "Last occurrence of 10 should be at index 4") && result;

        // test 3: not found
        search_val = 999;
        idx        = d_vector_find_last(vec, &search_val, int_comparator);
        result     = D_TEST_CHECK_MSG(_suite,
                                      "find_last_not_found",
                                      idx == -1,
                                      "Value not found should return -1") && result;

        d_vector_free(vec);
    }

    return result;
}


/*
d_tests_vector_contains
  Tests the d_vector_contains function.
  Tests the following:
  - NULL vector returns false
  - value found returns true
  - value not found returns false
*/
bool d_tests_vector_contains(
    struct dj_test_suite* _suite
)
{
    bool             result;
    struct dj_vector* vec;
    int              search_val;

    d_test_suite_test(_suite, "vector_contains");

    result = true;

    // test 1: NULL vector returns false
    search_val = 10;
    result     = D_TEST_CHECK_MSG(_suite,
                                  "contains_null_vector",
                                  d_vector_contains(NULL, &search_val, int_comparator) == false,
                                  "NULL vector should return false") && result;

    // test 2: value found returns true
    vec = d_vector_new_from_args(sizeof(int), 3, 10, 20, 30);

    if (vec)
    {
        search_val = 20;
        result     = D_TEST_CHECK_MSG(_suite,
                                      "contains_found",
                                      d_vector_contains(vec, &search_val, int_comparator) == true,
                                      "Value 20 should be found") && result;

        // test 3: value not found returns false
        search_val = 999;
        result     = D_TEST_CHECK_MSG(_suite,
                                      "contains_not_found",
                                      d_vector_contains(vec, &search_val, int_comparator) == false,
                                      "Value 999 should not be found") && result;

        d_vector_free(vec);
    }

    return result;
}


/*
d_tests_vector_count_value
  Tests the d_vector_count_value function.
  Tests the following:
  - NULL vector returns 0
  - NULL comparator returns 0
  - correct count of occurrences
*/
bool d_tests_vector_count_value(
    struct dj_test_suite* _suite
)
{
    bool             result;
    struct dj_vector* vec;
    int              search_val;
    size_t           count;

    d_test_suite_test(_suite, "vector_count_value");

    result = true;

    // test 1: NULL vector returns 0
    search_val = 10;
    count      = d_vector_count_value(NULL, &search_val, int_comparator);
    result     = D_TEST_CHECK_MSG(_suite,
                                  "count_value_null_vector",
                                  count == 0,
                                  "NULL vector should return 0") && result;

    // test 2: NULL comparator returns 0
    vec = d_vector_new_from_args(sizeof(int), 5, 10, 20, 10, 30, 10);

    if (vec)
    {
        count  = d_vector_count_value(vec, &search_val, NULL);
        result = D_TEST_CHECK_MSG(_suite,
                                  "count_value_null_comparator",
                                  count == 0,
                                  "NULL comparator should return 0") && result;

        // test 3: correct count of occurrences
        search_val = 10;
        count      = d_vector_count_value(vec, &search_val, int_comparator);
        result     = D_TEST_CHECK_MSG(_suite,
                                      "count_value_found",
                                      count == 3,
                                      "Value 10 should occur 3 times") && result;

        // test 4: value not present
        search_val = 999;
        count      = d_vector_count_value(vec, &search_val, int_comparator);
        result     = D_TEST_CHECK_MSG(_suite,
                                      "count_value_not_found",
                                      count == 0,
                                      "Value 999 should occur 0 times") && result;

        d_vector_free(vec);
    }

    return result;
}


/*
d_tests_vector_search_all
  Aggregation function that runs all search tests.
*/
bool d_tests_vector_search_all(
    struct dj_test_suite* _suite
)
{
    bool result;

    d_test_suite_block(_suite, "vector_search_all");

    result = true;

    result = d_tests_vector_find(_suite) && result;
    result = d_tests_vector_find_last(_suite) && result;
    result = d_tests_vector_contains(_suite) && result;
    result = d_tests_vector_count_value(_suite) && result;

    return result;
}
