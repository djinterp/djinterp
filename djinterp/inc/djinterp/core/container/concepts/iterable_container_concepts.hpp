/******************************************************************************
* djinterp [container] iterable_container_concepts.hpp C++20 concepts for the
* ITERABILITY axis -- the `requires`-facing view of
* iterable_container_traits.hpp. THE CONCEPTS ADD NO POLICY. Each is exactly its
* trait, spelled so it can constrain a template instead of gating one through
* enable_if. The trait stays the single source of truth. NAMES. Where the
* obvious name is taken by a CONTAINER CLASS in this namespace, the concept
* takes an adjective form instead. A concept and a class of the same name in one
* namespace is a hard redeclaration, and this framework has already been bitten
* by that three times. PORTABILITY: Gated on C++20 + concepts. Below that the
* header is empty and callers use the `::value` / `_v` forms directly. path:
* /inc/djinterp/core/container/concepts/iterable_container_concepts.hpp link(s):
* TBA author(s): Samuel 'teer' Neal-Blim created: 2026.07.14
* *****************************************************************************/

#ifndef DJINTERP_ITERABLE_CONTAINER_CONCEPTS_
#define DJINTERP_ITERABLE_CONTAINER_CONCEPTS_ 1

// djinterp
#include "../../djinterp.hpp"
#include "../../meta/concepts.hpp"   // D_CONCEPT_FROM_TRAIT
#include "../traits/iterable_container_traits.hpp"


#if D_ENV_LANG_IS_CPP20_OR_HIGHER && D_ENV_CPP_FEATURE_LANG_CONCEPTS


NS_DJINTERP

// ==========================================================================
//  TRAVERSAL
// ==========================================================================


// IterableContainer
// concept: exposes a begin()/end() traversal. Two thirds of the trait system
// keys on this -- copy, merge, filter, conversion, transform all walk elements
// -- so a container without it is not less convenient, it is INVISIBLE.
D_CONCEPT_FROM_TRAIT(IterableContainer, is_iterable_container_v)


// PositionalOnlyContainer
// concept: looks like a container (it has value_type) but offers NO traversal
// -- positional access only.
D_CONCEPT_FROM_TRAIT(PositionalOnlyContainer, is_non_iterable_container_v)


// ==========================================================================
//  MODE  (a capability order: none < const < non-const)
// ==========================================================================


// ConstIterableContainer
//   concept: a read-only visit is available.
D_CONCEPT_FROM_TRAIT(ConstIterableContainer, is_const_iterable_container_v)


// MutablyIterableContainer
// concept: *begin() is a settable lvalue -- values may be REPLACED at existing
// positions. This does NOT subsume insertion or erasure; that is the Mutability
// axis.
D_CONCEPT_FROM_TRAIT(MutablyIterableContainer, is_mutable_iterable_container_v)

NS_END  // djinterp


#endif  // D_ENV_LANG_IS_CPP20_OR_HIGHER && D_ENV_CPP_FEATURE_LANG_CONCEPTS


#endif  // DJINTERP_ITERABLE_CONTAINER_CONCEPTS_
