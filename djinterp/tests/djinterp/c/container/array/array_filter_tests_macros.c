/*******************************************************************************
* djinterp [container]                               array_filter_tests_macros.c
*
*
* path:      /tests/djinterp/c/container/array/array_filter_tests_macros.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.02.20
*                                                            revised: 2026.11.09
*******************************************************************************/
// djinterp
#include "array_filter_tests.h"
#include "../../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's C face


/******************************************************************************
 * VII. CONVENIENCE MACROS
 *****************************************************************************/

/*
d_tests_array_filter_macro_where
  Tests the D_CONTIGUOUS_FILTER_WHERE macro.
  Tests the following:
  - Expands correctly and produces right result
  - Infers element_size from type parameter
*/
bool d_tests_array_filter_macro_where(
    struct dj_test_suite* _suite
)
{
    bool                         result;
    int                          data[D_TEST_ARRAY_FILTER_DATA_SIZE];
    struct dj_contiguous_filter_result res;

    d_test_suite_test(_suite, "array_filter_macro_where");

    result = true;
    d_test_af_fill_sequential(data, D_TEST_ARRAY_FILTER_DATA_SIZE);

    // test 1: macro filters even from {0..9}
    res = D_CONTIGUOUS_FILTER_WHERE(int, data, D_TEST_ARRAY_FILTER_DATA_SIZE,
                               d_test_af_is_even);

    result = D_TEST_CHECK_MSG(_suite,
                              "macro_where_count",
                              res.count == 5,
                              "D_CONTIGUOUS_FILTER_WHERE(int, ..., is_even) should produce 5") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "macro_where_ok",
                              d_contiguous_filter_result_ok(&res) == true,
                              "D_CONTIGUOUS_FILTER_WHERE result should be ok") && result;

    d_contiguous_filter_result_free(&res);

    return result;
}


/*
d_tests_array_filter_macro_where_ctx
  Tests the D_CONTIGUOUS_FILTER_WHERE_CTX macro.
  Tests the following:
  - Passes context through correctly
  - Produces correct filtered count
*/
bool d_tests_array_filter_macro_where_ctx(
    struct dj_test_suite* _suite
)
{
    bool                         result;
    int                          data[D_TEST_ARRAY_FILTER_DATA_SIZE];
    struct dj_contiguous_filter_result res;
    int                          threshold;

    d_test_suite_test(_suite, "array_filter_macro_where_ctx");

    result    = true;
    threshold = 6;
    d_test_af_fill_sequential(data, D_TEST_ARRAY_FILTER_DATA_SIZE);

    // test 1: elements > 6 -> {7, 8, 9}
    res = D_CONTIGUOUS_FILTER_WHERE_CTX(int, data, D_TEST_ARRAY_FILTER_DATA_SIZE,
                                   d_test_af_is_greater_than,
                                   &threshold);

    result = D_TEST_CHECK_MSG(_suite,
                              "macro_where_ctx_count",
                              res.count == 3,
                              "D_CONTIGUOUS_FILTER_WHERE_CTX(>6) should produce 3") && result;

    d_contiguous_filter_result_free(&res);

    return result;
}


/*
d_tests_array_filter_macro_first_n
  Tests the D_CONTIGUOUS_FILTER_FIRST_N macro.
  Tests the following:
  - Expands to d_contiguous_filter_take_first with correct sizeof
  - Returns correct count
*/
bool d_tests_array_filter_macro_first_n(
    struct dj_test_suite* _suite
)
{
    bool                         result;
    int                          data[D_TEST_ARRAY_FILTER_DATA_SIZE];
    struct dj_contiguous_filter_result res;
    int*                         out;

    d_test_suite_test(_suite, "array_filter_macro_first_n");

    result = true;
    d_test_af_fill_sequential(data, D_TEST_ARRAY_FILTER_DATA_SIZE);

    // test 1: first 4 elements
    res = D_CONTIGUOUS_FILTER_FIRST_N(int, data, D_TEST_ARRAY_FILTER_DATA_SIZE, 4);

    result = D_TEST_CHECK_MSG(_suite,
                              "macro_first_n_count",
                              res.count == 4,
                              "D_CONTIGUOUS_FILTER_FIRST_N(4) should produce 4 elements") && result;

    out = (int*)res.data;

    if (out && res.count == 4)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "macro_first_n_values",
                                  ( (out[0] == 0) && (out[3] == 3) ),
                                  "First 4 elements should be {0, 1, 2, 3}") && result;
    }

    d_contiguous_filter_result_free(&res);

    return result;
}


/*
d_tests_array_filter_macro_last_n
  Tests the D_CONTIGUOUS_FILTER_LAST_N macro.
  Tests the following:
  - Expands to d_contiguous_filter_take_last with correct sizeof
  - Returns correct values
*/
bool d_tests_array_filter_macro_last_n(
    struct dj_test_suite* _suite
)
{
    bool                         result;
    int                          data[D_TEST_ARRAY_FILTER_DATA_SIZE];
    struct dj_contiguous_filter_result res;
    int*                         out;

    d_test_suite_test(_suite, "array_filter_macro_last_n");

    result = true;
    d_test_af_fill_sequential(data, D_TEST_ARRAY_FILTER_DATA_SIZE);

    // test 1: last 2 elements
    res = D_CONTIGUOUS_FILTER_LAST_N(int, data, D_TEST_ARRAY_FILTER_DATA_SIZE, 2);

    result = D_TEST_CHECK_MSG(_suite,
                              "macro_last_n_count",
                              res.count == 2,
                              "D_CONTIGUOUS_FILTER_LAST_N(2) should produce 2 elements") && result;

    out = (int*)res.data;

    if (out && res.count == 2)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "macro_last_n_values",
                                  ( (out[0] == 8) && (out[1] == 9) ),
                                  "Last 2 elements should be {8, 9}") && result;
    }

    d_contiguous_filter_result_free(&res);

    return result;
}


/*
d_tests_array_filter_macro_range
  Tests the D_CONTIGUOUS_FILTER_RANGE macro.
  Tests the following:
  - Expands correctly for half-open range
*/
bool d_tests_array_filter_macro_range(
    struct dj_test_suite* _suite
)
{
    bool                         result;
    int                          data[D_TEST_ARRAY_FILTER_DATA_SIZE];
    struct dj_contiguous_filter_result res;
    int*                         out;

    d_test_suite_test(_suite, "array_filter_macro_range");

    result = true;
    d_test_af_fill_sequential(data, D_TEST_ARRAY_FILTER_DATA_SIZE);

    // test 1: range [3, 7) -> {3, 4, 5, 6}
    res = D_CONTIGUOUS_FILTER_RANGE(int, data, D_TEST_ARRAY_FILTER_DATA_SIZE, 3, 7);

    result = D_TEST_CHECK_MSG(_suite,
                              "macro_range_count",
                              res.count == 4,
                              "D_CONTIGUOUS_FILTER_RANGE(3,7) should produce 4 elements") && result;

    out = (int*)res.data;

    if (out && res.count == 4)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "macro_range_values",
                                  ( (out[0] == 3) && (out[3] == 6) ),
                                  "D_CONTIGUOUS_FILTER_RANGE(3,7) should yield {3, 4, 5, 6}") && result;
    }

    d_contiguous_filter_result_free(&res);

    return result;
}


/*
d_tests_array_filter_macro_slice
  Tests the D_CONTIGUOUS_FILTER_SLICE macro.
  Tests the following:
  - Expands correctly for [start:end:step]
*/
bool d_tests_array_filter_macro_slice(
    struct dj_test_suite* _suite
)
{
    bool                         result;
    int                          data[D_TEST_ARRAY_FILTER_DATA_SIZE];
    struct dj_contiguous_filter_result res;

    d_test_suite_test(_suite, "array_filter_macro_slice");

    result = true;
    d_test_af_fill_sequential(data, D_TEST_ARRAY_FILTER_DATA_SIZE);

    // test 1: slice [0:10:3] -> {0, 3, 6, 9}
    res = D_CONTIGUOUS_FILTER_SLICE(int, data, D_TEST_ARRAY_FILTER_DATA_SIZE, 0, 10, 3);

    result = D_TEST_CHECK_MSG(_suite,
                              "macro_slice_count",
                              res.count == 4,
                              "D_CONTIGUOUS_FILTER_SLICE(0,10,3) should produce 4 elements") && result;

    d_contiguous_filter_result_free(&res);

    return result;
}


/*
d_tests_array_filter_macro_distinct
  Tests the D_CONTIGUOUS_FILTER_DISTINCT macro.
  Tests the following:
  - Expands correctly and removes duplicates
*/
bool d_tests_array_filter_macro_distinct(
    struct dj_test_suite* _suite
)
{
    bool                         result;
    int                          data[D_TEST_ARRAY_FILTER_DATA_SIZE];
    struct dj_contiguous_filter_result res;

    d_test_suite_test(_suite, "array_filter_macro_distinct");

    result = true;
    d_test_af_fill_with_duplicates(data, D_TEST_ARRAY_FILTER_DATA_SIZE);

    // test 1: removes duplicates
    res = D_CONTIGUOUS_FILTER_DISTINCT(int, data, D_TEST_ARRAY_FILTER_DATA_SIZE,
                                  d_test_af_compare_int);

    result = D_TEST_CHECK_MSG(_suite,
                              "macro_distinct_count",
                              res.count == 7,
                              "D_CONTIGUOUS_FILTER_DISTINCT should produce 7 unique elements") && result;

    d_contiguous_filter_result_free(&res);

    return result;
}


/*
d_tests_array_filter_macro_in_place
  Tests the D_CONTIGUOUS_FILTER_IN_PLACE macro.
  Tests the following:
  - Expands correctly and filters in-place
  - Infers element_size and passes NULL context
*/
bool d_tests_array_filter_macro_in_place(
    struct dj_test_suite* _suite
)
{
    bool   result;
    int    data[D_TEST_ARRAY_FILTER_DATA_SIZE];
    size_t new_count;

    d_test_suite_test(_suite, "array_filter_macro_in_place");

    result = true;
    d_test_af_fill_sequential(data, D_TEST_ARRAY_FILTER_DATA_SIZE);

    // test 1: in-place filter even
    new_count = D_CONTIGUOUS_FILTER_IN_PLACE(int, data, D_TEST_ARRAY_FILTER_DATA_SIZE,
                                        d_test_af_is_even);

    result = D_TEST_CHECK_MSG(_suite,
                              "macro_in_place_count",
                              new_count == 5,
                              "D_CONTIGUOUS_FILTER_IN_PLACE(is_even) should return 5") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "macro_in_place_values",
                              ( (data[0] == 0) && (data[4] == 8) ),
                              "Compacted array should start with 0 and end with 8") && result;

    return result;
}


/*
d_tests_array_filter_macro_all
  Aggregation function that runs all convenience macro tests.
*/
bool d_tests_array_filter_macro_all(
    struct dj_test_suite* _suite
)
{
    bool result;

    d_test_suite_block(_suite, "array_filter_macro_all");

    result = true;

    result = d_tests_array_filter_macro_where(_suite) && result;
    result = d_tests_array_filter_macro_where_ctx(_suite) && result;
    result = d_tests_array_filter_macro_first_n(_suite) && result;
    result = d_tests_array_filter_macro_last_n(_suite) && result;
    result = d_tests_array_filter_macro_range(_suite) && result;
    result = d_tests_array_filter_macro_slice(_suite) && result;
    result = d_tests_array_filter_macro_distinct(_suite) && result;
    result = d_tests_array_filter_macro_in_place(_suite) && result;

    return result;
}
