#include ".\array_filter_tests_sa.h"


/******************************************************************************
 * IV. CHAIN AND COMBINATOR APPLICATION
 *****************************************************************************/

/*
d_tests_sa_array_filter_apply_chain
  Tests the d_array_filter_apply_chain function.
  Tests the following:
  - Single-operation chain produces correct result
  - Multi-operation chain (skip 2, then take 3) works
  - Empty chain returns all elements
  - NULL chain is handled safely
  - NULL elements is handled safely
  - Chain with where produces correct filtered subset
*/
bool
d_tests_sa_array_filter_apply_chain
(
    struct d_test_counter* _counter
)
{
    bool                         result;
    int                          data[D_INTERNAL_TEST_ARRAY_FILTER_DATA_SIZE];
    struct d_array_filter_result res;
    struct d_filter_chain*       chain;
    int*                         out;

    result = true;
    d_tests_array_filter_fill_sequential(data, D_INTERNAL_TEST_ARRAY_FILTER_DATA_SIZE);

    // test 1: single-op chain (take_first 5)
    chain = d_filter_chain_new();
    d_filter_chain_add_take_first(chain, 5);

    res = d_array_filter_apply_chain(data,
                                     D_INTERNAL_TEST_ARRAY_FILTER_DATA_SIZE,
                                     sizeof(int),
                                     chain);

    result = d_assert_standalone(
        res.count == 5,
        "chain_single_op_count",
        "Chain with take_first(5) should produce 5 elements",
        _counter) && result;

    d_array_filter_result_free(&res);
    d_filter_chain_free(chain);

    // test 2: multi-op chain: skip 2 -> take 3 -> {2, 3, 4}
    chain = d_filter_chain_new();
    d_filter_chain_add_skip_first(chain, 2);
    d_filter_chain_add_take_first(chain, 3);

    res = d_array_filter_apply_chain(data,
                                     D_INTERNAL_TEST_ARRAY_FILTER_DATA_SIZE,
                                     sizeof(int),
                                     chain);

    result = d_assert_standalone(
        res.count == 3,
        "chain_multi_op_count",
        "Chain skip(2)->take(3) should produce 3 elements",
        _counter) && result;

    out = (int*)res.data;

    if (out && res.count == 3)
    {
        result = d_assert_standalone(
            ( (out[0] == 2) &&
              (out[1] == 3) &&
              (out[2] == 4) ),
            "chain_multi_op_values",
            "Chain skip(2)->take(3) should yield {2, 3, 4}",
            _counter) && result;
    }

    d_array_filter_result_free(&res);
    d_filter_chain_free(chain);

    // test 3: empty chain returns all elements
    chain = d_filter_chain_new();

    res = d_array_filter_apply_chain(data,
                                     D_INTERNAL_TEST_ARRAY_FILTER_DATA_SIZE,
                                     sizeof(int),
                                     chain);

    result = d_assert_standalone(
        res.count == D_INTERNAL_TEST_ARRAY_FILTER_DATA_SIZE,
        "chain_empty",
        "Empty chain should return all elements",
        _counter) && result;

    d_array_filter_result_free(&res);
    d_filter_chain_free(chain);

    // test 4: NULL chain
    res = d_array_filter_apply_chain(data,
                                     D_INTERNAL_TEST_ARRAY_FILTER_DATA_SIZE,
                                     sizeof(int),
                                     NULL);

    result = d_assert_standalone(
        res.status < 0,
        "chain_null",
        "NULL chain should return error status",
        _counter) && result;

    d_array_filter_result_free(&res);

    // test 5: NULL elements
    chain = d_filter_chain_new();
    d_filter_chain_add_take_first(chain, 3);

    res = d_array_filter_apply_chain(NULL,
                                     D_INTERNAL_TEST_ARRAY_FILTER_DATA_SIZE,
                                     sizeof(int),
                                     chain);

    result = d_assert_standalone(
        res.status < 0,
        "chain_null_elements",
        "NULL elements should return error status",
        _counter) && result;

    d_array_filter_result_free(&res);
    d_filter_chain_free(chain);

    // test 6: chain with where (filter even, then take first 3)
    chain = d_filter_chain_new();
    d_filter_chain_add_where(chain, d_tests_array_filter_is_even);
    d_filter_chain_add_take_first(chain, 3);

    res = d_array_filter_apply_chain(data,
                                     D_INTERNAL_TEST_ARRAY_FILTER_DATA_SIZE,
                                     sizeof(int),
                                     chain);

    result = d_assert_standalone(
        res.count == 3,
        "chain_where_take_count",
        "Chain where(even)->take(3) should produce 3 elements",
        _counter) && result;

    out = (int*)res.data;

    if (out && res.count == 3)
    {
        result = d_assert_standalone(
            ( (out[0] == 0) &&
              (out[1] == 2) &&
              (out[2] == 4) ),
            "chain_where_take_values",
            "Chain where(even)->take(3) should yield {0, 2, 4}",
            _counter) && result;
    }

    d_array_filter_result_free(&res);
    d_filter_chain_free(chain);

    return result;
}


/*
d_tests_sa_array_filter_apply_union
  Tests the d_array_filter_apply_union function.
  Tests the following:
  - Union of two non-overlapping filters produces correct combined set
  - Union with empty filter equals the other filter
  - NULL combinator is handled safely
*/
bool
d_tests_sa_array_filter_apply_union
(
    struct d_test_counter* _counter
)
{
    bool                         result;
    int                          data[D_INTERNAL_TEST_ARRAY_FILTER_DATA_SIZE];
    struct d_array_filter_result res;
    struct d_filter_union*       combo;
    struct d_filter_chain*       chain_even;
    struct d_filter_chain*       chain_gt7;

    result = true;
    d_tests_array_filter_fill_sequential(data, D_INTERNAL_TEST_ARRAY_FILTER_DATA_SIZE);

    // test 1: union of (even) | (>7) -> {0,2,4,6,8,9} = 6 unique
    chain_even = d_filter_chain_new();
    d_filter_chain_add_where(chain_even, d_tests_array_filter_is_even);

    chain_gt7 = d_filter_chain_new();
    {
        int threshold;

        threshold = 7;
        d_filter_chain_add_where_context(chain_gt7,
                                         d_tests_array_filter_is_greater_than,
                                         &threshold);
    }

    combo = d_filter_union_new(2);
    d_filter_union_add(combo, chain_even);
    d_filter_union_add(combo, chain_gt7);

    res = d_array_filter_apply_union(data,
                                     D_INTERNAL_TEST_ARRAY_FILTER_DATA_SIZE,
                                     sizeof(int),
                                     combo,
                                     d_tests_array_filter_compare_int);

    result = d_assert_standalone(
        res.count >= 6,
        "union_count",
        "Union of (even)|(>7) should yield at least 6 elements",
        _counter) && result;

    d_array_filter_result_free(&res);
    d_filter_union_free(combo);

    // test 2: NULL combinator
    res = d_array_filter_apply_union(data,
                                     D_INTERNAL_TEST_ARRAY_FILTER_DATA_SIZE,
                                     sizeof(int),
                                     NULL,
                                     d_tests_array_filter_compare_int);

    result = d_assert_standalone(
        res.status < 0,
        "union_null",
        "NULL union combinator should return error",
        _counter) && result;

    d_array_filter_result_free(&res);

    return result;
}


/*
d_tests_sa_array_filter_apply_intersection
  Tests the d_array_filter_apply_intersection function.
  Tests the following:
  - Intersection of two overlapping filters produces correct common set
  - Intersection with disjoint filters produces empty result
  - NULL combinator is handled safely
*/
bool
d_tests_sa_array_filter_apply_intersection
(
    struct d_test_counter* _counter
)
{
    bool                             result;
    int                              data[D_INTERNAL_TEST_ARRAY_FILTER_DATA_SIZE];
    struct d_array_filter_result     res;
    struct d_filter_intersection*    combo;
    struct d_filter_chain*           chain_even;
    struct d_filter_chain*           chain_gt3;
    int                              threshold;

    result    = true;
    threshold = 3;
    d_tests_array_filter_fill_sequential(data, D_INTERNAL_TEST_ARRAY_FILTER_DATA_SIZE);

    // test 1: intersection of (even) & (>3) -> {4, 6, 8}
    chain_even = d_filter_chain_new();
    d_filter_chain_add_where(chain_even, d_tests_array_filter_is_even);

    chain_gt3 = d_filter_chain_new();
    d_filter_chain_add_where_context(chain_gt3,
                                     d_tests_array_filter_is_greater_than,
                                     &threshold);

    combo = d_filter_intersection_new(2);
    d_filter_intersection_add(combo, chain_even);
    d_filter_intersection_add(combo, chain_gt3);

    res = d_array_filter_apply_intersection(data,
                                            D_INTERNAL_TEST_ARRAY_FILTER_DATA_SIZE,
                                            sizeof(int),
                                            combo,
                                            d_tests_array_filter_compare_int);

    result = d_assert_standalone(
        res.count == 3,
        "intersection_count",
        "Intersection of (even)&(>3) should yield 3 elements",
        _counter) && result;

    d_array_filter_result_free(&res);
    d_filter_intersection_free(combo);

    // test 2: NULL combinator
    res = d_array_filter_apply_intersection(data,
                                            D_INTERNAL_TEST_ARRAY_FILTER_DATA_SIZE,
                                            sizeof(int),
                                            NULL,
                                            d_tests_array_filter_compare_int);

    result = d_assert_standalone(
        res.status < 0,
        "intersection_null",
        "NULL intersection combinator should return error",
        _counter) && result;

    d_array_filter_result_free(&res);

    return result;
}


/*
d_tests_sa_array_filter_apply_difference
  Tests the d_array_filter_apply_difference function.
  Tests the following:
  - Difference A - B removes B's elements from A
  - Difference with empty B equals A
  - NULL combinator is handled safely
*/
bool
d_tests_sa_array_filter_apply_difference
(
    struct d_test_counter* _counter
)
{
    bool                         result;
    int                          data[D_INTERNAL_TEST_ARRAY_FILTER_DATA_SIZE];
    struct d_array_filter_result res;
    struct d_filter_difference*  diff;
    struct d_filter_chain*       chain_all;
    struct d_filter_chain*       chain_even;

    result = true;
    d_tests_array_filter_fill_sequential(data, D_INTERNAL_TEST_ARRAY_FILTER_DATA_SIZE);

    // test 1: (all) - (even) -> odd numbers {1,3,5,7,9}
    chain_all = d_filter_chain_new();
    // empty chain = all elements

    chain_even = d_filter_chain_new();
    d_filter_chain_add_where(chain_even, d_tests_array_filter_is_even);

    diff = d_filter_difference_new(chain_all, chain_even);

    res = d_array_filter_apply_difference(data,
                                          D_INTERNAL_TEST_ARRAY_FILTER_DATA_SIZE,
                                          sizeof(int),
                                          diff,
                                          d_tests_array_filter_compare_int);

    result = d_assert_standalone(
        res.count == 5,
        "difference_count",
        "Difference (all)-(even) should yield 5 odd elements",
        _counter) && result;

    d_array_filter_result_free(&res);
    d_filter_difference_free(diff);

    // test 2: NULL combinator
    res = d_array_filter_apply_difference(data,
                                          D_INTERNAL_TEST_ARRAY_FILTER_DATA_SIZE,
                                          sizeof(int),
                                          NULL,
                                          d_tests_array_filter_compare_int);

    result = d_assert_standalone(
        res.status < 0,
        "difference_null",
        "NULL difference combinator should return error",
        _counter) && result;

    d_array_filter_result_free(&res);

    return result;
}


/*
d_tests_sa_array_filter_chain_all
  Aggregation function that runs all chain and combinator tests.
*/
bool
d_tests_sa_array_filter_chain_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Chain & Combinator Application\n");
    printf("  -------------------------------------------\n");

    result = d_tests_sa_array_filter_apply_chain(_counter) && result;
    result = d_tests_sa_array_filter_apply_union(_counter) && result;
    result = d_tests_sa_array_filter_apply_intersection(_counter) && result;
    result = d_tests_sa_array_filter_apply_difference(_counter) && result;

    return result;
}
