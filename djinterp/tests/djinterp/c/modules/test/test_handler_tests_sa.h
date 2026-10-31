/*******************************************************************************
* djinterp [c/test]                                      test_handler_tests_sa.h
*
*   Unit test declarations for the `test_handler` module.
*   Organized by sections matching the implementation in `test_handler.c`.
*   This module is a dependency of DTest, so it does not use DTest
* rather than DTest for unit testing.
*
*
* link(s):   TBA
*
*
* path:      /tests/djinterp/c/modules/test/test_handler_tests_sa.h
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.10.05
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef DJINTERP_TESTING_TEST_HANDLER_SA_
#define DJINTERP_TESTING_TEST_HANDLER_SA_ 1

// std
#include <errno.h>
#include <stdlib.h>
// djinterp
#include "../../../../../inc/djinterp/c/djinterp.h"
#include "../../../../inc/djinterp/c/dmemory.h"
#include "../../../../../inc/djinterp/c/string_fn.h"
#include "../../../../../inc/djinterp/c/event/event.h"
#include "../../../../../inc/djinterp/c/modules/test/default/assert.h"
#include "../../../../../inc/djinterp/c/modules/test/default/test.h"
#include "../../../../../inc/djinterp/c/modules/test/test_handler.h"
// helpers
#include "test_handler_test_helpers.h"
#include "../../../../../inc/djinterp/c/modules/test/test_counter.h"


// I.    helper test functions
bool handler_test_passing(
    struct dj_test* _test
);
bool handler_test_failing(
    struct dj_test* _test
);
bool handler_test_with_assertion(
    struct dj_test* _test
);

// II.   global event callback trackers
extern int g_event_setup_count;
extern int g_event_start_count;
extern int g_event_success_count;
extern int g_event_failure_count;
extern int g_event_end_count;
extern int g_event_teardown_count;

void reset_event_counters(void);

// III.  event callback functions
void callback_setup(
    size_t _size,
    void** _elements
);
void callback_start(
    size_t _size,
    void** _elements
);
void callback_success(
    size_t _size,
    void** _elements
);
void callback_failure(
    size_t _size,
    void** _elements
);
void callback_end(
    size_t _size,
    void** _elements
);
void callback_teardown(
    size_t _size,
    void** _elements
);

// IV.   creation and destruction tests
bool d_tests_sa_handler_new(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_handler_new_with_events(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_handler_new_full(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_handler_free(
    struct dj_test_counter* _test_info
);

// V.    flag management tests
bool d_tests_sa_handler_set_flag(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_handler_clear_flag(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_handler_has_flag(
    struct dj_test_counter* _test_info
);

// VI.   event listener registration tests
bool d_tests_sa_handler_register_listener(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_handler_unregister_listener(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_handler_listener_enable_disable(
    struct dj_test_counter* _test_info
);

// VII.  event emission tests
bool d_tests_sa_handler_emit_event(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_handler_event_lifecycle(
    struct dj_test_counter* _test_info
);

// VIII. test execution tests
bool d_tests_sa_handler_run_test(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_handler_run_block(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_handler_run_test_type(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_handler_nested_execution(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_handler_config_override(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_handler_run_module(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_handler_run_tree_and_session(
    struct dj_test_counter* _test_info
);

// IX.   assertion tracking tests
bool d_tests_sa_handler_record_assertion(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_handler_assertion_statistics(
    struct dj_test_counter* _test_info
);

// X.    stack operations tests
bool d_tests_sa_handler_result_stack(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_handler_context_stack(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_handler_stack_no_alloc(
    struct dj_test_counter* _test_info
);

// XI.   result query tests
bool d_tests_sa_handler_get_results(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_handler_reset_results(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_handler_print_results(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_handler_get_pass_rate(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_handler_get_assertion_rate(
    struct dj_test_counter* _test_info
);

// XII.  context helper tests
bool d_tests_sa_handler_context_new(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_handler_context_init(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_handler_context_free(
    struct dj_test_counter* _test_info
);

// XIII. DSL helper tests
bool d_tests_sa_handler_create_module_metadata(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_handler_set_metadata_str(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_handler_create_block_from_nodes(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_handler_create_module_from_decl(
    struct dj_test_counter* _test_info
);

// XIV.  statistics and depth tracking tests
bool d_tests_sa_handler_depth_tracking(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_handler_max_depth(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_handler_block_counting(
    struct dj_test_counter* _test_info
);

// XV.   abort on failure tests
bool d_tests_sa_handler_abort_on_failure(
    struct dj_test_counter* _test_info
);

// XVI.  edge cases and error handling tests
bool d_tests_sa_handler_null_parameters(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_handler_empty_blocks(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_handler_no_events(
    struct dj_test_counter* _test_info
);

// XVII. integration tests
bool d_tests_sa_handler_complex_execution(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_handler_event_integration(
    struct dj_test_counter* _test_info
);
bool d_tests_sa_handler_statistics_accuracy(
    struct dj_test_counter* _test_info
);

// XVIII. comprehensive test suite
bool d_tests_sa_test_handler_all(
    struct dj_test_counter* _test_info
);


#endif  // DJINTERP_TESTING_TEST_HANDLER_SA_
