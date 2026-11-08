/*******************************************************************************
* djinterp [test]                                                 dmutex_tests.h
*
*   Unit test declarations for `dmutex.h` module.
*   Provides comprehensive testing of all dmutex functions including
* basic mutex operations, recursive mutex operations, thread operations,
* thread-specific storage, condition variables, read-write locks,
* synchronization primitives, and utility functions.
*
*
* path:      /tests/djinterp/c/sync/dmutex_tests.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.02.09
*                                                            revised: 2026.11.06
*******************************************************************************/

#ifndef DJINTERP_TESTS_C_SYNC_DMUTEX_TESTS_H
#define DJINTERP_TESTS_C_SYNC_DMUTEX_TESTS_H 1

// std
#include <stddef.h>
#include "../../../../inc/re_std/c/cstdint/dstdint.h"  // fixed-width integers
#include <stdio.h>
#include <stdlib.h>
// djinterp
#include "../../../../inc/djinterp/c/sync/dmutex.h"
#include "../../../../inc/djinterp/c/string_fn.h"
#include "../../../../inc/djinterp/c/dtime.h"
#include "../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's C face


/******************************************************************************
 * III. BASIC MUTEX OPERATION TESTS
 *****************************************************************************/
bool d_tests_mutex_init(
    struct dj_test_suite* _suite
);
bool d_tests_mutex_destroy(
    struct dj_test_suite* _suite
);
bool d_tests_mutex_lock(
    struct dj_test_suite* _suite
);
bool d_tests_mutex_trylock(
    struct dj_test_suite* _suite
);
bool d_tests_mutex_unlock(
    struct dj_test_suite* _suite
);
bool d_tests_mutex_timedlock(
    struct dj_test_suite* _suite
);

// III. aggregation function
bool d_tests_mutex_basic_all(
    struct dj_test_suite* _suite
);


/******************************************************************************
 * III-B. RECURSIVE MUTEX OPERATION TESTS
 *****************************************************************************/
bool d_tests_recursive_mutex_init(
    struct dj_test_suite* _suite
);
bool d_tests_recursive_mutex_destroy(
    struct dj_test_suite* _suite
);
bool d_tests_recursive_mutex_lock(
    struct dj_test_suite* _suite
);
bool d_tests_recursive_mutex_trylock(
    struct dj_test_suite* _suite
);
bool d_tests_recursive_mutex_unlock(
    struct dj_test_suite* _suite
);
bool d_tests_recursive_mutex_reentrant(
    struct dj_test_suite* _suite
);

// III-B. aggregation function
bool d_tests_mutex_recursive_all(
    struct dj_test_suite* _suite
);


/******************************************************************************
 * IV. THREAD OPERATION TESTS
 *****************************************************************************/
bool d_tests_thread_create_join(
    struct dj_test_suite* _suite
);
bool d_tests_thread_detach(
    struct dj_test_suite* _suite
);
bool d_tests_thread_current(
    struct dj_test_suite* _suite
);
bool d_tests_thread_equal(
    struct dj_test_suite* _suite
);
bool d_tests_thread_yield(
    struct dj_test_suite* _suite
);
bool d_tests_thread_sleep(
    struct dj_test_suite* _suite
);

// IV.  aggregation function
bool d_tests_thread_ops_all(
    struct dj_test_suite* _suite
);


/******************************************************************************
 * IV-B. THREAD-SPECIFIC STORAGE TESTS
 *****************************************************************************/
bool d_tests_tss_create_delete(
    struct dj_test_suite* _suite
);
bool d_tests_tss_get_set(
    struct dj_test_suite* _suite
);
bool d_tests_tss_per_thread(
    struct dj_test_suite* _suite
);

// IV-B. aggregation function
bool d_tests_tss_all(
    struct dj_test_suite* _suite
);


/******************************************************************************
 * V. CONDITION VARIABLE TESTS
 *****************************************************************************/
bool d_tests_cond_init_destroy(
    struct dj_test_suite* _suite
);
bool d_tests_cond_signal(
    struct dj_test_suite* _suite
);
bool d_tests_cond_broadcast(
    struct dj_test_suite* _suite
);
bool d_tests_cond_wait(
    struct dj_test_suite* _suite
);
bool d_tests_cond_timedwait(
    struct dj_test_suite* _suite
);

// V.   aggregation function
bool d_tests_cond_all(
    struct dj_test_suite* _suite
);


/******************************************************************************
 * VI. READ-WRITE LOCK TESTS
 *****************************************************************************/
bool d_tests_rwlock_init_destroy(
    struct dj_test_suite* _suite
);
bool d_tests_rwlock_rdlock(
    struct dj_test_suite* _suite
);
bool d_tests_rwlock_wrlock(
    struct dj_test_suite* _suite
);
bool d_tests_rwlock_tryrdlock(
    struct dj_test_suite* _suite
);
bool d_tests_rwlock_trywrlock(
    struct dj_test_suite* _suite
);
bool d_tests_rwlock_unlock(
    struct dj_test_suite* _suite
);
bool d_tests_rwlock_timedrdlock(
    struct dj_test_suite* _suite
);
bool d_tests_rwlock_timedwrlock(
    struct dj_test_suite* _suite
);
bool d_tests_rwlock_concurrent_readers(
    struct dj_test_suite* _suite
);

// VI.  aggregation function
bool d_tests_rwlock_all(
    struct dj_test_suite* _suite
);


/******************************************************************************
 * VII. SYNCHRONIZATION PRIMITIVE TESTS
 *****************************************************************************/
bool d_tests_call_once(
    struct dj_test_suite* _suite
);
bool d_tests_call_once_concurrent(
    struct dj_test_suite* _suite
);

// VII. aggregation function
bool d_tests_sync_all(
    struct dj_test_suite* _suite
);


/******************************************************************************
 * IX. UTILITY FUNCTION TESTS
 *****************************************************************************/
bool d_tests_hardware_concurrency(
    struct dj_test_suite* _suite
);

// IX.  aggregation function
bool d_tests_utility_all(
    struct dj_test_suite* _suite
);


/******************************************************************************
 * MODULE-LEVEL AGGREGATION
 *****************************************************************************/
bool d_tests_dmutex_run_all(
    struct dj_test_suite* _suite
);


#endif  // DJINTERP_TESTS_C_SYNC_DMUTEX_TESTS_H
