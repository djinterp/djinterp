/*******************************************************************************
* djinterp [c][sync]                                             datomic_tests.h
*
*   Unit test declarations for `datomic.h` module.
*   Provides comprehensive testing of all d_atomic functions including
* atomic flag operations, initialization, load/store operations, exchange
* operations, compare-and-exchange, fetch-and-modify operations, memory
* fences, utility functions, null-safety guards, and explicit memory-order
* variants.
*
*
* path:      /tests/djinterp/c/sync/datomic_tests.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.02.06
*                                                            revised: 2026.11.06
*******************************************************************************/

#ifndef DJINTERP_TESTS_C_SYNC_DATOMIC_TESTS_H
#define DJINTERP_TESTS_C_SYNC_DATOMIC_TESTS_H 1

// std
#include <stddef.h>
#include "../../../../inc/re_std/c/cstdint/dstdint.h"  // fixed-width integers
#include <stdio.h>
#include <stdlib.h>
// djinterp
#include "../../../../inc/djinterp/c/djinterp.h"
#include "../../../../inc/djinterp/c/string_fn.h"
#include "../../../../inc/djinterp/c/dtime.h"
#include "../../../../inc/djinterp/c/sync/datomic.h"
#include "../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's C face


/******************************************************************************
 * I. ATOMIC FLAG OPERATION TESTS
 *****************************************************************************/
bool d_tests_datomic_flag_test_and_set(
    struct dj_test_suite* _suite
);

bool d_tests_datomic_flag_clear(
    struct dj_test_suite* _suite
);

bool d_tests_datomic_flag_explicit(
    struct dj_test_suite* _suite
);

// I.   aggregation function
bool d_tests_datomic_flag_all(
    struct dj_test_suite* _suite
);


/******************************************************************************
 * II. ATOMIC INITIALIZATION TESTS
 *****************************************************************************/
bool d_tests_datomic_init_int(
    struct dj_test_suite* _suite
);

bool d_tests_datomic_init_uint(
    struct dj_test_suite* _suite
);

bool d_tests_datomic_init_long(
    struct dj_test_suite* _suite
);

bool d_tests_datomic_init_ulong(
    struct dj_test_suite* _suite
);

bool d_tests_datomic_init_llong(
    struct dj_test_suite* _suite
);

bool d_tests_datomic_init_ullong(
    struct dj_test_suite* _suite
);

bool d_tests_datomic_init_ptr(
    struct dj_test_suite* _suite
);

bool d_tests_datomic_init_size(
    struct dj_test_suite* _suite
);

// II.  aggregation function
bool d_tests_datomic_init_all(
    struct dj_test_suite* _suite
);


/******************************************************************************
 * III. ATOMIC LOAD OPERATION TESTS
 *****************************************************************************/
bool d_tests_datomic_load_int(
    struct dj_test_suite* _suite
);

bool d_tests_datomic_load_uint(
    struct dj_test_suite* _suite
);

bool d_tests_datomic_load_long(
    struct dj_test_suite* _suite
);

bool d_tests_datomic_load_ulong(
    struct dj_test_suite* _suite
);

bool d_tests_datomic_load_llong(
    struct dj_test_suite* _suite
);

bool d_tests_datomic_load_ullong(
    struct dj_test_suite* _suite
);

bool d_tests_datomic_load_ptr(
    struct dj_test_suite* _suite
);

bool d_tests_datomic_load_size(
    struct dj_test_suite* _suite
);

bool d_tests_datomic_load_explicit(
    struct dj_test_suite* _suite
);

// III. aggregation function
bool d_tests_datomic_load_all(
    struct dj_test_suite* _suite
);


/******************************************************************************
 * IV. ATOMIC STORE OPERATION TESTS
 *****************************************************************************/
bool d_tests_datomic_store_int(
    struct dj_test_suite* _suite
);

bool d_tests_datomic_store_uint(
    struct dj_test_suite* _suite
);

bool d_tests_datomic_store_long(
    struct dj_test_suite* _suite
);

bool d_tests_datomic_store_ulong(
    struct dj_test_suite* _suite
);

bool d_tests_datomic_store_llong(
    struct dj_test_suite* _suite
);

bool d_tests_datomic_store_ullong(
    struct dj_test_suite* _suite
);

bool d_tests_datomic_store_ptr(
    struct dj_test_suite* _suite
);

bool d_tests_datomic_store_size(
    struct dj_test_suite* _suite
);

bool d_tests_datomic_store_explicit(
    struct dj_test_suite* _suite
);

// IV.  aggregation function
bool d_tests_datomic_store_all(
    struct dj_test_suite* _suite
);


/******************************************************************************
 * V. ATOMIC EXCHANGE OPERATION TESTS
 *****************************************************************************/
bool d_tests_datomic_exchange_int(
    struct dj_test_suite* _suite
);

bool d_tests_datomic_exchange_uint(
    struct dj_test_suite* _suite
);

bool d_tests_datomic_exchange_long(
    struct dj_test_suite* _suite
);

bool d_tests_datomic_exchange_ulong(
    struct dj_test_suite* _suite
);

bool d_tests_datomic_exchange_llong(
    struct dj_test_suite* _suite
);

bool d_tests_datomic_exchange_ullong(
    struct dj_test_suite* _suite
);

bool d_tests_datomic_exchange_ptr(
    struct dj_test_suite* _suite
);

bool d_tests_datomic_exchange_size(
    struct dj_test_suite* _suite
);

bool d_tests_datomic_exchange_explicit(
    struct dj_test_suite* _suite
);

// V.   aggregation function
bool d_tests_datomic_exchange_all(
    struct dj_test_suite* _suite
);


/******************************************************************************
 * VI. ATOMIC COMPARE-AND-EXCHANGE TESTS
 *****************************************************************************/
bool d_tests_datomic_compare_exchange_strong_int(
    struct dj_test_suite* _suite
);

bool d_tests_datomic_compare_exchange_weak_int(
    struct dj_test_suite* _suite
);

bool d_tests_datomic_compare_exchange_strong_uint(
    struct dj_test_suite* _suite
);

bool d_tests_datomic_compare_exchange_strong_long(
    struct dj_test_suite* _suite
);

bool d_tests_datomic_compare_exchange_strong_llong(
    struct dj_test_suite* _suite
);

bool d_tests_datomic_compare_exchange_strong_ptr(
    struct dj_test_suite* _suite
);

bool d_tests_datomic_compare_exchange_strong_size(
    struct dj_test_suite* _suite
);

bool d_tests_datomic_compare_exchange_explicit(
    struct dj_test_suite* _suite
);

// VI.  aggregation function
bool d_tests_datomic_compare_exchange_all(
    struct dj_test_suite* _suite
);


/******************************************************************************
 * VII. ATOMIC FETCH-AND-MODIFY TESTS
 *****************************************************************************/
bool d_tests_datomic_fetch_add_int(
    struct dj_test_suite* _suite
);

bool d_tests_datomic_fetch_add_uint(
    struct dj_test_suite* _suite
);

bool d_tests_datomic_fetch_add_long(
    struct dj_test_suite* _suite
);

bool d_tests_datomic_fetch_add_llong(
    struct dj_test_suite* _suite
);

bool d_tests_datomic_fetch_add_size(
    struct dj_test_suite* _suite
);

bool d_tests_datomic_fetch_sub_int(
    struct dj_test_suite* _suite
);

bool d_tests_datomic_fetch_sub_long(
    struct dj_test_suite* _suite
);

bool d_tests_datomic_fetch_sub_size(
    struct dj_test_suite* _suite
);

bool d_tests_datomic_fetch_or_int(
    struct dj_test_suite* _suite
);

bool d_tests_datomic_fetch_xor_int(
    struct dj_test_suite* _suite
);

bool d_tests_datomic_fetch_and_int(
    struct dj_test_suite* _suite
);

bool d_tests_datomic_fetch_explicit(
    struct dj_test_suite* _suite
);

// VII. aggregation function
bool d_tests_datomic_fetch_all(
    struct dj_test_suite* _suite
);


/******************************************************************************
 * VIII. MEMORY FENCE AND UTILITY TESTS
 *****************************************************************************/
bool d_tests_datomic_thread_fence(
    struct dj_test_suite* _suite
);

bool d_tests_datomic_signal_fence(
    struct dj_test_suite* _suite
);

bool d_tests_datomic_is_lock_free(
    struct dj_test_suite* _suite
);

// VIII. aggregation function
bool d_tests_datomic_fence_all(
    struct dj_test_suite* _suite
);


/******************************************************************************
 * IX. NULL-SAFETY TESTS
 *****************************************************************************/
bool d_tests_datomic_null_load(
    struct dj_test_suite* _suite
);

bool d_tests_datomic_null_store(
    struct dj_test_suite* _suite
);

bool d_tests_datomic_null_exchange(
    struct dj_test_suite* _suite
);

bool d_tests_datomic_null_cas(
    struct dj_test_suite* _suite
);

bool d_tests_datomic_null_fetch(
    struct dj_test_suite* _suite
);

bool d_tests_datomic_null_flag(
    struct dj_test_suite* _suite
);

bool d_tests_datomic_null_ptr(
    struct dj_test_suite* _suite
);

// IX.  aggregation function
bool d_tests_datomic_null_all(
    struct dj_test_suite* _suite
);


/******************************************************************************
 * MODULE-LEVEL AGGREGATION
 *****************************************************************************/
bool d_tests_datomic_run_all(
    struct dj_test_suite* _suite
);


#endif  // DJINTERP_TESTS_ATOMIC_SA_
