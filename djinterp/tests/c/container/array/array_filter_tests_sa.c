#include ".\array_filter_tests_sa.h"


/*
d_tests_sa_array_filter_run_all
  Module-level aggregation function that runs all array_filter tests.
  Executes tests for all categories:
  - Result structure (d_array_filter_result struct, d_filter_result_type enum)
  - Single-operation filters (take, skip, range, slice, predicate, index,
    transformation)
  - In-place filter operations (predicate, positional, distinct)
  - Chain and combinator application (chain, union, intersection, difference)
  - Query functions (count_where, any_match, all_match, none_match,
    find_first, find_last)
  - Result management (data, count, ok, release, free)
  - Convenience macros (WHERE, WHERE_CTX, FIRST_N, LAST_N, RANGE, SLICE,
    DISTINCT, IN_PLACE)
  - Fluent builder helpers (BEGIN/END, apply_builder, multi-step chains)
*/
bool
d_tests_sa_array_filter_run_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    // run all test categories
    result = d_tests_sa_array_filter_result_all(_counter) && result;
    result = d_tests_sa_array_filter_single_op_all(_counter) && result;
    result = d_tests_sa_array_filter_in_place_all(_counter) && result;
    result = d_tests_sa_array_filter_chain_all(_counter) && result;
    result = d_tests_sa_array_filter_query_all(_counter) && result;
    result = d_tests_sa_array_filter_result_mgmt_all(_counter) && result;
    result = d_tests_sa_array_filter_macro_all(_counter) && result;
    result = d_tests_sa_array_filter_builder_all(_counter) && result;

    return result;
}
