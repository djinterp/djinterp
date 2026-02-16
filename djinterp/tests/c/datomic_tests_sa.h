/******************************************************************************
* djinterp [test]                                          datomic_tests_sa.h
*
*   Unit test declarations for `datomic.h` module.
*   Provides comprehensive testing of all d_atomic functions including
* atomic flag operations, initialization, load/store operations, exchange
* operations, compare-and-exchange, fetch-and-modify operations, memory
* fences, utility functions, null-safety guards, and explicit memory-order
* variants.
*
*
* path:      \tests\atomic\datomic_tests_sa.h
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.02.06
******************************************************************************/

#ifndef DJINTERP_TESTS_ATOMIC_SA_
#define DJINTERP_TESTS_ATOMIC_SA_ 1

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include "..\..\inc\c\djinterp.h"
#include "..\..\inc\c\string_fn.h"
#include "..\..\inc\c\dtime.h"
#include "..\..\inc\c\test\test_standalone.h"




/******************************************************************************
 * I. ATOMIC FLAG OPERATION TESTS
 *****************************************************************************/
bool d_tests_sa_atomic_flag_test_and_set(struct d_test_counter* _counter);
bool d_tests_sa_atomic_flag_clear(struct d_test_counter* _counter);
bool d_tests_sa_atomic_flag_explicit(struct d_test_counter* _counter);

// I.   aggregation function
bool d_tests_sa_atomic_flag_all(struct d_test_counter* _counter);


/******************************************************************************
 * II. ATOMIC INITIALIZATION TESTS
 *****************************************************************************/
bool d_tests_sa_atomic_init_int(struct d_test_counter* _counter);
bool d_tests_sa_atomic_init_uint(struct d_test_counter* _counter);
bool d_tests_sa_atomic_init_long(struct d_test_counter* _counter);
bool d_tests_sa_atomic_init_ulong(struct d_test_counter* _counter);
bool d_tests_sa_atomic_init_llong(struct d_test_counter* _counter);
bool d_tests_sa_atomic_init_ullong(struct d_test_counter* _counter);
bool d_tests_sa_atomic_init_ptr(struct d_test_counter* _counter);
bool d_tests_sa_atomic_init_size(struct d_test_counter* _counter);

// II.  aggregation function
bool d_tests_sa_atomic_init_all(struct d_test_counter* _counter);


/******************************************************************************
 * III. ATOMIC LOAD OPERATION TESTS
 *****************************************************************************/
bool d_tests_sa_atomic_load_int(struct d_test_counter* _counter);
bool d_tests_sa_atomic_load_uint(struct d_test_counter* _counter);
bool d_tests_sa_atomic_load_long(struct d_test_counter* _counter);
bool d_tests_sa_atomic_load_ulong(struct d_test_counter* _counter);
bool d_tests_sa_atomic_load_llong(struct d_test_counter* _counter);
bool d_tests_sa_atomic_load_ullong(struct d_test_counter* _counter);
bool d_tests_sa_atomic_load_ptr(struct d_test_counter* _counter);
bool d_tests_sa_atomic_load_size(struct d_test_counter* _counter);
bool d_tests_sa_atomic_load_explicit(struct d_test_counter* _counter);

// III. aggregation function
bool d_tests_sa_atomic_load_all(struct d_test_counter* _counter);


/******************************************************************************
 * IV. ATOMIC STORE OPERATION TESTS
 *****************************************************************************/
bool d_tests_sa_atomic_store_int(struct d_test_counter* _counter);
bool d_tests_sa_atomic_store_uint(struct d_test_counter* _counter);
bool d_tests_sa_atomic_store_long(struct d_test_counter* _counter);
bool d_tests_sa_atomic_store_ulong(struct d_test_counter* _counter);
bool d_tests_sa_atomic_store_llong(struct d_test_counter* _counter);
bool d_tests_sa_atomic_store_ullong(struct d_test_counter* _counter);
bool d_tests_sa_atomic_store_ptr(struct d_test_counter* _counter);
bool d_tests_sa_atomic_store_size(struct d_test_counter* _counter);
bool d_tests_sa_atomic_store_explicit(struct d_test_counter* _counter);

// IV.  aggregation function
bool d_tests_sa_atomic_store_all(struct d_test_counter* _counter);


/******************************************************************************
 * V. ATOMIC EXCHANGE OPERATION TESTS
 *****************************************************************************/
bool d_tests_sa_atomic_exchange_int(struct d_test_counter* _counter);
bool d_tests_sa_atomic_exchange_uint(struct d_test_counter* _counter);
bool d_tests_sa_atomic_exchange_long(struct d_test_counter* _counter);
bool d_tests_sa_atomic_exchange_ulong(struct d_test_counter* _counter);
bool d_tests_sa_atomic_exchange_llong(struct d_test_counter* _counter);
bool d_tests_sa_atomic_exchange_ullong(struct d_test_counter* _counter);
bool d_tests_sa_atomic_exchange_ptr(struct d_test_counter* _counter);
bool d_tests_sa_atomic_exchange_size(struct d_test_counter* _counter);
bool d_tests_sa_atomic_exchange_explicit(struct d_test_counter* _counter);

// V.   aggregation function
bool d_tests_sa_atomic_exchange_all(struct d_test_counter* _counter);


/******************************************************************************
 * VI. ATOMIC COMPARE-AND-EXCHANGE TESTS
 *****************************************************************************/
bool d_tests_sa_atomic_compare_exchange_strong_int(struct d_test_counter* _counter);
bool d_tests_sa_atomic_compare_exchange_weak_int(struct d_test_counter* _counter);
bool d_tests_sa_atomic_compare_exchange_strong_uint(struct d_test_counter* _counter);
bool d_tests_sa_atomic_compare_exchange_strong_long(struct d_test_counter* _counter);
bool d_tests_sa_atomic_compare_exchange_strong_llong(struct d_test_counter* _counter);
bool d_tests_sa_atomic_compare_exchange_strong_ptr(struct d_test_counter* _counter);
bool d_tests_sa_atomic_compare_exchange_strong_size(struct d_test_counter* _counter);
bool d_tests_sa_atomic_compare_exchange_explicit(struct d_test_counter* _counter);

// VI.  aggregation function
bool d_tests_sa_atomic_compare_exchange_all(struct d_test_counter* _counter);


/******************************************************************************
 * VII. ATOMIC FETCH-AND-MODIFY TESTS
 *****************************************************************************/
bool d_tests_sa_atomic_fetch_add_int(struct d_test_counter* _counter);
bool d_tests_sa_atomic_fetch_add_uint(struct d_test_counter* _counter);
bool d_tests_sa_atomic_fetch_add_long(struct d_test_counter* _counter);
bool d_tests_sa_atomic_fetch_add_llong(struct d_test_counter* _counter);
bool d_tests_sa_atomic_fetch_add_size(struct d_test_counter* _counter);
bool d_tests_sa_atomic_fetch_sub_int(struct d_test_counter* _counter);
bool d_tests_sa_atomic_fetch_sub_long(struct d_test_counter* _counter);
bool d_tests_sa_atomic_fetch_sub_size(struct d_test_counter* _counter);
bool d_tests_sa_atomic_fetch_or_int(struct d_test_counter* _counter);
bool d_tests_sa_atomic_fetch_xor_int(struct d_test_counter* _counter);
bool d_tests_sa_atomic_fetch_and_int(struct d_test_counter* _counter);
bool d_tests_sa_atomic_fetch_explicit(struct d_test_counter* _counter);

// VII. aggregation function
bool d_tests_sa_atomic_fetch_all(struct d_test_counter* _counter);


/******************************************************************************
 * VIII. MEMORY FENCE AND UTILITY TESTS
 *****************************************************************************/
bool d_tests_sa_atomic_thread_fence(struct d_test_counter* _counter);
bool d_tests_sa_atomic_signal_fence(struct d_test_counter* _counter);
bool d_tests_sa_atomic_is_lock_free(struct d_test_counter* _counter);

// VIII. aggregation function
bool d_tests_sa_atomic_fence_all(struct d_test_counter* _counter);


/******************************************************************************
 * IX. NULL-SAFETY TESTS
 *****************************************************************************/
bool d_tests_sa_atomic_null_load(struct d_test_counter* _counter);
bool d_tests_sa_atomic_null_store(struct d_test_counter* _counter);
bool d_tests_sa_atomic_null_exchange(struct d_test_counter* _counter);
bool d_tests_sa_atomic_null_cas(struct d_test_counter* _counter);
bool d_tests_sa_atomic_null_fetch(struct d_test_counter* _counter);
bool d_tests_sa_atomic_null_flag(struct d_test_counter* _counter);
bool d_tests_sa_atomic_null_ptr(struct d_test_counter* _counter);

// IX.  aggregation function
bool d_tests_sa_atomic_null_all(struct d_test_counter* _counter);


/******************************************************************************
 * MODULE-LEVEL AGGREGATION
 *****************************************************************************/
bool d_tests_sa_atomic_run_all(struct d_test_counter* _counter);


#endif  // DJINTERP_TESTS_ATOMIC_SA_



