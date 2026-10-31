/*******************************************************************************
* djinterp [c/test]                                      test_session_tests_sa.h
*
*   Unit tests for test_session.h using the standalone testing framework.
*   Covers constructor/destructor, configuration, child management, execution,
* output, statistics, status, and utility functions.
*
*
* path:      /tests/djinterp/c/modules/test/test_session_tests_sa.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.12.16
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef DJINTERP_TESTING_TEST_SESSION_STANDALONE_
#define DJINTERP_TESTING_TEST_SESSION_STANDALONE_ 1

// std
#include <stdlib.h>
// djinterp
#include "../../../../../inc/djinterp/c/djinterp.h"
#include "../../../../../inc/djinterp/c/modules/test/test_session.h"
#include "../../../../../inc/djinterp/c/modules/test/test_counter.h"


/******************************************************************************
 * CONSTRUCTOR / DESTRUCTOR TESTS
 *****************************************************************************/

bool d_tests_sa_test_session_new(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_test_session_new_with_config(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_test_session_new_with_modules(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_test_session_new_with_modules_and_config(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_test_session_validate_args(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_test_session_free(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_test_session_free_null(
    struct dj_test_counter* _test_info
);


/******************************************************************************
 * CONFIGURATION TESTS
 *****************************************************************************/

bool d_tests_sa_test_session_set_get_option(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_test_session_option_null(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_test_session_set_output_format(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_test_session_set_verbosity(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_test_session_enable_color(
    struct dj_test_counter* _test_info
);


/******************************************************************************
 * CHILD MANAGEMENT TESTS
 *****************************************************************************/

bool d_tests_sa_test_session_add_child(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_test_session_add_child_non_module(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_test_session_add_child_null(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_test_session_add_children(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_test_session_child_count(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_test_session_get_child_at(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_test_session_get_child_at_invalid(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_test_session_clear_children(
    struct dj_test_counter* _test_info
);


/******************************************************************************
 * EXECUTION TESTS
 *****************************************************************************/

bool d_tests_sa_test_session_run_empty(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_test_session_run_null(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_test_session_run_child_invalid(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_test_session_abort(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_test_session_reset(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_test_session_pause_resume(
    struct dj_test_counter* _test_info
);


/******************************************************************************
 * OUTPUT TESTS
 *****************************************************************************/

bool d_tests_sa_test_session_write_header(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_test_session_write_footer(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_test_session_write_summary(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_test_session_flush(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_test_session_output_null(
    struct dj_test_counter* _test_info
);


/******************************************************************************
 * STATISTICS TESTS
 *****************************************************************************/

bool d_tests_sa_test_session_get_stats(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_test_session_all_passed(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_test_session_total_counters(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_test_session_duration_ms(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_test_session_stats_null(
    struct dj_test_counter* _test_info
);


/******************************************************************************
 * STATUS TESTS
 *****************************************************************************/

bool d_tests_sa_test_session_get_status(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_test_session_status_to_string(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_test_session_status_to_string_all(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_test_session_is_running(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_test_session_is_complete(
    struct dj_test_counter* _test_info
);


/******************************************************************************
 * UTILITY TESTS
 *****************************************************************************/

bool d_tests_sa_test_session_print(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_test_session_exit_code(
    struct dj_test_counter* _test_info
);


/******************************************************************************
 * ROOT TEST FUNCTION
 *****************************************************************************/

bool d_tests_sa_test_session_all(
    struct dj_test_counter* _test_info
);


#endif  // DJINTERP_TESTING_TEST_SESSION_STANDALONE_
