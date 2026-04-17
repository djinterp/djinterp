/******************************************************************************
* djinterp [test]                                        event_table_tests_sa.h
*
*   Unit test declarations for `event_table.h` module.
*   Provides comprehensive testing of hash table creation/destruction,
* core CRUD operations, table management, iterator traversal,
* and statistics reporting.
*   Note: this module is required to build DTest, so it uses
* `test_standalone.h` rather than DTest for unit testing.
*
*
* path:      /tests/c/event/event_table_tests_sa.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.10.04
******************************************************************************/

#ifndef DJINTERP_TESTING_EVENT_TABLE_SA_
#define DJINTERP_TESTING_EVENT_TABLE_SA_ 1

#include <stdlib.h>
#include <errno.h>
#include "../../../inc/c/djinterp.h"
#include "../../../inc/c/dmemory.h"
#include "../../../inc/c/event/event.h"
#include "../../../inc/c/event/event_table.h"
#include "../../../inc/c/event/event_table_common.h"
#include "../../../inc/c/test/test_standalone.h"


/******************************************************************************
 * I. CREATION AND DESTRUCTION TESTS
 *****************************************************************************/
bool d_tests_sa_et_new_valid(struct d_test_counter* _counter);
bool d_tests_sa_et_new_zero(struct d_test_counter* _counter);
bool d_tests_sa_et_new_small(struct d_test_counter* _counter);
bool d_tests_sa_et_new_large(struct d_test_counter* _counter);
bool d_tests_sa_et_new_default(struct d_test_counter* _counter);
bool d_tests_sa_et_free_valid(struct d_test_counter* _counter);
bool d_tests_sa_et_free_null(struct d_test_counter* _counter);
bool d_tests_sa_et_free_with_elements(struct d_test_counter* _counter);

// I.   aggregation function
bool d_tests_sa_et_creation_all(struct d_test_counter* _counter);


/******************************************************************************
 * II. CORE OPERATIONS TESTS
 *****************************************************************************/
bool d_tests_sa_et_insert_valid(struct d_test_counter* _counter);
bool d_tests_sa_et_insert_disabled(struct d_test_counter* _counter);
bool d_tests_sa_et_insert_update(struct d_test_counter* _counter);
bool d_tests_sa_et_insert_null(struct d_test_counter* _counter);
bool d_tests_sa_et_lookup_found(struct d_test_counter* _counter);
bool d_tests_sa_et_lookup_missing(struct d_test_counter* _counter);
bool d_tests_sa_et_lookup_null(struct d_test_counter* _counter);
bool d_tests_sa_et_remove_valid(struct d_test_counter* _counter);
bool d_tests_sa_et_remove_missing_and_null(struct d_test_counter* _counter);
bool d_tests_sa_et_contains_basic(struct d_test_counter* _counter);

// II.  aggregation function
bool d_tests_sa_et_operations_all(struct d_test_counter* _counter);


/******************************************************************************
 * III. TABLE MANAGEMENT TESTS
 *****************************************************************************/
bool d_tests_sa_et_size_basic(struct d_test_counter* _counter);
bool d_tests_sa_et_count_basic(struct d_test_counter* _counter);
bool d_tests_sa_et_enabled_count_mixed(struct d_test_counter* _counter);
bool d_tests_sa_et_load_factor_basic(struct d_test_counter* _counter);
bool d_tests_sa_et_resize_valid(struct d_test_counter* _counter);
bool d_tests_sa_et_resize_null(struct d_test_counter* _counter);
bool d_tests_sa_et_clear_valid(struct d_test_counter* _counter);
bool d_tests_sa_et_clear_null(struct d_test_counter* _counter);

// III. aggregation function
bool d_tests_sa_et_management_all(struct d_test_counter* _counter);


/******************************************************************************
 * IV. ITERATOR TESTS
 *****************************************************************************/
bool d_tests_sa_et_iter_traversal(struct d_test_counter* _counter);
bool d_tests_sa_et_iter_empty(struct d_test_counter* _counter);
bool d_tests_sa_et_iter_null(struct d_test_counter* _counter);

// IV.  aggregation function
bool d_tests_sa_et_iterator_all(struct d_test_counter* _counter);


/******************************************************************************
 * V. STATISTICS TESTS
 *****************************************************************************/
bool d_tests_sa_et_stats_empty(struct d_test_counter* _counter);
bool d_tests_sa_et_stats_populated(struct d_test_counter* _counter);
bool d_tests_sa_et_stats_null(struct d_test_counter* _counter);

// V.   aggregation function
bool d_tests_sa_et_statistics_all(struct d_test_counter* _counter);


/******************************************************************************
 * VI. INTEGRATION, STRESS, AND EDGE CASE TESTS
 *****************************************************************************/
bool d_tests_sa_et_integration(struct d_test_counter* _counter);
bool d_tests_sa_et_stress(struct d_test_counter* _counter);
bool d_tests_sa_et_collision(struct d_test_counter* _counter);
bool d_tests_sa_et_auto_resize(struct d_test_counter* _counter);

// VI.  aggregation function
bool d_tests_sa_et_advanced_all(struct d_test_counter* _counter);


/******************************************************************************
 * MODULE-LEVEL AGGREGATION
 *****************************************************************************/
bool d_tests_sa_event_hash_table_all(struct d_test_counter* _counter);


#endif  // DJINTERP_TESTING_EVENT_TABLE_SA_
