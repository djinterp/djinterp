/******************************************************************************
* djinterp [sync]                                               lock_guard.hpp
*
* Policy-aware RAII lock guards for the thread-safe framework.
*   These guards are the primary mechanism for acquiring and releasing locks
* in threadsafe code.  They dispatch to the correct lock type based on the
* policy's type aliases, so implementations never name a concrete
* mutex or lock type directly.
*
* GUARD TYPES:
*   scoped_read_lock<Policy>   - acquires Policy::read_lock_type (shared
*                                when available, exclusive otherwise)
*   scoped_write_lock<Policy>  - acquires Policy::write_lock_type (always
*                                exclusive)
*   scoped_try_lock<Policy>    - non-blocking; exposes owns_lock() to
*                                test whether acquisition succeeded
* VERSIONING:
*   C++98/03:  all guards work (null_lock_policy only)
*   C++11:     + exclusive_lock_policy, timed_lock_policy
*   C++14:     + shared_timed_lock_policy
*   C++17:     + shared_lock_policy, if constexpr dispatch
*   C++20:     + concepts-guarded overloads
*
*
* path:      /inc/djinterp/core/sync/lock_guard.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.07
******************************************************************************/

#ifndef DJINTERP_THREADSAFE_LOCK_GUARD_
#define DJINTERP_THREADSAFE_LOCK_GUARD_ 1

//#ifndef DJINTERP_ENVIRONMENT_
//    #error "lock_guard.hpp requires env.h to be included first"
//#endif

//#ifndef __cplusplus
//    #error "lock_guard.hpp can only be used in C++ compilation mode"
//#endif

// djinterp
#include "../djinterp.hpp"
#include "./lock_policy.hpp"

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    #include <chrono>
#endif


NS_DJINTERP

// =========================================================================
// I.   SCOPED READ LOCK
// =========================================================================
// Acquires a read lock (shared when available) for the
// lifetime of the object.  On policies without shared
// locking, this acquires an exclusive lock - the
// user code doesn't need to know the difference.

template<typename _Policy>
class scoped_read_lock
{
public:
    using lock_type =
        typename _Policy::read_lock_type;
    using mutex_type =
        typename _Policy::mutex_type;

    explicit scoped_read_lock(mutex_type& _mutex)
        : m_lock(_mutex)
    {}

    ~scoped_read_lock() = default;

    scoped_read_lock(const scoped_read_lock&)            D_DELETE;
    scoped_read_lock& operator=(const scoped_read_lock&) D_DELETE;

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    scoped_read_lock(scoped_read_lock&&)                 = default;
    scoped_read_lock& operator=(scoped_read_lock&&)      = default;
#endif

private:
    lock_type m_lock;
};


// =========================================================================
// II.  SCOPED WRITE LOCK
// =========================================================================
// Acquires an exclusive write lock for the lifetime of
// the object.

template<typename _Policy>
class scoped_write_lock
{
public:
    using lock_type =
        typename _Policy::write_lock_type;
    using mutex_type =
        typename _Policy::mutex_type;

    explicit scoped_write_lock(mutex_type& _mutex)
        : m_lock(_mutex)
    {}

    ~scoped_write_lock() = default;

    scoped_write_lock(const scoped_write_lock&)            D_DELETE;
    scoped_write_lock& operator=(const scoped_write_lock&) D_DELETE;

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    scoped_write_lock(scoped_write_lock&&)                 = default;
    scoped_write_lock& operator=(scoped_write_lock&&)      = default;
#endif

private:
    lock_type m_lock;
};


// =========================================================================
// III. SCOPED TRY LOCK
// =========================================================================
// Non-blocking exclusive lock attempt.  The caller must
// check owns_lock() before accessing the protected
// resource.
//
// On null_lock_policy, try-lock always succeeds.

template<typename _Policy>
class scoped_try_lock
{
public:
    using mutex_type =
        typename _Policy::mutex_type;

    explicit scoped_try_lock(mutex_type& _mutex)
        : m_mutex(_mutex)
        , m_owned(false)
    {
        m_owned = m_mutex.try_lock();
    }

    ~scoped_try_lock()
    {
        if (m_owned)
        {
            m_mutex.unlock();
        }
    }

    scoped_try_lock(const scoped_try_lock&)            D_DELETE;
    scoped_try_lock& operator=(const scoped_try_lock&) D_DELETE;

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    scoped_try_lock(scoped_try_lock&& _other) noexcept
        : m_mutex(_other.m_mutex)
        , m_owned(_other.m_owned)
    {
        _other.m_owned = false;
    }
#endif

    bool owns_lock() const noexcept
    {
        return m_owned;
    }

    // release
    //   manually releases the lock before the guard's
    // destructor.  After calling release(), owns_lock()
    // returns false.
    void release()
    {
        if (m_owned)
        {
            m_mutex.unlock();
            m_owned = false;
        }
    }

private:
    mutex_type& m_mutex;
    bool        m_owned;
};


// =========================================================================
// IV.  TIMED SCOPED LOCK (C++11+)
// =========================================================================
// Acquires an exclusive lock with a timeout.  Only
// available on policies where is_timed == true.

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

template<typename _Policy>
class scoped_timed_lock
{
public:
    using mutex_type =
        typename _Policy::mutex_type;

    template<typename _Rep,
             typename _Period>
    scoped_timed_lock(
        mutex_type&                                  _mutex,
        const std::chrono::duration<_Rep, _Period>&  _timeout)
        : m_mutex(_mutex)
        , m_owned(false)
    {
        m_owned = m_mutex.try_lock_for(_timeout);
    }

    ~scoped_timed_lock()
    {
        if (m_owned)
        {
            m_mutex.unlock();
        }
    }

    scoped_timed_lock(const scoped_timed_lock&)            = delete;
    scoped_timed_lock& operator=(const scoped_timed_lock&) = delete;

    scoped_timed_lock(
        scoped_timed_lock&& _other
    ) noexcept
        : m_mutex(_other.m_mutex),
          m_owned(_other.m_owned)
    {
        _other.m_owned = false;
    }

    bool owns_lock() const noexcept
    {
        return m_owned;
    }

    void release()
    {
        if (m_owned)
        {
            m_mutex.unlock();
            m_owned = false;
        }
    }

private:
    mutex_type& m_mutex;
    bool        m_owned;
};

#endif  // C++11


// =========================================================================
// V.   UPGRADE LOCK (C++17+)
// =========================================================================
// Upgrades a read lock to a write lock.  The read lock is
// released and a write lock is acquired.  NOT atomic -
// there is a window where no lock is held.  Callers must
// re-validate shared state after upgrading.
//
// Note: true atomic upgrade requires platform-specific
// support not available in the C++ standard.  This
// implementation trades atomicity for portability.

#if D_ENV_LANG_IS_CPP17_OR_HIGHER

template<typename _Policy>
class upgrade_lock
{
public:
    using mutex_type = typename _Policy::mutex_type;
    using read_lock  = typename _Policy::read_lock_type;
    using write_lock = typename _Policy::write_lock_type;

    // construct from an existing read lock.
    // The read lock is released and a write lock is
    // acquired.
    explicit upgrade_lock(
        read_lock&&  _read,
        mutex_type&  _mutex
    )
        : m_write_lock(_mutex, std::defer_lock)
    {
        // release the read lock
        _read.unlock();

        // acquire exclusive write lock
        m_write_lock.lock();
    }

    ~upgrade_lock() = default;

    upgrade_lock(const upgrade_lock&)            = delete;
    upgrade_lock& operator=(const upgrade_lock&) = delete;
    upgrade_lock(upgrade_lock&&)                 = default;

    // downgrade
    //   releases the write lock.  The caller must
    // re-acquire a read lock separately.
    void downgrade()
    {
        m_write_lock.unlock();
    }

    bool owns_lock() const noexcept
    {
        return m_write_lock.owns_lock();
    }

private:
    write_lock m_write_lock;
};

#endif  // C++17


NS_END  // djinterp


#endif  // DJINTERP_THREADSAFE_LOCK_GUARD_