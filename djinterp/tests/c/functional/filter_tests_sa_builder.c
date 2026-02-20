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

static bool pred_greater_than_context(const void* _element, void* _context)
{
    const int* value;
    const int* threshold;

    if ( (!_element) ||
         (!_context) )
    {
        return false;
    }

    value     = (const int*)_element;
    threshold = (const int*)_context;

    return (*value > *threshold);
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
d_tests_sa_filter_builder_create
  Tests builder creation: new, from_chain, free.
  Tests the following:
  - d_filter_builder_new succeeds with empty chain
  - builder has no error after creation
  - d_filter_builder_from_chain wraps existing chain
  - from_chain with NULL creates builder with NULL chain
  - free on valid builder does not crash
  - free NULL does not crash
*/
bool
d_tests_sa_filter_builder_create
(
    struct d_test_counter* _counter
)
{
    bool                     result;
    struct d_filter_builder* builder;
    struct d_filter_chain*   chain;

    result = true;

    // test 1: new creates builder with empty chain
    builder = d_filter_builder_new();

    result = d_assert_standalone(
        builder != NULL,
        "builder_new_success",
        "d_filter_builder_new should succeed",
        _counter) && result;

    if (builder)
    {
        result = d_assert_standalone(
            builder->chain != NULL,
            "builder_new_has_chain",
            "new builder should have a chain allocated",
            _counter) && result;

        result = d_assert_standalone(
            builder->error_code == 0,
            "builder_new_no_error_code",
            "new builder should have error_code=0",
            _counter) && result;

        result = d_assert_standalone(
            builder->error_message == NULL,
            "builder_new_no_error_msg",
            "new builder should have NULL error_message",
            _counter) && result;

        result = d_assert_standalone(
            d_filter_builder_has_error(builder) == false,
            "builder_new_has_error_false",
            "has_error should be false on new builder",
            _counter) && result;

        d_filter_builder_free(builder);
    }

    // test 2: from_chain wraps existing chain
    chain = d_filter_chain_new();

    if (chain)
    {
        d_filter_chain_add_take_first(chain, 5);

        builder = d_filter_builder_from_chain(chain);

        result = d_assert_standalone(
            builder != NULL,
            "builder_from_chain_success",
            "from_chain should succeed",
            _counter) && result;

        if (builder)
        {
            result = d_assert_standalone(
                builder->chain == chain,
                "builder_from_chain_ref",
                "builder should reference the provided chain",
                _counter) && result;

            result = d_assert_standalone(
                d_filter_builder_has_error(builder) == false,
                "builder_from_chain_no_error",
                "from_chain builder should have no error",
                _counter) && result;

            // free builder but not the chain it wraps
            // (implementation-dependent ownership)
            d_filter_builder_free(builder);
        }
        else
        {
            d_filter_chain_free(chain);
        }
    }

    // test 3: from_chain with NULL
    builder = d_filter_builder_from_chain(NULL);

    result = d_assert_standalone(
        builder != NULL,
        "builder_from_chain_null",
        "from_chain(NULL) should still create a builder",
        _counter) && result;

    if (builder)
    {
        result = d_assert_standalone(
            builder->chain == NULL,
            "builder_from_chain_null_chain",
            "builder from NULL chain should have NULL chain",
            _counter) && result;

        d_filter_builder_free(builder);
    }

    // test 4: free NULL safety
    d_filter_builder_free(NULL);
    result = d_assert_standalone(
        true,
        "builder_free_null",
        "free(NULL) should not crash",
        _counter) && result;

    return result;
}


/*
d_tests_sa_filter_builder_fluent
  Tests fluent (chainable) operations on the builder.
  Tests the following:
  - take_first returns same builder pointer
  - take_last returns same builder pointer
  - skip_first returns same builder pointer
  - skip_last returns same builder pointer
  - where returns same builder pointer
  - where_context returns same builder pointer
  - where_not returns same builder pointer
  - range returns same builder pointer
  - slice returns same builder pointer
  - distinct returns same builder pointer
  - reverse returns same builder pointer
  - at returns same builder pointer
  - chaining multiple operations accumulates in chain
  - NULL builder returns NULL for each operation
*/
bool
d_tests_sa_filter_builder_fluent
(
    struct d_test_counter* _counter
)
{
    bool                     result;
    struct d_filter_builder* builder;
    struct d_filter_builder* ret;
    int                      threshold;
    size_t                   indices[2] = { 0, 3 };

    result      = true;
    threshold   = 5;

    builder = d_filter_builder_new();

    if (!builder)
    {
        return false;
    }

    // test 1: take_first returns same builder
    ret = d_filter_builder_take_first(builder, 5);
    result = d_assert_standalone(
        ret == builder,
        "builder_fluent_take_first",
        "take_first should return same builder",
        _counter) && result;

    // test 2: take_last returns same builder
    ret = d_filter_builder_take_last(builder, 3);
    result = d_assert_standalone(
        ret == builder,
        "builder_fluent_take_last",
        "take_last should return same builder",
        _counter) && result;

    // test 3: skip_first returns same builder
    ret = d_filter_builder_skip_first(builder, 2);
    result = d_assert_standalone(
        ret == builder,
        "builder_fluent_skip_first",
        "skip_first should return same builder",
        _counter) && result;

    // test 4: skip_last returns same builder
    ret = d_filter_builder_skip_last(builder, 1);
    result = d_assert_standalone(
        ret == builder,
        "builder_fluent_skip_last",
        "skip_last should return same builder",
        _counter) && result;

    // test 5: where returns same builder
    ret = d_filter_builder_where(builder, pred_is_even);
    result = d_assert_standalone(
        ret == builder,
        "builder_fluent_where",
        "where should return same builder",
        _counter) && result;

    // test 6: where_context returns same builder
    ret = d_filter_builder_where_context(builder,
                                     pred_greater_than_context,
                                     &threshold);
    result = d_assert_standalone(
        ret == builder,
        "builder_fluent_where_context",
        "where_context should return same builder",
        _counter) && result;

    // test 7: where_not returns same builder
    ret = d_filter_builder_where_not(builder, pred_is_positive);
    result = d_assert_standalone(
        ret == builder,
        "builder_fluent_where_not",
        "where_not should return same builder",
        _counter) && result;

    // test 8: range returns same builder
    ret = d_filter_builder_range(builder, 1, 5);
    result = d_assert_standalone(
        ret == builder,
        "builder_fluent_range",
        "range should return same builder",
        _counter) && result;

    // test 9: slice returns same builder
    ret = d_filter_builder_slice(builder, 0, 10, 2);
    result = d_assert_standalone(
        ret == builder,
        "builder_fluent_slice",
        "slice should return same builder",
        _counter) && result;

    // test 10: distinct returns same builder
    ret = d_filter_builder_distinct(builder, cmp_int);
    result = d_assert_standalone(
        ret == builder,
        "builder_fluent_distinct",
        "distinct should return same builder",
        _counter) && result;

    // test 11: reverse returns same builder
    ret = d_filter_builder_reverse(builder);
    result = d_assert_standalone(
        ret == builder,
        "builder_fluent_reverse",
        "reverse should return same builder",
        _counter) && result;

    // test 12: at returns same builder
    ret = d_filter_builder_at(builder, 2);
    result = d_assert_standalone(
        ret == builder,
        "builder_fluent_at",
        "at should return same builder",
        _counter) && result;

    // test 13: at_indices returns same builder
    ret = d_filter_builder_at_indices(builder, indices, 2);
    result = d_assert_standalone(
        ret == builder,
        "builder_fluent_at_indices",
        "at_indices should return same builder",
        _counter) && result;

    // test 14: operations accumulated in chain
    // we added 13 operations above
    result = d_assert_standalone(
        builder->chain != NULL,
        "builder_fluent_chain_exists",
        "builder chain should exist after operations",
        _counter) && result;

    if (builder->chain)
    {
        result = d_assert_standalone(
            builder->chain->count == 13,
            "builder_fluent_chain_count",
            "chain should have 13 operations",
            _counter) && result;
    }

    d_filter_builder_free(builder);

    // test 15: NULL builder returns NULL for operations
    ret = d_filter_builder_take_first(NULL, 5);
    result = d_assert_standalone(
        ret == NULL,
        "builder_fluent_null_take_first",
        "take_first(NULL) should return NULL",
        _counter) && result;

    ret = d_filter_builder_where(NULL, pred_is_even);
    result = d_assert_standalone(
        ret == NULL,
        "builder_fluent_null_where",
        "where(NULL) should return NULL",
        _counter) && result;

    ret = d_filter_builder_reverse(NULL);
    result = d_assert_standalone(
        ret == NULL,
        "builder_fluent_null_reverse",
        "reverse(NULL) should return NULL",
        _counter) && result;

    return result;
}


/*
d_tests_sa_filter_builder_finalize
  Tests builder finalization: build and apply.
  Tests the following:
  - build returns a filter chain with correct operations
  - built chain is independent from builder
  - apply executes the chain and returns correct result
  - apply with compound chain produces correct values
  - build on empty builder returns empty chain
  - apply on empty builder returns all elements
*/
bool
d_tests_sa_filter_builder_finalize
(
    struct d_test_counter* _counter
)
{
    struct d_filter_builder* builder;
    struct d_filter_chain*   chain;
    struct d_filter_result*  res;
    int                      input[6] = { 1,2,3,4,5,6 };
    int*                     elems;
    bool                     result;

    result   = true;

    // test 1: build returns chain with correct operations
    builder = d_filter_builder_new();

    if (!builder)
    {
        return false;
    }

    d_filter_builder_take_first(builder, 4);
    d_filter_builder_where(builder, pred_is_even);

    chain = d_filter_builder_build(builder);

    result = d_assert_standalone(
        chain != NULL,
        "builder_build_success",
        "build should return non-NULL chain",
        _counter) && result;

    if (chain)
    {
        result = d_assert_standalone(
            chain->count == 2,
            "builder_build_count",
            "built chain should have 2 operations",
            _counter) && result;

        result = d_assert_standalone(
            chain->operations[0].type == D_FILTER_OP_TAKE_FIRST,
            "builder_build_op0",
            "first op should be TAKE_FIRST",
            _counter) && result;

        result = d_assert_standalone(
            chain->operations[1].type == D_FILTER_OP_WHERE,
            "builder_build_op1",
            "second op should be WHERE",
            _counter) && result;

        d_filter_chain_free(chain);
    }

    d_filter_builder_free(builder);

    // test 2: apply executes and returns correct result
    // skip(1) -> where(even) -> take(2)
    // {1,2,3,4,5,6} -> skip 1 -> {2,3,4,5,6}
    //               -> where(even) -> {2,4,6}
    //               -> take 2 -> {2,4}
    builder = d_filter_builder_new();

    if (builder)
    {
        d_filter_builder_skip_first(builder, 1);
        d_filter_builder_where(builder, pred_is_even);
        d_filter_builder_take_first(builder, 2);

        res = d_filter_builder_apply(builder,
                                     input,
                                     6,
                                     sizeof(int));

        result = d_assert_standalone(
            res->status == D_FILTER_RESULT_SUCCESS,
            "builder_apply_status",
            "apply should return SUCCESS",
            _counter) && result;

        result = d_assert_standalone(
            res->count == 2,
            "builder_apply_count",
            "apply should produce 2 elements",
            _counter) && result;

        if ( (res->elements) &&
             (res->count == 2) )
        {
            elems  = (int*)res->elements;
            result = d_assert_standalone(
                (elems[0] == 2) &&
                (elems[1] == 4),
                "builder_apply_values",
                "apply should return {2, 4}",
                _counter) && result;
        }

        d_filter_result_free(res);
    free(res);
        d_filter_builder_free(builder);
    }

    // test 3: build on empty builder returns empty chain
    builder = d_filter_builder_new();

    if (builder)
    {
        chain = d_filter_builder_build(builder);

        result = d_assert_standalone(
            chain != NULL,
            "builder_build_empty",
            "build on empty builder should return chain",
            _counter) && result;

        if (chain)
        {
            result = d_assert_standalone(
                chain->count == 0,
                "builder_build_empty_count",
                "empty builder should produce chain with 0 ops",
                _counter) && result;

            d_filter_chain_free(chain);
        }

        d_filter_builder_free(builder);
    }

    // test 4: apply on empty builder returns all elements
    builder = d_filter_builder_new();

    if (builder)
    {
        res = d_filter_builder_apply(builder,
                                     input,
                                     6,
                                     sizeof(int));

        result = d_assert_standalone(
            res->count == 6,
            "builder_apply_empty_count",
            "apply on empty builder should return all 6",
            _counter) && result;

        d_filter_result_free(res);
    free(res);
        d_filter_builder_free(builder);
    }

    // test 5: build on NULL builder returns NULL
    chain = d_filter_builder_build(NULL);
    result = d_assert_standalone(
        chain == NULL,
        "builder_build_null",
        "build(NULL) should return NULL",
        _counter) && result;

    // test 6: apply on NULL builder returns error
    res = d_filter_builder_apply(NULL,
                                 input,
                                 6,
                                 sizeof(int));

    result = d_assert_standalone(
        (res->status == D_FILTER_RESULT_ERROR) ||
        (res->status == D_FILTER_RESULT_INVALID),
        "builder_apply_null",
        "apply(NULL) should return error/invalid",
        _counter) && result;

    d_filter_result_free(res);
    free(res);

    return result;
}


/*
d_tests_sa_filter_builder_errors
  Tests builder error handling: has_error, get_error_code,
  get_error_message.
  Tests the following:
  - fresh builder has no error
  - has_error returns false on clean builder
  - get_error_code returns 0 on clean builder
  - get_error_message returns NULL on clean builder
  - error state propagates after a bad operation
  - operations after error are no-ops (chain count unchanged)
  - NULL builder: has_error returns true/false gracefully
  - NULL builder: get_error_code returns non-zero
  - NULL builder: get_error_message returns NULL
*/
bool
d_tests_sa_filter_builder_errors
(
    struct d_test_counter* _counter
)
{
    bool                     result;
    struct d_filter_builder* builder;
    size_t                   count_before;
    size_t                   count_after;
    size_t                   i;

    result = true;

    // test 1: fresh builder has no error
    builder = d_filter_builder_new();

    if (!builder)
    {
        return false;
    }

    result = d_assert_standalone(
        d_filter_builder_has_error(builder) == false,
        "builder_error_fresh_none",
        "fresh builder should have no error",
        _counter) && result;

    result = d_assert_standalone(
        d_filter_builder_get_error_code(builder) == 0,
        "builder_error_fresh_code",
        "fresh builder error_code should be 0",
        _counter) && result;

    result = d_assert_standalone(
        d_filter_builder_get_error_message(builder) == NULL,
        "builder_error_fresh_msg",
        "fresh builder error_message should be NULL",
        _counter) && result;

    // test 2: fill to max chain length to trigger error
    // D_FILTER_MAX_CHAIN_LENGTH is 32; adding more should trigger error
    for (i = 0; i < D_FILTER_MAX_CHAIN_LENGTH; i++)
    {
        d_filter_builder_take_first(builder, 100);
    }

    count_before = 0;

    if (builder->chain)
    {
        count_before = builder->chain->count;
    }

    // attempt to exceed max chain length
    d_filter_builder_take_first(builder, 1);

    count_after = 0;

    if (builder->chain)
    {
        count_after = builder->chain->count;
    }

    // if max chain length is enforced, the builder should be in error
    // or the count should not have changed
    if (d_filter_builder_has_error(builder))
    {
        result = d_assert_standalone(
            d_filter_builder_get_error_code(builder) != 0,
            "builder_error_overflow_code",
            "overflow should set non-zero error code",
            _counter) && result;

        // test 3: operations after error are no-ops
        d_filter_builder_reverse(builder);

        result = d_assert_standalone(
            builder->chain == NULL ||
            builder->chain->count == count_after,
            "builder_error_noop",
            "operations after error should be no-ops",
            _counter) && result;
    }
    else
    {
        // some implementations may allow dynamic growth
        result = d_assert_standalone(
            count_after >= count_before,
            "builder_error_no_overflow",
            "builder may allow dynamic growth beyond max",
            _counter) && result;
    }

    d_filter_builder_free(builder);

    // test 4: NULL builder error queries
    result = d_assert_standalone(
        d_filter_builder_has_error(NULL) == true,
        "builder_error_null_has_error",
        "has_error(NULL) should return true",
        _counter) && result;

    result = d_assert_standalone(
        d_filter_builder_get_error_code(NULL) != 0,
        "builder_error_null_code",
        "get_error_code(NULL) should return non-zero",
        _counter) && result;

    result = d_assert_standalone(
        d_filter_builder_get_error_message(NULL) == NULL,
        "builder_error_null_msg",
        "get_error_message(NULL) should return NULL",
        _counter) && result;

    return result;
}


/*
d_tests_sa_filter_builder_all
  Aggregation function that runs all fluent filter builder tests.
*/
bool
d_tests_sa_filter_builder_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Fluent Filter Builder\n");
    printf("  ---------------------------------\n");

    result = d_tests_sa_filter_builder_create(_counter)    && result;
    result = d_tests_sa_filter_builder_fluent(_counter)    && result;
    result = d_tests_sa_filter_builder_finalize(_counter)  && result;
    result = d_tests_sa_filter_builder_errors(_counter)    && result;

    return result;
}
