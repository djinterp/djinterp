/******************************************************************************
* djinterp [test]                                event_table_common_tests_sa.h
*
*   Unit test declarations for `event_table_common.h` module.
*   Provides comprehensive testing of hash functions (FNV-1a and simple),
* prime number utilities, hash node management, statistics structures,
* and integration/stress scenarios.
*   Note: this module is required to build DTest, so it uses
* `test_standalone.h` rather than DTest for unit testing.
*
*
* path:      /tests/c/event/event_table_common_tests_sa.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.10.04
******************************************************************************/

#ifndef DJINTERP_TESTING_EVENT_TABLE_COMMON_STANDALONE_
#define DJINTERP_TESTING_EVENT_TABLE_COMMON_STANDALONE_ 1

#include <stdlib.h>
#include <errno.h>
#include "../../../inc/c/djinterp.h"
#include "../../../inc/c/dmemory.h"
#include "../../../inc/c/string_fn.h"
#include "../../../inc/c/test/test_standalone.h"
#include "../../../inc/c/event/event.h"
#include "../../../inc/c/event/event_table_common.h"


/******************************************************************************
 * I. CONSTANT AND TYPE TESTS
 *****************************************************************************/
bool d_tests_sa_event_hash_constants(struct d_test_counter* _counter);
bool d_tests_sa_event_hash_node_struct(struct d_test_counter* _counter);
bool d_tests_sa_event_hash_stats_struct(struct d_test_counter* _counter);

// I.   aggregation function
bool d_tests_sa_event_hash_constant_all(struct d_test_counter* _counter);


/******************************************************************************
 * II. HASH FUNCTION TESTS
 *****************************************************************************/
bool d_tests_sa_event_hash_fnv_bounds(struct d_test_counter* _counter);
bool d_tests_sa_event_hash_fnv_consistency(struct d_test_counter* _counter);
bool d_tests_sa_event_hash_fnv_differentiation(struct d_test_counter* _counter);
bool d_tests_sa_event_hash_fnv_edge_ids(struct d_test_counter* _counter);
bool d_tests_sa_event_hash_fnv_table_sizes(struct d_test_counter* _counter);
bool d_tests_sa_event_hash_simple_bounds(struct d_test_counter* _counter);
bool d_tests_sa_event_hash_simple_consistency(struct d_test_counter* _counter);
bool d_tests_sa_event_hash_simple_edge_ids(struct d_test_counter* _counter);
bool d_tests_sa_event_hash_collision(struct d_test_counter* _counter);
bool d_tests_sa_event_hash_distribution(struct d_test_counter* _counter);

// II.  aggregation function
bool d_tests_sa_event_hash_all(struct d_test_counter* _counter);


/******************************************************************************
 * III. PRIME NUMBER UTILITY TESTS
 *****************************************************************************/
bool d_tests_sa_event_hash_next_prime_basic(struct d_test_counter* _counter);
bool d_tests_sa_event_hash_next_prime_known(struct d_test_counter* _counter);
bool d_tests_sa_event_hash_next_prime_edge(struct d_test_counter* _counter);
bool d_tests_sa_event_hash_next_prime_sequence(struct d_test_counter* _counter);
bool d_tests_sa_event_hash_next_prime_large(struct d_test_counter* _counter);

// III. aggregation function
bool d_tests_sa_event_hash_prime_all(struct d_test_counter* _counter);


/******************************************************************************
 * IV. NODE MANAGEMENT TESTS
 *****************************************************************************/
bool d_tests_sa_event_hash_node_new_valid(struct d_test_counter* _counter);
bool d_tests_sa_event_hash_node_new_null_value(struct d_test_counter* _counter);
bool d_tests_sa_event_hash_node_new_ids(struct d_test_counter* _counter);
bool d_tests_sa_event_hash_node_free_valid(struct d_test_counter* _counter);
bool d_tests_sa_event_hash_node_free_null(struct d_test_counter* _counter);
bool d_tests_sa_event_hash_node_free_preserves_listener(struct d_test_counter* _counter);
bool d_tests_sa_event_hash_node_chaining(struct d_test_counter* _counter);

// IV.  aggregation function
bool d_tests_sa_event_hash_node_all(struct d_test_counter* _counter);


/******************************************************************************
 * V. STATISTICS PRINT TESTS
 *****************************************************************************/
bool d_tests_sa_event_hash_stats_print_valid(struct d_test_counter* _counter);
bool d_tests_sa_event_hash_stats_print_null(struct d_test_counter* _counter);

// V.   aggregation function
bool d_tests_sa_event_hash_stats_all(struct d_test_counter* _counter);


/******************************************************************************
 * VI. INTEGRATION, STRESS, AND EDGE CASE TESTS
 *****************************************************************************/
bool d_tests_sa_event_hash_integration(struct d_test_counter* _counter);
bool d_tests_sa_event_hash_stress_prime(struct d_test_counter* _counter);
bool d_tests_sa_event_hash_stress_hash(struct d_test_counter* _counter);
bool d_tests_sa_event_hash_edge_cases(struct d_test_counter* _counter);

// VI.  aggregation function
bool d_tests_sa_event_hash_advanced_all(struct d_test_counter* _counter);


/******************************************************************************
 * MODULE-LEVEL AGGREGATION
 *****************************************************************************/
bool d_tests_sa_event_table_common_all(struct d_test_counter* _counter);


#endif  // DJINTERP_TESTING_EVENT_TABLE_COMMON_STANDALONE_
