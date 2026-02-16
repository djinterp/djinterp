/******************************************************************************
* djinterp [test]                                     test_standalone_tests_sa.h
*
*   Unit test declarations for `test_standalone.h` module.
*   Provides comprehensive testing of all test_standalone functions including
* assertion macros, test object constants, test counter operations, test object
* creation/destruction, template substitution, print functions, output
* functions, formatting functions, unified test runner, and utility functions.
*
*
* path:      \tests\test\test_standalone_tests_sa.h
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.02.03
******************************************************************************/

#ifndef DJINTERP_TESTS_TEST_STANDALONE_SA_
#define DJINTERP_TESTS_TEST_STANDALONE_SA_ 1

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include "..\..\..\inc\c\test\test_standalone.h"
#include "..\..\..\inc\c\string_fn.h"


/******************************************************************************
 * I. ASSERTION MACRO TESTS
 *****************************************************************************/
// D_ASSERT_TRUE macro
bool d_tests_sa_standalone_assert_true(struct d_test_counter* _counter);
// D_ASSERT_FALSE macro
bool d_tests_sa_standalone_assert_false(struct d_test_counter* _counter);
// D_ASSERT_NULL macro
bool d_tests_sa_standalone_assert_null(struct d_test_counter* _counter);
// D_ASSERT_NOT_NULL macro
bool d_tests_sa_standalone_assert_not_null(struct d_test_counter* _counter);
// D_ASSERT_EQUAL macro
bool d_tests_sa_standalone_assert_equal(struct d_test_counter* _counter);
// D_ASSERT_STR_EQUAL macro
bool d_tests_sa_standalone_assert_str_equal(struct d_test_counter* _counter);

// I.   aggregation function
bool d_tests_sa_standalone_assertion_macro_all(struct d_test_counter* _counter);


/******************************************************************************
 * II. CONSTANT MACRO TESTS
 *****************************************************************************/
// D_TEST_OBJECT_LEAF and D_TEST_OBJECT_INTERIOR
bool d_tests_sa_standalone_object_constants(struct d_test_counter* _counter);
// D_TEST_SA_LINE_WIDTH
bool d_tests_sa_standalone_line_width(struct d_test_counter* _counter);
// D_TEST_SA_SEPARATOR_DOUBLE and D_TEST_SA_SEPARATOR_SINGLE
bool d_tests_sa_standalone_separators(struct d_test_counter* _counter);
// D_TEST_SA_MAX_MODULES
bool d_tests_sa_standalone_max_modules(struct d_test_counter* _counter);

// II.  aggregation function
bool d_tests_sa_standalone_constant_all(struct d_test_counter* _counter);


/******************************************************************************
 * III. TEST COUNTER TESTS
 *****************************************************************************/
// d_test_counter structure
bool d_tests_sa_standalone_counter_struct(struct d_test_counter* _counter);
// d_test_counter_reset function
bool d_tests_sa_standalone_counter_reset(struct d_test_counter* _counter);
// d_test_counter_add function
bool d_tests_sa_standalone_counter_add(struct d_test_counter* _counter);

// III. aggregation function
bool d_tests_sa_standalone_counter_all(struct d_test_counter* _counter);


/******************************************************************************
 * IV. TEST OBJECT TESTS
 *****************************************************************************/
// d_test_object structure
bool d_tests_sa_standalone_object_struct(struct d_test_counter* _counter);
// d_test_object_new_leaf function
bool d_tests_sa_standalone_object_new_leaf(struct d_test_counter* _counter);
// d_test_object_new_interior function
bool d_tests_sa_standalone_object_new_interior(struct d_test_counter* _counter);
// d_test_object_add_child function
bool d_tests_sa_standalone_object_add_child(struct d_test_counter* _counter);
// d_test_object_free function
bool d_tests_sa_standalone_object_free(struct d_test_counter* _counter);

// IV.  aggregation function
bool d_tests_sa_standalone_object_all(struct d_test_counter* _counter);


/******************************************************************************
 * V. RESULTS STRUCTURE TESTS
 *****************************************************************************/
// d_test_sa_module_results structure
bool d_tests_sa_standalone_module_results_struct(struct d_test_counter* _counter);
// d_test_sa_suite_results structure
bool d_tests_sa_standalone_suite_results_struct(struct d_test_counter* _counter);

// V.   aggregation function
bool d_tests_sa_standalone_results_all(struct d_test_counter* _counter);


/******************************************************************************
 * VI. NOTE STRUCTURE TESTS
 *****************************************************************************/
// d_test_sa_note_item structure
bool d_tests_sa_standalone_note_item_struct(struct d_test_counter* _counter);
// d_test_sa_note_section structure
bool d_tests_sa_standalone_note_section_struct(struct d_test_counter* _counter);

// VI.  aggregation function
bool d_tests_sa_standalone_note_all(struct d_test_counter* _counter);


/******************************************************************************
 * VII. MODULE ENTRY AND RUNNER STRUCTURE TESTS
 *****************************************************************************/
// fn_test_module function pointer type
bool d_tests_sa_standalone_fn_test_module(struct d_test_counter* _counter);
// fn_test_module_counter function pointer type
bool d_tests_sa_standalone_fn_test_module_counter(struct d_test_counter* _counter);
// d_test_sa_module_entry structure
bool d_tests_sa_standalone_module_entry_struct(struct d_test_counter* _counter);
// d_test_sa_runner structure
bool d_tests_sa_standalone_runner_struct(struct d_test_counter* _counter);

// VII. aggregation function
bool d_tests_sa_standalone_runner_struct_all(struct d_test_counter* _counter);


/******************************************************************************
 * VIII. FUNCTION POINTER TYPE TESTS
 *****************************************************************************/
// fn_print_object function pointer type
bool d_tests_sa_standalone_fn_print_object(struct d_test_counter* _counter);
// fn_print_object_file function pointer type
bool d_tests_sa_standalone_fn_print_object_file(struct d_test_counter* _counter);

// VIII. aggregation function
bool d_tests_sa_standalone_fn_ptr_all(struct d_test_counter* _counter);


/******************************************************************************
 * IX. ASSERTION FUNCTION TESTS
 *****************************************************************************/
// d_assert_standalone function
bool d_tests_sa_standalone_assert_fn(struct d_test_counter* _counter);

// IX.  aggregation function
bool d_tests_sa_standalone_assert_fn_all(struct d_test_counter* _counter);


/******************************************************************************
 * X. TEMPLATE SUBSTITUTION TESTS
 *****************************************************************************/
// d_test_substitute_template function
bool d_tests_sa_standalone_substitute_template(struct d_test_counter* _counter);

// X.   aggregation function
bool d_tests_sa_standalone_template_all(struct d_test_counter* _counter);


/******************************************************************************
 * XI. RUNNER FUNCTION TESTS
 *****************************************************************************/
// d_test_sa_runner_init function
bool d_tests_sa_standalone_runner_init(struct d_test_counter* _counter);
// d_test_sa_runner_add_module function
bool d_tests_sa_standalone_runner_add_module(struct d_test_counter* _counter);
// d_test_sa_runner_add_module_counter function
bool d_tests_sa_standalone_runner_add_module_counter(struct d_test_counter* _counter);
// d_test_sa_runner_set_wait_for_input function
bool d_tests_sa_standalone_runner_set_wait(struct d_test_counter* _counter);
// d_test_sa_runner_set_show_notes function
bool d_tests_sa_standalone_runner_set_notes(struct d_test_counter* _counter);
// d_test_sa_runner_cleanup function
bool d_tests_sa_standalone_runner_cleanup(struct d_test_counter* _counter);

// XI.  aggregation function
bool d_tests_sa_standalone_runner_fn_all(struct d_test_counter* _counter);


/******************************************************************************
 * XII. UTILITY FUNCTION TESTS
 *****************************************************************************/
// d_test_sa_get_elapsed_time function
bool d_tests_sa_standalone_get_elapsed_time(struct d_test_counter* _counter);

// XII. aggregation function
bool d_tests_sa_standalone_utility_all(struct d_test_counter* _counter);


/******************************************************************************
 * MODULE-LEVEL AGGREGATION
 *****************************************************************************/
bool d_tests_sa_standalone_run_all(struct d_test_counter* _counter);


#endif  // DJINTERP_TESTS_TEST_STANDALONE_SA_





