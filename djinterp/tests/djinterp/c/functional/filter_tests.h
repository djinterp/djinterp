/*******************************************************************************
* djinterp [test]                                                 filter_tests.h
*
*   Unit test declarations for `filter.h` module.
*   Provides comprehensive testing of all filter operations, chain management,
* combinators, execution, utility functions, iterator interface, fluent
* builder, and convenience macros.
*
*
* path:      /tests/djinterp/c/functional/filter_tests.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.02.11
*                                                            revised: 2026.11.09
*******************************************************************************/

#ifndef DJINTERP_TESTS_FILTER_STANDALONE_
#define DJINTERP_TESTS_FILTER_STANDALONE_ 1

// std
#include <stddef.h>
#include "../../../../inc/re_std/c/cstdint/dstdint.h"  // fixed-width integers
#include <stdlib.h>
// djinterp
#include "../../../../inc/djinterp/c/djinterp.h"
#include "../../../../inc/djinterp/c/string_fn.h"
#include "../../../../inc/djinterp/c/functional/filter.h"
#include "../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's C face


// d_tests_filter_op_release
//   helper: frees an operation a test made and still owns, its contents and
// then the operation; NULL is ignored. (An operation added to a chain is
// freed with free alone: the chain holds its contents.)
void d_tests_filter_op_release(
    struct dj_filter_operation* _op
);


/******************************************************************************
 * I. FILTER OPERATION TESTS
 *****************************************************************************/
bool d_tests_filter_op_take(
    struct dj_test_suite* _suite
);
bool d_tests_filter_op_skip(
    struct dj_test_suite* _suite
);
bool d_tests_filter_op_range_slice(
    struct dj_test_suite* _suite
);
bool d_tests_filter_op_where(
    struct dj_test_suite* _suite
);
bool d_tests_filter_op_indices(
    struct dj_test_suite* _suite
);
bool d_tests_filter_op_distinct_reverse(
    struct dj_test_suite* _suite
);
bool d_tests_filter_op_free(
    struct dj_test_suite* _suite
);

// I.   aggregation function
bool d_tests_filter_operations_all(
    struct dj_test_suite* _suite
);


/******************************************************************************
 * II. FILTER CHAIN MANAGEMENT TESTS
 *****************************************************************************/
bool d_tests_filter_chain_create(
    struct dj_test_suite* _suite
);
bool d_tests_filter_chain_add(
    struct dj_test_suite* _suite
);
bool d_tests_filter_chain_convenience(
    struct dj_test_suite* _suite
);
bool d_tests_filter_chain_combine(
    struct dj_test_suite* _suite
);
bool d_tests_filter_chain_manipulate(
    struct dj_test_suite* _suite
);
bool d_tests_filter_chain_properties(
    struct dj_test_suite* _suite
);

// II.  aggregation function
bool d_tests_filter_chain_all(
    struct dj_test_suite* _suite
);


/******************************************************************************
 * III. FILTER COMBINATOR TESTS
 *****************************************************************************/
bool d_tests_filter_union(
    struct dj_test_suite* _suite
);
bool d_tests_filter_intersection(
    struct dj_test_suite* _suite
);
bool d_tests_filter_difference(
    struct dj_test_suite* _suite
);

// III. aggregation function
bool d_tests_filter_combinators_all(
    struct dj_test_suite* _suite
);


/******************************************************************************
 * IV. EXECUTION AND APPLICATION TESTS
 *****************************************************************************/
bool d_tests_filter_apply_operation(
    struct dj_test_suite* _suite
);
bool d_tests_filter_apply_chain(
    struct dj_test_suite* _suite
);
bool d_tests_filter_apply_combinators(
    struct dj_test_suite* _suite
);
bool d_tests_filter_counting(
    struct dj_test_suite* _suite
);
bool d_tests_filter_get_indices(
    struct dj_test_suite* _suite
);
bool d_tests_filter_in_place(
    struct dj_test_suite* _suite
);
bool d_tests_filter_result_free(
    struct dj_test_suite* _suite
);
bool d_tests_filter_matches_element(
    struct dj_test_suite* _suite
);

// IV.  aggregation function
bool d_tests_filter_execution_all(
    struct dj_test_suite* _suite
);


/******************************************************************************
 * V. UTILITY FUNCTION TESTS
 *****************************************************************************/
bool d_tests_filter_validation(
    struct dj_test_suite* _suite
);
bool d_tests_filter_to_string(
    struct dj_test_suite* _suite
);
bool d_tests_filter_from_string(
    struct dj_test_suite* _suite
);
bool d_tests_filter_optimize(
    struct dj_test_suite* _suite
);
bool d_tests_filter_estimate(
    struct dj_test_suite* _suite
);

// V.   aggregation function
bool d_tests_filter_utility_all(
    struct dj_test_suite* _suite
);


/******************************************************************************
 * VI. ITERATOR INTERFACE TESTS
 *****************************************************************************/
bool d_tests_filter_iterator_create(
    struct dj_test_suite* _suite
);
bool d_tests_filter_iterator_traverse(
    struct dj_test_suite* _suite
);
bool d_tests_filter_iterator_reset(
    struct dj_test_suite* _suite
);
bool d_tests_filter_iterator_edge(
    struct dj_test_suite* _suite
);

// VI.  aggregation function
bool d_tests_filter_iterator_all(
    struct dj_test_suite* _suite
);


/******************************************************************************
 * VII. FLUENT FILTER BUILDER TESTS
 *****************************************************************************/
bool d_tests_filter_builder_create(
    struct dj_test_suite* _suite
);
bool d_tests_filter_builder_fluent(
    struct dj_test_suite* _suite
);
bool d_tests_filter_builder_finalize(
    struct dj_test_suite* _suite
);
bool d_tests_filter_builder_errors(
    struct dj_test_suite* _suite
);

// VII. aggregation function
bool d_tests_filter_builder_all(
    struct dj_test_suite* _suite
);


/******************************************************************************
 * VIII. CONVENIENCE MACRO TESTS
 *****************************************************************************/
bool d_tests_filter_macros_single(
    struct dj_test_suite* _suite
);
bool d_tests_filter_macros_fluent(
    struct dj_test_suite* _suite
);

// VIII. aggregation function
bool d_tests_filter_macros_all(
    struct dj_test_suite* _suite
);


/******************************************************************************
 * MODULE-LEVEL AGGREGATION
 *****************************************************************************/
bool d_tests_filter_run_all(
    struct dj_test_suite* _suite
);


#endif  // DJINTERP_TESTS_FILTER_STANDALONE_
