/******************************************************************************
* djinterp [test]                                       test_handler_tests_sa.h
*
*   Unit test declarations for the `test_handler` module.
*   Organized by sections matching the implementation in `test_handler.c`.
*   This module is a dependency of DTest, so it uses `test_standalone.h`
* rather than DTest for unit testing.
*
*
* link:      TBA
* file:      \test\test\test_handler_tests_sa.h
* author(s): Samuel 'teer' Neal-Blim                           date: 2025.10.05
******************************************************************************/

#ifndef DJINTERP_TESTING_TEST_HANDLER_SA_
#define DJINTERP_TESTING_TEST_HANDLER_SA_ 1

#include <stdlib.h>
#include <errno.h>
#include "../../../inc/c/djinterp.h"
#include "../../../inc/c/dmemory.h"
#include "../../../inc/c/string_fn.h"
#include "../../../inc/c/event/event.h"
#include "../../../inc/c/event/event_handler.h"
#include "../../../inc/c/test/assert.h"
#include "../../../inc/c/test/test.h"
#include "../../../inc/c/test/test_handler.h"
#include "../../../inc/c/test/test_standalone.h"


//=============================================================================
// HELPER TEST FUNCTIONS (fn_test-compatible: bool(void))
//=============================================================================

bool handler_test_passing(void);
bool handler_test_failing(void);
bool handler_test_with_assertion(void);


//=============================================================================
// HELPER FACTORY FUNCTIONS
//   Wrap test functions into proper d_test objects using the current API:
//   d_test_fn_new -> d_test_type_new -> d_test_new
//=============================================================================

// helper_make_passing_test
//   creates a d_test containing a single passing test_fn child.
struct d_test* helper_make_passing_test(void);

// helper_make_failing_test
//   creates a d_test containing a single failing test_fn child.
struct d_test* helper_make_failing_test(void);

// helper_add_passing_test_to_block
//   creates a passing test, wraps it in d_test_type, adds to block.
// returns true on success.
bool helper_add_passing_test_to_block(struct d_test_block* _block);

// helper_add_failing_test_to_block
//   creates a failing test, wraps it in d_test_type, adds to block.
// returns true on success.
bool helper_add_failing_test_to_block(struct d_test_block* _block);

// helper_add_block_child_to_block
//   wraps a child block in d_test_type and adds to parent block.
// returns true on success. parent takes ownership of child.
bool helper_add_block_child_to_block(struct d_test_block* _parent,
                                     struct d_test_block* _child);


//=============================================================================
// GLOBAL EVENT CALLBACK TRACKERS
//=============================================================================

extern int g_event_setup_count;
extern int g_event_start_count;
extern int g_event_success_count;
extern int g_event_failure_count;
extern int g_event_end_count;
extern int g_event_teardown_count;

void reset_event_counters(void);


//=============================================================================
// EVENT CALLBACK FUNCTIONS
//=============================================================================

void callback_setup(size_t _size, void** _elements);
void callback_start(size_t _size, void** _elements);
void callback_success(size_t _size, void** _elements);
void callback_failure(size_t _size, void** _elements);
void callback_end(size_t _size, void** _elements);
void callback_teardown(size_t _size, void** _elements);


//=============================================================================
// SECTION 1: CREATION AND DESTRUCTION TESTS
//=============================================================================
bool d_tests_sa_handler_new(struct d_test_counter* _test_info);
bool d_tests_sa_handler_new_with_events(struct d_test_counter* _test_info);
bool d_tests_sa_handler_new_full(struct d_test_counter* _test_info);
bool d_tests_sa_handler_free(struct d_test_counter* _test_info);

//=============================================================================
// SECTION 2: FLAG MANAGEMENT TESTS
//=============================================================================
bool d_tests_sa_handler_set_flag(struct d_test_counter* _test_info);
bool d_tests_sa_handler_clear_flag(struct d_test_counter* _test_info);
bool d_tests_sa_handler_has_flag(struct d_test_counter* _test_info);

//=============================================================================
// SECTION 3: EVENT LISTENER REGISTRATION TESTS
//=============================================================================
bool d_tests_sa_handler_register_listener(struct d_test_counter* _test_info);
bool d_tests_sa_handler_unregister_listener(struct d_test_counter* _test_info);
bool d_tests_sa_handler_listener_enable_disable(
         struct d_test_counter* _test_info);

//=============================================================================
// SECTION 4: EVENT EMISSION TESTS
//=============================================================================
bool d_tests_sa_handler_emit_event(struct d_test_counter* _test_info);
bool d_tests_sa_handler_event_lifecycle(struct d_test_counter* _test_info);

//=============================================================================
// SECTION 5: TEST EXECUTION TESTS
//=============================================================================
bool d_tests_sa_handler_run_test(struct d_test_counter* _test_info);
bool d_tests_sa_handler_run_block(struct d_test_counter* _test_info);
bool d_tests_sa_handler_run_test_type(struct d_test_counter* _test_info);
bool d_tests_sa_handler_nested_execution(struct d_test_counter* _test_info);
bool d_tests_sa_handler_config_override(struct d_test_counter* _test_info);
bool d_tests_sa_handler_run_module(struct d_test_counter* _test_info);
bool d_tests_sa_handler_run_tree_and_session(
         struct d_test_counter* _test_info);

//=============================================================================
// SECTION 6: ASSERTION TRACKING TESTS
//=============================================================================
bool d_tests_sa_handler_record_assertion(struct d_test_counter* _test_info);
bool d_tests_sa_handler_assertion_statistics(
         struct d_test_counter* _test_info);

//=============================================================================
// SECTION 7: STACK OPERATIONS TESTS
//=============================================================================
bool d_tests_sa_handler_result_stack(struct d_test_counter* _test_info);
bool d_tests_sa_handler_context_stack(struct d_test_counter* _test_info);
bool d_tests_sa_handler_stack_no_alloc(struct d_test_counter* _test_info);

//=============================================================================
// SECTION 8: RESULT QUERY TESTS
//=============================================================================
bool d_tests_sa_handler_get_results(struct d_test_counter* _test_info);
bool d_tests_sa_handler_reset_results(struct d_test_counter* _test_info);
bool d_tests_sa_handler_print_results(struct d_test_counter* _test_info);
bool d_tests_sa_handler_get_pass_rate(struct d_test_counter* _test_info);
bool d_tests_sa_handler_get_assertion_rate(struct d_test_counter* _test_info);

//=============================================================================
// SECTION 9: CONTEXT HELPER TESTS
//=============================================================================
bool d_tests_sa_handler_context_new(struct d_test_counter* _test_info);
bool d_tests_sa_handler_context_init(struct d_test_counter* _test_info);
bool d_tests_sa_handler_context_free(struct d_test_counter* _test_info);

//=============================================================================
// SECTION 10: DSL HELPER TESTS
//=============================================================================
bool d_tests_sa_handler_create_module_metadata(
         struct d_test_counter* _test_info);
bool d_tests_sa_handler_set_metadata_str(struct d_test_counter* _test_info);
bool d_tests_sa_handler_create_block_from_nodes(
         struct d_test_counter* _test_info);
bool d_tests_sa_handler_create_module_from_decl(
         struct d_test_counter* _test_info);

//=============================================================================
// SECTION 11: STATISTICS AND DEPTH TRACKING TESTS
//=============================================================================
bool d_tests_sa_handler_depth_tracking(struct d_test_counter* _test_info);
bool d_tests_sa_handler_max_depth(struct d_test_counter* _test_info);
bool d_tests_sa_handler_block_counting(struct d_test_counter* _test_info);

//=============================================================================
// SECTION 12: ABORT ON FAILURE TESTS
//=============================================================================
bool d_tests_sa_handler_abort_on_failure(struct d_test_counter* _test_info);

//=============================================================================
// SECTION 13: EDGE CASES AND ERROR HANDLING TESTS
//=============================================================================
bool d_tests_sa_handler_null_parameters(struct d_test_counter* _test_info);
bool d_tests_sa_handler_empty_blocks(struct d_test_counter* _test_info);
bool d_tests_sa_handler_no_events(struct d_test_counter* _test_info);

//=============================================================================
// SECTION 14: INTEGRATION TESTS
//=============================================================================
bool d_tests_sa_handler_complex_execution(struct d_test_counter* _test_info);
bool d_tests_sa_handler_event_integration(struct d_test_counter* _test_info);
bool d_tests_sa_handler_statistics_accuracy(
         struct d_test_counter* _test_info);

//=============================================================================
// COMPREHENSIVE TEST SUITE
//=============================================================================
bool d_tests_sa_test_handler_all(struct d_test_counter* _test_info);


#endif  // DJINTERP_TESTING_TEST_HANDLER_SA_
