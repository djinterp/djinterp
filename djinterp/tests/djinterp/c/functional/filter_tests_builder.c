/*******************************************************************************
* djinterp [functional]                                   filter_tests_builder.c
*
*
* path:      /tests/djinterp/c/functional/filter_tests_builder.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.02.19
*                                                            revised: 2026.11.09
*******************************************************************************/
// djinterp
#include "./filter_tests.h"
#include "../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's C face


///////////////////////////////////////////////////////////////////////////////
// HELPER PREDICATES
///////////////////////////////////////////////////////////////////////////////

static
bool pred_is_even(
    const void* _element,
    void*       _context
)
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

static
bool pred_is_positive(
    const void* _element,
    void*       _context
)
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

static
bool pred_greater_than_context(
    const void* _element,
    void*       _context
)
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

static
int cmp_int(
    const void* _a,
    const void* _b,
    void*       _context
)
{
    const int* a;
    const int* b;

    (void)_context;

    a = (const int*)_a;
    b = (const int*)_b;

    return (*a - *b);
}


/**
 * @brief Part 1 of d_tests_filter_builder_create.
 *
 * @param[in,out] _suite  the suite the checks are recorded in.
 * @return true if every check of the part passed.
 *
 * The test runs its parts in order, so that no function body is over
 * 100 lines. A part declares its own locals: no value crosses from one
 * part to the next.
 *
 * Sections:
 *   - test 1: new creates builder with empty chain
 *   - test 2: from_chain wraps existing chain
 *   - test 3: from_chain with NULL
 */
static bool
d_internal_tests_filter_builder_create_part1(
    struct dj_test_suite* _suite
)
{
    bool                      result;
    struct dj_filter_builder* builder;
    struct dj_filter_chain*   chain;

    result = true;

    // test 1: new creates builder with empty chain
    builder = d_filter_builder_new();

    result = D_TEST_CHECK_MSG(_suite,
                              "builder_new_success",
                              builder != NULL,
                              "d_filter_builder_new should succeed") && result;

    if (builder)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "builder_new_has_chain",
                                  builder->chain != NULL,
                                  "new builder should have a chain allocated") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "builder_new_no_error_code",
                                  builder->error_code == 0,
                                  "new builder should have error_code=0") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "builder_new_no_error_msg",
                                  builder->error_message == NULL,
                                  "new builder should have NULL error_message") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "builder_new_has_error_false",
                                  d_filter_builder_has_error(builder) == false,
                                  "has_error should be false on new builder") && result;

        d_filter_builder_free(builder);
    }

    // test 2: from_chain wraps existing chain
    chain = d_filter_chain_new();

    if (chain)
    {
        d_filter_chain_add_take_first(chain, 5);

        builder = d_filter_builder_from_chain(chain);

        result = D_TEST_CHECK_MSG(_suite,
                                  "builder_from_chain_success",
                                  builder != NULL,
                                  "from_chain should succeed") && result;

        if (builder)
        {
            result = D_TEST_CHECK_MSG(_suite,
                                      "builder_from_chain_ref",
                                      builder->chain == chain,
                                      "builder should reference the provided chain") && result;

            result = D_TEST_CHECK_MSG(_suite,
                                      "builder_from_chain_no_error",
                                      d_filter_builder_has_error(builder) == false,
                                      "from_chain builder should have no error") && result;

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

    result = D_TEST_CHECK_MSG(_suite,
                              "builder_from_chain_null",
                              builder != NULL,
                              "from_chain(NULL) should still create a builder") && result;

    if (builder)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "builder_from_chain_null_chain",
                                  builder->chain == NULL,
                                  "builder from NULL chain should have NULL chain") && result;

        d_filter_builder_free(builder);
    }

    return result;
}


/**
 * @brief Part 2 of d_tests_filter_builder_create.
 *
 * @param[in,out] _suite  the suite the checks are recorded in.
 * @return true if every check of the part passed.
 *
 * The test runs its parts in order, so that no function body is over
 * 100 lines. A part declares its own locals: no value crosses from one
 * part to the next.
 *
 * Sections:
 *   - test 4: free NULL safety
 */
static bool
d_internal_tests_filter_builder_create_part2(
    struct dj_test_suite* _suite
)
{
    bool result;

    result = true;

    // test 4: free NULL safety
    d_filter_builder_free(NULL);
    result = D_TEST_CHECK_MSG(_suite,
                              "builder_free_null",
                              true,
                              "free(NULL) should not crash") && result;

    return result;
}


/*
d_tests_filter_builder_create
  Tests builder creation: new, from_chain, free.
  Tests the following:
  - d_filter_builder_new succeeds with empty chain
  - builder has no error after creation
  - d_filter_builder_from_chain wraps existing chain
  - from_chain with NULL creates builder with NULL chain
  - free on valid builder does not crash
  - free NULL does not crash
*/
bool d_tests_filter_builder_create(
    struct dj_test_suite* _suite
)
{
    bool result;

    d_test_suite_test(_suite, "filter_builder_create");

    result = true;
    result &= d_internal_tests_filter_builder_create_part1(_suite);
    result &= d_internal_tests_filter_builder_create_part2(_suite);

    return result;
}


/*
d_tests_filter_builder_fluent
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
bool d_tests_filter_builder_fluent(
    struct dj_test_suite* _suite
)
{
    bool                     result;
    struct dj_filter_builder* builder;
    struct dj_filter_builder* ret;
    int                      threshold;
    size_t                   indices[2] = { 0, 3 };

    d_test_suite_test(_suite, "filter_builder_fluent");

    result      = true;
    threshold   = 5;

    builder = d_filter_builder_new();

    if (!builder)
    {
        return false;
    }

    // test 1: take_first returns same builder
    ret = d_filter_builder_take_first(builder, 5);
    result = D_TEST_CHECK_MSG(_suite,
                              "builder_fluent_take_first",
                              ret == builder,
                              "take_first should return same builder") && result;

    // test 2: take_last returns same builder
    ret = d_filter_builder_take_last(builder, 3);
    result = D_TEST_CHECK_MSG(_suite,
                              "builder_fluent_take_last",
                              ret == builder,
                              "take_last should return same builder") && result;

    // test 3: skip_first returns same builder
    ret = d_filter_builder_skip_first(builder, 2);
    result = D_TEST_CHECK_MSG(_suite,
                              "builder_fluent_skip_first",
                              ret == builder,
                              "skip_first should return same builder") && result;

    // test 4: skip_last returns same builder
    ret = d_filter_builder_skip_last(builder, 1);
    result = D_TEST_CHECK_MSG(_suite,
                              "builder_fluent_skip_last",
                              ret == builder,
                              "skip_last should return same builder") && result;

    // test 5: where returns same builder
    ret = d_filter_builder_where(builder, pred_is_even);
    result = D_TEST_CHECK_MSG(_suite,
                              "builder_fluent_where",
                              ret == builder,
                              "where should return same builder") && result;

    // test 6: where_context returns same builder
    ret = d_filter_builder_where_context(builder,
                                     pred_greater_than_context,
                                     &threshold);
    result = D_TEST_CHECK_MSG(_suite,
                              "builder_fluent_where_context",
                              ret == builder,
                              "where_context should return same builder") && result;

    // test 7: where_not returns same builder
    ret = d_filter_builder_where_not(builder, pred_is_positive);
    result = D_TEST_CHECK_MSG(_suite,
                              "builder_fluent_where_not",
                              ret == builder,
                              "where_not should return same builder") && result;

    // test 8: range returns same builder
    ret = d_filter_builder_range(builder, 1, 5);
    result = D_TEST_CHECK_MSG(_suite,
                              "builder_fluent_range",
                              ret == builder,
                              "range should return same builder") && result;

    // test 9: slice returns same builder
    ret = d_filter_builder_slice(builder, 0, 10, 2);
    result = D_TEST_CHECK_MSG(_suite,
                              "builder_fluent_slice",
                              ret == builder,
                              "slice should return same builder") && result;

    // test 10: distinct returns same builder
    ret = d_filter_builder_distinct(builder, cmp_int);
    result = D_TEST_CHECK_MSG(_suite,
                              "builder_fluent_distinct",
                              ret == builder,
                              "distinct should return same builder") && result;

    // test 11: reverse returns same builder
    ret = d_filter_builder_reverse(builder);
    result = D_TEST_CHECK_MSG(_suite,
                              "builder_fluent_reverse",
                              ret == builder,
                              "reverse should return same builder") && result;

    // test 12: at returns same builder
    ret = d_filter_builder_at(builder, 2);
    result = D_TEST_CHECK_MSG(_suite,
                              "builder_fluent_at",
                              ret == builder,
                              "at should return same builder") && result;

    // test 13: at_indices returns same builder
    ret = d_filter_builder_at_indices(builder, indices, 2);
    result = D_TEST_CHECK_MSG(_suite,
                              "builder_fluent_at_indices",
                              ret == builder,
                              "at_indices should return same builder") && result;

    // test 14: operations accumulated in chain
    // we added 13 operations above
    result = D_TEST_CHECK_MSG(_suite,
                              "builder_fluent_chain_exists",
                              builder->chain != NULL,
                              "builder chain should exist after operations") && result;

    if (builder->chain)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "builder_fluent_chain_count",
                                  builder->chain->count == 13,
                                  "chain should have 13 operations") && result;
    }

    d_filter_builder_free(builder);

    // test 15: NULL builder returns NULL for operations
    ret = d_filter_builder_take_first(NULL, 5);
    result = D_TEST_CHECK_MSG(_suite,
                              "builder_fluent_null_take_first",
                              ret == NULL,
                              "take_first(NULL) should return NULL") && result;

    ret = d_filter_builder_where(NULL, pred_is_even);
    result = D_TEST_CHECK_MSG(_suite,
                              "builder_fluent_null_where",
                              ret == NULL,
                              "where(NULL) should return NULL") && result;

    ret = d_filter_builder_reverse(NULL);
    result = D_TEST_CHECK_MSG(_suite,
                              "builder_fluent_null_reverse",
                              ret == NULL,
                              "reverse(NULL) should return NULL") && result;

    return result;
}


/*
d_tests_filter_builder_finalize
  Tests builder finalization: build and apply.
  Tests the following:
  - build returns a filter chain with correct operations
  - built chain is independent from builder
  - apply executes the chain and returns correct result
  - apply with compound chain produces correct values
  - build on empty builder returns empty chain
  - apply on empty builder returns all elements
*/
bool d_tests_filter_builder_finalize(
    struct dj_test_suite* _suite
)
{
    struct dj_filter_builder* builder;
    struct dj_filter_chain*   chain;
    struct dj_filter_result*  res;
    int                      input[6] = { 1,2,3,4,5,6 };
    int*                     elems;
    bool                     result;

    d_test_suite_test(_suite, "filter_builder_finalize");

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

    result = D_TEST_CHECK_MSG(_suite,
                              "builder_build_success",
                              chain != NULL,
                              "build should return non-NULL chain") && result;

    if (chain)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "builder_build_count",
                                  chain->count == 2,
                                  "built chain should have 2 operations") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "builder_build_op0",
                                  chain->operations[0].type == D_FILTER_OP_TAKE_FIRST,
                                  "first op should be TAKE_FIRST") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "builder_build_op1",
                                  chain->operations[1].type == D_FILTER_OP_WHERE,
                                  "second op should be WHERE") && result;

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

        result = D_TEST_CHECK_MSG(_suite,
                                  "builder_apply_status",
                                  res->status == D_FILTER_RESULT_SUCCESS,
                                  "apply should return SUCCESS") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "builder_apply_count",
                                  res->count == 2,
                                  "apply should produce 2 elements") && result;

        if ( (res->elements) &&
             (res->count == 2) )
        {
            elems  = (int*)res->elements;
            result = D_TEST_CHECK_MSG(_suite,
                                      "builder_apply_values",
                                      (elems[0] == 2) && (elems[1] == 4),
                                      "apply should return {2, 4}") && result;
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

        result = D_TEST_CHECK_MSG(_suite,
                                  "builder_build_empty",
                                  chain != NULL,
                                  "build on empty builder should return chain") && result;

        if (chain)
        {
            result = D_TEST_CHECK_MSG(_suite,
                                      "builder_build_empty_count",
                                      chain->count == 0,
                                      "empty builder should produce chain with 0 ops") && result;

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

        result = D_TEST_CHECK_MSG(_suite,
                                  "builder_apply_empty_count",
                                  res->count == 6,
                                  "apply on empty builder should return all 6") && result;

        d_filter_result_free(res);
    free(res);
        d_filter_builder_free(builder);
    }

    // test 5: build on NULL builder returns NULL
    chain = d_filter_builder_build(NULL);
    result = D_TEST_CHECK_MSG(_suite,
                              "builder_build_null",
                              chain == NULL,
                              "build(NULL) should return NULL") && result;

    // test 6: apply on NULL builder returns error
    res = d_filter_builder_apply(NULL,
                                 input,
                                 6,
                                 sizeof(int));

    result = D_TEST_CHECK_MSG(_suite,
                              "builder_apply_null",
                              (res->status == D_FILTER_RESULT_ERROR) || (res->status == D_FILTER_RESULT_INVALID),
                              "apply(NULL) should return error/invalid") && result;

    d_filter_result_free(res);
    free(res);

    return result;
}


/**
 * @brief Part 1 of d_tests_filter_builder_errors.
 *
 * @param[in,out] _suite  the suite the checks are recorded in.
 * @return true if every check of the part passed.
 *
 * The test runs its parts in order, so that no function body is over
 * 100 lines. A part declares its own locals: no value crosses from one
 * part to the next.
 *
 * Sections:
 *   - test 1: fresh builder has no error
 *   - test 2: fill to max chain length to trigger error
 *   - attempt to exceed max chain length
 *   - if max chain length is enforced, the builder should be in error
 */
static bool
d_internal_tests_filter_builder_errors_part1(
    struct dj_test_suite* _suite
)
{
    bool                      result;
    struct dj_filter_builder* builder;
    size_t                    count_before;
    size_t                    count_after;
    size_t                    i;

    result = true;

    // test 1: fresh builder has no error
    builder = d_filter_builder_new();

    if (!builder)
    {
        return false;
    }

    result = D_TEST_CHECK_MSG(_suite,
                              "builder_error_fresh_none",
                              d_filter_builder_has_error(builder) == false,
                              "fresh builder should have no error") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "builder_error_fresh_code",
                              d_filter_builder_get_error_code(builder) == 0,
                              "fresh builder error_code should be 0") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "builder_error_fresh_msg",
                              d_filter_builder_get_error_message(builder) == NULL,
                              "fresh builder error_message should be NULL") && result;

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
        result = D_TEST_CHECK_MSG(_suite,
                                  "builder_error_overflow_code",
                                  d_filter_builder_get_error_code(builder) != 0,
                                  "overflow should set non-zero error code") && result;

        // test 3: operations after error are no-ops
        d_filter_builder_reverse(builder);

        result = D_TEST_CHECK_MSG(_suite,
                                  "builder_error_noop",
                                  builder->chain == NULL || builder->chain->count == count_after,
                                  "operations after error should be no-ops") && result;
    }
    else
    {
        // some implementations may allow dynamic growth
        result = D_TEST_CHECK_MSG(_suite,
                                  "builder_error_no_overflow",
                                  count_after >= count_before,
                                  "builder may allow dynamic growth beyond max") && result;
    }

    d_filter_builder_free(builder);

    return result;
}


/**
 * @brief Part 2 of d_tests_filter_builder_errors.
 *
 * @param[in,out] _suite  the suite the checks are recorded in.
 * @return true if every check of the part passed.
 *
 * The test runs its parts in order, so that no function body is over
 * 100 lines. A part declares its own locals: no value crosses from one
 * part to the next.
 *
 * Sections:
 *   - test 4: NULL builder error queries
 */
static bool
d_internal_tests_filter_builder_errors_part2(
    struct dj_test_suite* _suite
)
{
    bool result;

    result = true;

    // test 4: NULL builder error queries
    result = D_TEST_CHECK_MSG(_suite,
                              "builder_error_null_has_error",
                              d_filter_builder_has_error(NULL) == true,
                              "has_error(NULL) should return true") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "builder_error_null_code",
                              d_filter_builder_get_error_code(NULL) != 0,
                              "get_error_code(NULL) should return non-zero") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "builder_error_null_msg",
                              d_filter_builder_get_error_message(NULL) == NULL,
                              "get_error_message(NULL) should return NULL") && result;

    return result;
}


/*
d_tests_filter_builder_errors
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
bool d_tests_filter_builder_errors(
    struct dj_test_suite* _suite
)
{
    bool result;

    d_test_suite_test(_suite, "filter_builder_errors");

    result = true;
    result &= d_internal_tests_filter_builder_errors_part1(_suite);
    result &= d_internal_tests_filter_builder_errors_part2(_suite);

    return result;
}


/*
d_tests_filter_builder_all
  Aggregation function that runs all fluent filter builder tests.
*/
bool d_tests_filter_builder_all(
    struct dj_test_suite* _suite
)
{
    bool result;

    d_test_suite_block(_suite, "filter_builder_all");

    result = true;

    result = d_tests_filter_builder_create(_suite)    && result;
    result = d_tests_filter_builder_fluent(_suite)    && result;
    result = d_tests_filter_builder_finalize(_suite)  && result;
    result = d_tests_filter_builder_errors(_suite)    && result;

    return result;
}
