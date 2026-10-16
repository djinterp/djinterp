/*******************************************************************************
* djinterp [test]                                               event_tests_sa.h
*
*   Unit test declarations for `event.h` module.
*   Provides comprehensive testing of all event functions including
* constant/type definitions, d_event constructors (d_event_new, d_event_new_ctx),
* d_event_listener constructors, comparison functions (d_event_compare,
* d_event_compare_deep, d_event_listener_compare), search
* (d_event_listener_find_index_of), and destructor functions.
*   No external container modules are required.
*
*
* path:      /tests/djinterp/c/event/event_tests_sa.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.02.03
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef DJINTERP_TESTING_EVENT_TESTS_STANDALONE_
#define DJINTERP_TESTING_EVENT_TESTS_STANDALONE_ 1

// std
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
// djinterp
#include "../../../../inc/djinterp/c/djinterp.h"
#include "../../../../inc/djinterp/c/string_fn.h"
#include "../../../../inc/djinterp/c/dtime.h"
#include "../../../../inc/djinterp/c/event/event.h"
#include "../../../../inc/djinterp/test/c/test_counter.h"


/******************************************************************************
 * I. CONSTANT AND TYPE TESTS
 *****************************************************************************/
bool d_tests_sa_event_default_size(struct d_test_counter* _counter);
bool d_tests_sa_event_id_type(struct d_test_counter* _counter);
bool d_tests_sa_event_struct(struct d_test_counter* _counter);
bool d_tests_sa_event_listener_struct(struct d_test_counter* _counter);

// I.   aggregation function
bool d_tests_sa_event_constant_all(struct d_test_counter* _counter);


/******************************************************************************
 * II. d_event CONSTRUCTOR TESTS
 *****************************************************************************/
bool d_tests_sa_event_new_basic(struct d_test_counter* _counter);
bool d_tests_sa_event_new_ids(struct d_test_counter* _counter);
bool d_tests_sa_event_new_distinct(struct d_test_counter* _counter);
bool d_tests_sa_event_new_ctx_null(struct d_test_counter* _counter);
bool d_tests_sa_event_new_ctx_valid(struct d_test_counter* _counter);
bool d_tests_sa_event_new_ctx_distinct(struct d_test_counter* _counter);

// II.  aggregation function
bool d_tests_sa_event_constructor_all(struct d_test_counter* _counter);


/******************************************************************************
 * III. d_event_listener CONSTRUCTOR TESTS
 *****************************************************************************/
bool d_tests_sa_event_listener_new_null_cb(struct d_test_counter* _counter);
bool d_tests_sa_event_listener_new_enabled(struct d_test_counter* _counter);
bool d_tests_sa_event_listener_new_disabled(struct d_test_counter* _counter);
bool d_tests_sa_event_listener_new_ids(struct d_test_counter* _counter);
bool d_tests_sa_event_listener_new_distinct(struct d_test_counter* _counter);
bool d_tests_sa_event_listener_default_null_cb(struct d_test_counter* _counter);
bool d_tests_sa_event_listener_default_valid(struct d_test_counter* _counter);
bool d_tests_sa_event_listener_default_ids(struct d_test_counter* _counter);
bool d_tests_sa_event_listener_default_distinct(struct d_test_counter* _counter);

// III. aggregation function
bool d_tests_sa_event_listener_constructor_all(struct d_test_counter* _counter);


/******************************************************************************
 * IV. COMPARISON FUNCTION TESTS
 *****************************************************************************/
bool d_tests_sa_event_compare_null(struct d_test_counter* _counter);
bool d_tests_sa_event_compare_ids(struct d_test_counter* _counter);
bool d_tests_sa_event_compare_context(struct d_test_counter* _counter);
bool d_tests_sa_event_compare_self(struct d_test_counter* _counter);
bool d_tests_sa_event_compare_deep_null(struct d_test_counter* _counter);
bool d_tests_sa_event_compare_deep_ids(struct d_test_counter* _counter);
bool d_tests_sa_event_compare_deep_context(struct d_test_counter* _counter);
bool d_tests_sa_event_compare_deep_no_comparator(struct d_test_counter* _counter);
bool d_tests_sa_event_listener_compare_null(struct d_test_counter* _counter);
bool d_tests_sa_event_listener_compare_ids(struct d_test_counter* _counter);
bool d_tests_sa_event_listener_compare_self(struct d_test_counter* _counter);
bool d_tests_sa_event_listener_compare_ignores_enabled(struct d_test_counter* _counter);

// IV.  aggregation function
bool d_tests_sa_event_compare_all(struct d_test_counter* _counter);


/******************************************************************************
 * V. SEARCH FUNCTION TESTS
 *****************************************************************************/
bool d_tests_sa_event_find_null(struct d_test_counter* _counter);
bool d_tests_sa_event_find_zero_count(struct d_test_counter* _counter);
bool d_tests_sa_event_find_single_match(struct d_test_counter* _counter);
bool d_tests_sa_event_find_single_no_match(struct d_test_counter* _counter);
bool d_tests_sa_event_find_multi_one_match(struct d_test_counter* _counter);
bool d_tests_sa_event_find_multi_many_matches(struct d_test_counter* _counter);
bool d_tests_sa_event_find_no_matches(struct d_test_counter* _counter);
bool d_tests_sa_event_find_null_entries(struct d_test_counter* _counter);
bool d_tests_sa_event_find_all_match(struct d_test_counter* _counter);

// V.   aggregation function
bool d_tests_sa_event_find_all(struct d_test_counter* _counter);


/******************************************************************************
 * VI. DESTRUCTOR TESTS
 *****************************************************************************/
bool d_tests_sa_event_free_null(struct d_test_counter* _counter);
bool d_tests_sa_event_free_basic(struct d_test_counter* _counter);
bool d_tests_sa_event_free_with_context(struct d_test_counter* _counter);
bool d_tests_sa_event_free_sequential(struct d_test_counter* _counter);
bool d_tests_sa_event_listener_free_null(struct d_test_counter* _counter);
bool d_tests_sa_event_listener_free_enabled(struct d_test_counter* _counter);
bool d_tests_sa_event_listener_free_disabled(struct d_test_counter* _counter);
bool d_tests_sa_event_listener_free_default(struct d_test_counter* _counter);
bool d_tests_sa_event_listener_free_sequential(struct d_test_counter* _counter);

// VI.  aggregation function
bool d_tests_sa_event_free_all(struct d_test_counter* _counter);


/******************************************************************************
 * MODULE-LEVEL AGGREGATION
 *****************************************************************************/
bool d_tests_sa_event_run_all(struct d_test_counter* _counter);


#endif  // DJINTERP_TESTING_EVENT_TESTS_STANDALONE_
