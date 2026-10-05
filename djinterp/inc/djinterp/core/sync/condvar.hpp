/*******************************************************************************
* djinterp [core]                                                    condvar.hpp
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
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.04.07
*                                                            revised: 2026.10.01
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    THREAD YIELD
      ------------

II.   HARDWARE CONCURRENCY
      --------------------

III.  PORTABLE ONCE (C++11+)
      ----------------------

IV.   PORTABLE CONDVAR (C++11+)
      -------------------------
*/

#ifndef DJINTERP_SYNC_CONDVAR_HPP
#define DJINTERP_SYNC_CONDVAR_HPP 1

// FLOOR, FOR NOW: below C++11 this file is empty, rather than an error (README
// rule 5). The owner's ruling: compile at every level first; port to C++98
// only where something needs it.
#include "../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP11_OR_HIGHER

//
// djinterp
#include "../../djinterp.hpp"
#include "./lock_policy.hpp"
#include "./sync_common.hpp"
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    // std
    #include <chrono>
    #include <condition_variable>
    #include <mutex>
    #include <thread>
#endif

#if D_ENV_LANG_IS_CPP20_OR_HIGHER
    // std
    #include <stop_token>
#endif

// platform includes for pre-C++11 or supplemental
#if D_ENV_IS_OS_WINDOWS(D_ENV_OS_ID)
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    // windows
    #include <windows.h>
#elif defined(_POSIX_VERSION) ||                                              \
      defined(__unix__)       ||                                              \
      defined(__APPLE__)
    // windows
    #include <unistd.h>
    #include <sched.h>
#endif


NS_DJINTERP

// I.    Thread yield
// Portable yield hint. Used by spinloops and backoff
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


// II.   Hardware concurrency
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


// III.  Portable once (C++11+)
// Wrapper around std::call_once / std::once_flag for
// thread-safe one-shot initialization.

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// portable_once
//   class: ensures a callable is executed exactly once,
// even under concurrent invocation from multiple threads.
class portable_once
{
public:
    // non-copyable, non-movable: others hold references
    // or pointers INTO this object. The MACRO form is used
    // rather than the nonmovable base because these types
    // nest one another - two empty bases in one object need
    // distinct addresses, which defeats the empty base
    // optimization and would grow every one of them.
    D_NONMOVABLE(portable_once)

    portable_once() = default;


    // call
    //   invokes _fn exactly once, regardless of how many
    // threads call this concurrently.
    template<typename Fn,
             typename... Args>
    void call(Fn&& _fn,
              Args&&... _args)
    {
        std::call_once(
            m_flag,
            std::forward<Fn>(_fn),
            std::forward<Args>(_args)...);
    }

private:
    std::once_flag m_flag;
};

#endif  // C++11


// IV.   Portable condvar (C++11+)
// Policy-aware condition variable. When the policy uses
// std::mutex or compatible, this wraps
// std::condition_variable. For shared_mutex policies
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
    template<typename MutexType,
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

        template<typename Lock>
        void wait(Lock& /*unused*/) {}

        template<typename Lock,
                 typename Predicate>
        void wait(Lock& /*unused*/,
                  Predicate   _predicate)
        {
            // single-threaded: if pred is false, it will
            // never become true (no other threads), so
            // this is a programming error. In debug
            // builds, assert.
            (void)_predicate;
        }

        template<typename Lock,
                 typename Rep,
                 typename Period>
        std::cv_status wait_for(
            Lock& /*unused*/,
            const std::chrono::duration<Rep, Period>&
                /*unused*/)
        {
            return std::cv_status::no_timeout;
        }

        template<typename Lock,
                 typename Rep,
                 typename Period,
                 typename Predicate>
        bool wait_for(
            Lock& /*unused*/,
            const std::chrono::duration<Rep, Period>&
                /*unused*/,
            Predicate _predicate)
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
//   class: policy-aware condition variable. Selects the
// most efficient condvar implementation for the policy's
// mutex type.
template<typename Policy>
class portable_condvar
{
public:
    // non-copyable, non-movable: others hold references
    // or pointers INTO this object. The MACRO form is used
    // rather than the nonmovable base because these types
    // nest one another - two empty bases in one object need
    // distinct addresses, which defeats the empty base
    // optimization and would grow every one of them.
    D_NONMOVABLE(portable_condvar)

    using condvar_type =
        typename internal::condvar_selector<
            typename Policy::mutex_type>::type;

    portable_condvar() = default;


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

    template<typename Lock>
    void wait(Lock& _lock)
    {
        m_cv.wait(_lock);
    }

    template<typename Lock,
             typename Predicate>
    void wait(Lock& _lock,
              Predicate  _predicate)
    {
        m_cv.wait(_lock, _predicate);
    }

    // --- wait_for (timed) ---

    template<typename Lock,
             typename Rep,
             typename Period>
    std::cv_status wait_for(
        Lock& _lock,
        const std::chrono::duration<Rep, Period>&
            _duration)
    {
        return m_cv.wait_for(_lock, _duration);
    }

    template<typename Lock,
             typename Rep,
             typename Period,
             typename Predicate>
    bool wait_for(
        Lock& _lock,
        const std::chrono::duration<Rep, Period>&
            _duration,
        Predicate  _predicate)
    {
        return m_cv.wait_for(
            _lock, _duration, _predicate);
    }

    // --- wait_until (timed) ---

    template<typename Lock,
             typename Clock,
             typename Duration>
    std::cv_status wait_until(
        Lock& _lock,
        const std::chrono::time_point<Clock, Duration>&
            _abs_time)
    {
        return m_cv.wait_until(_lock, _abs_time);
    }

    // --- C++20 jthread stop_token support ---

#if D_ENV_LANG_IS_CPP20_OR_HIGHER

    template<typename Lock,
             typename Predicate>
    bool wait(
        Lock&           _lock,
        std::stop_token  _stoken,
        Predicate            _predicate)
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

#endif  // floor, for now


#endif  // DJINTERP_SYNC_CONDVAR_HPP
