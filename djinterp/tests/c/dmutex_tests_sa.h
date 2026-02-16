/******************************************************************************
* djinterp [test]                                            dmutex_tests_sa.h
*
*   Unit test declarations for `dmutex.h` module.
*   Provides comprehensive testing of all dmutex functions including
* basic mutex operations, recursive mutex operations, thread operations,
* thread-specific storage, condition variables, read-write locks,
* synchronization primitives, and utility functions.
*
*
* path:      \tests\dmutex\dmutex_tests_sa.h
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.02.09
******************************************************************************/

#ifndef DJINTERP_TESTS_DMUTEX_STANDALONE_
#define DJINTERP_TESTS_DMUTEX_STANDALONE_ 1

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include "..\..\inc\c\test\test_standalone.h"
#include "..\..\inc\c\dmutex.h"
#include "..\..\inc\c\string_fn.h"
#include "..\..\inc\c\dtime.h"


/******************************************************************************
 * III. BASIC MUTEX OPERATION TESTS
 *****************************************************************************/
bool d_tests_sa_mutex_init(struct d_test_counter* _counter);
bool d_tests_sa_mutex_destroy(struct d_test_counter* _counter);
bool d_tests_sa_mutex_lock(struct d_test_counter* _counter);
bool d_tests_sa_mutex_trylock(struct d_test_counter* _counter);
bool d_tests_sa_mutex_unlock(struct d_test_counter* _counter);
bool d_tests_sa_mutex_timedlock(struct d_test_counter* _counter);

// III. aggregation function
bool d_tests_sa_mutex_basic_all(struct d_test_counter* _counter);


/******************************************************************************
 * III-B. RECURSIVE MUTEX OPERATION TESTS
 *****************************************************************************/
bool d_tests_sa_recursive_mutex_init(struct d_test_counter* _counter);
bool d_tests_sa_recursive_mutex_destroy(struct d_test_counter* _counter);
bool d_tests_sa_recursive_mutex_lock(struct d_test_counter* _counter);
bool d_tests_sa_recursive_mutex_trylock(struct d_test_counter* _counter);
bool d_tests_sa_recursive_mutex_unlock(struct d_test_counter* _counter);
bool d_tests_sa_recursive_mutex_reentrant(struct d_test_counter* _counter);

// III-B. aggregation function
bool d_tests_sa_mutex_recursive_all(struct d_test_counter* _counter);


/******************************************************************************
 * IV. THREAD OPERATION TESTS
 *****************************************************************************/
bool d_tests_sa_thread_create_join(struct d_test_counter* _counter);
bool d_tests_sa_thread_detach(struct d_test_counter* _counter);
bool d_tests_sa_thread_current(struct d_test_counter* _counter);
bool d_tests_sa_thread_equal(struct d_test_counter* _counter);
bool d_tests_sa_thread_yield(struct d_test_counter* _counter);
bool d_tests_sa_thread_sleep(struct d_test_counter* _counter);

// IV.  aggregation function
bool d_tests_sa_thread_ops_all(struct d_test_counter* _counter);


/******************************************************************************
 * IV-B. THREAD-SPECIFIC STORAGE TESTS
 *****************************************************************************/
bool d_tests_sa_tss_create_delete(struct d_test_counter* _counter);
bool d_tests_sa_tss_get_set(struct d_test_counter* _counter);
bool d_tests_sa_tss_per_thread(struct d_test_counter* _counter);

// IV-B. aggregation function
bool d_tests_sa_tss_all(struct d_test_counter* _counter);


/******************************************************************************
 * V. CONDITION VARIABLE TESTS
 *****************************************************************************/
bool d_tests_sa_cond_init_destroy(struct d_test_counter* _counter);
bool d_tests_sa_cond_signal(struct d_test_counter* _counter);
bool d_tests_sa_cond_broadcast(struct d_test_counter* _counter);
bool d_tests_sa_cond_wait(struct d_test_counter* _counter);
bool d_tests_sa_cond_timedwait(struct d_test_counter* _counter);

// V.   aggregation function
bool d_tests_sa_cond_all(struct d_test_counter* _counter);


/******************************************************************************
 * VI. READ-WRITE LOCK TESTS
 *****************************************************************************/
bool d_tests_sa_rwlock_init_destroy(struct d_test_counter* _counter);
bool d_tests_sa_rwlock_rdlock(struct d_test_counter* _counter);
bool d_tests_sa_rwlock_wrlock(struct d_test_counter* _counter);
bool d_tests_sa_rwlock_tryrdlock(struct d_test_counter* _counter);
bool d_tests_sa_rwlock_trywrlock(struct d_test_counter* _counter);
bool d_tests_sa_rwlock_unlock(struct d_test_counter* _counter);
bool d_tests_sa_rwlock_timedrdlock(struct d_test_counter* _counter);
bool d_tests_sa_rwlock_timedwrlock(struct d_test_counter* _counter);
bool d_tests_sa_rwlock_concurrent_readers(struct d_test_counter* _counter);

// VI.  aggregation function
bool d_tests_sa_rwlock_all(struct d_test_counter* _counter);


/******************************************************************************
 * VII. SYNCHRONIZATION PRIMITIVE TESTS
 *****************************************************************************/
bool d_tests_sa_call_once(struct d_test_counter* _counter);
bool d_tests_sa_call_once_concurrent(struct d_test_counter* _counter);

// VII. aggregation function
bool d_tests_sa_sync_all(struct d_test_counter* _counter);


/******************************************************************************
 * IX. UTILITY FUNCTION TESTS
 *****************************************************************************/
bool d_tests_sa_hardware_concurrency(struct d_test_counter* _counter);

// IX.  aggregation function
bool d_tests_sa_utility_all(struct d_test_counter* _counter);


/******************************************************************************
 * MODULE-LEVEL AGGREGATION
 *****************************************************************************/
bool d_tests_sa_dmutex_run_all(struct d_test_counter* _counter);


#endif  // DJINTERP_TESTS_DMUTEX_STANDALONE_





