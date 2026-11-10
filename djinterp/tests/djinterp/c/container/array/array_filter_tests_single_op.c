/*******************************************************************************
* djinterp [container]                            array_filter_tests_single_op.c
*
*
* path:      /tests/djinterp/c/container/array/array_filter_tests_single_op.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.02.20
*                                                            revised: 2026.11.09
*******************************************************************************/
// djinterp
#include "array_filter_tests.h"
#include "../../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's C face


/******************************************************************************
 * II. SINGLE-OPERATION FILTER FUNCTIONS
 *****************************************************************************/

/*
d_tests_array_filter_take_first
  Tests the d_contiguous_filter_take_first function.
  Tests the following:
  - Takes correct number of elements from front
  - Result contains the right values
  - n=0 produces empty result
  - n >= count returns all elements
  - NULL input is handled safely
  - Empty array (count=0) is handled safely
*/
bool d_tests_array_filter_take_first(
    struct dj_test_suite* _suite
)
{
    bool                         result;
    int                          data[D_TEST_ARRAY_FILTER_DATA_SIZE];
    struct dj_contiguous_filter_result res;
    int*                         out;

    d_test_suite_test(_suite, "array_filter_take_first");

    result = true;
    d_test_af_fill_sequential(data, D_TEST_ARRAY_FILTER_DATA_SIZE);

    // test 1: take first 3 elements from {0,1,2,...,9}
    res = d_contiguous_filter_take_first(data,
                                    D_TEST_ARRAY_FILTER_DATA_SIZE,
                                    sizeof(int),
                                    3);

    result = D_TEST_CHECK_MSG(_suite,
                              "take_first_count",
                              res.count == 3,
                              "take_first(3) should produce 3 elements") && result;

    // test 2: verify values are {0, 1, 2}
    out = (int*)res.data;

    if (out && res.count == 3)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "take_first_values",
                                  ( (out[0] == 0) && (out[1] == 1) && (out[2] == 2) ),
                                  "take_first(3) should yield {0, 1, 2}") && result;
    }

    d_contiguous_filter_result_free(&res);

    // test 3: n=0 produces empty result
    res = d_contiguous_filter_take_first(data,
                                    D_TEST_ARRAY_FILTER_DATA_SIZE,
                                    sizeof(int),
                                    0);

    result = D_TEST_CHECK_MSG(_suite,
                              "take_first_zero",
                              res.count == 0,
                              "take_first(0) should produce 0 elements") && result;

    d_contiguous_filter_result_free(&res);

    // test 4: n >= count returns all elements
    res = d_contiguous_filter_take_first(data,
                                    D_TEST_ARRAY_FILTER_DATA_SIZE,
                                    sizeof(int),
                                    100);

    result = D_TEST_CHECK_MSG(_suite,
                              "take_first_overflow",
                              res.count == D_TEST_ARRAY_FILTER_DATA_SIZE,
                              "take_first(100) on 10 elements should return all 10") && result;

    d_contiguous_filter_result_free(&res);

    // test 5: NULL input is handled safely
    res = d_contiguous_filter_take_first(NULL,
                                    D_TEST_ARRAY_FILTER_DATA_SIZE,
                                    sizeof(int),
                                    3);

    result = D_TEST_CHECK_MSG(_suite,
                              "take_first_null_input",
                              res.status < 0,
                              "take_first with NULL input should return error status") && result;

    d_contiguous_filter_result_free(&res);

    // test 6: empty array
    res = d_contiguous_filter_take_first(data,
                                    0,
                                    sizeof(int),
                                    3);

    result = D_TEST_CHECK_MSG(_suite,
                              "take_first_empty_array",
                              res.count == 0,
                              "take_first on empty array should produce 0 elements") && result;

    d_contiguous_filter_result_free(&res);

    return result;
}


/*
d_tests_array_filter_take_last
  Tests the d_contiguous_filter_take_last function.
  Tests the following:
  - Takes correct number of elements from end
  - Result contains the right values
  - n=0 produces empty result
  - n >= count returns all elements
  - Single element array
*/
bool d_tests_array_filter_take_last(
    struct dj_test_suite* _suite
)
{
    bool                         result;
    int                          data[D_TEST_ARRAY_FILTER_DATA_SIZE];
    struct dj_contiguous_filter_result res;
    int*                         out;

    d_test_suite_test(_suite, "array_filter_take_last");

    result = true;
    d_test_af_fill_sequential(data, D_TEST_ARRAY_FILTER_DATA_SIZE);

    // test 1: take last 3 from {0,1,...,9} -> {7,8,9}
    res = d_contiguous_filter_take_last(data,
                                   D_TEST_ARRAY_FILTER_DATA_SIZE,
                                   sizeof(int),
                                   3);

    result = D_TEST_CHECK_MSG(_suite,
                              "take_last_count",
                              res.count == 3,
                              "take_last(3) should produce 3 elements") && result;

    // test 2: verify values
    out = (int*)res.data;

    if (out && res.count == 3)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "take_last_values",
                                  ( (out[0] == 7) && (out[1] == 8) && (out[2] == 9) ),
                                  "take_last(3) should yield {7, 8, 9}") && result;
    }

    d_contiguous_filter_result_free(&res);

    // test 3: n=0 produces empty result
    res = d_contiguous_filter_take_last(data,
                                   D_TEST_ARRAY_FILTER_DATA_SIZE,
                                   sizeof(int),
                                   0);

    result = D_TEST_CHECK_MSG(_suite,
                              "take_last_zero",
                              res.count == 0,
                              "take_last(0) should produce 0 elements") && result;

    d_contiguous_filter_result_free(&res);

    // test 4: n >= count returns all
    res = d_contiguous_filter_take_last(data,
                                   D_TEST_ARRAY_FILTER_DATA_SIZE,
                                   sizeof(int),
                                   999);

    result = D_TEST_CHECK_MSG(_suite,
                              "take_last_overflow",
                              res.count == D_TEST_ARRAY_FILTER_DATA_SIZE,
                              "take_last(999) on 10 elements should return all 10") && result;

    d_contiguous_filter_result_free(&res);

    // test 5: single element array
    {
        int single;

        single = 42;
        res = d_contiguous_filter_take_last(&single, 1, sizeof(int), 1);
        out = (int*)res.data;

        result = D_TEST_CHECK_MSG(_suite,
                                  "take_last_single",
                                  ( (res.count == 1) && (out) && (out[0] == 42) ),
                                  "take_last(1) on single element should yield that element") && result;

        d_contiguous_filter_result_free(&res);
    }

    return result;
}


/*
d_tests_array_filter_take_nth
  Tests the d_contiguous_filter_take_nth function.
  Tests the following:
  - Takes every nth element
  - n=1 returns all elements
  - n=count returns only the first element
  - n > count returns only the first element
  - n=0 is handled safely (edge case)
*/
bool d_tests_array_filter_take_nth(
    struct dj_test_suite* _suite
)
{
    bool                         result;
    int                          data[D_TEST_ARRAY_FILTER_DATA_SIZE];
    struct dj_contiguous_filter_result res;
    int*                         out;

    d_test_suite_test(_suite, "array_filter_take_nth");

    result = true;
    d_test_af_fill_sequential(data, D_TEST_ARRAY_FILTER_DATA_SIZE);

    // test 1: every 2nd element from {0..9} -> {0,2,4,6,8}
    res = d_contiguous_filter_take_nth(data,
                                  D_TEST_ARRAY_FILTER_DATA_SIZE,
                                  sizeof(int),
                                  2);

    result = D_TEST_CHECK_MSG(_suite,
                              "take_nth_2_count",
                              res.count == 5,
                              "take_nth(2) on 10 elements should produce 5") && result;

    out = (int*)res.data;

    if (out && res.count == 5)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "take_nth_2_values",
                                  ( (out[0] == 0) && (out[1] == 2) && (out[2] == 4) && (out[3] == 6) && (out[4] == 8) ),
                                  "take_nth(2) should yield {0, 2, 4, 6, 8}") && result;
    }

    d_contiguous_filter_result_free(&res);

    // test 2: every 3rd element -> {0,3,6,9}
    res = d_contiguous_filter_take_nth(data,
                                  D_TEST_ARRAY_FILTER_DATA_SIZE,
                                  sizeof(int),
                                  3);

    result = D_TEST_CHECK_MSG(_suite,
                              "take_nth_3_count",
                              res.count == 4,
                              "take_nth(3) on 10 elements should produce 4") && result;

    d_contiguous_filter_result_free(&res);

    // test 3: n=1 returns all elements
    res = d_contiguous_filter_take_nth(data,
                                  D_TEST_ARRAY_FILTER_DATA_SIZE,
                                  sizeof(int),
                                  1);

    result = D_TEST_CHECK_MSG(_suite,
                              "take_nth_1_returns_all",
                              res.count == D_TEST_ARRAY_FILTER_DATA_SIZE,
                              "take_nth(1) should return all elements") && result;

    d_contiguous_filter_result_free(&res);

    // test 4: n > count returns only first element
    res = d_contiguous_filter_take_nth(data,
                                  D_TEST_ARRAY_FILTER_DATA_SIZE,
                                  sizeof(int),
                                  100);

    result = D_TEST_CHECK_MSG(_suite,
                              "take_nth_large_n",
                              res.count == 1,
                              "take_nth(100) on 10 elements should return 1 element") && result;

    d_contiguous_filter_result_free(&res);

    // test 5: n=0 edge case
    res = d_contiguous_filter_take_nth(data,
                                  D_TEST_ARRAY_FILTER_DATA_SIZE,
                                  sizeof(int),
                                  0);

    result = D_TEST_CHECK_MSG(_suite,
                              "take_nth_zero",
                              res.status < 0,
                              "take_nth(0) should return error status (invalid step)") && result;

    d_contiguous_filter_result_free(&res);

    return result;
}


/*
d_tests_array_filter_head
  Tests the d_contiguous_filter_head function.
  Tests the following:
  - Returns exactly 1 element (the first)
  - Value matches first element of source
  - Empty array is handled safely
*/
bool d_tests_array_filter_head(
    struct dj_test_suite* _suite
)
{
    bool                         result;
    int                          data[D_TEST_ARRAY_FILTER_DATA_SIZE];
    struct dj_contiguous_filter_result res;
    int*                         out;

    d_test_suite_test(_suite, "array_filter_head");

    result = true;
    d_test_af_fill_sequential(data, D_TEST_ARRAY_FILTER_DATA_SIZE);

    // test 1: head returns 1 element
    res = d_contiguous_filter_head(data,
                              D_TEST_ARRAY_FILTER_DATA_SIZE,
                              sizeof(int));

    result = D_TEST_CHECK_MSG(_suite,
                              "head_count",
                              res.count == 1,
                              "head should return exactly 1 element") && result;

    // test 2: value is the first element (0)
    out = (int*)res.data;

    if (out && res.count == 1)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "head_value",
                                  out[0] == 0,
                                  "head of {0,1,...,9} should be 0") && result;
    }

    d_contiguous_filter_result_free(&res);

    // test 3: empty array
    res = d_contiguous_filter_head(data, 0, sizeof(int));

    result = D_TEST_CHECK_MSG(_suite,
                              "head_empty",
                              res.count == 0,
                              "head of empty array should produce 0 elements") && result;

    d_contiguous_filter_result_free(&res);

    return result;
}


/*
d_tests_array_filter_tail
  Tests the d_contiguous_filter_tail function.
  Tests the following:
  - Returns exactly 1 element (the last)
  - Value matches last element of source
  - Empty array is handled safely
*/
bool d_tests_array_filter_tail(
    struct dj_test_suite* _suite
)
{
    bool                         result;
    int                          data[D_TEST_ARRAY_FILTER_DATA_SIZE];
    struct dj_contiguous_filter_result res;
    int*                         out;

    d_test_suite_test(_suite, "array_filter_tail");

    result = true;
    d_test_af_fill_sequential(data, D_TEST_ARRAY_FILTER_DATA_SIZE);

    // test 1: tail returns 1 element
    res = d_contiguous_filter_tail(data,
                              D_TEST_ARRAY_FILTER_DATA_SIZE,
                              sizeof(int));

    result = D_TEST_CHECK_MSG(_suite,
                              "tail_count",
                              res.count == 1,
                              "tail should return exactly 1 element") && result;

    // test 2: value is the last element (9)
    out = (int*)res.data;

    if (out && res.count == 1)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "tail_value",
                                  out[0] == 9,
                                  "tail of {0,1,...,9} should be 9") && result;
    }

    d_contiguous_filter_result_free(&res);

    // test 3: empty array
    res = d_contiguous_filter_tail(data, 0, sizeof(int));

    result = D_TEST_CHECK_MSG(_suite,
                              "tail_empty",
                              res.count == 0,
                              "tail of empty array should produce 0 elements") && result;

    d_contiguous_filter_result_free(&res);

    return result;
}


/*
d_tests_array_filter_skip_first
  Tests the d_contiguous_filter_skip_first function.
  Tests the following:
  - Skips correct number of elements from front
  - Result values are correct
  - n=0 returns all elements
  - n >= count returns empty
  - NULL input is handled safely
*/
bool d_tests_array_filter_skip_first(
    struct dj_test_suite* _suite
)
{
    bool                         result;
    int                          data[D_TEST_ARRAY_FILTER_DATA_SIZE];
    struct dj_contiguous_filter_result res;
    int*                         out;

    d_test_suite_test(_suite, "array_filter_skip_first");

    result = true;
    d_test_af_fill_sequential(data, D_TEST_ARRAY_FILTER_DATA_SIZE);

    // test 1: skip first 3 from {0..9} -> {3,4,5,6,7,8,9}
    res = d_contiguous_filter_skip_first(data,
                                    D_TEST_ARRAY_FILTER_DATA_SIZE,
                                    sizeof(int),
                                    3);

    result = D_TEST_CHECK_MSG(_suite,
                              "skip_first_count",
                              res.count == 7,
                              "skip_first(3) on 10 elements should produce 7") && result;

    out = (int*)res.data;

    if (out && res.count == 7)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "skip_first_values",
                                  ( (out[0] == 3) && (out[6] == 9) ),
                                  "skip_first(3) should start at 3 and end at 9") && result;
    }

    d_contiguous_filter_result_free(&res);

    // test 2: n=0 returns all elements
    res = d_contiguous_filter_skip_first(data,
                                    D_TEST_ARRAY_FILTER_DATA_SIZE,
                                    sizeof(int),
                                    0);

    result = D_TEST_CHECK_MSG(_suite,
                              "skip_first_zero",
                              res.count == D_TEST_ARRAY_FILTER_DATA_SIZE,
                              "skip_first(0) should return all elements") && result;

    d_contiguous_filter_result_free(&res);

    // test 3: n >= count returns empty
    res = d_contiguous_filter_skip_first(data,
                                    D_TEST_ARRAY_FILTER_DATA_SIZE,
                                    sizeof(int),
                                    100);

    result = D_TEST_CHECK_MSG(_suite,
                              "skip_first_overflow",
                              res.count == 0,
                              "skip_first(100) on 10 elements should produce 0") && result;

    d_contiguous_filter_result_free(&res);

    // test 4: NULL input
    res = d_contiguous_filter_skip_first(NULL,
                                    D_TEST_ARRAY_FILTER_DATA_SIZE,
                                    sizeof(int),
                                    3);

    result = D_TEST_CHECK_MSG(_suite,
                              "skip_first_null",
                              res.status < 0,
                              "skip_first with NULL should return error status") && result;

    d_contiguous_filter_result_free(&res);

    return result;
}


/*
d_tests_array_filter_skip_last
  Tests the d_contiguous_filter_skip_last function.
  Tests the following:
  - Skips correct number of elements from end
  - Result values are correct
  - n=0 returns all elements
  - n >= count returns empty
*/
bool d_tests_array_filter_skip_last(
    struct dj_test_suite* _suite
)
{
    bool                         result;
    int                          data[D_TEST_ARRAY_FILTER_DATA_SIZE];
    struct dj_contiguous_filter_result res;
    int*                         out;

    d_test_suite_test(_suite, "array_filter_skip_last");

    result = true;
    d_test_af_fill_sequential(data, D_TEST_ARRAY_FILTER_DATA_SIZE);

    // test 1: skip last 4 from {0..9} -> {0,1,2,3,4,5}
    res = d_contiguous_filter_skip_last(data,
                                   D_TEST_ARRAY_FILTER_DATA_SIZE,
                                   sizeof(int),
                                   4);

    result = D_TEST_CHECK_MSG(_suite,
                              "skip_last_count",
                              res.count == 6,
                              "skip_last(4) on 10 elements should produce 6") && result;

    out = (int*)res.data;

    if (out && res.count == 6)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "skip_last_values",
                                  ( (out[0] == 0) && (out[5] == 5) ),
                                  "skip_last(4) should yield {0..5}") && result;
    }

    d_contiguous_filter_result_free(&res);

    // test 2: n=0 returns all
    res = d_contiguous_filter_skip_last(data,
                                   D_TEST_ARRAY_FILTER_DATA_SIZE,
                                   sizeof(int),
                                   0);

    result = D_TEST_CHECK_MSG(_suite,
                              "skip_last_zero",
                              res.count == D_TEST_ARRAY_FILTER_DATA_SIZE,
                              "skip_last(0) should return all elements") && result;

    d_contiguous_filter_result_free(&res);

    // test 3: n >= count returns empty
    res = d_contiguous_filter_skip_last(data,
                                   D_TEST_ARRAY_FILTER_DATA_SIZE,
                                   sizeof(int),
                                   D_TEST_ARRAY_FILTER_DATA_SIZE);

    result = D_TEST_CHECK_MSG(_suite,
                              "skip_last_all",
                              res.count == 0,
                              "skip_last(count) should produce 0 elements") && result;

    d_contiguous_filter_result_free(&res);

    return result;
}


/*
d_tests_array_filter_init
  Tests the d_contiguous_filter_init function (all except last).
  Tests the following:
  - Returns count-1 elements
  - Result is the prefix of the source
  - Empty and single-element arrays
*/
bool d_tests_array_filter_init(
    struct dj_test_suite* _suite
)
{
    bool                         result;
    int                          data[D_TEST_ARRAY_FILTER_DATA_SIZE];
    struct dj_contiguous_filter_result res;
    int*                         out;

    d_test_suite_test(_suite, "array_filter_init");

    result = true;
    d_test_af_fill_sequential(data, D_TEST_ARRAY_FILTER_DATA_SIZE);

    // test 1: init of {0..9} -> {0..8}
    res = d_contiguous_filter_init(data,
                              D_TEST_ARRAY_FILTER_DATA_SIZE,
                              sizeof(int));

    result = D_TEST_CHECK_MSG(_suite,
                              "init_count",
                              res.count == D_TEST_ARRAY_FILTER_DATA_SIZE - 1,
                              "init should return count - 1 elements") && result;

    out = (int*)res.data;

    if (out && res.count > 0)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "init_values",
                                  ( (out[0] == 0) && (out[res.count - 1] == 8) ),
                                  "init should exclude the last element (9)") && result;
    }

    d_contiguous_filter_result_free(&res);

    // test 2: empty array
    res = d_contiguous_filter_init(data, 0, sizeof(int));

    result = D_TEST_CHECK_MSG(_suite,
                              "init_empty",
                              res.count == 0,
                              "init of empty array should produce 0 elements") && result;

    d_contiguous_filter_result_free(&res);

    // test 3: single element -> empty
    res = d_contiguous_filter_init(data, 1, sizeof(int));

    result = D_TEST_CHECK_MSG(_suite,
                              "init_single",
                              res.count == 0,
                              "init of single-element array should produce 0 elements") && result;

    d_contiguous_filter_result_free(&res);

    return result;
}


/*
d_tests_array_filter_rest
  Tests the d_contiguous_filter_rest function (all except first).
  Tests the following:
  - Returns count-1 elements
  - Result is the suffix of the source
  - Empty and single-element arrays
*/
bool d_tests_array_filter_rest(
    struct dj_test_suite* _suite
)
{
    bool                         result;
    int                          data[D_TEST_ARRAY_FILTER_DATA_SIZE];
    struct dj_contiguous_filter_result res;
    int*                         out;

    d_test_suite_test(_suite, "array_filter_rest");

    result = true;
    d_test_af_fill_sequential(data, D_TEST_ARRAY_FILTER_DATA_SIZE);

    // test 1: rest of {0..9} -> {1..9}
    res = d_contiguous_filter_rest(data,
                              D_TEST_ARRAY_FILTER_DATA_SIZE,
                              sizeof(int));

    result = D_TEST_CHECK_MSG(_suite,
                              "rest_count",
                              res.count == D_TEST_ARRAY_FILTER_DATA_SIZE - 1,
                              "rest should return count - 1 elements") && result;

    out = (int*)res.data;

    if (out && res.count > 0)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "rest_values",
                                  ( (out[0] == 1) && (out[res.count - 1] == 9) ),
                                  "rest should exclude the first element (0)") && result;
    }

    d_contiguous_filter_result_free(&res);

    // test 2: single element -> empty
    res = d_contiguous_filter_rest(data, 1, sizeof(int));

    result = D_TEST_CHECK_MSG(_suite,
                              "rest_single",
                              res.count == 0,
                              "rest of single-element array should produce 0 elements") && result;

    d_contiguous_filter_result_free(&res);

    return result;
}


/*
d_tests_array_filter_range
  Tests the d_contiguous_filter_range function.
  Tests the following:
  - Half-open range [start, end) returns correct elements
  - Full range [0, count) returns all elements
  - Empty range (start == end) returns 0 elements
  - start > end is handled safely
  - end > count clamps to count
*/
bool d_tests_array_filter_range(
    struct dj_test_suite* _suite
)
{
    bool                         result;
    int                          data[D_TEST_ARRAY_FILTER_DATA_SIZE];
    struct dj_contiguous_filter_result res;
    int*                         out;

    d_test_suite_test(_suite, "array_filter_range");

    result = true;
    d_test_af_fill_sequential(data, D_TEST_ARRAY_FILTER_DATA_SIZE);

    // test 1: range [2, 5) from {0..9} -> {2, 3, 4}
    res = d_contiguous_filter_range(data,
                               D_TEST_ARRAY_FILTER_DATA_SIZE,
                               sizeof(int),
                               2, 5);

    result = D_TEST_CHECK_MSG(_suite,
                              "range_count",
                              res.count == 3,
                              "range [2,5) should produce 3 elements") && result;

    out = (int*)res.data;

    if (out && res.count == 3)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "range_values",
                                  ( (out[0] == 2) && (out[1] == 3) && (out[2] == 4) ),
                                  "range [2,5) should yield {2, 3, 4}") && result;
    }

    d_contiguous_filter_result_free(&res);

    // test 2: full range
    res = d_contiguous_filter_range(data,
                               D_TEST_ARRAY_FILTER_DATA_SIZE,
                               sizeof(int),
                               0, D_TEST_ARRAY_FILTER_DATA_SIZE);

    result = D_TEST_CHECK_MSG(_suite,
                              "range_full",
                              res.count == D_TEST_ARRAY_FILTER_DATA_SIZE,
                              "range [0,count) should return all elements") && result;

    d_contiguous_filter_result_free(&res);

    // test 3: empty range (start == end)
    res = d_contiguous_filter_range(data,
                               D_TEST_ARRAY_FILTER_DATA_SIZE,
                               sizeof(int),
                               3, 3);

    result = D_TEST_CHECK_MSG(_suite,
                              "range_empty",
                              res.count == 0,
                              "range [3,3) should produce 0 elements") && result;

    d_contiguous_filter_result_free(&res);

    // test 4: start > end
    res = d_contiguous_filter_range(data,
                               D_TEST_ARRAY_FILTER_DATA_SIZE,
                               sizeof(int),
                               5, 2);

    result = D_TEST_CHECK_MSG(_suite,
                              "range_inverted",
                              ( (res.count == 0) || (res.status < 0) ),
                              "range [5,2) should produce 0 elements or error") && result;

    d_contiguous_filter_result_free(&res);

    // test 5: end > count clamps
    res = d_contiguous_filter_range(data,
                               D_TEST_ARRAY_FILTER_DATA_SIZE,
                               sizeof(int),
                               7, 100);

    result = D_TEST_CHECK_MSG(_suite,
                              "range_clamp_end",
                              res.count == 3,
                              "range [7,100) on 10 elements should produce 3") && result;

    d_contiguous_filter_result_free(&res);

    return result;
}


/*
d_tests_array_filter_slice
  Tests the d_contiguous_filter_slice function.
  Tests the following:
  - Basic slice [start:end:step] with step > 1
  - step=1 equivalent to range
  - step=2 takes every other element in range
  - step > (end - start) returns only first element in range
*/
bool d_tests_array_filter_slice(
    struct dj_test_suite* _suite
)
{
    bool                         result;
    int                          data[D_TEST_ARRAY_FILTER_DATA_SIZE];
    struct dj_contiguous_filter_result res;
    int*                         out;

    d_test_suite_test(_suite, "array_filter_slice");

    result = true;
    d_test_af_fill_sequential(data, D_TEST_ARRAY_FILTER_DATA_SIZE);

    // test 1: slice [0:10:2] -> {0, 2, 4, 6, 8}
    res = d_contiguous_filter_slice(data,
                               D_TEST_ARRAY_FILTER_DATA_SIZE,
                               sizeof(int),
                               0, D_TEST_ARRAY_FILTER_DATA_SIZE, 2);

    result = D_TEST_CHECK_MSG(_suite,
                              "slice_step2_count",
                              res.count == 5,
                              "slice [0:10:2] should produce 5 elements") && result;

    out = (int*)res.data;

    if (out && res.count == 5)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "slice_step2_values",
                                  ( (out[0] == 0) && (out[2] == 4) && (out[4] == 8) ),
                                  "slice [0:10:2] should yield {0, 2, 4, 6, 8}") && result;
    }

    d_contiguous_filter_result_free(&res);

    // test 2: slice [1:8:3] -> {1, 4, 7}
    res = d_contiguous_filter_slice(data,
                               D_TEST_ARRAY_FILTER_DATA_SIZE,
                               sizeof(int),
                               1, 8, 3);

    result = D_TEST_CHECK_MSG(_suite,
                              "slice_step3_count",
                              res.count == 3,
                              "slice [1:8:3] should produce 3 elements") && result;

    out = (int*)res.data;

    if (out && res.count == 3)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "slice_step3_values",
                                  ( (out[0] == 1) && (out[1] == 4) && (out[2] == 7) ),
                                  "slice [1:8:3] should yield {1, 4, 7}") && result;
    }

    d_contiguous_filter_result_free(&res);

    // test 3: step=1 equivalent to range
    res = d_contiguous_filter_slice(data,
                               D_TEST_ARRAY_FILTER_DATA_SIZE,
                               sizeof(int),
                               2, 5, 1);

    result = D_TEST_CHECK_MSG(_suite,
                              "slice_step1",
                              res.count == 3,
                              "slice [2:5:1] should equal range [2,5)") && result;

    d_contiguous_filter_result_free(&res);

    // test 4: step larger than range -> 1 element
    res = d_contiguous_filter_slice(data,
                               D_TEST_ARRAY_FILTER_DATA_SIZE,
                               sizeof(int),
                               3, 6, 100);

    result = D_TEST_CHECK_MSG(_suite,
                              "slice_large_step",
                              res.count == 1,
                              "slice with step > range should return 1 element") && result;

    d_contiguous_filter_result_free(&res);

    return result;
}


/*
d_tests_array_filter_where
  Tests the d_contiguous_filter_where function.
  Tests the following:
  - Filters even numbers correctly
  - Filters with context-based predicate
  - Always-true predicate returns all elements
  - Always-false predicate returns 0 elements
  - NULL predicate is handled safely
*/
bool d_tests_array_filter_where(
    struct dj_test_suite* _suite
)
{
    bool                         result;
    int                          data[D_TEST_ARRAY_FILTER_DATA_SIZE];
    struct dj_contiguous_filter_result res;
    int*                         out;
    int                          threshold;

    d_test_suite_test(_suite, "array_filter_where");

    result = true;
    d_test_af_fill_sequential(data, D_TEST_ARRAY_FILTER_DATA_SIZE);

    // test 1: filter even from {0..9} -> {0,2,4,6,8}
    res = d_contiguous_filter_where(data,
                               D_TEST_ARRAY_FILTER_DATA_SIZE,
                               sizeof(int),
                               d_test_af_is_even,
                               NULL);

    result = D_TEST_CHECK_MSG(_suite,
                              "where_even_count",
                              res.count == 5,
                              "where(is_even) on {0..9} should produce 5 elements") && result;

    out = (int*)res.data;

    if (out && res.count == 5)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "where_even_values",
                                  ( (out[0] == 0) && (out[1] == 2) && (out[4] == 8) ),
                                  "where(is_even) should yield {0, 2, 4, 6, 8}") && result;
    }

    d_contiguous_filter_result_free(&res);

    // test 2: filter with context: values > 5 -> {6,7,8,9}
    threshold = 5;
    res = d_contiguous_filter_where(data,
                               D_TEST_ARRAY_FILTER_DATA_SIZE,
                               sizeof(int),
                               d_test_af_is_greater_than,
                               &threshold);

    result = D_TEST_CHECK_MSG(_suite,
                              "where_ctx_count",
                              res.count == 4,
                              "where(>5) on {0..9} should produce 4 elements") && result;

    d_contiguous_filter_result_free(&res);

    // test 3: always_true returns all
    res = d_contiguous_filter_where(data,
                               D_TEST_ARRAY_FILTER_DATA_SIZE,
                               sizeof(int),
                               d_test_af_always_true,
                               NULL);

    result = D_TEST_CHECK_MSG(_suite,
                              "where_always_true",
                              res.count == D_TEST_ARRAY_FILTER_DATA_SIZE,
                              "where(always_true) should return all elements") && result;

    d_contiguous_filter_result_free(&res);

    // test 4: always_false returns 0
    res = d_contiguous_filter_where(data,
                               D_TEST_ARRAY_FILTER_DATA_SIZE,
                               sizeof(int),
                               d_test_af_always_false,
                               NULL);

    result = D_TEST_CHECK_MSG(_suite,
                              "where_always_false",
                              res.count == 0,
                              "where(always_false) should return 0 elements") && result;

    d_contiguous_filter_result_free(&res);

    // test 5: NULL predicate
    res = d_contiguous_filter_where(data,
                               D_TEST_ARRAY_FILTER_DATA_SIZE,
                               sizeof(int),
                               NULL,
                               NULL);

    result = D_TEST_CHECK_MSG(_suite,
                              "where_null_predicate",
                              res.status < 0,
                              "where(NULL) should return error status") && result;

    d_contiguous_filter_result_free(&res);

    return result;
}


/*
d_tests_array_filter_where_not
  Tests the d_contiguous_filter_where_not function.
  Tests the following:
  - Inverts predicate correctly (odd numbers)
  - where_not(always_true) returns 0
  - where_not(always_false) returns all
*/
bool d_tests_array_filter_where_not(
    struct dj_test_suite* _suite
)
{
    bool                         result;
    int                          data[D_TEST_ARRAY_FILTER_DATA_SIZE];
    struct dj_contiguous_filter_result res;

    d_test_suite_test(_suite, "array_filter_where_not");

    result = true;
    d_test_af_fill_sequential(data, D_TEST_ARRAY_FILTER_DATA_SIZE);

    // test 1: where_not(is_even) -> odd numbers {1,3,5,7,9}
    res = d_contiguous_filter_where_not(data,
                                   D_TEST_ARRAY_FILTER_DATA_SIZE,
                                   sizeof(int),
                                   d_test_af_is_even,
                                   NULL);

    result = D_TEST_CHECK_MSG(_suite,
                              "where_not_even_count",
                              res.count == 5,
                              "where_not(is_even) on {0..9} should produce 5 odd elements") && result;

    d_contiguous_filter_result_free(&res);

    // test 2: where_not(always_true) -> 0 elements
    res = d_contiguous_filter_where_not(data,
                                   D_TEST_ARRAY_FILTER_DATA_SIZE,
                                   sizeof(int),
                                   d_test_af_always_true,
                                   NULL);

    result = D_TEST_CHECK_MSG(_suite,
                              "where_not_always_true",
                              res.count == 0,
                              "where_not(always_true) should return 0 elements") && result;

    d_contiguous_filter_result_free(&res);

    // test 3: where_not(always_false) -> all elements
    res = d_contiguous_filter_where_not(data,
                                   D_TEST_ARRAY_FILTER_DATA_SIZE,
                                   sizeof(int),
                                   d_test_af_always_false,
                                   NULL);

    result = D_TEST_CHECK_MSG(_suite,
                              "where_not_always_false",
                              res.count == D_TEST_ARRAY_FILTER_DATA_SIZE,
                              "where_not(always_false) should return all elements") && result;

    d_contiguous_filter_result_free(&res);

    return result;
}


/*
d_tests_array_filter_at_indices
  Tests the d_contiguous_filter_at_indices function.
  Tests the following:
  - Selects elements at specified indices
  - Out-of-bounds indices are handled safely
  - Empty index list returns 0 elements
  - NULL index list is handled safely
  - Duplicate indices
*/
bool d_tests_array_filter_at_indices(
    struct dj_test_suite* _suite
)
{
    bool                         result;
    int                          data[D_TEST_ARRAY_FILTER_DATA_SIZE];
    struct dj_contiguous_filter_result res;
    int*                         out;
    size_t                       indices[3];
    size_t                       dup_indices[4];

    d_test_suite_test(_suite, "array_filter_at_indices");

    result = true;
    d_test_af_fill_sequential(data, D_TEST_ARRAY_FILTER_DATA_SIZE);

    // test 1: pick indices {0, 4, 9} -> values {0, 4, 9}
    indices[0] = 0;
    indices[1] = 4;
    indices[2] = 9;

    res = d_contiguous_filter_at_indices(data,
                                    D_TEST_ARRAY_FILTER_DATA_SIZE,
                                    sizeof(int),
                                    indices, 3);

    result = D_TEST_CHECK_MSG(_suite,
                              "at_indices_count",
                              res.count == 3,
                              "at_indices({0,4,9}) should produce 3 elements") && result;

    out = (int*)res.data;

    if (out && res.count == 3)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "at_indices_values",
                                  ( (out[0] == 0) && (out[1] == 4) && (out[2] == 9) ),
                                  "at_indices({0,4,9}) should yield {0, 4, 9}") && result;
    }

    d_contiguous_filter_result_free(&res);

    // test 2: empty index list
    res = d_contiguous_filter_at_indices(data,
                                    D_TEST_ARRAY_FILTER_DATA_SIZE,
                                    sizeof(int),
                                    indices, 0);

    result = D_TEST_CHECK_MSG(_suite,
                              "at_indices_empty",
                              res.count == 0,
                              "at_indices with 0 indices should return 0 elements") && result;

    d_contiguous_filter_result_free(&res);

    // test 3: NULL index list
    res = d_contiguous_filter_at_indices(data,
                                    D_TEST_ARRAY_FILTER_DATA_SIZE,
                                    sizeof(int),
                                    NULL, 3);

    result = D_TEST_CHECK_MSG(_suite,
                              "at_indices_null",
                              res.status < 0,
                              "at_indices with NULL indices should return error") && result;

    d_contiguous_filter_result_free(&res);

    // test 4: duplicate indices {2, 2, 5, 5}
    dup_indices[0] = 2;
    dup_indices[1] = 2;
    dup_indices[2] = 5;
    dup_indices[3] = 5;

    res = d_contiguous_filter_at_indices(data,
                                    D_TEST_ARRAY_FILTER_DATA_SIZE,
                                    sizeof(int),
                                    dup_indices, 4);

    result = D_TEST_CHECK_MSG(_suite,
                              "at_indices_duplicates",
                              res.count == 4,
                              "at_indices with duplicate indices should honour all of them") && result;

    d_contiguous_filter_result_free(&res);

    return result;
}


/*
d_tests_array_filter_distinct
  Tests the d_contiguous_filter_distinct function.
  Tests the following:
  - Removes duplicates correctly
  - Already-unique array returns all elements
  - All-same array returns 1 element
  - Empty array returns 0 elements
*/
bool d_tests_array_filter_distinct(
    struct dj_test_suite* _suite
)
{
    bool                         result;
    int                          data[D_TEST_ARRAY_FILTER_DATA_SIZE];
    int                          same[5];
    struct dj_contiguous_filter_result res;
    size_t                       i;

    d_test_suite_test(_suite, "array_filter_distinct");

    result = true;

    // test 1: {3,1,4,1,5,9,2,6,5,3} -> 7 unique values
    d_test_af_fill_with_duplicates(data, D_TEST_ARRAY_FILTER_DATA_SIZE);

    res = d_contiguous_filter_distinct(data,
                                  D_TEST_ARRAY_FILTER_DATA_SIZE,
                                  sizeof(int),
                                  d_test_af_compare_int);

    result = D_TEST_CHECK_MSG(_suite,
                              "distinct_dup_count",
                              res.count == 7,
                              "distinct on data with duplicates should yield 7 unique elements") && result;

    d_contiguous_filter_result_free(&res);

    // test 2: already unique
    d_test_af_fill_sequential(data, D_TEST_ARRAY_FILTER_DATA_SIZE);

    res = d_contiguous_filter_distinct(data,
                                  D_TEST_ARRAY_FILTER_DATA_SIZE,
                                  sizeof(int),
                                  d_test_af_compare_int);

    result = D_TEST_CHECK_MSG(_suite,
                              "distinct_unique",
                              res.count == D_TEST_ARRAY_FILTER_DATA_SIZE,
                              "distinct on already-unique data should return all elements") && result;

    d_contiguous_filter_result_free(&res);

    // test 3: all same -> 1 element
    for (i = 0; i < 5; ++i)
    {
        same[i] = 7;
    }

    res = d_contiguous_filter_distinct(same,
                                  5,
                                  sizeof(int),
                                  d_test_af_compare_int);

    result = D_TEST_CHECK_MSG(_suite,
                              "distinct_all_same",
                              res.count == 1,
                              "distinct on all-identical array should yield 1 element") && result;

    d_contiguous_filter_result_free(&res);

    // test 4: empty array
    res = d_contiguous_filter_distinct(data,
                                  0,
                                  sizeof(int),
                                  d_test_af_compare_int);

    result = D_TEST_CHECK_MSG(_suite,
                              "distinct_empty",
                              res.count == 0,
                              "distinct on empty array should yield 0 elements") && result;

    d_contiguous_filter_result_free(&res);

    return result;
}


/*
d_tests_array_filter_reverse
  Tests the d_contiguous_filter_reverse function.
  Tests the following:
  - Reverses element order correctly
  - Single element remains unchanged
  - Empty array returns 0 elements
  - Double reverse restores original
*/
bool d_tests_array_filter_reverse(
    struct dj_test_suite* _suite
)
{
    bool                         result;
    int                          data[D_TEST_ARRAY_FILTER_DATA_SIZE];
    struct dj_contiguous_filter_result res;
    struct dj_contiguous_filter_result res2;
    int*                         out;

    d_test_suite_test(_suite, "array_filter_reverse");

    result = true;
    d_test_af_fill_sequential(data, D_TEST_ARRAY_FILTER_DATA_SIZE);

    // test 1: reverse {0..9} -> {9,8,...,0}
    res = d_contiguous_filter_reverse(data,
                                 D_TEST_ARRAY_FILTER_DATA_SIZE,
                                 sizeof(int));

    result = D_TEST_CHECK_MSG(_suite,
                              "reverse_count",
                              res.count == D_TEST_ARRAY_FILTER_DATA_SIZE,
                              "reverse should preserve element count") && result;

    out = (int*)res.data;

    if (out && res.count == D_TEST_ARRAY_FILTER_DATA_SIZE)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "reverse_values",
                                  ( (out[0] == 9) && (out[9] == 0) && (out[4] == 5) ),
                                  "reverse of {0..9} should yield {9,8,...,0}") && result;
    }

    // test 2: double reverse restores original
    if (out && res.count == D_TEST_ARRAY_FILTER_DATA_SIZE)
    {
        res2 = d_contiguous_filter_reverse(res.data,
                                      res.count,
                                      sizeof(int));
        out = (int*)res2.data;

        if (out && res2.count == D_TEST_ARRAY_FILTER_DATA_SIZE)
        {
            result = D_TEST_CHECK_MSG(_suite,
                                      "reverse_double",
                                      ( (out[0] == 0) && (out[9] == 9) ),
                                      "Double reverse should restore original order") && result;
        }

        d_contiguous_filter_result_free(&res2);
    }

    d_contiguous_filter_result_free(&res);

    // test 3: single element
    res = d_contiguous_filter_reverse(data, 1, sizeof(int));

    result = D_TEST_CHECK_MSG(_suite,
                              "reverse_single",
                              res.count == 1,
                              "reverse of single element should return 1 element") && result;

    d_contiguous_filter_result_free(&res);

    // test 4: empty array
    res = d_contiguous_filter_reverse(data, 0, sizeof(int));

    result = D_TEST_CHECK_MSG(_suite,
                              "reverse_empty",
                              res.count == 0,
                              "reverse of empty array should return 0 elements") && result;

    d_contiguous_filter_result_free(&res);

    return result;
}


/*
d_tests_array_filter_single_op_all
  Aggregation function that runs all single-operation filter tests.
*/
bool d_tests_array_filter_single_op_all(
    struct dj_test_suite* _suite
)
{
    bool result;

    d_test_suite_block(_suite, "array_filter_single_op_all");

    result = true;

    result = d_tests_array_filter_take_first(_suite) && result;
    result = d_tests_array_filter_take_last(_suite) && result;
    result = d_tests_array_filter_take_nth(_suite) && result;
    result = d_tests_array_filter_head(_suite) && result;
    result = d_tests_array_filter_tail(_suite) && result;
    result = d_tests_array_filter_skip_first(_suite) && result;
    result = d_tests_array_filter_skip_last(_suite) && result;
    result = d_tests_array_filter_init(_suite) && result;
    result = d_tests_array_filter_rest(_suite) && result;
    result = d_tests_array_filter_range(_suite) && result;
    result = d_tests_array_filter_slice(_suite) && result;
    result = d_tests_array_filter_where(_suite) && result;
    result = d_tests_array_filter_where_not(_suite) && result;
    result = d_tests_array_filter_at_indices(_suite) && result;
    result = d_tests_array_filter_distinct(_suite) && result;
    result = d_tests_array_filter_reverse(_suite) && result;

    return result;
}
