/******************************************************************************
* djinterp [sync]                                lock_policy_c.hpp
*
* C-backed lock policies for the thread-safe framework.
*   Provides lock policies that use platform C APIs (pthreads on POSIX,
* CRITICAL_SECTION / SRWLOCK on Windows) instead of C++ standard library
* mutexes.  Useful when:
*   - Targeting C++98/03 (no <mutex>)
*   - Interoperating with C code that uses the same mutexes
*   - Requiring recursive or spin-lock semantics not offered by
*     the standard C++ policies
*
* POLICY STRUCTS:
*   c_exclusive    - platform exclusive mutex (pthread_mutex / CRITICAL_SECTION)
*   c_shared       - platform read-write lock (pthread_rwlock / SRWLOCK)
*   c_recursive    - platform recursive mutex
*   c_spinlock     - test-and-set spinlock (atomic_flag)
*
* VERSIONING:
*   C++98/03:  c_exclusive, c_recursive (pthreads / Win32)
*   C++11:     + c_spinlock (requires std::atomic_flag)
*   C++17:     + c_shared on all platforms
*
*
* path:      /inc/djinterp/core/sync/lock_policy_c.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.07
******************************************************************************/

#ifndef DJINTERP_THREADSAFE_LOCK_POLICY_C_
#define DJINTERP_THREADSAFE_LOCK_POLICY_C_ 1

#ifndef __cplusplus
    #error "lock_policy_c.hpp can only be used in C++ compilation mode"
#endif

// djinterp
#include "../djinterp.hpp"
#include "./lock_policy.hpp"

// --- platform includes ---
#if D_ENV_IS_OS_WINDOWS(D_ENV_OS_ID)
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #include <windows.h>
#elif defined(_POSIX_VERSION) ||                                              \
      defined(__unix__)       ||                                              \
      defined(__APPLE__)
    #include <pthread.h>
    #define D_HAS_PTHREADS 1
#else
    #define D_HAS_PTHREADS 0
#endif

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    #include <atomic>
#endif


NS_DJINTERP


// =========================================================================
// I.   PLATFORM EXCLUSIVE MUTEX
// =========================================================================

#if D_ENV_IS_OS_WINDOWS(D_ENV_OS_ID)

// c_mutex_win32
//   class: wraps CRITICAL_SECTION.
class c_mutex_win32
{
public:
    c_mutex_win32()
    {
        InitializeCriticalSection(&m_cs);
    }

    ~c_mutex_win32()
    {
        DeleteCriticalSection(&m_cs);
    }

    void lock()
    {
        EnterCriticalSection(&m_cs);
    }

    void unlock()
    {
        LeaveCriticalSection(&m_cs);
    }

    bool try_lock()
    {
        return (TryEnterCriticalSection(&m_cs) != 0);
    }

private:
    c_mutex_win32(const c_mutex_win32&);
    c_mutex_win32& operator=(const c_mutex_win32&);

    CRITICAL_SECTION m_cs;
};

// c_rwlock_win32
//   class: wraps SRWLOCK (Vista+).
class c_rwlock_win32
{
public:
    c_rwlock_win32()
    {
        InitializeSRWLock(&m_srw);
    }

    ~c_rwlock_win32() {}

    void lock()
    {
        AcquireSRWLockExclusive(&m_srw);
    }

    void unlock()
    {
        ReleaseSRWLockExclusive(&m_srw);
    }

    bool try_lock()
    {
        return (TryAcquireSRWLockExclusive(&m_srw) != 0);
    }

    void lock_shared()
    {
        AcquireSRWLockShared(&m_srw);
    }

    void unlock_shared()
    {
        ReleaseSRWLockShared(&m_srw);
    }

private:
    c_rwlock_win32(const c_rwlock_win32&);
    c_rwlock_win32& operator=(const c_rwlock_win32&);

    SRWLOCK m_srw;
};

#elif D_HAS_PTHREADS

// c_mutex_posix
//   class: wraps pthread_mutex_t.
class c_mutex_posix
{
public:
    c_mutex_posix()
    {
        pthread_mutex_init(&m_mutex, NULL);
    }

    ~c_mutex_posix()
    {
        pthread_mutex_destroy(&m_mutex);
    }

    void lock()
    {
        pthread_mutex_lock(&m_mutex);
    }

    void unlock()
    {
        pthread_mutex_unlock(&m_mutex);
    }

    bool try_lock()
    {
        return (pthread_mutex_trylock(&m_mutex) == 0);
    }

private:
    c_mutex_posix(const c_mutex_posix&);
    c_mutex_posix& operator=(const c_mutex_posix&);

    pthread_mutex_t m_mutex;
};

// c_recursive_mutex_posix
//   class: wraps pthread_mutex_t with PTHREAD_MUTEX_RECURSIVE.
class c_recursive_mutex_posix
{
public:
    c_recursive_mutex_posix()
    {
        pthread_mutexattr_t attr;
        pthread_mutexattr_init(&attr);
        pthread_mutexattr_settype(
            &attr, PTHREAD_MUTEX_RECURSIVE);
        pthread_mutex_init(&m_mutex, &attr);
        pthread_mutexattr_destroy(&attr);
    }

    ~c_recursive_mutex_posix()
    {
        pthread_mutex_destroy(&m_mutex);
    }

    void lock()
    {
        pthread_mutex_lock(&m_mutex);
    }

    void unlock()
    {
        pthread_mutex_unlock(&m_mutex);
    }

    bool try_lock()
    {
        return (pthread_mutex_trylock(&m_mutex) == 0);
    }

private:
    c_recursive_mutex_posix(const c_recursive_mutex_posix&);
    c_recursive_mutex_posix& operator=(
        const c_recursive_mutex_posix&);

    pthread_mutex_t m_mutex;
};

// c_rwlock_posix
//   class: wraps pthread_rwlock_t.
class c_rwlock_posix
{
public:
    c_rwlock_posix()
    {
        pthread_rwlock_init(&m_rwlock, NULL);
    }

    ~c_rwlock_posix()
    {
        pthread_rwlock_destroy(&m_rwlock);
    }

    void lock()
    {
        pthread_rwlock_wrlock(&m_rwlock);
    }

    void unlock()
    {
        pthread_rwlock_unlock(&m_rwlock);
    }

    bool try_lock()
    {
        return (pthread_rwlock_trywrlock(
            &m_rwlock) == 0);
    }

    void lock_shared()
    {
        pthread_rwlock_rdlock(&m_rwlock);
    }

    void unlock_shared()
    {
        pthread_rwlock_unlock(&m_rwlock);
    }

private:
    c_rwlock_posix(const c_rwlock_posix&);
    c_rwlock_posix& operator=(const c_rwlock_posix&);

    pthread_rwlock_t m_rwlock;
};

#endif  // platform


// =========================================================================
// II.  C-BACKED RAII GUARDS
// =========================================================================

// c_lock_guard
//   class: RAII exclusive guard for C-backed mutexes.
template<typename _Mutex>
class c_lock_guard
{
public:
    explicit c_lock_guard(_Mutex& _m)
        : m_mutex(_m)
    {
        m_mutex.lock();
    }

    ~c_lock_guard()
    {
        m_mutex.unlock();
    }

    c_lock_guard(const c_lock_guard&)            D_DELETE;
    c_lock_guard& operator=(const c_lock_guard&) D_DELETE;

private:
    _Mutex& m_mutex;
};

// c_shared_guard
//   class: RAII shared (reader) guard for C-backed
// read-write locks.
template<typename _RWLock>
class c_shared_guard
{
public:
    explicit c_shared_guard(_RWLock& _m)
        : m_rwlock(_m)
    {
        m_rwlock.lock_shared();
    }

    ~c_shared_guard()
    {
        m_rwlock.unlock_shared();
    }

    c_shared_guard(const c_shared_guard&)            D_DELETE;
    c_shared_guard& operator=(const c_shared_guard&) D_DELETE;

private:
    _RWLock& m_rwlock;
};


// =========================================================================
// III. C-BACKED LOCK POLICY STRUCTS
// =========================================================================

#if D_ENV_IS_OS_WINDOWS(D_ENV_OS_ID)

// c_exclusive
//   struct: lock policy using Win32 CRITICAL_SECTION.
struct c_exclusive
{
    typedef c_mutex_win32                    mutex_type;
    typedef c_lock_guard<c_mutex_win32>      read_lock_type;
    typedef c_lock_guard<c_mutex_win32>      write_lock_type;

    static const bool is_threadsafe = true;
    static const bool is_shared     = false;
    static const bool is_timed      = false;

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    static constexpr thread_safety_level level =
        thread_safety_level::exclusive;
#endif
};

// c_shared
//   struct: lock policy using Win32 SRWLOCK.
struct c_shared
{
    typedef c_rwlock_win32                       mutex_type;
    typedef c_shared_guard<c_rwlock_win32>       read_lock_type;
    typedef c_lock_guard<c_rwlock_win32>          write_lock_type;

    static const bool is_threadsafe = true;
    static const bool is_shared     = true;
    static const bool is_timed      = false;

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    static constexpr thread_safety_level level =
        thread_safety_level::shared;
#endif
};

// c_recursive
//   struct: lock policy using Win32 recursive
// CRITICAL_SECTION (CRITICAL_SECTION is recursive by
// default on Windows).
struct c_recursive
{
    typedef c_mutex_win32                    mutex_type;
    typedef c_lock_guard<c_mutex_win32>      read_lock_type;
    typedef c_lock_guard<c_mutex_win32>      write_lock_type;

    static const bool is_threadsafe = true;
    static const bool is_shared     = false;
    static const bool is_timed      = false;

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    static constexpr thread_safety_level level =
        thread_safety_level::exclusive;
#endif
};

#elif D_HAS_PTHREADS

// c_exclusive
//   struct: lock policy using pthread_mutex_t.
struct c_exclusive
{
    typedef c_mutex_posix                    mutex_type;
    typedef c_lock_guard<c_mutex_posix>      read_lock_type;
    typedef c_lock_guard<c_mutex_posix>      write_lock_type;

    static const bool is_threadsafe = true;
    static const bool is_shared     = false;
    static const bool is_timed      = false;

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    static constexpr thread_safety_level level =
        thread_safety_level::exclusive;
#endif
};

// c_shared
//   struct: lock policy using pthread_rwlock_t.
struct c_shared
{
    typedef c_rwlock_posix                       mutex_type;
    typedef c_shared_guard<c_rwlock_posix>       read_lock_type;
    typedef c_lock_guard<c_rwlock_posix>          write_lock_type;

    static const bool is_threadsafe = true;
    static const bool is_shared     = true;
    static const bool is_timed      = false;

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    static constexpr thread_safety_level level =
        thread_safety_level::shared;
#endif
};

// c_recursive
//   struct: lock policy using recursive pthread_mutex_t.
struct c_recursive
{
    typedef c_recursive_mutex_posix                    mutex_type;
    typedef c_lock_guard<c_recursive_mutex_posix>      read_lock_type;
    typedef c_lock_guard<c_recursive_mutex_posix>      write_lock_type;

    static const bool is_threadsafe = true;
    static const bool is_shared     = false;
    static const bool is_timed      = false;

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    static constexpr thread_safety_level level =
        thread_safety_level::exclusive;
#endif
};

#endif  // platform


// =========================================================================
// IV.  SPINLOCK POLICY (C++11+)
// =========================================================================

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// spinlock_mutex
//   class: test-and-set spinlock using std::atomic_flag.
// Suitable for very short critical sections where context
// switching overhead exceeds the expected wait time.
//
// WARNING: spinlocks are NOT fair and can cause
// priority inversion.  Do not use for long-held locks.
class spinlock_mutex
{
public:
    // works for C++11-26
    // TO DO: make compatible with C++98
    spinlock_mutex() noexcept
    {
        m_flag.clear(std::memory_order_relaxed);
    }

    void lock() noexcept
    {
        while (m_flag.test_and_set(
            std::memory_order_acquire))
        {
            // spin - platform pause hint
        #if defined(__x86_64__) || defined(_M_X64) || \
            defined(__i386__)   || defined(_M_IX86)
            #if defined(_MSC_VER)
                _mm_pause();
            #else
                __builtin_ia32_pause();
            #endif
        #elif defined(__aarch64__) || defined(_M_ARM64)
            #if defined(_MSC_VER)
                __yield();
            #else
                __asm__ volatile("yield");
            #endif
        #endif
        }
    }

    void unlock() noexcept
    {
        m_flag.clear(std::memory_order_release);
    }

    bool try_lock() noexcept
    {
        return !m_flag.test_and_set(
            std::memory_order_acquire);
    }

private:
    std::atomic_flag m_flag;
};

// c_spinlock
//   struct: lock policy using the spinlock_mutex.
struct c_spinlock
{
    using mutex_type      = spinlock_mutex;
    using read_lock_type  = c_lock_guard<spinlock_mutex>;
    using write_lock_type = c_lock_guard<spinlock_mutex>;

    static constexpr bool is_threadsafe = true;
    static constexpr bool is_shared     = false;
    static constexpr bool is_timed      = false;

    static constexpr thread_safety_level level =
        thread_safety_level::exclusive;
};

#endif  // C++11


NS_END  // djinterp


#endif  // DJINTERP_THREADSAFE_LOCK_POLICY_C_
