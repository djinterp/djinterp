/******************************************************************************
* djinterp [test]                                     test_thread_concepts.hpp
*
*   C++20 concepts layered over test_thread_traits.hpp.  These concepts
* provide readable constraints for thread-safe testable types without
* replacing the existing SFINAE trait surface.
*
*   The concepts mirror the public classification axes from
* test_thread_traits.hpp:
*     - nested type aliases     (safety level, strategy tag, lock policy)
*     - lock interface          (read/write/try lock methods)
*     - snapshot / COW          (snapshot, publish methods)
*     - atomic interface        (load, store, compare_exchange)
*     - strategy classification (locked, cow, rcu, hazard, lock-free)
*     - aggregate concepts      (testable, fully-described)
*
*
* path:      /inc/djinterp/test/test_thread_concepts.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.27
******************************************************************************/

#ifndef DJINTERP_TEST_THREAD_CONCEPTS_
#define DJINTERP_TEST_THREAD_CONCEPTS_ 1

#ifndef __cplusplus
    #error "test_thread_concepts.hpp requires C++ compilation"
#endif

// djinterp
#include "../core/djinterp.hpp"
#include "./test_thread_traits.hpp"


#if D_ENV_CPP_FEATURE_LANG_CONCEPTS


NS_DJINTERP
NS_TEST
NS_TRAITS


///////////////////////////////////////////////////////////////////////////////
///                I.   NESTED TYPE ALIAS CONCEPTS                          ///
///////////////////////////////////////////////////////////////////////////////

// safety_level_aware_type
//   concept: the type exposes a thread_safety_level alias.
template<typename _Type>
concept safety_level_aware_type =
    has_thread_safety_level<_Type>::value;

// strategy_tagged_type
//   concept: the type exposes a concurrency_strategy_tag.
template<typename _Type>
concept strategy_tagged_type =
    has_concurrency_strategy_tag<_Type>::value;

// lock_policy_aware_type
//   concept: the type exposes a lock_policy_type alias.
template<typename _Type>
concept lock_policy_aware_type =
    has_lock_policy_type<_Type>::value;

// mutex_aware_type
//   concept: the type exposes a mutex_type alias.
template<typename _Type>
concept mutex_aware_type =
    has_mutex_type<_Type>::value;

// cow_state_aware_type
//   concept: the type exposes a cow_state_type alias.
template<typename _Type>
concept cow_state_aware_type =
    has_cow_state_type<_Type>::value;

// rcu_protected_aware_type
//   concept: the type exposes an rcu_protected_type alias.
template<typename _Type>
concept rcu_protected_aware_type =
    has_rcu_protected_type<_Type>::value;

// hazard_domain_aware_type
//   concept: the type exposes a hazard_domain_type alias.
template<typename _Type>
concept hazard_domain_aware_type =
    has_hazard_domain_type<_Type>::value;


///////////////////////////////////////////////////////////////////////////////
///                II.  LOCK INTERFACE CONCEPTS                             ///
///////////////////////////////////////////////////////////////////////////////

// read_lockable_type
//   concept: the type exposes read_lock() as a const member.
template<typename _Type>
concept read_lockable_type =
    has_read_lock_method<_Type>::value;

// write_lockable_type
//   concept: the type exposes write_lock() as a mutable member.
template<typename _Type>
concept write_lockable_type =
    has_write_lock_method<_Type>::value;

// try_lockable_type
//   concept: the type exposes try_lock().
template<typename _Type>
concept try_lockable_type =
    has_try_lock_method<_Type>::value;

// fully_lockable_type
//   concept: the type exposes both read_lock() and
// write_lock() - full reader/writer interface.
template<typename _Type>
concept fully_lockable_type =
    has_full_lock_interface<_Type>::value;


///////////////////////////////////////////////////////////////////////////////
///                III. SNAPSHOT / COW CONCEPTS                             ///
///////////////////////////////////////////////////////////////////////////////

// snapshottable_type
//   concept: the type exposes snapshot().
template<typename _Type>
concept snapshottable_type =
    has_snapshot_method<_Type>::value;

// publishable_type
//   concept: the type exposes publish().
template<typename _Type>
concept publishable_type =
    has_publish_method<_Type>::value;

// cow_capable_type
//   concept: the type exposes the full COW interface.
template<typename _Type>
concept cow_capable_type =
    has_cow_interface<_Type>::value;


///////////////////////////////////////////////////////////////////////////////
///                IV.  ATOMIC INTERFACE CONCEPTS                           ///
///////////////////////////////////////////////////////////////////////////////

// atomic_loadable_type
//   concept: the type exposes load().
template<typename _Type>
concept atomic_loadable_type =
    has_atomic_load_method<_Type>::value;

// atomic_storable_type
//   concept: the type exposes store(value).
template<typename _Type>
concept atomic_storable_type =
    has_atomic_store_method<_Type>::value;

// atomic_cas_type
//   concept: the type exposes compare_exchange_weak.
template<typename _Type>
concept atomic_cas_type =
    has_atomic_compare_exchange_method<_Type>::value;

// atomic_capable_type
//   concept: the type exposes the full atomic interface
// (load + store).
template<typename _Type>
concept atomic_capable_type =
    has_atomic_interface<_Type>::value;


///////////////////////////////////////////////////////////////////////////////
///                V.   STRATEGY CLASSIFICATION CONCEPTS                    ///
///////////////////////////////////////////////////////////////////////////////

// locked_testable_type
//   concept: the type uses lock-based synchronization.
template<typename _Type>
concept locked_testable_type =
    is_locked_testable<_Type>::value;

// cow_testable_type
//   concept: the type uses copy-on-write synchronization.
template<typename _Type>
concept cow_testable_type =
    is_cow_testable<_Type>::value;

// rcu_testable_type
//   concept: the type uses RCU / epoch-based reclamation.
template<typename _Type>
concept rcu_testable_type =
    is_rcu_testable<_Type>::value;

// hazard_testable_type
//   concept: the type uses hazard-pointer protection.
template<typename _Type>
concept hazard_testable_type =
    is_hazard_testable<_Type>::value;

// lock_free_testable_type
//   concept: the type appears lock-free (atomic OR hazard
// OR RCU).
template<typename _Type>
concept lock_free_testable_type =
    is_lock_free_testable<_Type>::value;


///////////////////////////////////////////////////////////////////////////////
///                VI.  AGGREGATE PROFILE CONCEPTS                          ///
///////////////////////////////////////////////////////////////////////////////

// threadsafe_testable_type
//   concept: shorthand for any type recognized as
// threadsafe by the trait system.
template<typename _Type>
concept threadsafe_testable_type =
    is_threadsafe_testable<_Type>::value;

// classified_thread_test_type
//   concept: shorthand for any type recognized by
// thread_test_class as threadsafe.
template<typename _Type>
concept classified_thread_test_type =
    thread_test_class<_Type>::is_threadsafe;

// fully_described_threadsafe_type
//   concept: a thread-safe type that exposes both type
// aliases AND a method-level interface - the richest
// classification suitable for full-spectrum tests.
template<typename _Type>
concept fully_described_threadsafe_type =
    threadsafe_testable_type<_Type>          &&
    ( safety_level_aware_type<_Type>         ||
      strategy_tagged_type<_Type> )          &&
    ( fully_lockable_type<_Type>             ||
      cow_capable_type<_Type>                ||
      atomic_capable_type<_Type> );

// race_testable_type
//   concept: a type that can meaningfully be subjected to
// race-probing - exposes either a reader/writer interface
// or atomic loads.
template<typename _Type>
concept race_testable_type =
    fully_lockable_type<_Type> ||
    snapshottable_type<_Type>  ||
    atomic_loadable_type<_Type>;


NS_END  // traits
NS_END  // test
NS_END  // djinterp


#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS

#endif  // DJINTERP_TEST_THREAD_CONCEPTS_
