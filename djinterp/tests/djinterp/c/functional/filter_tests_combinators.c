/*******************************************************************************
* djinterp [functional]                               filter_tests_combinators.c
*
*
* path:      /tests/djinterp/c/functional/filter_tests_combinators.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.02.19
*                                                            revised: 2026.11.09
*******************************************************************************/
// djinterp
#include "./filter_tests.h"
#include "../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's C face


// helper predicates for combinator tests
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


/**
 * @brief Part 1 of d_tests_filter_union.
 *
 * @param[in,out] _suite  the suite the checks are recorded in.
 * @return true if every check of the part passed.
 *
 * The test runs its parts in order, so that no function body is over
 * 100 lines. A part declares its own locals: no value crosses from one
 * part to the next.
 *
 * Sections:
 *   - test 1: creation with capacity
 *   - test 2: creation with capacity 0
 */
static bool
d_internal_tests_filter_union_part1(
    struct dj_test_suite* _suite
)
{
    bool                    result;
    struct dj_filter_union* u;

    result = true;

    // test 1: creation with capacity
    u      = d_filter_union_new(2);
    result = D_TEST_CHECK_MSG(_suite,
                              "union_new_success",
                              u != NULL,
                              "d_filter_union_new(2) should succeed") && result;

    if (u)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "union_new_count",
                                  u->count == 0,
                                  "new union should have count=0") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "union_new_capacity",
                                  u->capacity == 2,
                                  "new union should have capacity=2") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "union_new_filters_allocated",
                                  u->filters != NULL,
                                  "new union should have filters array allocated") && result;

        d_filter_union_free(u);
    }

    // test 2: creation with capacity 0
    u      = d_filter_union_new(0);
    result = D_TEST_CHECK_MSG(_suite,
                              "union_new_zero_capacity",
                              u != NULL,
                              "d_filter_union_new(0) should succeed") && result;

    if (u)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "union_new_zero_null_filters",
                                  u->filters == NULL,
                                  "union with capacity=0 should have NULL filters") && result;

        d_filter_union_free(u);
    }

    return result;
}


/**
 * @brief Part 2 of d_tests_filter_union.
 *
 * @param[in,out] _suite  the suite the checks are recorded in.
 * @return true if every check of the part passed.
 *
 * The test runs its parts in order, so that no function body is over
 * 100 lines. A part declares its own locals: no value crosses from one
 * part to the next.
 *
 * Sections:
 *   - test 3: add chains up to capacity
 *   - test 5: add NULL parameters
 *   - test 6: free NULL safety
 */
static bool
d_internal_tests_filter_union_part2(
    struct dj_test_suite* _suite
)
{
    bool                    result;
    struct dj_filter_union* u;
    struct dj_filter_chain* chain_a;
    struct dj_filter_chain* chain_b;
    struct dj_filter_chain* chain_c;

    result = true;

    // test 3: add chains up to capacity
    u       = d_filter_union_new(2);
    chain_a = d_filter_chain_new();
    chain_b = d_filter_chain_new();

    if ( (u) &&
         (chain_a) &&
         (chain_b) )
    {
        d_filter_chain_add_where(chain_a, pred_is_even);
        d_filter_chain_add_where(chain_b, pred_is_positive);

        result = D_TEST_CHECK_MSG(_suite,
                                  "union_add_first",
                                  d_filter_union_add(u, chain_a) == true,
                                  "adding first chain should succeed") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "union_add_first_count",
                                  u->count == 1,
                                  "count should be 1 after first add") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "union_add_second",
                                  d_filter_union_add(u, chain_b) == true,
                                  "adding second chain should succeed") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "union_add_second_count",
                                  u->count == 2,
                                  "count should be 2 after second add") && result;

        // test 4: add beyond capacity
        chain_c = d_filter_chain_new();

        if (chain_c)
        {
            result = D_TEST_CHECK_MSG(_suite,
                                      "union_add_over_capacity",
                                      d_filter_union_add(u, chain_c) == false,
                                      "adding beyond capacity should fail") && result;

            d_filter_chain_free(chain_c);
        }
    }

    d_filter_union_free(u);
    d_filter_chain_free(chain_a);
    d_filter_chain_free(chain_b);

    // test 5: add NULL parameters
    u = d_filter_union_new(1);

    if (u)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "union_add_null_filter",
                                  d_filter_union_add(u, NULL) == false,
                                  "adding NULL filter should fail") && result;

        d_filter_union_free(u);
    }

    chain_a = d_filter_chain_new();
    result  = D_TEST_CHECK_MSG(_suite,
                               "union_add_null_union",
                               d_filter_union_add(NULL, chain_a) == false,
                               "adding to NULL union should fail") && result;

    d_filter_chain_free(chain_a);

    // test 6: free NULL safety
    d_filter_union_free(NULL);
    result = D_TEST_CHECK_MSG(_suite,
                              "union_free_null",
                              true,
                              "free(NULL) should not crash") && result;

    return result;
}


/*
d_tests_filter_union
  Tests union combinator: creation, adding chains, capacity bounds, free.
  Tests the following:
  - creation with capacity
  - adding chains up to capacity
  - adding beyond capacity returns false
  - NULL parameters return false
  - free with and without chains
  - free NULL safety
*/
bool d_tests_filter_union(
    struct dj_test_suite* _suite
)
{
    bool result;

    d_test_suite_test(_suite, "filter_union");

    result = true;
    result &= d_internal_tests_filter_union_part1(_suite);
    result &= d_internal_tests_filter_union_part2(_suite);

    return result;
}


/*
d_tests_filter_intersection
  Tests intersection combinator: creation, adding chains, capacity bounds,
free.
  Tests the following:
  - creation with capacity
  - adding chains
  - capacity overflow rejection
  - NULL handling
  - free safety
*/
bool d_tests_filter_intersection(
    struct dj_test_suite* _suite
)
{
    bool                           result;
    struct dj_filter_intersection*  inter;
    struct dj_filter_chain*         chain_a;
    struct dj_filter_chain*         chain_b;

    d_test_suite_test(_suite, "filter_intersection");

    result = true;

    // test 1: creation
    inter  = d_filter_intersection_new(3);
    result = D_TEST_CHECK_MSG(_suite,
                              "intersection_new_success",
                              inter != NULL,
                              "d_filter_intersection_new(3) should succeed") && result;

    if (inter)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "intersection_new_fields",
                                  inter->count == 0 && inter->capacity == 3,
                                  "new intersection: count=0, capacity=3") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "intersection_new_allocated",
                                  inter->filters != NULL,
                                  "filters array should be allocated") && result;

        d_filter_intersection_free(inter);
    }

    // test 2: add chains
    inter   = d_filter_intersection_new(2);
    chain_a = d_filter_chain_new();
    chain_b = d_filter_chain_new();

    if ( (inter) &&
         (chain_a) &&
         (chain_b) )
    {
        d_filter_chain_add_where(chain_a, pred_is_even);
        d_filter_chain_add_where(chain_b, pred_is_positive);

        result = D_TEST_CHECK_MSG(_suite,
                                  "intersection_add_first",
                                  d_filter_intersection_add(inter, chain_a) == true,
                                  "adding first chain should succeed") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "intersection_add_second",
                                  d_filter_intersection_add(inter, chain_b) == true,
                                  "adding second chain should succeed") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "intersection_add_count",
                                  inter->count == 2,
                                  "count should be 2 after two adds") && result;
    }

    d_filter_intersection_free(inter);
    d_filter_chain_free(chain_a);
    d_filter_chain_free(chain_b);

    // test 3: add beyond capacity
    inter   = d_filter_intersection_new(1);
    chain_a = d_filter_chain_new();
    chain_b = d_filter_chain_new();

    if ( (inter) &&
         (chain_a) &&
         (chain_b) )
    {
        d_filter_intersection_add(inter, chain_a);

        result = D_TEST_CHECK_MSG(_suite,
                                  "intersection_add_over_capacity",
                                  d_filter_intersection_add(inter, chain_b) == false,
                                  "adding beyond capacity should fail") && result;
    }

    d_filter_intersection_free(inter);
    d_filter_chain_free(chain_a);
    d_filter_chain_free(chain_b);

    // test 4: NULL handling
    result = D_TEST_CHECK_MSG(_suite,
                              "intersection_add_null_inter",
                              d_filter_intersection_add(NULL, chain_a) == false,
                              "adding to NULL intersection should fail") && result;

    inter = d_filter_intersection_new(1);

    if (inter)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "intersection_add_null_filter",
                                  d_filter_intersection_add(inter, NULL) == false,
                                  "adding NULL filter should fail") && result;

        d_filter_intersection_free(inter);
    }

    // test 5: free NULL safety
    d_filter_intersection_free(NULL);
    result = D_TEST_CHECK_MSG(_suite,
                              "intersection_free_null",
                              true,
                              "free(NULL) should not crash") && result;

    return result;
}


/*
d_tests_filter_difference
  Tests difference combinator: creation and free.
  Tests the following:
  - creation with include and exclude chains
  - creation with NULL chains
  - field assignment verification
  - free safety
*/
bool d_tests_filter_difference(
    struct dj_test_suite* _suite
)
{
    bool                          result;
    struct dj_filter_difference*   diff;
    struct dj_filter_chain*        chain_inc;
    struct dj_filter_chain*        chain_exc;
    int                           threshold;

    d_test_suite_test(_suite, "filter_difference");

    result    = true;
    threshold = 5;

    // test 1: creation with both chains
    chain_inc = d_filter_chain_new();
    chain_exc = d_filter_chain_new();

    if ( (chain_inc) &&
         (chain_exc) )
    {
        d_filter_chain_add_where(chain_inc, pred_is_even);
        d_filter_chain_add_where_context(chain_exc,
                                     pred_greater_than_context,
                                     &threshold);

        diff   = d_filter_difference_new(chain_inc, chain_exc);
        result = D_TEST_CHECK_MSG(_suite,
                                  "difference_new_success",
                                  diff != NULL,
                                  "d_filter_difference_new should succeed") && result;

        if (diff)
        {
            result = D_TEST_CHECK_MSG(_suite,
                                      "difference_include_set",
                                      diff->include == chain_inc,
                                      "include chain should be stored") && result;

            result = D_TEST_CHECK_MSG(_suite,
                                      "difference_exclude_set",
                                      diff->exclude == chain_exc,
                                      "exclude chain should be stored") && result;

            d_filter_difference_free(diff);
        }
    }

    d_filter_chain_free(chain_inc);
    d_filter_chain_free(chain_exc);

    // test 2: creation with NULL chains (allowed — caller's choice)
    diff   = d_filter_difference_new(NULL, NULL);
    result = D_TEST_CHECK_MSG(_suite,
                              "difference_new_null_chains",
                              diff != NULL,
                              "difference_new with NULL chains should succeed") && result;

    if (diff)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "difference_null_fields",
                                  diff->include == NULL && diff->exclude == NULL,
                                  "NULL chains should be stored as NULL") && result;

        d_filter_difference_free(diff);
    }

    // test 3: free NULL safety
    d_filter_difference_free(NULL);
    result = D_TEST_CHECK_MSG(_suite,
                              "difference_free_null",
                              true,
                              "free(NULL) should not crash") && result;

    return result;
}


/*
d_tests_filter_combinators_all
  Aggregation function that runs all combinator tests.
*/
bool d_tests_filter_combinators_all(
    struct dj_test_suite* _suite
)
{
    bool result;

    d_test_suite_block(_suite, "filter_combinators_all");

    result = true;

    result = d_tests_filter_union(_suite)        && result;
    result = d_tests_filter_intersection(_suite)  && result;
    result = d_tests_filter_difference(_suite)    && result;

    return result;
}
