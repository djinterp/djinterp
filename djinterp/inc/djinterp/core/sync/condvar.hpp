/******************************************************************************
* djinterp [sync]                                                  condvar.hpp
*
* Portable condition variable, call-once, and concurrency query utilities
* for the thread-safe module.
*
* TYPES:
*   portable_condvar     - policy-aware condition variable wrapper
*   portable_once        - call_once wrapper (C++11 std::call_once or
*                          platform fallback)
*   hardware_concurrency - portable query for available CPU cores
*   d_thread_yield       - portable thread yield hint
*
* VERSIONING:
*   C++98/03:  `d_thread_yield` (platform fallback),
*              hardware_concurrency (platform API)
*   C++11:     + portable_condvar, portable_once
*   C++20:     + jthread-compatible condvar wait
*
*
* path:      /inc/djinterp/core/sync/condvar.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.07
******************************************************************************/

#ifndef DJINTERP_THREADSAFE_CONDVAR_
#define DJINTERP_THREADSAFE_CONDVAR_ 1

//#ifndef DJINTERP_ENVIRONMENT_
//    #error "condvar.hpp requires env.h to be included first"
//#endif
//
//#ifndef __cplusplus
//    #error "condvar.hpp can only be used in C++ compilation mode"
//#endif


// djinterp
#include "../djinterp.hpp"
#include "./lock_policy.hpp"

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    #include <condition_variable>
    #include <mutex>
    #include <thread>
    #include <chrono>
#endif

#if D_ENV_LANG_IS_CPP20_OR_HIGHER
    #include <stop_token>
#endif

// platform includes for pre-C++11 or supplemental
#if D_ENV_IS_OS_WINDOWS(D_ENV_OS_ID)
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #include <windows.h>
#elif defined(_POSIX_VERSION) ||                                              \
      defined(__unix__)       ||                                              \
      defined(__APPLE__)
    #include <unistd.h>
    #include <sched.h>
#endif


NS_DJINTERP

// =========================================================================
// I.   THREAD YIELD
// =========================================================================
// Portable yield hint.  Used by spinloops and backoff
// strategies when spinning is no longer productive.

inline void 
d_thread_yield()
{
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    std::this_thread::yield();
#elif D_ENV_IS_OS_WINDOWS(D_ENV_OS_ID)
    SwitchToThread();
#elif defined(_POSIX_VERSION)
    sched_yield();
#else
    // no-op
#endif
}


// =========================================================================
// II.  HARDWARE CONCURRENCY
// =========================================================================
// Returns the number of hardware threads available.
// Returns 0 if the value cannot be determined (the
// standard allows this).

inline unsigned hardware_concurrency()
{
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    return std::thread::hardware_concurrency();
#elif D_ENV_IS_OS_WINDOWS(D_ENV_OS_ID)
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    return static_cast<unsigned>(si.dwNumberOfProcessors);
#elif defined(_SC_NPROCESSORS_ONLN)
    long n = sysconf(_SC_NPROCESSORS_ONLN);
    return (n > 0) ? static_cast<unsigned>(n) : 0;
#else
    return 0;
#endif
}


// =========================================================================
// III. PORTABLE ONCE (C++11+)
// =========================================================================
// Wrapper around std::call_once / std::once_flag for
// thread-safe one-shot initialization.

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// portable_once
//   class: ensures a callable is executed exactly once,
// even under concurrent invocation from multiple threads.
class portable_once
{
public:
    portable_once() = default;

    portable_once(const portable_once&)            = delete;
    portable_once& operator=(const portable_once&) = delete;

    // call
    //   invokes _fn exactly once, regardless of how many
    // threads call this concurrently.
    template<typename _Fn,
             typename... _Args>
    void call(_Fn&& _fn,
              _Args&&... _args)
    {
        std::call_once(
            m_flag,
            std::forward<_Fn>(_fn),
            std::forward<_Args>(_args)...);
    }

private:
    std::once_flag m_flag;
};

#endif  // C++11


// =========================================================================
// IV.  PORTABLE CONDVAR (C++11+)
// =========================================================================
// Policy-aware condition variable.  When the policy uses
// std::mutex or compatible, this wraps
// std::condition_variable.  For shared_mutex policies
// or non-standard mutexes, it uses
// std::condition_variable_any.
//
// On null_lock_policy, the condvar is a no-op (no threads
// to notify).

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

NS_INTERNAL

    // condvar_selector
    //   trait: selects the appropriate condition_variable
    // type based on the policy's mutex type.

    // primary: use condition_variable_any (safe for any
    // mutex)
    template<typename _MutexType,
             typename = void>
    struct condvar_selector
    {
        using type = std::condition_variable_any;
    };

    // specialization: std::mutex gets the more efficient
    // std::condition_variable
    template<>
    struct condvar_selector<std::mutex>
    {
        using type = std::condition_variable;
    };

    // specialization: no_op_mutex gets a no-op condvar
    struct no_op_condvar
    {
        void notify_one() noexcept {}
        void notify_all() noexcept {}

        template<typename _Lock>
        void wait(_Lock& /*unused*/) {}

        template<typename _Lock,
                 typename _Predicate>
        void wait(_Lock& /*unused*/,
                  _Predicate   _predicate)
        {
            // single-threaded: if pred is false, it will
            // never become true (no other threads), so
            // this is a programming error.  In debug
            // builds, assert.
            (void)_predicate;
        }

        template<typename _Lock,
                 typename _Rep,
                 typename _Period>
        std::cv_status wait_for(
            _Lock& /*unused*/,
            const std::chrono::duration<_Rep, _Period>&
                /*unused*/)
        {
            return std::cv_status::no_timeout;
        }

        template<typename _Lock,
                 typename _Rep,
                 typename _Period,
                 typename _Predicate>
        bool wait_for(
            _Lock& /*unused*/,
            const std::chrono::duration<_Rep, _Period>&
                /*unused*/,
            _Predicate _predicate)
        {
            return _predicate();
        }
    };

    template<>
    struct condvar_selector<no_op_mutex>
    {
        using type = no_op_condvar;
    };

NS_END  // internal


// portable_condvar
//   class: policy-aware condition variable.  Selects the
// most efficient condvar implementation for the policy's
// mutex type.
template<typename _Policy>
class portable_condvar
{
public:
    using condvar_type =
        typename internal::condvar_selector<
            typename _Policy::mutex_type>::type;

    portable_condvar() = default;

    portable_condvar(const portable_condvar&)            = delete;
    portable_condvar& operator=(const portable_condvar&) = delete;

    // --- notify ---

    void notify_one() noexcept
    {
        m_cv.notify_one();
    }

    void notify_all() noexcept
    {
        m_cv.notify_all();
    }

    // --- wait (with lock) ---

    template<typename _Lock>
    void wait(_Lock& _lock)
    {
        m_cv.wait(_lock);
    }

    template<typename _Lock,
             typename _Predicate>
    void wait(_Lock& _lock,
              _Predicate  _predicate)
    {
        m_cv.wait(_lock, _predicate);
    }

    // --- wait_for (timed) ---

    template<typename _Lock,
             typename _Rep,
             typename _Period>
    std::cv_status wait_for(
        _Lock& _lock,
        const std::chrono::duration<_Rep, _Period>&
            _duration)
    {
        return m_cv.wait_for(_lock, _duration);
    }

    template<typename _Lock,
             typename _Rep,
             typename _Period,
             typename _Predicate>
    bool wait_for(
        _Lock& _lock,
        const std::chrono::duration<_Rep, _Period>&
            _duration,
        _Predicate  _predicate)
    {
        return m_cv.wait_for(
            _lock, _duration, _predicate);
    }

    // --- wait_until (timed) ---

    template<typename _Lock,
             typename _Clock,
             typename _Duration>
    std::cv_status wait_until(
        _Lock& _lock,
        const std::chrono::time_point<_Clock, _Duration>&
            _abs_time)
    {
        return m_cv.wait_until(_lock, _abs_time);
    }

    // --- C++20 jthread stop_token support ---

#if D_ENV_LANG_IS_CPP20_OR_HIGHER

    template<typename _Lock,
             typename _Predicate>
    bool wait(
        _Lock&           _lock,
        std::stop_token  _stoken,
        _Predicate            _predicate)
    {
        m_cv.wait(_lock, _stoken, _predicate);
        return _predicate();
    }

#endif  // C++20

    // --- direct access ---

    condvar_type& native() noexcept
    {
        return m_cv;
    }

    const condvar_type& native() const noexcept
    {
        return m_cv;
    }

private:
    condvar_type m_cv;
};

#endif  // C++11


NS_END  // djinterp


#endif  // DJINTERP_THREADSAFE_CONDVAR_