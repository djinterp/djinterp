/*******************************************************************************
* djinterp [test]                                            djinterp_tests_sa.h
*
* Unified test suite header for djinterp.h - all test declarations.
*
* This is the single header file that declares all test functions from all
* test modules. Each module has its own .c file:
*   - djinterp_tests_sa_index.c          (dj_index functions)
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
*   struct dj_test_counter counter = {0, 0, 0, 0};
*   bool result = d_tests_sa_run_all(&counter);
*
*
* path:      /tests/djinterp/c/djinterp_tests_sa.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.01.15
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef DJINTERP_HEADER_TESTS_STANDALONE_
#define DJINTERP_HEADER_TESTS_STANDALONE_ 1

// std
#include <limits.h>
#include <stdlib.h>
// djinterp
#include "../../../inc/djinterp/c/djinterp.h"
#include "../../../inc/djinterp/c/modules/test/test_counter.h"


/******************************************************************************
 * MODULE 1: dj_index FUNCTION TESTS (djinterp_tests_sa_index.c)
 *****************************************************************************/

// d_index_convert_fast() tests
bool d_tests_sa_index_convert_fast_basic(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_index_convert_fast_edge_cases(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_index_convert_fast_large_arrays(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_index_convert_fast_all(
    struct dj_test_counter* _test_info
);

// d_index_convert_safe() tests
bool d_tests_sa_index_convert_safe_valid(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_index_convert_safe_invalid(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_index_convert_safe_null_handling(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_index_convert_safe_all(
    struct dj_test_counter* _test_info
);

// d_index_is_valid() tests
bool d_tests_sa_index_is_valid_positive(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_index_is_valid_negative(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_index_is_valid_boundary(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_index_is_valid_all(
    struct dj_test_counter* _test_info
);

// Comprehensive dj_index function tests
bool d_tests_sa_index_functions_all(
    struct dj_test_counter* _test_info
);


/******************************************************************************
 * MODULE 2: INDEX MANIPULATION MACRO TESTS (djinterp_tests_sa_index_macros.c)
 *****************************************************************************/

// Individual macro tests
bool d_tests_sa_macro_clamp_index(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_macro_index_in_bounds(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_macro_safe_arr_idx(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_macro_is_valid_index(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_macro_is_valid_index_n(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_macro_neg_idx(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_macro_arr_idx(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_macro_function_consistency(
    struct dj_test_counter* _test_info
);

// Comprehensive index macro tests
bool d_tests_sa_index_macros_all(
    struct dj_test_counter* _test_info
);


/******************************************************************************
 * MODULE 3: ARRAY UTILITY MACRO TESTS (djinterp_tests_sa_array.c)
 *****************************************************************************/

// Array macro tests
bool d_tests_sa_macro_array_total_size(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_macro_array_count(
    struct dj_test_counter* _test_info
);

// Comprehensive array macro tests
bool d_tests_sa_array_macros_all(
    struct dj_test_counter* _test_info
);


/******************************************************************************
 * MODULE 4: BOOLEAN CONSTANT/MACRO TESTS (djinterp_tests_sa_boolean.c)
 *****************************************************************************/

// Boolean constant tests
bool d_tests_sa_constants_success_failure(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_constants_enabled_disabled(
    struct dj_test_counter* _test_info
);

// Boolean macro tests
bool d_tests_sa_macro_is_enabled(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_macro_is_disabled(
    struct dj_test_counter* _test_info
);

// Comprehensive boolean tests
bool d_tests_sa_boolean_all(
    struct dj_test_counter* _test_info
);


/******************************************************************************
 * MODULE 5: FUNCTION POINTER TESTS (djinterp_tests_sa_function_ptr.c)
 *****************************************************************************/

// Function pointer type tests
bool d_tests_sa_fn_apply(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_fn_apply_ctx(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_fn_callback(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_fn_comparator(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_fn_free(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_fn_print(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_fn_to_string(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_fn_write(
    struct dj_test_counter* _test_info
);

// Comprehensive function pointer tests
bool d_tests_sa_function_pointers_all(
    struct dj_test_counter* _test_info
);


/******************************************************************************
 * MODULE 6: EDGE CASE TESTS (djinterp_tests_sa_edge.c)
 *****************************************************************************/

// Boundary condition tests
bool d_tests_sa_edge_size_max(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_edge_ssize_overflow(
    struct dj_test_counter* _test_info
);

// Special size tests
bool d_tests_sa_edge_zero_arrays(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_edge_single_element(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_edge_large_arrays(
    struct dj_test_counter* _test_info
);

// Comprehensive edge case tests
bool d_tests_sa_edge_cases_all(
    struct dj_test_counter* _test_info
);


/******************************************************************************
 * MASTER TEST RUNNERS (djinterp_tests_sa.c)
 *****************************************************************************/

// Module-level runners
bool d_tests_sa_run_index_functions(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_run_index_macros(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_run_array_macros(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_run_boolean(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_run_function_pointers(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_run_edge_cases(
    struct dj_test_counter* _test_info
);

// Category-level runners
bool d_tests_sa_run_all_functions(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_run_all_macros(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_run_all_types(
    struct dj_test_counter* _test_info
);

// Master runner - runs EVERYTHING
bool d_tests_sa_run_all(
    struct dj_test_counter* _test_info
);

// Utility functions
void d_tests_sa_print_module_results(
    const char*                   _module_name,
    const struct dj_test_counter* _counter,
    bool                          _passed
);
void d_tests_sa_print_summary(
    const struct dj_test_counter* _counter
);


#endif  // DJINTERP_HEADER_TESTS_STANDALONE_
