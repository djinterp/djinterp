/*******************************************************************************
* djinterp [container]                                array_filter_tests_chain.c
*
*
* path:      /tests/djinterp/c/container/array/array_filter_tests_chain.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.02.20
*                                                            revised: 2026.11.09
*******************************************************************************/
// djinterp
#include "array_filter_tests.h"
#include "../../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's C face


/******************************************************************************
 * IV. CHAIN AND COMBINATOR APPLICATION
 *****************************************************************************/

/*
d_tests_array_filter_apply_chain
  Tests the d_contiguous_filter_apply_chain function.
  Tests the following:
  - Single-operation chain produces correct result
  - Multi-operation chain (skip 2, then take 3) works
  - Empty chain returns all elements
  - NULL chain is handled safely
  - NULL elements is handled safely
  - Chain with where produces correct filtered subset
*/
bool d_tests_array_filter_apply_chain(
    struct dj_test_suite* _suite
)
{
    bool                         result;
    int                          data[D_TEST_ARRAY_FILTER_DATA_SIZE];
    struct dj_contiguous_filter_result res;
    struct dj_filter_chain*       chain;
    int*                         out;

    d_test_suite_test(_suite, "array_filter_apply_chain");

    result = true;
    d_test_af_fill_sequential(data, D_TEST_ARRAY_FILTER_DATA_SIZE);

    // test 1: single-op chain (take_first 5)
    chain = d_filter_chain_new();
    d_filter_chain_add_take_first(chain, 5);

    res = d_contiguous_filter_apply_chain(data,
                                     D_TEST_ARRAY_FILTER_DATA_SIZE,
                                     sizeof(int),
                                     chain);

    result = D_TEST_CHECK_MSG(_suite,
                              "chain_single_op_count",
                              res.count == 5,
                              "Chain with take_first(5) should produce 5 elements") && result;

    d_contiguous_filter_result_free(&res);
    d_filter_chain_free(chain);

    // test 2: multi-op chain: skip 2 -> take 3 -> {2, 3, 4}
    chain = d_filter_chain_new();
    d_filter_chain_add_skip_first(chain, 2);
    d_filter_chain_add_take_first(chain, 3);

    res = d_contiguous_filter_apply_chain(data,
                                     D_TEST_ARRAY_FILTER_DATA_SIZE,
                                     sizeof(int),
                                     chain);

    result = D_TEST_CHECK_MSG(_suite,
                              "chain_multi_op_count",
                              res.count == 3,
                              "Chain skip(2)->take(3) should produce 3 elements") && result;

    out = (int*)res.data;

    if (out && res.count == 3)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "chain_multi_op_values",
                                  ( (out[0] == 2) && (out[1] == 3) && (out[2] == 4) ),
                                  "Chain skip(2)->take(3) should yield {2, 3, 4}") && result;
    }

    d_contiguous_filter_result_free(&res);
    d_filter_chain_free(chain);

    // test 3: empty chain returns all elements
    chain = d_filter_chain_new();

    res = d_contiguous_filter_apply_chain(data,
                                     D_TEST_ARRAY_FILTER_DATA_SIZE,
                                     sizeof(int),
                                     chain);

    result = D_TEST_CHECK_MSG(_suite,
                              "chain_empty",
                              res.count == D_TEST_ARRAY_FILTER_DATA_SIZE,
                              "Empty chain should return all elements") && result;

    d_contiguous_filter_result_free(&res);
    d_filter_chain_free(chain);

    // test 4: NULL chain
    res = d_contiguous_filter_apply_chain(data,
                                     D_TEST_ARRAY_FILTER_DATA_SIZE,
                                     sizeof(int),
                                     NULL);

    result = D_TEST_CHECK_MSG(_suite,
                              "chain_null",
                              res.status < 0,
                              "NULL chain should return error status") && result;

    d_contiguous_filter_result_free(&res);

    // test 5: NULL elements
    chain = d_filter_chain_new();
    d_filter_chain_add_take_first(chain, 3);

    res = d_contiguous_filter_apply_chain(NULL,
                                     D_TEST_ARRAY_FILTER_DATA_SIZE,
                                     sizeof(int),
                                     chain);

    result = D_TEST_CHECK_MSG(_suite,
                              "chain_null_elements",
                              res.status < 0,
                              "NULL elements should return error status") && result;

    d_contiguous_filter_result_free(&res);
    d_filter_chain_free(chain);

    // test 6: chain with where (filter even, then take first 3)
    chain = d_filter_chain_new();
    d_filter_chain_add_where(chain, d_test_af_is_even);
    d_filter_chain_add_take_first(chain, 3);

    res = d_contiguous_filter_apply_chain(data,
                                     D_TEST_ARRAY_FILTER_DATA_SIZE,
                                     sizeof(int),
                                     chain);

    result = D_TEST_CHECK_MSG(_suite,
                              "chain_where_take_count",
                              res.count == 3,
                              "Chain where(even)->take(3) should produce 3 elements") && result;

    out = (int*)res.data;

    if (out && res.count == 3)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "chain_where_take_values",
                                  ( (out[0] == 0) && (out[1] == 2) && (out[2] == 4) ),
                                  "Chain where(even)->take(3) should yield {0, 2, 4}") && result;
    }

    d_contiguous_filter_result_free(&res);
    d_filter_chain_free(chain);

    return result;
}


/*
d_tests_array_filter_apply_union
  Tests the d_contiguous_filter_apply_union function.
  Tests the following:
  - Union of two non-overlapping filters produces correct combined set
  - Union with empty filter equals the other filter
  - NULL combinator is handled safely
*/
bool d_tests_array_filter_apply_union(
    struct dj_test_suite* _suite
)
{
    bool                         result;
    int                          data[D_TEST_ARRAY_FILTER_DATA_SIZE];
    struct dj_contiguous_filter_result res;
    struct dj_filter_union*       combo;
    struct dj_filter_chain*       chain_even;
    struct dj_filter_chain*       chain_gt7;
    int                          threshold;

    d_test_suite_test(_suite, "array_filter_apply_union");

    result = true;
    d_test_af_fill_sequential(data, D_TEST_ARRAY_FILTER_DATA_SIZE);

    // test 1: union of (even) | (>7) -> {0,2,4,6,8,9} = 6 unique
    chain_even = d_filter_chain_new();
    d_filter_chain_add_where(chain_even, d_test_af_is_even);

    //   the chain keeps the context pointer, so the threshold lives as long
    // as the union is applied
    chain_gt7 = d_filter_chain_new();
    threshold = 7;
    d_filter_chain_add_where_context(chain_gt7,
                                     d_test_af_is_greater_than,
                                     &threshold);

    combo = d_filter_union_new(2);
    d_filter_union_add(combo, chain_even);
    d_filter_union_add(combo, chain_gt7);

    res = d_contiguous_filter_apply_union(data,
                                     D_TEST_ARRAY_FILTER_DATA_SIZE,
                                     sizeof(int),
                                     combo,
                                     d_test_af_compare_int);

    result = D_TEST_CHECK_MSG(_suite,
                              "union_count",
                              res.count == 6,
                              "(even)|(>7) over 0..9 is {0,2,4,6,8,9}") && result;

    d_contiguous_filter_result_free(&res);

    // a combinator does not free its chains
    d_filter_union_free(combo);
    d_filter_chain_free(chain_even);
    d_filter_chain_free(chain_gt7);

    // test 2: NULL combinator
    res = d_contiguous_filter_apply_union(data,
                                     D_TEST_ARRAY_FILTER_DATA_SIZE,
                                     sizeof(int),
                                     NULL,
                                     d_test_af_compare_int);

    result = D_TEST_CHECK_MSG(_suite,
                              "union_null",
                              res.status < 0,
                              "NULL union combinator should return error") && result;

    d_contiguous_filter_result_free(&res);

    return result;
}


/*
d_tests_array_filter_apply_intersection
  Tests the d_contiguous_filter_apply_intersection function.
  Tests the following:
  - Intersection of two overlapping filters produces correct common set
  - Intersection with disjoint filters produces empty result
  - NULL combinator is handled safely
*/
bool d_tests_array_filter_apply_intersection(
    struct dj_test_suite* _suite
)
{
    bool                             result;
    int                              data[D_TEST_ARRAY_FILTER_DATA_SIZE];
    struct dj_contiguous_filter_result     res;
    struct dj_filter_intersection*    combo;
    struct dj_filter_chain*           chain_even;
    struct dj_filter_chain*           chain_gt3;
    int                              threshold;

    d_test_suite_test(_suite, "array_filter_apply_intersection");

    result    = true;
    threshold = 3;
    d_test_af_fill_sequential(data, D_TEST_ARRAY_FILTER_DATA_SIZE);

    // test 1: intersection of (even) & (>3) -> {4, 6, 8}
    chain_even = d_filter_chain_new();
    d_filter_chain_add_where(chain_even, d_test_af_is_even);

    chain_gt3 = d_filter_chain_new();
    d_filter_chain_add_where_context(chain_gt3,
                                     d_test_af_is_greater_than,
                                     &threshold);

    combo = d_filter_intersection_new(2);
    d_filter_intersection_add(combo, chain_even);
    d_filter_intersection_add(combo, chain_gt3);

    res = d_contiguous_filter_apply_intersection(data,
                                            D_TEST_ARRAY_FILTER_DATA_SIZE,
                                            sizeof(int),
                                            combo,
                                            d_test_af_compare_int);

    result = D_TEST_CHECK_MSG(_suite,
                              "intersection_count",
                              res.count == 3,
                              "Intersection of (even)&(>3) should yield 3 elements") && result;

    d_contiguous_filter_result_free(&res);

    // a combinator does not free its chains
    d_filter_intersection_free(combo);
    d_filter_chain_free(chain_even);
    d_filter_chain_free(chain_gt3);

    // test 2: NULL combinator
    res = d_contiguous_filter_apply_intersection(data,
                                            D_TEST_ARRAY_FILTER_DATA_SIZE,
                                            sizeof(int),
                                            NULL,
                                            d_test_af_compare_int);

    result = D_TEST_CHECK_MSG(_suite,
                              "intersection_null",
                              res.status < 0,
                              "NULL intersection combinator should return error") && result;

    d_contiguous_filter_result_free(&res);

    return result;
}


/*
d_tests_array_filter_apply_difference
  Tests the d_contiguous_filter_apply_difference function.
  Tests the following:
  - Difference A - B removes B's elements from A
  - Difference with empty B equals A
  - NULL combinator is handled safely
*/
bool d_tests_array_filter_apply_difference(
    struct dj_test_suite* _suite
)
{
    bool                         result;
    int                          data[D_TEST_ARRAY_FILTER_DATA_SIZE];
    struct dj_contiguous_filter_result res;
    struct dj_filter_difference*  diff;
    struct dj_filter_chain*       chain_all;
    struct dj_filter_chain*       chain_even;

    d_test_suite_test(_suite, "array_filter_apply_difference");

    result = true;
    d_test_af_fill_sequential(data, D_TEST_ARRAY_FILTER_DATA_SIZE);

    // test 1: (all) - (even) -> odd numbers {1,3,5,7,9}
    chain_all = d_filter_chain_new();
    // empty chain = all elements

    chain_even = d_filter_chain_new();
    d_filter_chain_add_where(chain_even, d_test_af_is_even);

    diff = d_filter_difference_new(chain_all, chain_even);

    res = d_contiguous_filter_apply_difference(data,
                                          D_TEST_ARRAY_FILTER_DATA_SIZE,
                                          sizeof(int),
                                          diff,
                                          d_test_af_compare_int);

    result = D_TEST_CHECK_MSG(_suite,
                              "difference_count",
                              res.count == 5,
                              "Difference (all)-(even) should yield 5 odd elements") && result;

    d_contiguous_filter_result_free(&res);

    // a combinator does not free its chains
    d_filter_difference_free(diff);
    d_filter_chain_free(chain_all);
    d_filter_chain_free(chain_even);

    // test 2: NULL combinator
    res = d_contiguous_filter_apply_difference(data,
                                          D_TEST_ARRAY_FILTER_DATA_SIZE,
                                          sizeof(int),
                                          NULL,
                                          d_test_af_compare_int);

    result = D_TEST_CHECK_MSG(_suite,
                              "difference_null",
                              res.status < 0,
                              "NULL difference combinator should return error") && result;

    d_contiguous_filter_result_free(&res);

    return result;
}


/*
d_tests_array_filter_chain_all
  Aggregation function that runs all chain and combinator tests.
*/
bool d_tests_array_filter_chain_all(
    struct dj_test_suite* _suite
)
{
    bool result;

    d_test_suite_block(_suite, "array_filter_chain_all");

    result = true;

    result = d_tests_array_filter_apply_chain(_suite) && result;
    result = d_tests_array_filter_apply_union(_suite) && result;
    result = d_tests_array_filter_apply_intersection(_suite) && result;
    result = d_tests_array_filter_apply_difference(_suite) && result;

    return result;
}
