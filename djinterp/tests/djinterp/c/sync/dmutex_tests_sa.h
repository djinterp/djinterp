/*******************************************************************************
* djinterp [test]                                              dmutex_tests_sa.h
*
*   Unit test declarations for `dmutex.h` module.
*   Provides comprehensive testing of all dmutex functions including
* basic mutex operations, recursive mutex operations, thread operations,
* thread-specific storage, condition variables, read-write locks,
* synchronization primitives, and utility functions.
*
*
* path:      /tests/djinterp/c/sync/dmutex_tests_sa.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.02.09
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef DJINTERP_TESTS_DMUTEX_STANDALONE_
#define DJINTERP_TESTS_DMUTEX_STANDALONE_ 1

// std
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
// djinterp
#include "../../../../inc/djinterp/c/sync/dmutex.h"
#include "../../../../inc/djinterp/c/string_fn.h"
#include "../../../../inc/djinterp/c/dtime.h"
#include "../../../../inc/djinterp/c/modules/test/test_counter.h"


/******************************************************************************
 * III. BASIC MUTEX OPERATION TESTS
 *****************************************************************************/
bool d_tests_sa_mutex_init(
    struct dj_test_counter* _counter
);
bool d_tests_sa_mutex_destroy(
    struct dj_test_counter* _counter
);
bool d_tests_sa_mutex_lock(
    struct dj_test_counter* _counter
);
bool d_tests_sa_mutex_trylock(
    struct dj_test_counter* _counter
);
bool d_tests_sa_mutex_unlock(
    struct dj_test_counter* _counter
);
bool d_tests_sa_mutex_timedlock(
    struct dj_test_counter* _counter
);

// III. aggregation function
bool d_tests_sa_mutex_basic_all(
    struct dj_test_counter* _counter
);


/******************************************************************************
 * III-B. RECURSIVE MUTEX OPERATION TESTS
 *****************************************************************************/
bool d_tests_sa_recursive_mutex_init(
    struct dj_test_counter* _counter
);
bool d_tests_sa_recursive_mutex_destroy(
    struct dj_test_counter* _counter
);
bool d_tests_sa_recursive_mutex_lock(
    struct dj_test_counter* _counter
);
bool d_tests_sa_recursive_mutex_trylock(
    struct dj_test_counter* _counter
);
bool d_tests_sa_recursive_mutex_unlock(
    struct dj_test_counter* _counter
);
bool d_tests_sa_recursive_mutex_reentrant(
    struct dj_test_counter* _counter
);

// III-B. aggregation function
bool d_tests_sa_mutex_recursive_all(
    struct dj_test_counter* _counter
);


/******************************************************************************
 * IV. THREAD OPERATION TESTS
 *****************************************************************************/
bool d_tests_sa_thread_create_join(
    struct dj_test_counter* _counter
);
bool d_tests_sa_thread_detach(
    struct dj_test_counter* _counter
);
bool d_tests_sa_thread_current(
    struct dj_test_counter* _counter
);
bool d_tests_sa_thread_equal(
    struct dj_test_counter* _counter
);
bool d_tests_sa_thread_yield(
    struct dj_test_counter* _counter
);
bool d_tests_sa_thread_sleep(
    struct dj_test_counter* _counter
);

// IV.  aggregation function
bool d_tests_sa_thread_ops_all(
    struct dj_test_counter* _counter
);


/******************************************************************************
 * IV-B. THREAD-SPECIFIC STORAGE TESTS
 *****************************************************************************/
bool d_tests_sa_tss_create_delete(
    struct dj_test_counter* _counter
);
bool d_tests_sa_tss_get_set(
    struct dj_test_counter* _counter
);
bool d_tests_sa_tss_per_thread(
    struct dj_test_counter* _counter
);

// IV-B. aggregation function
bool d_tests_sa_tss_all(
    struct dj_test_counter* _counter
);


/******************************************************************************
 * V. CONDITION VARIABLE TESTS
 *****************************************************************************/
bool d_tests_sa_cond_init_destroy(
    struct dj_test_counter* _counter
);
bool d_tests_sa_cond_signal(
    struct dj_test_counter* _counter
);
bool d_tests_sa_cond_broadcast(
    struct dj_test_counter* _counter
);
bool d_tests_sa_cond_wait(
    struct dj_test_counter* _counter
);
bool d_tests_sa_cond_timedwait(
    struct dj_test_counter* _counter
);

// V.   aggregation function
bool d_tests_sa_cond_all(
    struct dj_test_counter* _counter
);


/******************************************************************************
 * VI. READ-WRITE LOCK TESTS
 *****************************************************************************/
bool d_tests_sa_rwlock_init_destroy(
    struct dj_test_counter* _counter
);
bool d_tests_sa_rwlock_rdlock(
    struct dj_test_counter* _counter
);
bool d_tests_sa_rwlock_wrlock(
    struct dj_test_counter* _counter
);
bool d_tests_sa_rwlock_tryrdlock(
    struct dj_test_counter* _counter
);
bool d_tests_sa_rwlock_trywrlock(
    struct dj_test_counter* _counter
);
bool d_tests_sa_rwlock_unlock(
    struct dj_test_counter* _counter
);
bool d_tests_sa_rwlock_timedrdlock(
    struct dj_test_counter* _counter
);
bool d_tests_sa_rwlock_timedwrlock(
    struct dj_test_counter* _counter
);
bool d_tests_sa_rwlock_concurrent_readers(
    struct dj_test_counter* _counter
);

// VI.  aggregation function
bool d_tests_sa_rwlock_all(
    struct dj_test_counter* _counter
);


/******************************************************************************
 * VII. SYNCHRONIZATION PRIMITIVE TESTS
 *****************************************************************************/
bool d_tests_sa_call_once(
    struct dj_test_counter* _counter
);
bool d_tests_sa_call_once_concurrent(
    struct dj_test_counter* _counter
);

// VII. aggregation function
bool d_tests_sa_sync_all(
    struct dj_test_counter* _counter
);


/******************************************************************************
 * IX. UTILITY FUNCTION TESTS
 *****************************************************************************/
bool d_tests_sa_hardware_concurrency(
    struct dj_test_counter* _counter
);

// IX.  aggregation function
bool d_tests_sa_utility_all(
    struct dj_test_counter* _counter
);


/******************************************************************************
 * MODULE-LEVEL AGGREGATION
 *****************************************************************************/
bool d_tests_sa_dmutex_run_all(
    struct dj_test_counter* _counter
);


#endif  // DJINTERP_TESTS_DMUTEX_STANDALONE_
