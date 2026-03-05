/******************************************************************************
* djinterp [test]                                        test_block_tests_sa.h
*
*   Unit tests for test_block.h using the standalone testing framework.
*   Covers constructor/destructor, child management, run-config stage hooks,
* execution, and utility functions with full code coverage.
*
* path:      \test\test\test_block_tests_sa.h
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2026.03.01
******************************************************************************/

#ifndef DJINTERP_TESTING_SA_TEST_BLOCK_
#define DJINTERP_TESTING_SA_TEST_BLOCK_ 1

#include <stdlib.h>
#include <errno.h>
#include "../../../inc/c/djinterp.h"
#include "../../../inc/c/dmemory.h"
#include "../../../inc/c/string_fn.h"
#include "../../../inc/c/test/test.h"
#include "../../../inc/c/test/test_block.h"
#include "../../../inc/c/test/test_options.h"
#include "../../../inc/c/test/test_standalone.h"


/******************************************************************************
 * CONSTRUCTOR/DESTRUCTOR TESTS
 *****************************************************************************/

// d_test_block_new, d_test_block_new_args, d_test_block_validate_args,
// d_test_block_free
bool d_tests_sa_test_block_new_no_children(struct d_test_counter* _test_info);
bool d_tests_sa_test_block_new_with_children(struct d_test_counter* _test_info);
bool d_tests_sa_test_block_new_null_children_zero_count(struct d_test_counter* _test_info);
bool d_tests_sa_test_block_new_null_children_nonzero_count(struct d_test_counter* _test_info);
bool d_tests_sa_test_block_new_with_null_child_entry(struct d_test_counter* _test_info);
bool d_tests_sa_test_block_new_args_basic(struct d_test_counter* _test_info);
bool d_tests_sa_test_block_new_args_no_args(struct d_test_counter* _test_info);
bool d_tests_sa_test_block_new_args_null_args_nonzero(struct d_test_counter* _test_info);
bool d_tests_sa_test_block_new_args_with_children(struct d_test_counter* _test_info);
bool d_tests_sa_test_block_validate_args_null(struct d_test_counter* _test_info);
bool d_tests_sa_test_block_validate_args_zero_count(struct d_test_counter* _test_info);
bool d_tests_sa_test_block_validate_args_null_key_entry(struct d_test_counter* _test_info);
bool d_tests_sa_test_block_free_null(struct d_test_counter* _test_info);
bool d_tests_sa_test_block_free_valid(struct d_test_counter* _test_info);
bool d_tests_sa_test_block_constructor_all(struct d_test_counter* _test_info);


/******************************************************************************
 * CHILD MANAGEMENT TESTS
 *****************************************************************************/

// d_test_block_add_child, d_test_block_add_test, d_test_block_add_block,
// d_test_block_child_count, d_test_block_get_child_at
bool d_tests_sa_test_block_add_child_valid(struct d_test_counter* _test_info);
bool d_tests_sa_test_block_add_child_null_block(struct d_test_counter* _test_info);
bool d_tests_sa_test_block_add_child_null_child(struct d_test_counter* _test_info);
bool d_tests_sa_test_block_add_test_valid(struct d_test_counter* _test_info);
bool d_tests_sa_test_block_add_test_null_block(struct d_test_counter* _test_info);
bool d_tests_sa_test_block_add_test_null_test(struct d_test_counter* _test_info);
bool d_tests_sa_test_block_add_block_valid(struct d_test_counter* _test_info);
bool d_tests_sa_test_block_add_block_null_parent(struct d_test_counter* _test_info);
bool d_tests_sa_test_block_add_block_null_child(struct d_test_counter* _test_info);
bool d_tests_sa_test_block_child_count_valid(struct d_test_counter* _test_info);
bool d_tests_sa_test_block_child_count_null(struct d_test_counter* _test_info);
bool d_tests_sa_test_block_child_count_empty(struct d_test_counter* _test_info);
bool d_tests_sa_test_block_get_child_at_valid(struct d_test_counter* _test_info);
bool d_tests_sa_test_block_get_child_at_null_block(struct d_test_counter* _test_info);
bool d_tests_sa_test_block_get_child_at_out_of_bounds(struct d_test_counter* _test_info);
bool d_tests_sa_test_block_child_mgmt_all(struct d_test_counter* _test_info);


/******************************************************************************
 * RUN-CONFIG STAGE HOOK TESTS
 *****************************************************************************/

// hooks passed via d_test_options to d_test_block_run
bool d_tests_sa_test_block_run_config_setup_hook(struct d_test_counter* _test_info);
bool d_tests_sa_test_block_run_config_teardown_hook(struct d_test_counter* _test_info);
bool d_tests_sa_test_block_run_config_setup_and_teardown(struct d_test_counter* _test_info);
bool d_tests_sa_test_block_run_config_setup_fails(struct d_test_counter* _test_info);
bool d_tests_sa_test_block_run_config_null_hooks_map(struct d_test_counter* _test_info);
bool d_tests_sa_test_block_run_config_no_hook_set(struct d_test_counter* _test_info);
bool d_tests_sa_test_block_run_config_overwrite_hook(struct d_test_counter* _test_info);
bool d_tests_sa_test_block_run_config_null_config(struct d_test_counter* _test_info);
bool d_tests_sa_test_block_stage_hooks_all(struct d_test_counter* _test_info);


/******************************************************************************
 * EXECUTION TESTS
 *****************************************************************************/

// d_test_block_run
bool d_tests_sa_test_block_run_empty(struct d_test_counter* _test_info);
bool d_tests_sa_test_block_run_null(struct d_test_counter* _test_info);
bool d_tests_sa_test_block_run_with_passing_assertions(struct d_test_counter* _test_info);
bool d_tests_sa_test_block_run_with_failing_assertion(struct d_test_counter* _test_info);
bool d_tests_sa_test_block_run_with_test_fn(struct d_test_counter* _test_info);
bool d_tests_sa_test_block_run_with_nested_block(struct d_test_counter* _test_info);
bool d_tests_sa_test_block_run_mixed_children(struct d_test_counter* _test_info);
bool d_tests_sa_test_block_execution_all(struct d_test_counter* _test_info);


/******************************************************************************
 * UTILITY TESTS
 *****************************************************************************/

// d_test_block_print, d_test_block_count_tests, d_test_block_count_blocks
bool d_tests_sa_test_block_print_valid(struct d_test_counter* _test_info);
bool d_tests_sa_test_block_print_null(struct d_test_counter* _test_info);
bool d_tests_sa_test_block_print_null_prefix(struct d_test_counter* _test_info);
bool d_tests_sa_test_block_count_tests_empty(struct d_test_counter* _test_info);
bool d_tests_sa_test_block_count_tests_mixed(struct d_test_counter* _test_info);
bool d_tests_sa_test_block_count_tests_null(struct d_test_counter* _test_info);
bool d_tests_sa_test_block_count_blocks_empty(struct d_test_counter* _test_info);
bool d_tests_sa_test_block_count_blocks_mixed(struct d_test_counter* _test_info);
bool d_tests_sa_test_block_count_blocks_null(struct d_test_counter* _test_info);
bool d_tests_sa_test_block_utility_all(struct d_test_counter* _test_info);


/******************************************************************************
 * ROOT AGGREGATION
 *****************************************************************************/

bool d_tests_sa_test_block_all(struct d_test_counter* _test_info);


#endif  // DJINTERP_TESTING_SA_TEST_BLOCK_
