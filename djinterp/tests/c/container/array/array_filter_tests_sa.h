/******************************************************************************
* djinterp [test]                                     array_filter_tests_sa.h
*
*   Unit test declarations for the `contiguous_filter.h` module.
*   Provides comprehensive testing of all contiguous_filter functions including
* result structure validation, single-operation filters (take, skip, range,
* slice, predicate, index, transformation), in-place filtering, chain and
* combinator application, query functions, result management, convenience
* macros, and fluent builder helpers.
*
*   Each section mirrors a section of contiguous_filter.h and has a
* corresponding .c source file containing the function definitions.
*
*
* path:      \tests\container\array_filter_tests_sa.h
* links(s):  TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.02.20
******************************************************************************/

#ifndef DJINTERP_TESTS_CONTIGUOUS_FILTER_SA_
#define DJINTERP_TESTS_CONTIGUOUS_FILTER_SA_ 1

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include "..\..\..\..\inc\c\djinterp.h"
#include "..\..\..\..\inc\c\dmemory.h"
#include "..\..\..\..\inc\c\string_fn.h"
#include "..\..\..\..\inc\c\test\test_standalone.h"
#include "..\..\..\..\inc\c\container\array\array_filter.h"


/******************************************************************************
 * TEST FIXTURE HELPERS
 *****************************************************************************/
// Shared predicate, comparator, and data setup functions used across
// multiple test source files.  Defined in array_filter_tests_sa_fixtures.c.

// D_TEST_ARRAY_FILTER_DATA_SIZE
//   constant: standard test array size.
#define D_TEST_ARRAY_FILTER_DATA_SIZE 10

// d_test_af_fill_sequential
//   fills an int array with values [0, 1, 2, ..., count-1].
void d_test_af_fill_sequential(int* _arr, size_t _count);

// d_test_af_fill_with_duplicates
//   fills an int array with values that contain duplicates:
//   {3, 1, 4, 1, 5, 9, 2, 6, 5, 3}.
void d_test_af_fill_with_duplicates(int* _arr, size_t _count);

// d_test_af_is_even
//   predicate: returns true if the int element is even.
bool d_test_af_is_even(const void* _element, void* _context);

// d_test_af_is_positive
//   predicate: returns true if the int element is > 0.
bool d_test_af_is_positive(const void* _element, void* _context);

// d_test_af_is_greater_than
//   predicate: returns true if the int element > *(int*)_context.
bool d_test_af_is_greater_than(const void* _element, void* _context);

// d_test_af_always_true
//   predicate: returns true for any element.
bool d_test_af_always_true(const void* _element, void* _context);

// d_test_af_always_false
//   predicate: returns false for any element.
bool d_test_af_always_false(const void* _element, void* _context);

// d_test_af_compare_int
//   comparator: three-way comparison for int elements.
int d_test_af_compare_int(const void* _a, const void* _b, void* _context);


/******************************************************************************
 * I. ARRAY FILTER RESULT STRUCTURE TESTS
 *****************************************************************************/
// d_contiguous_filter_result struct members
bool d_tests_sa_array_filter_result_struct(struct d_test_counter* _counter);
// d_filter_result_type enum values
bool d_tests_sa_array_filter_result_status_enum(struct d_test_counter* _counter);

// I.   aggregation function
bool d_tests_sa_array_filter_result_all(struct d_test_counter* _counter);


/******************************************************************************
 * II. SINGLE-OPERATION FILTER FUNCTIONS
 *****************************************************************************/
// ii.i.  take operations
// d_contiguous_filter_take_first function
bool d_tests_sa_array_filter_take_first(struct d_test_counter* _counter);
// d_contiguous_filter_take_last function
bool d_tests_sa_array_filter_take_last(struct d_test_counter* _counter);
// d_contiguous_filter_take_nth function
bool d_tests_sa_array_filter_take_nth(struct d_test_counter* _counter);
// d_contiguous_filter_head function
bool d_tests_sa_array_filter_head(struct d_test_counter* _counter);
// d_contiguous_filter_tail function
bool d_tests_sa_array_filter_tail(struct d_test_counter* _counter);

// ii.ii. skip operations
// d_contiguous_filter_skip_first function
bool d_tests_sa_array_filter_skip_first(struct d_test_counter* _counter);
// d_contiguous_filter_skip_last function
bool d_tests_sa_array_filter_skip_last(struct d_test_counter* _counter);
// d_contiguous_filter_init function
bool d_tests_sa_array_filter_init(struct d_test_counter* _counter);
// d_contiguous_filter_rest function
bool d_tests_sa_array_filter_rest(struct d_test_counter* _counter);

// ii.iii. range and slice operations
// d_contiguous_filter_range function
bool d_tests_sa_array_filter_range(struct d_test_counter* _counter);
// d_contiguous_filter_slice function
bool d_tests_sa_array_filter_slice(struct d_test_counter* _counter);

// ii.iv. predicate-based operations
// d_contiguous_filter_where function
bool d_tests_sa_array_filter_where(struct d_test_counter* _counter);
// d_contiguous_filter_where_not function
bool d_tests_sa_array_filter_where_not(struct d_test_counter* _counter);

// ii.v.  index-based operations
// d_contiguous_filter_at_indices function
bool d_tests_sa_array_filter_at_indices(struct d_test_counter* _counter);

// ii.vi. transformation operations
// d_contiguous_filter_distinct function
bool d_tests_sa_array_filter_distinct(struct d_test_counter* _counter);
// d_contiguous_filter_reverse function
bool d_tests_sa_array_filter_reverse(struct d_test_counter* _counter);

// II.  aggregation function
bool d_tests_sa_array_filter_single_op_all(struct d_test_counter* _counter);


/******************************************************************************
 * III. IN-PLACE FILTER OPERATIONS
 *****************************************************************************/
// d_contiguous_filter_in_place function
bool d_tests_sa_array_filter_in_place(struct d_test_counter* _counter);
// d_contiguous_filter_in_place_not function
bool d_tests_sa_array_filter_in_place_not(struct d_test_counter* _counter);
// d_contiguous_filter_in_place_take_first function
bool d_tests_sa_array_filter_in_place_take_first(struct d_test_counter* _counter);
// d_contiguous_filter_in_place_skip_first function
bool d_tests_sa_array_filter_in_place_skip_first(struct d_test_counter* _counter);
// d_contiguous_filter_in_place_distinct function
bool d_tests_sa_array_filter_in_place_distinct(struct d_test_counter* _counter);

// III. aggregation function
bool d_tests_sa_array_filter_in_place_all(struct d_test_counter* _counter);


/******************************************************************************
 * IV. CHAIN AND COMBINATOR APPLICATION
 *****************************************************************************/
// d_contiguous_filter_apply_chain function
bool d_tests_sa_array_filter_apply_chain(struct d_test_counter* _counter);
// d_contiguous_filter_apply_union function
bool d_tests_sa_array_filter_apply_union(struct d_test_counter* _counter);
// d_contiguous_filter_apply_intersection function
bool d_tests_sa_array_filter_apply_intersection(struct d_test_counter* _counter);
// d_contiguous_filter_apply_difference function
bool d_tests_sa_array_filter_apply_difference(struct d_test_counter* _counter);

// IV.  aggregation function
bool d_tests_sa_array_filter_chain_all(struct d_test_counter* _counter);


/******************************************************************************
 * V. QUERY FUNCTIONS
 *****************************************************************************/
// d_contiguous_filter_count_where function
bool d_tests_sa_array_filter_count_where(struct d_test_counter* _counter);
// d_contiguous_filter_any_match function
bool d_tests_sa_array_filter_any_match(struct d_test_counter* _counter);
// d_contiguous_filter_all_match function
bool d_tests_sa_array_filter_all_match(struct d_test_counter* _counter);
// d_contiguous_filter_none_match function
bool d_tests_sa_array_filter_none_match(struct d_test_counter* _counter);
// d_contiguous_filter_find_first function
bool d_tests_sa_array_filter_find_first(struct d_test_counter* _counter);
// d_contiguous_filter_find_last function
bool d_tests_sa_array_filter_find_last(struct d_test_counter* _counter);

// V.   aggregation function
bool d_tests_sa_array_filter_query_all(struct d_test_counter* _counter);


/******************************************************************************
 * VI. RESULT MANAGEMENT
 *****************************************************************************/
// d_contiguous_filter_result_data function
bool d_tests_sa_array_filter_result_data(struct d_test_counter* _counter);
// d_contiguous_filter_result_count function
bool d_tests_sa_array_filter_result_count_fn(struct d_test_counter* _counter);
// d_contiguous_filter_result_ok function
bool d_tests_sa_array_filter_result_ok(struct d_test_counter* _counter);
// d_contiguous_filter_result_release function
bool d_tests_sa_array_filter_result_release(struct d_test_counter* _counter);
// d_contiguous_filter_result_free function
bool d_tests_sa_array_filter_result_free(struct d_test_counter* _counter);

// VI.  aggregation function
bool d_tests_sa_array_filter_result_mgmt_all(struct d_test_counter* _counter);


/******************************************************************************
 * VII. CONVENIENCE MACROS
 *****************************************************************************/
// D_CONTIGUOUS_FILTER_WHERE macro
bool d_tests_sa_array_filter_macro_where(struct d_test_counter* _counter);
// D_CONTIGUOUS_FILTER_WHERE_CTX macro
bool d_tests_sa_array_filter_macro_where_ctx(struct d_test_counter* _counter);
// D_CONTIGUOUS_FILTER_FIRST_N macro
bool d_tests_sa_array_filter_macro_first_n(struct d_test_counter* _counter);
// D_CONTIGUOUS_FILTER_LAST_N macro
bool d_tests_sa_array_filter_macro_last_n(struct d_test_counter* _counter);
// D_CONTIGUOUS_FILTER_RANGE macro
bool d_tests_sa_array_filter_macro_range(struct d_test_counter* _counter);
// D_CONTIGUOUS_FILTER_SLICE macro
bool d_tests_sa_array_filter_macro_slice(struct d_test_counter* _counter);
// D_CONTIGUOUS_FILTER_DISTINCT macro
bool d_tests_sa_array_filter_macro_distinct(struct d_test_counter* _counter);
// D_CONTIGUOUS_FILTER_IN_PLACE macro
bool d_tests_sa_array_filter_macro_in_place(struct d_test_counter* _counter);

// VII. aggregation function
bool d_tests_sa_array_filter_macro_all(struct d_test_counter* _counter);


/******************************************************************************
 * VIII. FLUENT BUILDER HELPERS
 *****************************************************************************/
// D_CONTIGUOUS_FILTER_BEGIN / D_CONTIGUOUS_FILTER_END macros
bool d_tests_sa_array_filter_builder_begin_end(struct d_test_counter* _counter);
// d_contiguous_filter_apply_builder function
bool d_tests_sa_array_filter_apply_builder(struct d_test_counter* _counter);
// fluent builder multi-step chain
bool d_tests_sa_array_filter_builder_multi_step(struct d_test_counter* _counter);

// VIII. aggregation function
bool d_tests_sa_array_filter_builder_all(struct d_test_counter* _counter);


/******************************************************************************
 * MODULE-LEVEL AGGREGATION
 *****************************************************************************/
bool d_tests_sa_array_filter_run_all(struct d_test_counter* _counter);


#endif  // DJINTERP_TESTS_CONTIGUOUS_FILTER_SA_
