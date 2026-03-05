/******************************************************************************
* djinterp [test]                                       test_handler_tests_sa.h
*
* This is a test file for `test_handler.h` unit tests.
* For the file itself, go to `\inc\test\test_handler.h`.
* Note: this module is required to build DTest, so it uses `test_standalone.h`,
* rather than DTest for unit testing. Any modules that are not dependencies of
* DTest should use DTest for unit tests.
*
*
* link:      TBA
* file:      \test\test\test_tests_standalone.h
* author(s): Samuel 'teer' Neal-Blim                           date: 2025.10.05
******************************************************************************/

#ifndef DJINTERP_TESTING_TEST_STANDALONE_
#define	DJINTERP_TESTING_TEST_STANDALONE_ 1

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "../../../inc/c/djinterp.h"
#include "../../../inc/c/dmemory.h"
#include "../../../inc/c/event/event.h"
#include "../../../inc/c/event/event_handler.h"
#include "../../../inc/c/test/assert.h"
#include "../../../inc/c/test/test.h"
#include "../../../inc/c/test/test_handler.h"
#include "../../../inc/c/test/test_standalone.h"



//=============================================================================
// HANDLER CREATION AND DESTRUCTION TESTS
//=============================================================================

// tests d_test_handler_new with various default configurations
bool d_tests_sa_handler_new(struct d_test_counter* _test_info);

// tests d_test_handler_new_with_events with different event capacities
bool d_tests_sa_handler_new_with_events(struct d_test_counter* _test_info);

// tests d_test_handler_free with valid and NULL handlers
bool d_tests_sa_handler_free(struct d_test_counter* _test_info);

//=============================================================================
// EVENT LISTENER REGISTRATION TESTS
//=============================================================================

// tests `d_test_handler_register_listener` with various event types
bool d_tests_sa_handler_register_listener(struct d_test_counter* _test_info);

// tests `d_test_handler_unregister_listener` behavior
bool d_tests_sa_handler_unregister_listener(struct d_test_counter* _test_info);

// tests listener enable/disable functionality
bool d_tests_sa_handler_listener_enable_disable(struct d_test_counter* _test_info);

//=============================================================================
// TEST EXECUTION TESTS
//=============================================================================

// tests d_test_handler_run_test with passing and failing tests
bool d_tests_sa_handler_run_test(struct d_test_counter* _test_info);

// tests d_test_handler_run_block with various block configurations
bool d_tests_sa_handler_run_block(struct d_test_counter* _test_info);

// tests d_test_handler_run_test_type with both test and block types
bool d_tests_sa_handler_run_test_type(struct d_test_counter* _test_info);

// tests nested block execution with depth tracking
bool d_tests_sa_handler_nested_execution(struct d_test_counter* _test_info);

// tests configuration override behavior during execution
bool d_tests_sa_handler_config_override(struct d_test_counter* _test_info);

//=============================================================================
// ASSERTION TRACKING TESTS
//=============================================================================

// tests d_test_handler_record_assertion with passing assertions
bool d_tests_sa_handler_record_assertion(struct d_test_counter* _test_info);

// tests assertion statistics accumulation
bool d_tests_sa_handler_assertion_statistics(struct d_test_counter* _test_info);

//=============================================================================
// RESULT QUERY TESTS
//=============================================================================

// tests d_test_handler_get_results accuracy
bool d_tests_sa_handler_get_results(struct d_test_counter* _test_info);

// tests d_test_handler_reset_results functionality
bool d_tests_sa_handler_reset_results(struct d_test_counter* _test_info);

// tests d_test_handler_print_results output
bool d_tests_sa_handler_print_results(struct d_test_counter* _test_info);

// tests d_test_handler_get_pass_rate calculations
bool d_tests_sa_handler_get_pass_rate(struct d_test_counter* _test_info);

// tests d_test_handler_get_assertion_rate calculations
bool d_tests_sa_handler_get_assertion_rate(struct d_test_counter* _test_info);

//=============================================================================
// EVENT EMISSION TESTS
//=============================================================================

// tests d_test_handler_emit_event for all lifecycle events
bool d_tests_sa_handler_emit_event(struct d_test_counter* _test_info);

// tests event firing during test execution
bool d_tests_sa_handler_event_lifecycle(struct d_test_counter* _test_info);

//=============================================================================
// STATISTICS AND DEPTH TRACKING TESTS
//=============================================================================

// tests depth tracking during nested block execution
bool d_tests_sa_handler_depth_tracking(struct d_test_counter* _test_info);

// tests max_depth calculation
bool d_tests_sa_handler_max_depth(struct d_test_counter* _test_info);

// tests block count tracking
bool d_tests_sa_handler_block_counting(struct d_test_counter* _test_info);

//=============================================================================
// ABORT ON FAILURE TESTS
//=============================================================================

// tests abort_on_failure behavior
bool d_tests_sa_handler_abort_on_failure(struct d_test_counter* _test_info);

//=============================================================================
// EDGE CASES AND ERROR HANDLING TESTS
//=============================================================================

// tests NULL parameter handling across all functions
bool d_tests_sa_handler_null_parameters(struct d_test_counter* _test_info);

// tests empty block execution
bool d_tests_sa_handler_empty_blocks(struct d_test_counter* _test_info);

// tests handler with no event system
bool d_tests_sa_handler_no_events(struct d_test_counter* _test_info);

//=============================================================================
// INTEGRATION TESTS
//=============================================================================

// tests complex execution scenarios with multiple blocks
bool d_tests_sa_handler_complex_execution(struct d_test_counter* _test_info);

// tests event system integration during complex execution
bool d_tests_sa_handler_event_integration(struct d_test_counter* _test_info);

// tests statistics accuracy in complex scenarios
bool d_tests_sa_handler_statistics_accuracy(struct d_test_counter* _test_info);

//=============================================================================
// COMPREHENSIVE TEST SUITE
//=============================================================================

// Runs all test_handler unit tests
bool d_tests_sa_test_handler_all(struct d_test_counter* _test_info);

//=============================================================================
// HELPER TEST FUNCTIONS AND EVENT CALLBACKS
//=============================================================================

// Simple test that always passes
bool handler_test_passing(struct d_test* _test);

// Simple test that always fails
bool handler_test_failing(struct d_test* _test);

// Test that records assertion
bool handler_test_with_assertion(struct d_test* _test);

// Global event callback tracker for testing
extern int g_event_setup_count;
extern int g_event_start_count;
extern int g_event_success_count;
extern int g_event_failure_count;
extern int g_event_end_count;
extern int g_event_teardown_count;

// Reset event counters
void reset_event_counters(void);

// Event callback functions for testing
void callback_setup(size_t _size, void** _elements);
void callback_start(size_t _size, void** _elements);
void callback_success(size_t _size, void** _elements);
void callback_failure(size_t _size, void** _elements);
void callback_end(size_t _size, void** _elements);
void callback_teardown(size_t _size, void** _elements);


#endif	// DJINTERP_TESTING_TEST_STANDALONE_