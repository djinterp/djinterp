/******************************************************************************
* djinterp [container]                                        threadsafe.hpp
*
* Foundation module for thread-safe containers in the djinterp framework.
*   Provides the vocabulary types, lock policy templates, and RAII
* utilities that all threadsafe_<container> implementations build upon.
*
*   The design is policy-based: a thread-safe container is parameterized
* on a lock policy type that governs its synchronization strategy.  The
* lock policy is a plain struct exposing a mutex type and associated
* guard types — no virtual functions, no inheritance hierarchy.  The
* framework provides four built-in policies plus a null policy for
* single-threaded builds; users can define their own.
*
*   All policies are structural: they are detected at compile time via
* the traits in threadsafe_container_traits.hpp.  The container need
* not know which policy it holds at the type level — it queries the
* policy's capabilities (exclusive, shared, timed) and selects its
* locking strategy accordingly.
*
* BUILT-IN LOCK POLICIES:
*   null_lock_policy       — no synchronization (single-threaded)
*   exclusive_lock_policy  — std::mutex based
*   shared_lock_policy     — std::shared_mutex (reader/writer)
*   timed_lock_policy      — std::timed_mutex
*   shared_timed_lock_policy — std::shared_timed_mutex
*
* RAII GUARDS:
*   scoped_read_lock<Policy>    — shared (reader) lock
*   scoped_write_lock<Policy>   — exclusive (writer) lock
*   scoped_try_lock<Policy>     — non-blocking try-lock
*
* DEPENDENCIES:
*   djinterp.hpp       - namespace macros
*   env.h              - threading availability detection
*   cpp_named11.hpp    - lockable trait detection
*
* TABLE OF CONTENTS
* =================
* I.      Thread Safety Level Enum
* II.     Null Lock Policy (single-threaded)
* III.    Exclusive Lock Policy (std::mutex)
* IV.     Shared Lock Policy (std::shared_mutex, C++17)
* V.      Timed Lock Policy (std::timed_mutex)
* VI.     Shared Timed Lock Policy
* VII.    RAII Lock Guards
* VIII.   Lock Policy Selection Helpers
* IX.     Atomic Utilities
*
*
* path:      \inc\container\threadsafe.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                      date: 2026.03.23
******************************************************************************/

#ifndef DJINTERP_CONTAINER_THREADSAFE_
#define DJINTERP_CONTAINER_THREADSAFE_ 1

#include <atomic>
#include <cstddef>
#include <mutex>
#include <type_traits>
#include "..\djinterp.hpp"

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    #include <shared_mutex>
#endif

#if D_ENV_LANG_IS_CPP14_OR_HIGHER
    #include <shared_mutex>
#endif


NS_DJINTERP
NS_CONTAINER

// =============================================================================
// I.   Thread Safety Level Enum
// =============================================================================

// DThreadSafetyLevel
//   enum: classifies the synchronization guarantee a
// container provides.
enum class DThreadSafetyLevel
{
    // no thread safety — single-threaded only
    none,

    // element-level atomics only (lock-free where
    // possible, no mutex)
    atomic_only,

    // exclusive locking — one writer at a time, no
    // concurrent readers during writes
    exclusive,

    // shared locking — multiple concurrent readers,
    // exclusive writer
    shared,

    // timed locking — exclusive with timeout support
    timed,

    // shared timed — reader/writer with timeout
    shared_timed
};


// =============================================================================
// II.  Null Lock Policy (single-threaded)
// =============================================================================

// null_lock_policy
//   struct: a zero-overhead lock policy that performs no
// synchronization.  All lock/unlock operations are no-ops.
// Used for single-threaded builds or when the user
// explicitly opts out of thread safety.
struct null_lock_policy
{
    // --- vocabulary types ---
    struct null_mutex
    {
        void lock()         {}
        void unlock()       {}
        bool try_lock()     { return true; }
    };

    using mutex_type = null_mutex;

    // --- policy constants ---
    static constexpr DThreadSafetyLevel level =
        DThreadSafetyLevel::none;
    static constexpr bool is_threadsafe    = false;
    static constexpr bool supports_shared  = false;
    static constexpr bool supports_timed   = false;

    // --- null guard ---
    struct null_guard
    {
        explicit null_guard(null_mutex&) {}
        ~null_guard() = default;

        null_guard(const null_guard&)            = delete;
        null_guard& operator=(const null_guard&) = delete;
    };

    using read_lock_type  = null_guard;
    using write_lock_type = null_guard;
};


// =============================================================================
// III. Exclusive Lock Policy (std::mutex)
// =============================================================================

// exclusive_lock_policy
//   struct: provides exclusive (non-shared, non-timed)
// locking via std::mutex + std::lock_guard.
struct exclusive_lock_policy
{
    using mutex_type      = std::mutex;
    using read_lock_type  = std::lock_guard<std::mutex>;
    using write_lock_type = std::lock_guard<std::mutex>;

    static constexpr DThreadSafetyLevel level =
        DThreadSafetyLevel::exclusive;
    static constexpr bool is_threadsafe    = true;
    static constexpr bool supports_shared  = false;
    static constexpr bool supports_timed   = false;
};


// =============================================================================
// IV.  Shared Lock Policy (std::shared_mutex, C++17)
// =============================================================================

#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// shared_lock_policy
//   struct: provides reader/writer locking via
// std::shared_mutex.  Multiple concurrent readers are
// allowed; writers are exclusive.
struct shared_lock_policy
{
    using mutex_type      = std::shared_mutex;
    using read_lock_type  =
        std::shared_lock<std::shared_mutex>;
    using write_lock_type =
        std::unique_lock<std::shared_mutex>;

    static constexpr DThreadSafetyLevel level =
        DThreadSafetyLevel::shared;
    static constexpr bool is_threadsafe    = true;
    static constexpr bool supports_shared  = true;
    static constexpr bool supports_timed   = false;
};

#endif  // C++17


// =============================================================================
// V.   Timed Lock Policy (std::timed_mutex)
// =============================================================================

// timed_lock_policy
//   struct: provides exclusive locking with timeout
// support via std::timed_mutex.
struct timed_lock_policy
{
    using mutex_type      = std::timed_mutex;
    using read_lock_type  =
        std::unique_lock<std::timed_mutex>;
    using write_lock_type =
        std::unique_lock<std::timed_mutex>;

    static constexpr DThreadSafetyLevel level =
        DThreadSafetyLevel::timed;
    static constexpr bool is_threadsafe    = true;
    static constexpr bool supports_shared  = false;
    static constexpr bool supports_timed   = true;
};


// =============================================================================
// VI.  Shared Timed Lock Policy
// =============================================================================

#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// shared_timed_lock_policy
//   struct: provides reader/writer locking with timeout
// support via std::shared_timed_mutex.
struct shared_timed_lock_policy
{
    using mutex_type      = std::shared_timed_mutex;
    using read_lock_type  =
        std::shared_lock<std::shared_timed_mutex>;
    using write_lock_type =
        std::unique_lock<std::shared_timed_mutex>;

    static constexpr DThreadSafetyLevel level =
        DThreadSafetyLevel::shared_timed;
    static constexpr bool is_threadsafe    = true;
    static constexpr bool supports_shared  = true;
    static constexpr bool supports_timed   = true;
};

#endif  // C++17


// =============================================================================
// VII. RAII Lock Guards
// =============================================================================
// Generic guards parameterized on the lock policy.  These
// adapt to the policy's mutex type and lock level
// automatically.

// scoped_read_lock
//   class: RAII guard acquiring a read (shared) lock if the
// policy supports it, or an exclusive lock otherwise.
template<typename _Policy>
class scoped_read_lock
{
public:
    using lock_type =
        typename _Policy::read_lock_type;

    explicit scoped_read_lock(
        typename _Policy::mutex_type& _mutex)
        : m_lock(_mutex)
    {
    }

    ~scoped_read_lock() = default;

    scoped_read_lock(const scoped_read_lock&)            = delete;
    scoped_read_lock& operator=(const scoped_read_lock&) = delete;

private:
    lock_type m_lock;
};

// scoped_write_lock
//   class: RAII guard acquiring an exclusive (write) lock.
template<typename _Policy>
class scoped_write_lock
{
public:
    using lock_type =
        typename _Policy::write_lock_type;

    explicit scoped_write_lock(
        typename _Policy::mutex_type& _mutex)
        : m_lock(_mutex)
    {
    }

    ~scoped_write_lock() = default;

    scoped_write_lock(const scoped_write_lock&)            = delete;
    scoped_write_lock& operator=(const scoped_write_lock&) = delete;

private:
    lock_type m_lock;
};

// scoped_try_lock
//   class: RAII guard that attempts a non-blocking lock.
// Exposes owns_lock() to check whether the lock was
// acquired.
template<typename _Policy>
class scoped_try_lock
{
public:
    explicit scoped_try_lock(
        typename _Policy::mutex_type& _mutex)
        : m_lock(_mutex, std::try_to_lock)
    {
    }

    ~scoped_try_lock() = default;

    scoped_try_lock(const scoped_try_lock&)            = delete;
    scoped_try_lock& operator=(const scoped_try_lock&) = delete;

    bool owns_lock() const noexcept
    {
        return m_lock.owns_lock();
    }

    explicit operator bool() const noexcept
    {
        return owns_lock();
    }

private:
    std::unique_lock<
        typename _Policy::mutex_type> m_lock;
};


// =============================================================================
// VIII. Lock Policy Selection Helpers
// =============================================================================
// Compile-time utilities for selecting the best lock
// policy based on requirements.

// default_lock_policy
//   type alias: the default lock policy used when no
// explicit policy is specified.  Uses shared locking on
// C++17+, exclusive otherwise.
#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    using default_lock_policy = shared_lock_policy;
#else
    using default_lock_policy = exclusive_lock_policy;
#endif

// select_lock_policy
//   trait: selects the most capable lock policy that
// satisfies the requested level.
template<DThreadSafetyLevel _Level>
struct select_lock_policy;

template<>
struct select_lock_policy<DThreadSafetyLevel::none>
{
    using type = null_lock_policy;
};

template<>
struct select_lock_policy<DThreadSafetyLevel::exclusive>
{
    using type = exclusive_lock_policy;
};

template<>
struct select_lock_policy<DThreadSafetyLevel::timed>
{
    using type = timed_lock_policy;
};

#if D_ENV_LANG_IS_CPP17_OR_HIGHER

template<>
struct select_lock_policy<DThreadSafetyLevel::shared>
{
    using type = shared_lock_policy;
};

template<>
struct select_lock_policy<
    DThreadSafetyLevel::shared_timed>
{
    using type = shared_timed_lock_policy;
};

#endif  // C++17

template<DThreadSafetyLevel _Level>
using select_lock_policy_t =
    typename select_lock_policy<_Level>::type;


// =============================================================================
// IX.  Atomic Utilities
// =============================================================================
// Lightweight atomic helpers for lock-free container
// metadata (size counters, version stamps, etc.).

// atomic_size
//   type alias: atomic size_t for thread-safe size
// tracking.
using atomic_size = std::atomic<std::size_t>;

// atomic_version
//   type alias: atomic uint64_t for ABA-safe version
// stamping.
using atomic_version = std::atomic<std::uint64_t>;

// atomic_flag_guard
//   class: RAII spinlock using std::atomic_flag.
// Intended for very short critical sections where
// mutex overhead is disproportionate.
class atomic_flag_guard
{
public:
    explicit atomic_flag_guard(
        std::atomic_flag& _flag) noexcept
        : m_flag(_flag)
    {
        while (m_flag.test_and_set(
            std::memory_order_acquire))
        {
            // spin
        }
    }

    ~atomic_flag_guard() noexcept
    {
        m_flag.clear(std::memory_order_release);
    }

    atomic_flag_guard(const atomic_flag_guard&)            = delete;
    atomic_flag_guard& operator=(const atomic_flag_guard&) = delete;

private:
    std::atomic_flag& m_flag;
};


NS_END  // container
NS_END  // djinterp


#endif  // DJINTERP_CONTAINER_THREADSAFE_
