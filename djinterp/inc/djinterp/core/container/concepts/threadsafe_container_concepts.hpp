/******************************************************************************
* djinterp [container] threadsafe_container_concepts.hpp C++20 concepts for the
* THREAD SAFETY (the lock surface) axis -- the `requires`-facing view of
* threadsafe_container_traits.hpp. THE CONCEPTS ADD NO POLICY. Each is exactly
* its trait, spelled so it can constrain a template instead of gating one
* through enable_if. The trait stays the single source of truth. NAMES. Where
* the obvious name is taken by a CONTAINER CLASS in this namespace, the concept
* takes an adjective form instead. A concept and a class of the same name in one
* namespace is a hard redeclaration, and this framework has already been bitten
* by that three times. PORTABILITY: Gated on C++20 + concepts. Below that the
* header is empty and callers use the `::value` / `_v` forms directly. path:
* /inc/djinterp/core/container/concepts/threadsafe_container_concepts.hpp
* link(s): TBA author(s): Samuel 'teer' Neal-Blim created: 2026.07.14
* *****************************************************************************/

#ifndef DJINTERP_THREADSAFE_CONTAINER_CONCEPTS_
#define DJINTERP_THREADSAFE_CONTAINER_CONCEPTS_ 1

// djinterp
#include "../../djinterp.hpp"
#include "../../meta/concepts.hpp"   // D_CONCEPT_FROM_TRAIT
#include "../traits/threadsafe_container_traits.hpp"


#if D_ENV_LANG_IS_CPP20_OR_HIGHER && D_ENV_CPP_FEATURE_LANG_CONCEPTS


NS_DJINTERP

// ==========================================================================
//  THE LOCK SURFACE
// ==========================================================================


// ThreadsafeContainer
// concept: exposes lock() / unlock(). Named `lockable_`: threadsafe_container
// is a class in this namespace.
template<typename _Type>
concept ThreadsafeContainer =
    has_lock_method_v<clean_t<_Type>> && has_unlock_method_v<clean_t<_Type>>;


// SharedLockableContainer
//   concept: exposes lock_shared() -- many readers, one writer.
D_CONCEPT_FROM_TRAIT(SharedLockableContainer, has_lock_shared_method_v)


// TryLockableContainer
// concept: exposes try_lock() -- acquisition that may FAIL rather than block,
// which is the only kind a caller can back out of.
D_CONCEPT_FROM_TRAIT(TryLockableContainer, has_try_lock_method_v)


// DeclaresLockPolicy
// concept: carries a lock_policy_type -- it names HOW it is guarded, rather
// than leaving the caller to infer it from the surface.
D_CONCEPT_FROM_TRAIT(DeclaresLockPolicy, has_lock_policy_type_v)


// VersionedContainer
// concept: exposes a version stamp -- the basis of optimistic reads, where you
// check afterwards whether what you read was torn.
D_CONCEPT_FROM_TRAIT(VersionedContainer, has_version_method_v)

NS_END  // djinterp


#endif  // D_ENV_LANG_IS_CPP20_OR_HIGHER && D_ENV_CPP_FEATURE_LANG_CONCEPTS


#endif  // DJINTERP_THREADSAFE_CONTAINER_CONCEPTS_
