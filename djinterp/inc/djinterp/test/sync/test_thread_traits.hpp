/******************************************************************************
* djinterp [test]                                       test_thread_traits.hpp
*
*   Structural SFINAE detection for types participating in the DTest
* multithreading harness.  These traits classify types along axes that
* matter for concurrent testing:
*     - thread-safety classification     (level / strategy / lock policy)
*     - lock interface presence          (read_lock / write_lock)
*     - snapshot/COW interface presence  (snapshot, immutable_snapshot)
*     - atomic interface presence        (load / store)
*     - hazard / RCU strategy markers
*   The probes mirror the approach in concurrency_strategy_traits.hpp
* but live in the test namespace so the test harness can reason
* about a type's threadsafety without dragging the container metaprogramming
* graph into the test umbrella.
*   All detection is purely structural — expose the right members or
* aliases and the trait system classifies the type automatically.
*
*
* TABLE OF CONTENTS
* =================
* I.    NESTED TYPE DETECTION
* II.   LOCK INTERFACE DETECTION
* III.  SNAPSHOT / COW DETECTION
* IV.   ATOMIC INTERFACE DETECTION
* V.    STRATEGY MARKER DETECTION
* VI.   COMBINED CLASSIFICATION
* VII.  VARIABLE TEMPLATES
*
*
* path:      /inc/djinterp/test/sync/test_thread_traits.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.27
******************************************************************************/

#ifndef DJINTERP_TEST_THREAD_TRAITS_
#define DJINTERP_TEST_THREAD_TRAITS_ 1

// std
#include <cstddef>
#include <type_traits>
// djinterp
#include "../../core/djinterp.hpp"
#include "../test_common.hpp"


NS_DJINTERP
NS_TEST


///////////////////////////////////////////////////////////////////////////////
///                I.   NESTED TYPE DETECTION                               ///
///////////////////////////////////////////////////////////////////////////////

// has_thread_safety_level
//   trait: true if _Type exposes a thread_safety_level
// alias or member typedef.
template<typename _Type,
         typename = void>
struct has_thread_safety_level : std::false_type
{};

template<typename _Type>
struct has_thread_safety_level<_Type, void_t<
    typename _Type::thread_safety_level
>> : std::true_type
{};

// has_concurrency_strategy_tag
//   trait: true if _Type exposes a concurrency_strategy_tag
// alias indicating its synchronization strategy.
template<typename _Type,
         typename = void>
struct has_concurrency_strategy_tag : std::false_type
{};

template<typename _Type>
struct has_concurrency_strategy_tag<_Type, void_t<
    typename _Type::concurrency_strategy_tag
>> : std::true_type
{};

// has_lock_policy_type
//   trait: true if _Type exposes a lock_policy_type alias.
template<typename _Type,
         typename = void>
struct has_lock_policy_type : std::false_type
{};

template<typename _Type>
struct has_lock_policy_type<_Type, void_t<
    typename _Type::lock_policy_type
>> : std::true_type
{};

// has_mutex_type
//   trait: true if _Type exposes a mutex_type alias.
template<typename _Type,
         typename = void>
struct has_mutex_type : std::false_type
{};

template<typename _Type>
struct has_mutex_type<_Type, void_t<
    typename _Type::mutex_type
>> : std::true_type
{};

// has_cow_state_type
//   trait: true if _Type exposes a cow_state_type alias.
template<typename _Type,
         typename = void>
struct has_cow_state_type : std::false_type
{};

template<typename _Type>
struct has_cow_state_type<_Type, void_t<
    typename _Type::cow_state_type
>> : std::true_type
{};

// has_rcu_protected_type
//   trait: true if _Type exposes an rcu_protected_type alias.
template<typename _Type,
         typename = void>
struct has_rcu_protected_type : std::false_type
{};

template<typename _Type>
struct has_rcu_protected_type<_Type, void_t<
    typename _Type::rcu_protected_type
>> : std::true_type
{};

// has_hazard_domain_type
//   trait: true if _Type exposes a hazard_domain_type alias.
template<typename _Type,
         typename = void>
struct has_hazard_domain_type : std::false_type
{};

template<typename _Type>
struct has_hazard_domain_type<_Type, void_t<
    typename _Type::hazard_domain_type
>> : std::true_type
{};


///////////////////////////////////////////////////////////////////////////////
///                II.  LOCK INTERFACE DETECTION                            ///
///////////////////////////////////////////////////////////////////////////////

// has_read_lock_method
//   trait: true if _Type exposes read_lock() as a const
// member returning a guard.
template<typename _Type,
         typename = void>
struct has_read_lock_method : std::false_type
{};

template<typename _Type>
struct has_read_lock_method<_Type, void_t<
    decltype(std::declval<const _Type&>().read_lock())
>> : std::true_type
{};

// has_write_lock_method
//   trait: true if _Type exposes write_lock() as a mutable
// member returning a guard.
template<typename _Type,
         typename = void>
struct has_write_lock_method : std::false_type
{};

template<typename _Type>
struct has_write_lock_method<_Type, void_t<
    decltype(std::declval<_Type&>().write_lock())
>> : std::true_type
{};

// has_try_lock_method
//   trait: true if _Type exposes try_lock() returning an
// optional / pointer guard.
template<typename _Type,
         typename = void>
struct has_try_lock_method : std::false_type
{};

template<typename _Type>
struct has_try_lock_method<_Type, void_t<
    decltype(std::declval<_Type&>().try_lock())
>> : std::true_type
{};

// has_full_lock_interface
//   trait: composite — both read_lock() and write_lock()
// are present.
template<typename _Type>
struct has_full_lock_interface
{
    static D_CONSTEXPR bool value =
        ( has_read_lock_method<_Type>::value &&
          has_write_lock_method<_Type>::value );
};


///////////////////////////////////////////////////////////////////////////////
///                III. SNAPSHOT / COW DETECTION                            ///
///////////////////////////////////////////////////////////////////////////////

// has_snapshot_method
//   trait: true if _Type exposes snapshot() returning an
// immutable view of the current state.
template<typename _Type,
         typename = void>
struct has_snapshot_method : std::false_type
{};

template<typename _Type>
struct has_snapshot_method<_Type, void_t<
    decltype(std::declval<const _Type&>().snapshot())
>> : std::true_type
{};

// has_publish_method
//   trait: true if _Type exposes publish(...) for COW-style
// state replacement.
template<typename _Type,
         typename = void>
struct has_publish_method : std::false_type
{};

template<typename _Type>
struct has_publish_method<_Type, void_t<
    decltype(std::declval<_Type&>().publish())
>> : std::true_type
{};

// has_cow_interface
//   trait: composite — snapshot() and publish() both
// present, indicating COW-style lifecycle.
template<typename _Type>
struct has_cow_interface
{
    static D_CONSTEXPR bool value =
        ( has_snapshot_method<_Type>::value &&
          has_publish_method<_Type>::value );
};


///////////////////////////////////////////////////////////////////////////////
///                IV.  ATOMIC INTERFACE DETECTION                          ///
///////////////////////////////////////////////////////////////////////////////

// has_atomic_load_method
//   trait: true if _Type exposes load() as a const member.
template<typename _Type,
         typename = void>
struct has_atomic_load_method : std::false_type
{};

template<typename _Type>
struct has_atomic_load_method<_Type, void_t<
    decltype(std::declval<const _Type&>().load())
>> : std::true_type
{};

// has_atomic_store_method
//   trait: true if _Type exposes store(value) as a mutable
// member.
template<typename _Type,
         typename = void>
struct has_atomic_store_method : std::false_type
{};

template<typename _Type>
struct has_atomic_store_method<_Type, void_t<
    decltype(std::declval<_Type&>().store(
        std::declval<typename _Type::value_type>()))
>> : std::true_type
{};

// has_atomic_compare_exchange_method
//   trait: true if _Type exposes
// compare_exchange_weak(expected, desired) as a mutable
// member.
template<typename _Type,
         typename = void>
struct has_atomic_compare_exchange_method : std::false_type
{};

template<typename _Type>
struct has_atomic_compare_exchange_method<_Type, void_t<
    decltype(std::declval<_Type&>().compare_exchange_weak(
        std::declval<typename _Type::value_type&>(),
        std::declval<typename _Type::value_type>()))
>> : std::true_type
{};

// has_atomic_interface
//   trait: composite — load() and store() both present.
template<typename _Type>
struct has_atomic_interface
{
    static D_CONSTEXPR bool value =
        ( has_atomic_load_method<_Type>::value &&
          has_atomic_store_method<_Type>::value );
};


///////////////////////////////////////////////////////////////////////////////
///                V.   STRATEGY MARKER DETECTION                           ///
///////////////////////////////////////////////////////////////////////////////
// Composites that classify a type's primary concurrency strategy.

// is_locked_testable
//   trait: true if _Type appears to use lock-based
// synchronization (lock_policy_type alias OR
// read_lock/write_lock methods).
template<typename _Type>
struct is_locked_testable
{
    static D_CONSTEXPR bool value =
        ( has_lock_policy_type<_Type>::value     ||
          has_full_lock_interface<_Type>::value );
};

// is_cow_testable
//   trait: true if _Type appears to use copy-on-write
// (cow_state_type alias OR snapshot()/publish() methods).
template<typename _Type>
struct is_cow_testable
{
    static D_CONSTEXPR bool value =
        ( has_cow_state_type<_Type>::value      ||
          has_cow_interface<_Type>::value );
};

// is_rcu_testable
//   trait: true if _Type exposes RCU markers.
template<typename _Type>
struct is_rcu_testable
{
    static D_CONSTEXPR bool value =
        has_rcu_protected_type<_Type>::value;
};

// is_hazard_testable
//   trait: true if _Type exposes hazard-pointer markers.
template<typename _Type>
struct is_hazard_testable
{
    static D_CONSTEXPR bool value =
        has_hazard_domain_type<_Type>::value;
};

// is_lock_free_testable
//   trait: true if _Type appears to be lock-free (atomic
// interface OR hazard / RCU strategy).
template<typename _Type>
struct is_lock_free_testable
{
    static D_CONSTEXPR bool value =
        ( has_atomic_interface<_Type>::value ||
          is_hazard_testable<_Type>::value   ||
          is_rcu_testable<_Type>::value );
};


///////////////////////////////////////////////////////////////////////////////
///                VI.  COMBINED CLASSIFICATION                             ///
///////////////////////////////////////////////////////////////////////////////

// is_threadsafe_testable
//   trait: true if _Type is recognized as threadsafe under
// any supported strategy.
template<typename _Type>
struct is_threadsafe_testable
{
    static D_CONSTEXPR bool value =
        ( is_locked_testable<_Type>::value     ||
          is_cow_testable<_Type>::value        ||
          is_rcu_testable<_Type>::value        ||
          is_hazard_testable<_Type>::value     ||
          has_atomic_interface<_Type>::value   ||
          has_concurrency_strategy_tag<_Type>::value ||
          has_thread_safety_level<_Type>::value );
};

// thread_test_class
//   struct: comprehensive structural classification of a
// type for the threadsafe testing harness.  Every flag
// is independent and may be queried in isolation.
template<typename _Type>
struct thread_test_class
{
    // nested type aliases
    static D_CONSTEXPR bool has_safety_level =
        has_thread_safety_level<_Type>::value;
    static D_CONSTEXPR bool has_strategy_tag =
        has_concurrency_strategy_tag<_Type>::value;
    static D_CONSTEXPR bool has_lock_policy =
        has_lock_policy_type<_Type>::value;
    static D_CONSTEXPR bool has_mutex =
        has_mutex_type<_Type>::value;
    static D_CONSTEXPR bool has_cow_state =
        has_cow_state_type<_Type>::value;
    static D_CONSTEXPR bool has_rcu_protected =
        has_rcu_protected_type<_Type>::value;
    static D_CONSTEXPR bool has_hazard_domain =
        has_hazard_domain_type<_Type>::value;

    // lock interface
    static D_CONSTEXPR bool has_read_lock =
        has_read_lock_method<_Type>::value;
    static D_CONSTEXPR bool has_write_lock =
        has_write_lock_method<_Type>::value;
    static D_CONSTEXPR bool has_try_lock =
        has_try_lock_method<_Type>::value;
    static D_CONSTEXPR bool has_full_lock =
        has_full_lock_interface<_Type>::value;

    // cow interface
    static D_CONSTEXPR bool has_snapshot =
        has_snapshot_method<_Type>::value;
    static D_CONSTEXPR bool has_publish =
        has_publish_method<_Type>::value;
    static D_CONSTEXPR bool has_cow =
        has_cow_interface<_Type>::value;

    // atomic interface
    static D_CONSTEXPR bool has_load =
        has_atomic_load_method<_Type>::value;
    static D_CONSTEXPR bool has_store =
        has_atomic_store_method<_Type>::value;
    static D_CONSTEXPR bool has_compare_exchange =
        has_atomic_compare_exchange_method<_Type>::value;
    static D_CONSTEXPR bool has_atomic =
        has_atomic_interface<_Type>::value;

    // strategy classification
    static D_CONSTEXPR bool is_locked =
        is_locked_testable<_Type>::value;
    static D_CONSTEXPR bool is_cow =
        is_cow_testable<_Type>::value;
    static D_CONSTEXPR bool is_rcu =
        is_rcu_testable<_Type>::value;
    static D_CONSTEXPR bool is_hazard =
        is_hazard_testable<_Type>::value;
    static D_CONSTEXPR bool is_lock_free =
        is_lock_free_testable<_Type>::value;
    static D_CONSTEXPR bool is_threadsafe =
        is_threadsafe_testable<_Type>::value;
};


///////////////////////////////////////////////////////////////////////////////
///                VII. VARIABLE TEMPLATES                                  ///
///////////////////////////////////////////////////////////////////////////////

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    template<typename _Type>
    D_CONSTEXPR bool has_thread_safety_level_v =
        has_thread_safety_level<_Type>::value;

    template<typename _Type>
    D_CONSTEXPR bool has_concurrency_strategy_tag_v =
        has_concurrency_strategy_tag<_Type>::value;

    template<typename _Type>
    D_CONSTEXPR bool has_lock_policy_type_v =
        has_lock_policy_type<_Type>::value;

    template<typename _Type>
    D_CONSTEXPR bool has_read_lock_method_v =
        has_read_lock_method<_Type>::value;

    template<typename _Type>
    D_CONSTEXPR bool has_write_lock_method_v =
        has_write_lock_method<_Type>::value;

    template<typename _Type>
    D_CONSTEXPR bool has_full_lock_interface_v =
        has_full_lock_interface<_Type>::value;

    template<typename _Type>
    D_CONSTEXPR bool has_snapshot_method_v =
        has_snapshot_method<_Type>::value;

    template<typename _Type>
    D_CONSTEXPR bool has_cow_interface_v =
        has_cow_interface<_Type>::value;

    template<typename _Type>
    D_CONSTEXPR bool has_atomic_interface_v =
        has_atomic_interface<_Type>::value;

    template<typename _Type>
    D_CONSTEXPR bool is_locked_testable_v =
        is_locked_testable<_Type>::value;

    template<typename _Type>
    D_CONSTEXPR bool is_cow_testable_v =
        is_cow_testable<_Type>::value;

    template<typename _Type>
    D_CONSTEXPR bool is_rcu_testable_v =
        is_rcu_testable<_Type>::value;

    template<typename _Type>
    D_CONSTEXPR bool is_hazard_testable_v =
        is_hazard_testable<_Type>::value;

    template<typename _Type>
    D_CONSTEXPR bool is_lock_free_testable_v =
        is_lock_free_testable<_Type>::value;

    template<typename _Type>
    D_CONSTEXPR bool is_threadsafe_testable_v =
        is_threadsafe_testable<_Type>::value;

#endif  // variable templates


NS_END  // test
NS_END  // djinterp


#endif  // DJINTERP_TEST_THREAD_TRAITS_