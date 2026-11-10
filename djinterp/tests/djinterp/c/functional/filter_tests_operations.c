/*******************************************************************************
* djinterp [functional]                                filter_tests_operations.c
*
*
* path:      /tests/djinterp/c/functional/filter_tests_operations.c
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
bool pred_always_true(
    const void* _element,
    void*       _context
)
{
    (void)_element;
    (void)_context;

    return true;
}

static
bool pred_always_false(
    const void* _element,
    void*       _context
)
{
    (void)_element;
    (void)_context;

    return false;
}

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
d_tests_filter_op_take
  Tests take operation constructors: take_first, take_last, take_nth,
head, and tail.
  Tests the following:
  - field initialization for each take variant
  - correct operation type assignment
  - parameter storage (count, step)
  - head/tail as special cases of take_first(1)/take_last(1)
*/
bool d_tests_filter_op_take(
    struct dj_test_suite* _suite
)
{
    bool                      result;
    struct dj_filter_operation* op;

    d_test_suite_test(_suite, "filter_op_take");
    op = NULL;

    result = true;

    // test 1: take_first sets type and count
    op     = d_filter_take_first(5);
    result = D_TEST_CHECK_MSG(_suite,
                              "take_first_type",
                              op->type == D_FILTER_OP_TAKE_FIRST,
                              "take_first should set D_FILTER_OP_TAKE_FIRST") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "take_first_count",
                              op->params.count == 5,
                              "take_first(5) should store count=5") && result;

    // test 2: take_first(0) — degenerate case
    d_tests_filter_op_release(op);
    op     = d_filter_take_first(0);
    result = D_TEST_CHECK_MSG(_suite,
                              "take_first_zero",
                              op->params.count == 0,
                              "take_first(0) should store count=0") && result;

    // test 3: take_last sets type and count
    d_tests_filter_op_release(op);
    op     = d_filter_take_last(3);
    result = D_TEST_CHECK_MSG(_suite,
                              "take_last_type",
                              op->type == D_FILTER_OP_TAKE_LAST,
                              "take_last should set D_FILTER_OP_TAKE_LAST") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "take_last_count",
                              op->params.count == 3,
                              "take_last(3) should store count=3") && result;

    // test 4: take_nth sets type and step
    d_tests_filter_op_release(op);
    op     = d_filter_take_nth(2);
    result = D_TEST_CHECK_MSG(_suite,
                              "take_nth_type",
                              op->type == D_FILTER_OP_TAKE_NTH,
                              "take_nth should set D_FILTER_OP_TAKE_NTH") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "take_nth_step",
                              op->params.step == 2,
                              "take_nth(2) should store step=2") && result;

    // test 5: take_nth(0) defaults step to 1
    d_tests_filter_op_release(op);
    op     = d_filter_take_nth(0);
    result = D_TEST_CHECK_MSG(_suite,
                              "take_nth_zero_defaults",
                              op->params.step == 1,
                              "take_nth(0) should default step to 1") && result;

    // test 6: head = take_first(1)
    d_tests_filter_op_release(op);
    op     = d_filter_head();
    result = D_TEST_CHECK_MSG(_suite,
                              "head_type",
                              op->type == D_FILTER_OP_HEAD,
                              "head should set D_FILTER_OP_HEAD") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "head_count",
                              op->params.count == 1,
                              "head should store count=1") && result;

    // test 7: tail = take_last(1)
    d_tests_filter_op_release(op);
    op     = d_filter_tail();
    result = D_TEST_CHECK_MSG(_suite,
                              "tail_type",
                              op->type == D_FILTER_OP_TAIL,
                              "tail should set D_FILTER_OP_TAIL") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "tail_count",
                              op->params.count == 1,
                              "tail should store count=1") && result;

    d_tests_filter_op_release(op);
    return result;
}


/*
d_tests_filter_op_skip
  Tests skip operation constructors: skip_first, skip_last, init, rest.
  Tests the following:
  - field initialization for each skip variant
  - correct operation type assignment
  - init/rest as special cases
*/
bool d_tests_filter_op_skip(
    struct dj_test_suite* _suite
)
{
    struct dj_filter_operation* op;
    bool                       result;

    d_test_suite_test(_suite, "filter_op_skip");
    op = NULL;

    result = true;

    // test 1: skip_first sets type and count
    op     = d_filter_skip_first(3);
    result = D_TEST_CHECK_MSG(_suite,
                              "skip_first_type",
                              op->type == D_FILTER_OP_SKIP_FIRST,
                              "skip_first should set D_FILTER_OP_SKIP_FIRST") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "skip_first_count",
                              op->params.count == 3,
                              "skip_first(3) should store count=3") && result;

    // test 2: skip_first(0) — no-op equivalent
    d_tests_filter_op_release(op);
    op     = d_filter_skip_first(0);
    result = D_TEST_CHECK_MSG(_suite,
                              "skip_first_zero",
                              op->params.count == 0,
                              "skip_first(0) should store count=0") && result;

    // test 3: skip_last sets type and count
    d_tests_filter_op_release(op);
    op     = d_filter_skip_last(2);
    result = D_TEST_CHECK_MSG(_suite,
                              "skip_last_type",
                              op->type == D_FILTER_OP_SKIP_LAST,
                              "skip_last should set D_FILTER_OP_SKIP_LAST") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "skip_last_count",
                              op->params.count == 2,
                              "skip_last(2) should store count=2") && result;

    // test 4: init (all except last)
    d_tests_filter_op_release(op);
    op     = d_filter_init();
    result = D_TEST_CHECK_MSG(_suite,
                              "init_type",
                              op->type == D_FILTER_OP_INIT,
                              "init should set D_FILTER_OP_INIT") && result;

    // test 5: rest (all except first)
    d_tests_filter_op_release(op);
    op     = d_filter_rest();
    result = D_TEST_CHECK_MSG(_suite,
                              "rest_type",
                              op->type == D_FILTER_OP_REST,
                              "rest should set D_FILTER_OP_REST") && result;

    d_tests_filter_op_release(op);
    return result;
}


/*
d_tests_filter_op_range_slice
  Tests range and slice operation constructors.
  Tests the following:
  - range stores start and end correctly
  - slice stores start, end, and step correctly
  - slice step=0 defaults to 1
*/
bool d_tests_filter_op_range_slice(
    struct dj_test_suite* _suite
)
{
    struct dj_filter_operation* op;
    bool                       result;

    d_test_suite_test(_suite, "filter_op_range_slice");
    op = NULL;

    result = true;

    // test 1: range sets type, start, end
    op     = d_filter_range(2, 7);
    result = D_TEST_CHECK_MSG(_suite,
                              "range_type",
                              op->type == D_FILTER_OP_RANGE,
                              "range should set D_FILTER_OP_RANGE") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "range_params",
                              op->params.start == 2 && op->params.end == 7,
                              "range(2,7) should store start=2, end=7") && result;

    // test 2: range with start == end (empty range)
    d_tests_filter_op_release(op);
    op     = d_filter_range(5, 5);
    result = D_TEST_CHECK_MSG(_suite,
                              "range_empty",
                              op->params.start == 5 && op->params.end == 5,
                              "range(5,5) should store start=end=5") && result;

    // test 3: slice sets type, start, end, step
    d_tests_filter_op_release(op);
    op     = d_filter_slice(1, 10, 3);
    result = D_TEST_CHECK_MSG(_suite,
                              "slice_type",
                              op->type == D_FILTER_OP_SLICE,
                              "slice should set D_FILTER_OP_SLICE") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "slice_params",
                              op->params.start == 1 && op->params.end == 10 && op->params.step == 3,
                              "slice(1,10,3) should store start=1, end=10, step=3") && result;

    // test 4: slice step=0 defaults to 1
    d_tests_filter_op_release(op);
    op     = d_filter_slice(0, 5, 0);
    result = D_TEST_CHECK_MSG(_suite,
                              "slice_step_zero",
                              op->params.step == 1,
                              "slice with step=0 should default to step=1") && result;

    d_tests_filter_op_release(op);
    return result;
}


/*
d_tests_filter_op_where
  Tests predicate-based operation constructors: where, where_context,
where_not, where_not_context.
  Tests the following:
  - correct type assignment (WHERE vs WHERE_NOT)
  - predicate function pointer storage
  - context pointer storage (NULL and non-NULL)
*/
bool d_tests_filter_op_where(
    struct dj_test_suite* _suite
)
{
    struct dj_filter_operation* op;
    int                        threshold;
    bool                       result;

    d_test_suite_test(_suite, "filter_op_where");
    op = NULL;

    result    = true;
    threshold = 10;

    // test 1: where stores predicate, NULL context
    op     = d_filter_where(pred_is_even);
    result = D_TEST_CHECK_MSG(_suite,
                              "where_type",
                              op->type == D_FILTER_OP_WHERE,
                              "where should set D_FILTER_OP_WHERE") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "where_predicate",
                              op->params.test == pred_is_even,
                              "where should store the predicate function") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "where_context_null",
                              op->params.context == NULL,
                              "where should store NULL context") && result;

    // test 2: where_context stores predicate and context
    d_tests_filter_op_release(op);
    op     = d_filter_where_context(pred_greater_than_context, &threshold);
    result = D_TEST_CHECK_MSG(_suite,
                              "where_context_type",
                              op->type == D_FILTER_OP_WHERE,
                              "where_context should set D_FILTER_OP_WHERE") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "where_context_predicate",
                              op->params.test == pred_greater_than_context,
                              "where_context should store the predicate function") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "where_context_context",
                              op->params.context == &threshold,
                              "where_context should store the context pointer") && result;

    // test 3: where_not stores predicate with WHERE_NOT type
    d_tests_filter_op_release(op);
    op     = d_filter_where_not(pred_is_positive);
    result = D_TEST_CHECK_MSG(_suite,
                              "where_not_type",
                              op->type == D_FILTER_OP_WHERE_NOT,
                              "where_not should set D_FILTER_OP_WHERE_NOT") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "where_not_predicate",
                              op->params.test == pred_is_positive,
                              "where_not should store the predicate function") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "where_not_context_null",
                              op->params.context == NULL,
                              "where_not should store NULL context") && result;

    // test 4: where_not_context stores predicate and context
    d_tests_filter_op_release(op);
    op     = d_filter_where_not_context(pred_greater_than_context, &threshold);
    result = D_TEST_CHECK_MSG(_suite,
                              "where_not_context_type",
                              op->type == D_FILTER_OP_WHERE_NOT,
                              "where_not_context should set D_FILTER_OP_WHERE_NOT") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "where_not_context_context",
                              op->params.context == &threshold,
                              "where_not_context should store the context pointer") && result;

    d_tests_filter_op_release(op);
    return result;
}

/*
d_tests_filter_op_indices
  Tests index-based operation constructors: at, at_indices.
  Tests the following:
  - at stores single index via start field
  - at_indices copies the indices array
  - at_indices with NULL/empty input
*/
bool d_tests_filter_op_indices(
    struct dj_test_suite* _suite
)
{
    struct dj_filter_operation* op;
    size_t                     idx_array[4];
    bool                       result;

    d_test_suite_test(_suite, "filter_op_indices");
    op = NULL;

    result       = true;
    idx_array[0] = 0;
    idx_array[1] = 3;
    idx_array[2] = 5;
    idx_array[3] = 9;

    // test 1: at stores type INDICES, single index via start
    op     = d_filter_at(7);
    result = D_TEST_CHECK_MSG(_suite,
                              "at_type",
                              op->type == D_FILTER_OP_INDICES,
                              "at should set D_FILTER_OP_INDICES") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "at_index",
                              op->params.start == 7,
                              "at(7) should store start=7") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "at_count",
                              op->params.count == 1,
                              "at should store count=1") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "at_no_indices_array",
                              op->params.indices == NULL,
                              "at should not allocate indices array") && result;

    // test 2: at_indices copies the array
    d_tests_filter_op_release(op);
    op     = d_filter_at_indices(idx_array, 4);
    result = D_TEST_CHECK_MSG(_suite,
                              "at_indices_type",
                              op->type == D_FILTER_OP_INDICES,
                              "at_indices should set D_FILTER_OP_INDICES") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "at_indices_count",
                              op->params.indices_count == 4,
                              "at_indices should store count=4") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "at_indices_allocated",
                              op->params.indices != NULL,
                              "at_indices should allocate indices array") && result;

    if (op->params.indices)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "at_indices_values",
                                  op->params.indices[0] == 0 && op->params.indices[1] == 3 && op->params.indices[2] == 5 && op->params.indices[3] == 9,
                                  "at_indices should copy all index values") && result;

        // verify it is a copy, not the original pointer
        result = D_TEST_CHECK_MSG(_suite,
                                  "at_indices_is_copy",
                                  op->params.indices != idx_array,
                                  "at_indices should allocate a new array (deep copy)") && result;
    }

    d_filter_operation_free(op);

    // test 3: at_indices with NULL array
    d_tests_filter_op_release(op);
    op     = d_filter_at_indices(NULL, 5);
    result = D_TEST_CHECK_MSG(_suite,
                              "at_indices_null_array",
                              op->params.indices == NULL,
                              "at_indices with NULL array should store NULL") && result;

    // test 4: at_indices with zero count
    d_tests_filter_op_release(op);
    op     = d_filter_at_indices(idx_array, 0);
    result = D_TEST_CHECK_MSG(_suite,
                              "at_indices_zero_count",
                              op->params.indices == NULL,
                              "at_indices with count=0 should store NULL") && result;

    d_tests_filter_op_release(op);
    return result;
}


/*
d_tests_filter_op_distinct_reverse
  Tests distinct and reverse operation constructors.
  Tests the following:
  - distinct stores comparator function pointer
  - reverse sets type with no parameters
*/
bool d_tests_filter_op_distinct_reverse(
    struct dj_test_suite* _suite
)
{
    struct dj_filter_operation* op;
    bool                       result;

    d_test_suite_test(_suite, "filter_op_distinct_reverse");
    op = NULL;

    result = true;

    // test 1: distinct stores comparator
    op     = d_filter_distinct(cmp_int);
    result = D_TEST_CHECK_MSG(_suite,
                              "distinct_type",
                              op->type == D_FILTER_OP_DISTINCT,
                              "distinct should set D_FILTER_OP_DISTINCT") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "distinct_comparator",
                              op->params.comparator == cmp_int,
                              "distinct should store the comparator function") && result;

    // test 2: reverse sets type
    d_tests_filter_op_release(op);
    op     = d_filter_reverse();
    result = D_TEST_CHECK_MSG(_suite,
                              "reverse_type",
                              op->type == D_FILTER_OP_REVERSE,
                              "reverse should set D_FILTER_OP_REVERSE") && result;

    d_tests_filter_op_release(op);
    return result;
}


/*
d_tests_filter_op_free
  Tests d_filter_operation_free.
  Tests the following:
  - freeing operation with allocated indices
  - freeing operation with allocated name
  - freeing operation with no allocated resources
  - NULL safety
*/
bool d_tests_filter_op_free(
    struct dj_test_suite* _suite
)
{
    struct dj_filter_operation* op;
    size_t                     idx_array[2];
    bool                       result;

    d_test_suite_test(_suite, "filter_op_free");
    op = NULL;

    result       = true;
    idx_array[0] = 1;
    idx_array[1] = 2;

    // test 1: free operation with allocated indices
    op = d_filter_at_indices(idx_array, 2);
    result = D_TEST_CHECK_MSG(_suite,
                              "op_free_has_indices",
                              op->params.indices != NULL,
                              "at_indices should have allocated indices") && result;

    d_filter_operation_free(op);

    result = D_TEST_CHECK_MSG(_suite,
                              "op_free_indices_nulled",
                              op->params.indices == NULL,
                              "free should NULL the indices pointer") && result;

    // test 2: free operation with name
    d_tests_filter_op_release(op);
    op = d_filter_take_first(1);
    op->name = d_strdup("test_op_name");
    result = D_TEST_CHECK_MSG(_suite,
                              "op_free_has_name",
                              op->name != NULL,
                              "operation should have name set") && result;

    d_filter_operation_free(op);

    result = D_TEST_CHECK_MSG(_suite,
                              "op_free_name_nulled",
                              op->name == NULL,
                              "free should NULL the name pointer") && result;

    // test 3: free simple operation (no allocated resources)
    d_tests_filter_op_release(op);
    op = d_filter_reverse();

    d_filter_operation_free(op);

    result = D_TEST_CHECK_MSG(_suite,
                              "op_free_simple",
                              true,
                              "free on operation with no resources should not crash") && result;

    // test 4: NULL safety
    d_filter_operation_free(NULL);
    result = D_TEST_CHECK_MSG(_suite,
                              "op_free_null",
                              true,
                              "free(NULL) should not crash") && result;

    d_tests_filter_op_release(op);
    return result;
}

/*
d_tests_filter_operations_all
  Aggregation function that runs all filter operation tests.
*/
bool d_tests_filter_operations_all(
    struct dj_test_suite* _suite
)
{
    bool result;

    d_test_suite_block(_suite, "filter_operations_all");

    result = true;

    result = d_tests_filter_op_take(_suite)             && result;
    result = d_tests_filter_op_skip(_suite)             && result;
    result = d_tests_filter_op_range_slice(_suite)      && result;
    result = d_tests_filter_op_where(_suite)            && result;
    result = d_tests_filter_op_indices(_suite)          && result;
    result = d_tests_filter_op_distinct_reverse(_suite) && result;
    result = d_tests_filter_op_free(_suite)             && result;

    return result;
}
