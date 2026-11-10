/*******************************************************************************
* djinterp [functional]                                     filter_tests_chain.c
*
*
* path:      /tests/djinterp/c/functional/filter_tests_chain.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.02.19
*                                                            revised: 2026.11.09
*******************************************************************************/
// djinterp
#include "./filter_tests.h"
#include "../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's C face


// helper predicate for chain tests
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


/**
 * @brief Part 1 of d_tests_filter_chain_create.
 *
 * @param[in,out] _suite  the suite the checks are recorded in.
 * @return true if every check of the part passed.
 *
 * The test runs its parts in order, so that no function body is over
 * 100 lines. A part declares its own locals: no value crosses from one
 * part to the next.
 *
 * Sections:
 *   - test 1: new creates empty chain
 *   - test 2: new_with_capacity pre-allocates
 *   - test 3: new_with_capacity(0) same as new()
 */
static bool
d_internal_tests_filter_chain_create_part1(
    struct dj_test_suite* _suite
)
{
    bool                    result;
    struct dj_filter_chain* chain;

    result = true;

    // test 1: new creates empty chain
    chain  = d_filter_chain_new();
    result = D_TEST_CHECK_MSG(_suite,
                              "chain_new_success",
                              chain != NULL,
                              "d_filter_chain_new should succeed") && result;

    if (chain)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "chain_new_empty_count",
                                  chain->count == 0,
                                  "new chain should have count=0") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "chain_new_empty_capacity",
                                  chain->capacity == 0,
                                  "new chain should have capacity=0") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "chain_new_null_ops",
                                  chain->operations == NULL,
                                  "new chain should have NULL operations") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "chain_new_owns_ops",
                                  chain->owns_operations == true,
                                  "new chain should own its operations") && result;

        d_filter_chain_free(chain);
    }

    // test 2: new_with_capacity pre-allocates
    chain  = d_filter_chain_new_with_capacity(8);
    result = D_TEST_CHECK_MSG(_suite,
                              "chain_capacity_success",
                              chain != NULL,
                              "d_filter_chain_new_with_capacity(8) should succeed") && result;

    if (chain)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "chain_capacity_set",
                                  chain->capacity == 8,
                                  "chain should have capacity=8") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "chain_capacity_count_zero",
                                  chain->count == 0,
                                  "chain should still have count=0") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "chain_capacity_ops_allocated",
                                  chain->operations != NULL,
                                  "chain should have operations allocated") && result;

        d_filter_chain_free(chain);
    }

    // test 3: new_with_capacity(0) same as new()
    chain  = d_filter_chain_new_with_capacity(0);
    result = D_TEST_CHECK_MSG(_suite,
                              "chain_capacity_zero",
                              chain != NULL,
                              "new_with_capacity(0) should succeed") && result;

    if (chain)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "chain_capacity_zero_empty",
                                  chain->capacity == 0 && chain->operations == NULL,
                                  "capacity(0) should be same as new()") && result;

        d_filter_chain_free(chain);
    }

    return result;
}


/**
 * @brief Part 2 of d_tests_filter_chain_create.
 *
 * @param[in,out] _suite  the suite the checks are recorded in.
 * @return true if every check of the part passed.
 *
 * The test runs its parts in order, so that no function body is over
 * 100 lines. A part declares its own locals: no value crosses from one
 * part to the next.
 *
 * Sections:
 *   - test 4: clone preserves operations
 *   - test 5: clone NULL returns NULL
 *   - test 6: free NULL safety
 */
static bool
d_internal_tests_filter_chain_create_part2(
    struct dj_test_suite* _suite
)
{
    bool                        result;
    struct dj_filter_chain*     chain;
    struct dj_filter_chain*     clone;
    struct dj_filter_operation* op;

    op     = NULL;
    result = true;

    // test 4: clone preserves operations
    chain = d_filter_chain_new();

    if (chain)
    {
        op = d_filter_take_first(3);
        d_filter_chain_add(chain, op);

        free(op);
        op = d_filter_reverse();
        d_filter_chain_add(chain, op);

        clone  = d_filter_chain_clone(chain);
        result = D_TEST_CHECK_MSG(_suite,
                                  "chain_clone_success",
                                  clone != NULL,
                                  "clone should succeed") && result;

        if (clone)
        {
            result = D_TEST_CHECK_MSG(_suite,
                                      "chain_clone_count",
                                      clone->count == 2,
                                      "clone should have same count") && result;

            result = D_TEST_CHECK_MSG(_suite,
                                      "chain_clone_op0",
                                      clone->operations[0].type == D_FILTER_OP_TAKE_FIRST,
                                      "clone op[0] should be TAKE_FIRST") && result;

            result = D_TEST_CHECK_MSG(_suite,
                                      "chain_clone_op1",
                                      clone->operations[1].type == D_FILTER_OP_REVERSE,
                                      "clone op[1] should be REVERSE") && result;

            result = D_TEST_CHECK_MSG(_suite,
                                      "chain_clone_independent",
                                      clone->operations != chain->operations,
                                      "clone operations array should be independent") && result;

            d_filter_chain_free(clone);
        }

        d_filter_chain_free(chain);
    }

    // test 5: clone NULL returns NULL
    clone  = d_filter_chain_clone(NULL);
    result = D_TEST_CHECK_MSG(_suite,
                              "chain_clone_null",
                              clone == NULL,
                              "clone(NULL) should return NULL") && result;

    // test 6: free NULL safety
    d_filter_chain_free(NULL);
    result = D_TEST_CHECK_MSG(_suite,
                              "chain_free_null",
                              true,
                              "free(NULL) should not crash") && result;

    free(op);

    return result;
}


/*
d_tests_filter_chain_create
  Tests chain creation functions: new, new_with_capacity, clone, free.
  Tests the following:
  - default creation (NULL operations, zero count/capacity)
  - capacity pre-allocation
  - cloning preserves operations
  - cloning NULL returns NULL
  - free on empty chain
  - free on populated chain
  - free NULL safety
*/
bool d_tests_filter_chain_create(
    struct dj_test_suite* _suite
)
{
    bool result;

    d_test_suite_test(_suite, "filter_chain_create");

    result = true;
    result &= d_internal_tests_filter_chain_create_part1(_suite);
    result &= d_internal_tests_filter_chain_create_part2(_suite);

    return result;
}


/*
d_tests_filter_chain_add
  Tests the generic d_filter_chain_add function.
  Tests the following:
  - adding first operation grows capacity
  - adding multiple operations
  - NULL chain rejection
  - NULL operation rejection
  - capacity growth (dynamic reallocation)
*/
bool d_tests_filter_chain_add(
    struct dj_test_suite* _suite
)
{
    bool                       result;
    struct dj_filter_chain*     chain;
    struct dj_filter_operation* op;
    size_t                     i;
    bool                       add_ok;

    d_test_suite_test(_suite, "filter_chain_add");
    op = NULL;

    result = true;

    chain = d_filter_chain_new();

    if (!chain)
    {
        return false;
    }

    // test 1: add first operation triggers allocation
    op     = d_filter_take_first(5);
    add_ok = d_filter_chain_add(chain, op);
    result = D_TEST_CHECK_MSG(_suite,
                              "chain_add_first",
                              add_ok == true,
                              "adding first operation should succeed") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "chain_add_first_count",
                              chain->count == 1,
                              "count should be 1 after first add") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "chain_add_first_capacity",
                              chain->capacity >= 1,
                              "capacity should grow on first add") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "chain_add_first_stored",
                              chain->operations[0].type == D_FILTER_OP_TAKE_FIRST,
                              "first operation should be stored correctly") && result;

    // test 2: add many operations to trigger reallocation
    for (i = 1; i < 10; i++)
    {
        free(op);
        op = d_filter_skip_first(i);
        d_filter_chain_add(chain, op);
    }

    result = D_TEST_CHECK_MSG(_suite,
                              "chain_add_many_count",
                              chain->count == 10,
                              "count should be 10 after 10 adds") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "chain_add_many_capacity",
                              chain->capacity >= 10,
                              "capacity should accommodate 10 operations") && result;

    d_filter_chain_free(chain);

    // test 3: NULL chain rejection
    free(op);
    op     = d_filter_reverse();
    add_ok = d_filter_chain_add(NULL, op);
    result = D_TEST_CHECK_MSG(_suite,
                              "chain_add_null_chain",
                              add_ok == false,
                              "add to NULL chain should return false") && result;

    // test 4: NULL operation rejection
    chain  = d_filter_chain_new();
    add_ok = d_filter_chain_add(chain, NULL);
    result = D_TEST_CHECK_MSG(_suite,
                              "chain_add_null_op",
                              add_ok == false,
                              "add NULL operation should return false") && result;

    d_filter_chain_free(chain);

    free(op);
    return result;
}


/*
d_tests_filter_chain_convenience
  Tests convenience chain-add helpers: add_take_first, add_take_last,
add_skip_first, add_skip_last, add_range, add_where, add_where_context.
  Tests the following:
  - each convenience function creates and adds the correct op type
  - NULL chain handling
*/
bool d_tests_filter_chain_convenience(
    struct dj_test_suite* _suite
)
{
    bool                   result;
    struct dj_filter_chain* chain;
    int                    threshold;

    d_test_suite_test(_suite, "filter_chain_convenience");

    result    = true;
    threshold = 5;

    chain = d_filter_chain_new();

    if (!chain)
    {
        return false;
    }

    // test 1: add_take_first
    result = D_TEST_CHECK_MSG(_suite,
                              "chain_conv_take_first",
                              d_filter_chain_add_take_first(chain, 3) == true,
                              "add_take_first should succeed") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "chain_conv_take_first_stored",
                              chain->operations[0].type == D_FILTER_OP_TAKE_FIRST && chain->operations[0].params.count == 3,
                              "take_first(3) should be stored correctly") && result;

    // test 2: add_take_last
    result = D_TEST_CHECK_MSG(_suite,
                              "chain_conv_take_last",
                              d_filter_chain_add_take_last(chain, 2) == true,
                              "add_take_last should succeed") && result;

    // test 3: add_skip_first
    result = D_TEST_CHECK_MSG(_suite,
                              "chain_conv_skip_first",
                              d_filter_chain_add_skip_first(chain, 1) == true,
                              "add_skip_first should succeed") && result;

    // test 4: add_skip_last
    result = D_TEST_CHECK_MSG(_suite,
                              "chain_conv_skip_last",
                              d_filter_chain_add_skip_last(chain, 1) == true,
                              "add_skip_last should succeed") && result;

    // test 5: add_range
    result = D_TEST_CHECK_MSG(_suite,
                              "chain_conv_range",
                              d_filter_chain_add_range(chain, 0, 5) == true,
                              "add_range should succeed") && result;

    // test 6: add_where
    result = D_TEST_CHECK_MSG(_suite,
                              "chain_conv_where",
                              d_filter_chain_add_where(chain, pred_is_even) == true,
                              "add_where should succeed") && result;

    // test 7: add_where_context
    result = D_TEST_CHECK_MSG(_suite,
                              "chain_conv_where_context",
                              d_filter_chain_add_where_context(chain, pred_is_positive, &threshold) == true,
                              "add_where_context should succeed") && result;

    // verify total count
    result = D_TEST_CHECK_MSG(_suite,
                              "chain_conv_total_count",
                              chain->count == 7,
                              "chain should have 7 operations") && result;

    d_filter_chain_free(chain);

    // test 8: NULL chain rejection for all convenience functions
    result = D_TEST_CHECK_MSG(_suite,
                              "chain_conv_null_take_first",
                              d_filter_chain_add_take_first(NULL, 1) == false,
                              "add_take_first(NULL) should return false") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "chain_conv_null_where",
                              d_filter_chain_add_where(NULL, pred_is_even) == false,
                              "add_where(NULL) should return false") && result;

    return result;
}


/*
d_tests_filter_chain_combine
  Tests chain combination functions: concat, append.
  Tests the following:
  - concat creates new chain with all operations from both
  - concat preserves operation order
  - concat with NULL returns NULL
  - append adds all operations from source to dest
  - append with NULL returns false
*/
bool d_tests_filter_chain_combine(
    struct dj_test_suite* _suite
)
{
    bool                       result;
    struct dj_filter_chain*     chain_a;
    struct dj_filter_chain*     chain_b;
    struct dj_filter_chain*     combined;
    struct dj_filter_operation* op;

    d_test_suite_test(_suite, "filter_chain_combine");
    op = NULL;

    result = true;

    // build two chains
    chain_a = d_filter_chain_new();
    chain_b = d_filter_chain_new();

    if ( (!chain_a) ||
         (!chain_b) )
    {
        d_filter_chain_free(chain_a);
        d_filter_chain_free(chain_b);

        return false;
    }

    op = d_filter_take_first(3);
    d_filter_chain_add(chain_a, op);

    free(op);
    op = d_filter_reverse();
    d_filter_chain_add(chain_a, op);

    free(op);
    op = d_filter_skip_first(1);
    d_filter_chain_add(chain_b, op);

    // test 1: concat creates combined chain
    combined = d_filter_chain_concat(chain_a, chain_b);
    result   = D_TEST_CHECK_MSG(_suite,
                                "chain_concat_success",
                                combined != NULL,
                                "concat should succeed") && result;

    if (combined)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "chain_concat_count",
                                  combined->count == 3,
                                  "concat should have 3 operations (2+1)") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "chain_concat_order_0",
                                  combined->operations[0].type == D_FILTER_OP_TAKE_FIRST,
                                  "concat op[0] should be TAKE_FIRST from chain_a") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "chain_concat_order_1",
                                  combined->operations[1].type == D_FILTER_OP_REVERSE,
                                  "concat op[1] should be REVERSE from chain_a") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "chain_concat_order_2",
                                  combined->operations[2].type == D_FILTER_OP_SKIP_FIRST,
                                  "concat op[2] should be SKIP_FIRST from chain_b") && result;

        d_filter_chain_free(combined);
    }

    // test 2: concat with NULL returns NULL
    result = D_TEST_CHECK_MSG(_suite,
                              "chain_concat_null_first",
                              d_filter_chain_concat(NULL, chain_b) == NULL,
                              "concat(NULL, chain) should return NULL") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "chain_concat_null_second",
                              d_filter_chain_concat(chain_a, NULL) == NULL,
                              "concat(chain, NULL) should return NULL") && result;

    // test 3: append adds to dest
    result = D_TEST_CHECK_MSG(_suite,
                              "chain_append_pre_count",
                              chain_a->count == 2,
                              "chain_a should have 2 before append") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "chain_append_success",
                              d_filter_chain_append(chain_a, chain_b) == true,
                              "append should succeed") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "chain_append_post_count",
                              chain_a->count == 3,
                              "chain_a should have 3 after append") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "chain_append_op_added",
                              chain_a->operations[2].type == D_FILTER_OP_SKIP_FIRST,
                              "appended operation should be at end") && result;

    // test 4: append with NULL returns false
    result = D_TEST_CHECK_MSG(_suite,
                              "chain_append_null_dest",
                              d_filter_chain_append(NULL, chain_b) == false,
                              "append(NULL, chain) should return false") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "chain_append_null_source",
                              d_filter_chain_append(chain_a, NULL) == false,
                              "append(chain, NULL) should return false") && result;

    d_filter_chain_free(chain_a);
    d_filter_chain_free(chain_b);

    free(op);
    return result;
}


/*
d_tests_filter_chain_manipulate
  Tests chain manipulation: insert, remove, clear.
  Tests the following:
  - insert at beginning, middle, end
  - insert out of bounds rejected
  - insert with NULL rejected
  - remove from beginning, middle, end
  - remove out of bounds rejected
  - clear removes all operations but keeps chain
*/
bool d_tests_filter_chain_manipulate(
    struct dj_test_suite* _suite
)
{
    bool                       result;
    struct dj_filter_chain*     chain;
    struct dj_filter_operation* op;

    d_test_suite_test(_suite, "filter_chain_manipulate");
    op = NULL;

    result = true;

    chain = d_filter_chain_new();

    if (!chain)
    {
        return false;
    }

    // populate with [TAKE_FIRST, REVERSE, SKIP_FIRST]
    op = d_filter_take_first(1);
    d_filter_chain_add(chain, op);

    free(op);
    op = d_filter_reverse();
    d_filter_chain_add(chain, op);

    free(op);
    op = d_filter_skip_first(1);
    d_filter_chain_add(chain, op);

    // test 1: insert at beginning (index 0)
    free(op);
    op     = d_filter_take_last(5);
    result = D_TEST_CHECK_MSG(_suite,
                              "chain_insert_begin",
                              d_filter_chain_insert(chain, 0, op) == true,
                              "insert at index 0 should succeed") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "chain_insert_begin_count",
                              chain->count == 4,
                              "count should be 4 after insert") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "chain_insert_begin_op",
                              chain->operations[0].type == D_FILTER_OP_TAKE_LAST,
                              "inserted op should be at index 0") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "chain_insert_shifted",
                              chain->operations[1].type == D_FILTER_OP_TAKE_FIRST,
                              "original op[0] should shift to index 1") && result;

    // test 2: insert at end (index == count)
    d_tests_filter_op_release(op);
    op     = d_filter_init();
    result = D_TEST_CHECK_MSG(_suite,
                              "chain_insert_end",
                              d_filter_chain_insert(chain, chain->count, op) == true,
                              "insert at index==count should succeed") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "chain_insert_end_op",
                              chain->operations[chain->count - 1].type == D_FILTER_OP_INIT,
                              "inserted op should be at the end") && result;

    // test 3: insert out of bounds
    d_tests_filter_op_release(op);
    op     = d_filter_rest();
    result = D_TEST_CHECK_MSG(_suite,
                              "chain_insert_oob",
                              d_filter_chain_insert(chain, 999, op) == false,
                              "insert at out-of-bounds index should fail") && result;

    // test 4: insert NULL chain
    result = D_TEST_CHECK_MSG(_suite,
                              "chain_insert_null_chain",
                              d_filter_chain_insert(NULL, 0, op) == false,
                              "insert into NULL chain should fail") && result;

    // test 5: insert NULL op
    result = D_TEST_CHECK_MSG(_suite,
                              "chain_insert_null_op",
                              d_filter_chain_insert(chain, 0, NULL) == false,
                              "insert NULL op should fail") && result;

    // test 6: remove from middle
    // current: [TAKE_LAST, TAKE_FIRST, REVERSE, SKIP_FIRST, INIT]
    result = D_TEST_CHECK_MSG(_suite,
                              "chain_remove_mid",
                              d_filter_chain_remove(chain, 2) == true,
                              "remove at index 2 should succeed") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "chain_remove_shifted",
                              chain->operations[2].type == D_FILTER_OP_SKIP_FIRST,
                              "after removing index 2, SKIP_FIRST should move to index 2") && result;

    // test 7: remove from beginning (index 0)
    result = D_TEST_CHECK_MSG(_suite,
                              "chain_remove_begin",
                              d_filter_chain_remove(chain, 0) == true,
                              "remove at index 0 should succeed") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "chain_remove_begin_shifted",
                              chain->operations[0].type == D_FILTER_OP_TAKE_FIRST,
                              "after removing index 0, TAKE_FIRST should be at 0") && result;

    // test 8: remove out of bounds
    result = D_TEST_CHECK_MSG(_suite,
                              "chain_remove_oob",
                              d_filter_chain_remove(chain, 999) == false,
                              "remove out-of-bounds should fail") && result;

    // test 9: remove from NULL chain
    result = D_TEST_CHECK_MSG(_suite,
                              "chain_remove_null",
                              d_filter_chain_remove(NULL, 0) == false,
                              "remove from NULL chain should fail") && result;

    // test 10: clear
    d_filter_chain_clear(chain);
    result = D_TEST_CHECK_MSG(_suite,
                              "chain_clear_count",
                              chain->count == 0,
                              "clear should set count to 0") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "chain_clear_capacity_kept",
                              chain->capacity > 0,
                              "clear should preserve capacity") && result;

    // test 11: clear NULL safety
    d_filter_chain_clear(NULL);
    result = D_TEST_CHECK_MSG(_suite,
                              "chain_clear_null",
                              true,
                              "clear(NULL) should not crash") && result;

    d_filter_chain_free(chain);

    d_tests_filter_op_release(op);
    return result;
}


/*
d_tests_filter_chain_properties
  Tests chain property functions: length, is_empty.
  Tests the following:
  - length of empty chain is 0
  - length of populated chain
  - length of NULL is 0
  - is_empty on empty chain
  - is_empty on populated chain
  - is_empty on NULL
*/
bool d_tests_filter_chain_properties(
    struct dj_test_suite* _suite
)
{
    bool                       result;
    struct dj_filter_chain*     chain;
    struct dj_filter_operation* op;

    d_test_suite_test(_suite, "filter_chain_properties");
    op = NULL;

    result = true;

    chain = d_filter_chain_new();

    if (!chain)
    {
        return false;
    }

    // test 1: empty chain length
    result = D_TEST_CHECK_MSG(_suite,
                              "chain_length_empty",
                              d_filter_chain_length(chain) == 0,
                              "empty chain length should be 0") && result;

    // test 2: empty chain is_empty
    result = D_TEST_CHECK_MSG(_suite,
                              "chain_is_empty_true",
                              d_filter_chain_is_empty(chain) == true,
                              "empty chain should report is_empty=true") && result;

    // test 3: populated chain
    op = d_filter_take_first(1);
    d_filter_chain_add(chain, op);

    free(op);
    op = d_filter_reverse();
    d_filter_chain_add(chain, op);

    result = D_TEST_CHECK_MSG(_suite,
                              "chain_length_populated",
                              d_filter_chain_length(chain) == 2,
                              "chain with 2 ops should have length 2") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "chain_is_empty_false",
                              d_filter_chain_is_empty(chain) == false,
                              "populated chain should report is_empty=false") && result;

    d_filter_chain_free(chain);

    // test 4: NULL chain
    result = D_TEST_CHECK_MSG(_suite,
                              "chain_length_null",
                              d_filter_chain_length(NULL) == 0,
                              "NULL chain length should be 0") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "chain_is_empty_null",
                              d_filter_chain_is_empty(NULL) == true,
                              "NULL chain should report is_empty=true") && result;

    free(op);
    return result;
}


/*
d_tests_filter_chain_all
  Aggregation function that runs all chain management tests.
*/
bool d_tests_filter_chain_all(
    struct dj_test_suite* _suite
)
{
    bool result;

    d_test_suite_block(_suite, "filter_chain_all");

    result = true;

    result = d_tests_filter_chain_create(_suite)      && result;
    result = d_tests_filter_chain_add(_suite)         && result;
    result = d_tests_filter_chain_convenience(_suite) && result;
    result = d_tests_filter_chain_combine(_suite)     && result;
    result = d_tests_filter_chain_manipulate(_suite)  && result;
    result = d_tests_filter_chain_properties(_suite)  && result;

    return result;
}
