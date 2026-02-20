/******************************************************************************
* djinterp [test]                                            filter_tests_sa.h
*
*   Unit test declarations for `filter.h` module.
*   Provides comprehensive testing of all filter operations, chain management,
* combinators, execution, utility functions, iterator interface, fluent
* builder, and convenience macros.
*
*
* path:      \tests\functional\filter_tests_sa.h
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2026.02.11
******************************************************************************/

#ifndef DJINTERP_TESTS_FILTER_STANDALONE_
#define DJINTERP_TESTS_FILTER_STANDALONE_ 1

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include "..\..\inc\djinterp.h"
#include "..\..\inc\string_fn.h"
#include "..\..\inc\test\test_standalone.h"
#include "..\..\inc\functional\filter.h"


/******************************************************************************
 * I. FILTER OPERATION TESTS
 *****************************************************************************/
bool d_tests_sa_filter_op_take(struct d_test_counter* _counter);
bool d_tests_sa_filter_op_skip(struct d_test_counter* _counter);
bool d_tests_sa_filter_op_range_slice(struct d_test_counter* _counter);
bool d_tests_sa_filter_op_where(struct d_test_counter* _counter);
bool d_tests_sa_filter_op_indices(struct d_test_counter* _counter);
bool d_tests_sa_filter_op_distinct_reverse(struct d_test_counter* _counter);
bool d_tests_sa_filter_op_free(struct d_test_counter* _counter);

// I.   aggregation function
bool d_tests_sa_filter_operations_all(struct d_test_counter* _counter);


/******************************************************************************
 * II. FILTER CHAIN MANAGEMENT TESTS
 *****************************************************************************/
bool d_tests_sa_filter_chain_create(struct d_test_counter* _counter);
bool d_tests_sa_filter_chain_add(struct d_test_counter* _counter);
bool d_tests_sa_filter_chain_convenience(struct d_test_counter* _counter);
bool d_tests_sa_filter_chain_combine(struct d_test_counter* _counter);
bool d_tests_sa_filter_chain_manipulate(struct d_test_counter* _counter);
bool d_tests_sa_filter_chain_properties(struct d_test_counter* _counter);

// II.  aggregation function
bool d_tests_sa_filter_chain_all(struct d_test_counter* _counter);


/******************************************************************************
 * III. FILTER COMBINATOR TESTS
 *****************************************************************************/
bool d_tests_sa_filter_union(struct d_test_counter* _counter);
bool d_tests_sa_filter_intersection(struct d_test_counter* _counter);
bool d_tests_sa_filter_difference(struct d_test_counter* _counter);

// III. aggregation function
bool d_tests_sa_filter_combinators_all(struct d_test_counter* _counter);


/******************************************************************************
 * IV. EXECUTION AND APPLICATION TESTS
 *****************************************************************************/
bool d_tests_sa_filter_apply_operation(struct d_test_counter* _counter);
bool d_tests_sa_filter_apply_chain(struct d_test_counter* _counter);
bool d_tests_sa_filter_apply_combinators(struct d_test_counter* _counter);
bool d_tests_sa_filter_counting(struct d_test_counter* _counter);
bool d_tests_sa_filter_get_indices(struct d_test_counter* _counter);
bool d_tests_sa_filter_in_place(struct d_test_counter* _counter);
bool d_tests_sa_filter_result_free(struct d_test_counter* _counter);
bool d_tests_sa_filter_matches_element(struct d_test_counter* _counter);

// IV.  aggregation function
bool d_tests_sa_filter_execution_all(struct d_test_counter* _counter);


/******************************************************************************
 * V. UTILITY FUNCTION TESTS
 *****************************************************************************/
bool d_tests_sa_filter_validation(struct d_test_counter* _counter);
bool d_tests_sa_filter_to_string(struct d_test_counter* _counter);
bool d_tests_sa_filter_from_string(struct d_test_counter* _counter);
bool d_tests_sa_filter_optimize(struct d_test_counter* _counter);
bool d_tests_sa_filter_estimate(struct d_test_counter* _counter);

// V.   aggregation function
bool d_tests_sa_filter_utility_all(struct d_test_counter* _counter);


/******************************************************************************
 * VI. ITERATOR INTERFACE TESTS
 *****************************************************************************/
bool d_tests_sa_filter_iterator_create(struct d_test_counter* _counter);
bool d_tests_sa_filter_iterator_traverse(struct d_test_counter* _counter);
bool d_tests_sa_filter_iterator_reset(struct d_test_counter* _counter);
bool d_tests_sa_filter_iterator_edge(struct d_test_counter* _counter);

// VI.  aggregation function
bool d_tests_sa_filter_iterator_all(struct d_test_counter* _counter);


/******************************************************************************
 * VII. FLUENT FILTER BUILDER TESTS
 *****************************************************************************/
bool d_tests_sa_filter_builder_create(struct d_test_counter* _counter);
bool d_tests_sa_filter_builder_fluent(struct d_test_counter* _counter);
bool d_tests_sa_filter_builder_finalize(struct d_test_counter* _counter);
bool d_tests_sa_filter_builder_errors(struct d_test_counter* _counter);

// VII. aggregation function
bool d_tests_sa_filter_builder_all(struct d_test_counter* _counter);


/******************************************************************************
 * VIII. CONVENIENCE MACRO TESTS
 *****************************************************************************/
bool d_tests_sa_filter_macros_single(struct d_test_counter* _counter);
bool d_tests_sa_filter_macros_fluent(struct d_test_counter* _counter);

// VIII. aggregation function
bool d_tests_sa_filter_macros_all(struct d_test_counter* _counter);


/******************************************************************************
 * MODULE-LEVEL AGGREGATION
 *****************************************************************************/
bool d_tests_sa_filter_run_all(struct d_test_counter* _counter);


#endif  // DJINTERP_TESTS_FILTER_STANDALONE_
