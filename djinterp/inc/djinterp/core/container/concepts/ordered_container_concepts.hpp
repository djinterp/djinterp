/******************************************************************************
* djinterp [container] ordered_container_concepts.hpp C++20 concepts for the
* ORDER axis -- the `requires`-facing view of ordered_container_traits.hpp. THE
* CONCEPTS ADD NO POLICY. Each is exactly its trait, spelled so it can constrain
* a template instead of gating one through enable_if. The trait stays the single
* source of truth. NAMES. Where the obvious name is taken by a CONTAINER CLASS
* in this namespace, the concept takes an adjective form instead. A concept and
* a class of the same name in one namespace is a hard redeclaration, and this
* framework has already been bitten by that three times. PORTABILITY: Gated on
* C++20 + concepts. Below that the header is empty and callers use the `::value`
* / `_v` forms directly. path:
* /inc/djinterp/core/container/concepts/ordered_container_concepts.hpp link(s):
* TBA author(s): Samuel 'teer' Neal-Blim created: 2026.07.14
* *****************************************************************************/

#ifndef DJINTERP_ORDERED_CONTAINER_CONCEPTS_
#define DJINTERP_ORDERED_CONTAINER_CONCEPTS_ 1

// djinterp
#include "../../djinterp.hpp"
#include "../../meta/concepts.hpp"   // D_CONCEPT_FROM_TRAIT
#include "../traits/ordered_container_traits.hpp"


#if D_ENV_LANG_IS_CPP20_OR_HIGHER && D_ENV_CPP_FEATURE_LANG_CONCEPTS


NS_DJINTERP

// ==========================================================================
//  POSITIONAL IDENTITY
// ==========================================================================


// OrderedContainer
//   concept: iterable and carrying no key_type -- a positional sequence, where
// position MEANS something. Named in PascalCase; every concept is, so a concept
// and a same-named snake_case class cannot collide and the natural name is
// safe.
D_CONCEPT_FROM_TRAIT(OrderedContainer, is_ordered_container_v)


// UnorderedContainer
// concept: iterable and keyed -- identity is the BAG, not any order. An
// associative container is this, and so is a radix tree, even though its
// enumeration happens to come out sorted (that is sortedness, a different
// axis).
D_CONCEPT_FROM_TRAIT(UnorderedContainer, is_unordered_container_v)

NS_END  // djinterp


#endif  // D_ENV_LANG_IS_CPP20_OR_HIGHER && D_ENV_CPP_FEATURE_LANG_CONCEPTS


#endif  // DJINTERP_ORDERED_CONTAINER_CONCEPTS_
