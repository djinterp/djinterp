/*******************************************************************************
* djinterp [core]                                 container_storage_concepts.hpp
*
* C++20 concepts for the STORAGE (siting) axis -- the `requires`-facing view
* of container_storage_traits.hpp.
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
* path:      /inc/djinterp/core/container/concepts/container_storage_concepts.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.14
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef DJINTERP_CONTAINER_CONCEPTS_CONTAINER_STORAGE_CONCEPTS_HPP
#define DJINTERP_CONTAINER_CONCEPTS_CONTAINER_STORAGE_CONCEPTS_HPP 1

// FLOOR, FOR NOW: below C++17 this file is empty, rather than an error (round
// 2's rule). The owner's ruling: compile at every level first; port down only
// where something needs it.
#include "../../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// djinterp
#include "../../../djinterp.hpp"
#include "../traits/container_storage_traits.hpp"


#if D_ENV_LANG_IS_CPP20_OR_HIGHER && D_ENV_CPP_FEATURE_LANG_CONCEPTS


NS_DJINTERP

// ==========================================================================
//  WHERE THE CELLS LIVE
// ==========================================================================


// StaticStorageContainer
//   concept: the cells sit in the container's own footprint -- an extent or a
// tuple_size. Storage answers WHERE; Lifetime answers WHEN, and the two are
// independent.
template<typename Type>
concept StaticStorageContainer = is_static_storage_container_v<Type>;


// DynamicStorageContainer
//   concept: the cells are acquired out of line -- an allocator_type or a
// reserve(n). Static storage takes no allocator, so either probe is decisive.
template<typename Type>
concept DynamicStorageContainer = is_dynamic_storage_container_v<Type>;


// HybridStorageContainer
//   concept: the siting spans both -- small-buffer optimisation. Only ever
// true via the opt-in: SBO is not legible from a public surface, so it is
// DECLARED, never detected.
template<typename Type>
concept HybridStorageContainer = is_hybrid_storage_container_v<Type>;


// ==========================================================================
//  COMPONENTS  (the ones to use when hybrid should count on both sides)
// ==========================================================================


// HasStaticStorage
//   concept: the siting INCLUDES an inline part -- static or hybrid.
template<typename Type>
concept HasStaticStorage = has_static_storage_component_container_v<Type>;


// HasDynamicStorage
//   concept: the siting INCLUDES an out-of-line part -- dynamic or hybrid.
template<typename Type>
concept HasDynamicStorage = has_dynamic_storage_component_container_v<Type>;

NS_END  // djinterp


#endif  // D_ENV_LANG_IS_CPP20_OR_HIGHER && D_ENV_CPP_FEATURE_LANG_CONCEPTS

#endif  // floor, for now

#endif  // DJINTERP_CONTAINER_CONCEPTS_CONTAINER_STORAGE_CONCEPTS_HPP
