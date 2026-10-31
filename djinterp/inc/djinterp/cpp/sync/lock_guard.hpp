/*******************************************************************************
* djinterp [cpp]                                                  lock_guard.hpp
*
* Policy-aware RAII lock guards for the thread-safe framework.
*   These guards are the primary mechanism for acquiring and releasing locks
* in threadsafe code. They dispatch to the correct lock type based on the
* policy's type aliases, so implementations never name a concrete
* mutex or lock type directly.
*
* GUARD TYPES:
*   DScopedReadLock<Policy>   - acquires Policy::read_lock_type (shared
*                                when available, exclusive otherwise)
*   DScopedWriteLock<Policy>  - acquires Policy::write_lock_type (always
*                                exclusive)
*   DScopedTryLock<Policy>    - non-blocking; exposes owns_lock() to
*                                test whether acquisition succeeded
* VERSIONING:
*   C++98/03:  all guards work (null_lock_policy only)
*   C++11:     + exclusive_lock_policy, timed_lock_policy
*   C++14:     + shared_timed_lock_policy
*   C++17:     + shared_lock_policy, if constexpr dispatch
*   C++20:     + concepts-guarded overloads
*
*
* path:      /inc/djinterp/cpp/sync/lock_guard.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.04.07
*                                                            revised: 2026.10.01
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    SCOPED READ LOCK
      ----------------

II.   SCOPED WRITE LOCK
      -----------------

III.  SCOPED TRY LOCK
      ---------------

IV.   TIMED SCOPED LOCK (C++11+)
      --------------------------

V.    UPGRADE LOCK (C++17+)
      ---------------------
*/

#ifndef DJINTERP_SYNC_LOCK_GUARD_HPP
#define DJINTERP_SYNC_LOCK_GUARD_HPP 1

// FLOOR, FOR NOW: below C++11 this file is empty, rather than an error (README
// rule 5). The owner's ruling: compile at every level first; port to C++98
// only where something needs it.
#include "../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// djinterp
#include "../djinterp.hpp"
#include "./lock_policy.hpp"

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    // std
    #include <chrono>
#endif


NS_DJINTERP

// I.    Scoped read lock
// Acquires a read lock (shared when available) for the
// lifetime of the object. On policies without shared
// locking, this acquires an exclusive lock - the
// user code doesn't need to know the difference.

template<typename Policy>
class DScopedReadLock
{
public:
    using lock_type =
        typename Policy::read_lock_type;
    using mutex_type =
        typename Policy::mutex_type;

    explicit DScopedReadLock(mutex_type& _mutex)
        : m_lock(_mutex)
    {}

    ~DScopedReadLock() = default;

    DScopedReadLock(
        const DScopedReadLock&
    ) D_DELETE;
    DScopedReadLock& operator=(
        const DScopedReadLock&
    ) D_DELETE;

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    DScopedReadLock(
        DScopedReadLock&&
    ) = default;
    DScopedReadLock& operator=(
        DScopedReadLock&&
    ) = default;
#endif

private:
    lock_type m_lock;
};


// II.   Scoped write lock
// Acquires an exclusive write lock for the lifetime of
// the object.

template<typename Policy>
class DScopedWriteLock
{
public:
    using lock_type =
        typename Policy::write_lock_type;
    using mutex_type =
        typename Policy::mutex_type;

    explicit DScopedWriteLock(mutex_type& _mutex)
        : m_lock(_mutex)
    {}

    ~DScopedWriteLock() = default;

    DScopedWriteLock(
        const DScopedWriteLock&
    ) D_DELETE;
    DScopedWriteLock& operator=(
        const DScopedWriteLock&
    ) D_DELETE;

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    DScopedWriteLock(
        DScopedWriteLock&&
    ) = default;
    DScopedWriteLock& operator=(
        DScopedWriteLock&&
    ) = default;
#endif

private:
    lock_type m_lock;
};


// III.  Scoped try lock
// Non-blocking exclusive lock attempt. The caller must
// check owns_lock() before accessing the protected
// resource.
//
// On null_lock_policy, try-lock always succeeds.

template<typename Policy>
class DScopedTryLock
{
public:
    using mutex_type =
        typename Policy::mutex_type;

    explicit DScopedTryLock(mutex_type& _mutex)
        : m_mutex(_mutex)
        , m_owned(false)
    {
        m_owned = m_mutex.try_lock();
    }

    ~DScopedTryLock()
    {
        if (m_owned)
        {
            m_mutex.unlock();
        }
    }

    DScopedTryLock(
        const DScopedTryLock&
    ) D_DELETE;
    DScopedTryLock& operator=(
        const DScopedTryLock&
    ) D_DELETE;

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    DScopedTryLock(DScopedTryLock&& _other) noexcept
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
    // destructor. After calling release(), owns_lock()
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


// IV.   Timed scoped lock (C++11+)
// Acquires an exclusive lock with a timeout. Only
// available on policies where is_timed == true.

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

template<typename Policy>
class DScopedTimedLock
{
public:
    using mutex_type =
        typename Policy::mutex_type;

    template<typename Rep,
             typename Period>
    DScopedTimedLock(
        mutex_type&                                  _mutex,
        const std::chrono::duration<Rep, Period>&  _timeout)
        : m_mutex(_mutex)
        , m_owned(false)
    {
        m_owned = m_mutex.try_lock_for(_timeout);
    }

    ~DScopedTimedLock()
    {
        if (m_owned)
        {
            m_mutex.unlock();
        }
    }

    DScopedTimedLock(
        const DScopedTimedLock&
    ) = delete;
    DScopedTimedLock& operator=(
        const DScopedTimedLock&
    ) = delete;

    DScopedTimedLock(
        DScopedTimedLock&& _other
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


// V.    Upgrade lock (C++17+)
// Upgrades a read lock to a write lock. The read lock is
// released and a write lock is acquired. NOT atomic -
// there is a window where no lock is held. Callers must
// re-validate shared state after upgrading.
//
// Note: true atomic upgrade requires platform-specific
// support not available in the C++ standard. This
// implementation trades atomicity for portability.

#if D_ENV_LANG_IS_CPP17_OR_HIGHER

template<typename Policy>
class DUpgradeLock
{
public:
    using mutex_type = typename Policy::mutex_type;
    using read_lock  = typename Policy::read_lock_type;
    using write_lock = typename Policy::write_lock_type;

    // construct from an existing read lock.
    // The read lock is released and a write lock is
    // acquired.
    explicit DUpgradeLock(
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

    ~DUpgradeLock() = default;

    DUpgradeLock(
        const DUpgradeLock&
    ) = delete;
    DUpgradeLock& operator=(
        const DUpgradeLock&
    ) = delete;
    DUpgradeLock(
        DUpgradeLock&&
    ) = default;

    // downgrade
    //   releases the write lock. The caller must
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

#endif  // floor, for now


#endif  // DJINTERP_SYNC_LOCK_GUARD_HPP
