/*******************************************************************************
* djinterp [core]                            hierarchical_container_concepts.hpp
*
* C++20 concepts for the STRUCTURE (nesting) axis -- the `requires`-facing
* view of hierarchical_container_traits.hpp.
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
* path:      /inc/djinterp/core/container/concepts/hierarchical_container_concepts.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.14
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef DJINTERP_CONTAINER_CONCEPTS_HIERARCHICAL_CONTAINER_CONCEPTS_HPP
#define DJINTERP_CONTAINER_CONCEPTS_HIERARCHICAL_CONTAINER_CONCEPTS_HPP 1

// FLOOR, FOR NOW: below C++11 this file is empty, rather than an error (round
// 2's rule). The owner's ruling: compile at every level first; port down only
// where something needs it.
#include "../../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// djinterp
#include "../../../djinterp.hpp"
#include "../traits/hierarchical_container_traits.hpp"


#if D_ENV_LANG_IS_CPP20_OR_HIGHER && D_ENV_CPP_FEATURE_LANG_CONCEPTS


NS_DJINTERP

// ==========================================================================
//  NESTING
// ==========================================================================


// ContainerShaped
//   concept: quacks like a container -- a value_type and a const size(). The
// recursion guard the whole structure axis rests on.
template<typename Type>
concept ContainerShaped = is_container_shape_v<Type>;


// HierarchicalContainer
//   concept: it NESTS: T = tau + F[T] with the node summand present, depth >=
// 2. Named in PascalCase; every concept is, so a concept and a same-named
// snake_case class cannot collide and the natural name is safe.
template<typename Type>
concept HierarchicalContainer = is_hierarchical_container_v<Type>;


// DeclaresNodeSummand
//   concept: advertises F[T] directly -- a node_type that is itself
// container-shaped. The STRONG signal: it decides outright, and a flat tag
// cannot override it. One does not un-nest a declared node summand.
template<typename Type>
concept DeclaresNodeSummand = has_node_summand_v<Type>;


// DeclaresStructureCategory
//   concept: carries the opt-in flat / hierarchical tag. The way to assert
// nesting the value_type chain cannot expose -- a trie, whose value_type is
// just its mapped type, is exactly this case.
template<typename Type>
concept DeclaresStructureCategory = has_structure_category_v<Type>;

NS_END  // djinterp


#endif  // D_ENV_LANG_IS_CPP20_OR_HIGHER && D_ENV_CPP_FEATURE_LANG_CONCEPTS

#endif  // floor, for now

#endif  // DJINTERP_CONTAINER_CONCEPTS_HIERARCHICAL_CONTAINER_CONCEPTS_HPP
