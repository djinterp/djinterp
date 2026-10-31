/*******************************************************************************
* djinterp [test]                                         event_table_tests_sa.h
*
*   Unit test declarations for `event_table.h` module.
*   Provides comprehensive testing of hash table creation/destruction,
* core CRUD operations, table management, iterator traversal,
* and statistics reporting.
*   Note: this module is required to build DTest, so it uses
* a self-contained harness rather than DTest for unit testing.
*
*
* path:      /tests/djinterp/c/event/event_table_tests_sa.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.10.04
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef DJINTERP_TESTING_EVENT_TABLE_SA_
#define DJINTERP_TESTING_EVENT_TABLE_SA_ 1

// std
#include <errno.h>
#include <stdlib.h>
// djinterp
#include "../../../../inc/djinterp/c/djinterp.h"
#include "../../../../inc/djinterp/c/dmemory.h"
#include "../../../../inc/djinterp/c/event/event.h"
#include "../../../../inc/djinterp/c/event/event_table_common.h"
#include "../../../../inc/djinterp/c/modules/test/test_counter.h"


/******************************************************************************
 * I. CREATION AND DESTRUCTION TESTS
 *****************************************************************************/
bool d_tests_sa_et_new_valid(
    struct dj_test_counter* _counter
);
bool d_tests_sa_et_new_zero(
    struct dj_test_counter* _counter
);
bool d_tests_sa_et_new_small(
    struct dj_test_counter* _counter
);
bool d_tests_sa_et_new_large(
    struct dj_test_counter* _counter
);
bool d_tests_sa_et_new_default(
    struct dj_test_counter* _counter
);
bool d_tests_sa_et_free_valid(
    struct dj_test_counter* _counter
);
bool d_tests_sa_et_free_null(
    struct dj_test_counter* _counter
);
bool d_tests_sa_et_free_with_elements(
    struct dj_test_counter* _counter
);

// I.   aggregation function
bool d_tests_sa_et_creation_all(
    struct dj_test_counter* _counter
);


/******************************************************************************
 * II. CORE OPERATIONS TESTS
 *****************************************************************************/
bool d_tests_sa_et_insert_valid(
    struct dj_test_counter* _counter
);
bool d_tests_sa_et_insert_disabled(
    struct dj_test_counter* _counter
);
bool d_tests_sa_et_insert_update(
    struct dj_test_counter* _counter
);
bool d_tests_sa_et_insert_null(
    struct dj_test_counter* _counter
);
bool d_tests_sa_et_lookup_found(
    struct dj_test_counter* _counter
);
bool d_tests_sa_et_lookup_missing(
    struct dj_test_counter* _counter
);
bool d_tests_sa_et_lookup_null(
    struct dj_test_counter* _counter
);
bool d_tests_sa_et_remove_valid(
    struct dj_test_counter* _counter
);
bool d_tests_sa_et_remove_missing_and_null(
    struct dj_test_counter* _counter
);
bool d_tests_sa_et_contains_basic(
    struct dj_test_counter* _counter
);

// II.  aggregation function
bool d_tests_sa_et_operations_all(
    struct dj_test_counter* _counter
);


/******************************************************************************
 * III. TABLE MANAGEMENT TESTS
 *****************************************************************************/
bool d_tests_sa_et_size_basic(
    struct dj_test_counter* _counter
);
bool d_tests_sa_et_count_basic(
    struct dj_test_counter* _counter
);
bool d_tests_sa_et_enabled_count_mixed(
    struct dj_test_counter* _counter
);
bool d_tests_sa_et_load_factor_basic(
    struct dj_test_counter* _counter
);
bool d_tests_sa_et_resize_valid(
    struct dj_test_counter* _counter
);
bool d_tests_sa_et_resize_null(
    struct dj_test_counter* _counter
);
bool d_tests_sa_et_clear_valid(
    struct dj_test_counter* _counter
);
bool d_tests_sa_et_clear_null(
    struct dj_test_counter* _counter
);

// III. aggregation function
bool d_tests_sa_et_management_all(
    struct dj_test_counter* _counter
);


/******************************************************************************
 * IV. ITERATOR TESTS
 *****************************************************************************/
bool d_tests_sa_et_iter_traversal(
    struct dj_test_counter* _counter
);
bool d_tests_sa_et_iter_empty(
    struct dj_test_counter* _counter
);
bool d_tests_sa_et_iter_null(
    struct dj_test_counter* _counter
);

// IV.  aggregation function
bool d_tests_sa_et_iterator_all(
    struct dj_test_counter* _counter
);


/******************************************************************************
 * V. STATISTICS TESTS
 *****************************************************************************/
bool d_tests_sa_et_stats_empty(
    struct dj_test_counter* _counter
);
bool d_tests_sa_et_stats_populated(
    struct dj_test_counter* _counter
);
bool d_tests_sa_et_stats_null(
    struct dj_test_counter* _counter
);

// V.   aggregation function
bool d_tests_sa_et_statistics_all(
    struct dj_test_counter* _counter
);


/******************************************************************************
 * VI. INTEGRATION, STRESS, AND EDGE CASE TESTS
 *****************************************************************************/
bool d_tests_sa_et_integration(
    struct dj_test_counter* _counter
);
bool d_tests_sa_et_stress(
    struct dj_test_counter* _counter
);
bool d_tests_sa_et_collision(
    struct dj_test_counter* _counter
);
bool d_tests_sa_et_auto_resize(
    struct dj_test_counter* _counter
);

// VI.  aggregation function
bool d_tests_sa_et_advanced_all(
    struct dj_test_counter* _counter
);


/******************************************************************************
 * MODULE-LEVEL AGGREGATION
 *****************************************************************************/
bool d_tests_sa_event_hash_table_all(
    struct dj_test_counter* _counter
);


#endif  // DJINTERP_TESTING_EVENT_TABLE_SA_
