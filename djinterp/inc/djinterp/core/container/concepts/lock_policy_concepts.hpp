/******************************************************************************
* djinterp [container]                                 lock_policy_concepts.hpp
*
*   C++20 concepts for the LOCK-POLICY contract -- the `requires`-facing view of
* sync/lock_policy.hpp.
*
*   THE CONCEPTS ADD NO POLICY.  Each is exactly its trait, spelled so it can
* constrain a template instead of gating one through enable_if.  The trait stays
* the single source of truth.
*
*   NAMES.  meta/concepts.hpp already owns the general type-level concepts (the
* `_c` family), and constexpr_iterator_concepts.hpp the constexpr-iteration
* ones; neither is duplicated here.  Where an obvious name is otherwise taken,
* the concept takes a form that cannot collide -- a concept and a class of one
* name in one namespace is a hard redeclaration.
*
*   PORTABILITY:
*   Gated on C++20 + concepts.  Below that the header is empty and callers use
* the `::value` / `_v` forms directly.
*
*
* path:      /inc/djinterp/core/container/concepts/lock_policy_concepts.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.14
******************************************************************************/

#ifndef DJINTERP_LOCK_POLICY_CONCEPTS_
#define DJINTERP_LOCK_POLICY_CONCEPTS_ 1

// djinterp
#include "../../djinterp.hpp"
#include "../../sync/lock_policy.hpp"


#if D_ENV_LANG_IS_CPP20_OR_HIGHER && D_ENV_CPP_FEATURE_LANG_CONCEPTS


NS_DJINTERP

// ==========================================================================
//  THE POLICY CONTRACT
// ==========================================================================


// LockPolicy
// concept: a well-formed lock policy: it names its mutex_type, read_lock_type
// and write_lock_type, so the RAII guards can dispatch without ever naming a
// concrete mutex. The `_c` suffix follows meta/concepts.hpp, where the general
// type-level concepts live.
template<typename _Type>
concept LockPolicy =
    requires {
        typename clean_t<_Type>::mutex_type;
        typename clean_t<_Type>::read_lock_type;
        typename clean_t<_Type>::write_lock_type;
    };


// SharedLockPolicy
// concept: a policy that permits concurrent readers -- is_shared. A
// reader/writer lock, as against a plain exclusive one.
template<typename _Type>
concept SharedLockPolicy =
    LockPolicy<_Type> && clean_t<_Type>::is_shared;


// TimedLockPolicy
// concept: a policy whose acquisition can time out -- is_timed. The only kind a
// caller can wait on with a bound.
template<typename _Type>
concept TimedLockPolicy =
    LockPolicy<_Type> && clean_t<_Type>::is_timed;


// SynchronizingLockPolicy
// concept: a policy that actually synchronises -- is_threadsafe. The null
// policy is a valid policy that is NOT this, which is exactly what lets the
// same code compile with locking switched off.
template<typename _Type>
concept SynchronizingLockPolicy =
    LockPolicy<_Type> && clean_t<_Type>::is_threadsafe;

NS_END  // djinterp


#endif  // D_ENV_LANG_IS_CPP20_OR_HIGHER && D_ENV_CPP_FEATURE_LANG_CONCEPTS


#endif  // DJINTERP_LOCK_POLICY_CONCEPTS_
