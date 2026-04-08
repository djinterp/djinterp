/******************************************************************************
* djinterp [threadsafe]                                        lock_policy.hpp
*
* Lock policy definitions for the thread-safe framework.
*   Provides a hierarchy of lock policies that wrap different mutex types,
* each exposing a uniform interface for RAII guard construction.  Policies
* are selected at compile time via template parameters; the null_lock_policy
* compiles to zero instructions, enabling the same code to be
* used in both single-threaded and multi-threaded contexts.
*
* THREAD SAFETY LEVEL HIERARCHY:
*   none            — no synchronization (single-threaded)
*   atomic_only     — lock-free atomics only (no mutex)
*   exclusive       — std::mutex (C++11) or platform mutex
*   timed           — std::timed_mutex (C++11)
*   shared          — std::shared_mutex (C++17)
*   shared_timed    — std::shared_timed_mutex (C++14/17)
*
* POLICY STRUCTS:
*   null_lock_policy         — no-op (all lock/unlock inlined away)
*   exclusive_lock_policy    — std::mutex-backed exclusive locking
*   timed_lock_policy        — std::timed_mutex-backed timed locking
*   shared_lock_policy       — std::shared_mutex reader/writer locking
*   shared_timed_lock_policy — std::shared_timed_mutex
*
* SELECTORS:
*   select_lock_policy<Level> — maps DThreadSafetyLevel to a policy type
*   default_lock_policy       — alias for the project default
*
* VERSIONING:
*   C++98/03:  null_lock_policy only (no <mutex>)
*   C++11:     + exclusive_lock_policy, timed_lock_policy
*   C++14:     + shared_timed_lock_policy
*   C++17:     + shared_lock_policy
*
*
* path:      /inc/djinterp/sync/lock_policy.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2026.04.07
******************************************************************************/

#ifndef DJINTERP_THREADSAFE_LOCK_POLICY_
#define DJINTERP_THREADSAFE_LOCK_POLICY_ 1

#ifndef DJINTERP_ENVIRONMENT_
    #error "lock_policy.hpp requires env.h to be included first"
#endif

#ifndef __cplusplus
    #error "lock_policy.hpp can only be used in C++ compilation mode"
#endif

#include <cstddef>

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    #include <mutex>
#endif

#if D_ENV_LANG_IS_CPP14_OR_HIGHER
    #include <shared_mutex>
#endif

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    #include <shared_mutex>
#endif


// NS_THREADSAFE
//   namespace: the `threadsafe` namespace for general-purpose
// synchronization primitives (lock policies, guards, atomics,
// COW, hazard pointers, RCU).  Container-agnostic.
#ifndef NS_THREADSAFE
    #define NS_THREADSAFE   D_NAMESPACE(threadsafe)
#endif


NS_DJINTERP
NS_THREADSAFE

// =========================================================================
// I.   THREAD SAFETY LEVEL ENUM
// =========================================================================

// DThreadSafetyLevel
//   enum: ordered hierarchy of thread-safety guarantees.
// Used by the trait system to classify types and by
// select_lock_policy to map levels to concrete policy types.
//
// The ordering is significant: each level is a strict
// superset of the one below it.
#if D_ENV_LANG_IS_CPP11_OR_HIGHER

enum class DThreadSafetyLevel
{
    none         = 0,
    atomic_only  = 1,
    exclusive    = 2,
    timed        = 3,
    shared       = 4,
    shared_timed = 5
};

// Alias for backward compatibility and trait queries.
using thread_safety_level = DThreadSafetyLevel;

#else

// C++98: simulate with struct + constants
struct DThreadSafetyLevel
{
    enum value_type
    {
        none         = 0,
        atomic_only  = 1,
        exclusive    = 2,
        timed        = 3,
        shared       = 4,
        shared_timed = 5
    };
};

typedef DThreadSafetyLevel::value_type thread_safety_level;

#endif  // C++11


// =========================================================================
// II.  NULL LOCK POLICY
// =========================================================================

// no_op_mutex
//   struct: zero-cost mutex substitute.  Every operation
// is a no-op, compiled to nothing.  Used by
// null_lock_policy for single-threaded use.
struct no_op_mutex
{
    void lock()           {}
    void unlock()         {}
    bool try_lock()       { return true; }

    // shared interface (no-ops)
    void lock_shared()    {}
    void unlock_shared()  {}
};

// no_op_guard
//   struct: zero-cost RAII guard substitute.  Holds a
// reference to the no-op mutex for interface consistency.
struct no_op_guard
{
    explicit no_op_guard(no_op_mutex& /*unused*/) {}
    ~no_op_guard() {}

    no_op_guard(const no_op_guard&)            D_DELETE;
    no_op_guard& operator=(const no_op_guard&) D_DELETE;
};

// null_lock_policy
//   struct: lock policy that provides no synchronization.
// All operations compile to nothing.  This is the default
// for single-threaded use and allows the same
// implementation to be used without locking
// overhead.
struct null_lock_policy
{
    // --- type aliases ---
    typedef no_op_mutex mutex_type;
    typedef no_op_guard read_lock_type;
    typedef no_op_guard write_lock_type;

    // --- policy descriptors ---
    static const bool is_threadsafe = false;
    static const bool is_shared     = false;
    static const bool is_timed      = false;

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    static constexpr DThreadSafetyLevel level =
        DThreadSafetyLevel::none;
#endif
};


// =========================================================================
// III. EXCLUSIVE LOCK POLICY (C++11+)
// =========================================================================

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// exclusive_lock_policy
//   struct: wraps std::mutex for exclusive (writer-only)
// locking.  Both read and write operations acquire the
// same exclusive lock — no reader concurrency.
struct exclusive_lock_policy
{
    // --- type aliases ---
    using mutex_type      = std::mutex;
    using read_lock_type  = std::unique_lock<std::mutex>;
    using write_lock_type = std::unique_lock<std::mutex>;

    // --- policy descriptors ---
    static constexpr bool is_threadsafe = true;
    static constexpr bool is_shared     = false;
    static constexpr bool is_timed      = false;

    static constexpr DThreadSafetyLevel level =
        DThreadSafetyLevel::exclusive;
};


// =========================================================================
// IV.  TIMED LOCK POLICY (C++11+)
// =========================================================================

// timed_lock_policy
//   struct: wraps std::timed_mutex for exclusive locking
// with timeout support.  Enables try_lock_for /
// try_lock_until on the underlying mutex.
struct timed_lock_policy
{
    // --- type aliases ---
    using mutex_type      = std::timed_mutex;
    using read_lock_type  = std::unique_lock<std::timed_mutex>;
    using write_lock_type = std::unique_lock<std::timed_mutex>;

    // --- policy descriptors ---
    static constexpr bool is_threadsafe = true;
    static constexpr bool is_shared     = false;
    static constexpr bool is_timed      = true;

    static constexpr DThreadSafetyLevel level =
        DThreadSafetyLevel::timed;
};

#endif  // C++11


// =========================================================================
// V.   SHARED LOCK POLICY (C++17+)
// =========================================================================

#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// shared_lock_policy
//   struct: wraps std::shared_mutex for reader/writer
// locking.  Multiple readers can hold the lock
// concurrently; writers are exclusive.
struct shared_lock_policy
{
    // --- type aliases ---
    using mutex_type      = std::shared_mutex;
    using read_lock_type  = std::shared_lock<std::shared_mutex>;
    using write_lock_type = std::unique_lock<std::shared_mutex>;

    // --- policy descriptors ---
    static constexpr bool is_threadsafe = true;
    static constexpr bool is_shared     = true;
    static constexpr bool is_timed      = false;

    static constexpr DThreadSafetyLevel level =
        DThreadSafetyLevel::shared;
};

#endif  // C++17


// =========================================================================
// VI.  SHARED TIMED LOCK POLICY (C++14+)
// =========================================================================

#if D_ENV_LANG_IS_CPP14_OR_HIGHER

// shared_timed_lock_policy
//   struct: wraps std::shared_timed_mutex for reader/writer
// locking with timeout support.  The most capable policy —
// supports concurrent readers, exclusive writers, and
// timed lock acquisition.
struct shared_timed_lock_policy
{
    // --- type aliases ---
    using mutex_type      = std::shared_timed_mutex;
    using read_lock_type  = std::shared_lock<std::shared_timed_mutex>;
    using write_lock_type = std::unique_lock<std::shared_timed_mutex>;

    // --- policy descriptors ---
    static constexpr bool is_threadsafe = true;
    static constexpr bool is_shared     = true;
    static constexpr bool is_timed      = true;

    static constexpr DThreadSafetyLevel level =
        DThreadSafetyLevel::shared_timed;
};

#endif  // C++14


// =========================================================================
// VII. POLICY SELECTOR
// =========================================================================

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

NS_INTERNAL

    // select_lock_policy_impl
    //   trait: primary template (unspecialized).
    template<DThreadSafetyLevel _Level>
    struct select_lock_policy_impl;

    // none
    template<>
    struct select_lock_policy_impl<DThreadSafetyLevel::none>
    {
        using type = null_lock_policy;
    };

    // atomic_only — no mutex, use null_lock_policy
    template<>
    struct select_lock_policy_impl<DThreadSafetyLevel::atomic_only>
    {
        using type = null_lock_policy;
    };

    // exclusive
    template<>
    struct select_lock_policy_impl<DThreadSafetyLevel::exclusive>
    {
        using type = exclusive_lock_policy;
    };

    // timed
    template<>
    struct select_lock_policy_impl<DThreadSafetyLevel::timed>
    {
        using type = timed_lock_policy;
    };

#if D_ENV_LANG_IS_CPP17_OR_HIGHER

    // shared
    template<>
    struct select_lock_policy_impl<DThreadSafetyLevel::shared>
    {
        using type = shared_lock_policy;
    };

#else

    // shared falls back to exclusive pre-C++17
    template<>
    struct select_lock_policy_impl<DThreadSafetyLevel::shared>
    {
        using type = exclusive_lock_policy;
    };

#endif  // C++17

#if D_ENV_LANG_IS_CPP14_OR_HIGHER

    // shared_timed
    template<>
    struct select_lock_policy_impl<DThreadSafetyLevel::shared_timed>
    {
        using type = shared_timed_lock_policy;
    };

#else

    // shared_timed falls back to timed pre-C++14
    template<>
    struct select_lock_policy_impl<DThreadSafetyLevel::shared_timed>
    {
        using type = timed_lock_policy;
    };

#endif  // C++14

NS_END  // internal

// select_lock_policy
//   type: maps a DThreadSafetyLevel to the corresponding
// lock policy struct.  Falls back to the highest available
// policy when the requested level is not supported by the
// current C++ standard.
template<DThreadSafetyLevel _Level>
using select_lock_policy =
    typename internal::select_lock_policy_impl<_Level>::type;

#endif  // C++11


// =========================================================================
// VIII. DEFAULT LOCK POLICY
// =========================================================================
// The project default is shared locking when available
// (C++17+), exclusive otherwise, null pre-C++11.

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    using default_lock_policy = shared_lock_policy;
#elif D_ENV_LANG_IS_CPP11_OR_HIGHER
    using default_lock_policy = exclusive_lock_policy;
#else
    typedef null_lock_policy default_lock_policy;
#endif


NS_END  // threadsafe
NS_END  // djinterp


#endif  // DJINTERP_THREADSAFE_LOCK_POLICY_
