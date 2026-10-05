/*******************************************************************************
* djinterp [core]                                           storage_concepts.hpp
*
* C++20 concepts for the STORAGE-SITING vocabulary -- the `requires`-facing view
* of {traits}. THE CONCEPTS ADD NO POLICY. Each is exactly its trait, spelled so
* it can constrain a template instead of gating one through enable_if. The trait
* stays the single source of truth. NAMES. meta/concepts.hpp already owns the
* general type-level concepts (the `_c` family), and
* constexpr_iterator_concepts.hpp the constexpr-iteration ones; neither is
* duplicated here. Where an obvious name is otherwise taken, the concept takes a
* form that cannot collide -- a concept and a class of one name in one namespace
* is a hard redeclaration. PORTABILITY: Gated on C++20 + concepts. Below that
*
*
* path:      /inc/djinterp/core/meta/concepts/storage_concepts.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.14
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef DJINTERP_META_CONCEPTS_STORAGE_CONCEPTS_HPP
#define DJINTERP_META_CONCEPTS_STORAGE_CONCEPTS_HPP 1

// FLOOR, FOR NOW: below C++11 this file is empty, rather than an error (README
// rule 5). The owner's ruling: compile at every level first; port to C++98
// only where something needs it.
#include "../../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// djinterp
#include "../../../djinterp.hpp"
#include "../type_utility.hpp"  // clean_t
#include "../storage.hpp"


#if D_ENV_LANG_IS_CPP20_OR_HIGHER && D_ENV_CPP_FEATURE_LANG_CONCEPTS


NS_DJINTERP

// ==========================================================================
//  SITING, AT THE TYPE LEVEL
// ==========================================================================


// StaticStorageTyped
// concept: the type IS its own storage duration -- statically sited. This is
// the meta-level predicate on a type; the container-level view
// (StaticStorageContainer) is built on top of it and is usually what you
// want.
template<typename Type>
concept StaticStorageTyped =
    is_static_storage_type_v<clean_t<Type>>;


// DynamicStorageTyped
//   concept: dynamically sited at the type level -- cells acquired out of line.
template<typename Type>
concept DynamicStorageTyped =
    is_dynamic_storage_type_v<clean_t<Type>>;


// HybridStorageTyped
// concept: spans both at the type level -- small-buffer optimisation, which is
// declared and not detected.
template<typename Type>
concept HybridStorageTyped =
    is_hybrid_storage_type_v<clean_t<Type>>;


// ==========================================================================
//  THE OPT-IN
// ==========================================================================


// DeclaresStorageDuration
// concept: carries the static `storage_duration_category` member -- the
// highest- priority signal, the way a type corrects a misread or pins SBO.
template<typename Type>
concept DeclaresStorageDuration =
    has_storage_duration_category_v<clean_t<Type>>;

NS_END  // djinterp


#endif  // D_ENV_LANG_IS_CPP20_OR_HIGHER && D_ENV_CPP_FEATURE_LANG_CONCEPTS

#endif  // floor, for now


#endif  // DJINTERP_META_CONCEPTS_STORAGE_CONCEPTS_HPP
