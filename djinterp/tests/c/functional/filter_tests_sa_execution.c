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
d_tests_sa_filter_apply_operation
  Tests d_filter_apply_operation for single operation execution.
  Tests the following:
  - take_first(3) on a 5-element array returns first 3
  - take_last(2) on a 5-element array returns last 2
  - skip_first(2) on a 5-element array returns last 3
  - where predicate filters correctly
  - range [1,4) selects correct elements
  - reverse reverses element order
  - result status is D_FILTER_RESULT_SUCCESS on success
  - NULL operation returns error result
  - NULL input returns error result
  - zero count returns empty result
  - zero element_size returns error result
*/
bool
d_tests_sa_filter_apply_operation
(
    struct d_test_counter* _counter
)
{
    bool                      result;
    struct d_filter_operation* op;
    struct d_filter_result*   res;
    int                       input[5] = {10,20,30,40,50};
    int*                      elems;

    result   = true;

    // test 1: take_first(3) returns first 3 elements
    op  = d_filter_take_first(3);
    res = d_filter_apply_operation(op,
                                   input,
                                   5,
                                   sizeof(int));

    result = d_assert_standalone(
        res->status == D_FILTER_RESULT_SUCCESS,
        "apply_op_take_first_status",
        "take_first(3) should return SUCCESS status",
        _counter) && result;

    result = d_assert_standalone(
        res->count == 3,
        "apply_op_take_first_count",
        "take_first(3) should produce 3 elements",
        _counter) && result;

    if ( (res->elements) &&
         (res->count == 3) )
    {
        elems  = (int*)res->elements;
        result = d_assert_standalone(
            (elems[0] == 10) &&
            (elems[1] == 20) &&
            (elems[2] == 30),
            "apply_op_take_first_values",
            "take_first(3) should return {10, 20, 30}",
            _counter) && result;
    }

    d_filter_result_free(res);
    free(res);

    // test 2: take_last(2) returns last 2 elements
    op  = d_filter_take_last(2);
    res = d_filter_apply_operation(op,
                                   input,
                                   5,
                                   sizeof(int));

    result = d_assert_standalone(
        res->count == 2,
        "apply_op_take_last_count",
        "take_last(2) should produce 2 elements",
        _counter) && result;

    if ( (res->elements) &&
         (res->count == 2) )
    {
        elems  = (int*)res->elements;
        result = d_assert_standalone(
            (elems[0] == 40) &&
            (elems[1] == 50),
            "apply_op_take_last_values",
            "take_last(2) should return {40, 50}",
            _counter) && result;
    }

    d_filter_result_free(res);
    free(res);

    // test 3: skip_first(2) returns last 3 elements
    op  = d_filter_skip_first(2);
    res = d_filter_apply_operation(op,
                                   input,
                                   5,
                                   sizeof(int));

    result = d_assert_standalone(
        res->count == 3,
        "apply_op_skip_first_count",
        "skip_first(2) should produce 3 elements",
        _counter) && result;

    if ( (res->elements) &&
         (res->count == 3) )
    {
        elems  = (int*)res->elements;
        result = d_assert_standalone(
            (elems[0] == 30) &&
            (elems[1] == 40) &&
            (elems[2] == 50),
            "apply_op_skip_first_values",
            "skip_first(2) should return {30, 40, 50}",
            _counter) && result;
    }

    d_filter_result_free(res);
    free(res);

    // test 4: where predicate filters even elements
    // all of {10,20,30,40,50} are even, so all should pass
    op  = d_filter_where(pred_is_even);
    res = d_filter_apply_operation(op,
                                   input,
                                   5,
                                   sizeof(int));

    result = d_assert_standalone(
        res->status == D_FILTER_RESULT_SUCCESS,
        "apply_op_where_status",
        "where(is_even) should return SUCCESS",
        _counter) && result;

    result = d_assert_standalone(
        res->count == 5,
        "apply_op_where_count",
        "where(is_even) on {10,20,30,40,50} should produce 5",
        _counter) && result;

    if ( (res->elements) &&
         (res->count == 5) )
    {
        elems  = (int*)res->elements;
        result = d_assert_standalone(
            (elems[0] == 10) &&
            (elems[1] == 20) &&
            (elems[2] == 30) &&
            (elems[3] == 40) &&
            (elems[4] == 50),
            "apply_op_where_values",
            "where(is_even) should return all elements",
            _counter) && result;
    }

    d_filter_result_free(res);
    free(res);

    // test 5: range [1, 4) selects elements at indices 1, 2, 3
    op  = d_filter_range(1, 4);
    res = d_filter_apply_operation(op,
                                   input,
                                   5,
                                   sizeof(int));

    result = d_assert_standalone(
        res->count == 3,
        "apply_op_range_count",
        "range(1,4) should produce 3 elements",
        _counter) && result;

    if ( (res->elements) &&
         (res->count == 3) )
    {
        elems  = (int*)res->elements;
        result = d_assert_standalone(
            (elems[0] == 20) &&
            (elems[1] == 30) &&
            (elems[2] == 40),
            "apply_op_range_values",
            "range(1,4) should return {20, 30, 40}",
            _counter) && result;
    }

    d_filter_result_free(res);
    free(res);

    // test 6: reverse reverses order
    op  = d_filter_reverse();
    res = d_filter_apply_operation(op,
                                   input,
                                   5,
                                   sizeof(int));

    result = d_assert_standalone(
        res->count == 5,
        "apply_op_reverse_count",
        "reverse should produce 5 elements",
        _counter) && result;

    if ( (res->elements) &&
         (res->count == 5) )
    {
        elems  = (int*)res->elements;
        result = d_assert_standalone(
            (elems[0] == 50) &&
            (elems[1] == 40) &&
            (elems[2] == 30) &&
            (elems[3] == 20) &&
            (elems[4] == 10),
            "apply_op_reverse_values",
            "reverse should return {50, 40, 30, 20, 10}",
            _counter) && result;
    }

    d_filter_result_free(res);
    free(res);

    // test 7: NULL operation returns error
    res = d_filter_apply_operation(NULL,
                                   input,
                                   5,
                                   sizeof(int));

    result = d_assert_standalone(
        (res->status == D_FILTER_RESULT_ERROR) ||
        (res->status == D_FILTER_RESULT_INVALID),
        "apply_op_null_op",
        "NULL operation should return error/invalid status",
        _counter) && result;

    result = d_assert_standalone(
        res->count == 0,
        "apply_op_null_op_count",
        "NULL operation should produce 0 elements",
        _counter) && result;

    d_filter_result_free(res);
    free(res);

    // test 8: NULL input returns error
    res = d_filter_apply_operation(op,
                                   NULL,
                                   5,
                                   sizeof(int));

    result = d_assert_standalone(
        (res->status == D_FILTER_RESULT_ERROR) ||
        (res->status == D_FILTER_RESULT_INVALID),
        "apply_op_null_input",
        "NULL input should return error/invalid status",
        _counter) && result;

    d_filter_result_free(res);
    free(res);

    // test 9: zero count returns empty result
    op  = d_filter_take_first(3);
    res = d_filter_apply_operation(op,
                                   input,
                                   0,
                                   sizeof(int));

    result = d_assert_standalone(
        res->count == 0,
        "apply_op_zero_count",
        "zero count should produce 0 elements",
        _counter) && result;

    d_filter_result_free(res);
    free(res);

    // test 10: zero element_size returns error
    res = d_filter_apply_operation(op,
                                   input,
                                   5,
                                   0);

    result = d_assert_standalone(
        (res->status == D_FILTER_RESULT_ERROR) ||
        (res->status == D_FILTER_RESULT_INVALID),
        "apply_op_zero_elem_size",
        "zero element_size should return error/invalid status",
        _counter) && result;

    d_filter_result_free(res);
    free(res);

    return result;
}


/*
d_tests_sa_filter_apply_chain
  Tests d_filter_apply_chain for chained operation execution.
  Tests the following:
  - empty chain returns all elements unchanged
  - single-operation chain behaves like apply_operation
  - multi-operation chain (skip 1 -> take 3) composes correctly
  - chain with where predicate filters elements
  - chain with skip + where + take composes correctly
  - NULL chain returns error result
  - NULL input returns error result
  - zero count returns empty result
*/
bool
d_tests_sa_filter_apply_chain
(
    struct d_test_counter* _counter
)
{
    struct d_filter_chain*   chain;
    struct d_filter_result*  res;
    int                      input[6] = { 1,2,3,4,5,6 };
    int*                     elems;
    bool                     result;

    result   = true;

    // test 1: empty chain returns all elements
    chain = d_filter_chain_new();

    if (chain)
    {
        res = d_filter_apply_chain(chain,
                                   input,
                                   6,
                                   sizeof(int));

        result = d_assert_standalone(
            res->status == D_FILTER_RESULT_SUCCESS,
            "apply_chain_empty_status",
            "empty chain should return SUCCESS",
            _counter) && result;

        result = d_assert_standalone(
            res->count == 6,
            "apply_chain_empty_count",
            "empty chain should return all 6 elements",
            _counter) && result;

        d_filter_result_free(res);
    free(res);
        d_filter_chain_free(chain);
    }

    // test 2: single-operation chain (take_first 3)
    chain = d_filter_chain_new();

    if (chain)
    {
        d_filter_chain_add_take_first(chain, 3);

        res = d_filter_apply_chain(chain,
                                   input,
                                   6,
                                   sizeof(int));

        result = d_assert_standalone(
            res->count == 3,
            "apply_chain_single_count",
            "chain with take_first(3) should produce 3",
            _counter) && result;

        if ( (res->elements) &&
             (res->count == 3) )
        {
            elems  = (int*)res->elements;
            result = d_assert_standalone(
                (elems[0] == 1) &&
                (elems[1] == 2) &&
                (elems[2] == 3),
                "apply_chain_single_values",
                "take_first(3) should return {1, 2, 3}",
                _counter) && result;
        }

        d_filter_result_free(res);
    free(res);
        d_filter_chain_free(chain);
    }

    // test 3: multi-operation chain: skip(1) -> take(3)
    // input: {1, 2, 3, 4, 5, 6} -> skip 1 -> {2,3,4,5,6} -> take 3 -> {2,3,4}
    chain = d_filter_chain_new();

    if (chain)
    {
        d_filter_chain_add_skip_first(chain, 1);
        d_filter_chain_add_take_first(chain, 3);

        res = d_filter_apply_chain(chain,
                                   input,
                                   6,
                                   sizeof(int));

        result = d_assert_standalone(
            res->count == 3,
            "apply_chain_skip_take_count",
            "skip(1)->take(3) should produce 3",
            _counter) && result;

        if ( (res->elements) &&
             (res->count == 3) )
        {
            elems  = (int*)res->elements;
            result = d_assert_standalone(
                (elems[0] == 2) &&
                (elems[1] == 3) &&
                (elems[2] == 4),
                "apply_chain_skip_take_values",
                "skip(1)->take(3) should return {2, 3, 4}",
                _counter) && result;
        }

        d_filter_result_free(res);
    free(res);
        d_filter_chain_free(chain);
    }

    // test 4: chain with where predicate
    // input: {1,2,3,4,5,6} -> where(is_even) -> {2, 4, 6}
    chain = d_filter_chain_new();

    if (chain)
    {
        d_filter_chain_add_where(chain, pred_is_even);

        res = d_filter_apply_chain(chain,
                                   input,
                                   6,
                                   sizeof(int));

        result = d_assert_standalone(
            res->count == 3,
            "apply_chain_where_count",
            "where(is_even) on {1..6} should produce 3",
            _counter) && result;

        if ( (res->elements) &&
             (res->count == 3) )
        {
            elems  = (int*)res->elements;
            result = d_assert_standalone(
                (elems[0] == 2) &&
                (elems[1] == 4) &&
                (elems[2] == 6),
                "apply_chain_where_values",
                "where(is_even) should return {2, 4, 6}",
                _counter) && result;
        }

        d_filter_result_free(res);
    free(res);
        d_filter_chain_free(chain);
    }

    // test 5: compound chain: skip(1) -> where(is_even) -> take(2)
    // input: {1,2,3,4,5,6} -> skip 1 -> {2,3,4,5,6}
    //        -> where(is_even) -> {2,4,6} -> take 2 -> {2, 4}
    chain = d_filter_chain_new();

    if (chain)
    {
        d_filter_chain_add_skip_first(chain, 1);
        d_filter_chain_add_where(chain, pred_is_even);
        d_filter_chain_add_take_first(chain, 2);

        res = d_filter_apply_chain(chain,
                                   input,
                                   6,
                                   sizeof(int));

        result = d_assert_standalone(
            res->count == 2,
            "apply_chain_compound_count",
            "skip(1)->where(even)->take(2) should produce 2",
            _counter) && result;

        if ( (res->elements) &&
             (res->count == 2) )
        {
            elems  = (int*)res->elements;
            result = d_assert_standalone(
                (elems[0] == 2) &&
                (elems[1] == 4),
                "apply_chain_compound_values",
                "compound chain should return {2, 4}",
                _counter) && result;
        }

        d_filter_result_free(res);
    free(res);
        d_filter_chain_free(chain);
    }

    // test 6: NULL chain returns error
    res = d_filter_apply_chain(NULL,
                               input,
                               6,
                               sizeof(int));

    result = d_assert_standalone(
        (res->status == D_FILTER_RESULT_ERROR) ||
        (res->status == D_FILTER_RESULT_INVALID),
        "apply_chain_null_chain",
        "NULL chain should return error/invalid status",
        _counter) && result;

    d_filter_result_free(res);
    free(res);

    // test 7: NULL input returns error
    chain = d_filter_chain_new();

    if (chain)
    {
        d_filter_chain_add_take_first(chain, 3);

        res = d_filter_apply_chain(chain,
                                   NULL,
                                   6,
                                   sizeof(int));

        result = d_assert_standalone(
            (res->status == D_FILTER_RESULT_ERROR) ||
            (res->status == D_FILTER_RESULT_INVALID),
            "apply_chain_null_input",
            "NULL input should return error/invalid status",
            _counter) && result;

        d_filter_result_free(res);
    free(res);
        d_filter_chain_free(chain);
    }

    // test 8: zero count returns empty result
    chain = d_filter_chain_new();

    if (chain)
    {
        d_filter_chain_add_take_first(chain, 3);

        res = d_filter_apply_chain(chain,
                                   input,
                                   0,
                                   sizeof(int));

        result = d_assert_standalone(
            res->count == 0,
            "apply_chain_zero_count",
            "zero count should produce 0 elements",
            _counter) && result;

        d_filter_result_free(res);
    free(res);
        d_filter_chain_free(chain);
    }

    return result;
}


/*
d_tests_sa_filter_apply_combinators
  Tests d_filter_apply_union, d_filter_apply_intersection,
  d_filter_apply_difference.
  Tests the following:
  - union of two chains produces OR semantics
  - intersection of two chains produces AND semantics
  - difference produces include-but-not-exclude semantics
  - NULL combinator returns error
  - empty combinator returns appropriate result
*/
bool
d_tests_sa_filter_apply_combinators
(
    struct d_test_counter* _counter
)
{
    struct d_filter_union*        u;
    struct d_filter_intersection* inter;
    struct d_filter_difference*   diff;
    struct d_filter_chain*        chain_even;
    struct d_filter_chain*        chain_positive;
    struct d_filter_result*       res;
    int                           input[6] = { -4, -1, 0, 3, 4, 7 };
    int*                          elems;
    size_t                        i;
    bool                          found;
    bool                          result;

    result   = true;

    // setup: create even chain and positive chain
    chain_even     = d_filter_chain_new();
    chain_positive = d_filter_chain_new();

    if ( (!chain_even) ||
         (!chain_positive) )
    {
        d_filter_chain_free(chain_even);
        d_filter_chain_free(chain_positive);

        return false;
    }

    d_filter_chain_add_where(chain_even, pred_is_even);
    d_filter_chain_add_where(chain_positive, pred_is_positive);

    // test 1: union (even OR positive)
    // even: {-4, 0, 4}  positive: {3, 4, 7}  union: {-4, 0, 3, 4, 7}
    u = d_filter_union_new(2);

    if (u)
    {
        d_filter_union_add(u, chain_even);
        d_filter_union_add(u, chain_positive);

        res = d_filter_apply_union(u,
                                   input,
                                   6,
                                   sizeof(int));

        result = d_assert_standalone(
            res->status == D_FILTER_RESULT_SUCCESS,
            "apply_union_status",
            "union apply should return SUCCESS",
            _counter) && result;

        result = d_assert_standalone(
            res->count == 5,
            "apply_union_count",
            "union(even, positive) should produce 5 elements",
            _counter) && result;

        if ( (res->elements) &&
             (res->count >= 1) )
        {
            // verify -1 is NOT in results (neither even nor positive)
            elems = (int*)res->elements;
            found = false;

            for (i = 0; i < res->count; i++)
            {
                if (elems[i] == -1)
                {
                    found = true;
                }
            }

            result = d_assert_standalone(
                found == false,
                "apply_union_excludes_neg1",
                "union should exclude -1 (neither even nor positive)",
                _counter) && result;
        }

        d_filter_result_free(res);
    free(res);
        d_filter_union_free(u);
    }

    // test 2: intersection (even AND positive)
    // even: {-4, 0, 4}  positive: {3, 4, 7}  intersection: {4}
    inter = d_filter_intersection_new(2);

    if (inter)
    {
        d_filter_intersection_add(inter, chain_even);
        d_filter_intersection_add(inter, chain_positive);

        res = d_filter_apply_intersection(inter,
                                          input,
                                          6,
                                          sizeof(int));

        result = d_assert_standalone(
            res->status == D_FILTER_RESULT_SUCCESS,
            "apply_intersection_status",
            "intersection apply should return SUCCESS",
            _counter) && result;

        result = d_assert_standalone(
            res->count == 1,
            "apply_intersection_count",
            "intersection(even, positive) should produce 1",
            _counter) && result;

        if ( (res->elements) &&
             (res->count == 1) )
        {
            elems  = (int*)res->elements;
            result = d_assert_standalone(
                elems[0] == 4,
                "apply_intersection_value",
                "intersection should return {4}",
                _counter) && result;
        }

        d_filter_result_free(res);
    free(res);
        d_filter_intersection_free(inter);
    }

    // test 3: difference (even BUT NOT positive)
    // even: {-4, 0, 4}  positive: {3, 4, 7}  diff: {-4, 0}
    diff = d_filter_difference_new(chain_even, chain_positive);

    if (diff)
    {
        res = d_filter_apply_difference(diff,
                                        input,
                                        6,
                                        sizeof(int));

        result = d_assert_standalone(
            res->status == D_FILTER_RESULT_SUCCESS,
            "apply_difference_status",
            "difference apply should return SUCCESS",
            _counter) && result;

        result = d_assert_standalone(
            res->count == 2,
            "apply_difference_count",
            "difference(even, positive) should produce 2",
            _counter) && result;

        if ( (res->elements) &&
             (res->count == 2) )
        {
            elems  = (int*)res->elements;
            result = d_assert_standalone(
                (elems[0] == -4) &&
                (elems[1] == 0),
                "apply_difference_values",
                "difference should return {-4, 0}",
                _counter) && result;
        }

        d_filter_result_free(res);
    free(res);
        d_filter_difference_free(diff);
    }

    // test 4: NULL union returns error
    res = d_filter_apply_union(NULL,
                               input,
                               6,
                               sizeof(int));

    result = d_assert_standalone(
        (res->status == D_FILTER_RESULT_ERROR) ||
        (res->status == D_FILTER_RESULT_INVALID),
        "apply_union_null",
        "NULL union should return error/invalid",
        _counter) && result;

    d_filter_result_free(res);
    free(res);

    // test 5: NULL intersection returns error
    res = d_filter_apply_intersection(NULL,
                                      input,
                                      6,
                                      sizeof(int));

    result = d_assert_standalone(
        (res->status == D_FILTER_RESULT_ERROR) ||
        (res->status == D_FILTER_RESULT_INVALID),
        "apply_intersection_null",
        "NULL intersection should return error/invalid",
        _counter) && result;

    d_filter_result_free(res);
    free(res);

    // test 6: NULL difference returns error
    res = d_filter_apply_difference(NULL,
                                    input,
                                    6,
                                    sizeof(int));

    result = d_assert_standalone(
        (res->status == D_FILTER_RESULT_ERROR) ||
        (res->status == D_FILTER_RESULT_INVALID),
        "apply_difference_null",
        "NULL difference should return error/invalid",
        _counter) && result;

    d_filter_result_free(res);
    free(res);

    // cleanup shared chains
    d_filter_chain_free(chain_even);
    d_filter_chain_free(chain_positive);

    return result;
}


/*
d_tests_sa_filter_counting
  Tests d_filter_count_matches, d_filter_any_match, d_filter_all_match,
  d_filter_none_match.
  Tests the following:
  - count_matches returns correct count for where(is_even)
  - count_matches with no matches returns 0
  - count_matches with all matches returns full count
  - any_match returns true when some match
  - any_match returns false when none match
  - all_match returns true when all match
  - all_match returns false when some fail
  - none_match returns true when none match
  - none_match returns false when some match
  - NULL chain returns 0/false for each
*/
bool
d_tests_sa_filter_counting
(
    struct d_test_counter* _counter
)
{
    struct d_filter_chain*  chain;
    int                     mixed[6] = { 1,2,3,4,5,6 };
    int                     all_even[4] = {2,4,6,8};
    int                     all_odd[3] = {1,3,5};
    bool                    result;

    result      = true;
    
    chain = d_filter_chain_new();

    if (!chain)
    {
        return false;
    }

    d_filter_chain_add_where(chain, pred_is_even);

    // test 1: count_matches on mixed array
    result = d_assert_standalone(
        d_filter_count_matches(chain,
                               mixed,
                               6,
                               sizeof(int)) == 3,
        "count_matches_mixed",
        "count_matches(is_even) on {1..6} should be 3",
        _counter) && result;

    // test 2: count_matches when no matches
    result = d_assert_standalone(
        d_filter_count_matches(chain,
                               all_odd,
                               3,
                               sizeof(int)) == 0,
        "count_matches_none",
        "count_matches(is_even) on all odd should be 0",
        _counter) && result;

    // test 3: count_matches when all match
    result = d_assert_standalone(
        d_filter_count_matches(chain,
                               all_even,
                               4,
                               sizeof(int)) == 4,
        "count_matches_all",
        "count_matches(is_even) on all even should be 4",
        _counter) && result;

    // test 4: any_match on mixed (some match)
    result = d_assert_standalone(
        d_filter_any_match(chain,
                           mixed,
                           6,
                           sizeof(int)) == true,
        "any_match_true",
        "any_match(is_even) on mixed should be true",
        _counter) && result;

    // test 5: any_match when none match
    result = d_assert_standalone(
        d_filter_any_match(chain,
                           all_odd,
                           3,
                           sizeof(int)) == false,
        "any_match_false",
        "any_match(is_even) on all odd should be false",
        _counter) && result;

    // test 6: all_match when all match
    result = d_assert_standalone(
        d_filter_all_match(chain,
                           all_even,
                           4,
                           sizeof(int)) == true,
        "all_match_true",
        "all_match(is_even) on all even should be true",
        _counter) && result;

    // test 7: all_match when not all match
    result = d_assert_standalone(
        d_filter_all_match(chain,
                           mixed,
                           6,
                           sizeof(int)) == false,
        "all_match_false",
        "all_match(is_even) on mixed should be false",
        _counter) && result;

    // test 8: none_match when none match
    result = d_assert_standalone(
        d_filter_none_match(chain,
                            all_odd,
                            3,
                            sizeof(int)) == true,
        "none_match_true",
        "none_match(is_even) on all odd should be true",
        _counter) && result;

    // test 9: none_match when some match
    result = d_assert_standalone(
        d_filter_none_match(chain,
                            mixed,
                            6,
                            sizeof(int)) == false,
        "none_match_false",
        "none_match(is_even) on mixed should be false",
        _counter) && result;

    d_filter_chain_free(chain);

    // test 10: NULL chain returns 0 for count_matches
    result = d_assert_standalone(
        d_filter_count_matches(NULL,
                               mixed,
                               6,
                               sizeof(int)) == 0,
        "count_matches_null_chain",
        "count_matches with NULL chain should be 0",
        _counter) && result;

    // test 11: NULL chain returns false for any_match
    result = d_assert_standalone(
        d_filter_any_match(NULL,
                           mixed,
                           6,
                           sizeof(int)) == false,
        "any_match_null_chain",
        "any_match with NULL chain should be false",
        _counter) && result;

    // test 12: NULL chain returns false for all_match
    result = d_assert_standalone(
        d_filter_all_match(NULL,
                           mixed,
                           6,
                           sizeof(int)) == false,
        "all_match_null_chain",
        "all_match with NULL chain should be false",
        _counter) && result;

    // test 13: NULL chain returns true/false for none_match
    // (implementation-dependent; typically true or false)
    result = d_assert_standalone(
        d_filter_none_match(NULL,
                            mixed,
                            6,
                            sizeof(int)) == true,
        "none_match_null_chain",
        "none_match with NULL chain should be true",
        _counter) && result;

    return result;
}


/*
d_tests_sa_filter_get_indices
  Tests d_filter_get_indices for index retrieval without element copying.
  Tests the following:
  - returns correct indices for where(is_even) on mixed array
  - out_count is set correctly
  - returned indices are a new allocation (caller must free)
  - NULL chain returns NULL
  - NULL input returns NULL
  - zero count returns NULL
  - NULL out_count returns NULL
*/
bool
d_tests_sa_filter_get_indices
(
    struct d_test_counter* _counter
)
{
    struct d_filter_chain*  chain;
    size_t*                 indices;
    size_t                  out_count;
    int                     input[6] = { 1,2,3,4,5,6 };
    bool                    result;

    result   = true;

    chain = d_filter_chain_new();

    if (!chain)
    {
        return false;
    }

    d_filter_chain_add_where(chain, pred_is_even);

    // test 1: get indices for even elements
    out_count = 0;
    indices   = d_filter_get_indices(chain,
                                     input,
                                     6,
                                     sizeof(int),
                                     &out_count);

    result = d_assert_standalone(
        indices != NULL,
        "get_indices_not_null",
        "get_indices should return non-NULL for matching elements",
        _counter) && result;

    result = d_assert_standalone(
        out_count == 3,
        "get_indices_count",
        "get_indices(is_even) on {1..6} should return 3 indices",
        _counter) && result;

    if ( (indices) &&
         (out_count == 3) )
    {
        result = d_assert_standalone(
            (indices[0] == 1) &&
            (indices[1] == 3) &&
            (indices[2] == 5),
            "get_indices_values",
            "even indices in {1,2,3,4,5,6} are {1, 3, 5}",
            _counter) && result;
    }

    free(indices);

    d_filter_chain_free(chain);

    // test 2: NULL chain returns NULL
    out_count = 99;
    indices   = d_filter_get_indices(NULL,
                                     input,
                                     6,
                                     sizeof(int),
                                     &out_count);

    result = d_assert_standalone(
        indices == NULL,
        "get_indices_null_chain",
        "NULL chain should return NULL",
        _counter) && result;

    // test 3: NULL input returns NULL
    chain = d_filter_chain_new();

    if (chain)
    {
        d_filter_chain_add_where(chain, pred_is_even);

        indices = d_filter_get_indices(chain,
                                       NULL,
                                       6,
                                       sizeof(int),
                                       &out_count);

        result = d_assert_standalone(
            indices == NULL,
            "get_indices_null_input",
            "NULL input should return NULL",
            _counter) && result;

        d_filter_chain_free(chain);
    }

    // test 4: zero count returns NULL
    chain = d_filter_chain_new();

    if (chain)
    {
        d_filter_chain_add_where(chain, pred_is_even);

        indices = d_filter_get_indices(chain,
                                       input,
                                       0,
                                       sizeof(int),
                                       &out_count);

        result = d_assert_standalone(
            indices == NULL,
            "get_indices_zero_count",
            "zero count should return NULL",
            _counter) && result;

        d_filter_chain_free(chain);
    }

    // test 5: NULL out_count returns NULL
    chain = d_filter_chain_new();

    if (chain)
    {
        d_filter_chain_add_where(chain, pred_is_even);

        indices = d_filter_get_indices(chain,
                                       input,
                                       6,
                                       sizeof(int),
                                       NULL);

        result = d_assert_standalone(
            indices == NULL,
            "get_indices_null_out_count",
            "NULL out_count should return NULL",
            _counter) && result;

        d_filter_chain_free(chain);
    }

    return result;
}


/*
d_tests_sa_filter_in_place
  Tests d_filter_apply_in_place for in-place array modification.
  Tests the following:
  - in-place filter keeps only matching elements
  - returns new count of remaining elements
  - elements are compacted (no gaps)
  - all elements pass: array unchanged, returns full count
  - no elements pass: returns 0
  - NULL chain returns 0
  - NULL array returns 0
  - zero count returns 0
*/
bool
d_tests_sa_filter_in_place
(
    struct d_test_counter* _counter
)
{
    bool                    result;
    struct d_filter_chain*  chain;
    int                     data[6] = { 1,2,3,4,5,6 };
    size_t                  new_count;

    result = true;

    // test 1: in-place filter keeps even elements

    chain = d_filter_chain_new();

    if (!chain)
    {
        return false;
    }

    d_filter_chain_add_where(chain, pred_is_even);

    new_count = d_filter_apply_in_place(chain,
                                        data,
                                        6,
                                        sizeof(int));

    result = d_assert_standalone(
        new_count == 3,
        "in_place_even_count",
        "in-place filter(is_even) on {1..6} should return 3",
        _counter) && result;

    result = d_assert_standalone(
        (data[0] == 2) &&
        (data[1] == 4) &&
        (data[2] == 6),
        "in_place_even_values",
        "in-place should compact to {2, 4, 6}",
        _counter) && result;

    d_filter_chain_free(chain);

    // test 2: all elements pass — array unchanged
    data[0] = 2;
    data[1] = 4;
    data[2] = 6;
    data[3] = 8;

    chain = d_filter_chain_new();

    if (chain)
    {
        d_filter_chain_add_where(chain, pred_is_even);

        new_count = d_filter_apply_in_place(chain,
                                            data,
                                            4,
                                            sizeof(int));

        result = d_assert_standalone(
            new_count == 4,
            "in_place_all_pass_count",
            "all even should return full count 4",
            _counter) && result;

        result = d_assert_standalone(
            (data[0] == 2) &&
            (data[1] == 4) &&
            (data[2] == 6) &&
            (data[3] == 8),
            "in_place_all_pass_values",
            "all even should leave array unchanged",
            _counter) && result;

        d_filter_chain_free(chain);
    }

    // test 3: no elements pass — returns 0
    data[0] = 1;
    data[1] = 3;
    data[2] = 5;

    chain = d_filter_chain_new();

    if (chain)
    {
        d_filter_chain_add_where(chain, pred_is_even);

        new_count = d_filter_apply_in_place(chain,
                                            data,
                                            3,
                                            sizeof(int));

        result = d_assert_standalone(
            new_count == 0,
            "in_place_none_pass",
            "no even elements should return 0",
            _counter) && result;

        d_filter_chain_free(chain);
    }

    // test 4: NULL chain returns 0
    new_count = d_filter_apply_in_place(NULL,
                                        data,
                                        3,
                                        sizeof(int));

    result = d_assert_standalone(
        new_count == 0,
        "in_place_null_chain",
        "NULL chain should return 0",
        _counter) && result;

    // test 5: NULL array returns 0
    chain = d_filter_chain_new();

    if (chain)
    {
        d_filter_chain_add_where(chain, pred_is_even);

        new_count = d_filter_apply_in_place(chain,
                                            NULL,
                                            6,
                                            sizeof(int));

        result = d_assert_standalone(
            new_count == 0,
            "in_place_null_array",
            "NULL array should return 0",
            _counter) && result;

        d_filter_chain_free(chain);
    }

    // test 6: zero count returns 0
    chain = d_filter_chain_new();

    if (chain)
    {
        d_filter_chain_add_where(chain, pred_is_even);

        new_count = d_filter_apply_in_place(chain,
                                            data,
                                            0,
                                            sizeof(int));

        result = d_assert_standalone(
            new_count == 0,
            "in_place_zero_count",
            "zero count should return 0",
            _counter) && result;

        d_filter_chain_free(chain);
    }

    return result;
}


/*
d_tests_sa_filter_result_free
  Tests d_filter_result_free for proper cleanup.
  Tests the following:
  - freeing result with elements and indices NULLs them
  - freeing result with error_message NULLs it
  - freeing result with no allocations does not crash
  - freeing NULL pointer does not crash
*/
bool
d_tests_sa_filter_result_free
(
    struct d_test_counter* _counter
)
{
    struct d_filter_operation* op;
    struct d_filter_result*    res;
    int                        input[3] = { 10,20,30 };
    bool                       result;

    result   = true;

    // test 1: free result with allocated elements
    op  = d_filter_take_first(2);
    res = d_filter_apply_operation(op,
                                   input,
                                   3,
                                   sizeof(int));

    result = d_assert_standalone(
        res->elements != NULL,
        "result_free_has_elements",
        "result should have allocated elements",
        _counter) && result;

    d_filter_result_free(res);

    result = d_assert_standalone(
        res->elements == NULL,
        "result_free_elements_nulled",
        "free should NULL the elements pointer",
        _counter) && result;

    result = d_assert_standalone(
        res->indices == NULL,
        "result_free_indices_nulled",
        "free should NULL the indices pointer",
        _counter) && result;

    result = d_assert_standalone(
        res->count == 0,
        "result_free_count_zeroed",
        "free should zero the count",
        _counter) && result;

    // test 2: free empty/zeroed result does not crash
    d_memset(&res, 0, sizeof(res));
    d_filter_result_free(res);
    free(res);

    result = d_assert_standalone(
        true,
        "result_free_empty",
        "free on zeroed result should not crash",
        _counter) && result;

    // test 3: free NULL pointer does not crash
    d_filter_result_free(NULL);

    result = d_assert_standalone(
        true,
        "result_free_null",
        "free(NULL) should not crash",
        _counter) && result;

    return result;
}


/*
d_tests_sa_filter_matches_element
  Tests d_filter_chain_matches_element for single-element evaluation.
  Tests the following:
  - element matching a WHERE(is_even) chain returns true
  - element not matching a WHERE(is_even) chain returns false
  - WHERE_NOT(is_even) correctly negates
  - compound predicate chain: WHERE(is_even) + WHERE(is_positive)
  - element matching compound predicate returns true
  - element failing one predicate in compound returns false
  - empty chain matches any element
  - positional op: take_first(1) on single element returns true
  - positional op: skip_first(1) on single element returns false
  - mixed chain: skip(0) + WHERE(is_even) on matching element
  - NULL chain returns false
  - NULL element returns false
  - zero element_size returns false
*/
bool
d_tests_sa_filter_matches_element
(
    struct d_test_counter* _counter
)
{
    bool                    result;
    struct d_filter_chain*  chain;
    int                     even_val;
    int                     odd_val;
    int                     neg_even;

    result   = true;
    even_val = 4;
    odd_val  = 7;
    neg_even = -2;

    // test 1: WHERE(is_even) on even element -> true
    chain = d_filter_chain_new();

    if (!chain)
    {
        return false;
    }

    d_filter_chain_add_where(chain, pred_is_even);

    result = d_assert_standalone(
        d_filter_chain_matches_element(chain,
                                       &even_val,
                                       sizeof(int)) == true,
        "matches_elem_where_even_pass",
        "even value should match WHERE(is_even)",
        _counter) && result;

    // test 2: WHERE(is_even) on odd element -> false
    result = d_assert_standalone(
        d_filter_chain_matches_element(chain,
                                       &odd_val,
                                       sizeof(int)) == false,
        "matches_elem_where_even_fail",
        "odd value should not match WHERE(is_even)",
        _counter) && result;

    d_filter_chain_free(chain);

    // test 3: WHERE_NOT(is_even) on odd element -> true
    chain = d_filter_chain_new();

    if (chain)
    {
        struct d_filter_operation* op_wn;

        op_wn = d_filter_where_not(pred_is_even);
        d_filter_chain_add(chain, op_wn);

        result = d_assert_standalone(
            d_filter_chain_matches_element(chain,
                                           &odd_val,
                                           sizeof(int)) == true,
            "matches_elem_where_not_pass",
            "odd value should match WHERE_NOT(is_even)",
            _counter) && result;

        result = d_assert_standalone(
            d_filter_chain_matches_element(chain,
                                           &even_val,
                                           sizeof(int)) == false,
            "matches_elem_where_not_fail",
            "even value should not match WHERE_NOT(is_even)",
            _counter) && result;

        d_filter_chain_free(chain);
    }

    // test 4: compound predicates: WHERE(is_even) + WHERE(is_positive)
    chain = d_filter_chain_new();

    if (chain)
    {
        d_filter_chain_add_where(chain, pred_is_even);
        d_filter_chain_add_where(chain, pred_is_positive);

        // even_val = 4 -> even AND positive -> true
        result = d_assert_standalone(
            d_filter_chain_matches_element(chain,
                                           &even_val,
                                           sizeof(int)) == true,
            "matches_elem_compound_pass",
            "4 should match WHERE(even) + WHERE(positive)",
            _counter) && result;

        // neg_even = -2 -> even but NOT positive -> false
        result = d_assert_standalone(
            d_filter_chain_matches_element(chain,
                                           &neg_even,
                                           sizeof(int)) == false,
            "matches_elem_compound_fail",
            "-2 should not match WHERE(even) + WHERE(positive)",
            _counter) && result;

        // odd_val = 7 -> not even -> false (short-circuits)
        result = d_assert_standalone(
            d_filter_chain_matches_element(chain,
                                           &odd_val,
                                           sizeof(int)) == false,
            "matches_elem_compound_fail_first",
            "7 should not match WHERE(even) + WHERE(positive)",
            _counter) && result;

        d_filter_chain_free(chain);
    }

    // test 5: empty chain matches any element
    chain = d_filter_chain_new();

    if (chain)
    {
        result = d_assert_standalone(
            d_filter_chain_matches_element(chain,
                                           &odd_val,
                                           sizeof(int)) == true,
            "matches_elem_empty_chain",
            "empty chain should match any element",
            _counter) && result;

        d_filter_chain_free(chain);
    }

    // test 6: positional op take_first(1) includes the single element
    chain = d_filter_chain_new();

    if (chain)
    {
        d_filter_chain_add_take_first(chain, 1);

        result = d_assert_standalone(
            d_filter_chain_matches_element(chain,
                                           &even_val,
                                           sizeof(int)) == true,
            "matches_elem_take_first_1",
            "take_first(1) on single element should match",
            _counter) && result;

        d_filter_chain_free(chain);
    }

    // test 7: positional op skip_first(1) excludes the single element
    chain = d_filter_chain_new();

    if (chain)
    {
        d_filter_chain_add_skip_first(chain, 1);

        result = d_assert_standalone(
            d_filter_chain_matches_element(chain,
                                           &even_val,
                                           sizeof(int)) == false,
            "matches_elem_skip_first_1",
            "skip_first(1) on single element should not match",
            _counter) && result;

        d_filter_chain_free(chain);
    }

    // test 8: mixed chain: skip(0) + WHERE(is_even)
    // skip(0) is a no-op, so this tests the general fallback path
    chain = d_filter_chain_new();

    if (chain)
    {
        d_filter_chain_add_skip_first(chain, 0);
        d_filter_chain_add_where(chain, pred_is_even);

        result = d_assert_standalone(
            d_filter_chain_matches_element(chain,
                                           &even_val,
                                           sizeof(int)) == true,
            "matches_elem_mixed_pass",
            "skip(0)+WHERE(even) should match even element",
            _counter) && result;

        result = d_assert_standalone(
            d_filter_chain_matches_element(chain,
                                           &odd_val,
                                           sizeof(int)) == false,
            "matches_elem_mixed_fail",
            "skip(0)+WHERE(even) should not match odd element",
            _counter) && result;

        d_filter_chain_free(chain);
    }

    // test 9: NULL chain returns false
    result = d_assert_standalone(
        d_filter_chain_matches_element(NULL,
                                       &even_val,
                                       sizeof(int)) == false,
        "matches_elem_null_chain",
        "NULL chain should return false",
        _counter) && result;

    // test 10: NULL element returns false
    chain = d_filter_chain_new();

    if (chain)
    {
        d_filter_chain_add_where(chain, pred_is_even);

        result = d_assert_standalone(
            d_filter_chain_matches_element(chain,
                                           NULL,
                                           sizeof(int)) == false,
            "matches_elem_null_element",
            "NULL element should return false",
            _counter) && result;

        d_filter_chain_free(chain);
    }

    // test 11: zero element_size returns false
    chain = d_filter_chain_new();

    if (chain)
    {
        d_filter_chain_add_where(chain, pred_is_even);

        result = d_assert_standalone(
            d_filter_chain_matches_element(chain,
                                           &even_val,
                                           0) == false,
            "matches_elem_zero_size",
            "zero element_size should return false",
            _counter) && result;

        d_filter_chain_free(chain);
    }

    return result;
}


/*
d_tests_sa_filter_execution_all
  Aggregation function that runs all execution and application tests.
*/
bool
d_tests_sa_filter_execution_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Filter Execution & Application\n");
    printf("  ------------------------------------------\n");

    result = d_tests_sa_filter_apply_operation(_counter)   && result;
    result = d_tests_sa_filter_apply_chain(_counter)       && result;
    result = d_tests_sa_filter_apply_combinators(_counter) && result;
    result = d_tests_sa_filter_counting(_counter)          && result;
    result = d_tests_sa_filter_get_indices(_counter)       && result;
    result = d_tests_sa_filter_in_place(_counter)          && result;
    result = d_tests_sa_filter_result_free(_counter)       && result;
    result = d_tests_sa_filter_matches_element(_counter)   && result;

    return result;
}
