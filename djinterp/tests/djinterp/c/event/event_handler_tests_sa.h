/*******************************************************************************
* djinterp [test]                                       event_handler_tests_sa.h
*
*   Unit test declarations for `event_handler.h` module.
*   Provides comprehensive testing of handler creation/destruction,
* listener management (bind/unbind/enable/disable), event operations
* (fire/queue/process), query functions, and integration/stress scenarios.
*   Note: this module is required to build DTest, so it uses
* a self-contained harness rather than DTest for unit testing.
*
*
* path:      /tests/djinterp/c/event/event_handler_tests_sa.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.10.05
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef DJINTERP_TESTING_EVENT_HANDLER_STANDALONE_
#define DJINTERP_TESTING_EVENT_HANDLER_STANDALONE_ 1

// std
#include <errno.h>
#include <stdlib.h>
#include <string.h>
// djinterp
#include "../../../../inc/djinterp/c/djinterp.h"
#include "../../../../inc/djinterp/c/dmemory.h"
#include "../../../../inc/djinterp/c/container/array/circular_array.h"
#include "../../../../inc/djinterp/c/modules/test/test_counter.h"


/******************************************************************************
 * I. CREATION AND DESTRUCTION TESTS
 *****************************************************************************/
bool d_tests_sa_event_handler_new_valid(
    struct dj_test_counter* _counter
);
bool d_tests_sa_event_handler_new_zero(
    struct dj_test_counter* _counter
);
bool d_tests_sa_event_handler_new_large(
    struct dj_test_counter* _counter
);
bool d_tests_sa_event_handler_free_valid(
    struct dj_test_counter* _counter
);
bool d_tests_sa_event_handler_free_null(
    struct dj_test_counter* _counter
);
bool d_tests_sa_event_handler_free_with_listeners(
    struct dj_test_counter* _counter
);

// I.   aggregation function
bool d_tests_sa_event_handler_creation_all(
    struct dj_test_counter* _counter
);


/******************************************************************************
 * II. LISTENER MANAGEMENT TESTS
 *****************************************************************************/
bool d_tests_sa_event_handler_bind_valid(
    struct dj_test_counter* _counter
);
bool d_tests_sa_event_handler_bind_update(
    struct dj_test_counter* _counter
);
bool d_tests_sa_event_handler_bind_null(
    struct dj_test_counter* _counter
);
bool d_tests_sa_event_handler_unbind_valid(
    struct dj_test_counter* _counter
);
bool d_tests_sa_event_handler_unbind_missing_and_null(
    struct dj_test_counter* _counter
);
bool d_tests_sa_event_handler_get_listener_basic(
    struct dj_test_counter* _counter
);
bool d_tests_sa_event_handler_enable_disable(
    struct dj_test_counter* _counter
);
bool d_tests_sa_event_handler_enable_disable_null(
    struct dj_test_counter* _counter
);

// II.  aggregation function
bool d_tests_sa_event_handler_listener_mgmt_all(
    struct dj_test_counter* _counter
);


/******************************************************************************
 * III. EVENT OPERATIONS TESTS
 *****************************************************************************/
bool d_tests_sa_event_handler_fire_valid(
    struct dj_test_counter* _counter
);
bool d_tests_sa_event_handler_fire_no_listener(
    struct dj_test_counter* _counter
);
bool d_tests_sa_event_handler_fire_disabled(
    struct dj_test_counter* _counter
);
bool d_tests_sa_event_handler_fire_null(
    struct dj_test_counter* _counter
);
bool d_tests_sa_event_handler_queue_valid(
    struct dj_test_counter* _counter
);
bool d_tests_sa_event_handler_queue_null(
    struct dj_test_counter* _counter
);
bool d_tests_sa_event_handler_process_all(
    struct dj_test_counter* _counter
);
bool d_tests_sa_event_handler_process_limited(
    struct dj_test_counter* _counter
);
bool d_tests_sa_event_handler_process_null(
    struct dj_test_counter* _counter
);

// III. aggregation function
bool d_tests_sa_event_handler_event_ops_all(
    struct dj_test_counter* _counter
);


/******************************************************************************
 * IV. QUERY FUNCTION TESTS
 *****************************************************************************/
bool d_tests_sa_event_handler_queries_empty(
    struct dj_test_counter* _counter
);
bool d_tests_sa_event_handler_queries_populated(
    struct dj_test_counter* _counter
);
bool d_tests_sa_event_handler_queries_null(
    struct dj_test_counter* _counter
);

// IV.  aggregation function
bool d_tests_sa_event_handler_query_all(
    struct dj_test_counter* _counter
);


/******************************************************************************
 * V. INTEGRATION, STRESS, AND EDGE CASE TESTS
 *****************************************************************************/
bool d_tests_sa_event_handler_integration(
    struct dj_test_counter* _counter
);
bool d_tests_sa_event_handler_stress(
    struct dj_test_counter* _counter
);
bool d_tests_sa_event_handler_edge_empty_queue(
    struct dj_test_counter* _counter
);
bool d_tests_sa_event_handler_edge_idempotent(
    struct dj_test_counter* _counter
);

// V.   aggregation function
bool d_tests_sa_event_handler_advanced_all(
    struct dj_test_counter* _counter
);


/******************************************************************************
 * MODULE-LEVEL AGGREGATION
 *****************************************************************************/
bool d_tests_sa_event_handler_all(
    struct dj_test_counter* _counter
);


#endif  // DJINTERP_TESTING_EVENT_HANDLER_STANDALONE_
