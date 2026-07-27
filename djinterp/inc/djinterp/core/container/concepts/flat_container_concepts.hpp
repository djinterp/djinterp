/******************************************************************************
* djinterp [container] flat_container_concepts.hpp C++20 concepts for the
* STRUCTURE (flat side) axis -- the `requires`-facing view of
* flat_container_traits.hpp. THE CONCEPTS ADD NO POLICY. Each is exactly its
* trait, spelled so it can constrain a template instead of gating one through
* enable_if. The trait stays the single source of truth. NAMES. Where the
* obvious name is taken by a CONTAINER CLASS in this namespace, the concept
* takes an adjective form instead. A concept and a class of the same name in one
* namespace is a hard redeclaration, and this framework has already been bitten
* by that three times. PORTABILITY: Gated on C++20 + concepts. Below that the
* header is empty and callers use the `::value` / `_v` forms directly. path:
* /inc/djinterp/core/container/concepts/flat_container_concepts.hpp link(s): TBA
* author(s): Samuel 'teer' Neal-Blim created: 2026.07.14
* *****************************************************************************/

#ifndef DJINTERP_FLAT_CONTAINER_CONCEPTS_
#define DJINTERP_FLAT_CONTAINER_CONCEPTS_ 1

// djinterp
#include "../../djinterp.hpp"
#include "../../meta/concepts.hpp"   // D_CONCEPT_FROM_TRAIT
#include "../traits/flat_container_traits.hpp"


#if D_ENV_LANG_IS_CPP20_OR_HIGHER && D_ENV_CPP_FEATURE_LANG_CONCEPTS


NS_DJINTERP

// ==========================================================================
//  FLATNESS  (the complement of nesting)
// ==========================================================================


// LeafOnlyContainer
// concept: a container that does NOT nest -- depth 1, leaves only. Defined as
// the complement of nesting over container-shaped types, so every nuance of
// that verdict carries over: a node_type keeps a type OUT of this.
D_CONCEPT_FROM_TRAIT(LeafOnlyContainer, is_flat_container_v)


// StrictlyLeafOnlyContainer
//   concept: flat AND of value_type-chain depth exactly 1 -- flat by the DEPTH
// measure, not merely by the absence of the hierarchy signals. The two
// differ only for a type whose flatness rests on a tag.
D_CONCEPT_FROM_TRAIT(StrictlyLeafOnlyContainer, is_strictly_flat_container_v)

NS_END  // djinterp


#endif  // D_ENV_LANG_IS_CPP20_OR_HIGHER && D_ENV_CPP_FEATURE_LANG_CONCEPTS


#endif  // DJINTERP_FLAT_CONTAINER_CONCEPTS_
