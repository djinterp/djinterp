/*******************************************************************************
* djinterp [core]                              threadsafe_container_concepts.hpp
*
* C++20 concepts for the THREAD SAFETY (the lock surface) axis -- the
* `requires`-facing view of threadsafe_container_traits.hpp.
*
*   THE CONCEPTS ADD NO POLICY.
*   Each is exactly its trait, spelled so it can constrain a template instead
* of gating one through enable_if. The trait stays the single source of truth.
*
*   NAMES.
*   Where the obvious name is taken by a CONTAINER CLASS in this namespace,
* the concept takes an adjective form instead. A concept and a class of the
* same name in one namespace is a hard redeclaration, and this framework has
* already been bitten by that three times.
*
*   PORTABILITY:
*   Gated on C++20 + concepts.
*
*
* path:      /inc/djinterp/core/container/concepts/threadsafe_container_concepts.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.14
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef DJINTERP_CONTAINER_CONCEPTS_THREADSAFE_CONTAINER_CONCEPTS_HPP
#define DJINTERP_CONTAINER_CONCEPTS_THREADSAFE_CONTAINER_CONCEPTS_HPP 1

// FLOOR, FOR NOW: below C++17 this file is empty, rather than an error (round
// 2's rule). The owner's ruling: compile at every level first; port down only
// where something needs it.
#include "../../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// djinterp
#include "../../../djinterp.hpp"
#include "../traits/threadsafe_container_traits.hpp"


#if D_ENV_LANG_IS_CPP20_OR_HIGHER && D_ENV_CPP_FEATURE_LANG_CONCEPTS


NS_DJINTERP

// ==========================================================================
//  THE LOCK SURFACE
// ==========================================================================


// ThreadsafeContainer
//   concept: exposes lock() / unlock(). Named `lockable_`:
// threadsafe_container is a class in this namespace.
template<typename Type>
concept ThreadsafeContainer =
    has_lock_method_v<clean_t<Type>> && has_unlock_method_v<clean_t<Type>>;


// SharedLockableContainer
//   concept: exposes lock_shared() -- many readers, one writer.
template<typename Type>
concept SharedLockableContainer = has_lock_shared_method_v<Type>;


// TryLockableContainer
//   concept: exposes try_lock() -- acquisition that may FAIL rather than
// block, which is the only kind a caller can back out of.
template<typename Type>
concept TryLockableContainer = has_try_lock_method_v<Type>;


// DeclaresLockPolicy
//   concept: carries a lock_policy_type -- it names HOW it is guarded, rather
// than leaving the caller to infer it from the surface.
template<typename Type>
concept DeclaresLockPolicy = has_lock_policy_type_v<Type>;


// VersionedContainer
//   concept: exposes a version stamp -- the basis of optimistic reads, where
// you check afterwards whether what you read was torn.
template<typename Type>
concept VersionedContainer = has_version_method_v<Type>;

NS_END  // djinterp


#endif  // D_ENV_LANG_IS_CPP20_OR_HIGHER && D_ENV_CPP_FEATURE_LANG_CONCEPTS

#endif  // floor, for now

#endif  // DJINTERP_CONTAINER_CONCEPTS_THREADSAFE_CONTAINER_CONCEPTS_HPP
