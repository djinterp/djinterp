/******************************************************************************
* djinterp [c/test]                                        test_block_tests_sa.h
*
*   Unit tests for test_block.h using the standalone testing framework.
*   Covers constructor/destructor, child management, run-config stage hooks,
* execution, and utility functions with full code coverage.
*
* path:      /tests/djinterp/c/modules/test/test_block_tests_sa.h
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2026.03.01
******************************************************************************/

#ifndef DJINTERP_TESTING_SA_TEST_BLOCK_
#define DJINTERP_TESTING_SA_TEST_BLOCK_ 1

// std
#include <errno.h>
#include <stdlib.h>
// djinterp
#include "../../../../../inc/djinterp/c/djinterp.h"
#include "../../../../inc/djinterp/c/dmemory.h"
#include "../../../../../inc/djinterp/c/string_fn.h"
#include "../../../../inc/djinterp/test/c/test.h"
#include "../../../../inc/djinterp/test/c/test_block.h"
#include "../../../../../inc/djinterp/c/modules/test/test_options.h"
#include "../../../../../inc/djinterp/c/modules/test/test_standalone.h"


/******************************************************************************
 * CONSTRUCTOR/DESTRUCTOR TESTS
 *****************************************************************************/

// d_test_block_new, d_test_block_new_args, d_test_block_validate_args,
// d_test_block_free
bool d_tests_sa_test_block_new_no_children(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_test_block_new_with_children(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_test_block_new_null_children_zero_count(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_test_block_new_null_children_nonzero_count(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_test_block_new_with_null_child_entry(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_test_block_new_args_basic(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_test_block_new_args_no_args(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_test_block_new_args_null_args_nonzero(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_test_block_new_args_with_children(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_test_block_validate_args_null(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_test_block_validate_args_zero_count(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_test_block_validate_args_null_key_entry(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_test_block_free_null(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_test_block_free_valid(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_test_block_constructor_all(
    struct dj_test_counter* _test_info
);


/******************************************************************************
 * CHILD MANAGEMENT TESTS
 *****************************************************************************/

// d_test_block_add_child, d_test_block_add_test, d_test_block_add_block,
// d_test_block_child_count, d_test_block_get_child_at
bool d_tests_sa_test_block_add_child_valid(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_test_block_add_child_null_block(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_test_block_add_child_null_child(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_test_block_add_test_valid(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_test_block_add_test_null_block(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_test_block_add_test_null_test(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_test_block_add_block_valid(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_test_block_add_block_null_parent(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_test_block_add_block_null_child(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_test_block_child_count_valid(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_test_block_child_count_null(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_test_block_child_count_empty(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_test_block_get_child_at_valid(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_test_block_get_child_at_null_block(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_test_block_get_child_at_out_of_bounds(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_test_block_child_mgmt_all(
    struct dj_test_counter* _test_info
);


/******************************************************************************
 * RUN-CONFIG STAGE HOOK TESTS
 *****************************************************************************/

// hooks passed via dj_test_options to d_test_block_run
bool d_tests_sa_test_block_run_config_setup_hook(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_test_block_run_config_teardown_hook(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_test_block_run_config_setup_and_teardown(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_test_block_run_config_setup_fails(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_test_block_run_config_null_hooks_map(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_test_block_run_config_no_hook_set(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_test_block_run_config_overwrite_hook(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_test_block_run_config_null_config(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_test_block_stage_hooks_all(
    struct dj_test_counter* _test_info
);


/******************************************************************************
 * EXECUTION TESTS
 *****************************************************************************/

// d_test_block_run
bool d_tests_sa_test_block_run_empty(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_test_block_run_null(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_test_block_run_with_passing_assertions(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_test_block_run_with_failing_assertion(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_test_block_run_with_test_fn(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_test_block_run_with_nested_block(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_test_block_run_mixed_children(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_test_block_execution_all(
    struct dj_test_counter* _test_info
);


/******************************************************************************
 * UTILITY TESTS
 *****************************************************************************/

// d_test_block_print, d_test_block_count_tests, d_test_block_count_blocks
bool d_tests_sa_test_block_print_valid(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_test_block_print_null(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_test_block_print_null_prefix(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_test_block_count_tests_empty(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_test_block_count_tests_mixed(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_test_block_count_tests_null(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_test_block_count_blocks_empty(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_test_block_count_blocks_mixed(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_test_block_count_blocks_null(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_test_block_utility_all(
    struct dj_test_counter* _test_info
);


/******************************************************************************
 * ROOT AGGREGATION
 *****************************************************************************/

bool d_tests_sa_test_block_all(
    struct dj_test_counter* _test_info
);


#endif  // DJINTERP_TESTING_SA_TEST_BLOCK_
