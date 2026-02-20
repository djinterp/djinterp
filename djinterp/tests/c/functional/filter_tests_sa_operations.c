#include ".\filter_tests_sa.h"


///////////////////////////////////////////////////////////////////////////////
// HELPER PREDICATES
///////////////////////////////////////////////////////////////////////////////

static bool pred_always_true(const void* _element, void* _context)
{
    (void)_element;
    (void)_context;

    return true;
}

static bool pred_always_false(const void* _element, void* _context)
{
    (void)_element;
    (void)_context;

    return false;
}

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
d_tests_sa_filter_op_take
  Tests take operation constructors: take_first, take_last, take_nth,
head, and tail.
  Tests the following:
  - field initialization for each take variant
  - correct operation type assignment
  - parameter storage (count, step)
  - head/tail as special cases of take_first(1)/take_last(1)
*/
bool
d_tests_sa_filter_op_take
(
    struct d_test_counter* _counter
)
{
    bool                      result;
    struct d_filter_operation* op;

    result = true;

    // test 1: take_first sets type and count
    op     = d_filter_take_first(5);
    result = d_assert_standalone(
        op->type == D_FILTER_OP_TAKE_FIRST,
        "take_first_type",
        "take_first should set D_FILTER_OP_TAKE_FIRST",
        _counter) && result;

    result = d_assert_standalone(
        op->params.count == 5,
        "take_first_count",
        "take_first(5) should store count=5",
        _counter) && result;

    // test 2: take_first(0) — degenerate case
    op     = d_filter_take_first(0);
    result = d_assert_standalone(
        op->params.count == 0,
        "take_first_zero",
        "take_first(0) should store count=0",
        _counter) && result;

    // test 3: take_last sets type and count
    op     = d_filter_take_last(3);
    result = d_assert_standalone(
        op->type == D_FILTER_OP_TAKE_LAST,
        "take_last_type",
        "take_last should set D_FILTER_OP_TAKE_LAST",
        _counter) && result;

    result = d_assert_standalone(
        op->params.count == 3,
        "take_last_count",
        "take_last(3) should store count=3",
        _counter) && result;

    // test 4: take_nth sets type and step
    op     = d_filter_take_nth(2);
    result = d_assert_standalone(
        op->type == D_FILTER_OP_TAKE_NTH,
        "take_nth_type",
        "take_nth should set D_FILTER_OP_TAKE_NTH",
        _counter) && result;

    result = d_assert_standalone(
        op->params.step == 2,
        "take_nth_step",
        "take_nth(2) should store step=2",
        _counter) && result;

    // test 5: take_nth(0) defaults step to 1
    op     = d_filter_take_nth(0);
    result = d_assert_standalone(
        op->params.step == 1,
        "take_nth_zero_defaults",
        "take_nth(0) should default step to 1",
        _counter) && result;

    // test 6: head = take_first(1)
    op     = d_filter_head();
    result = d_assert_standalone(
        op->type == D_FILTER_OP_HEAD,
        "head_type",
        "head should set D_FILTER_OP_HEAD",
        _counter) && result;

    result = d_assert_standalone(
        op->params.count == 1,
        "head_count",
        "head should store count=1",
        _counter) && result;

    // test 7: tail = take_last(1)
    op     = d_filter_tail();
    result = d_assert_standalone(
        op->type == D_FILTER_OP_TAIL,
        "tail_type",
        "tail should set D_FILTER_OP_TAIL",
        _counter) && result;

    result = d_assert_standalone(
        op->params.count == 1,
        "tail_count",
        "tail should store count=1",
        _counter) && result;

    return result;
}


/*
d_tests_sa_filter_op_skip
  Tests skip operation constructors: skip_first, skip_last, init, rest.
  Tests the following:
  - field initialization for each skip variant
  - correct operation type assignment
  - init/rest as special cases
*/
bool
d_tests_sa_filter_op_skip
(
    struct d_test_counter* _counter
)
{
    struct d_filter_operation* op;
    bool                       result;

    result = true;

    // test 1: skip_first sets type and count
    op     = d_filter_skip_first(3);
    result = d_assert_standalone(
        op->type == D_FILTER_OP_SKIP_FIRST,
        "skip_first_type",
        "skip_first should set D_FILTER_OP_SKIP_FIRST",
        _counter) && result;

    result = d_assert_standalone(
        op->params.count == 3,
        "skip_first_count",
        "skip_first(3) should store count=3",
        _counter) && result;

    // test 2: skip_first(0) — no-op equivalent
    op     = d_filter_skip_first(0);
    result = d_assert_standalone(
        op->params.count == 0,
        "skip_first_zero",
        "skip_first(0) should store count=0",
        _counter) && result;

    // test 3: skip_last sets type and count
    op     = d_filter_skip_last(2);
    result = d_assert_standalone(
        op->type == D_FILTER_OP_SKIP_LAST,
        "skip_last_type",
        "skip_last should set D_FILTER_OP_SKIP_LAST",
        _counter) && result;

    result = d_assert_standalone(
        op->params.count == 2,
        "skip_last_count",
        "skip_last(2) should store count=2",
        _counter) && result;

    // test 4: init (all except last)
    op     = d_filter_init();
    result = d_assert_standalone(
        op->type == D_FILTER_OP_INIT,
        "init_type",
        "init should set D_FILTER_OP_INIT",
        _counter) && result;

    // test 5: rest (all except first)
    op     = d_filter_rest();
    result = d_assert_standalone(
        op->type == D_FILTER_OP_REST,
        "rest_type",
        "rest should set D_FILTER_OP_REST",
        _counter) && result;

    return result;
}


/*
d_tests_sa_filter_op_range_slice
  Tests range and slice operation constructors.
  Tests the following:
  - range stores start and end correctly
  - slice stores start, end, and step correctly
  - slice step=0 defaults to 1
*/
bool
d_tests_sa_filter_op_range_slice
(
    struct d_test_counter* _counter
)
{
    struct d_filter_operation* op;
    bool                       result;

    result = true;

    // test 1: range sets type, start, end
    op     = d_filter_range(2, 7);
    result = d_assert_standalone(
        op->type == D_FILTER_OP_RANGE,
        "range_type",
        "range should set D_FILTER_OP_RANGE",
        _counter) && result;

    result = d_assert_standalone(
        op->params.start == 2 && op->params.end == 7,
        "range_params",
        "range(2,7) should store start=2, end=7",
        _counter) && result;

    // test 2: range with start == end (empty range)
    op     = d_filter_range(5, 5);
    result = d_assert_standalone(
        op->params.start == 5 && op->params.end == 5,
        "range_empty",
        "range(5,5) should store start=end=5",
        _counter) && result;

    // test 3: slice sets type, start, end, step
    op     = d_filter_slice(1, 10, 3);
    result = d_assert_standalone(
        op->type == D_FILTER_OP_SLICE,
        "slice_type",
        "slice should set D_FILTER_OP_SLICE",
        _counter) && result;

    result = d_assert_standalone(
        op->params.start == 1 &&
        op->params.end == 10  &&
        op->params.step == 3,
        "slice_params",
        "slice(1,10,3) should store start=1, end=10, step=3",
        _counter) && result;

    // test 4: slice step=0 defaults to 1
    op     = d_filter_slice(0, 5, 0);
    result = d_assert_standalone(
        op->params.step == 1,
        "slice_step_zero",
        "slice with step=0 should default to step=1",
        _counter) && result;

    return result;
}


/*
d_tests_sa_filter_op_where
  Tests predicate-based operation constructors: where, where_context,
where_not, where_not_context.
  Tests the following:
  - correct type assignment (WHERE vs WHERE_NOT)
  - predicate function pointer storage
  - context pointer storage (NULL and non-NULL)
*/
bool
d_tests_sa_filter_op_where
(
    struct d_test_counter* _counter
)
{
    struct d_filter_operation* op;
    int                        threshold;
    bool                       result;

    result    = true;
    threshold = 10;

    // test 1: where stores predicate, NULL context
    op     = d_filter_where(pred_is_even);
    result = d_assert_standalone(
        op->type == D_FILTER_OP_WHERE,
        "where_type",
        "where should set D_FILTER_OP_WHERE",
        _counter) && result;

    result = d_assert_standalone(
        op->params.test == pred_is_even,
        "where_predicate",
        "where should store the predicate function",
        _counter) && result;

    result = d_assert_standalone(
        op->params.context == NULL,
        "where_context_null",
        "where should store NULL context",
        _counter) && result;

    // test 2: where_context stores predicate and context
    op     = d_filter_where_context(pred_greater_than_context, &threshold);
    result = d_assert_standalone(
        op->type == D_FILTER_OP_WHERE,
        "where_context_type",
        "where_context should set D_FILTER_OP_WHERE",
        _counter) && result;

    result = d_assert_standalone(
        op->params.test == pred_greater_than_context,
        "where_context_predicate",
        "where_context should store the predicate function",
        _counter) && result;

    result = d_assert_standalone(
        op->params.context == &threshold,
        "where_context_context",
        "where_context should store the context pointer",
        _counter) && result;

    // test 3: where_not stores predicate with WHERE_NOT type
    op     = d_filter_where_not(pred_is_positive);
    result = d_assert_standalone(
        op->type == D_FILTER_OP_WHERE_NOT,
        "where_not_type",
        "where_not should set D_FILTER_OP_WHERE_NOT",
        _counter) && result;

    result = d_assert_standalone(
        op->params.test == pred_is_positive,
        "where_not_predicate",
        "where_not should store the predicate function",
        _counter) && result;

    result = d_assert_standalone(
        op->params.context == NULL,
        "where_not_context_null",
        "where_not should store NULL context",
        _counter) && result;

    // test 4: where_not_context stores predicate and context
    op     = d_filter_where_not_context(pred_greater_than_context, &threshold);
    result = d_assert_standalone(
        op->type == D_FILTER_OP_WHERE_NOT,
        "where_not_context_type",
        "where_not_context should set D_FILTER_OP_WHERE_NOT",
        _counter) && result;

    result = d_assert_standalone(
        op->params.context == &threshold,
        "where_not_context_context",
        "where_not_context should store the context pointer",
        _counter) && result;

    return result;
}

/*
d_tests_sa_filter_op_indices
  Tests index-based operation constructors: at, at_indices.
  Tests the following:
  - at stores single index via start field
  - at_indices copies the indices array
  - at_indices with NULL/empty input
*/
bool
d_tests_sa_filter_op_indices
(
    struct d_test_counter* _counter
)
{
    struct d_filter_operation* op;
    size_t                     idx_array[4];
    bool                       result;

    result       = true;
    idx_array[0] = 0;
    idx_array[1] = 3;
    idx_array[2] = 5;
    idx_array[3] = 9;

    // test 1: at stores type INDICES, single index via start
    op     = d_filter_at(7);
    result = d_assert_standalone(
        op->type == D_FILTER_OP_INDICES,
        "at_type",
        "at should set D_FILTER_OP_INDICES",
        _counter) && result;

    result = d_assert_standalone(
        op->params.start == 7,
        "at_index",
        "at(7) should store start=7",
        _counter) && result;

    result = d_assert_standalone(
        op->params.count == 1,
        "at_count",
        "at should store count=1",
        _counter) && result;

    result = d_assert_standalone(
        op->params.indices == NULL,
        "at_no_indices_array",
        "at should not allocate indices array",
        _counter) && result;

    // test 2: at_indices copies the array
    op     = d_filter_at_indices(idx_array, 4);
    result = d_assert_standalone(
        op->type == D_FILTER_OP_INDICES,
        "at_indices_type",
        "at_indices should set D_FILTER_OP_INDICES",
        _counter) && result;

    result = d_assert_standalone(
        op->params.indices_count == 4,
        "at_indices_count",
        "at_indices should store count=4",
        _counter) && result;

    result = d_assert_standalone(
        op->params.indices != NULL,
        "at_indices_allocated",
        "at_indices should allocate indices array",
        _counter) && result;

    if (op->params.indices)
    {
        result = d_assert_standalone(
            op->params.indices[0] == 0 &&
            op->params.indices[1] == 3 &&
            op->params.indices[2] == 5 &&
            op->params.indices[3] == 9,
            "at_indices_values",
            "at_indices should copy all index values",
            _counter) && result;

        // verify it is a copy, not the original pointer
        result = d_assert_standalone(
            op->params.indices != idx_array,
            "at_indices_is_copy",
            "at_indices should allocate a new array (deep copy)",
            _counter) && result;
    }

    d_filter_operation_free(op);

    // test 3: at_indices with NULL array
    op     = d_filter_at_indices(NULL, 5);
    result = d_assert_standalone(
        op->params.indices == NULL,
        "at_indices_null_array",
        "at_indices with NULL array should store NULL",
        _counter) && result;

    // test 4: at_indices with zero count
    op     = d_filter_at_indices(idx_array, 0);
    result = d_assert_standalone(
        op->params.indices == NULL,
        "at_indices_zero_count",
        "at_indices with count=0 should store NULL",
        _counter) && result;

    return result;
}


/*
d_tests_sa_filter_op_distinct_reverse
  Tests distinct and reverse operation constructors.
  Tests the following:
  - distinct stores comparator function pointer
  - reverse sets type with no parameters
*/
bool
d_tests_sa_filter_op_distinct_reverse
(
    struct d_test_counter* _counter
)
{
    struct d_filter_operation* op;
    bool                       result;

    result = true;

    // test 1: distinct stores comparator
    op     = d_filter_distinct(cmp_int);
    result = d_assert_standalone(
        op->type == D_FILTER_OP_DISTINCT,
        "distinct_type",
        "distinct should set D_FILTER_OP_DISTINCT",
        _counter) && result;

    result = d_assert_standalone(
        op->params.comparator == cmp_int,
        "distinct_comparator",
        "distinct should store the comparator function",
        _counter) && result;

    // test 2: reverse sets type
    op     = d_filter_reverse();
    result = d_assert_standalone(
        op->type == D_FILTER_OP_REVERSE,
        "reverse_type",
        "reverse should set D_FILTER_OP_REVERSE",
        _counter) && result;

    return result;
}


/*
d_tests_sa_filter_op_free
  Tests d_filter_operation_free.
  Tests the following:
  - freeing operation with allocated indices
  - freeing operation with allocated name
  - freeing operation with no allocated resources
  - NULL safety
*/
bool
d_tests_sa_filter_op_free
(
    struct d_test_counter* _counter
)
{
    struct d_filter_operation* op;
    size_t                     idx_array[2];
    bool                       result;

    result       = true;
    idx_array[0] = 1;
    idx_array[1] = 2;

    // test 1: free operation with allocated indices
    op = d_filter_at_indices(idx_array, 2);
    result = d_assert_standalone(
        op->params.indices != NULL,
        "op_free_has_indices",
        "at_indices should have allocated indices",
        _counter) && result;

    d_filter_operation_free(op);

    result = d_assert_standalone(
        op->params.indices == NULL,
        "op_free_indices_nulled",
        "free should NULL the indices pointer",
        _counter) && result;

    // test 2: free operation with name
    op = d_filter_take_first(1);
    op->name = d_strdup("test_op_name");
    result = d_assert_standalone(
        op->name != NULL,
        "op_free_has_name",
        "operation should have name set",
        _counter) && result;

    d_filter_operation_free(op);

    result = d_assert_standalone(
        op->name == NULL,
        "op_free_name_nulled",
        "free should NULL the name pointer",
        _counter) && result;

    // test 3: free simple operation (no allocated resources)
    op = d_filter_reverse();

    d_filter_operation_free(op);

    result = d_assert_standalone(
        true,
        "op_free_simple",
        "free on operation with no resources should not crash",
        _counter) && result;

    // test 4: NULL safety
    d_filter_operation_free(NULL);
    result = d_assert_standalone(
        true,
        "op_free_null",
        "free(NULL) should not crash",
        _counter) && result;

    return result;
}

/*
d_tests_sa_filter_operations_all
  Aggregation function that runs all filter operation tests.
*/
bool
d_tests_sa_filter_operations_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Filter Operations\n");
    printf("  ----------------------------\n");

    result = d_tests_sa_filter_op_take(_counter)             && result;
    result = d_tests_sa_filter_op_skip(_counter)             && result;
    result = d_tests_sa_filter_op_range_slice(_counter)      && result;
    result = d_tests_sa_filter_op_where(_counter)            && result;
    result = d_tests_sa_filter_op_indices(_counter)          && result;
    result = d_tests_sa_filter_op_distinct_reverse(_counter) && result;
    result = d_tests_sa_filter_op_free(_counter)             && result;

    return result;
}
