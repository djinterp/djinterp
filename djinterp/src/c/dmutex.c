/******************************************************************************
* djinterp [core]                                                     dmutex.c
*
* Implementation of cross-platform mutex and threading operations.
*
* path:      \src\dmutex.c
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.02.06
******************************************************************************/
#include "..\..\inc\c\dmutex.h"


///////////////////////////////////////////////////////////////////////////////
///             III.  MUTEX OPERATIONS                                      ///
///////////////////////////////////////////////////////////////////////////////

#if D_MUTEX_HAS_C11_THREADS

int
d_mutex_init
(
    d_mutex_t* _mutex
)
{
    int result = mtx_init(_mutex, mtx_plain);
    return (result == thrd_success) ? D_MUTEX_SUCCESS : D_MUTEX_ERROR;
}

int
d_mutex_destroy
(
    d_mutex_t* _mutex
)
{
    mtx_destroy(_mutex);
    return D_MUTEX_SUCCESS;
}

int
d_mutex_lock
(
    d_mutex_t* _mutex
)
{
    int result = mtx_lock(_mutex);
    return (result == thrd_success) ? D_MUTEX_SUCCESS : D_MUTEX_ERROR;
}

int
d_mutex_trylock
(
    d_mutex_t* _mutex
)
{
    int result = mtx_trylock(_mutex);
    if (result == thrd_success)
        return D_MUTEX_SUCCESS;
    else if (result == thrd_busy)
        return D_MUTEX_BUSY;
    else
        return D_MUTEX_ERROR;
}

int
d_mutex_unlock
(
    d_mutex_t* _mutex
)
{
    int result = mtx_unlock(_mutex);
    return (result == thrd_success) ? D_MUTEX_SUCCESS : D_MUTEX_ERROR;
}

int
d_mutex_timedlock
(
    d_mutex_t*              _mutex,
    const struct timespec* _timeout
)
{
    int result = mtx_timedlock(_mutex, _timeout);
    if (result == thrd_success)
        return D_MUTEX_SUCCESS;
    else if (result == thrd_timedout)
        return D_MUTEX_TIMEDOUT;
    else
        return D_MUTEX_ERROR;
}

#elif D_MUTEX_HAS_PTHREAD

int
d_mutex_init
(
    d_mutex_t* _mutex
)
{
    int result = pthread_mutex_init(_mutex, NULL);
    return (result == 0) ? D_MUTEX_SUCCESS : D_MUTEX_ERROR;
}

int
d_mutex_destroy
(
    d_mutex_t* _mutex
)
{
    int result = pthread_mutex_destroy(_mutex);
    return (result == 0) ? D_MUTEX_SUCCESS : D_MUTEX_ERROR;
}

int
d_mutex_lock
(
    d_mutex_t* _mutex
)
{
    int result = pthread_mutex_lock(_mutex);
    return (result == 0) ? D_MUTEX_SUCCESS : D_MUTEX_ERROR;
}

int
d_mutex_trylock
(
    d_mutex_t* _mutex
)
{
    int result = pthread_mutex_trylock(_mutex);
    if (result == 0)
        return D_MUTEX_SUCCESS;
    else if (result == EBUSY)
        return D_MUTEX_BUSY;
    else
        return D_MUTEX_ERROR;
}

int
d_mutex_unlock
(
    d_mutex_t* _mutex
)
{
    int result = pthread_mutex_unlock(_mutex);
    return (result == 0) ? D_MUTEX_SUCCESS : D_MUTEX_ERROR;
}

int
d_mutex_timedlock
(
    d_mutex_t*              _mutex,
    const struct timespec* _timeout
)
{
    #ifdef _POSIX_TIMEOUTS
        int result = pthread_mutex_timedlock(_mutex, _timeout);
        if (result == 0)
            return D_MUTEX_SUCCESS;
        else if (result == ETIMEDOUT)
            return D_MUTEX_TIMEDOUT;
        else
            return D_MUTEX_ERROR;
    #else
        // Fallback: busy-wait with timeout
        struct timespec now;
        struct timespec end = *_timeout;
        
        while (1)
        {
            if (pthread_mutex_trylock(_mutex) == 0)
                return D_MUTEX_SUCCESS;
            
            clock_gettime(CLOCK_REALTIME, &now);
            if (now.tv_sec > end.tv_sec ||
                (now.tv_sec == end.tv_sec && now.tv_nsec >= end.tv_nsec))
                return D_MUTEX_TIMEDOUT;
            
            // Sleep briefly to avoid busy-waiting
            struct timespec sleep_time = {0, 1000000}; // 1ms
            nanosleep(&sleep_time, NULL);
        }
    #endif
}

#elif D_MUTEX_HAS_WINDOWS_THREADS

int
d_mutex_init
(
    d_mutex_t* _mutex
)
{
    _mutex->handle = CreateMutex(NULL, FALSE, NULL);
    _mutex->is_recursive = false;
    return (_mutex->handle != NULL) ? D_MUTEX_SUCCESS : D_MUTEX_ERROR;
}

int
d_mutex_destroy
(
    d_mutex_t* _mutex
)
{
    BOOL result = CloseHandle(_mutex->handle);
    return result ? D_MUTEX_SUCCESS : D_MUTEX_ERROR;
}

int
d_mutex_lock
(
    d_mutex_t* _mutex
)
{
    DWORD result = WaitForSingleObject(_mutex->handle, INFINITE);
    return (result == WAIT_OBJECT_0) ? D_MUTEX_SUCCESS : D_MUTEX_ERROR;
}

int
d_mutex_trylock
(
    d_mutex_t* _mutex
)
{
    DWORD result = WaitForSingleObject(_mutex->handle, 0);
    if (result == WAIT_OBJECT_0)
        return D_MUTEX_SUCCESS;
    else if (result == WAIT_TIMEOUT)
        return D_MUTEX_BUSY;
    else
        return D_MUTEX_ERROR;
}

int
d_mutex_unlock
(
    d_mutex_t* _mutex
)
{
    BOOL result = ReleaseMutex(_mutex->handle);
    return result ? D_MUTEX_SUCCESS : D_MUTEX_ERROR;
}

int
d_mutex_timedlock
(
    d_mutex_t*              _mutex,
    const struct timespec* _timeout
)
{
    // Convert timespec to milliseconds
    DWORD timeout_ms = (DWORD)(_timeout->tv_sec * 1000 + 
                                _timeout->tv_nsec / 1000000);
    
    DWORD result = WaitForSingleObject(_mutex->handle, timeout_ms);
    if (result == WAIT_OBJECT_0)
        return D_MUTEX_SUCCESS;
    else if (result == WAIT_TIMEOUT)
        return D_MUTEX_TIMEDOUT;
    else
        return D_MUTEX_ERROR;
}

#endif


// Recursive mutex operations
#if D_MUTEX_HAS_C11_THREADS

int
d_recursive_mutex_init
(
    d_recursive_mutex_t* _mutex
)
{
    int result = mtx_init(_mutex, mtx_recursive);
    return (result == thrd_success) ? D_MUTEX_SUCCESS : D_MUTEX_ERROR;
}

int
d_recursive_mutex_destroy
(
    d_recursive_mutex_t* _mutex
)
{
    mtx_destroy(_mutex);
    return D_MUTEX_SUCCESS;
}

int
d_recursive_mutex_lock
(
    d_recursive_mutex_t* _mutex
)
{
    int result = mtx_lock(_mutex);
    return (result == thrd_success) ? D_MUTEX_SUCCESS : D_MUTEX_ERROR;
}

int
d_recursive_mutex_trylock
(
    d_recursive_mutex_t* _mutex
)
{
    int result = mtx_trylock(_mutex);
    if (result == thrd_success)
        return D_MUTEX_SUCCESS;
    else if (result == thrd_busy)
        return D_MUTEX_BUSY;
    else
        return D_MUTEX_ERROR;
}

int
d_recursive_mutex_unlock
(
    d_recursive_mutex_t* _mutex
)
{
    int result = mtx_unlock(_mutex);
    return (result == thrd_success) ? D_MUTEX_SUCCESS : D_MUTEX_ERROR;
}

#elif D_MUTEX_HAS_PTHREAD

int
d_recursive_mutex_init
(
    d_recursive_mutex_t* _mutex
)
{
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    int result = pthread_mutex_init(_mutex, &attr);
    pthread_mutexattr_destroy(&attr);
    return (result == 0) ? D_MUTEX_SUCCESS : D_MUTEX_ERROR;
}

int
d_recursive_mutex_destroy
(
    d_recursive_mutex_t* _mutex
)
{
    int result = pthread_mutex_destroy(_mutex);
    return (result == 0) ? D_MUTEX_SUCCESS : D_MUTEX_ERROR;
}

int
d_recursive_mutex_lock
(
    d_recursive_mutex_t* _mutex
)
{
    int result = pthread_mutex_lock(_mutex);
    return (result == 0) ? D_MUTEX_SUCCESS : D_MUTEX_ERROR;
}

int
d_recursive_mutex_trylock
(
    d_recursive_mutex_t* _mutex
)
{
    int result = pthread_mutex_trylock(_mutex);
    if (result == 0)
        return D_MUTEX_SUCCESS;
    else if (result == EBUSY)
        return D_MUTEX_BUSY;
    else
        return D_MUTEX_ERROR;
}

int
d_recursive_mutex_unlock
(
    d_recursive_mutex_t* _mutex
)
{
    int result = pthread_mutex_unlock(_mutex);
    return (result == 0) ? D_MUTEX_SUCCESS : D_MUTEX_ERROR;
}

#elif D_MUTEX_HAS_WINDOWS_THREADS

int
d_recursive_mutex_init
(
    d_recursive_mutex_t* _mutex
)
{
    InitializeCriticalSection(_mutex);
    return D_MUTEX_SUCCESS;
}

int
d_recursive_mutex_destroy
(
    d_recursive_mutex_t* _mutex
)
{
    DeleteCriticalSection(_mutex);
    return D_MUTEX_SUCCESS;
}

int
d_recursive_mutex_lock
(
    d_recursive_mutex_t* _mutex
)
{
    EnterCriticalSection(_mutex);
    return D_MUTEX_SUCCESS;
}

int
d_recursive_mutex_trylock
(
    d_recursive_mutex_t* _mutex
)
{
    BOOL result = TryEnterCriticalSection(_mutex);
    return result ? D_MUTEX_SUCCESS : D_MUTEX_BUSY;
}

int
d_recursive_mutex_unlock
(
    d_recursive_mutex_t* _mutex
)
{
    LeaveCriticalSection(_mutex);
    return D_MUTEX_SUCCESS;
}

#endif


///////////////////////////////////////////////////////////////////////////////
///             IV.   THREAD OPERATIONS                                     ///
///////////////////////////////////////////////////////////////////////////////

#if D_MUTEX_HAS_C11_THREADS

int
d_thread_create
(
    d_thread_t*      _thread,
    d_thread_func_t _func,
    void*            _arg
)
{
    int result = thrd_create(_thread, _func, _arg);
    return (result == thrd_success) ? D_MUTEX_SUCCESS : D_MUTEX_ERROR;
}

int
d_thread_join
(
    d_thread_t           _thread,
    d_thread_result_t* _result
)
{
    int result = thrd_join(_thread, _result);
    return (result == thrd_success) ? D_MUTEX_SUCCESS : D_MUTEX_ERROR;
}

int
d_thread_detach
(
    d_thread_t _thread
)
{
    int result = thrd_detach(_thread);
    return (result == thrd_success) ? D_MUTEX_SUCCESS : D_MUTEX_ERROR;
}

void
d_thread_exit
(
    d_thread_result_t _result
)
{
    thrd_exit(_result);
}

void
d_thread_yield
(
    void
)
{
    thrd_yield();
}

int
d_thread_sleep
(
    const struct timespec* _duration,
    struct timespec*       _remaining
)
{
    int result = thrd_sleep(_duration, _remaining);
    return (result == 0) ? D_MUTEX_SUCCESS : D_MUTEX_ERROR;
}

d_thread_t
d_thread_current
(
    void
)
{
    return thrd_current();
}

int
d_thread_equal
(
    d_thread_t _t1,
    d_thread_t _t2
)
{
    return thrd_equal(_t1, _t2);
}

#elif D_MUTEX_HAS_PTHREAD

// Wrapper structure for pthread to handle return value conversion
typedef struct {
    d_thread_func_t func;
    void*           arg;
} d_pthread_wrapper_arg_t;

static void*
d_pthread_wrapper
(
    void* _arg
)
{
    d_pthread_wrapper_arg_t* wrapper = (d_pthread_wrapper_arg_t*)_arg;
    d_thread_func_t func = wrapper->func;
    void* arg = wrapper->arg;
    free(wrapper);
    return func(arg);
}

int
d_thread_create
(
    d_thread_t*      _thread,
    d_thread_func_t _func,
    void*            _arg
)
{
    d_pthread_wrapper_arg_t* wrapper = 
        (d_pthread_wrapper_arg_t*)malloc(sizeof(d_pthread_wrapper_arg_t));
    if (wrapper == NULL)
        return D_MUTEX_NOMEM;
    
    wrapper->func = _func;
    wrapper->arg = _arg;
    
    int result = pthread_create(_thread, NULL, d_pthread_wrapper, wrapper);
    if (result != 0)
    {
        free(wrapper);
        return D_MUTEX_ERROR;
    }
    return D_MUTEX_SUCCESS;
}

int
d_thread_join
(
    d_thread_t           _thread,
    d_thread_result_t* _result
)
{
    int result = pthread_join(_thread, _result);
    return (result == 0) ? D_MUTEX_SUCCESS : D_MUTEX_ERROR;
}

int
d_thread_detach
(
    d_thread_t _thread
)
{
    int result = pthread_detach(_thread);
    return (result == 0) ? D_MUTEX_SUCCESS : D_MUTEX_ERROR;
}

void
d_thread_exit
(
    d_thread_result_t _result
)
{
    pthread_exit(_result);
}

void
d_thread_yield
(
    void
)
{
    #ifdef _POSIX_PRIORITY_SCHEDULING
        sched_yield();
    #else
        // Fallback: short sleep
        struct timespec ts = {0, 1};
        nanosleep(&ts, NULL);
    #endif
}

int
d_thread_sleep
(
    const struct timespec* _duration,
    struct timespec*       _remaining
)
{
    int result = nanosleep(_duration, _remaining);
    return (result == 0) ? D_MUTEX_SUCCESS : D_MUTEX_ERROR;
}

d_thread_t
d_thread_current
(
    void
)
{
    return pthread_self();
}

int
d_thread_equal
(
    d_thread_t _t1,
    d_thread_t _t2
)
{
    return pthread_equal(_t1, _t2);
}

#elif D_MUTEX_HAS_WINDOWS_THREADS

// Wrapper structure for Windows threads
typedef struct {
    d_thread_func_t func;
    void*           arg;
} d_win_thread_wrapper_arg_t;

static unsigned int __stdcall
d_win_thread_wrapper
(
    void* _arg
)
{
    d_win_thread_wrapper_arg_t* wrapper = (d_win_thread_wrapper_arg_t*)_arg;
    d_thread_func_t func = wrapper->func;
    void* arg = wrapper->arg;
    free(wrapper);
    
    void* result = func(arg);
    return (unsigned int)(uintptr_t)result;
}

int
d_thread_create
(
    d_thread_t*      _thread,
    d_thread_func_t _func,
    void*            _arg
)
{
    d_win_thread_wrapper_arg_t* wrapper = 
        (d_win_thread_wrapper_arg_t*)malloc(sizeof(d_win_thread_wrapper_arg_t));
    if (wrapper == NULL)
        return D_MUTEX_NOMEM;
    
    wrapper->func = _func;
    wrapper->arg = _arg;
    
    _thread->handle = (HANDLE)_beginthreadex(
        NULL, 0, d_win_thread_wrapper, wrapper, 0, &_thread->id);
    
    if (_thread->handle == NULL)
    {
        free(wrapper);
        return D_MUTEX_ERROR;
    }
    return D_MUTEX_SUCCESS;
}

int
d_thread_join
(
    d_thread_t           _thread,
    d_thread_result_t* _result
)
{
    DWORD wait_result = WaitForSingleObject(_thread.handle, INFINITE);
    if (wait_result != WAIT_OBJECT_0)
        return D_MUTEX_ERROR;
    
    if (_result != NULL)
    {
        DWORD exit_code;
        if (GetExitCodeThread(_thread.handle, &exit_code))
            *_result = (void*)(uintptr_t)exit_code;
    }
    
    CloseHandle(_thread.handle);
    return D_MUTEX_SUCCESS;
}

int
d_thread_detach
(
    d_thread_t _thread
)
{
    BOOL result = CloseHandle(_thread.handle);
    return result ? D_MUTEX_SUCCESS : D_MUTEX_ERROR;
}

void
d_thread_exit
(
    d_thread_result_t _result
)
{
    _endthreadex((unsigned int)(uintptr_t)_result);
}

void
d_thread_yield
(
    void
)
{
    SwitchToThread();
}

int
d_thread_sleep
(
    const struct timespec* _duration,
    struct timespec*       _remaining
)
{
    DWORD milliseconds = (DWORD)(_duration->tv_sec * 1000 + 
                                  _duration->tv_nsec / 1000000);
    Sleep(milliseconds);
    
    if (_remaining != NULL)
    {
        _remaining->tv_sec = 0;
        _remaining->tv_nsec = 0;
    }
    return D_MUTEX_SUCCESS;
}

d_thread_t
d_thread_current
(
    void
)
{
    d_thread_t thread;
    thread.handle = GetCurrentThread();
    thread.id = GetCurrentThreadId();
    return thread;
}

int
d_thread_equal
(
    d_thread_t _t1,
    d_thread_t _t2
)
{
    return (_t1.id == _t2.id);
}

#endif


// Thread-specific storage operations
#if D_MUTEX_HAS_C11_THREADS

int
d_tss_create
(
    d_tss_t*      _key,
    d_tss_dtor_t _dtor
)
{
    int result = tss_create(_key, _dtor);
    return (result == thrd_success) ? D_MUTEX_SUCCESS : D_MUTEX_ERROR;
}

int
d_tss_delete
(
    d_tss_t _key
)
{
    tss_delete(_key);
    return D_MUTEX_SUCCESS;
}

void*
d_tss_get
(
    d_tss_t _key
)
{
    return tss_get(_key);
}

int
d_tss_set
(
    d_tss_t _key,
    void*   _value
)
{
    int result = tss_set(_key, _value);
    return (result == thrd_success) ? D_MUTEX_SUCCESS : D_MUTEX_ERROR;
}

#elif D_MUTEX_HAS_PTHREAD

int
d_tss_create
(
    d_tss_t*      _key,
    d_tss_dtor_t _dtor
)
{
    int result = pthread_key_create(_key, _dtor);
    return (result == 0) ? D_MUTEX_SUCCESS : D_MUTEX_ERROR;
}

int
d_tss_delete
(
    d_tss_t _key
)
{
    int result = pthread_key_delete(_key);
    return (result == 0) ? D_MUTEX_SUCCESS : D_MUTEX_ERROR;
}

void*
d_tss_get
(
    d_tss_t _key
)
{
    return pthread_getspecific(_key);
}

int
d_tss_set
(
    d_tss_t _key,
    void*   _value
)
{
    int result = pthread_setspecific(_key, _value);
    return (result == 0) ? D_MUTEX_SUCCESS : D_MUTEX_ERROR;
}

#elif D_MUTEX_HAS_WINDOWS_THREADS

int
d_tss_create
(
    d_tss_t*      _key,
    d_tss_dtor_t _dtor
)
{
    (void)_dtor;  // Windows doesn't support destructors in TLS
    *_key = TlsAlloc();
    return (*_key != TLS_OUT_OF_INDEXES) ? D_MUTEX_SUCCESS : D_MUTEX_ERROR;
}

int
d_tss_delete
(
    d_tss_t _key
)
{
    BOOL result = TlsFree(_key);
    return result ? D_MUTEX_SUCCESS : D_MUTEX_ERROR;
}

void*
d_tss_get
(
    d_tss_t _key
)
{
    return TlsGetValue(_key);
}

int
d_tss_set
(
    d_tss_t _key,
    void*   _value
)
{
    BOOL result = TlsSetValue(_key, _value);
    return result ? D_MUTEX_SUCCESS : D_MUTEX_ERROR;
}

#endif


///////////////////////////////////////////////////////////////////////////////
///             V.    CONDITION VARIABLES                                   ///
///////////////////////////////////////////////////////////////////////////////

#if D_MUTEX_HAS_C11_THREADS

int
d_cond_init
(
    d_cond_t* _cond
)
{
    int result = cnd_init(_cond);
    return (result == thrd_success) ? D_MUTEX_SUCCESS : D_MUTEX_ERROR;
}

int
d_cond_destroy
(
    d_cond_t* _cond
)
{
    cnd_destroy(_cond);
    return D_MUTEX_SUCCESS;
}

int
d_cond_signal
(
    d_cond_t* _cond
)
{
    int result = cnd_signal(_cond);
    return (result == thrd_success) ? D_MUTEX_SUCCESS : D_MUTEX_ERROR;
}

int
d_cond_broadcast
(
    d_cond_t* _cond
)
{
    int result = cnd_broadcast(_cond);
    return (result == thrd_success) ? D_MUTEX_SUCCESS : D_MUTEX_ERROR;
}

int
d_cond_wait
(
    d_cond_t*  _cond,
    d_mutex_t* _mutex
)
{
    int result = cnd_wait(_cond, _mutex);
    return (result == thrd_success) ? D_MUTEX_SUCCESS : D_MUTEX_ERROR;
}

int
d_cond_timedwait
(
    d_cond_t*               _cond,
    d_mutex_t*              _mutex,
    const struct timespec* _timeout
)
{
    int result = cnd_timedwait(_cond, _mutex, _timeout);
    if (result == thrd_success)
        return D_MUTEX_SUCCESS;
    else if (result == thrd_timedout)
        return D_MUTEX_TIMEDOUT;
    else
        return D_MUTEX_ERROR;
}

#elif D_MUTEX_HAS_PTHREAD

int
d_cond_init
(
    d_cond_t* _cond
)
{
    int result = pthread_cond_init(_cond, NULL);
    return (result == 0) ? D_MUTEX_SUCCESS : D_MUTEX_ERROR;
}

int
d_cond_destroy
(
    d_cond_t* _cond
)
{
    int result = pthread_cond_destroy(_cond);
    return (result == 0) ? D_MUTEX_SUCCESS : D_MUTEX_ERROR;
}

int
d_cond_signal
(
    d_cond_t* _cond
)
{
    int result = pthread_cond_signal(_cond);
    return (result == 0) ? D_MUTEX_SUCCESS : D_MUTEX_ERROR;
}

int
d_cond_broadcast
(
    d_cond_t* _cond
)
{
    int result = pthread_cond_broadcast(_cond);
    return (result == 0) ? D_MUTEX_SUCCESS : D_MUTEX_ERROR;
}

int
d_cond_wait
(
    d_cond_t*  _cond,
    d_mutex_t* _mutex
)
{
    int result = pthread_cond_wait(_cond, _mutex);
    return (result == 0) ? D_MUTEX_SUCCESS : D_MUTEX_ERROR;
}

int
d_cond_timedwait
(
    d_cond_t*               _cond,
    d_mutex_t*              _mutex,
    const struct timespec* _timeout
)
{
    int result = pthread_cond_timedwait(_cond, _mutex, _timeout);
    if (result == 0)
        return D_MUTEX_SUCCESS;
    else if (result == ETIMEDOUT)
        return D_MUTEX_TIMEDOUT;
    else
        return D_MUTEX_ERROR;
}

#elif D_MUTEX_HAS_WINDOWS_THREADS && D_MUTEX_HAS_WINDOWS_CONDITION_VARIABLE

int
d_cond_init
(
    d_cond_t* _cond
)
{
    InitializeConditionVariable(_cond);
    return D_MUTEX_SUCCESS;
}

int
d_cond_destroy
(
    d_cond_t* _cond
)
{
    (void)_cond;  // Windows condition variables don't need cleanup
    return D_MUTEX_SUCCESS;
}

int
d_cond_signal
(
    d_cond_t* _cond
)
{
    WakeConditionVariable(_cond);
    return D_MUTEX_SUCCESS;
}

int
d_cond_broadcast
(
    d_cond_t* _cond
)
{
    WakeAllConditionVariable(_cond);
    return D_MUTEX_SUCCESS;
}

int
d_cond_wait
(
    d_cond_t*  _cond,
    d_mutex_t* _mutex
)
{
    // Note: This assumes the mutex is a Windows mutex handle
    // For proper integration, might need to use CRITICAL_SECTION instead
    BOOL result = SleepConditionVariableCS(_cond, 
                                           (CRITICAL_SECTION*)_mutex, 
                                           INFINITE);
    return result ? D_MUTEX_SUCCESS : D_MUTEX_ERROR;
}

int
d_cond_timedwait
(
    d_cond_t*               _cond,
    d_mutex_t*              _mutex,
    const struct timespec* _timeout
)
{
    DWORD timeout_ms = (DWORD)(_timeout->tv_sec * 1000 + 
                                _timeout->tv_nsec / 1000000);
    
    BOOL result = SleepConditionVariableCS(_cond, 
                                           (CRITICAL_SECTION*)_mutex, 
                                           timeout_ms);
    if (result)
        return D_MUTEX_SUCCESS;
    else if (GetLastError() == ERROR_TIMEOUT)
        return D_MUTEX_TIMEDOUT;
    else
        return D_MUTEX_ERROR;
}

#elif D_MUTEX_HAS_WINDOWS_THREADS

// Fallback condition variable implementation for older Windows
int
d_cond_init
(
    d_cond_t* _cond
)
{
    _cond->event = CreateEvent(NULL, TRUE, FALSE, NULL);
    _cond->waiters_count = 0;
    InitializeCriticalSection(&_cond->waiters_count_lock);
    return (_cond->event != NULL) ? D_MUTEX_SUCCESS : D_MUTEX_ERROR;
}

int
d_cond_destroy
(
    d_cond_t* _cond
)
{
    CloseHandle(_cond->event);
    DeleteCriticalSection(&_cond->waiters_count_lock);
    return D_MUTEX_SUCCESS;
}

int
d_cond_signal
(
    d_cond_t* _cond
)
{
    EnterCriticalSection(&_cond->waiters_count_lock);
    int have_waiters = (_cond->waiters_count > 0);
    LeaveCriticalSection(&_cond->waiters_count_lock);
    
    if (have_waiters)
        SetEvent(_cond->event);
    
    return D_MUTEX_SUCCESS;
}

int
d_cond_broadcast
(
    d_cond_t* _cond
)
{
    return d_cond_signal(_cond);
}

int
d_cond_wait
(
    d_cond_t*  _cond,
    d_mutex_t* _mutex
)
{
    EnterCriticalSection(&_cond->waiters_count_lock);
    _cond->waiters_count++;
    LeaveCriticalSection(&_cond->waiters_count_lock);
    
    d_mutex_unlock(_mutex);
    WaitForSingleObject(_cond->event, INFINITE);
    
    EnterCriticalSection(&_cond->waiters_count_lock);
    _cond->waiters_count--;
    LeaveCriticalSection(&_cond->waiters_count_lock);
    
    d_mutex_lock(_mutex);
    return D_MUTEX_SUCCESS;
}

int
d_cond_timedwait
(
    d_cond_t*               _cond,
    d_mutex_t*              _mutex,
    const struct timespec* _timeout
)
{
    DWORD timeout_ms = (DWORD)(_timeout->tv_sec * 1000 + 
                                _timeout->tv_nsec / 1000000);
    
    EnterCriticalSection(&_cond->waiters_count_lock);
    _cond->waiters_count++;
    LeaveCriticalSection(&_cond->waiters_count_lock);
    
    d_mutex_unlock(_mutex);
    DWORD result = WaitForSingleObject(_cond->event, timeout_ms);
    
    EnterCriticalSection(&_cond->waiters_count_lock);
    _cond->waiters_count--;
    LeaveCriticalSection(&_cond->waiters_count_lock);
    
    d_mutex_lock(_mutex);
    
    if (result == WAIT_OBJECT_0)
        return D_MUTEX_SUCCESS;
    else if (result == WAIT_TIMEOUT)
        return D_MUTEX_TIMEDOUT;
    else
        return D_MUTEX_ERROR;
}

#endif


///////////////////////////////////////////////////////////////////////////////
///             VI.   READ-WRITE LOCKS                                      ///
///////////////////////////////////////////////////////////////////////////////

#if D_MUTEX_HAS_PTHREAD

int
d_rwlock_init
(
    struct d_rwlock_t* _rwlock
)
{
    int result = pthread_rwlock_init(&_rwlock->rwlock, NULL);
    return (result == 0) ? D_MUTEX_SUCCESS : D_MUTEX_ERROR;
}

int
d_rwlock_destroy
(
    struct d_rwlock_t* _rwlock
)
{
    int result = pthread_rwlock_destroy(&_rwlock->rwlock);
    return (result == 0) ? D_MUTEX_SUCCESS : D_MUTEX_ERROR;
}

int
d_rwlock_rdlock
(
    struct d_rwlock_t* _rwlock
)
{
    int result = pthread_rwlock_rdlock(&_rwlock->rwlock);
    return (result == 0) ? D_MUTEX_SUCCESS : D_MUTEX_ERROR;
}

int
d_rwlock_tryrdlock
(
    struct d_rwlock_t* _rwlock
)
{
    int result = pthread_rwlock_tryrdlock(&_rwlock->rwlock);
    if (result == 0)
        return D_MUTEX_SUCCESS;
    else if (result == EBUSY)
        return D_MUTEX_BUSY;
    else
        return D_MUTEX_ERROR;
}

int
d_rwlock_wrlock
(
    struct d_rwlock_t* _rwlock
)
{
    int result = pthread_rwlock_wrlock(&_rwlock->rwlock);
    return (result == 0) ? D_MUTEX_SUCCESS : D_MUTEX_ERROR;
}

int
d_rwlock_trywrlock
(
    struct d_rwlock_t* _rwlock
)
{
    int result = pthread_rwlock_trywrlock(&_rwlock->rwlock);
    if (result == 0)
        return D_MUTEX_SUCCESS;
    else if (result == EBUSY)
        return D_MUTEX_BUSY;
    else
        return D_MUTEX_ERROR;
}

D_INLINE int
d_rwlock_unlock
(
    struct d_rwlock_t* _rwlock
)
{
    return (pthread_rwlock_unlock(&_rwlock->rwlock) == 0)
        ? D_MUTEX_SUCCESS
        : D_MUTEX_ERROR;
}

int
d_rwlock_timedrdlock
(
    struct d_rwlock_t*     _rwlock,
    const struct timespec* _timeout
)
{
    #ifdef _POSIX_TIMEOUTS
        int result = pthread_rwlock_timedrdlock(&_rwlock->rwlock, _timeout);

        if (result == 0)
        {
            return D_MUTEX_SUCCESS;
        }
        else if (result == ETIMEDOUT)
        {
            return D_MUTEX_TIMEDOUT;
        }
        else
        {
            return D_MUTEX_ERROR;
        }
    #else
        (void)_timeout;

        return d_rwlock_rdlock(_rwlock);
    #endif
}

int
d_rwlock_timedwrlock
(
    struct d_rwlock_t*     _rwlock,
    const struct timespec* _timeout
)
{
    #ifdef _POSIX_TIMEOUTS
        int result = pthread_rwlock_timedwrlock(&_rwlock->rwlock, _timeout);

        if (result == 0)
        {
            return D_MUTEX_SUCCESS;
        }
        else if (result == ETIMEDOUT)
        {
            return D_MUTEX_TIMEDOUT;
        }
        else
        {
            return D_MUTEX_ERROR;
        }
    #else
        (void)_timeout;

        return d_rwlock_wrlock(_rwlock);
    #endif
}

#elif ( D_MUTEX_HAS_WINDOWS_THREADS &&  \
        D_MUTEX_HAS_WINDOWS_SRWLOCK )

int
d_rwlock_init
(
    struct d_rwlock_t* _rwlock
)
{
    InitializeSRWLock(&_rwlock->srwlock);

    return D_MUTEX_SUCCESS;
}

int
d_rwlock_destroy
(
    struct d_rwlock_t* _rwlock
)
{
    (void)_rwlock;  // SRW locks don't need cleanup

    return D_MUTEX_SUCCESS;
}

int
d_rwlock_rdlock
(
    struct d_rwlock_t* _rwlock
)
{
    AcquireSRWLockShared(&_rwlock->srwlock);

    return D_MUTEX_SUCCESS;
}

int
d_rwlock_tryrdlock
(
    struct d_rwlock_t* _rwlock
)
{
    BOOLEAN result = TryAcquireSRWLockShared(&_rwlock->srwlock);

    return result 
        ? D_MUTEX_SUCCESS
        : D_MUTEX_BUSY;
}

int
d_rwlock_wrlock
(
    struct d_rwlock_t* _rwlock
)
{
    AcquireSRWLockExclusive(&_rwlock->srwlock);

    return D_MUTEX_SUCCESS;
}

int
d_rwlock_trywrlock
(
    struct d_rwlock_t* _rwlock
)
{
    BOOLEAN result = TryAcquireSRWLockExclusive(&_rwlock->srwlock);

    return result
        ? D_MUTEX_SUCCESS
        : D_MUTEX_BUSY;
}

int
d_rwlock_unlock
(
    struct d_rwlock_t* _rwlock
)
{
    // Need to track whether we have read or write lock - limitation of API
    // For now, try exclusive unlock first, then shared
    ReleaseSRWLockExclusive(&_rwlock->srwlock);

    return D_MUTEX_SUCCESS;
}

int
d_rwlock_timedrdlock
(
    struct d_rwlock_t*     _rwlock,
    const struct timespec* _timeout
)
{
    (void)_timeout;  // Windows SRW locks don't support timed operations

    return d_rwlock_rdlock(_rwlock);
}

int
d_rwlock_timedwrlock
(
    struct d_rwlock_t*     _rwlock,
    const struct timespec* _timeout
)
{
    (void)_timeout;

    return d_rwlock_wrlock(_rwlock);
}

#else

// Fallback implementation using mutex and condition variables
int
d_rwlock_init
(
    struct d_rwlock_t* _rwlock
)
{
    d_mutex_init(&_rwlock->mutex);
    d_cond_init(&_rwlock->readers_cond);
    d_cond_init(&_rwlock->writers_cond);
    
    _rwlock->readers = 0;
    _rwlock->writers = 0;
    _rwlock->waiting_writers = 0;

    return D_MUTEX_SUCCESS;
}

int
d_rwlock_destroy
(
    struct d_rwlock_t* _rwlock
)
{
    d_mutex_destroy(&_rwlock->mutex);
    d_cond_destroy(&_rwlock->readers_cond);
    d_cond_destroy(&_rwlock->writers_cond);

    return D_MUTEX_SUCCESS;
}

int
d_rwlock_rdlock
(
    struct d_rwlock_t* _rwlock
)
{
    d_mutex_lock(&_rwlock->mutex);
    
    while ( (_rwlock->writers > 0) ||
            (_rwlock->waiting_writers > 0) )
    {
        d_cond_wait(&_rwlock->readers_cond, &_rwlock->mutex);
    }
    
    _rwlock->readers++;
    d_mutex_unlock(&_rwlock->mutex);

    return D_MUTEX_SUCCESS;
}

int
d_rwlock_tryrdlock
(
    struct d_rwlock_t* _rwlock
)
{
    if (d_mutex_trylock(&_rwlock->mutex) != D_MUTEX_SUCCESS)
    {
        return D_MUTEX_BUSY;
    }
    
    if (_rwlock->writers > 0 || _rwlock->waiting_writers > 0)
    {
        d_mutex_unlock(&_rwlock->mutex);
        return D_MUTEX_BUSY;
    }
    
    _rwlock->readers++;
    d_mutex_unlock(&_rwlock->mutex);
    return D_MUTEX_SUCCESS;
}

int
d_rwlock_wrlock
(
    struct d_rwlock_t* _rwlock
)
{
    d_mutex_lock(&_rwlock->mutex);
    
    _rwlock->waiting_writers++;
    while (_rwlock->readers > 0 || _rwlock->writers > 0)
        d_cond_wait(&_rwlock->writers_cond, &_rwlock->mutex);
    _rwlock->waiting_writers--;
    
    _rwlock->writers++;
    d_mutex_unlock(&_rwlock->mutex);
    return D_MUTEX_SUCCESS;
}

int
d_rwlock_trywrlock
(
    struct d_rwlock_t* _rwlock
)
{
    if (d_mutex_trylock(&_rwlock->mutex) != D_MUTEX_SUCCESS)
        return D_MUTEX_BUSY;
    
    if (_rwlock->readers > 0 || _rwlock->writers > 0)
    {
        d_mutex_unlock(&_rwlock->mutex);
        return D_MUTEX_BUSY;
    }
    
    _rwlock->writers++;
    d_mutex_unlock(&_rwlock->mutex);
    return D_MUTEX_SUCCESS;
}

int
d_rwlock_unlock
(
    struct d_rwlock_t* _rwlock
)
{
    d_mutex_lock(&_rwlock->mutex);
    
    if (_rwlock->writers > 0)
    {
        _rwlock->writers--;
        if (_rwlock->waiting_writers > 0)
            d_cond_signal(&_rwlock->writers_cond);
        else
            d_cond_broadcast(&_rwlock->readers_cond);
    }
    else if (_rwlock->readers > 0)
    {
        _rwlock->readers--;
        if (_rwlock->readers == 0 && _rwlock->waiting_writers > 0)
            d_cond_signal(&_rwlock->writers_cond);
    }
    
    d_mutex_unlock(&_rwlock->mutex);
    return D_MUTEX_SUCCESS;
}

int
d_rwlock_timedrdlock
(
    struct d_rwlock_t*             _rwlock,
    const struct timespec* _timeout
)
{
    (void)_timeout;
    return d_rwlock_rdlock(_rwlock);
}

int
d_rwlock_timedwrlock
(
    struct d_rwlock_t*             _rwlock,
    const struct timespec* _timeout
)
{
    (void)_timeout;
    return d_rwlock_wrlock(_rwlock);
}

#endif


///////////////////////////////////////////////////////////////////////////////
///             VII.  SYNCHRONIZATION PRIMITIVES                            ///
///////////////////////////////////////////////////////////////////////////////

#if D_MUTEX_HAS_C11_THREADS

void
d_call_once
(
    d_once_flag_t* _flag,
    void         (*_func)(void)
)
{
    call_once(_flag, _func);
}

#elif D_MUTEX_HAS_PTHREAD

void
d_call_once
(
    d_once_flag_t* _flag,
    void         (*_func)(void)
)
{
    pthread_once(_flag, _func);
}

#elif D_MUTEX_HAS_WINDOWS_THREADS

static BOOL CALLBACK
d_once_wrapper
(
    PINIT_ONCE _init_once,
    PVOID      _parameter,
    PVOID*     _context
)
{
    void (*func)(void) = (void (*)(void))_parameter;
    (void)_init_once;
    (void)_context;
    func();
    return TRUE;
}

void
d_call_once
(
    d_once_flag_t* _flag,
    void         (*_func)(void)
)
{
    InitOnceExecuteOnce(_flag, d_once_wrapper, _func, NULL);
}

#else

// Fallback using atomic operations
void
d_call_once
(
    d_once_flag_t* _flag,
    void         (*_func)(void)
)
{
    // Simple but not entirely thread-safe fallback
    if (_flag->done == 0)
    {
        _func();
        _flag->done = 1;
    }
}

#endif


///////////////////////////////////////////////////////////////////////////////
///             IX.   UTILITY FUNCTIONS                                     ///
///////////////////////////////////////////////////////////////////////////////

int
d_thread_hardware_concurrency
(
    void
)
{
    #if D_MUTEX_HAS_WINDOWS_THREADS
        SYSTEM_INFO sysinfo;
        GetSystemInfo(&sysinfo);
        return (int)sysinfo.dwNumberOfProcessors;
    #elif defined(_SC_NPROCESSORS_ONLN)
        long nprocs = sysconf(_SC_NPROCESSORS_ONLN);
        return (nprocs > 0) ? (int)nprocs : 1;
    #elif defined(_SC_NPROC_ONLN)
        long nprocs = sysconf(_SC_NPROC_ONLN);
        return (nprocs > 0) ? (int)nprocs : 1;
    #else
        return 1;  // Conservative fallback
    #endif
}


