/******************************************************************************
* djinterp [container]                         threadsafe_container_traits.hpp
*
* Thread-safe container traits for the djinterp framework.
*   Provides compile-time detection of thread-safety capabilities at
* the container level, including lock policy classification, mutex
* type extraction, and synchronization strategy selection.
*   Detection operates at three levels:
*     1. Lock policy:    does the container expose a lock_policy_type
*        alias with the expected structural members (mutex_type,
*        read_lock_type, write_lock_type, is_threadsafe, etc.)?
*     2. Direct locking: does the container itself expose lock(),
*        unlock(), try_lock(), lock_shared(), etc.?
*     3. Atomic state:   does the container use atomic members for
*        lock-free metadata (size, version)?
*   The lockable named requirement traits (is_basic_lockable,
* is_lockable, is_shared_lockable, is_timed_lockable) are defined
* in section III below using structural SFINAE detection.  They
* mirror the standard library's named requirements:
*     BasicLockable    - lock(), unlock()
*     Lockable         - BasicLockable + try_lock()
*     SharedLockable   - lock_shared(), unlock_shared() (C++17)
*     TimedLockable    - try_lock_for(duration)        (C++11)
*   All detection is purely structural SFINAE.
*
*   LAYERING (monograph "Concurrency").  This is the MECHANISM layer: it
* answers HOW a container synchronizes and distils that into a
* thread_safety_level (a lock-policy taxonomy).  The formal concurrency
* AXIS - is a container linearizable, and with what (progress, arity,
* iteration, reclamation) signature - is the SEMANTICS layer's remit and
* lives in concurrency_strategy_traits.hpp, which is built ON TOP of this
* file.  The one formal coordinate the mechanism can supply by itself is the
* progress grade of the lock discipline; section VI.b bridges to it
* (lock_progress_of).  Everything else is deliberately left upstream so the
* two layers do not restate each other.
*
* DEPENDENCIES:
*   container_traits.hpp            - container classification
*   threadsafe.hpp                  - lock policies, thread_safety_level
*   concurrency_strategy_tags.hpp   - concurrency_progress (bridge, VI.b)
*
*
* path:      /inc/djinterp/core/container/traits/
*                threadsafe_container_traits.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.03.23
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.      lock policy detection
II.     lock policy classification
III.    direct locking detection
IV.     atomic state detection
V.      mutex type extraction
VI.     thread safety level deduction
VI.b    monograph progress bridge
VII.    convenience predicates
VIII.   combined classification
*/

#ifndef DJINTERP_THREADSAFE_CONTAINER_TRAITS_
#define DJINTERP_THREADSAFE_CONTAINER_TRAITS_ 1

// std
#include <atomic>
#include <chrono>
#include <cstddef>
#include <type_traits>
// djinterp
#include "../../djinterp.hpp"
#include "../../meta/type_traits.hpp"
#include "../../sync/threadsafe.hpp"
#include "../../sync/concurrency_strategy_tags.hpp"  // concurrency_progress (monograph bridge)
#include "./container_traits.hpp"


NS_DJINTERP


// ===========================================================================
// I.   Lock Policy Detection
// ===========================================================================
// Detects whether a container exposes a lock_policy_type
// alias and whether that policy satisfies the structural
// contract defined in threadsafe.hpp.

// has_lock_policy_type
//   type trait: true if the container exposes a
// lock_policy_type alias.
D_TYPE_TRAIT_DETECTED(has_lock_policy_type,
                  typename _Type::lock_policy_type)

// has_mutex_type_alias
//   type trait: true if the container (or its policy)
// exposes a mutex_type alias.
D_TYPE_TRAIT_DETECTED(has_mutex_type_alias,
                  typename _Type::mutex_type)

NS_INTERNAL
    // --- policy structural checks ---

    // policy_has_mutex_type
    template<typename _Predicate,
             typename = void>
    struct policy_has_mutex_type : std::false_type
    {};

    template<typename _Predicate>
    struct policy_has_mutex_type<_Predicate,
        void_t<typename _Predicate::mutex_type>>
        : std::true_type
    {};

    // policy_has_read_lock
    template<typename _Predicate,
             typename = void>
    struct policy_has_read_lock : std::false_type
    {};

    template<typename _Predicate>
    struct policy_has_read_lock<_Predicate, void_t<typename _Predicate::read_lock_type>>
        : std::true_type
    {};

    // policy_has_write_lock
    template<typename _Predicate, typename = void>
    struct policy_has_write_lock : std::false_type
    {};

    template<typename _Predicate>
    struct policy_has_write_lock<_Predicate,
        void_t<typename _Predicate::write_lock_type>>
        : std::true_type
    {};

    // policy_has_is_threadsafe
    template<typename _Predicate, typename = void>
    struct policy_has_is_threadsafe : std::false_type
    {};

    template<typename _Predicate>
    struct policy_has_is_threadsafe<_Predicate,
        void_t<decltype(_Predicate::is_threadsafe)>>
        : std::true_type
    {};

    // policy_has_level
    template<typename _Predicate, typename = void>
    struct policy_has_level : std::false_type
    {};

    template<typename _Predicate>
    struct policy_has_level<_Predicate,
        void_t<decltype(_Predicate::level)>>
        : std::true_type
    {};

    // policy_has_supports_shared
    template<typename _Predicate, typename = void>
    struct policy_has_supports_shared : std::false_type
    {};

    template<typename _Predicate>
    struct policy_has_supports_shared<_Predicate,
        void_t<decltype(_Predicate::supports_shared)>>
        : std::true_type
    {};

    // policy_has_supports_timed
    template<typename _Predicate, typename = void>
    struct policy_has_supports_timed : std::false_type
    {};

    template<typename _Predicate>
    struct policy_has_supports_timed<_Predicate,
        void_t<decltype(_Predicate::supports_timed)>>
        : std::true_type
    {};

    // is_valid_lock_policy_check
    //   helper: true when a policy type satisfies the
    // minimum structural contract (mutex_type +
    // read_lock_type + write_lock_type + is_threadsafe).
    template<typename _Predicate>
    struct is_valid_lock_policy_check
    {
        static constexpr bool value =
            ( policy_has_mutex_type<_Predicate>::value     &&
              policy_has_read_lock<_Predicate>::value      &&
              policy_has_write_lock<_Predicate>::value     &&
              policy_has_is_threadsafe<_Predicate>::value );
    };

    // safe_lock_policy
    //   helper: extracts lock_policy_type from a container
    // if present, otherwise yields null_lock_policy.
    template<typename _Type, typename = void>
    struct safe_lock_policy
    {
        using type = null_lock_policy;
    };

    template<typename _Type>
    struct safe_lock_policy<_Type, void_t<typename _Type::lock_policy_type>>
    {
        using type = typename _Type::lock_policy_type;
    };

    template<typename _Type>
    using safe_lock_policy_t = typename safe_lock_policy<_Type>::type;

NS_END  // internal

// has_valid_lock_policy
//   type trait: true if the container exposes a
// lock_policy_type that satisfies the structural contract
// (mutex_type, read/write lock types, is_threadsafe flag).
template<typename _Type>
struct has_valid_lock_policy
{
    using clean_type = clean_t<_Type>;

    static constexpr bool value =
        ( has_lock_policy_type_v<clean_type> &&
          internal::is_valid_lock_policy_check<
              internal::safe_lock_policy_t<
                  clean_type>>::value );
};

template<typename _Type>
inline constexpr bool has_valid_lock_policy_v =
    has_valid_lock_policy<_Type>::value;


// ===========================================================================
// II.  Lock Policy Classification
// ===========================================================================
// Queries the policy's static constexpr members to
// classify its capabilities.

// policy_is_threadsafe
//   type trait: true if the container's lock policy has
// is_threadsafe == true.
template<typename _Type>
struct policy_is_threadsafe
{
    using clean_type = clean_t<_Type>;
    using policy =
        internal::safe_lock_policy_t<clean_type>;

    static constexpr bool value =
        ( has_valid_lock_policy_v<clean_type> &&
          policy::is_threadsafe );
};

template<typename _Type>
inline constexpr bool policy_is_threadsafe_v =
    policy_is_threadsafe<_Type>::value;

// policy_supports_shared
//   type trait: true if the container's lock policy
// supports reader/writer (shared) locking.
template<typename _Type>
struct policy_supports_shared
{
    using clean_type = clean_t<_Type>;
    using policy =
        internal::safe_lock_policy_t<clean_type>;

    static constexpr bool value =
        ( has_valid_lock_policy_v<clean_type> &&
          internal::policy_has_supports_shared<
              policy>::value                  &&
          policy::supports_shared );
};

template<typename _Type>
inline constexpr bool policy_supports_shared_v =
    policy_supports_shared<_Type>::value;

// policy_supports_timed
//   type trait: true if the container's lock policy
// supports timeout-based locking.
template<typename _Type>
struct policy_supports_timed
{
    using clean_type = clean_t<_Type>;
    using policy =
        internal::safe_lock_policy_t<clean_type>;

    static constexpr bool value =
        ( has_valid_lock_policy_v<clean_type> &&
          internal::policy_has_supports_timed<
              policy>::value                  &&
          policy::supports_timed );
};

template<typename _Type>
inline constexpr bool policy_supports_timed_v =
    policy_supports_timed<_Type>::value;


// ===========================================================================
// III. Direct Locking Detection
// ===========================================================================
// Detects whether the container itself exposes lock/unlock
// methods (as opposed to delegating to a policy).
//
// Section III.A defines the lockable named requirement
// traits (BasicLockable, Lockable, SharedLockable,
// TimedLockable).  Section III.B applies them to container
// types via the is_directly_* family.

// ---------------------------------------------------------------------------
// III.A  Lockable Named Requirement Traits
// ---------------------------------------------------------------------------
// Structural SFINAE detection mirroring the standard
// library's lockable named requirements.  Detection is
// purely on the presence of the required member functions;
// the return types are not constrained, matching the named
// requirement specifications.

// has_lock_method
//   type trait: true if _Type exposes a .lock() method.
D_TYPE_TRAIT_DETECTED(has_lock_method,
    decltype(std::declval<_Type&>().lock()))

// has_unlock_method
//   type trait: true if _Type exposes a .unlock() method.
D_TYPE_TRAIT_DETECTED(has_unlock_method,
    decltype(std::declval<_Type&>().unlock()))

// has_try_lock_method
//   type trait: true if _Type exposes a .try_lock() method.
D_TYPE_TRAIT_DETECTED(has_try_lock_method,
    decltype(std::declval<_Type&>().try_lock()))

// has_lock_shared_method
//   type trait: true if _Type exposes a .lock_shared()
// method.
D_TYPE_TRAIT_DETECTED(has_lock_shared_method,
    decltype(std::declval<_Type&>().lock_shared()))

// has_unlock_shared_method
//   type trait: true if _Type exposes a .unlock_shared()
// method.
D_TYPE_TRAIT_DETECTED(has_unlock_shared_method,
    decltype(std::declval<_Type&>().unlock_shared()))

// has_try_lock_shared_method
//   type trait: true if _Type exposes a .try_lock_shared()
// method.
D_TYPE_TRAIT_DETECTED(has_try_lock_shared_method,
    decltype(std::declval<_Type&>().try_lock_shared()))

NS_INTERNAL

    // has_try_lock_for_helper
    //   helper: detects try_lock_for(duration) by probing
    // with a concrete std::chrono::nanoseconds argument.
    template<typename _Type, typename = void>
    struct has_try_lock_for_helper : std::false_type
    {};

    template<typename _Type>
    struct has_try_lock_for_helper<_Type,
        void_t<decltype(std::declval<_Type&>().try_lock_for(
            std::declval<std::chrono::nanoseconds>()))>>
        : std::true_type
    {};

NS_END  // internal

// has_try_lock_for_method
//   type trait: true if _Type exposes try_lock_for(duration).
template<typename _Type>
struct has_try_lock_for_method
{
    static constexpr bool value =
        internal::has_try_lock_for_helper<_Type>::value;
};

template<typename _Type>
inline constexpr bool has_try_lock_for_method_v =
    has_try_lock_for_method<_Type>::value;

// is_basic_lockable
//   type trait: true if _Type satisfies the BasicLockable
// named requirement: exposes lock() and unlock().
template<typename _Type>
struct is_basic_lockable
{
    static constexpr bool value =
        ( has_lock_method<_Type>::value    &&
          has_unlock_method<_Type>::value );
};

template<typename _Type>
inline constexpr bool is_basic_lockable_v =
    is_basic_lockable<_Type>::value;

// is_lockable
//   type trait: true if _Type satisfies the Lockable named
// requirement: BasicLockable + try_lock().
template<typename _Type>
struct is_lockable
{
    static constexpr bool value =
        ( is_basic_lockable<_Type>::value      &&
          has_try_lock_method<_Type>::value );
};

template<typename _Type>
inline constexpr bool is_lockable_v =
    is_lockable<_Type>::value;

// is_shared_lockable
//   type trait: true if _Type satisfies the SharedLockable
// named requirement: lock_shared() and unlock_shared().
template<typename _Type>
struct is_shared_lockable
{
    static constexpr bool value =
        ( has_lock_shared_method<_Type>::value    &&
          has_unlock_shared_method<_Type>::value );
};

template<typename _Type>
inline constexpr bool is_shared_lockable_v =
    is_shared_lockable<_Type>::value;

// is_timed_lockable
//   type trait: true if _Type satisfies the TimedLockable
// named requirement: BasicLockable + try_lock_for(duration).
template<typename _Type>
struct is_timed_lockable
{
    static constexpr bool value =
        ( is_basic_lockable<_Type>::value          &&
          has_try_lock_for_method<_Type>::value );
};

template<typename _Type>
inline constexpr bool is_timed_lockable_v =
    is_timed_lockable<_Type>::value;


// ---------------------------------------------------------------------------
// III.B  Direct Lockable Container Detection
// ---------------------------------------------------------------------------

// is_directly_lockable
//   type trait: true if the container itself satisfies the
// BasicLockable named requirement (has lock()/unlock()).
template<typename _Type>
struct is_directly_lockable
{
    using clean_type = clean_t<_Type>;

    static constexpr bool value =
        djinterp::is_basic_lockable<
            clean_type>::value;
};

template<typename _Type>
inline constexpr bool is_directly_lockable_v =
    is_directly_lockable<_Type>::value;

// is_directly_shared_lockable
//   type trait: true if the container itself satisfies the
// SharedLockable named requirement.
template<typename _Type>
struct is_directly_shared_lockable
{
    using clean_type = clean_t<_Type>;

    static constexpr bool value =
        djinterp::is_shared_lockable<
            clean_type>::value;
};

template<typename _Type>
inline constexpr bool is_directly_shared_lockable_v =
    is_directly_shared_lockable<_Type>::value;

// is_directly_timed_lockable
//   type trait: true if the container itself satisfies the
// TimedLockable named requirement.
template<typename _Type>
struct is_directly_timed_lockable
{
    using clean_type = clean_t<_Type>;

    static constexpr bool value =
        djinterp::is_timed_lockable<
            clean_type>::value;
};

template<typename _Type>
inline constexpr bool is_directly_timed_lockable_v =
    is_directly_timed_lockable<_Type>::value;

// has_get_mutex_method
//   type trait: true if the container exposes a
// .get_mutex() or .mutex() method returning a reference
// to its internal mutex.
D_TYPE_TRAIT_DETECTED(has_get_mutex_method,
    decltype(std::declval<_Type&>().mutex()))

D_TYPE_TRAIT_DETECTED(has_get_mutex_accessor,
    decltype(std::declval<_Type&>().get_mutex()))


// ===========================================================================
// IV.  Atomic State Detection
// ===========================================================================
// Detects whether the container uses atomic members for
// lock-free metadata tracking.

// has_atomic_size
//   type trait: true if the container exposes an
// atomic_size_type alias or uses std::atomic<size_t>
// for its size member.
D_TYPE_TRAIT_DETECTED(has_atomic_size_type,
                  typename _Type::atomic_size_type)

// has_atomic_version
//   type trait: true if the container exposes a version
// stamp via std::atomic for ABA-safe operations.
D_TYPE_TRAIT_DETECTED(has_atomic_version_type,
                  typename _Type::atomic_version_type)

// has_version_method
//   type trait: true if the container has a .version()
// const method returning a version stamp.
D_TYPE_TRAIT_DETECTED(has_version_method,
                  decltype(std::declval<const _Type&>().version()))


// ===========================================================================
// V.   Mutex Type Extraction
// ===========================================================================
// SFINAE-safe extraction of the mutex type from a
// container's lock policy or direct members.

NS_INTERNAL

    template<typename _Type, typename = void>
    struct container_mutex_type_impl
    {
        using type = void;
    };

    // priority 1: lock_policy_type::mutex_type
    template<typename _Type>
    struct container_mutex_type_impl<_Type,
        std::enable_if_t<
            has_lock_policy_type_v<_Type> &&
            policy_has_mutex_type<
                safe_lock_policy_t<_Type>>::value>>
    {
        using type =
            typename safe_lock_policy_t<
                _Type>::mutex_type;
    };

    // priority 2: direct mutex_type alias
    template<typename _Type>
    struct container_mutex_type_impl<_Type,
        std::enable_if_t<
            !has_lock_policy_type_v<_Type> &&
            has_mutex_type_alias_v<_Type>>>
    {
        using type = typename _Type::mutex_type;
    };

NS_END  // internal

// container_mutex_type
//   type trait: extracts the mutex type from a container,
// yielding void if none is available.
template<typename _Type>
struct container_mutex_type
{
    using type =
        typename internal::container_mutex_type_impl<
            clean_t<_Type>>::type;
};

template<typename _Type>
using container_mutex_type_t =
    typename container_mutex_type<_Type>::type;

// lock_policy_of
//   type trait: extracts the lock policy from a container,
// yielding null_lock_policy if none is declared.
template<typename _Type>
struct lock_policy_of
{
    using type =
        internal::safe_lock_policy_t<clean_t<_Type>>;
};

template<typename _Type>
using lock_policy_of_t =
    typename lock_policy_of<_Type>::type;


// ===========================================================================
// VI.  Thread Safety Level Deduction
// ===========================================================================
// Determines the effective thread_safety_level for a
// container based on its detected capabilities.

NS_INTERNAL

    template<typename _Type>
    struct thread_safety_level_impl
    {
        using clean_type = clean_t<_Type>;
        using policy = safe_lock_policy_t<clean_type>;

        static constexpr thread_safety_level value =

            // policy with explicit level
            ( has_valid_lock_policy_v<clean_type> &&
              policy_has_level<policy>::value )
                ? policy::level

            // directly shared + timed lockable
            : ( is_directly_shared_lockable_v<
                    clean_type> &&
                is_directly_timed_lockable_v<
                    clean_type> )
                ? thread_safety_level::shared_timed

            // directly shared lockable
            : is_directly_shared_lockable_v<clean_type>
                ? thread_safety_level::shared

            // directly timed lockable
            : is_directly_timed_lockable_v<clean_type>
                ? thread_safety_level::timed

            // directly exclusively lockable
            : is_directly_lockable_v<clean_type>
                ? thread_safety_level::exclusive

            // has atomic state only
            : ( has_atomic_size_type_v<clean_type> ||
                has_atomic_version_type_v<clean_type> )
                ? thread_safety_level::atomic_only

            : thread_safety_level::none;
    };

NS_END  // internal

// container_thread_safety_level
//   type trait: deduces the effective thread safety level
// for a container.
template<typename _Type>
struct container_thread_safety_level
{
    static constexpr thread_safety_level value =
        internal::thread_safety_level_impl<
            _Type>::value;
};

template<typename _Type>
inline constexpr thread_safety_level
    container_thread_safety_level_v =
        container_thread_safety_level<_Type>::value;


// ===========================================================================
// VI.b Monograph Progress Bridge
// ===========================================================================
//   This file is the MECHANISM layer: it detects HOW a container
// synchronizes (lock policy, lockable requirements, mutex, atomic state) and
// distils it into thread_safety_level.  That level is a lock-policy taxonomy,
// not one of the monograph's concurrency coordinates.  The single bridge to
// the formal vocabulary that the mechanism alone can supply is the PROGRESS
// grade of the lock discipline:
//
//     none          -> sequential   (safe only under a single agent)
//     atomic_only   -> lock_free     (lock-free metadata; no held lock)
//     exclusive     -> blocking      (an agent waits on a held lock)
//     timed         -> blocking
//     shared        -> blocking      (readers concurrent - see
//     shared_timed  -> blocking       supports_concurrent_reads - but a
//                                     writer still blocks)
//
//   The rest of the monograph signature - ARITY, ITERATION overlap-
// semantics, and the RECLAMATION obligation - is NOT derivable from the lock
// mechanism (a lock-free or immutable container carries level `none` yet is
// concurrent), so it is deliberately left to the strategy layer,
// concurrency_strategy_traits.hpp, which maps a strategy to a full
// concurrency_signature.  Keeping the bridge this thin is what prevents the
// two layers from duplicating each other.
//   The reader-side arity refinement the mechanism DOES know
// (shared -> concurrent readers) is already exposed as
// supports_concurrent_reads below.

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

NS_INTERNAL

    // lock_progress_impl
    //   helper: maps a container's deduced thread_safety_level to the
    // progress grade of its lock discipline.
    template<typename _Type>
    struct lock_progress_impl
    {
        static constexpr thread_safety_level lvl =
            container_thread_safety_level_v<clean_t<_Type>>;

        static constexpr concurrency_progress value =
            ( lvl == thread_safety_level::none )
                ? concurrency_progress::sequential
            : ( lvl == thread_safety_level::atomic_only )
                ? concurrency_progress::lock_free
                : concurrency_progress::blocking;
    };

NS_END  // internal

// lock_progress_of
//   type trait: the monograph progress grade (sequential / lock_free /
// blocking) that a container's LOCK discipline provides.  This is the only
// concurrency-axis coordinate the mechanism layer can supply on its own;
// arity, iteration, and reclamation come from the strategy layer.
template<typename _Type>
struct lock_progress_of
{
    static constexpr concurrency_progress value =
        internal::lock_progress_impl<_Type>::value;
};

template<typename _Type>
inline constexpr concurrency_progress lock_progress_of_v =
    lock_progress_of<_Type>::value;

#endif  // C++11


// ===========================================================================
// VII. Convenience Predicates
// ===========================================================================

// is_threadsafe_container
//   type trait: true if the container provides any form of
// thread safety (policy-based, direct locking, or atomic
// state).
template<typename _Type>
struct is_threadsafe_container
{
    static constexpr bool value =
        ( container_thread_safety_level_v<_Type> !=
          thread_safety_level::none );
};

template<typename _Type>
inline constexpr bool is_threadsafe_container_v =
    is_threadsafe_container<_Type>::value;

// is_non_threadsafe_container
//   type trait: true if the container has no thread safety
// mechanisms.
template<typename _Type>
struct is_non_threadsafe_container
{
    static constexpr bool value =
        ( container_thread_safety_level_v<_Type> ==
          thread_safety_level::none );
};

template<typename _Type>
inline constexpr bool is_non_threadsafe_container_v =
    is_non_threadsafe_container<_Type>::value;

// supports_concurrent_reads
//   type trait: true if the container supports multiple
// concurrent readers (shared locking or atomic-only).
template<typename _Type>
struct supports_concurrent_reads
{
    using clean_type = clean_t<_Type>;

    static constexpr bool value =
        ( policy_supports_shared_v<clean_type>        ||
          is_directly_shared_lockable_v<clean_type>   ||
          ( container_thread_safety_level_v<
                clean_type> ==
            thread_safety_level::atomic_only ) );
};

template<typename _Type>
inline constexpr bool supports_concurrent_reads_v =
    supports_concurrent_reads<_Type>::value;

// supports_timed_locking
//   type trait: true if the container supports
// timeout-based lock acquisition.
template<typename _Type>
struct supports_timed_locking
{
    using clean_type = clean_t<_Type>;

    static constexpr bool value =
        ( policy_supports_timed_v<clean_type> ||
          is_directly_timed_lockable_v<clean_type> );
};

template<typename _Type>
inline constexpr bool supports_timed_locking_v =
    supports_timed_locking<_Type>::value;

// has_version_tracking
//   type trait: true if the container tracks a version
// stamp for optimistic concurrency control or ABA
// prevention.
template<typename _Type>
struct has_version_tracking
{
    using clean_type = clean_t<_Type>;

    static constexpr bool value =
        ( has_atomic_version_type_v<clean_type> ||
          has_version_method_v<clean_type> );
};

template<typename _Type>
inline constexpr bool has_version_tracking_v =
    has_version_tracking<_Type>::value;


// ===========================================================================
// VIII. Combined Classification
// ===========================================================================

// container_threadsafe_class
//   struct: complete thread-safety classification of a
// container type.  All members are static constexpr.
template<typename _Type>
struct container_threadsafe_class
{
    // lock policy
    static constexpr bool has_policy =
        has_valid_lock_policy_v<_Type>;
    static constexpr bool policy_threadsafe =
        policy_is_threadsafe_v<_Type>;
    static constexpr bool policy_shared =
        policy_supports_shared_v<_Type>;
    static constexpr bool policy_timed =
        policy_supports_timed_v<_Type>;

    // direct locking
    static constexpr bool directly_lockable =
        is_directly_lockable_v<_Type>;
    static constexpr bool directly_shared =
        is_directly_shared_lockable_v<_Type>;
    static constexpr bool directly_timed =
        is_directly_timed_lockable_v<_Type>;
    static constexpr bool has_mutex_accessor =
        ( has_get_mutex_method_v<_Type> ||
          has_get_mutex_accessor_v<_Type> );

    // atomic state
    static constexpr bool has_atomic_size =
        has_atomic_size_type_v<_Type>;
    static constexpr bool has_atomic_version =
        has_atomic_version_type_v<_Type>;
    static constexpr bool has_version =
        has_version_tracking_v<_Type>;

    // deduced level
    static constexpr thread_safety_level level =
        container_thread_safety_level_v<_Type>;

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    // monograph progress bridge (the lock discipline's progress grade;
    // arity / iteration / reclamation are the strategy layer's remit)
    static constexpr concurrency_progress lock_progress =
        lock_progress_of_v<_Type>;
#endif  // C++11

    // aggregate
    static constexpr bool is_threadsafe =
        is_threadsafe_container_v<_Type>;
    static constexpr bool concurrent_reads =
        supports_concurrent_reads_v<_Type>;
    static constexpr bool timed_locking =
        supports_timed_locking_v<_Type>;
};


NS_END  // djinterp


#endif  // DJINTERP_THREADSAFE_CONTAINER_TRAITS_