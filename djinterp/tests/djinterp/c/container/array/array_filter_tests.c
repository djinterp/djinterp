/*******************************************************************************
* djinterp [container]                                      array_filter_tests.c
*
*
* path:      /tests/djinterp/c/container/array/array_filter_tests.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.02.20
*                                                            revised: 2026.11.09
*******************************************************************************/
// djinterp
#include "array_filter_tests.h"
#include "../../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's C face


/*
d_tests_array_filter_run_all
  Module-level aggregation function that runs all contiguous_filter tests.
  Executes tests for all categories:
  - Result structure (dj_contiguous_filter_result struct, dj_filter_result_type enum)
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
bool d_tests_array_filter_run_all(
    struct dj_test_suite* _suite
)
{
    bool result;

    result = true;

    // run all test categories
    result = d_tests_array_filter_result_all(_suite) && result;
    result = d_tests_array_filter_single_op_all(_suite) && result;
    result = d_tests_array_filter_in_place_all(_suite) && result;
    result = d_tests_array_filter_chain_all(_suite) && result;
    result = d_tests_array_filter_query_all(_suite) && result;
    result = d_tests_array_filter_result_mgmt_all(_suite) && result;
    result = d_tests_array_filter_macro_all(_suite) && result;
    result = d_tests_array_filter_builder_all(_suite) && result;

    return result;
}
