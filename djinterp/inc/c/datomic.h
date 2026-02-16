/******************************************************************************
* djinterp [core]                                                    datomic.h
*
* Cross-platform atomic operations interface.
*   This header provides a unified interface for atomic operations across
* platforms, wrapping C11 stdatomic.h on supported platforms and providing
* Windows-specific implementations using Interlocked* functions, with a
* GCC __sync_* builtin fallback for pre-C11 compilers.
*   Integer-type functions are generated via the D_ATOMIC_INTEGER_TYPES
* X-macro to eliminate per-type code duplication. Pointer atomics (which
* lack arithmetic/bitwise operations) are declared separately.
*
* path:      \inc\datomic.h
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.02.06
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    PLATFORM DETECTION AND INCLUDES

II.   TYPE DEFINITIONS
      1.  Atomic integer types
      2.  Atomic pointer and flag types
      3.  Memory order

III.  TYPE REGISTRY AND DECLARATION STAMPS

IV.   ATOMIC FLAG OPERATIONS

V.    ATOMIC INITIALIZATION

VI.   ATOMIC LOAD OPERATIONS

VII.  ATOMIC STORE OPERATIONS

VIII. ATOMIC EXCHANGE OPERATIONS

IX.   ATOMIC COMPARE-AND-EXCHANGE

X.    ATOMIC FETCH-AND-MODIFY OPERATIONS

XI.   MEMORY ORDERING AND FENCES
*/

#ifndef DJINTERP_ATOMIC_
#define DJINTERP_ATOMIC_ 1


///////////////////////////////////////////////////////////////////////////////
///             I.    PLATFORM DETECTION AND INCLUDES                       ///
///////////////////////////////////////////////////////////////////////////////

#include <stddef.h>      // for size_t
#include <stdint.h>      // for fixed-width types
#include <stdbool.h>     // for bool
#include ".\djinterp.h"


// D_ATOMIC_USE_STDATOMIC
//   feature: true when C11/C++11 atomics are available.
// Prefers env_c_lib.h detection; falls back to local checks for C++.
#ifndef D_ATOMIC_USE_STDATOMIC
    #if defined(D_ENV_C_HAS_STDATOMIC) && D_ENV_C_HAS_STDATOMIC
        #define D_ATOMIC_USE_STDATOMIC 1
    #elif defined(__cplusplus) && (__cplusplus >= 201103L)
        #define D_ATOMIC_USE_STDATOMIC 1
    #elif D_ENV_LANG_IS_C11_OR_HIGHER &&  \
          !defined(__STDC_NO_ATOMICS__)
        #define D_ATOMIC_USE_STDATOMIC 1
    #else
        #define D_ATOMIC_USE_STDATOMIC 0
    #endif
#endif

// D_ATOMIC_USE_WINDOWS
//   feature: true when we must use Interlocked* functions.
#ifndef D_ATOMIC_USE_WINDOWS
    #if ( (!D_ATOMIC_USE_STDATOMIC) &&                     \
          (defined(_WIN32) || defined(_WIN64)) )
        #define D_ATOMIC_USE_WINDOWS 1
    #else
        #define D_ATOMIC_USE_WINDOWS 0
    #endif
#endif

// D_ATOMIC_USE_SYNC
//   feature: true when we fall back to GCC __sync_* builtins.
#ifndef D_ATOMIC_USE_SYNC
    #if ( (!D_ATOMIC_USE_STDATOMIC) && (!D_ATOMIC_USE_WINDOWS) )
        #define D_ATOMIC_USE_SYNC 1
    #else
        #define D_ATOMIC_USE_SYNC 0
    #endif
#endif


// conditional includes
#if D_ATOMIC_USE_STDATOMIC
    #ifdef __cplusplus
        #include <atomic>
        using std::atomic_int;
        using std::atomic_uint;
        using std::atomic_long;
        using std::atomic_ulong;
        using std::atomic_llong;
        using std::atomic_ullong;
        using std::atomic_size_t;
        using std::atomic_flag;
        using std::memory_order;
        using std::memory_order_relaxed;
        using std::memory_order_consume;
        using std::memory_order_acquire;
        using std::memory_order_release;
        using std::memory_order_acq_rel;
        using std::memory_order_seq_cst;
    #else
        #include <stdatomic.h>
    #endif
#elif D_ATOMIC_USE_WINDOWS
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #include <windows.h>
    #include <intrin.h>
#endif


///////////////////////////////////////////////////////////////////////////////
///             II.   TYPE DEFINITIONS                                      ///
///////////////////////////////////////////////////////////////////////////////

#if D_ATOMIC_USE_STDATOMIC
    typedef atomic_int     d_atomic_int;
    typedef atomic_uint    d_atomic_uint;
    typedef atomic_long    d_atomic_long;
    typedef atomic_ulong   d_atomic_ulong;
    typedef atomic_llong   d_atomic_llong;
    typedef atomic_ullong  d_atomic_ullong;
    typedef atomic_size_t  d_atomic_size_t;
    typedef atomic_flag    d_atomic_flag;

    #define D_ATOMIC_PTR_TYPE(T) _Atomic(T*)
    typedef _Atomic(void*) d_atomic_ptr;

    typedef memory_order d_memory_order;

    #define D_MEMORY_ORDER_RELAXED memory_order_relaxed
    #define D_MEMORY_ORDER_CONSUME memory_order_consume
    #define D_MEMORY_ORDER_ACQUIRE memory_order_acquire
    #define D_MEMORY_ORDER_RELEASE memory_order_release
    #define D_MEMORY_ORDER_ACQ_REL memory_order_acq_rel
    #define D_MEMORY_ORDER_SEQ_CST memory_order_seq_cst

#elif D_ATOMIC_USE_WINDOWS
    typedef volatile long               d_atomic_int;
    typedef volatile unsigned long      d_atomic_uint;
    typedef volatile long               d_atomic_long;
    typedef volatile unsigned long      d_atomic_ulong;
    typedef volatile long long          d_atomic_llong;
    typedef volatile unsigned long long d_atomic_ullong;
    typedef volatile size_t             d_atomic_size_t;
    typedef volatile void*              d_atomic_ptr;

    typedef struct { volatile long value; } d_atomic_flag;

    typedef enum {
        D_MEMORY_ORDER_RELAXED = 0,
        D_MEMORY_ORDER_CONSUME = 1,
        D_MEMORY_ORDER_ACQUIRE = 2,
        D_MEMORY_ORDER_RELEASE = 3,
        D_MEMORY_ORDER_ACQ_REL = 4,
        D_MEMORY_ORDER_SEQ_CST = 5
    } d_memory_order;

#else
    typedef volatile int                d_atomic_int;
    typedef volatile unsigned int       d_atomic_uint;
    typedef volatile long               d_atomic_long;
    typedef volatile unsigned long      d_atomic_ulong;
    typedef volatile long long          d_atomic_llong;
    typedef volatile unsigned long long d_atomic_ullong;
    typedef volatile size_t             d_atomic_size_t;
    typedef volatile void*              d_atomic_ptr;

    typedef struct { volatile int value; } d_atomic_flag;

    typedef enum {
        D_MEMORY_ORDER_RELAXED = 0,
        D_MEMORY_ORDER_CONSUME = 1,
        D_MEMORY_ORDER_ACQUIRE = 2,
        D_MEMORY_ORDER_RELEASE = 3,
        D_MEMORY_ORDER_ACQ_REL = 4,
        D_MEMORY_ORDER_SEQ_CST = 5
    } d_memory_order;

#endif  // D_ATOMIC_USE_STDATOMIC


// D_ATOMIC_VAR_INIT
//   macro: initialize atomic variable at declaration.
#if D_ATOMIC_USE_STDATOMIC && !defined(__cplusplus)
    #ifdef ATOMIC_VAR_INIT
        #define D_ATOMIC_VAR_INIT(value) ATOMIC_VAR_INIT(value)
    #else
        #define D_ATOMIC_VAR_INIT(value) (value)
    #endif
#else
    #define D_ATOMIC_VAR_INIT(value) (value)
#endif

// D_ATOMIC_FLAG_INIT
//   macro: initialize atomic flag to clear state.
#if D_ATOMIC_USE_STDATOMIC
    #define D_ATOMIC_FLAG_INIT ATOMIC_FLAG_INIT
#else
    #define D_ATOMIC_FLAG_INIT {0}
#endif


///////////////////////////////////////////////////////////////////////////////
///             III.  TYPE REGISTRY AND DECLARATION STAMPS                  ///
///////////////////////////////////////////////////////////////////////////////

// D_ATOMIC_INTEGER_TYPES
//   X-macro: invokes X(suffix, value_type, atomic_type, zero_literal)
// for every integer-width atomic type. Pointer atomics are excluded
// because they lack arithmetic and bitwise operations.
#define D_ATOMIC_INTEGER_TYPES(X)                                      \
    X(int,    int,                d_atomic_int,    0)                   \
    X(uint,   unsigned int,       d_atomic_uint,   0U)                 \
    X(long,   long,               d_atomic_long,   0L)                 \
    X(ulong,  unsigned long,      d_atomic_ulong,  0UL)               \
    X(llong,  long long,          d_atomic_llong,  0LL)               \
    X(ullong, unsigned long long, d_atomic_ullong, 0ULL)              \
    X(size,   size_t,             d_atomic_size_t, (size_t)0)


// declaration stamp macros (D_INTERNAL_ — used only within this header)

#define D_INTERNAL_ATOMIC_DECL_INIT(sfx, vtype, atype, zero)           \
    void  d_atomic_init_##sfx(atype* _obj, vtype _value);

#define D_INTERNAL_ATOMIC_DECL_LOAD(sfx, vtype, atype, zero)           \
    vtype d_atomic_load_##sfx(const atype* _obj);                      \
    vtype d_atomic_load_##sfx##_explicit(const atype* _obj,            \
                                         d_memory_order _order);

#define D_INTERNAL_ATOMIC_DECL_STORE(sfx, vtype, atype, zero)          \
    void  d_atomic_store_##sfx(atype* _obj, vtype _value);             \
    void  d_atomic_store_##sfx##_explicit(atype* _obj,                 \
                                          vtype _value,                \
                                          d_memory_order _order);

#define D_INTERNAL_ATOMIC_DECL_EXCHANGE(sfx, vtype, atype, zero)       \
    vtype d_atomic_exchange_##sfx(atype* _obj, vtype _value);          \
    vtype d_atomic_exchange_##sfx##_explicit(atype* _obj,              \
                                             vtype _value,             \
                                             d_memory_order _order);

#define D_INTERNAL_ATOMIC_DECL_CAS(sfx, vtype, atype, zero)            \
    bool  d_atomic_compare_exchange_strong_##sfx(                       \
              atype* _obj, vtype* _expected, vtype _desired);           \
    bool  d_atomic_compare_exchange_weak_##sfx(                         \
              atype* _obj, vtype* _expected, vtype _desired);           \
    bool  d_atomic_compare_exchange_strong_##sfx##_explicit(            \
              atype* _obj, vtype* _expected, vtype _desired,            \
              d_memory_order _success, d_memory_order _failure);        \
    bool  d_atomic_compare_exchange_weak_##sfx##_explicit(              \
              atype* _obj, vtype* _expected, vtype _desired,            \
              d_memory_order _success, d_memory_order _failure);

#define D_INTERNAL_ATOMIC_DECL_FETCH(sfx, vtype, atype, zero)          \
    vtype d_atomic_fetch_add_##sfx(atype* _obj, vtype _arg);           \
    vtype d_atomic_fetch_sub_##sfx(atype* _obj, vtype _arg);           \
    vtype d_atomic_fetch_or_##sfx(atype* _obj, vtype _arg);            \
    vtype d_atomic_fetch_xor_##sfx(atype* _obj, vtype _arg);           \
    vtype d_atomic_fetch_and_##sfx(atype* _obj, vtype _arg);           \
    vtype d_atomic_fetch_add_##sfx##_explicit(atype* _obj,             \
              vtype _arg, d_memory_order _order);                       \
    vtype d_atomic_fetch_sub_##sfx##_explicit(atype* _obj,             \
              vtype _arg, d_memory_order _order);                       \
    vtype d_atomic_fetch_or_##sfx##_explicit(atype* _obj,              \
              vtype _arg, d_memory_order _order);                       \
    vtype d_atomic_fetch_xor_##sfx##_explicit(atype* _obj,             \
              vtype _arg, d_memory_order _order);                       \
    vtype d_atomic_fetch_and_##sfx##_explicit(atype* _obj,             \
              vtype _arg, d_memory_order _order);


///////////////////////////////////////////////////////////////////////////////
///             IV.   ATOMIC FLAG OPERATIONS                                ///
///////////////////////////////////////////////////////////////////////////////

bool               d_atomic_flag_test_and_set(d_atomic_flag* _flag);
bool               d_atomic_flag_test_and_set_explicit(d_atomic_flag* _flag, d_memory_order _order);
void               d_atomic_flag_clear(d_atomic_flag* _flag);
void               d_atomic_flag_clear_explicit(d_atomic_flag* _flag, d_memory_order _order);


///////////////////////////////////////////////////////////////////////////////
///             V.    ATOMIC INITIALIZATION                                 ///
///////////////////////////////////////////////////////////////////////////////

D_ATOMIC_INTEGER_TYPES(D_INTERNAL_ATOMIC_DECL_INIT)
void               d_atomic_init_ptr(d_atomic_ptr* _obj, void* _value);


///////////////////////////////////////////////////////////////////////////////
///             VI.   ATOMIC LOAD OPERATIONS                                ///
///////////////////////////////////////////////////////////////////////////////

D_ATOMIC_INTEGER_TYPES(D_INTERNAL_ATOMIC_DECL_LOAD)
void*              d_atomic_load_ptr(const d_atomic_ptr* _obj);
void*              d_atomic_load_ptr_explicit(const d_atomic_ptr* _obj, d_memory_order _order);


///////////////////////////////////////////////////////////////////////////////
///             VII.  ATOMIC STORE OPERATIONS                               ///
///////////////////////////////////////////////////////////////////////////////

D_ATOMIC_INTEGER_TYPES(D_INTERNAL_ATOMIC_DECL_STORE)
void               d_atomic_store_ptr(d_atomic_ptr* _obj, void* _value);
void               d_atomic_store_ptr_explicit(d_atomic_ptr* _obj, void* _value, d_memory_order _order);


///////////////////////////////////////////////////////////////////////////////
///             VIII. ATOMIC EXCHANGE OPERATIONS                            ///
///////////////////////////////////////////////////////////////////////////////

D_ATOMIC_INTEGER_TYPES(D_INTERNAL_ATOMIC_DECL_EXCHANGE)
void*              d_atomic_exchange_ptr(d_atomic_ptr* _obj, void* _value);
void*              d_atomic_exchange_ptr_explicit(d_atomic_ptr* _obj, void* _value, d_memory_order _order);


///////////////////////////////////////////////////////////////////////////////
///             IX.   ATOMIC COMPARE-AND-EXCHANGE                          ///
///////////////////////////////////////////////////////////////////////////////

D_ATOMIC_INTEGER_TYPES(D_INTERNAL_ATOMIC_DECL_CAS)
bool               d_atomic_compare_exchange_strong_ptr(d_atomic_ptr* _obj, void** _expected, void* _desired);
bool               d_atomic_compare_exchange_weak_ptr(d_atomic_ptr* _obj, void** _expected, void* _desired);
bool               d_atomic_compare_exchange_strong_ptr_explicit(d_atomic_ptr* _obj, void** _expected, void* _desired, d_memory_order _success, d_memory_order _failure);
bool               d_atomic_compare_exchange_weak_ptr_explicit(d_atomic_ptr* _obj, void** _expected, void* _desired, d_memory_order _success, d_memory_order _failure);


///////////////////////////////////////////////////////////////////////////////
///             X.    ATOMIC FETCH-AND-MODIFY OPERATIONS                   ///
///////////////////////////////////////////////////////////////////////////////

D_ATOMIC_INTEGER_TYPES(D_INTERNAL_ATOMIC_DECL_FETCH)


///////////////////////////////////////////////////////////////////////////////
///             XI.   MEMORY ORDERING AND FENCES                           ///
///////////////////////////////////////////////////////////////////////////////

void               d_atomic_thread_fence(d_memory_order _order);
void               d_atomic_signal_fence(d_memory_order _order);
bool               d_atomic_is_lock_free_1(void);
bool               d_atomic_is_lock_free_2(void);
bool               d_atomic_is_lock_free_4(void);
bool               d_atomic_is_lock_free_8(void);


#endif  // DJINTERP_ATOMIC_