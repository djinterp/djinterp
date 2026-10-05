/*******************************************************************************
* djinterp [core]                               container_transform_concepts.hpp
*
* C++20 concepts for the TRANSFORM axis -- the `requires`-facing view of
* container_transform_traits.hpp.
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
* path:      /inc/djinterp/core/container/concepts/container_transform_concepts.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.14
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef DJINTERP_CONTAINER_CONCEPTS_CONTAINER_TRANSFORM_CONCEPTS_HPP
#define DJINTERP_CONTAINER_CONCEPTS_CONTAINER_TRANSFORM_CONCEPTS_HPP 1

// FLOOR, FOR NOW: below C++11 this file is empty, rather than an error (round
// 2's rule). The owner's ruling: compile at every level first; port down only
// where something needs it.
#include "../../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// djinterp
#include "../../../djinterp.hpp"
#include "../traits/container_transform_traits.hpp"


#if D_ENV_LANG_IS_CPP20_OR_HIGHER && D_ENV_CPP_FEATURE_LANG_CONCEPTS


NS_DJINTERP

// ==========================================================================
//  TRANSFORMABILITY
// ==========================================================================


// TransformableContainer
//   concept: can be transformed into a container of its own kind -- walked AND
// rebuilt.
template<typename Type>
concept TransformableContainer = is_container_transformable_v<Type>;


// NativelyTransformableContainer
//   concept: carries its OWN transform, so it need not go through the generic
// walk- and-rebuild -- and can keep structure the generic path would have to
// discard.
template<typename Type>
concept NativelyTransformableContainer = has_native_transform_v<Type>;


// TransformSourceContainer
//   concept: can be transformed FROM even if not INTO. The same asymmetry
// filtering has, and for the same reason: a container whose elements do not
// carry their keys can be read but not rebuilt elementwise.
template<typename Type>
concept TransformSourceContainer = is_transform_source_v<Type>;


// TransformInputOnlyContainer
//   concept: the explicit statement of that asymmetry: readable, not
// rebuildable.
template<typename Type>
concept TransformInputOnlyContainer = is_transform_input_only_v<Type>;

NS_END  // djinterp


#endif  // D_ENV_LANG_IS_CPP20_OR_HIGHER && D_ENV_CPP_FEATURE_LANG_CONCEPTS

#endif  // floor, for now

#endif  // DJINTERP_CONTAINER_CONCEPTS_CONTAINER_TRANSFORM_CONCEPTS_HPP
