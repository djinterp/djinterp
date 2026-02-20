#include ".\filter_tests_sa.h"


// helper predicate for chain tests
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


/*
d_tests_sa_filter_chain_create
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
bool
d_tests_sa_filter_chain_create
(
    struct d_test_counter* _counter
)
{
    bool                       result;
    struct d_filter_chain*     chain;
    struct d_filter_chain*     clone;
    struct d_filter_operation* op;

    result = true;

    // test 1: new creates empty chain
    chain  = d_filter_chain_new();
    result = d_assert_standalone(
        chain != NULL,
        "chain_new_success",
        "d_filter_chain_new should succeed",
        _counter) && result;

    if (chain)
    {
        result = d_assert_standalone(
            chain->count == 0,
            "chain_new_empty_count",
            "new chain should have count=0",
            _counter) && result;

        result = d_assert_standalone(
            chain->capacity == 0,
            "chain_new_empty_capacity",
            "new chain should have capacity=0",
            _counter) && result;

        result = d_assert_standalone(
            chain->operations == NULL,
            "chain_new_null_ops",
            "new chain should have NULL operations",
            _counter) && result;

        result = d_assert_standalone(
            chain->owns_operations == true,
            "chain_new_owns_ops",
            "new chain should own its operations",
            _counter) && result;

        d_filter_chain_free(chain);
    }

    // test 2: new_with_capacity pre-allocates
    chain  = d_filter_chain_new_with_capacity(8);
    result = d_assert_standalone(
        chain != NULL,
        "chain_capacity_success",
        "d_filter_chain_new_with_capacity(8) should succeed",
        _counter) && result;

    if (chain)
    {
        result = d_assert_standalone(
            chain->capacity == 8,
            "chain_capacity_set",
            "chain should have capacity=8",
            _counter) && result;

        result = d_assert_standalone(
            chain->count == 0,
            "chain_capacity_count_zero",
            "chain should still have count=0",
            _counter) && result;

        result = d_assert_standalone(
            chain->operations != NULL,
            "chain_capacity_ops_allocated",
            "chain should have operations allocated",
            _counter) && result;

        d_filter_chain_free(chain);
    }

    // test 3: new_with_capacity(0) same as new()
    chain  = d_filter_chain_new_with_capacity(0);
    result = d_assert_standalone(
        chain != NULL,
        "chain_capacity_zero",
        "new_with_capacity(0) should succeed",
        _counter) && result;

    if (chain)
    {
        result = d_assert_standalone(
            chain->capacity == 0 && chain->operations == NULL,
            "chain_capacity_zero_empty",
            "capacity(0) should be same as new()",
            _counter) && result;

        d_filter_chain_free(chain);
    }

    // test 4: clone preserves operations
    chain = d_filter_chain_new();

    if (chain)
    {
        op = d_filter_take_first(3);
        d_filter_chain_add(chain, op);

        op = d_filter_reverse();
        d_filter_chain_add(chain, op);

        clone  = d_filter_chain_clone(chain);
        result = d_assert_standalone(
            clone != NULL,
            "chain_clone_success",
            "clone should succeed",
            _counter) && result;

        if (clone)
        {
            result = d_assert_standalone(
                clone->count == 2,
                "chain_clone_count",
                "clone should have same count",
                _counter) && result;

            result = d_assert_standalone(
                clone->operations[0].type == D_FILTER_OP_TAKE_FIRST,
                "chain_clone_op0",
                "clone op[0] should be TAKE_FIRST",
                _counter) && result;

            result = d_assert_standalone(
                clone->operations[1].type == D_FILTER_OP_REVERSE,
                "chain_clone_op1",
                "clone op[1] should be REVERSE",
                _counter) && result;

            result = d_assert_standalone(
                clone->operations != chain->operations,
                "chain_clone_independent",
                "clone operations array should be independent",
                _counter) && result;

            d_filter_chain_free(clone);
        }

        d_filter_chain_free(chain);
    }

    // test 5: clone NULL returns NULL
    clone  = d_filter_chain_clone(NULL);
    result = d_assert_standalone(
        clone == NULL,
        "chain_clone_null",
        "clone(NULL) should return NULL",
        _counter) && result;

    // test 6: free NULL safety
    d_filter_chain_free(NULL);
    result = d_assert_standalone(
        true,
        "chain_free_null",
        "free(NULL) should not crash",
        _counter) && result;

    return result;
}


/*
d_tests_sa_filter_chain_add
  Tests the generic d_filter_chain_add function.
  Tests the following:
  - adding first operation grows capacity
  - adding multiple operations
  - NULL chain rejection
  - NULL operation rejection
  - capacity growth (dynamic reallocation)
*/
bool
d_tests_sa_filter_chain_add
(
    struct d_test_counter* _counter
)
{
    bool                       result;
    struct d_filter_chain*     chain;
    struct d_filter_operation* op;
    size_t                     i;
    bool                       add_ok;

    result = true;

    chain = d_filter_chain_new();

    if (!chain)
    {
        return false;
    }

    // test 1: add first operation triggers allocation
    op     = d_filter_take_first(5);
    add_ok = d_filter_chain_add(chain, op);
    result = d_assert_standalone(
        add_ok == true,
        "chain_add_first",
        "adding first operation should succeed",
        _counter) && result;

    result = d_assert_standalone(
        chain->count == 1,
        "chain_add_first_count",
        "count should be 1 after first add",
        _counter) && result;

    result = d_assert_standalone(
        chain->capacity >= 1,
        "chain_add_first_capacity",
        "capacity should grow on first add",
        _counter) && result;

    result = d_assert_standalone(
        chain->operations[0].type == D_FILTER_OP_TAKE_FIRST,
        "chain_add_first_stored",
        "first operation should be stored correctly",
        _counter) && result;

    // test 2: add many operations to trigger reallocation
    for (i = 1; i < 10; i++)
    {
        op = d_filter_skip_first(i);
        d_filter_chain_add(chain, op);
    }

    result = d_assert_standalone(
        chain->count == 10,
        "chain_add_many_count",
        "count should be 10 after 10 adds",
        _counter) && result;

    result = d_assert_standalone(
        chain->capacity >= 10,
        "chain_add_many_capacity",
        "capacity should accommodate 10 operations",
        _counter) && result;

    d_filter_chain_free(chain);

    // test 3: NULL chain rejection
    op     = d_filter_reverse();
    add_ok = d_filter_chain_add(NULL, op);
    result = d_assert_standalone(
        add_ok == false,
        "chain_add_null_chain",
        "add to NULL chain should return false",
        _counter) && result;

    // test 4: NULL operation rejection
    chain  = d_filter_chain_new();
    add_ok = d_filter_chain_add(chain, NULL);
    result = d_assert_standalone(
        add_ok == false,
        "chain_add_null_op",
        "add NULL operation should return false",
        _counter) && result;

    d_filter_chain_free(chain);

    return result;
}


/*
d_tests_sa_filter_chain_convenience
  Tests convenience chain-add helpers: add_take_first, add_take_last,
add_skip_first, add_skip_last, add_range, add_where, add_where_context.
  Tests the following:
  - each convenience function creates and adds the correct op type
  - NULL chain handling
*/
bool
d_tests_sa_filter_chain_convenience
(
    struct d_test_counter* _counter
)
{
    bool                   result;
    struct d_filter_chain* chain;
    int                    threshold;

    result    = true;
    threshold = 5;

    chain = d_filter_chain_new();

    if (!chain)
    {
        return false;
    }

    // test 1: add_take_first
    result = d_assert_standalone(
        d_filter_chain_add_take_first(chain, 3) == true,
        "chain_conv_take_first",
        "add_take_first should succeed",
        _counter) && result;

    result = d_assert_standalone(
        chain->operations[0].type == D_FILTER_OP_TAKE_FIRST &&
        chain->operations[0].params.count == 3,
        "chain_conv_take_first_stored",
        "take_first(3) should be stored correctly",
        _counter) && result;

    // test 2: add_take_last
    result = d_assert_standalone(
        d_filter_chain_add_take_last(chain, 2) == true,
        "chain_conv_take_last",
        "add_take_last should succeed",
        _counter) && result;

    // test 3: add_skip_first
    result = d_assert_standalone(
        d_filter_chain_add_skip_first(chain, 1) == true,
        "chain_conv_skip_first",
        "add_skip_first should succeed",
        _counter) && result;

    // test 4: add_skip_last
    result = d_assert_standalone(
        d_filter_chain_add_skip_last(chain, 1) == true,
        "chain_conv_skip_last",
        "add_skip_last should succeed",
        _counter) && result;

    // test 5: add_range
    result = d_assert_standalone(
        d_filter_chain_add_range(chain, 0, 5) == true,
        "chain_conv_range",
        "add_range should succeed",
        _counter) && result;

    // test 6: add_where
    result = d_assert_standalone(
        d_filter_chain_add_where(chain, pred_is_even) == true,
        "chain_conv_where",
        "add_where should succeed",
        _counter) && result;

    // test 7: add_where_context
    result = d_assert_standalone(
        d_filter_chain_add_where_context(chain, pred_is_positive,
                                     &threshold) == true,
        "chain_conv_where_context",
        "add_where_context should succeed",
        _counter) && result;

    // verify total count
    result = d_assert_standalone(
        chain->count == 7,
        "chain_conv_total_count",
        "chain should have 7 operations",
        _counter) && result;

    d_filter_chain_free(chain);

    // test 8: NULL chain rejection for all convenience functions
    result = d_assert_standalone(
        d_filter_chain_add_take_first(NULL, 1) == false,
        "chain_conv_null_take_first",
        "add_take_first(NULL) should return false",
        _counter) && result;

    result = d_assert_standalone(
        d_filter_chain_add_where(NULL, pred_is_even) == false,
        "chain_conv_null_where",
        "add_where(NULL) should return false",
        _counter) && result;

    return result;
}


/*
d_tests_sa_filter_chain_combine
  Tests chain combination functions: concat, append.
  Tests the following:
  - concat creates new chain with all operations from both
  - concat preserves operation order
  - concat with NULL returns NULL
  - append adds all operations from source to dest
  - append with NULL returns false
*/
bool
d_tests_sa_filter_chain_combine
(
    struct d_test_counter* _counter
)
{
    bool                       result;
    struct d_filter_chain*     chain_a;
    struct d_filter_chain*     chain_b;
    struct d_filter_chain*     combined;
    struct d_filter_operation* op;

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

    op = d_filter_reverse();
    d_filter_chain_add(chain_a, op);

    op = d_filter_skip_first(1);
    d_filter_chain_add(chain_b, op);

    // test 1: concat creates combined chain
    combined = d_filter_chain_concat(chain_a, chain_b);
    result   = d_assert_standalone(
        combined != NULL,
        "chain_concat_success",
        "concat should succeed",
        _counter) && result;

    if (combined)
    {
        result = d_assert_standalone(
            combined->count == 3,
            "chain_concat_count",
            "concat should have 3 operations (2+1)",
            _counter) && result;

        result = d_assert_standalone(
            combined->operations[0].type == D_FILTER_OP_TAKE_FIRST,
            "chain_concat_order_0",
            "concat op[0] should be TAKE_FIRST from chain_a",
            _counter) && result;

        result = d_assert_standalone(
            combined->operations[1].type == D_FILTER_OP_REVERSE,
            "chain_concat_order_1",
            "concat op[1] should be REVERSE from chain_a",
            _counter) && result;

        result = d_assert_standalone(
            combined->operations[2].type == D_FILTER_OP_SKIP_FIRST,
            "chain_concat_order_2",
            "concat op[2] should be SKIP_FIRST from chain_b",
            _counter) && result;

        d_filter_chain_free(combined);
    }

    // test 2: concat with NULL returns NULL
    result = d_assert_standalone(
        d_filter_chain_concat(NULL, chain_b) == NULL,
        "chain_concat_null_first",
        "concat(NULL, chain) should return NULL",
        _counter) && result;

    result = d_assert_standalone(
        d_filter_chain_concat(chain_a, NULL) == NULL,
        "chain_concat_null_second",
        "concat(chain, NULL) should return NULL",
        _counter) && result;

    // test 3: append adds to dest
    result = d_assert_standalone(
        chain_a->count == 2,
        "chain_append_pre_count",
        "chain_a should have 2 before append",
        _counter) && result;

    result = d_assert_standalone(
        d_filter_chain_append(chain_a, chain_b) == true,
        "chain_append_success",
        "append should succeed",
        _counter) && result;

    result = d_assert_standalone(
        chain_a->count == 3,
        "chain_append_post_count",
        "chain_a should have 3 after append",
        _counter) && result;

    result = d_assert_standalone(
        chain_a->operations[2].type == D_FILTER_OP_SKIP_FIRST,
        "chain_append_op_added",
        "appended operation should be at end",
        _counter) && result;

    // test 4: append with NULL returns false
    result = d_assert_standalone(
        d_filter_chain_append(NULL, chain_b) == false,
        "chain_append_null_dest",
        "append(NULL, chain) should return false",
        _counter) && result;

    result = d_assert_standalone(
        d_filter_chain_append(chain_a, NULL) == false,
        "chain_append_null_source",
        "append(chain, NULL) should return false",
        _counter) && result;

    d_filter_chain_free(chain_a);
    d_filter_chain_free(chain_b);

    return result;
}


/*
d_tests_sa_filter_chain_manipulate
  Tests chain manipulation: insert, remove, clear.
  Tests the following:
  - insert at beginning, middle, end
  - insert out of bounds rejected
  - insert with NULL rejected
  - remove from beginning, middle, end
  - remove out of bounds rejected
  - clear removes all operations but keeps chain
*/
bool
d_tests_sa_filter_chain_manipulate
(
    struct d_test_counter* _counter
)
{
    bool                       result;
    struct d_filter_chain*     chain;
    struct d_filter_operation* op;

    result = true;

    chain = d_filter_chain_new();

    if (!chain)
    {
        return false;
    }

    // populate with [TAKE_FIRST, REVERSE, SKIP_FIRST]
    op = d_filter_take_first(1);
    d_filter_chain_add(chain, op);

    op = d_filter_reverse();
    d_filter_chain_add(chain, op);

    op = d_filter_skip_first(1);
    d_filter_chain_add(chain, op);

    // test 1: insert at beginning (index 0)
    op     = d_filter_take_last(5);
    result = d_assert_standalone(
        d_filter_chain_insert(chain, 0, op) == true,
        "chain_insert_begin",
        "insert at index 0 should succeed",
        _counter) && result;

    result = d_assert_standalone(
        chain->count == 4,
        "chain_insert_begin_count",
        "count should be 4 after insert",
        _counter) && result;

    result = d_assert_standalone(
        chain->operations[0].type == D_FILTER_OP_TAKE_LAST,
        "chain_insert_begin_op",
        "inserted op should be at index 0",
        _counter) && result;

    result = d_assert_standalone(
        chain->operations[1].type == D_FILTER_OP_TAKE_FIRST,
        "chain_insert_shifted",
        "original op[0] should shift to index 1",
        _counter) && result;

    // test 2: insert at end (index == count)
    op     = d_filter_init();
    result = d_assert_standalone(
        d_filter_chain_insert(chain, chain->count, op) == true,
        "chain_insert_end",
        "insert at index==count should succeed",
        _counter) && result;

    result = d_assert_standalone(
        chain->operations[chain->count - 1].type == D_FILTER_OP_INIT,
        "chain_insert_end_op",
        "inserted op should be at the end",
        _counter) && result;

    // test 3: insert out of bounds
    op     = d_filter_rest();
    result = d_assert_standalone(
        d_filter_chain_insert(chain, 999, op) == false,
        "chain_insert_oob",
        "insert at out-of-bounds index should fail",
        _counter) && result;

    // test 4: insert NULL chain
    result = d_assert_standalone(
        d_filter_chain_insert(NULL, 0, op) == false,
        "chain_insert_null_chain",
        "insert into NULL chain should fail",
        _counter) && result;

    // test 5: insert NULL op
    result = d_assert_standalone(
        d_filter_chain_insert(chain, 0, NULL) == false,
        "chain_insert_null_op",
        "insert NULL op should fail",
        _counter) && result;

    // test 6: remove from middle
    // current: [TAKE_LAST, TAKE_FIRST, REVERSE, SKIP_FIRST, INIT]
    result = d_assert_standalone(
        d_filter_chain_remove(chain, 2) == true,
        "chain_remove_mid",
        "remove at index 2 should succeed",
        _counter) && result;

    result = d_assert_standalone(
        chain->operations[2].type == D_FILTER_OP_SKIP_FIRST,
        "chain_remove_shifted",
        "after removing index 2, SKIP_FIRST should move to index 2",
        _counter) && result;

    // test 7: remove from beginning (index 0)
    result = d_assert_standalone(
        d_filter_chain_remove(chain, 0) == true,
        "chain_remove_begin",
        "remove at index 0 should succeed",
        _counter) && result;

    result = d_assert_standalone(
        chain->operations[0].type == D_FILTER_OP_TAKE_FIRST,
        "chain_remove_begin_shifted",
        "after removing index 0, TAKE_FIRST should be at 0",
        _counter) && result;

    // test 8: remove out of bounds
    result = d_assert_standalone(
        d_filter_chain_remove(chain, 999) == false,
        "chain_remove_oob",
        "remove out-of-bounds should fail",
        _counter) && result;

    // test 9: remove from NULL chain
    result = d_assert_standalone(
        d_filter_chain_remove(NULL, 0) == false,
        "chain_remove_null",
        "remove from NULL chain should fail",
        _counter) && result;

    // test 10: clear
    d_filter_chain_clear(chain);
    result = d_assert_standalone(
        chain->count == 0,
        "chain_clear_count",
        "clear should set count to 0",
        _counter) && result;

    result = d_assert_standalone(
        chain->capacity > 0,
        "chain_clear_capacity_kept",
        "clear should preserve capacity",
        _counter) && result;

    // test 11: clear NULL safety
    d_filter_chain_clear(NULL);
    result = d_assert_standalone(
        true,
        "chain_clear_null",
        "clear(NULL) should not crash",
        _counter) && result;

    d_filter_chain_free(chain);

    return result;
}


/*
d_tests_sa_filter_chain_properties
  Tests chain property functions: length, is_empty.
  Tests the following:
  - length of empty chain is 0
  - length of populated chain
  - length of NULL is 0
  - is_empty on empty chain
  - is_empty on populated chain
  - is_empty on NULL
*/
bool
d_tests_sa_filter_chain_properties
(
    struct d_test_counter* _counter
)
{
    bool                       result;
    struct d_filter_chain*     chain;
    struct d_filter_operation* op;

    result = true;

    chain = d_filter_chain_new();

    if (!chain)
    {
        return false;
    }

    // test 1: empty chain length
    result = d_assert_standalone(
        d_filter_chain_length(chain) == 0,
        "chain_length_empty",
        "empty chain length should be 0",
        _counter) && result;

    // test 2: empty chain is_empty
    result = d_assert_standalone(
        d_filter_chain_is_empty(chain) == true,
        "chain_is_empty_true",
        "empty chain should report is_empty=true",
        _counter) && result;

    // test 3: populated chain
    op = d_filter_take_first(1);
    d_filter_chain_add(chain, op);

    op = d_filter_reverse();
    d_filter_chain_add(chain, op);

    result = d_assert_standalone(
        d_filter_chain_length(chain) == 2,
        "chain_length_populated",
        "chain with 2 ops should have length 2",
        _counter) && result;

    result = d_assert_standalone(
        d_filter_chain_is_empty(chain) == false,
        "chain_is_empty_false",
        "populated chain should report is_empty=false",
        _counter) && result;

    d_filter_chain_free(chain);

    // test 4: NULL chain
    result = d_assert_standalone(
        d_filter_chain_length(NULL) == 0,
        "chain_length_null",
        "NULL chain length should be 0",
        _counter) && result;

    result = d_assert_standalone(
        d_filter_chain_is_empty(NULL) == true,
        "chain_is_empty_null",
        "NULL chain should report is_empty=true",
        _counter) && result;

    return result;
}


/*
d_tests_sa_filter_chain_all
  Aggregation function that runs all chain management tests.
*/
bool
d_tests_sa_filter_chain_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Filter Chain Management\n");
    printf("  ----------------------------------\n");

    result = d_tests_sa_filter_chain_create(_counter)      && result;
    result = d_tests_sa_filter_chain_add(_counter)         && result;
    result = d_tests_sa_filter_chain_convenience(_counter) && result;
    result = d_tests_sa_filter_chain_combine(_counter)     && result;
    result = d_tests_sa_filter_chain_manipulate(_counter)  && result;
    result = d_tests_sa_filter_chain_properties(_counter)  && result;

    return result;
}
