/******************************************************************************
* djinterp [test]                                       test_module_tests_sa.h
*
*   Unit tests for test_module.h using the standalone testing framework.
*   Covers constructor/destructor, child management, configuration, stage
* hooks, execution, result tracking, and output functions.
*
*
* path:      \test\test\test_module_tests_sa.h
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.12.16
******************************************************************************/

#ifndef DJINTERP_TESTING_SA_TEST_MODULE_
#define DJINTERP_TESTING_SA_TEST_MODULE_ 1

#include <stdlib.h>
#include "../../../inc/c/djinterp.h"
#include "../../../inc/c/dmemory.h"
#include "../../../inc/c/string_fn.h"
#include "../../../inc/c/test/test_module.h"
#include "../../../inc/c/test/test_block.h"
#include "../../../inc/c/test/test.h"
#include "../../../inc/c/test/test_common.h"
#include "../../../inc/c/test/test_standalone.h"


/******************************************************************************
 * CONSTRUCTOR / DESTRUCTOR TESTS
 *****************************************************************************/

bool d_tests_sa_test_module_new(struct d_test_counter* _test_info);
bool d_tests_sa_test_module_new_with_children(struct d_test_counter* _test_info);
bool d_tests_sa_test_module_new_args(struct d_test_counter* _test_info);
bool d_tests_sa_test_module_validate_args(struct d_test_counter* _test_info);
bool d_tests_sa_test_module_free(struct d_test_counter* _test_info);
bool d_tests_sa_test_module_free_null(struct d_test_counter* _test_info);


/******************************************************************************
 * CHILD MANAGEMENT TESTS
 *****************************************************************************/

bool d_tests_sa_test_module_add_child(struct d_test_counter* _test_info);
bool d_tests_sa_test_module_add_child_null(struct d_test_counter* _test_info);
bool d_tests_sa_test_module_child_count(struct d_test_counter* _test_info);
bool d_tests_sa_test_module_child_count_empty(struct d_test_counter* _test_info);
bool d_tests_sa_test_module_get_child_at(struct d_test_counter* _test_info);
bool d_tests_sa_test_module_get_child_at_invalid(struct d_test_counter* _test_info);


/******************************************************************************
 * CONFIGURATION TESTS
 *****************************************************************************/

bool d_tests_sa_test_module_get_effective_settings(struct d_test_counter* _test_info);
bool d_tests_sa_test_module_get_effective_settings_null_parent(struct d_test_counter* _test_info);
bool d_tests_sa_test_module_get_name(struct d_test_counter* _test_info);
bool d_tests_sa_test_module_get_name_null(struct d_test_counter* _test_info);


/******************************************************************************
 * STAGE HOOK TESTS
 *****************************************************************************/

bool d_tests_sa_test_module_set_stage_hook(struct d_test_counter* _test_info);
bool d_tests_sa_test_module_get_stage_hook(struct d_test_counter* _test_info);
bool d_tests_sa_test_module_get_stage_hook_unset(struct d_test_counter* _test_info);
bool d_tests_sa_test_module_set_stage_hook_null(struct d_test_counter* _test_info);
bool d_tests_sa_test_module_stage_hooks_all_stages(struct d_test_counter* _test_info);


/******************************************************************************
 * EXECUTION TESTS
 *****************************************************************************/

bool d_tests_sa_test_module_run(struct d_test_counter* _test_info);
bool d_tests_sa_test_module_run_empty(struct d_test_counter* _test_info);
bool d_tests_sa_test_module_run_with_failing_child(struct d_test_counter* _test_info);
bool d_tests_sa_test_module_run_child(struct d_test_counter* _test_info);
bool d_tests_sa_test_module_run_child_invalid_index(struct d_test_counter* _test_info);
bool d_tests_sa_test_module_run_null(struct d_test_counter* _test_info);


/******************************************************************************
 * RESULT TESTS
 *****************************************************************************/

bool d_tests_sa_test_module_get_result(struct d_test_counter* _test_info);
bool d_tests_sa_test_module_get_result_null(struct d_test_counter* _test_info);
bool d_tests_sa_test_module_reset_result(struct d_test_counter* _test_info);
bool d_tests_sa_test_module_get_pass_rate(struct d_test_counter* _test_info);
bool d_tests_sa_test_module_get_pass_rate_empty(struct d_test_counter* _test_info);


/******************************************************************************
 * OUTPUT TESTS
 *****************************************************************************/

bool d_tests_sa_test_module_print(struct d_test_counter* _test_info);
bool d_tests_sa_test_module_print_null(struct d_test_counter* _test_info);
bool d_tests_sa_test_module_print_result(struct d_test_counter* _test_info);
bool d_tests_sa_test_module_status_to_string(struct d_test_counter* _test_info);
bool d_tests_sa_test_module_status_to_string_all(struct d_test_counter* _test_info);


/******************************************************************************
 * ROOT TEST FUNCTION
 *****************************************************************************/

bool d_tests_sa_test_module_all(struct d_test_counter* _test_info);


#endif  // DJINTERP_TESTING_SA_TEST_MODULE_
