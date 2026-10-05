/*******************************************************************************
* djinterp [core]                                 container_overlay_concepts.hpp
*
* C++20 concepts for the OVERLAY axis -- the `requires`-facing view of
* container_overlay_traits.hpp.
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
* path:      /inc/djinterp/core/container/concepts/container_overlay_concepts.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.14
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef DJINTERP_CONTAINER_CONCEPTS_CONTAINER_OVERLAY_CONCEPTS_HPP
#define DJINTERP_CONTAINER_CONCEPTS_CONTAINER_OVERLAY_CONCEPTS_HPP 1

// FLOOR, FOR NOW: below C++11 this file is empty, rather than an error (round
// 2's rule). The owner's ruling: compile at every level first; port down only
// where something needs it.
#include "../../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// djinterp
#include "../../../djinterp.hpp"
#include "../traits/container_overlay_traits.hpp"


#if D_ENV_LANG_IS_CPP20_OR_HIGHER && D_ENV_CPP_FEATURE_LANG_CONCEPTS


NS_DJINTERP

// ==========================================================================
//  OVERLAYS
// ==========================================================================


// KeyedContainer
//   concept: an overlay imposes a key -- identity is by key, not by position.
template<typename Type>
concept KeyedContainer = is_keyed_container_v<Type>;


// OrderBlindOverlayContainer
//   concept: the overlay makes order unobservable, so two realizations
// differing only in order are the same container.
template<typename Type>
concept OrderBlindOverlayContainer = is_order_blind_overlay_v<Type>;

NS_END  // djinterp


#endif  // D_ENV_LANG_IS_CPP20_OR_HIGHER && D_ENV_CPP_FEATURE_LANG_CONCEPTS

#endif  // floor, for now

#endif  // DJINTERP_CONTAINER_CONCEPTS_CONTAINER_OVERLAY_CONCEPTS_HPP
