/******************************************************************************
* djinterp [test]                                             test_tests_sa.h
*
*   Unit test declarations for `test.h` module.
*   Provides comprehensive testing of all test.h functions including
* construction/destruction, test function wrappers, child management,
* stage hooks, test execution, assertion wrapper macros, auto-message
* assertion macros, validate_args, and utility functions.
*
*
* path:      \tests\test\test_tests_sa.h
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.02.19
******************************************************************************/

#ifndef DJINTERP_TESTS_TEST_TEST_SA_
#define DJINTERP_TESTS_TEST_TEST_SA_ 1

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include "../../../inc/c/test/test_standalone.h"
#include "../../../inc/c/test/test.h"
#include "../../../inc/c/string_fn.h"


/******************************************************************************
 * I. CONSTRUCTION TESTS
 *****************************************************************************/
// d_test_new with NULL children and 0 count
bool d_tests_sa_test_new_empty(struct d_test_counter* _counter);
// d_test_new returns non-NULL with valid allocation
bool d_tests_sa_test_new_valid(struct d_test_counter* _counter);
// d_test_new_args with arguments and children
bool d_tests_sa_test_new_args(struct d_test_counter* _counter);
// d_test_new_args with NULL args
bool d_tests_sa_test_new_args_null(struct d_test_counter* _counter);

// I.   aggregation function
bool d_tests_sa_test_construction_all(struct d_test_counter* _counter);


/******************************************************************************
 * II. VALIDATE_ARGS TESTS
 *****************************************************************************/
// d_test_validate_args with NULL args
bool d_tests_sa_test_validate_args_null(struct d_test_counter* _counter);
// d_test_validate_args with zero count
bool d_tests_sa_test_validate_args_zero(struct d_test_counter* _counter);
// d_test_fn_validate_args with NULL args
bool d_tests_sa_test_fn_validate_args_null(struct d_test_counter* _counter);
// d_test_fn_validate_args with zero count
bool d_tests_sa_test_fn_validate_args_zero(struct d_test_counter* _counter);

// II.  aggregation function
bool d_tests_sa_test_validate_args_all(struct d_test_counter* _counter);


/******************************************************************************
 * III. TEST FUNCTION WRAPPER TESTS
 *****************************************************************************/
// d_test_fn_new with valid function pointer
bool d_tests_sa_test_fn_new_valid(struct d_test_counter* _counter);
// d_test_fn_new initializes fields correctly
bool d_tests_sa_test_fn_new_fields(struct d_test_counter* _counter);
// d_test_fn_new with NULL function pointer
bool d_tests_sa_test_fn_new_null(struct d_test_counter* _counter);

// III. aggregation function
bool d_tests_sa_test_fn_wrapper_all(struct d_test_counter* _counter);


/******************************************************************************
 * IV. CHILD MANAGEMENT TESTS
 *****************************************************************************/
// d_test_add_child with NULL test
bool d_tests_sa_test_add_child_null_test(struct d_test_counter* _counter);
// d_test_add_child with NULL child
bool d_tests_sa_test_add_child_null_child(struct d_test_counter* _counter);
// d_test_add_child with valid assertion type
bool d_tests_sa_test_add_child_assert(struct d_test_counter* _counter);
// d_test_add_child rejects invalid child types
bool d_tests_sa_test_add_child_invalid_type(struct d_test_counter* _counter);
// d_test_add_function with valid function
bool d_tests_sa_test_add_function_valid(struct d_test_counter* _counter);
// d_test_add_function with NULL arguments
bool d_tests_sa_test_add_function_null(struct d_test_counter* _counter);
// d_test_child_count with NULL test
bool d_tests_sa_test_child_count_null(struct d_test_counter* _counter);
// d_test_child_count after adding children
bool d_tests_sa_test_child_count_after_add(struct d_test_counter* _counter);
// d_test_get_child_at with valid index
bool d_tests_sa_test_get_child_at_valid(struct d_test_counter* _counter);
// d_test_get_child_at with NULL test
bool d_tests_sa_test_get_child_at_null(struct d_test_counter* _counter);

// IV.  aggregation function
bool d_tests_sa_test_child_mgmt_all(struct d_test_counter* _counter);


/******************************************************************************
 * V. STAGE HOOK TESTS
 *****************************************************************************/
// d_test_set_stage_hook with NULL test
bool d_tests_sa_test_set_hook_null_test(struct d_test_counter* _counter);
// d_test_set_stage_hook with valid hook
bool d_tests_sa_test_set_hook_valid(struct d_test_counter* _counter);
// d_test_get_stage_hook with NULL test
bool d_tests_sa_test_get_hook_null_test(struct d_test_counter* _counter);
// d_test_get_stage_hook retrieves set hook
bool d_tests_sa_test_get_hook_valid(struct d_test_counter* _counter);
// d_test_get_stage_hook for unset stage returns NULL
bool d_tests_sa_test_get_hook_unset(struct d_test_counter* _counter);
// d_test_set_stage_hook for all stages
bool d_tests_sa_test_set_hook_all_stages(struct d_test_counter* _counter);

// V.   aggregation function
bool d_tests_sa_test_stage_hooks_all(struct d_test_counter* _counter);


/******************************************************************************
 * VI. TEST EXECUTION TESTS
 *****************************************************************************/
// d_test_run with NULL test
bool d_tests_sa_test_run_null(struct d_test_counter* _counter);
// d_test_run with empty test (no children)
bool d_tests_sa_test_run_empty(struct d_test_counter* _counter);
// d_test_run with passing test function
bool d_tests_sa_test_run_pass_fn(struct d_test_counter* _counter);
// d_test_run with failing test function
bool d_tests_sa_test_run_fail_fn(struct d_test_counter* _counter);
// d_test_run with mixed pass/fail children
bool d_tests_sa_test_run_mixed(struct d_test_counter* _counter);
// d_test_run with setup hook
bool d_tests_sa_test_run_setup_hook(struct d_test_counter* _counter);
// d_test_run with teardown hook
bool d_tests_sa_test_run_teardown_hook(struct d_test_counter* _counter);
// d_test_run with failing setup hook
bool d_tests_sa_test_run_setup_fail(struct d_test_counter* _counter);

// VI.  aggregation function
bool d_tests_sa_test_execution_all(struct d_test_counter* _counter);


/******************************************************************************
 * VII. UTILITY TESTS
 *****************************************************************************/
// d_test_type_flag_to_string for all enum values
bool d_tests_sa_test_type_flag_to_string(struct d_test_counter* _counter);
// d_test_type_flag_to_string for unknown value
bool d_tests_sa_test_type_flag_unknown(struct d_test_counter* _counter);
// d_test_print with NULL test
bool d_tests_sa_test_print_null(struct d_test_counter* _counter);
// d_test_print with valid test
bool d_tests_sa_test_print_valid(struct d_test_counter* _counter);

// VII. aggregation function
bool d_tests_sa_test_utility_all(struct d_test_counter* _counter);


/******************************************************************************
 * VIII. DESTRUCTION TESTS
 *****************************************************************************/
// d_test_fn_free with NULL
bool d_tests_sa_test_fn_free_null(struct d_test_counter* _counter);
// d_test_fn_free with valid test fn
bool d_tests_sa_test_fn_free_valid(struct d_test_counter* _counter);
// d_test_free with NULL
bool d_tests_sa_test_free_null(struct d_test_counter* _counter);
// d_test_free with valid test
bool d_tests_sa_test_free_valid(struct d_test_counter* _counter);
// d_test_free with test containing children
bool d_tests_sa_test_free_with_children(struct d_test_counter* _counter);

// VIII. aggregation function
bool d_tests_sa_test_destruction_all(struct d_test_counter* _counter);


/******************************************************************************
 * IX. DTestTypeFlag ENUM TESTS
 *****************************************************************************/
// DTestTypeFlag enum values are unique
bool d_tests_sa_test_type_flag_enum_unique(struct d_test_counter* _counter);
// DTestTypeFlag enum values are sequential
bool d_tests_sa_test_type_flag_enum_sequential(struct d_test_counter* _counter);

// IX.  aggregation function
bool d_tests_sa_test_enum_all(struct d_test_counter* _counter);


/******************************************************************************
 * X. DTestStage ENUM TESTS
 *****************************************************************************/
// DTestStage enum values are unique and cover all stages
bool d_tests_sa_test_stage_enum_values(struct d_test_counter* _counter);
// DTestStage enum covers setup through after
bool d_tests_sa_test_stage_enum_coverage(struct d_test_counter* _counter);

// X.   aggregation function
bool d_tests_sa_test_stage_enum_all(struct d_test_counter* _counter);


/******************************************************************************
 * MODULE-LEVEL AGGREGATION
 *****************************************************************************/
bool d_tests_sa_test_run_all(struct d_test_counter* _counter);


#endif  // DJINTERP_TESTS_TEST_TEST_SA_
