#include ".\filter_tests_sa.h"


///////////////////////////////////////////////////////////////////////////////
// HELPER PREDICATES
///////////////////////////////////////////////////////////////////////////////

static bool pred_is_even(const void* _element, void* _context)
{
    const int* value;

    (void)_context;

    if (!_element)
    {
        return false;
    }

    value = (const int*)_element;

    return (*value % 2 == 0);
}

static bool pred_is_positive(const void* _element, void* _context)
{
    const int* value;

    (void)_context;

    if (!_element)
    {
        return false;
    }

    value = (const int*)_element;

    return (*value > 0);
}

static int cmp_int(const void* _a, const void* _b, void* _context)
{
    const int* a;
    const int* b;

    (void)_context;

    a = (const int*)_a;
    b = (const int*)_b;

    return (*a - *b);
}


/*
d_tests_sa_filter_macros_single
  Tests single-expression convenience macros: D_FILTER, D_FILTER_FIRST_N,
  D_FILTER_LAST_N, D_FILTER_WHERE, D_FILTER_CHAIN_BEGIN/ADD/END.
  Tests the following:
  - D_FILTER with take_first op returns correct result
  - D_FILTER_FIRST_N returns first N elements
  - D_FILTER_FIRST_N with N > count returns all elements
  - D_FILTER_LAST_N returns last N elements
  - D_FILTER_LAST_N with N > count returns all elements
  - D_FILTER_WHERE filters by predicate correctly
  - D_FILTER_WHERE with no matches returns empty result
  - D_FILTER_CHAIN_BEGIN/ADD/END constructs and returns a chain
*/
bool
d_tests_sa_filter_macros_single
(
    struct d_test_counter* _counter
)
{
    bool                      result;
    struct d_filter_operation* op;
    struct d_filter_result*   res;
    struct d_filter_chain*    chain;
    int                       input[6];
    int*                      elems;

    result   = true;
    input[0] = 10;
    input[1] = 20;
    input[2] = 30;
    input[3] = 40;
    input[4] = 50;
    input[5] = 60;

    // test 1: D_FILTER with take_first(3)
    op  = d_filter_take_first(3);
    res = D_FILTER(input, 6, sizeof(int), op);

    result = d_assert_standalone(
        res->status == D_FILTER_RESULT_SUCCESS,
        "macro_d_filter_status",
        "D_FILTER should return SUCCESS",
        _counter) && result;

    result = d_assert_standalone(
        res->count == 3,
        "macro_d_filter_count",
        "D_FILTER with take_first(3) should produce 3",
        _counter) && result;

    if ( (res->elements) &&
         (res->count == 3) )
    {
        elems  = (int*)res->elements;
        result = d_assert_standalone(
            (elems[0] == 10) &&
            (elems[1] == 20) &&
            (elems[2] == 30),
            "macro_d_filter_values",
            "D_FILTER should return {10, 20, 30}",
            _counter) && result;
    }

    d_filter_result_free(res);
    free(res);

    // test 2: D_FILTER_FIRST_N returns first N
    res = D_FILTER_FIRST_N(input, 6, sizeof(int), 2);

    result = d_assert_standalone(
        res->count == 2,
        "macro_first_n_count",
        "D_FILTER_FIRST_N(2) should produce 2",
        _counter) && result;

    if ( (res->elements) &&
         (res->count == 2) )
    {
        elems  = (int*)res->elements;
        result = d_assert_standalone(
            (elems[0] == 10) &&
            (elems[1] == 20),
            "macro_first_n_values",
            "D_FILTER_FIRST_N(2) should return {10, 20}",
            _counter) && result;
    }

    d_filter_result_free(res);
    free(res);

    // test 3: D_FILTER_FIRST_N with N > count
    res = D_FILTER_FIRST_N(input, 6, sizeof(int), 100);

    result = d_assert_standalone(
        res->count == 6,
        "macro_first_n_clamp",
        "D_FILTER_FIRST_N(100) on 6 should return 6",
        _counter) && result;

    d_filter_result_free(res);
    free(res);

    // test 4: D_FILTER_LAST_N returns last N
    res = D_FILTER_LAST_N(input, 6, sizeof(int), 2);

    result = d_assert_standalone(
        res->count == 2,
        "macro_last_n_count",
        "D_FILTER_LAST_N(2) should produce 2",
        _counter) && result;

    if ( (res->elements) &&
         (res->count == 2) )
    {
        elems  = (int*)res->elements;
        result = d_assert_standalone(
            (elems[0] == 50) &&
            (elems[1] == 60),
            "macro_last_n_values",
            "D_FILTER_LAST_N(2) should return {50, 60}",
            _counter) && result;
    }

    d_filter_result_free(res);
    free(res);

    // test 5: D_FILTER_LAST_N with N > count
    res = D_FILTER_LAST_N(input, 6, sizeof(int), 100);

    result = d_assert_standalone(
        res->count == 6,
        "macro_last_n_clamp",
        "D_FILTER_LAST_N(100) on 6 should return 6",
        _counter) && result;

    d_filter_result_free(res);
    free(res);

    // test 6: D_FILTER_WHERE filters correctly
    // all values {10,20,30,40,50,60} are even, so all pass
    res = D_FILTER_WHERE(input, 6, sizeof(int), pred_is_even);

    result = d_assert_standalone(
        res->count == 6,
        "macro_where_all_even",
        "D_FILTER_WHERE(is_even) on all-even should return 6",
        _counter) && result;

    d_filter_result_free(res);
    free(res);

    // test 7: D_FILTER_WHERE with no matches
    // none are negative, so is_positive passes all;
    // use a mixed array for a real no-match test
    {
        int neg_input[3];

        neg_input[0] = -1;
        neg_input[1] = -2;
        neg_input[2] = -3;

        res = D_FILTER_WHERE(neg_input,
                             3,
                             sizeof(int),
                             pred_is_positive);

        result = d_assert_standalone(
            res->count == 0,
            "macro_where_no_match",
            "D_FILTER_WHERE(is_positive) on negatives should be 0",
            _counter) && result;

        d_filter_result_free(res);
    free(res);
    }

    // test 8: D_FILTER_CHAIN_BEGIN / ADD / END
    chain = D_FILTER_CHAIN_BEGIN();

    result = d_assert_standalone(
        chain != NULL,
        "macro_chain_begin",
        "D_FILTER_CHAIN_BEGIN should return non-NULL",
        _counter) && result;

    if (chain)
    {
        op = d_filter_take_first(4);
        D_FILTER_CHAIN_ADD(chain, op);

        op = d_filter_reverse();
        D_FILTER_CHAIN_ADD(chain, op);

        chain = D_FILTER_CHAIN_END(chain);

        result = d_assert_standalone(
            chain != NULL,
            "macro_chain_end",
            "D_FILTER_CHAIN_END should return the chain",
            _counter) && result;

        result = d_assert_standalone(
            chain->count == 2,
            "macro_chain_count",
            "chain should have 2 operations after ADD calls",
            _counter) && result;

        // verify the chain actually works
        res = d_filter_apply_chain(chain,
                                   input,
                                   6,
                                   sizeof(int));

        if ( (res->elements) &&
             (res->count == 4) )
        {
            elems  = (int*)res->elements;
            result = d_assert_standalone(
                (elems[0] == 40) &&
                (elems[1] == 30) &&
                (elems[2] == 20) &&
                (elems[3] == 10),
                "macro_chain_result",
                "take(4)->reverse should return {40,30,20,10}",
                _counter) && result;
        }

        d_filter_result_free(res);
    free(res);
        d_filter_chain_free(chain);
    }

    return result;
}


/*
d_tests_sa_filter_macros_fluent
  Tests fluent chain syntax macros: D_FILTER_BEGIN, D_THEN_* operations,
  D_FILTER_END.
  Tests the following:
  - D_FILTER_BEGIN creates a builder
  - D_THEN_TAKE_FIRST chains correctly
  - D_THEN_SKIP_FIRST chains correctly
  - D_THEN_WHERE chains correctly
  - D_THEN_WHERE_NOT chains correctly
  - D_THEN_RANGE chains correctly
  - D_THEN_REVERSE chains correctly
  - D_THEN_DISTINCT chains correctly
  - D_FILTER_END applies and returns result
  - full fluent pipeline produces correct values
  - multiple fluent pipelines can coexist
*/
bool
d_tests_sa_filter_macros_fluent
(
    struct d_test_counter* _counter
)
{
    bool                     result;
    struct d_filter_builder* builder;
    struct d_filter_result*  res;
    int                      input[8];
    int*                     elems;

    result   = true;
    input[0] = 1;
    input[1] = 2;
    input[2] = 3;
    input[3] = 4;
    input[4] = 5;
    input[5] = 6;
    input[6] = 7;
    input[7] = 8;

    // test 1: D_FILTER_BEGIN creates a builder
    builder = D_FILTER_BEGIN();

    result = d_assert_standalone(
        builder != NULL,
        "macro_fluent_begin",
        "D_FILTER_BEGIN should return non-NULL builder",
        _counter) && result;

    if (!builder)
    {
        return false;
    }

    d_filter_builder_free(builder);

    // test 2: full fluent pipeline
    // skip(2) -> where(even) -> take(2)
    // {1,2,3,4,5,6,7,8} -> skip 2 -> {3,4,5,6,7,8}
    //                    -> where(even) -> {4, 6, 8}
    //                    -> take 2 -> {4, 6}
    builder = D_FILTER_BEGIN();

    if (builder)
    {
        D_THEN_SKIP_FIRST(builder, 2);
        D_THEN_WHERE(builder, pred_is_even);
        D_THEN_TAKE_FIRST(builder, 2);

        res = D_FILTER_END(builder,
                           input,
                           8,
                           sizeof(int));

        result = d_assert_standalone(
            res->status == D_FILTER_RESULT_SUCCESS,
            "macro_fluent_pipeline_status",
            "fluent pipeline should return SUCCESS",
            _counter) && result;

        result = d_assert_standalone(
            res->count == 2,
            "macro_fluent_pipeline_count",
            "skip(2)->where(even)->take(2) should produce 2",
            _counter) && result;

        if ( (res->elements) &&
             (res->count == 2) )
        {
            elems  = (int*)res->elements;
            result = d_assert_standalone(
                (elems[0] == 4) &&
                (elems[1] == 6),
                "macro_fluent_pipeline_values",
                "fluent pipeline should return {4, 6}",
                _counter) && result;
        }

        d_filter_result_free(res);
    free(res);
        d_filter_builder_free(builder);
    }

    // test 3: fluent with D_THEN_TAKE_LAST
    builder = D_FILTER_BEGIN();

    if (builder)
    {
        D_THEN_TAKE_LAST(builder, 3);

        res = D_FILTER_END(builder,
                           input,
                           8,
                           sizeof(int));

        result = d_assert_standalone(
            res->count == 3,
            "macro_fluent_take_last_count",
            "D_THEN_TAKE_LAST(3) should produce 3",
            _counter) && result;

        if ( (res->elements) &&
             (res->count == 3) )
        {
            elems  = (int*)res->elements;
            result = d_assert_standalone(
                (elems[0] == 6) &&
                (elems[1] == 7) &&
                (elems[2] == 8),
                "macro_fluent_take_last_values",
                "take_last(3) should return {6, 7, 8}",
                _counter) && result;
        }

        d_filter_result_free(res);
    free(res);
        d_filter_builder_free(builder);
    }

    // test 4: fluent with D_THEN_SKIP_LAST
    builder = D_FILTER_BEGIN();

    if (builder)
    {
        D_THEN_SKIP_LAST(builder, 3);

        res = D_FILTER_END(builder,
                           input,
                           8,
                           sizeof(int));

        result = d_assert_standalone(
            res->count == 5,
            "macro_fluent_skip_last_count",
            "D_THEN_SKIP_LAST(3) on 8 should produce 5",
            _counter) && result;

        if ( (res->elements) &&
             (res->count == 5) )
        {
            elems  = (int*)res->elements;
            result = d_assert_standalone(
                (elems[0] == 1) &&
                (elems[4] == 5),
                "macro_fluent_skip_last_values",
                "skip_last(3) should return {1,2,3,4,5}",
                _counter) && result;
        }

        d_filter_result_free(res);
    free(res);
        d_filter_builder_free(builder);
    }

    // test 5: fluent with D_THEN_WHERE_NOT
    // where_not(is_even) keeps odd: {1,3,5,7}
    builder = D_FILTER_BEGIN();

    if (builder)
    {
        D_THEN_WHERE_NOT(builder, pred_is_even);

        res = D_FILTER_END(builder,
                           input,
                           8,
                           sizeof(int));

        result = d_assert_standalone(
            res->count == 4,
            "macro_fluent_where_not_count",
            "D_THEN_WHERE_NOT(is_even) on {1..8} should produce 4",
            _counter) && result;

        if ( (res->elements) &&
             (res->count == 4) )
        {
            elems  = (int*)res->elements;
            result = d_assert_standalone(
                (elems[0] == 1) &&
                (elems[1] == 3) &&
                (elems[2] == 5) &&
                (elems[3] == 7),
                "macro_fluent_where_not_values",
                "where_not(even) should return {1, 3, 5, 7}",
                _counter) && result;
        }

        d_filter_result_free(res);
    free(res);
        d_filter_builder_free(builder);
    }

    // test 6: fluent with D_THEN_RANGE
    // range(2, 5) selects indices 2, 3, 4 -> {3, 4, 5}
    builder = D_FILTER_BEGIN();

    if (builder)
    {
        D_THEN_RANGE(builder, 2, 5);

        res = D_FILTER_END(builder,
                           input,
                           8,
                           sizeof(int));

        result = d_assert_standalone(
            res->count == 3,
            "macro_fluent_range_count",
            "D_THEN_RANGE(2,5) should produce 3",
            _counter) && result;

        if ( (res->elements) &&
             (res->count == 3) )
        {
            elems  = (int*)res->elements;
            result = d_assert_standalone(
                (elems[0] == 3) &&
                (elems[1] == 4) &&
                (elems[2] == 5),
                "macro_fluent_range_values",
                "range(2,5) should return {3, 4, 5}",
                _counter) && result;
        }

        d_filter_result_free(res);
    free(res);
        d_filter_builder_free(builder);
    }

    // test 7: fluent with D_THEN_REVERSE
    // take_first(4) -> reverse -> {4, 3, 2, 1}
    builder = D_FILTER_BEGIN();

    if (builder)
    {
        D_THEN_TAKE_FIRST(builder, 4);
        D_THEN_REVERSE(builder);

        res = D_FILTER_END(builder,
                           input,
                           8,
                           sizeof(int));

        result = d_assert_standalone(
            res->count == 4,
            "macro_fluent_reverse_count",
            "take(4)->reverse should produce 4",
            _counter) && result;

        if ( (res->elements) &&
             (res->count == 4) )
        {
            elems  = (int*)res->elements;
            result = d_assert_standalone(
                (elems[0] == 4) &&
                (elems[1] == 3) &&
                (elems[2] == 2) &&
                (elems[3] == 1),
                "macro_fluent_reverse_values",
                "take(4)->reverse should return {4,3,2,1}",
                _counter) && result;
        }

        d_filter_result_free(res);
    free(res);
        d_filter_builder_free(builder);
    }

    // test 8: fluent with D_THEN_DISTINCT
    {
        int dup_input[7];

        dup_input[0] = 1;
        dup_input[1] = 2;
        dup_input[2] = 2;
        dup_input[3] = 3;
        dup_input[4] = 3;
        dup_input[5] = 3;
        dup_input[6] = 4;

        builder = D_FILTER_BEGIN();

        if (builder)
        {
            D_THEN_DISTINCT(builder, cmp_int);

            res = D_FILTER_END(builder,
                               dup_input,
                               7,
                               sizeof(int));

            result = d_assert_standalone(
                res->count == 4,
                "macro_fluent_distinct_count",
                "D_THEN_DISTINCT on {1,2,2,3,3,3,4} should be 4",
                _counter) && result;

            if ( (res->elements) &&
                 (res->count == 4) )
            {
                elems  = (int*)res->elements;
                result = d_assert_standalone(
                    (elems[0] == 1) &&
                    (elems[1] == 2) &&
                    (elems[2] == 3) &&
                    (elems[3] == 4),
                    "macro_fluent_distinct_values",
                    "distinct should return {1, 2, 3, 4}",
                    _counter) && result;
            }

            d_filter_result_free(res);
    free(res);
            d_filter_builder_free(builder);
        }
    }

    // test 9: two independent fluent pipelines coexist
    {
        struct d_filter_builder* builder_a;
        struct d_filter_builder* builder_b;
        struct d_filter_result*  res_a;
        struct d_filter_result*  res_b;

        builder_a = D_FILTER_BEGIN();
        builder_b = D_FILTER_BEGIN();

        if ( (builder_a) &&
             (builder_b) )
        {
            D_THEN_TAKE_FIRST(builder_a, 2);
            D_THEN_TAKE_LAST(builder_b, 2);

            res_a = D_FILTER_END(builder_a,
                                 input,
                                 8,
                                 sizeof(int));

            res_b = D_FILTER_END(builder_b,
                                 input,
                                 8,
                                 sizeof(int));

            result = d_assert_standalone(
                res_a->count == 2,
                "macro_fluent_coexist_a_count",
                "pipeline A (take_first 2) should produce 2",
                _counter) && result;

            result = d_assert_standalone(
                res_b->count == 2,
                "macro_fluent_coexist_b_count",
                "pipeline B (take_last 2) should produce 2",
                _counter) && result;

            if ( (res_a->elements) &&
                 (res_b->elements) &&
                 (res_a->count == 2) &&
                 (res_b->count == 2) )
            {
                result = d_assert_standalone(
                    ((int*)res_a->elements)[0] == 1 &&
                    ((int*)res_a->elements)[1] == 2,
                    "macro_fluent_coexist_a_values",
                    "pipeline A should return {1, 2}",
                    _counter) && result;

                result = d_assert_standalone(
                    ((int*)res_b->elements)[0] == 7 &&
                    ((int*)res_b->elements)[1] == 8,
                    "macro_fluent_coexist_b_values",
                    "pipeline B should return {7, 8}",
                    _counter) && result;
            }

            d_filter_result_free(res_a);
            free(res_a);
            d_filter_result_free(res_b);
            free(res_b);
        }

        d_filter_builder_free(builder_a);
        d_filter_builder_free(builder_b);
    }

    return result;
}


/*
d_tests_sa_filter_macros_all
  Aggregation function that runs all convenience macro tests.
*/
bool
d_tests_sa_filter_macros_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Filter Convenience Macros\n");
    printf("  -------------------------------------\n");

    result = d_tests_sa_filter_macros_single(_counter)  && result;
    result = d_tests_sa_filter_macros_fluent(_counter)   && result;

    return result;
}
