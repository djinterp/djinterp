/******************************************************************************
* djinterp [container] hierarchical_container_concepts.hpp C++20 concepts for
* the STRUCTURE (nesting) axis -- the `requires`-facing view of
* hierarchical_container_traits.hpp. THE CONCEPTS ADD NO POLICY. Each is exactly
* its trait, spelled so it can constrain a template instead of gating one
* through enable_if. The trait stays the single source of truth. NAMES. Where
* the obvious name is taken by a CONTAINER CLASS in this namespace, the concept
* takes an adjective form instead. A concept and a class of the same name in one
* namespace is a hard redeclaration, and this framework has already been bitten
* by that three times. PORTABILITY: Gated on C++20 + concepts. Below that the
* header is empty and callers use the `::value` / `_v` forms directly. path:
* /inc/djinterp/core/container/concepts/hierarchical_container_concepts.hpp
* link(s): TBA author(s): Samuel 'teer' Neal-Blim created: 2026.07.14
* *****************************************************************************/

#ifndef DJINTERP_HIERARCHICAL_CONTAINER_CONCEPTS_
#define DJINTERP_HIERARCHICAL_CONTAINER_CONCEPTS_ 1

// djinterp
#include "../../djinterp.hpp"
#include "../../meta/concepts.hpp"   // D_CONCEPT_FROM_TRAIT
#include "../traits/hierarchical_container_traits.hpp"


#if D_ENV_LANG_IS_CPP20_OR_HIGHER && D_ENV_CPP_FEATURE_LANG_CONCEPTS


NS_DJINTERP

// ==========================================================================
//  NESTING
// ==========================================================================


// ContainerShaped
//   concept: quacks like a container -- a value_type and a const size(). The
// recursion guard the whole structure axis rests on.
D_CONCEPT_FROM_TRAIT(ContainerShaped, is_container_shape_v)


// HierarchicalContainer
// concept: it NESTS: T = tau + F[T] with the node summand present, depth >= 2.
// Named in PascalCase; every concept is, so a concept and a same-named
// snake_case class cannot collide and the natural name is safe.
D_CONCEPT_FROM_TRAIT(HierarchicalContainer, is_hierarchical_container_v)


// DeclaresNodeSummand
// concept: advertises F[T] directly -- a node_type that is itself
// container-shaped. The STRONG signal: it decides outright, and a flat tag
// cannot override it. One does not un-nest a declared node summand.
D_CONCEPT_FROM_TRAIT(DeclaresNodeSummand, has_node_summand_v)


// DeclaresStructureCategory
// concept: carries the opt-in flat / hierarchical tag. The way to assert
// nesting the value_type chain cannot expose -- a trie, whose value_type is
// just its mapped type, is exactly this case.
D_CONCEPT_FROM_TRAIT(DeclaresStructureCategory, has_structure_category_v)

NS_END  // djinterp


#endif  // D_ENV_LANG_IS_CPP20_OR_HIGHER && D_ENV_CPP_FEATURE_LANG_CONCEPTS


#endif  // DJINTERP_HIERARCHICAL_CONTAINER_CONCEPTS_
