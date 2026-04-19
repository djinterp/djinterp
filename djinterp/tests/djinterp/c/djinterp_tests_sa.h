/******************************************************************************
* djinterp [test]                                      djinterp_tests_sa.h
*
* Unified test suite header for djinterp.h - all test declarations.
*
* This is the single header file that declares all test functions from all
* test modules. Each module has its own .c file:
*   - djinterp_tests_sa_index.c          (d_index functions)
*   - djinterp_tests_sa_index_macros.c   (index manipulation macros)
*   - djinterp_tests_sa_array.c          (array utility macros)
*   - djinterp_tests_sa_boolean.c        (boolean constants/macros)
*   - djinterp_tests_sa_function_ptr.c   (function pointer types)
*   - djinterp_tests_sa_edge.c           (edge cases and boundaries)
*   - djinterp_tests_sa.c                (master runner)
*
* Usage:
*   #include ".\djinterp_tests_sa.h"
*   
*   struct d_test_counter counter = {0, 0, 0, 0};
*   bool result = d_tests_sa_run_all(&counter);
*
* path:      \test\djinterp_tests_sa.h
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2026.01.15
******************************************************************************/

#ifndef DJINTERP_HEADER_TESTS_STANDALONE_
#define DJINTERP_HEADER_TESTS_STANDALONE_ 1

#include <limits.h>
#include <stdlib.h>
#include "..\..\inc\c\djinterp.h"
#include "..\..\inc\c\test\test_standalone.h"


/******************************************************************************
 * MODULE 1: d_index FUNCTION TESTS (djinterp_tests_sa_index.c)
 *****************************************************************************/

// d_index_convert_fast() tests
bool d_tests_sa_index_convert_fast_basic(struct d_test_counter* _test_info);
bool d_tests_sa_index_convert_fast_edge_cases(struct d_test_counter* _test_info);
bool d_tests_sa_index_convert_fast_large_arrays(struct d_test_counter* _test_info);
bool d_tests_sa_index_convert_fast_all(struct d_test_counter* _test_info);

// d_index_convert_safe() tests
bool d_tests_sa_index_convert_safe_valid(struct d_test_counter* _test_info);
bool d_tests_sa_index_convert_safe_invalid(struct d_test_counter* _test_info);
bool d_tests_sa_index_convert_safe_null_handling(struct d_test_counter* _test_info);
bool d_tests_sa_index_convert_safe_all(struct d_test_counter* _test_info);

// d_index_is_valid() tests
bool d_tests_sa_index_is_valid_positive(struct d_test_counter* _test_info);
bool d_tests_sa_index_is_valid_negative(struct d_test_counter* _test_info);
bool d_tests_sa_index_is_valid_boundary(struct d_test_counter* _test_info);
bool d_tests_sa_index_is_valid_all(struct d_test_counter* _test_info);

// Comprehensive d_index function tests
bool d_tests_sa_index_functions_all(struct d_test_counter* _test_info);


/******************************************************************************
 * MODULE 2: INDEX MANIPULATION MACRO TESTS (djinterp_tests_sa_index_macros.c)
 *****************************************************************************/

// Individual macro tests
bool d_tests_sa_macro_clamp_index(struct d_test_counter* _test_info);
bool d_tests_sa_macro_index_in_bounds(struct d_test_counter* _test_info);
bool d_tests_sa_macro_safe_arr_idx(struct d_test_counter* _test_info);
bool d_tests_sa_macro_is_valid_index(struct d_test_counter* _test_info);
bool d_tests_sa_macro_is_valid_index_n(struct d_test_counter* _test_info);
bool d_tests_sa_macro_neg_idx(struct d_test_counter* _test_info);
bool d_tests_sa_macro_arr_idx(struct d_test_counter* _test_info);
bool d_tests_sa_macro_function_consistency(struct d_test_counter* _test_info);

// Comprehensive index macro tests
bool d_tests_sa_index_macros_all(struct d_test_counter* _test_info);


/******************************************************************************
 * MODULE 3: ARRAY UTILITY MACRO TESTS (djinterp_tests_sa_array.c)
 *****************************************************************************/

// Array macro tests
bool d_tests_sa_macro_array_total_size(struct d_test_counter* _test_info);
bool d_tests_sa_macro_array_count(struct d_test_counter* _test_info);

// Comprehensive array macro tests
bool d_tests_sa_array_macros_all(struct d_test_counter* _test_info);


/******************************************************************************
 * MODULE 4: BOOLEAN CONSTANT/MACRO TESTS (djinterp_tests_sa_boolean.c)
 *****************************************************************************/

// Boolean constant tests
bool d_tests_sa_constants_success_failure(struct d_test_counter* _test_info);
bool d_tests_sa_constants_enabled_disabled(struct d_test_counter* _test_info);

// Boolean macro tests
bool d_tests_sa_macro_is_enabled(struct d_test_counter* _test_info);
bool d_tests_sa_macro_is_disabled(struct d_test_counter* _test_info);

// Comprehensive boolean tests
bool d_tests_sa_boolean_all(struct d_test_counter* _test_info);


/******************************************************************************
 * MODULE 5: FUNCTION POINTER TESTS (djinterp_tests_sa_function_ptr.c)
 *****************************************************************************/

// Function pointer type tests
bool d_tests_sa_fn_apply(struct d_test_counter* _test_info);
bool d_tests_sa_fn_apply_ctx(struct d_test_counter* _test_info);
bool d_tests_sa_fn_callback(struct d_test_counter* _test_info);
bool d_tests_sa_fn_comparator(struct d_test_counter* _test_info);
bool d_tests_sa_fn_free(struct d_test_counter* _test_info);
bool d_tests_sa_fn_print(struct d_test_counter* _test_info);
bool d_tests_sa_fn_to_string(struct d_test_counter* _test_info);
bool d_tests_sa_fn_write(struct d_test_counter* _test_info);

// Comprehensive function pointer tests
bool d_tests_sa_function_pointers_all(struct d_test_counter* _test_info);


/******************************************************************************
 * MODULE 6: EDGE CASE TESTS (djinterp_tests_sa_edge.c)
 *****************************************************************************/

// Boundary condition tests
bool d_tests_sa_edge_size_max(struct d_test_counter* _test_info);
bool d_tests_sa_edge_ssize_overflow(struct d_test_counter* _test_info);

// Special size tests
bool d_tests_sa_edge_zero_arrays(struct d_test_counter* _test_info);
bool d_tests_sa_edge_single_element(struct d_test_counter* _test_info);
bool d_tests_sa_edge_large_arrays(struct d_test_counter* _test_info);

// Comprehensive edge case tests
bool d_tests_sa_edge_cases_all(struct d_test_counter* _test_info);


/******************************************************************************
 * MASTER TEST RUNNERS (djinterp_tests_sa.c)
 *****************************************************************************/

// Module-level runners
bool d_tests_sa_run_index_functions(struct d_test_counter* _test_info);
bool d_tests_sa_run_index_macros(struct d_test_counter* _test_info);
bool d_tests_sa_run_array_macros(struct d_test_counter* _test_info);
bool d_tests_sa_run_boolean(struct d_test_counter* _test_info);
bool d_tests_sa_run_function_pointers(struct d_test_counter* _test_info);
bool d_tests_sa_run_edge_cases(struct d_test_counter* _test_info);

// Category-level runners
bool d_tests_sa_run_all_functions(struct d_test_counter* _test_info);
bool d_tests_sa_run_all_macros(struct d_test_counter* _test_info);
bool d_tests_sa_run_all_types(struct d_test_counter* _test_info);

// Master runner - runs EVERYTHING
bool d_tests_sa_run_all(struct d_test_counter* _test_info);

// Utility functions
void d_tests_sa_print_module_results(const char*                  _module_name,
                                     const struct d_test_counter* _counter,
                                     bool                         _passed);
void d_tests_sa_print_summary(const struct d_test_counter* _counter);


#endif  // DJINTERP_HEADER_TESTS_STANDALONE_





