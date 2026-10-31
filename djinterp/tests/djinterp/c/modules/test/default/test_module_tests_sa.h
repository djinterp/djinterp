/*******************************************************************************
* djinterp [c/test]                                       test_module_tests_sa.h
*
*   Unit tests for test_module.h using the standalone testing framework.
*   Covers constructor/destructor, child management, configuration, stage
* hooks, execution, result tracking, and output functions.
*
*
* path:      /tests/djinterp/c/modules/test/default/test_module_tests_sa.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.12.16
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef DJINTERP_TESTING_SA_TEST_MODULE_
#define DJINTERP_TESTING_SA_TEST_MODULE_ 1

// std
#include <stdlib.h>
// djinterp
#include "../../../../../../inc/djinterp/c/djinterp.h"
#include "../../../../../inc/djinterp/c/dmemory.h"
#include "../../../../../../inc/djinterp/c/string_fn.h"
#include "../../../../../../inc/djinterp/c/modules/test/default/test_module.h"
#include "../../../../../../inc/djinterp/c/modules/test/default/test_block.h"
#include "../../../../../../inc/djinterp/c/modules/test/default/test.h"
#include "../../../../../../inc/djinterp/c/modules/test/test_common.h"
#include "../../../../../../inc/djinterp/c/modules/test/test_counter.h"


/******************************************************************************
 * CONSTRUCTOR / DESTRUCTOR TESTS
 *****************************************************************************/

bool d_tests_sa_test_module_new(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_test_module_new_with_children(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_test_module_new_args(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_test_module_validate_args(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_test_module_free(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_test_module_free_null(
    struct dj_test_counter* _test_info
);


/******************************************************************************
 * CHILD MANAGEMENT TESTS
 *****************************************************************************/

bool d_tests_sa_test_module_add_child(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_test_module_add_child_null(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_test_module_child_count(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_test_module_child_count_empty(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_test_module_get_child_at(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_test_module_get_child_at_invalid(
    struct dj_test_counter* _test_info
);


/******************************************************************************
 * CONFIGURATION TESTS
 *****************************************************************************/

bool d_tests_sa_test_module_get_effective_settings(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_test_module_get_effective_settings_null_parent(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_test_module_get_name(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_test_module_get_name_null(
    struct dj_test_counter* _test_info
);


/******************************************************************************
 * STAGE HOOK TESTS
 *****************************************************************************/

bool d_tests_sa_test_module_set_stage_hook(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_test_module_get_stage_hook(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_test_module_get_stage_hook_unset(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_test_module_set_stage_hook_null(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_test_module_stage_hooks_all_stages(
    struct dj_test_counter* _test_info
);


/******************************************************************************
 * EXECUTION TESTS
 *****************************************************************************/

bool d_tests_sa_test_module_run(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_test_module_run_empty(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_test_module_run_with_failing_child(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_test_module_run_child(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_test_module_run_child_invalid_index(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_test_module_run_null(
    struct dj_test_counter* _test_info
);


/******************************************************************************
 * RESULT TESTS
 *****************************************************************************/

bool d_tests_sa_test_module_get_result(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_test_module_get_result_null(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_test_module_reset_result(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_test_module_get_pass_rate(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_test_module_get_pass_rate_empty(
    struct dj_test_counter* _test_info
);


/******************************************************************************
 * OUTPUT TESTS
 *****************************************************************************/

bool d_tests_sa_test_module_print(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_test_module_print_null(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_test_module_print_result(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_test_module_status_to_string(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_test_module_status_to_string_all(
    struct dj_test_counter* _test_info
);


/******************************************************************************
 * ROOT TEST FUNCTION
 *****************************************************************************/

bool d_tests_sa_test_module_all(
    struct dj_test_counter* _test_info
);


#endif  // DJINTERP_TESTING_SA_TEST_MODULE_
