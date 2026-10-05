/*******************************************************************************
* djinterp [core]                                    node_container_concepts.hpp
*
* C++20 concepts for the NODE CONTAINERS axis -- the `requires`-facing view of
* node_container_traits.hpp.
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
* path:      /inc/djinterp/core/container/concepts/node_container_concepts.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.14
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef DJINTERP_CONTAINER_CONCEPTS_NODE_CONTAINER_CONCEPTS_HPP
#define DJINTERP_CONTAINER_CONCEPTS_NODE_CONTAINER_CONCEPTS_HPP 1

// FLOOR, FOR NOW: below C++14 this file is empty, rather than an error (round
// 2's rule). The owner's ruling: compile at every level first; port down only
// where something needs it.
#include "../../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP14_OR_HIGHER

// djinterp
#include "../../../djinterp.hpp"
#include "../traits/node_container_traits.hpp"


#if D_ENV_LANG_IS_CPP20_OR_HIGHER && D_ENV_CPP_FEATURE_LANG_CONCEPTS


NS_DJINTERP

// ==========================================================================
//  NODE SHAPE
// ==========================================================================


// NodeContainer
//   concept: exposes a node_type. Named `declares_` because `node_container`
// is a CLASS in this namespace -- a concept of that name is a hard
// redeclaration.
template<typename Type>
concept NodeContainer = has_node_type_v<Type>;


// HasEntryPoint
//   concept: exposes an entry point into the node graph -- the handle
// everything else is reached through.
template<typename Type>
concept HasEntryPoint = has_entry_point_method_v<Type>;


// RootedNodeContainer
//   concept: exposes a root -- a node container with a distinguished entry,
// i.e. a tree rather than a general graph.
template<typename Type>
concept RootedNodeContainer = has_root_method_v<Type>;


// LinkedNodeContainer
//   concept: exposes a head -- a node container threaded as a list.
template<typename Type>
concept LinkedNodeContainer = has_head_method_v<Type>;


// DeclaresOwnershipPolicy
//   concept: states who owns its nodes. Ownership is a policy, not a shape,
// and
// it is the difference between a pointer that stays valid and one that does
// not.
template<typename Type>
concept DeclaresOwnershipPolicy = has_ownership_policy_v<Type>;

NS_END  // djinterp


#endif  // D_ENV_LANG_IS_CPP20_OR_HIGHER && D_ENV_CPP_FEATURE_LANG_CONCEPTS

#endif  // floor, for now

#endif  // DJINTERP_CONTAINER_CONCEPTS_NODE_CONTAINER_CONCEPTS_HPP
