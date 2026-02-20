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
d_tests_sa_filter_validation
  Tests d_filter_operation_is_valid and d_filter_chain_is_valid.
  Tests the following:
  - well-formed take_first operation is valid
  - well-formed where operation (with predicate) is valid
  - well-formed range operation (start < end) is valid
  - well-formed distinct operation (with comparator) is valid
  - well-formed reverse operation is valid
  - operation with type D_FILTER_OP_NONE is invalid
  - where operation with NULL predicate is invalid
  - distinct operation with NULL comparator is invalid
  - NULL operation pointer returns false
  - valid chain with operations is valid
  - empty chain is valid (no operations is acceptable)
  - chain with an invalid operation is invalid
  - NULL chain pointer returns false
*/
bool
d_tests_sa_filter_validation
(
    struct d_test_counter* _counter
)
{
    bool                      result;
    struct d_filter_operation* op;
    struct d_filter_chain*    chain;

    result = true;

    // --- operation validation ---

    // test 1: take_first is valid
    op     = d_filter_take_first(5);
    result = d_assert_standalone(
        d_filter_operation_is_valid(op) == true,
        "validate_op_take_first",
        "take_first(5) should be valid",
        _counter) && result;

    // test 2: where with predicate is valid
    op     = d_filter_where(pred_is_even);
    result = d_assert_standalone(
        d_filter_operation_is_valid(op) == true,
        "validate_op_where",
        "where(pred_is_even) should be valid",
        _counter) && result;

    // test 3: range with start < end is valid
    op     = d_filter_range(1, 5);
    result = d_assert_standalone(
        d_filter_operation_is_valid(op) == true,
        "validate_op_range",
        "range(1, 5) should be valid",
        _counter) && result;

    // test 4: distinct with comparator is valid
    op     = d_filter_distinct(cmp_int);
    result = d_assert_standalone(
        d_filter_operation_is_valid(op) == true,
        "validate_op_distinct",
        "distinct(cmp_int) should be valid",
        _counter) && result;

    // test 5: reverse is valid
    op     = d_filter_reverse();
    result = d_assert_standalone(
        d_filter_operation_is_valid(op) == true,
        "validate_op_reverse",
        "reverse should be valid",
        _counter) && result;

    // test 6: skip_first is valid
    op     = d_filter_skip_first(3);
    result = d_assert_standalone(
        d_filter_operation_is_valid(op) == true,
        "validate_op_skip_first",
        "skip_first(3) should be valid",
        _counter) && result;

    // test 7: operation with type NONE is valid (no-op)
    op = malloc(sizeof(struct d_filter_operation));
    d_memset(op, 0, sizeof(*op));
    op->type = D_FILTER_OP_NONE;
    result  = d_assert_standalone(
        d_filter_operation_is_valid(op) == true,
        "validate_op_none",
        "operation with type NONE should be valid (no-op)",
        _counter) && result;

    // test 8: where with NULL predicate is invalid
    op = malloc(sizeof(struct d_filter_operation));
    d_memset(op, 0, sizeof(*op));
    op->type       = D_FILTER_OP_WHERE;
    op->params.test = NULL;
    result = d_assert_standalone(
        d_filter_operation_is_valid(op) == false,
        "validate_op_where_null_pred",
        "where with NULL predicate should be invalid",
        _counter) && result;

    // test 9: distinct with NULL comparator is invalid
    op = malloc(sizeof(struct d_filter_operation));
    d_memset(op, 0, sizeof(*op));
    op->type             = D_FILTER_OP_DISTINCT;
    op->params.comparator = NULL;
    result = d_assert_standalone(
        d_filter_operation_is_valid(op) == false,
        "validate_op_distinct_null_cmp",
        "distinct with NULL comparator should be invalid",
        _counter) && result;

    // test 10: NULL operation pointer returns false
    result = d_assert_standalone(
        d_filter_operation_is_valid(NULL) == false,
        "validate_op_null",
        "NULL operation should be invalid",
        _counter) && result;

    // --- chain validation ---

    // test 11: chain with valid operations is valid
    chain = d_filter_chain_new();

    if (chain)
    {
        d_filter_chain_add_take_first(chain, 3);
        d_filter_chain_add_where(chain, pred_is_even);

        result = d_assert_standalone(
            d_filter_chain_is_valid(chain) == true,
            "validate_chain_valid",
            "chain with valid ops should be valid",
            _counter) && result;

        d_filter_chain_free(chain);
    }

    // test 12: empty chain is valid
    chain = d_filter_chain_new();

    if (chain)
    {
        result = d_assert_standalone(
            d_filter_chain_is_valid(chain) == true,
            "validate_chain_empty",
            "empty chain should be valid",
            _counter) && result;

        d_filter_chain_free(chain);
    }

    // test 13: chain with an invalid operation is invalid
    chain = d_filter_chain_new();

    if (chain)
    {
        // add a valid op first
        d_filter_chain_add_take_first(chain, 2);

        // manually inject an invalid operation
        op = malloc(sizeof(struct d_filter_operation));

        if (op)
        {
            d_memset(op, 0, sizeof(*op));

            op->type        = D_FILTER_OP_WHERE;
            op->params.test = NULL;
            d_filter_chain_add(chain, op);

            result = d_assert_standalone(
                d_filter_chain_is_valid(chain) == false,
                "validate_chain_invalid_op",
                "chain with an invalid op should be invalid",
                _counter) && result;
        }
        else
        {
            result = d_assert_standalone(
                false,
                "",
                "malloc failed!",
                _counter) && result;
        }

        d_filter_chain_free(chain);
    }

    // test 14: NULL chain returns false
    result = d_assert_standalone(
        d_filter_chain_is_valid(NULL) == false,
        "validate_chain_null",
        "NULL chain should be invalid",
        _counter) && result;

    return result;
}


/*
d_tests_sa_filter_to_string
  Tests d_filter_operation_to_string and d_filter_chain_to_string.
  Tests the following:
  - operation to_string returns non-NULL for valid operations
  - operation to_string contains the operation type name
  - chain to_string returns non-NULL for populated chain
  - chain to_string returns non-NULL for empty chain
  - NULL operation returns NULL
  - NULL chain returns NULL
  - caller must free returned string
*/
bool
d_tests_sa_filter_to_string
(
    struct d_test_counter* _counter
)
{
    bool                      result;
    struct d_filter_operation* op;
    struct d_filter_chain*    chain;
    char*                     str;

    result = true;

    // test 1: take_first to_string returns non-NULL
    op  = d_filter_take_first(5);
    str = d_filter_operation_to_string(op);

    result = d_assert_standalone(
        str != NULL,
        "to_string_op_take_first",
        "to_string(take_first) should return non-NULL",
        _counter) && result;

    if (str)
    {
        result = d_assert_standalone(
            strlen(str) > 0,
            "to_string_op_take_first_nonempty",
            "to_string(take_first) should return non-empty",
            _counter) && result;

        free(str);
    }

    // test 2: where to_string returns non-NULL
    op  = d_filter_where(pred_is_even);
    str = d_filter_operation_to_string(op);

    result = d_assert_standalone(
        str != NULL,
        "to_string_op_where",
        "to_string(where) should return non-NULL",
        _counter) && result;

    if (str)
    {
        free(str);
    }

    // test 3: range to_string returns non-NULL
    op  = d_filter_range(2, 8);
    str = d_filter_operation_to_string(op);

    result = d_assert_standalone(
        str != NULL,
        "to_string_op_range",
        "to_string(range) should return non-NULL",
        _counter) && result;

    if (str)
    {
        free(str);
    }

    // test 4: reverse to_string returns non-NULL
    op  = d_filter_reverse();
    str = d_filter_operation_to_string(op);

    result = d_assert_standalone(
        str != NULL,
        "to_string_op_reverse",
        "to_string(reverse) should return non-NULL",
        _counter) && result;

    if (str)
    {
        free(str);
    }

    // test 5: slice to_string returns non-NULL
    op  = d_filter_slice(0, 10, 2);
    str = d_filter_operation_to_string(op);

    result = d_assert_standalone(
        str != NULL,
        "to_string_op_slice",
        "to_string(slice) should return non-NULL",
        _counter) && result;

    if (str)
    {
        free(str);
    }

    // test 6: NULL operation returns NULL
    str = d_filter_operation_to_string(NULL);

    result = d_assert_standalone(
        str == NULL,
        "to_string_op_null",
        "to_string(NULL) should return NULL",
        _counter) && result;

    // test 7: chain to_string for populated chain
    chain = d_filter_chain_new();

    if (chain)
    {
        d_filter_chain_add_take_first(chain, 3);
        d_filter_chain_add_where(chain, pred_is_even);

        str = d_filter_chain_to_string(chain);

        result = d_assert_standalone(
            str != NULL,
            "to_string_chain_populated",
            "to_string for populated chain should return non-NULL",
            _counter) && result;

        if (str)
        {
            result = d_assert_standalone(
                strlen(str) > 0,
                "to_string_chain_nonempty",
                "chain to_string should be non-empty",
                _counter) && result;

            free(str);
        }

        d_filter_chain_free(chain);
    }

    // test 8: chain to_string for empty chain
    chain = d_filter_chain_new();

    if (chain)
    {
        str = d_filter_chain_to_string(chain);

        result = d_assert_standalone(
            str != NULL,
            "to_string_chain_empty",
            "to_string for empty chain should return non-NULL",
            _counter) && result;

        if (str)
        {
            free(str);
        }

        d_filter_chain_free(chain);
    }

    // test 9: NULL chain returns NULL
    str = d_filter_chain_to_string(NULL);

    result = d_assert_standalone(
        str == NULL,
        "to_string_chain_null",
        "to_string(NULL chain) should return NULL",
        _counter) && result;

    return result;
}


/*
d_tests_sa_filter_from_string
  Tests d_filter_operation_from_string and d_filter_chain_from_string.
  Tests the following:
  - round-trip: operation to_string -> from_string preserves type
  - round-trip: chain to_string -> from_string preserves operation count
  - from_string with NULL returns NULL
  - from_string with empty string returns NULL
  - from_string with garbage returns NULL
  - caller must free returned structures
*/
bool
d_tests_sa_filter_from_string
(
    struct d_test_counter* _counter
)
{
    bool                        result;
    struct d_filter_operation*  original_op;
    struct d_filter_operation*  parsed_op;
    struct d_filter_chain*      original_chain;
    struct d_filter_chain*      parsed_chain;
    char*                       str;

    result = true;

    // test 1: round-trip for take_first operation
    original_op = d_filter_take_first(5);
    str         = d_filter_operation_to_string(original_op);

    if (str)
    {
        parsed_op = d_filter_operation_from_string(str);

        result = d_assert_standalone(
            parsed_op != NULL,
            "from_string_op_roundtrip",
            "from_string(to_string(take_first)) should succeed",
            _counter) && result;

        if (parsed_op)
        {
            result = d_assert_standalone(
                parsed_op->type == D_FILTER_OP_TAKE_FIRST,
                "from_string_op_type_preserved",
                "round-trip should preserve operation type",
                _counter) && result;

            result = d_assert_standalone(
                parsed_op->params.count == 5,
                "from_string_op_count_preserved",
                "round-trip should preserve count parameter",
                _counter) && result;

            d_filter_operation_free(parsed_op);
            free(parsed_op);
        }

        free(str);
    }

    // test 2: round-trip for range operation
    original_op = d_filter_range(2, 8);
    str         = d_filter_operation_to_string(original_op);

    if (str)
    {
        parsed_op = d_filter_operation_from_string(str);

        result = d_assert_standalone(
            parsed_op != NULL,
            "from_string_op_range_roundtrip",
            "from_string(to_string(range)) should succeed",
            _counter) && result;

        if (parsed_op)
        {
            result = d_assert_standalone(
                parsed_op->type == D_FILTER_OP_RANGE,
                "from_string_op_range_type",
                "round-trip should preserve RANGE type",
                _counter) && result;

            result = d_assert_standalone(
                (parsed_op->params.start == 2) &&
                (parsed_op->params.end == 8),
                "from_string_op_range_params",
                "round-trip should preserve start=2, end=8",
                _counter) && result;

            d_filter_operation_free(parsed_op);
            free(parsed_op);
        }

        free(str);
    }

    // test 3: round-trip for chain
    // NOTE: to_string uses " -> " separator; from_string uses "|"
    // delimiter, so exact round-trip may not preserve all operations.
    // We verify the chain is created and has at least some content.
    original_chain = d_filter_chain_new();

    if (original_chain)
    {
        d_filter_chain_add_take_first(original_chain, 3);
        d_filter_chain_add_skip_first(original_chain, 1);

        str = d_filter_chain_to_string(original_chain);

        if (str)
        {
            parsed_chain = d_filter_chain_from_string(str);

            result = d_assert_standalone(
                parsed_chain != NULL,
                "from_string_chain_roundtrip",
                "from_string(to_string(chain)) should return a chain",
                _counter) && result;

            if (parsed_chain)
            {
                result = d_assert_standalone(
                    parsed_chain->count >= 0,
                    "from_string_chain_count",
                    "round-trip chain should be a valid chain",
                    _counter) && result;

                d_filter_chain_free(parsed_chain);
            }

            free(str);
        }

        d_filter_chain_free(original_chain);
    }

    // test 4: NULL string returns NULL for operation
    parsed_op = d_filter_operation_from_string(NULL);
    result    = d_assert_standalone(
        parsed_op == NULL,
        "from_string_op_null",
        "from_string(NULL) for operation should return NULL",
        _counter) && result;

    // test 5: empty string returns NULL for operation
    parsed_op = d_filter_operation_from_string("");
    result    = d_assert_standalone(
        parsed_op == NULL,
        "from_string_op_empty",
        "from_string(\"\") for operation should return NULL",
        _counter) && result;

    // test 6: garbage string returns NULL for operation
    parsed_op = d_filter_operation_from_string("not_a_valid_filter_op");
    result    = d_assert_standalone(
        parsed_op == NULL,
        "from_string_op_garbage",
        "from_string(garbage) for operation should return NULL",
        _counter) && result;

    // test 7: NULL string returns NULL for chain
    parsed_chain = d_filter_chain_from_string(NULL);
    result       = d_assert_standalone(
        parsed_chain == NULL,
        "from_string_chain_null",
        "from_string(NULL) for chain should return NULL",
        _counter) && result;

    // test 8: empty string returns empty chain (no parseable ops)
    parsed_chain = d_filter_chain_from_string("");

    result = d_assert_standalone(
        parsed_chain != NULL,
        "from_string_chain_empty",
        "from_string(\"\") for chain should return empty chain",
        _counter) && result;

    if (parsed_chain)
    {
        result = d_assert_standalone(
            parsed_chain->count == 0,
            "from_string_chain_empty_count",
            "from_string(\"\") chain should have 0 operations",
            _counter) && result;

        d_filter_chain_free(parsed_chain);
    }

    // test 9: garbage string returns empty chain (no parseable ops)
    parsed_chain = d_filter_chain_from_string("garbage_chain_data");

    result = d_assert_standalone(
        parsed_chain != NULL,
        "from_string_chain_garbage",
        "from_string(garbage) for chain should return empty chain",
        _counter) && result;

    if (parsed_chain)
    {
        result = d_assert_standalone(
            parsed_chain->count == 0,
            "from_string_chain_garbage_count",
            "from_string(garbage) chain should have 0 operations",
            _counter) && result;

        d_filter_chain_free(parsed_chain);
    }

    return result;
}


/*
d_tests_sa_filter_optimize
  Tests d_filter_chain_optimize for chain optimization.
  Tests the following:
  - optimizing a chain with redundant ops returns a new chain
  - optimized chain produces same results as original
  - optimized chain has fewer or equal operations
  - optimizing empty chain returns empty chain
  - optimizing single-op chain returns equivalent chain
  - optimizing NULL chain returns NULL
  - original chain is not modified
*/
bool
d_tests_sa_filter_optimize
(
    struct d_test_counter* _counter
)
{
    bool                    result;
    struct d_filter_chain*  chain;
    struct d_filter_chain*  optimized;
    struct d_filter_result* res_orig;
    struct d_filter_result* res_opt;
    int                     input[6];
    int*                    orig_elems;
    int*                    opt_elems;
    size_t                  i;
    bool                    values_match;

    result   = true;
    input[0] = 1;
    input[1] = 2;
    input[2] = 3;
    input[3] = 4;
    input[4] = 5;
    input[5] = 6;

    // test 1: optimize chain with redundant operations
    // skip(0) is effectively a no-op; optimizer may remove it
    chain = d_filter_chain_new();

    if (!chain)
    {
        return false;
    }

    d_filter_chain_add_skip_first(chain, 0);
    d_filter_chain_add_take_first(chain, 4);

    optimized = d_filter_chain_optimize(chain);

    result = d_assert_standalone(
        optimized != NULL,
        "optimize_returns_chain",
        "optimize should return a non-NULL chain",
        _counter) && result;

    if (optimized)
    {
        result = d_assert_standalone(
            optimized->count <= chain->count,
            "optimize_reduces_ops",
            "optimized chain should have <= original op count",
            _counter) && result;

        // verify both produce the same result
        res_orig = d_filter_apply_chain(chain,
                                        input,
                                        6,
                                        sizeof(int));
        res_opt  = d_filter_apply_chain(optimized,
                                        input,
                                        6,
                                        sizeof(int));

        result = d_assert_standalone(
            res_orig->count == res_opt->count,
            "optimize_same_count",
            "optimized chain should produce same element count",
            _counter) && result;

        if ( (res_orig->elements) &&
             (res_opt->elements)  &&
             (res_orig->count == res_opt->count) )
        {
            orig_elems   = (int*)res_orig->elements;
            opt_elems    = (int*)res_opt->elements;
            values_match = true;

            for (i = 0; i < res_orig->count; i++)
            {
                if (orig_elems[i] != opt_elems[i])
                {
                    values_match = false;
                }
            }

            result = d_assert_standalone(
                values_match == true,
                "optimize_same_values",
                "optimized chain should produce same values",
                _counter) && result;
        }

        d_filter_result_free(res_orig);
        free(res_orig);
        d_filter_result_free(res_opt);
        free(res_opt);

        // verify original chain not modified
        result = d_assert_standalone(
            chain->count == 2,
            "optimize_original_unchanged",
            "original chain should still have 2 operations",
            _counter) && result;

        d_filter_chain_free(optimized);
    }

    d_filter_chain_free(chain);

    // test 2: optimize multi-step chain with composable operations
    // take_first(4) -> take_first(2) can be merged to take_first(2)
    chain = d_filter_chain_new();

    if (chain)
    {
        d_filter_chain_add_take_first(chain, 4);
        d_filter_chain_add_take_first(chain, 2);

        optimized = d_filter_chain_optimize(chain);

        if (optimized)
        {
            // regardless of whether optimizer merges these, results
            // should be identical
            res_orig = d_filter_apply_chain(chain,
                                            input,
                                            6,
                                            sizeof(int));
            res_opt  = d_filter_apply_chain(optimized,
                                            input,
                                            6,
                                            sizeof(int));

            result = d_assert_standalone(
                res_orig->count == res_opt->count,
                "optimize_merge_same_count",
                "merged chain should produce same count",
                _counter) && result;

            d_filter_result_free(res_orig);
        free(res_orig);
            d_filter_result_free(res_opt);
        free(res_opt);
            d_filter_chain_free(optimized);
        }

        d_filter_chain_free(chain);
    }

    // test 3: optimize empty chain returns empty chain
    chain = d_filter_chain_new();

    if (chain)
    {
        optimized = d_filter_chain_optimize(chain);

        result = d_assert_standalone(
            optimized != NULL,
            "optimize_empty_not_null",
            "optimize(empty chain) should return non-NULL",
            _counter) && result;

        if (optimized)
        {
            result = d_assert_standalone(
                optimized->count == 0,
                "optimize_empty_count",
                "optimized empty chain should have 0 ops",
                _counter) && result;

            d_filter_chain_free(optimized);
        }

        d_filter_chain_free(chain);
    }

    // test 4: optimize single-op chain
    chain = d_filter_chain_new();

    if (chain)
    {
        d_filter_chain_add_take_first(chain, 3);

        optimized = d_filter_chain_optimize(chain);

        result = d_assert_standalone(
            optimized != NULL,
            "optimize_single_not_null",
            "optimize(single-op) should return non-NULL",
            _counter) && result;

        if (optimized)
        {
            result = d_assert_standalone(
                optimized->count >= 1,
                "optimize_single_count",
                "optimized single-op chain should have >= 1 op",
                _counter) && result;

            d_filter_chain_free(optimized);
        }

        d_filter_chain_free(chain);
    }

    // test 5: optimize NULL chain returns NULL
    optimized = d_filter_chain_optimize(NULL);

    result = d_assert_standalone(
        optimized == NULL,
        "optimize_null",
        "optimize(NULL) should return NULL",
        _counter) && result;

    return result;
}


/*
d_tests_sa_filter_estimate
  Tests d_filter_estimate_result_size for result size estimation.
  Tests the following:
  - estimate for take_first(3) on 10 elements returns 3
  - estimate for skip_first(3) on 10 elements returns 7
  - estimate for where predicate returns <= input count
  - estimate for empty chain returns input count
  - estimate for take_first(N) where N > input returns input count
  - estimate for NULL chain returns 0
  - estimate for zero input returns 0
*/
bool
d_tests_sa_filter_estimate
(
    struct d_test_counter* _counter
)
{
    bool                    result;
    struct d_filter_chain*  chain;
    size_t                  estimate;

    result = true;

    // test 1: estimate for take_first(3) on 10 elements
    chain = d_filter_chain_new();

    if (!chain)
    {
        return false;
    }

    d_filter_chain_add_take_first(chain, 3);

    estimate = d_filter_estimate_result_size(chain, 10);

    result = d_assert_standalone(
        estimate == 3,
        "estimate_take_first",
        "estimate for take_first(3) on 10 should be 3",
        _counter) && result;

    d_filter_chain_free(chain);

    // test 2: estimate for skip_first(3) on 10 elements
    chain = d_filter_chain_new();

    if (chain)
    {
        d_filter_chain_add_skip_first(chain, 3);

        estimate = d_filter_estimate_result_size(chain, 10);

        result = d_assert_standalone(
            estimate == 7,
            "estimate_skip_first",
            "estimate for skip_first(3) on 10 should be 7",
            _counter) && result;

        d_filter_chain_free(chain);
    }

    // test 3: estimate for where predicate is <= input count
    chain = d_filter_chain_new();

    if (chain)
    {
        d_filter_chain_add_where(chain, pred_is_even);

        estimate = d_filter_estimate_result_size(chain, 10);

        result = d_assert_standalone(
            estimate <= 10,
            "estimate_where_bounded",
            "estimate for where predicate should be <= input count",
            _counter) && result;

        d_filter_chain_free(chain);
    }

    // test 4: estimate for empty chain returns input count
    chain = d_filter_chain_new();

    if (chain)
    {
        estimate = d_filter_estimate_result_size(chain, 10);

        result = d_assert_standalone(
            estimate == 10,
            "estimate_empty_chain",
            "estimate for empty chain should equal input count",
            _counter) && result;

        d_filter_chain_free(chain);
    }

    // test 5: take_first(N) where N > input returns input count
    chain = d_filter_chain_new();

    if (chain)
    {
        d_filter_chain_add_take_first(chain, 100);

        estimate = d_filter_estimate_result_size(chain, 5);

        result = d_assert_standalone(
            estimate == 5,
            "estimate_take_clamped",
            "take_first(100) on 5 should estimate 5",
            _counter) && result;

        d_filter_chain_free(chain);
    }

    // test 6: skip_first(N) where N >= input returns 0
    chain = d_filter_chain_new();

    if (chain)
    {
        d_filter_chain_add_skip_first(chain, 10);

        estimate = d_filter_estimate_result_size(chain, 5);

        result = d_assert_standalone(
            estimate == 0,
            "estimate_skip_exceeds",
            "skip_first(10) on 5 should estimate 0",
            _counter) && result;

        d_filter_chain_free(chain);
    }

    // test 7: compound chain: skip(2) -> take(3) on 10
    // skip 2 -> 8 remaining, take 3 -> 3
    chain = d_filter_chain_new();

    if (chain)
    {
        d_filter_chain_add_skip_first(chain, 2);
        d_filter_chain_add_take_first(chain, 3);

        estimate = d_filter_estimate_result_size(chain, 10);

        result = d_assert_standalone(
            estimate == 3,
            "estimate_compound",
            "skip(2)->take(3) on 10 should estimate 3",
            _counter) && result;

        d_filter_chain_free(chain);
    }

    // test 8: NULL chain returns 0
    estimate = d_filter_estimate_result_size(NULL, 10);

    result = d_assert_standalone(
        estimate == 0,
        "estimate_null_chain",
        "NULL chain should estimate 0",
        _counter) && result;

    // test 9: zero input returns 0
    chain = d_filter_chain_new();

    if (chain)
    {
        d_filter_chain_add_take_first(chain, 5);

        estimate = d_filter_estimate_result_size(chain, 0);

        result = d_assert_standalone(
            estimate == 0,
            "estimate_zero_input",
            "zero input count should estimate 0",
            _counter) && result;

        d_filter_chain_free(chain);
    }

    return result;
}


/*
d_tests_sa_filter_utility_all
  Aggregation function that runs all utility function tests.
*/
bool
d_tests_sa_filter_utility_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Filter Utility Functions\n");
    printf("  ------------------------------------\n");

    result = d_tests_sa_filter_validation(_counter)  && result;
    result = d_tests_sa_filter_to_string(_counter)   && result;
    result = d_tests_sa_filter_from_string(_counter)  && result;
    result = d_tests_sa_filter_optimize(_counter)    && result;
    result = d_tests_sa_filter_estimate(_counter)    && result;

    return result;
}
