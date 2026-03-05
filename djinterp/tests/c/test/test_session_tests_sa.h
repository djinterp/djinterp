/******************************************************************************
* djinterp [test]                                      test_session_tests_sa.h
*
*   Unit tests for test_session.h using the standalone testing framework.
*   Covers constructor/destructor, configuration, child management, execution,
* output, statistics, status, and utility functions.
*
*
* path:      \test\test\test_session_tests_sa.h
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.12.16
******************************************************************************/

#ifndef DJINTERP_TESTING_SA_TEST_SESSION_
#define DJINTERP_TESTING_SA_TEST_SESSION_ 1

#include <stdlib.h>
#include "../../../inc/c/djinterp.h"
#include "../../../inc/c/dmemory.h"
#include "../../../inc/c/string_fn.h"
#include "../../../inc/c/test/test_session.h"
#include "../../../inc/c/test/test_module.h"
#include "../../../inc/c/test/test.h"
#include "../../../inc/c/test/test_common.h"
#include "../../../inc/c/test/test_standalone.h"


/******************************************************************************
 * CONSTRUCTOR / DESTRUCTOR TESTS
 *****************************************************************************/

bool d_tests_sa_test_session_new(struct d_test_counter* _test_info);
bool d_tests_sa_test_session_new_with_config(struct d_test_counter* _test_info);
bool d_tests_sa_test_session_new_with_modules(struct d_test_counter* _test_info);
bool d_tests_sa_test_session_new_with_modules_and_config(struct d_test_counter* _test_info);
bool d_tests_sa_test_session_validate_args(struct d_test_counter* _test_info);
bool d_tests_sa_test_session_free(struct d_test_counter* _test_info);
bool d_tests_sa_test_session_free_null(struct d_test_counter* _test_info);


/******************************************************************************
 * CONFIGURATION TESTS
 *****************************************************************************/

bool d_tests_sa_test_session_set_get_option(struct d_test_counter* _test_info);
bool d_tests_sa_test_session_option_null(struct d_test_counter* _test_info);
bool d_tests_sa_test_session_set_output_format(struct d_test_counter* _test_info);
bool d_tests_sa_test_session_set_verbosity(struct d_test_counter* _test_info);
bool d_tests_sa_test_session_enable_color(struct d_test_counter* _test_info);


/******************************************************************************
 * CHILD MANAGEMENT TESTS
 *****************************************************************************/

bool d_tests_sa_test_session_add_child(struct d_test_counter* _test_info);
bool d_tests_sa_test_session_add_child_non_module(struct d_test_counter* _test_info);
bool d_tests_sa_test_session_add_child_null(struct d_test_counter* _test_info);
bool d_tests_sa_test_session_add_children(struct d_test_counter* _test_info);
bool d_tests_sa_test_session_child_count(struct d_test_counter* _test_info);
bool d_tests_sa_test_session_get_child_at(struct d_test_counter* _test_info);
bool d_tests_sa_test_session_get_child_at_invalid(struct d_test_counter* _test_info);
bool d_tests_sa_test_session_clear_children(struct d_test_counter* _test_info);


/******************************************************************************
 * EXECUTION TESTS
 *****************************************************************************/

bool d_tests_sa_test_session_run_empty(struct d_test_counter* _test_info);
bool d_tests_sa_test_session_run_null(struct d_test_counter* _test_info);
bool d_tests_sa_test_session_run_child_invalid(struct d_test_counter* _test_info);
bool d_tests_sa_test_session_abort(struct d_test_counter* _test_info);
bool d_tests_sa_test_session_reset(struct d_test_counter* _test_info);
bool d_tests_sa_test_session_pause_resume(struct d_test_counter* _test_info);


/******************************************************************************
 * OUTPUT TESTS
 *****************************************************************************/

bool d_tests_sa_test_session_write_header(struct d_test_counter* _test_info);
bool d_tests_sa_test_session_write_footer(struct d_test_counter* _test_info);
bool d_tests_sa_test_session_write_summary(struct d_test_counter* _test_info);
bool d_tests_sa_test_session_flush(struct d_test_counter* _test_info);
bool d_tests_sa_test_session_output_null(struct d_test_counter* _test_info);


/******************************************************************************
 * STATISTICS TESTS
 *****************************************************************************/

bool d_tests_sa_test_session_get_stats(struct d_test_counter* _test_info);
bool d_tests_sa_test_session_all_passed(struct d_test_counter* _test_info);
bool d_tests_sa_test_session_total_counters(struct d_test_counter* _test_info);
bool d_tests_sa_test_session_duration_ms(struct d_test_counter* _test_info);
bool d_tests_sa_test_session_stats_null(struct d_test_counter* _test_info);


/******************************************************************************
 * STATUS TESTS
 *****************************************************************************/

bool d_tests_sa_test_session_get_status(struct d_test_counter* _test_info);
bool d_tests_sa_test_session_status_to_string(struct d_test_counter* _test_info);
bool d_tests_sa_test_session_status_to_string_all(struct d_test_counter* _test_info);
bool d_tests_sa_test_session_is_running(struct d_test_counter* _test_info);
bool d_tests_sa_test_session_is_complete(struct d_test_counter* _test_info);


/******************************************************************************
 * UTILITY TESTS
 *****************************************************************************/

bool d_tests_sa_test_session_print(struct d_test_counter* _test_info);
bool d_tests_sa_test_session_exit_code(struct d_test_counter* _test_info);


/******************************************************************************
 * ROOT TEST FUNCTION
 *****************************************************************************/

bool d_tests_sa_test_session_all(struct d_test_counter* _test_info);


#endif  // DJINTERP_TESTING_SA_TEST_SESSION_
