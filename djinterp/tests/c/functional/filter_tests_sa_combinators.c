#include ".\filter_tests_sa.h"


// helper predicates for combinator tests
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


/*
d_tests_sa_filter_union
  Tests union combinator: creation, adding chains, capacity bounds, free.
  Tests the following:
  - creation with capacity
  - adding chains up to capacity
  - adding beyond capacity returns false
  - NULL parameters return false
  - free with and without chains
  - free NULL safety
*/
bool
d_tests_sa_filter_union
(
    struct d_test_counter* _counter
)
{
    bool                    result;
    struct d_filter_union*  u;
    struct d_filter_chain*  chain_a;
    struct d_filter_chain*  chain_b;
    struct d_filter_chain*  chain_c;

    result = true;

    // test 1: creation with capacity
    u      = d_filter_union_new(2);
    result = d_assert_standalone(
        u != NULL,
        "union_new_success",
        "d_filter_union_new(2) should succeed",
        _counter) && result;

    if (u)
    {
        result = d_assert_standalone(
            u->count == 0,
            "union_new_count",
            "new union should have count=0",
            _counter) && result;

        result = d_assert_standalone(
            u->capacity == 2,
            "union_new_capacity",
            "new union should have capacity=2",
            _counter) && result;

        result = d_assert_standalone(
            u->filters != NULL,
            "union_new_filters_allocated",
            "new union should have filters array allocated",
            _counter) && result;

        d_filter_union_free(u);
    }

    // test 2: creation with capacity 0
    u      = d_filter_union_new(0);
    result = d_assert_standalone(
        u != NULL,
        "union_new_zero_capacity",
        "d_filter_union_new(0) should succeed",
        _counter) && result;

    if (u)
    {
        result = d_assert_standalone(
            u->filters == NULL,
            "union_new_zero_null_filters",
            "union with capacity=0 should have NULL filters",
            _counter) && result;

        d_filter_union_free(u);
    }

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

        result = d_assert_standalone(
            d_filter_union_add(u, chain_a) == true,
            "union_add_first",
            "adding first chain should succeed",
            _counter) && result;

        result = d_assert_standalone(
            u->count == 1,
            "union_add_first_count",
            "count should be 1 after first add",
            _counter) && result;

        result = d_assert_standalone(
            d_filter_union_add(u, chain_b) == true,
            "union_add_second",
            "adding second chain should succeed",
            _counter) && result;

        result = d_assert_standalone(
            u->count == 2,
            "union_add_second_count",
            "count should be 2 after second add",
            _counter) && result;

        // test 4: add beyond capacity
        chain_c = d_filter_chain_new();

        if (chain_c)
        {
            result = d_assert_standalone(
                d_filter_union_add(u, chain_c) == false,
                "union_add_over_capacity",
                "adding beyond capacity should fail",
                _counter) && result;

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
        result = d_assert_standalone(
            d_filter_union_add(u, NULL) == false,
            "union_add_null_filter",
            "adding NULL filter should fail",
            _counter) && result;

        d_filter_union_free(u);
    }

    chain_a = d_filter_chain_new();
    result  = d_assert_standalone(
        d_filter_union_add(NULL, chain_a) == false,
        "union_add_null_union",
        "adding to NULL union should fail",
        _counter) && result;

    d_filter_chain_free(chain_a);

    // test 6: free NULL safety
    d_filter_union_free(NULL);
    result = d_assert_standalone(
        true,
        "union_free_null",
        "free(NULL) should not crash",
        _counter) && result;

    return result;
}


/*
d_tests_sa_filter_intersection
  Tests intersection combinator: creation, adding chains, capacity bounds,
free.
  Tests the following:
  - creation with capacity
  - adding chains
  - capacity overflow rejection
  - NULL handling
  - free safety
*/
bool
d_tests_sa_filter_intersection
(
    struct d_test_counter* _counter
)
{
    bool                           result;
    struct d_filter_intersection*  inter;
    struct d_filter_chain*         chain_a;
    struct d_filter_chain*         chain_b;

    result = true;

    // test 1: creation
    inter  = d_filter_intersection_new(3);
    result = d_assert_standalone(
        inter != NULL,
        "intersection_new_success",
        "d_filter_intersection_new(3) should succeed",
        _counter) && result;

    if (inter)
    {
        result = d_assert_standalone(
            inter->count == 0 && inter->capacity == 3,
            "intersection_new_fields",
            "new intersection: count=0, capacity=3",
            _counter) && result;

        result = d_assert_standalone(
            inter->filters != NULL,
            "intersection_new_allocated",
            "filters array should be allocated",
            _counter) && result;

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

        result = d_assert_standalone(
            d_filter_intersection_add(inter, chain_a) == true,
            "intersection_add_first",
            "adding first chain should succeed",
            _counter) && result;

        result = d_assert_standalone(
            d_filter_intersection_add(inter, chain_b) == true,
            "intersection_add_second",
            "adding second chain should succeed",
            _counter) && result;

        result = d_assert_standalone(
            inter->count == 2,
            "intersection_add_count",
            "count should be 2 after two adds",
            _counter) && result;
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

        result = d_assert_standalone(
            d_filter_intersection_add(inter, chain_b) == false,
            "intersection_add_over_capacity",
            "adding beyond capacity should fail",
            _counter) && result;
    }

    d_filter_intersection_free(inter);
    d_filter_chain_free(chain_a);
    d_filter_chain_free(chain_b);

    // test 4: NULL handling
    result = d_assert_standalone(
        d_filter_intersection_add(NULL, chain_a) == false,
        "intersection_add_null_inter",
        "adding to NULL intersection should fail",
        _counter) && result;

    inter = d_filter_intersection_new(1);

    if (inter)
    {
        result = d_assert_standalone(
            d_filter_intersection_add(inter, NULL) == false,
            "intersection_add_null_filter",
            "adding NULL filter should fail",
            _counter) && result;

        d_filter_intersection_free(inter);
    }

    // test 5: free NULL safety
    d_filter_intersection_free(NULL);
    result = d_assert_standalone(
        true,
        "intersection_free_null",
        "free(NULL) should not crash",
        _counter) && result;

    return result;
}


/*
d_tests_sa_filter_difference
  Tests difference combinator: creation and free.
  Tests the following:
  - creation with include and exclude chains
  - creation with NULL chains
  - field assignment verification
  - free safety
*/
bool
d_tests_sa_filter_difference
(
    struct d_test_counter* _counter
)
{
    bool                          result;
    struct d_filter_difference*   diff;
    struct d_filter_chain*        chain_inc;
    struct d_filter_chain*        chain_exc;
    int                           threshold;

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
        result = d_assert_standalone(
            diff != NULL,
            "difference_new_success",
            "d_filter_difference_new should succeed",
            _counter) && result;

        if (diff)
        {
            result = d_assert_standalone(
                diff->include == chain_inc,
                "difference_include_set",
                "include chain should be stored",
                _counter) && result;

            result = d_assert_standalone(
                diff->exclude == chain_exc,
                "difference_exclude_set",
                "exclude chain should be stored",
                _counter) && result;

            d_filter_difference_free(diff);
        }
    }

    d_filter_chain_free(chain_inc);
    d_filter_chain_free(chain_exc);

    // test 2: creation with NULL chains (allowed — caller's choice)
    diff   = d_filter_difference_new(NULL, NULL);
    result = d_assert_standalone(
        diff != NULL,
        "difference_new_null_chains",
        "difference_new with NULL chains should succeed",
        _counter) && result;

    if (diff)
    {
        result = d_assert_standalone(
            diff->include == NULL && diff->exclude == NULL,
            "difference_null_fields",
            "NULL chains should be stored as NULL",
            _counter) && result;

        d_filter_difference_free(diff);
    }

    // test 3: free NULL safety
    d_filter_difference_free(NULL);
    result = d_assert_standalone(
        true,
        "difference_free_null",
        "free(NULL) should not crash",
        _counter) && result;

    return result;
}


/*
d_tests_sa_filter_combinators_all
  Aggregation function that runs all combinator tests.
*/
bool
d_tests_sa_filter_combinators_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Filter Combinators\n");
    printf("  -----------------------------\n");

    result = d_tests_sa_filter_union(_counter)        && result;
    result = d_tests_sa_filter_intersection(_counter)  && result;
    result = d_tests_sa_filter_difference(_counter)    && result;

    return result;
}
