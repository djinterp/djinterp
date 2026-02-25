/******************************************************************************
* djinterp [test]                                    event_handler_tests_sa.h
*
*   Unit test declarations for `event_handler.h` module.
*   Provides comprehensive testing of handler creation/destruction,
* listener management (bind/unbind/enable/disable), event operations
* (fire/queue/process), query functions, and integration/stress scenarios.
*   Note: this module is required to build DTest, so it uses
* `test_standalone.h` rather than DTest for unit testing.
*
*
* path:      /tests/c/event/event_handler_tests_sa.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.10.05
******************************************************************************/

#ifndef DJINTERP_TESTING_EVENT_HANDLER_STANDALONE_
#define DJINTERP_TESTING_EVENT_HANDLER_STANDALONE_ 1

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "../../../inc/c/djinterp.h"
#include "../../../inc/c/dmemory.h"
#include "../../../inc/c/test/test_standalone.h"
#include "../../../inc/c/container/array/circular_array.h"
#include "../../../inc/c/event/event_handler.h"


/******************************************************************************
 * I. CREATION AND DESTRUCTION TESTS
 *****************************************************************************/
bool d_tests_sa_event_handler_new_valid(struct d_test_counter* _counter);
bool d_tests_sa_event_handler_new_zero(struct d_test_counter* _counter);
bool d_tests_sa_event_handler_new_large(struct d_test_counter* _counter);
bool d_tests_sa_event_handler_free_valid(struct d_test_counter* _counter);
bool d_tests_sa_event_handler_free_null(struct d_test_counter* _counter);
bool d_tests_sa_event_handler_free_with_listeners(struct d_test_counter* _counter);

// I.   aggregation function
bool d_tests_sa_event_handler_creation_all(struct d_test_counter* _counter);


/******************************************************************************
 * II. LISTENER MANAGEMENT TESTS
 *****************************************************************************/
bool d_tests_sa_event_handler_bind_valid(struct d_test_counter* _counter);
bool d_tests_sa_event_handler_bind_update(struct d_test_counter* _counter);
bool d_tests_sa_event_handler_bind_null(struct d_test_counter* _counter);
bool d_tests_sa_event_handler_unbind_valid(struct d_test_counter* _counter);
bool d_tests_sa_event_handler_unbind_missing_and_null(struct d_test_counter* _counter);
bool d_tests_sa_event_handler_get_listener_basic(struct d_test_counter* _counter);
bool d_tests_sa_event_handler_enable_disable(struct d_test_counter* _counter);
bool d_tests_sa_event_handler_enable_disable_null(struct d_test_counter* _counter);

// II.  aggregation function
bool d_tests_sa_event_handler_listener_mgmt_all(struct d_test_counter* _counter);


/******************************************************************************
 * III. EVENT OPERATIONS TESTS
 *****************************************************************************/
bool d_tests_sa_event_handler_fire_valid(struct d_test_counter* _counter);
bool d_tests_sa_event_handler_fire_no_listener(struct d_test_counter* _counter);
bool d_tests_sa_event_handler_fire_disabled(struct d_test_counter* _counter);
bool d_tests_sa_event_handler_fire_null(struct d_test_counter* _counter);
bool d_tests_sa_event_handler_queue_valid(struct d_test_counter* _counter);
bool d_tests_sa_event_handler_queue_null(struct d_test_counter* _counter);
bool d_tests_sa_event_handler_process_all(struct d_test_counter* _counter);
bool d_tests_sa_event_handler_process_limited(struct d_test_counter* _counter);
bool d_tests_sa_event_handler_process_null(struct d_test_counter* _counter);

// III. aggregation function
bool d_tests_sa_event_handler_event_ops_all(struct d_test_counter* _counter);


/******************************************************************************
 * IV. QUERY FUNCTION TESTS
 *****************************************************************************/
bool d_tests_sa_event_handler_queries_empty(struct d_test_counter* _counter);
bool d_tests_sa_event_handler_queries_populated(struct d_test_counter* _counter);
bool d_tests_sa_event_handler_queries_null(struct d_test_counter* _counter);

// IV.  aggregation function
bool d_tests_sa_event_handler_query_all(struct d_test_counter* _counter);


/******************************************************************************
 * V. INTEGRATION, STRESS, AND EDGE CASE TESTS
 *****************************************************************************/
bool d_tests_sa_event_handler_integration(struct d_test_counter* _counter);
bool d_tests_sa_event_handler_stress(struct d_test_counter* _counter);
bool d_tests_sa_event_handler_edge_empty_queue(struct d_test_counter* _counter);
bool d_tests_sa_event_handler_edge_idempotent(struct d_test_counter* _counter);

// V.   aggregation function
bool d_tests_sa_event_handler_advanced_all(struct d_test_counter* _counter);


/******************************************************************************
 * MODULE-LEVEL AGGREGATION
 *****************************************************************************/
bool d_tests_sa_event_handler_all(struct d_test_counter* _counter);


#endif  // DJINTERP_TESTING_EVENT_HANDLER_STANDALONE_
