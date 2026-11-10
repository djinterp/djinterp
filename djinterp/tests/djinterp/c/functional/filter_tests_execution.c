/*******************************************************************************
* djinterp [functional]                                 filter_tests_execution.c
*
*
* path:      /tests/djinterp/c/functional/filter_tests_execution.c
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


/*
d_tests_filter_apply_operation
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
bool d_tests_filter_apply_operation(
    struct dj_test_suite* _suite
)
{
    bool                      result;
    struct dj_filter_operation* op;
    struct dj_filter_result*   res;
    int                       input[5] = {10,20,30,40,50};
    int*                      elems;

    d_test_suite_test(_suite, "filter_apply_operation");
    op = NULL;

    result   = true;

    // test 1: take_first(3) returns first 3 elements
    op  = d_filter_take_first(3);
    res = d_filter_apply_operation(op,
                                   input,
                                   5,
                                   sizeof(int));

    result = D_TEST_CHECK_MSG(_suite,
                              "apply_op_take_first_status",
                              res->status == D_FILTER_RESULT_SUCCESS,
                              "take_first(3) should return SUCCESS status") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "apply_op_take_first_count",
                              res->count == 3,
                              "take_first(3) should produce 3 elements") && result;

    if ( (res->elements) &&
         (res->count == 3) )
    {
        elems  = (int*)res->elements;
        result = D_TEST_CHECK_MSG(_suite,
                                  "apply_op_take_first_values",
                                  (elems[0] == 10) && (elems[1] == 20) && (elems[2] == 30),
                                  "take_first(3) should return {10, 20, 30}") && result;
    }

    d_filter_result_free(res);
    free(res);

    // test 2: take_last(2) returns last 2 elements
    d_tests_filter_op_release(op);
    op  = d_filter_take_last(2);
    res = d_filter_apply_operation(op,
                                   input,
                                   5,
                                   sizeof(int));

    result = D_TEST_CHECK_MSG(_suite,
                              "apply_op_take_last_count",
                              res->count == 2,
                              "take_last(2) should produce 2 elements") && result;

    if ( (res->elements) &&
         (res->count == 2) )
    {
        elems  = (int*)res->elements;
        result = D_TEST_CHECK_MSG(_suite,
                                  "apply_op_take_last_values",
                                  (elems[0] == 40) && (elems[1] == 50),
                                  "take_last(2) should return {40, 50}") && result;
    }

    d_filter_result_free(res);
    free(res);

    // test 3: skip_first(2) returns last 3 elements
    d_tests_filter_op_release(op);
    op  = d_filter_skip_first(2);
    res = d_filter_apply_operation(op,
                                   input,
                                   5,
                                   sizeof(int));

    result = D_TEST_CHECK_MSG(_suite,
                              "apply_op_skip_first_count",
                              res->count == 3,
                              "skip_first(2) should produce 3 elements") && result;

    if ( (res->elements) &&
         (res->count == 3) )
    {
        elems  = (int*)res->elements;
        result = D_TEST_CHECK_MSG(_suite,
                                  "apply_op_skip_first_values",
                                  (elems[0] == 30) && (elems[1] == 40) && (elems[2] == 50),
                                  "skip_first(2) should return {30, 40, 50}") && result;
    }

    d_filter_result_free(res);
    free(res);

    // test 4: where predicate filters even elements
    // all of {10,20,30,40,50} are even, so all should pass
    d_tests_filter_op_release(op);
    op  = d_filter_where(pred_is_even);
    res = d_filter_apply_operation(op,
                                   input,
                                   5,
                                   sizeof(int));

    result = D_TEST_CHECK_MSG(_suite,
                              "apply_op_where_status",
                              res->status == D_FILTER_RESULT_SUCCESS,
                              "where(is_even) should return SUCCESS") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "apply_op_where_count",
                              res->count == 5,
                              "where(is_even) on {10,20,30,40,50} should produce 5") && result;

    if ( (res->elements) &&
         (res->count == 5) )
    {
        elems  = (int*)res->elements;
        result = D_TEST_CHECK_MSG(_suite,
                                  "apply_op_where_values",
                                  (elems[0] == 10) && (elems[1] == 20) && (elems[2] == 30) && (elems[3] == 40) && (elems[4] == 50),
                                  "where(is_even) should return all elements") && result;
    }

    d_filter_result_free(res);
    free(res);

    // test 5: range [1, 4) selects elements at indices 1, 2, 3
    d_tests_filter_op_release(op);
    op  = d_filter_range(1, 4);
    res = d_filter_apply_operation(op,
                                   input,
                                   5,
                                   sizeof(int));

    result = D_TEST_CHECK_MSG(_suite,
                              "apply_op_range_count",
                              res->count == 3,
                              "range(1,4) should produce 3 elements") && result;

    if ( (res->elements) &&
         (res->count == 3) )
    {
        elems  = (int*)res->elements;
        result = D_TEST_CHECK_MSG(_suite,
                                  "apply_op_range_values",
                                  (elems[0] == 20) && (elems[1] == 30) && (elems[2] == 40),
                                  "range(1,4) should return {20, 30, 40}") && result;
    }

    d_filter_result_free(res);
    free(res);

    // test 6: reverse reverses order
    d_tests_filter_op_release(op);
    op  = d_filter_reverse();
    res = d_filter_apply_operation(op,
                                   input,
                                   5,
                                   sizeof(int));

    result = D_TEST_CHECK_MSG(_suite,
                              "apply_op_reverse_count",
                              res->count == 5,
                              "reverse should produce 5 elements") && result;

    if ( (res->elements) &&
         (res->count == 5) )
    {
        elems  = (int*)res->elements;
        result = D_TEST_CHECK_MSG(_suite,
                                  "apply_op_reverse_values",
                                  (elems[0] == 50) && (elems[1] == 40) && (elems[2] == 30) && (elems[3] == 20) && (elems[4] == 10),
                                  "reverse should return {50, 40, 30, 20, 10}") && result;
    }

    d_filter_result_free(res);
    free(res);

    // test 7: NULL operation returns error
    res = d_filter_apply_operation(NULL,
                                   input,
                                   5,
                                   sizeof(int));

    result = D_TEST_CHECK_MSG(_suite,
                              "apply_op_null_op",
                              (res->status == D_FILTER_RESULT_ERROR) || (res->status == D_FILTER_RESULT_INVALID),
                              "NULL operation should return error/invalid status") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "apply_op_null_op_count",
                              res->count == 0,
                              "NULL operation should produce 0 elements") && result;

    d_filter_result_free(res);
    free(res);

    // test 8: NULL input returns error
    res = d_filter_apply_operation(op,
                                   NULL,
                                   5,
                                   sizeof(int));

    result = D_TEST_CHECK_MSG(_suite,
                              "apply_op_null_input",
                              (res->status == D_FILTER_RESULT_ERROR) || (res->status == D_FILTER_RESULT_INVALID),
                              "NULL input should return error/invalid status") && result;

    d_filter_result_free(res);
    free(res);

    // test 9: zero count returns empty result
    d_tests_filter_op_release(op);
    op  = d_filter_take_first(3);
    res = d_filter_apply_operation(op,
                                   input,
                                   0,
                                   sizeof(int));

    result = D_TEST_CHECK_MSG(_suite,
                              "apply_op_zero_count",
                              res->count == 0,
                              "zero count should produce 0 elements") && result;

    d_filter_result_free(res);
    free(res);

    // test 10: zero element_size returns error
    res = d_filter_apply_operation(op,
                                   input,
                                   5,
                                   0);

    result = D_TEST_CHECK_MSG(_suite,
                              "apply_op_zero_elem_size",
                              (res->status == D_FILTER_RESULT_ERROR) || (res->status == D_FILTER_RESULT_INVALID),
                              "zero element_size should return error/invalid status") && result;

    d_filter_result_free(res);
    free(res);

    d_tests_filter_op_release(op);
    return result;
}


/*
d_tests_filter_apply_chain
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
bool d_tests_filter_apply_chain(
    struct dj_test_suite* _suite
)
{
    struct dj_filter_chain*   chain;
    struct dj_filter_result*  res;
    int                      input[6] = { 1,2,3,4,5,6 };
    int*                     elems;
    bool                     result;

    d_test_suite_test(_suite, "filter_apply_chain");

    result   = true;

    // test 1: empty chain returns all elements
    chain = d_filter_chain_new();

    if (chain)
    {
        res = d_filter_apply_chain(chain,
                                   input,
                                   6,
                                   sizeof(int));

        result = D_TEST_CHECK_MSG(_suite,
                                  "apply_chain_empty_status",
                                  res->status == D_FILTER_RESULT_SUCCESS,
                                  "empty chain should return SUCCESS") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "apply_chain_empty_count",
                                  res->count == 6,
                                  "empty chain should return all 6 elements") && result;

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

        result = D_TEST_CHECK_MSG(_suite,
                                  "apply_chain_single_count",
                                  res->count == 3,
                                  "chain with take_first(3) should produce 3") && result;

        if ( (res->elements) &&
             (res->count == 3) )
        {
            elems  = (int*)res->elements;
            result = D_TEST_CHECK_MSG(_suite,
                                      "apply_chain_single_values",
                                      (elems[0] == 1) && (elems[1] == 2) && (elems[2] == 3),
                                      "take_first(3) should return {1, 2, 3}") && result;
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

        result = D_TEST_CHECK_MSG(_suite,
                                  "apply_chain_skip_take_count",
                                  res->count == 3,
                                  "skip(1)->take(3) should produce 3") && result;

        if ( (res->elements) &&
             (res->count == 3) )
        {
            elems  = (int*)res->elements;
            result = D_TEST_CHECK_MSG(_suite,
                                      "apply_chain_skip_take_values",
                                      (elems[0] == 2) && (elems[1] == 3) && (elems[2] == 4),
                                      "skip(1)->take(3) should return {2, 3, 4}") && result;
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

        result = D_TEST_CHECK_MSG(_suite,
                                  "apply_chain_where_count",
                                  res->count == 3,
                                  "where(is_even) on {1..6} should produce 3") && result;

        if ( (res->elements) &&
             (res->count == 3) )
        {
            elems  = (int*)res->elements;
            result = D_TEST_CHECK_MSG(_suite,
                                      "apply_chain_where_values",
                                      (elems[0] == 2) && (elems[1] == 4) && (elems[2] == 6),
                                      "where(is_even) should return {2, 4, 6}") && result;
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

        result = D_TEST_CHECK_MSG(_suite,
                                  "apply_chain_compound_count",
                                  res->count == 2,
                                  "skip(1)->where(even)->take(2) should produce 2") && result;

        if ( (res->elements) &&
             (res->count == 2) )
        {
            elems  = (int*)res->elements;
            result = D_TEST_CHECK_MSG(_suite,
                                      "apply_chain_compound_values",
                                      (elems[0] == 2) && (elems[1] == 4),
                                      "compound chain should return {2, 4}") && result;
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

    result = D_TEST_CHECK_MSG(_suite,
                              "apply_chain_null_chain",
                              (res->status == D_FILTER_RESULT_ERROR) || (res->status == D_FILTER_RESULT_INVALID),
                              "NULL chain should return error/invalid status") && result;

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

        result = D_TEST_CHECK_MSG(_suite,
                                  "apply_chain_null_input",
                                  (res->status == D_FILTER_RESULT_ERROR) || (res->status == D_FILTER_RESULT_INVALID),
                                  "NULL input should return error/invalid status") && result;

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

        result = D_TEST_CHECK_MSG(_suite,
                                  "apply_chain_zero_count",
                                  res->count == 0,
                                  "zero count should produce 0 elements") && result;

        d_filter_result_free(res);
    free(res);
        d_filter_chain_free(chain);
    }

    return result;
}


/*
d_tests_filter_apply_combinators
  Tests d_filter_apply_union, d_filter_apply_intersection,
  d_filter_apply_difference.
  Tests the following:
  - union of two chains produces OR semantics
  - intersection of two chains produces AND semantics
  - difference produces include-but-not-exclude semantics
  - NULL combinator returns error
  - empty combinator returns appropriate result
*/
bool d_tests_filter_apply_combinators(
    struct dj_test_suite* _suite
)
{
    struct dj_filter_union*        u;
    struct dj_filter_intersection* inter;
    struct dj_filter_difference*   diff;
    struct dj_filter_chain*        chain_even;
    struct dj_filter_chain*        chain_positive;
    struct dj_filter_result*       res;
    int                           input[6] = { -4, -1, 0, 3, 4, 7 };
    int*                          elems;
    size_t                        i;
    bool                          found;
    bool                          result;

    d_test_suite_test(_suite, "filter_apply_combinators");

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

        result = D_TEST_CHECK_MSG(_suite,
                                  "apply_union_status",
                                  res->status == D_FILTER_RESULT_SUCCESS,
                                  "union apply should return SUCCESS") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "apply_union_count",
                                  res->count == 5,
                                  "union(even, positive) should produce 5 elements") && result;

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

            result = D_TEST_CHECK_MSG(_suite,
                                      "apply_union_excludes_neg1",
                                      found == false,
                                      "union should exclude -1 (neither even nor positive)") && result;
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

        result = D_TEST_CHECK_MSG(_suite,
                                  "apply_intersection_status",
                                  res->status == D_FILTER_RESULT_SUCCESS,
                                  "intersection apply should return SUCCESS") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "apply_intersection_count",
                                  res->count == 1,
                                  "intersection(even, positive) should produce 1") && result;

        if ( (res->elements) &&
             (res->count == 1) )
        {
            elems  = (int*)res->elements;
            result = D_TEST_CHECK_MSG(_suite,
                                      "apply_intersection_value",
                                      elems[0] == 4,
                                      "intersection should return {4}") && result;
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

        result = D_TEST_CHECK_MSG(_suite,
                                  "apply_difference_status",
                                  res->status == D_FILTER_RESULT_SUCCESS,
                                  "difference apply should return SUCCESS") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "apply_difference_count",
                                  res->count == 2,
                                  "difference(even, positive) should produce 2") && result;

        if ( (res->elements) &&
             (res->count == 2) )
        {
            elems  = (int*)res->elements;
            result = D_TEST_CHECK_MSG(_suite,
                                      "apply_difference_values",
                                      (elems[0] == -4) && (elems[1] == 0),
                                      "difference should return {-4, 0}") && result;
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

    result = D_TEST_CHECK_MSG(_suite,
                              "apply_union_null",
                              (res->status == D_FILTER_RESULT_ERROR) || (res->status == D_FILTER_RESULT_INVALID),
                              "NULL union should return error/invalid") && result;

    d_filter_result_free(res);
    free(res);

    // test 5: NULL intersection returns error
    res = d_filter_apply_intersection(NULL,
                                      input,
                                      6,
                                      sizeof(int));

    result = D_TEST_CHECK_MSG(_suite,
                              "apply_intersection_null",
                              (res->status == D_FILTER_RESULT_ERROR) || (res->status == D_FILTER_RESULT_INVALID),
                              "NULL intersection should return error/invalid") && result;

    d_filter_result_free(res);
    free(res);

    // test 6: NULL difference returns error
    res = d_filter_apply_difference(NULL,
                                    input,
                                    6,
                                    sizeof(int));

    result = D_TEST_CHECK_MSG(_suite,
                              "apply_difference_null",
                              (res->status == D_FILTER_RESULT_ERROR) || (res->status == D_FILTER_RESULT_INVALID),
                              "NULL difference should return error/invalid") && result;

    d_filter_result_free(res);
    free(res);

    // cleanup shared chains
    d_filter_chain_free(chain_even);
    d_filter_chain_free(chain_positive);

    return result;
}


/*
d_tests_filter_counting
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
bool d_tests_filter_counting(
    struct dj_test_suite* _suite
)
{
    struct dj_filter_chain*  chain;
    int                     mixed[6] = { 1,2,3,4,5,6 };
    int                     all_even[4] = {2,4,6,8};
    int                     all_odd[3] = {1,3,5};
    bool                    result;

    d_test_suite_test(_suite, "filter_counting");

    result      = true;

    chain = d_filter_chain_new();

    if (!chain)
    {
        return false;
    }

    d_filter_chain_add_where(chain, pred_is_even);

    // test 1: count_matches on mixed array
    result = D_TEST_CHECK_MSG(_suite,
                              "count_matches_mixed",
                              d_filter_count_matches(chain, mixed, 6, sizeof(int)) == 3,
                              "count_matches(is_even) on {1..6} should be 3") && result;

    // test 2: count_matches when no matches
    result = D_TEST_CHECK_MSG(_suite,
                              "count_matches_none",
                              d_filter_count_matches(chain, all_odd, 3, sizeof(int)) == 0,
                              "count_matches(is_even) on all odd should be 0") && result;

    // test 3: count_matches when all match
    result = D_TEST_CHECK_MSG(_suite,
                              "count_matches_all",
                              d_filter_count_matches(chain, all_even, 4, sizeof(int)) == 4,
                              "count_matches(is_even) on all even should be 4") && result;

    // test 4: any_match on mixed (some match)
    result = D_TEST_CHECK_MSG(_suite,
                              "any_match_true",
                              d_filter_any_match(chain, mixed, 6, sizeof(int)) == true,
                              "any_match(is_even) on mixed should be true") && result;

    // test 5: any_match when none match
    result = D_TEST_CHECK_MSG(_suite,
                              "any_match_false",
                              d_filter_any_match(chain, all_odd, 3, sizeof(int)) == false,
                              "any_match(is_even) on all odd should be false") && result;

    // test 6: all_match when all match
    result = D_TEST_CHECK_MSG(_suite,
                              "all_match_true",
                              d_filter_all_match(chain, all_even, 4, sizeof(int)) == true,
                              "all_match(is_even) on all even should be true") && result;

    // test 7: all_match when not all match
    result = D_TEST_CHECK_MSG(_suite,
                              "all_match_false",
                              d_filter_all_match(chain, mixed, 6, sizeof(int)) == false,
                              "all_match(is_even) on mixed should be false") && result;

    // test 8: none_match when none match
    result = D_TEST_CHECK_MSG(_suite,
                              "none_match_true",
                              d_filter_none_match(chain, all_odd, 3, sizeof(int)) == true,
                              "none_match(is_even) on all odd should be true") && result;

    // test 9: none_match when some match
    result = D_TEST_CHECK_MSG(_suite,
                              "none_match_false",
                              d_filter_none_match(chain, mixed, 6, sizeof(int)) == false,
                              "none_match(is_even) on mixed should be false") && result;

    d_filter_chain_free(chain);

    // test 10: NULL chain returns 0 for count_matches
    result = D_TEST_CHECK_MSG(_suite,
                              "count_matches_null_chain",
                              d_filter_count_matches(NULL, mixed, 6, sizeof(int)) == 0,
                              "count_matches with NULL chain should be 0") && result;

    // test 11: NULL chain returns false for any_match
    result = D_TEST_CHECK_MSG(_suite,
                              "any_match_null_chain",
                              d_filter_any_match(NULL, mixed, 6, sizeof(int)) == false,
                              "any_match with NULL chain should be false") && result;

    // test 12: NULL chain returns false for all_match
    result = D_TEST_CHECK_MSG(_suite,
                              "all_match_null_chain",
                              d_filter_all_match(NULL, mixed, 6, sizeof(int)) == false,
                              "all_match with NULL chain should be false") && result;

    // test 13: NULL chain returns true/false for none_match
    // (implementation-dependent; typically true or false)
    result = D_TEST_CHECK_MSG(_suite,
                              "none_match_null_chain",
                              d_filter_none_match(NULL, mixed, 6, sizeof(int)) == true,
                              "none_match with NULL chain should be true") && result;

    return result;
}


/*
d_tests_filter_get_indices
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
bool d_tests_filter_get_indices(
    struct dj_test_suite* _suite
)
{
    struct dj_filter_chain*  chain;
    size_t*                 indices;
    size_t                  out_count;
    int                     input[6] = { 1,2,3,4,5,6 };
    bool                    result;

    d_test_suite_test(_suite, "filter_get_indices");

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

    result = D_TEST_CHECK_MSG(_suite,
                              "get_indices_not_null",
                              indices != NULL,
                              "get_indices should return non-NULL for matching elements") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "get_indices_count",
                              out_count == 3,
                              "get_indices(is_even) on {1..6} should return 3 indices") && result;

    if ( (indices) &&
         (out_count == 3) )
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "get_indices_values",
                                  (indices[0] == 1) && (indices[1] == 3) && (indices[2] == 5),
                                  "even indices in {1,2,3,4,5,6} are {1, 3, 5}") && result;
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

    result = D_TEST_CHECK_MSG(_suite,
                              "get_indices_null_chain",
                              indices == NULL,
                              "NULL chain should return NULL") && result;

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

        result = D_TEST_CHECK_MSG(_suite,
                                  "get_indices_null_input",
                                  indices == NULL,
                                  "NULL input should return NULL") && result;

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

        result = D_TEST_CHECK_MSG(_suite,
                                  "get_indices_zero_count",
                                  indices == NULL,
                                  "zero count should return NULL") && result;

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

        result = D_TEST_CHECK_MSG(_suite,
                                  "get_indices_null_out_count",
                                  indices == NULL,
                                  "NULL out_count should return NULL") && result;

        d_filter_chain_free(chain);
    }

    return result;
}


/*
d_tests_filter_in_place
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
bool d_tests_filter_in_place(
    struct dj_test_suite* _suite
)
{
    bool                    result;
    struct dj_filter_chain*  chain;
    int                     data[6] = { 1,2,3,4,5,6 };
    size_t                  new_count;

    d_test_suite_test(_suite, "filter_in_place");

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

    result = D_TEST_CHECK_MSG(_suite,
                              "in_place_even_count",
                              new_count == 3,
                              "in-place filter(is_even) on {1..6} should return 3") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "in_place_even_values",
                              (data[0] == 2) && (data[1] == 4) && (data[2] == 6),
                              "in-place should compact to {2, 4, 6}") && result;

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

        result = D_TEST_CHECK_MSG(_suite,
                                  "in_place_all_pass_count",
                                  new_count == 4,
                                  "all even should return full count 4") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "in_place_all_pass_values",
                                  (data[0] == 2) && (data[1] == 4) && (data[2] == 6) && (data[3] == 8),
                                  "all even should leave array unchanged") && result;

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

        result = D_TEST_CHECK_MSG(_suite,
                                  "in_place_none_pass",
                                  new_count == 0,
                                  "no even elements should return 0") && result;

        d_filter_chain_free(chain);
    }

    // test 4: NULL chain returns 0
    new_count = d_filter_apply_in_place(NULL,
                                        data,
                                        3,
                                        sizeof(int));

    result = D_TEST_CHECK_MSG(_suite,
                              "in_place_null_chain",
                              new_count == 0,
                              "NULL chain should return 0") && result;

    // test 5: NULL array returns 0
    chain = d_filter_chain_new();

    if (chain)
    {
        d_filter_chain_add_where(chain, pred_is_even);

        new_count = d_filter_apply_in_place(chain,
                                            NULL,
                                            6,
                                            sizeof(int));

        result = D_TEST_CHECK_MSG(_suite,
                                  "in_place_null_array",
                                  new_count == 0,
                                  "NULL array should return 0") && result;

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

        result = D_TEST_CHECK_MSG(_suite,
                                  "in_place_zero_count",
                                  new_count == 0,
                                  "zero count should return 0") && result;

        d_filter_chain_free(chain);
    }

    return result;
}


/*
d_tests_filter_result_free
  Tests d_filter_result_free for proper cleanup.
  Tests the following:
  - freeing result with elements and indices NULLs them
  - freeing result with error_message NULLs it
  - freeing result with no allocations does not crash
  - freeing NULL pointer does not crash
*/
bool d_tests_filter_result_free(
    struct dj_test_suite* _suite
)
{
    struct dj_filter_operation* op;
    struct dj_filter_result*    res;
    int                        input[3] = { 10,20,30 };
    bool                       result;

    d_test_suite_test(_suite, "filter_result_free");
    op = NULL;

    result   = true;

    // test 1: free result with allocated elements
    op  = d_filter_take_first(2);
    res = d_filter_apply_operation(op,
                                   input,
                                   3,
                                   sizeof(int));

    result = D_TEST_CHECK_MSG(_suite,
                              "result_free_has_elements",
                              res->elements != NULL,
                              "result should have allocated elements") && result;

    d_filter_result_free(res);

    result = D_TEST_CHECK_MSG(_suite,
                              "result_free_elements_nulled",
                              res->elements == NULL,
                              "free should NULL the elements pointer") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "result_free_indices_nulled",
                              res->indices == NULL,
                              "free should NULL the indices pointer") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "result_free_count_zeroed",
                              res->count == 0,
                              "free should zero the count") && result;

    // the result itself is the caller's, after its contents
    free(res);

    // test 2: free empty/zeroed result does not crash
    res = calloc(1, sizeof(*res));
    d_filter_result_free(res);
    free(res);

    result = D_TEST_CHECK_MSG(_suite,
                              "result_free_empty",
                              true,
                              "free on zeroed result should not crash") && result;

    // test 3: free NULL pointer does not crash
    d_filter_result_free(NULL);

    result = D_TEST_CHECK_MSG(_suite,
                              "result_free_null",
                              true,
                              "free(NULL) should not crash") && result;

    d_tests_filter_op_release(op);
    return result;
}


/*
d_tests_filter_matches_element
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
bool d_tests_filter_matches_element(
    struct dj_test_suite* _suite
)
{
    bool                    result;
    struct dj_filter_chain*  chain;
    int                     even_val;
    int                     odd_val;
    int                     neg_even;

    d_test_suite_test(_suite, "filter_matches_element");

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

    result = D_TEST_CHECK_MSG(_suite,
                              "matches_elem_where_even_pass",
                              d_filter_chain_matches_element(chain, &even_val, sizeof(int)) == true,
                              "even value should match WHERE(is_even)") && result;

    // test 2: WHERE(is_even) on odd element -> false
    result = D_TEST_CHECK_MSG(_suite,
                              "matches_elem_where_even_fail",
                              d_filter_chain_matches_element(chain, &odd_val, sizeof(int)) == false,
                              "odd value should not match WHERE(is_even)") && result;

    d_filter_chain_free(chain);

    // test 3: WHERE_NOT(is_even) on odd element -> true
    chain = d_filter_chain_new();

    if (chain)
    {
        struct dj_filter_operation* op_wn;

        op_wn = d_filter_where_not(pred_is_even);
        d_filter_chain_add(chain, op_wn);
        free(op_wn);  // the chain holds its contents

        result = D_TEST_CHECK_MSG(_suite,
                                  "matches_elem_where_not_pass",
                                  d_filter_chain_matches_element(chain, &odd_val, sizeof(int)) == true,
                                  "odd value should match WHERE_NOT(is_even)") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "matches_elem_where_not_fail",
                                  d_filter_chain_matches_element(chain, &even_val, sizeof(int)) == false,
                                  "even value should not match WHERE_NOT(is_even)") && result;

        d_filter_chain_free(chain);
    }

    // test 4: compound predicates: WHERE(is_even) + WHERE(is_positive)
    chain = d_filter_chain_new();

    if (chain)
    {
        d_filter_chain_add_where(chain, pred_is_even);
        d_filter_chain_add_where(chain, pred_is_positive);

        // even_val = 4 -> even AND positive -> true
        result = D_TEST_CHECK_MSG(_suite,
                                  "matches_elem_compound_pass",
                                  d_filter_chain_matches_element(chain, &even_val, sizeof(int)) == true,
                                  "4 should match WHERE(even) + WHERE(positive)") && result;

        // neg_even = -2 -> even but NOT positive -> false
        result = D_TEST_CHECK_MSG(_suite,
                                  "matches_elem_compound_fail",
                                  d_filter_chain_matches_element(chain, &neg_even, sizeof(int)) == false,
                                  "-2 should not match WHERE(even) + WHERE(positive)") && result;

        // odd_val = 7 -> not even -> false (short-circuits)
        result = D_TEST_CHECK_MSG(_suite,
                                  "matches_elem_compound_fail_first",
                                  d_filter_chain_matches_element(chain, &odd_val, sizeof(int)) == false,
                                  "7 should not match WHERE(even) + WHERE(positive)") && result;

        d_filter_chain_free(chain);
    }

    // test 5: empty chain matches any element
    chain = d_filter_chain_new();

    if (chain)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "matches_elem_empty_chain",
                                  d_filter_chain_matches_element(chain, &odd_val, sizeof(int)) == true,
                                  "empty chain should match any element") && result;

        d_filter_chain_free(chain);
    }

    // test 6: positional op take_first(1) includes the single element
    chain = d_filter_chain_new();

    if (chain)
    {
        d_filter_chain_add_take_first(chain, 1);

        result = D_TEST_CHECK_MSG(_suite,
                                  "matches_elem_take_first_1",
                                  d_filter_chain_matches_element(chain, &even_val, sizeof(int)) == true,
                                  "take_first(1) on single element should match") && result;

        d_filter_chain_free(chain);
    }

    // test 7: positional op skip_first(1) excludes the single element
    chain = d_filter_chain_new();

    if (chain)
    {
        d_filter_chain_add_skip_first(chain, 1);

        result = D_TEST_CHECK_MSG(_suite,
                                  "matches_elem_skip_first_1",
                                  d_filter_chain_matches_element(chain, &even_val, sizeof(int)) == false,
                                  "skip_first(1) on single element should not match") && result;

        d_filter_chain_free(chain);
    }

    // test 8: mixed chain: skip(0) + WHERE(is_even)
    // skip(0) is a no-op, so this tests the general fallback path
    chain = d_filter_chain_new();

    if (chain)
    {
        d_filter_chain_add_skip_first(chain, 0);
        d_filter_chain_add_where(chain, pred_is_even);

        result = D_TEST_CHECK_MSG(_suite,
                                  "matches_elem_mixed_pass",
                                  d_filter_chain_matches_element(chain, &even_val, sizeof(int)) == true,
                                  "skip(0)+WHERE(even) should match even element") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "matches_elem_mixed_fail",
                                  d_filter_chain_matches_element(chain, &odd_val, sizeof(int)) == false,
                                  "skip(0)+WHERE(even) should not match odd element") && result;

        d_filter_chain_free(chain);
    }

    // test 9: NULL chain returns false
    result = D_TEST_CHECK_MSG(_suite,
                              "matches_elem_null_chain",
                              d_filter_chain_matches_element(NULL, &even_val, sizeof(int)) == false,
                              "NULL chain should return false") && result;

    // test 10: NULL element returns false
    chain = d_filter_chain_new();

    if (chain)
    {
        d_filter_chain_add_where(chain, pred_is_even);

        result = D_TEST_CHECK_MSG(_suite,
                                  "matches_elem_null_element",
                                  d_filter_chain_matches_element(chain, NULL, sizeof(int)) == false,
                                  "NULL element should return false") && result;

        d_filter_chain_free(chain);
    }

    // test 11: zero element_size returns false
    chain = d_filter_chain_new();

    if (chain)
    {
        d_filter_chain_add_where(chain, pred_is_even);

        result = D_TEST_CHECK_MSG(_suite,
                                  "matches_elem_zero_size",
                                  d_filter_chain_matches_element(chain, &even_val, 0) == false,
                                  "zero element_size should return false") && result;

        d_filter_chain_free(chain);
    }

    return result;
}


/*
d_tests_filter_execution_all
  Aggregation function that runs all execution and application tests.
*/
bool d_tests_filter_execution_all(
    struct dj_test_suite* _suite
)
{
    bool result;

    d_test_suite_block(_suite, "filter_execution_all");

    result = true;

    result = d_tests_filter_apply_operation(_suite)   && result;
    result = d_tests_filter_apply_chain(_suite)       && result;
    result = d_tests_filter_apply_combinators(_suite) && result;
    result = d_tests_filter_counting(_suite)          && result;
    result = d_tests_filter_get_indices(_suite)       && result;
    result = d_tests_filter_in_place(_suite)          && result;
    result = d_tests_filter_result_free(_suite)       && result;
    result = d_tests_filter_matches_element(_suite)   && result;

    return result;
}
