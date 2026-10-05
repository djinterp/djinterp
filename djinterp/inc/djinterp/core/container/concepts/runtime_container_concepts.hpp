/*******************************************************************************
* djinterp [core]                                 runtime_container_concepts.hpp
*
* C++20 concepts for the LIFETIME (runtime end) axis -- the `requires`-facing
* view of runtime_container_traits.hpp.
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
* path:      /inc/djinterp/core/container/concepts/runtime_container_concepts.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.14
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef DJINTERP_CONTAINER_CONCEPTS_RUNTIME_CONTAINER_CONCEPTS_HPP
#define DJINTERP_CONTAINER_CONCEPTS_RUNTIME_CONTAINER_CONCEPTS_HPP 1

// FLOOR, FOR NOW: below C++17 this file is empty, rather than an error (round
// 2's rule). The owner's ruling: compile at every level first; port down only
// where something needs it.
#include "../../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// djinterp
#include "../../../djinterp.hpp"
#include "../traits/runtime_container_traits.hpp"


#if D_ENV_LANG_IS_CPP20_OR_HIGHER && D_ENV_CPP_FEATURE_LANG_CONCEPTS


NS_DJINTERP

// ==========================================================================
//  RUNTIME LIFETIME
// ==========================================================================


// RuntimeOnlyContainer
//   concept: container-shaped, and its value cannot be settled in a constant
// expression.
template<typename Type>
concept RuntimeOnlyContainer = is_runtime_container_v<Type>;


// RequiresRuntimeStorageContainer
//   concept: the STRONGER claim: it actively requires runtime storage -- an
// allocator or a growable capacity. Lifetime says WHEN the data are fixed;
// this isolates the genuine STORAGE signals, which is a different question.
template<typename Type>
concept RequiresRuntimeStorageContainer = requires_runtime_storage_v<Type>;

NS_END  // djinterp


#endif  // D_ENV_LANG_IS_CPP20_OR_HIGHER && D_ENV_CPP_FEATURE_LANG_CONCEPTS

#endif  // floor, for now

#endif  // DJINTERP_CONTAINER_CONCEPTS_RUNTIME_CONTAINER_CONCEPTS_HPP
