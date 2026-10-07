/*******************************************************************************
* djinterp [c]                                                          dmutex.h
*
* Cross-platform mutex and threading interface.
*   This header provides a unified interface for mutex operations and basic
* threading across platforms, wrapping C11 threads.h on supported platforms,
* pthreads on POSIX systems, and Windows threading APIs on Windows.
*   The interface supports recursive mutexes, timed operations, condition
* variables, and thread-local storage. Native implementations are used
* whenever available, with fallback support for older systems.
*   Where the C library hides POSIX in a strict ISO mode, every file that
* includes this header builds with -D_XOPEN_SOURCE=700 (the C guide's rule):
* glibc declares pthread_rwlock_t only then.
*
*
* path:      /inc/djinterp/c/sync/dmutex.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.02.06
*                                                            revised: 2026.10.04
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    PLATFORM DETECTION AND INCLUDES
      -------------------------------
      1.    Standard headers
      2.    Feature detection macros
      3.    Platform-specific headers

II.   TYPE DEFINITIONS
      ----------------
      1.    Mutex types            (d_mutex_t, d_recursive_mutex_t)
      2.    Thread types           (d_thread_t, d_thread_func_t)
      3.    Condition variable     (d_cond_t)
      4.    Thread-local storage   (d_tss_t, d_tss_dtor_t)
      5.    Once flag              (d_once_flag_t, D_ONCE_FLAG_INIT)
      6.    Read-write lock        (d_rwlock_t)
      7.    Result types           (d_mutex_result_t, d_thread_result_t)

III.  MUTEX OPERATIONS
      ----------------
      1.    Basic mutex operations
      2.    Timed mutex operations
      3.    Recursive mutex operations

IV.   THREAD OPERATIONS
      -----------------
      1.    Thread creation and management
      2.    Thread identification
      3.    Thread yielding and sleeping
      4.    Thread-local storage

V.    CONDITION VARIABLES
      -------------------
      1.    Signal and broadcast
      2.    Wait and timed wait

VI.   READ-WRITE LOCKS
      ----------------
      1.    Read-write lock operations
      2.    Timed read-write operations

VII.  SYNCHRONIZATION PRIMITIVES
      --------------------------
      1.    Once-only initialization

VIII. UTILITY FUNCTIONS
      -----------------
      1.    Hardware concurrency
*/

#ifndef DJINTERP_C_SYNC_DMUTEX_H
#define DJINTERP_C_SYNC_DMUTEX_H 1


// I.    Platform detection and includes

// std
#include <stddef.h>      // for size_t
// djinterp
#include "../djinterp.h"
#include "../dtime.h"

D_EXTERN_C_BEGIN


// D_MUTEX_HAS_C11_THREADS
//   feature: 1 when C11 <threads.h> is usable..h
#ifndef D_MUTEX_HAS_C11_THREADS
    #if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L)
        #if !defined(__STDC_NO_THREADS__)
            #define D_MUTEX_HAS_C11_THREADS 1
        #else
            #define D_MUTEX_HAS_C11_THREADS 0
        #endif
    #else
        #define D_MUTEX_HAS_C11_THREADS 0
    #endif
#endif

// D_MUTEX_HAS_PTHREAD
//   feature: 1 when POSIX threads are available.
#ifndef D_MUTEX_HAS_PTHREAD
    #if defined(__unix__) || defined(__unix) ||     \
        defined(__linux__) || defined(__APPLE__) || \
        defined(__MACH__) || defined(__FreeBSD__) || \
        defined(__OpenBSD__) || defined(__NetBSD__)
        #define D_MUTEX_HAS_PTHREAD 1
    #else
        #define D_MUTEX_HAS_PTHREAD 0
    #endif
#endif

// D_MUTEX_HAS_WINDOWS_THREADS
//   feature: 1 when the Windows threading API is available.
#ifndef D_MUTEX_HAS_WINDOWS_THREADS
    #if defined(_WIN32) || defined(_WIN64) || defined(__CYGWIN__)
        #define D_MUTEX_HAS_WINDOWS_THREADS 1
    #else
        #define D_MUTEX_HAS_WINDOWS_THREADS 0
    #endif
#endif


// platform headers, selected by the detection above
#if D_MUTEX_HAS_C11_THREADS
    // std
    #include <threads.h>
#endif

#if D_MUTEX_HAS_PTHREAD
    #include <pthread.h>
    #include <errno.h>
    #include <unistd.h>
    #include <sys/time.h>
#endif

#if D_MUTEX_HAS_WINDOWS_THREADS
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    // windows
    #include <windows.h>
    #include <process.h>

    // D_MUTEX_HAS_WINDOWS_SRWLOCK / _CONDITION_VARIABLE
    //   feature: 1 when the Vista+ synchronization API is present.
    #if defined(_WIN32_WINNT) && (_WIN32_WINNT >= 0x0600)
        #define D_MUTEX_HAS_WINDOWS_SRWLOCK 1
        #define D_MUTEX_HAS_WINDOWS_CONDITION_VARIABLE 1
    #else
        #define D_MUTEX_HAS_WINDOWS_SRWLOCK 0
        #define D_MUTEX_HAS_WINDOWS_CONDITION_VARIABLE 0
    #endif
#endif


// II.   Type definitions

// d_mutex_t
//   type: basic mutex type
#if D_MUTEX_HAS_C11_THREADS
    typedef mtx_t d_mutex_t;
#elif D_MUTEX_HAS_PTHREAD
    typedef pthread_mutex_t d_mutex_t;
#elif D_MUTEX_HAS_WINDOWS_THREADS
    typedef struct d_mutex_t
    {
        HANDLE handle;
        bool   is_recursive;
    } d_mutex_t;
#else
    #error "No mutex implementation available for this platform"
#endif

// d_recursive_mutex_t
//   type: recursive mutex type (can be locked multiple times by same thread)
#if D_MUTEX_HAS_C11_THREADS
    typedef mtx_t d_recursive_mutex_t;
#elif D_MUTEX_HAS_PTHREAD
    typedef pthread_mutex_t d_recursive_mutex_t;
#elif D_MUTEX_HAS_WINDOWS_THREADS
    typedef CRITICAL_SECTION d_recursive_mutex_t;
#else
    typedef d_mutex_t d_recursive_mutex_t;
#endif

// d_thread_t
//   type: thread identifier
#if D_MUTEX_HAS_C11_THREADS
    typedef thrd_t d_thread_t;
#elif D_MUTEX_HAS_PTHREAD
    typedef pthread_t d_thread_t;
#elif D_MUTEX_HAS_WINDOWS_THREADS
    typedef struct d_thread_t
    {
        HANDLE handle;
        DWORD  id;
    } d_thread_t;
#else
    typedef unsigned long d_thread_t;
#endif

// d_thread_func_t
//   type: thread start function signature
#if D_MUTEX_HAS_C11_THREADS
    typedef int (*d_thread_func_t)(void*);
#else
    typedef void* (*d_thread_func_t)(void*);
#endif

// d_cond_t
//   type: condition variable
#if D_MUTEX_HAS_C11_THREADS
    typedef cnd_t d_cond_t;
#elif D_MUTEX_HAS_PTHREAD
    typedef pthread_cond_t d_cond_t;
#elif D_MUTEX_HAS_WINDOWS_THREADS && D_MUTEX_HAS_WINDOWS_CONDITION_VARIABLE
    typedef CONDITION_VARIABLE d_cond_t;
#elif D_MUTEX_HAS_WINDOWS_THREADS
    typedef struct d_cond_t
    {
        HANDLE event;
        int    waiters_count;
        CRITICAL_SECTION waiters_count_lock;
    } d_cond_t;
#else
    typedef struct d_cond_t
    {
        int dummy;
    } d_cond_t;
#endif

// d_tss_t
//   type: thread-specific storage key
#if D_MUTEX_HAS_C11_THREADS
    typedef tss_t d_tss_t;
#elif D_MUTEX_HAS_PTHREAD
    typedef pthread_key_t d_tss_t;
#elif D_MUTEX_HAS_WINDOWS_THREADS
    typedef DWORD d_tss_t;
#else
    typedef int d_tss_t;
#endif

// d_tss_dtor_t
//   type: thread-specific storage destructor function
typedef void (*d_tss_dtor_t)(void*);

// d_once_flag_t
//   type: one-time initialization flag
#if D_MUTEX_HAS_C11_THREADS
    typedef once_flag d_once_flag_t;

    #define D_ONCE_FLAG_INIT ONCE_FLAG_INIT
#elif D_MUTEX_HAS_PTHREAD
    typedef pthread_once_t d_once_flag_t;

    #define D_ONCE_FLAG_INIT PTHREAD_ONCE_INIT
#elif D_MUTEX_HAS_WINDOWS_THREADS
    typedef INIT_ONCE d_once_flag_t;

    #define D_ONCE_FLAG_INIT INIT_ONCE_STATIC_INIT
#else
    typedef struct d_once_flag_t
    {
        volatile int done;
    } d_once_flag_t;

    #define D_ONCE_FLAG_INIT {0}
#endif

// d_rwlock_t
//   type: read-write lock. Many readers or one writer.
//   Note: the operations below take `struct d_rwlock_t*` rather than the
// typedef, so the tag name is part of the public surface here.
// D_RWLOCK_INIT
//   macro: static initializer for a d_rwlock_t.
//   Note: only the Windows SRWLOCK backend can be statically initialized.
// On the other two, d_rwlock_init must be called, so D_RWLOCK_INIT is
// deliberately NOT defined there -- a portable caller uses d_rwlock_init.
#if D_MUTEX_HAS_PTHREAD
    typedef struct d_rwlock_t
    {
        pthread_rwlock_t rwlock;
    } d_rwlock_t;

#elif D_MUTEX_HAS_WINDOWS_THREADS && D_MUTEX_HAS_WINDOWS_SRWLOCK
    typedef struct d_rwlock_t
    {
        SRWLOCK srwlock;
    } d_rwlock_t;

    #define D_RWLOCK_INIT { SRWLOCK_INIT }
#else
    typedef struct d_rwlock_t
    {
        d_mutex_t mutex;
        d_cond_t  readers_cond;
        d_cond_t  writers_cond;
        int       readers;
        int       writers;
        int       waiting_writers;
    } d_rwlock_t;
#endif

// d_mutex_result_t
//   type: the return code shared by the mutex, rwlock and spinlock calls.
// 0 is success; every failure is negative, so `< 0` is a valid error test.
typedef enum {
    D_MUTEX_SUCCESS      = 0,
    D_MUTEX_ERROR        = -1,
    D_MUTEX_BUSY         = -2,
    D_MUTEX_TIMEDOUT     = -3,
    D_MUTEX_NOMEM        = -4
} d_mutex_result_t;

// d_thread_result_t
//   type: what a thread function returns and what d_thread_join writes back.
// C11 threads use int; pthreads and the Windows shim use void*, so this is
// one of the few places where the backend is visible in a signature.

// D_THREAD_SUCCESS / D_THREAD_ERROR
//   constant: the two portable values for a d_thread_result_t, spelled so a
// caller never has to know which backend is in play.
#if D_MUTEX_HAS_C11_THREADS
    typedef int d_thread_result_t;

    #define D_THREAD_SUCCESS 0
    #define D_THREAD_ERROR   -1
#else
    typedef void* d_thread_result_t;

    #define D_THREAD_SUCCESS ((void*)0)
    #define D_THREAD_ERROR   ((void*)-1)
#endif


// III.  Mutex operations
int        d_mutex_init(d_mutex_t* _mutex);
int        d_mutex_destroy(d_mutex_t* _mutex);
int        d_mutex_lock(d_mutex_t* _mutex);
int        d_mutex_trylock(d_mutex_t* _mutex);
int        d_mutex_unlock(d_mutex_t* _mutex);
int        d_mutex_timedlock(d_mutex_t*             _mutex,
                             const struct timespec* _timeout);
int        d_recursive_mutex_init(d_recursive_mutex_t* _mutex);
int        d_recursive_mutex_destroy(d_recursive_mutex_t* _mutex);
int        d_recursive_mutex_lock(d_recursive_mutex_t* _mutex);
int        d_recursive_mutex_trylock(d_recursive_mutex_t* _mutex);
int        d_recursive_mutex_unlock(d_recursive_mutex_t* _mutex);

// IV.   Thread operations
int        d_thread_create(d_thread_t*     _thread,
                           d_thread_func_t _func,
                           void*           _arg);
int        d_thread_join(d_thread_t         _thread,
                         d_thread_result_t* _result);
int        d_thread_detach(d_thread_t _thread);
void       d_thread_exit(d_thread_result_t _result);
void       d_thread_yield(void);
int        d_thread_sleep(const struct timespec* _duration,
                          struct timespec*       _remaining);
d_thread_t d_thread_current(void);
int        d_thread_equal(d_thread_t _t1,
                          d_thread_t _t2);
int        d_tss_create(d_tss_t*     _key,
                        d_tss_dtor_t _dtor);
int        d_tss_delete(d_tss_t _key);
void*      d_tss_get(d_tss_t _key);
int        d_tss_set(d_tss_t _key,
                     void*   _value);
// V.    Condition variables
int        d_cond_init(d_cond_t* _cond);
int        d_cond_destroy(d_cond_t* _cond);
int        d_cond_signal(d_cond_t* _cond);
int        d_cond_broadcast(d_cond_t* _cond);
int        d_cond_wait(d_cond_t*  _cond,
                       d_mutex_t* _mutex);
int        d_cond_timedwait(d_cond_t*              _cond,
                            d_mutex_t*             _mutex,
                            const struct timespec* _timeout);
// VI.   Read-write locks
int        d_rwlock_init(struct d_rwlock_t* _rwlock);
int        d_rwlock_destroy(struct d_rwlock_t* _rwlock);
int        d_rwlock_rdlock(struct d_rwlock_t* _rwlock);
int        d_rwlock_tryrdlock(struct d_rwlock_t* _rwlock);
int        d_rwlock_wrlock(struct d_rwlock_t* _rwlock);
int        d_rwlock_trywrlock(struct d_rwlock_t* _rwlock);
int        d_rwlock_unlock(struct d_rwlock_t* _rwlock);
int        d_rwlock_timedrdlock(struct d_rwlock_t*     _rwlock,
                                const struct timespec* _timeout);
int        d_rwlock_timedwrlock(struct d_rwlock_t*     _rwlock,
                                const struct timespec* _timeout);
// VII.  Synchronization primitives
void       d_call_once(d_once_flag_t* _flag, void (*_func)(void));

// VIII. Utility functions
int        d_thread_hardware_concurrency(void);

D_EXTERN_C_END

#endif  // DJINTERP_C_SYNC_DMUTEX_H
