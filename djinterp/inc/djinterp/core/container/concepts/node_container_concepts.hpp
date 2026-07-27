/******************************************************************************
* djinterp [container] node_container_concepts.hpp C++20 concepts for the NODE
* CONTAINERS axis -- the `requires`-facing view of node_container_traits.hpp.
* THE CONCEPTS ADD NO POLICY. Each is exactly its trait, spelled so it can
* constrain a template instead of gating one through enable_if. The trait stays
* the single source of truth. NAMES. Where the obvious name is taken by a
* CONTAINER CLASS in this namespace, the concept takes an adjective form
* instead. A concept and a class of the same name in one namespace is a hard
* redeclaration, and this framework has already been bitten by that three times.
* PORTABILITY: Gated on C++20 + concepts. Below that the header is empty and
* callers use the `::value` / `_v` forms directly. path:
* /inc/djinterp/core/container/concepts/node_container_concepts.hpp link(s): TBA
* author(s): Samuel 'teer' Neal-Blim created: 2026.07.14
* *****************************************************************************/

#ifndef DJINTERP_NODE_CONTAINER_CONCEPTS_
#define DJINTERP_NODE_CONTAINER_CONCEPTS_ 1

// djinterp
#include "../../djinterp.hpp"
#include "../../meta/concepts.hpp"   // D_CONCEPT_FROM_TRAIT
#include "../traits/node_container_traits.hpp"


#if D_ENV_LANG_IS_CPP20_OR_HIGHER && D_ENV_CPP_FEATURE_LANG_CONCEPTS


NS_DJINTERP

// ==========================================================================
//  NODE SHAPE
// ==========================================================================


// NodeContainer
// concept: exposes a node_type. Named `declares_` because `node_container` is a
// CLASS in this namespace -- a concept of that name is a hard redeclaration.
D_CONCEPT_FROM_TRAIT(NodeContainer, has_node_type_v)


// HasEntryPoint
// concept: exposes an entry point into the node graph -- the handle everything
// else is reached through.
D_CONCEPT_FROM_TRAIT(HasEntryPoint, has_entry_point_method_v)


// RootedNodeContainer
// concept: exposes a root -- a node container with a distinguished entry, i.e.
// a tree rather than a general graph.
D_CONCEPT_FROM_TRAIT(RootedNodeContainer, has_root_method_v)


// LinkedNodeContainer
//   concept: exposes a head -- a node container threaded as a list.
D_CONCEPT_FROM_TRAIT(LinkedNodeContainer, has_head_method_v)


// DeclaresOwnershipPolicy
// concept: states who owns its nodes. Ownership is a policy, not a shape, and
// it is the difference between a pointer that stays valid and one that does
// not.
D_CONCEPT_FROM_TRAIT(DeclaresOwnershipPolicy, has_ownership_policy_v)

NS_END  // djinterp


#endif  // D_ENV_LANG_IS_CPP20_OR_HIGHER && D_ENV_CPP_FEATURE_LANG_CONCEPTS


#endif  // DJINTERP_NODE_CONTAINER_CONCEPTS_
