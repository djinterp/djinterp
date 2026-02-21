/******************************************************************************
* djinterp [test]                                     array_filter_tests_sa.h
*
*   Unit test declarations for the `array_filter.h` module.
*   Provides comprehensive testing of all array_filter functions including
* result structure validation, single-operation filters (take, skip, range,
* slice, predicate, index, transformation), in-place filtering, chain and
* combinator application, query functions, result management, convenience
* macros, and fluent builder helpers.
*
*   Each section mirrors a section of array_filter.h and has a corresponding
* .c source file containing the function definitions.
*
*
* path:      \tests\container\array\array_filter_tests_sa.h
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.02.20
******************************************************************************/

#ifndef DJINTERP_TESTS_ARRAY_FILTER_STANDALONE_
#define DJINTERP_TESTS_ARRAY_FILTER_STANDALONE_ 1

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include "..\..\..\inc\test\test_standalone.h"
#include "..\..\..\inc\string_fn.h"
#include "..\..\..\inc\container\array\array_filter.h"


/******************************************************************************
 * TEST FIXTURE HELPERS
 *****************************************************************************/
 // Shared predicate, comparator, and data setup functions used across
 // multiple test source files.  Defined in array_filter_tests_sa_fixtures.c.

 // D_INTERNAL_TEST_ARRAY_FILTER_DATA_SIZE 
 //   constant: standard test array size.
#define D_INTERNAL_TEST_ARRAY_FILTER_DATA_SIZE  10

void d_tests_array_filter_fill_sequential(int* _arr, size_t _count);
void d_tests_array_filter_fill_with_duplicates(int* _arr, size_t _count);
bool d_tests_array_filter_is_even(const void* _element, void* _context);
bool d_tests_array_filter_is_positive(const void* _element, void* _context);
bool d_tests_array_filter_is_greater_than(const void* _element, void* _context);
bool d_tests_array_filter_always_true(const void* _element, void* _context);
bool d_tests_array_filter_always_false(const void* _element, void* _context);
int  d_tests_array_filter_compare_int(const void* _a, const void* _b, void* _context);


/******************************************************************************
 * I. ARRAY FILTER RESULT STRUCTURE TESTS
 *****************************************************************************/
bool d_tests_sa_array_filter_result_struct(struct d_test_counter* _counter);
bool d_tests_sa_array_filter_result_status_enum(struct d_test_counter* _counter);
bool d_tests_sa_array_filter_result_all(struct d_test_counter* _counter);


/******************************************************************************
 * II. SINGLE-OPERATION FILTER FUNCTIONS
 *****************************************************************************/
 // ii.i.  take operations
bool d_tests_sa_array_filter_take_first(struct d_test_counter* _counter);
bool d_tests_sa_array_filter_take_last(struct d_test_counter* _counter);
bool d_tests_sa_array_filter_take_nth(struct d_test_counter* _counter);
bool d_tests_sa_array_filter_head(struct d_test_counter* _counter);
bool d_tests_sa_array_filter_tail(struct d_test_counter* _counter);

// ii.ii. skip operations
bool d_tests_sa_array_filter_skip_first(struct d_test_counter* _counter);
bool d_tests_sa_array_filter_skip_last(struct d_test_counter* _counter);
bool d_tests_sa_array_filter_init(struct d_test_counter* _counter);
bool d_tests_sa_array_filter_rest(struct d_test_counter* _counter);

// ii.iii. range and slice operations
bool d_tests_sa_array_filter_range(struct d_test_counter* _counter);
bool d_tests_sa_array_filter_slice(struct d_test_counter* _counter);

// ii.iv. predicate-based operations
bool d_tests_sa_array_filter_where(struct d_test_counter* _counter);
bool d_tests_sa_array_filter_where_not(struct d_test_counter* _counter);

// ii.v.  index-based operations
bool d_tests_sa_array_filter_at_indices(struct d_test_counter* _counter);

// ii.vi. transformation operations
bool d_tests_sa_array_filter_distinct(struct d_test_counter* _counter);
bool d_tests_sa_array_filter_reverse(struct d_test_counter* _counter);

bool d_tests_sa_array_filter_single_op_all(struct d_test_counter* _counter);


/******************************************************************************
 * III. IN-PLACE FILTER OPERATIONS
 *****************************************************************************/
bool d_tests_sa_array_filter_in_place(struct d_test_counter* _counter);
bool d_tests_sa_array_filter_in_place_not(struct d_test_counter* _counter);
bool d_tests_sa_array_filter_in_place_take_first(struct d_test_counter* _counter);
bool d_tests_sa_array_filter_in_place_skip_first(struct d_test_counter* _counter);
bool d_tests_sa_array_filter_in_place_distinct(struct d_test_counter* _counter);
bool d_tests_sa_array_filter_in_place_all(struct d_test_counter* _counter);


/******************************************************************************
 * IV. CHAIN AND COMBINATOR APPLICATION
 *****************************************************************************/
bool d_tests_sa_array_filter_apply_chain(struct d_test_counter* _counter);
bool d_tests_sa_array_filter_apply_union(struct d_test_counter* _counter);
bool d_tests_sa_array_filter_apply_intersection(struct d_test_counter* _counter);
bool d_tests_sa_array_filter_apply_difference(struct d_test_counter* _counter);
bool d_tests_sa_array_filter_chain_all(struct d_test_counter* _counter);


/******************************************************************************
 * V. QUERY FUNCTIONS
 *****************************************************************************/
bool d_tests_sa_array_filter_count_where(struct d_test_counter* _counter);
bool d_tests_sa_array_filter_any_match(struct d_test_counter* _counter);
bool d_tests_sa_array_filter_all_match(struct d_test_counter* _counter);
bool d_tests_sa_array_filter_none_match(struct d_test_counter* _counter);
bool d_tests_sa_array_filter_find_first(struct d_test_counter* _counter);
bool d_tests_sa_array_filter_find_last(struct d_test_counter* _counter);
bool d_tests_sa_array_filter_query_all(struct d_test_counter* _counter);


/******************************************************************************
 * VI. RESULT MANAGEMENT
 *****************************************************************************/
bool d_tests_sa_array_filter_result_data(struct d_test_counter* _counter);
bool d_tests_sa_array_filter_result_count_fn(struct d_test_counter* _counter);
bool d_tests_sa_array_filter_result_ok(struct d_test_counter* _counter);
bool d_tests_sa_array_filter_result_release(struct d_test_counter* _counter);
bool d_tests_sa_array_filter_result_free(struct d_test_counter* _counter);
bool d_tests_sa_array_filter_result_mgmt_all(struct d_test_counter* _counter);


/******************************************************************************
 * VII. CONVENIENCE MACROS
 *****************************************************************************/
bool d_tests_sa_array_filter_macro_where(struct d_test_counter* _counter);
bool d_tests_sa_array_filter_macro_where_ctx(struct d_test_counter* _counter);
bool d_tests_sa_array_filter_macro_first_n(struct d_test_counter* _counter);
bool d_tests_sa_array_filter_macro_last_n(struct d_test_counter* _counter);
bool d_tests_sa_array_filter_macro_range(struct d_test_counter* _counter);
bool d_tests_sa_array_filter_macro_slice(struct d_test_counter* _counter);
bool d_tests_sa_array_filter_macro_distinct(struct d_test_counter* _counter);
bool d_tests_sa_array_filter_macro_in_place(struct d_test_counter* _counter);
bool d_tests_sa_array_filter_macro_all(struct d_test_counter* _counter);


/******************************************************************************
 * VIII. FLUENT BUILDER HELPERS
 *****************************************************************************/
bool d_tests_sa_array_filter_builder_begin_end(struct d_test_counter* _counter);
bool d_tests_sa_array_filter_apply_builder(struct d_test_counter* _counter);
bool d_tests_sa_array_filter_builder_multi_step(struct d_test_counter* _counter);
bool d_tests_sa_array_filter_builder_all(struct d_test_counter* _counter);


/******************************************************************************
 * MODULE-LEVEL AGGREGATION
 *****************************************************************************/
bool d_tests_sa_array_filter_run_all(struct d_test_counter* _counter);


#endif  // DJINTERP_TESTS_ARRAY_FILTER_SA_
