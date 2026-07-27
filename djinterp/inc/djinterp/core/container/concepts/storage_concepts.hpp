/******************************************************************************
* djinterp [container]                                     storage_concepts.hpp
*
* C++20 concepts for the STORAGE-SITING vocabulary -- the `requires`-facing view
* of meta/storage.hpp. THE CONCEPTS ADD NO POLICY. Each is exactly its trait,
* spelled so
* it can constrain a template instead of gating one through enable_if. The trait
* stays the single source of truth. NAMES. meta/concepts.hpp already owns the
* general type-level concepts (the `_c` family), and
* constexpr_iterator_concepts.hpp the constexpr-iteration ones; neither is
* duplicated here. Where an obvious name is otherwise taken, the concept takes a
* form that cannot collide -- a concept and a class of one name in one namespace
* is a hard redeclaration. PORTABILITY: Gated on C++20 + concepts. Below that
* the header is empty and callers use the `::value` / `_v` forms directly. path:
* /inc/djinterp/core/meta/storage_concepts.hpp link(s): TBA author(s): Samuel
* 'teer' Neal-Blim created: 2026.07.14
* *****************************************************************************/

#ifndef DJINTERP_META_STORAGE_CONCEPTS_
#define DJINTERP_META_STORAGE_CONCEPTS_ 1

// djinterp
#include "../../djinterp.hpp"
#include "../../meta/concepts.hpp"   // D_CONCEPT_FROM_TRAIT
#include "../../meta/storage.hpp"


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
D_CONCEPT_FROM_TRAIT(StaticStorageTyped, is_static_storage_type_v)


// DynamicStorageTyped
//   concept: dynamically sited at the type level -- cells acquired out of line.
D_CONCEPT_FROM_TRAIT(DynamicStorageTyped, is_dynamic_storage_type_v)


// HybridStorageTyped
// concept: spans both at the type level -- small-buffer optimisation, which is
// declared and not detected.
D_CONCEPT_FROM_TRAIT(HybridStorageTyped, is_hybrid_storage_type_v)


// ==========================================================================
//  THE OPT-IN
// ==========================================================================


// DeclaresStorageDuration
// concept: carries the static `storage_duration_category` member -- the
// highest- priority signal, the way a type corrects a misread or pins SBO.
D_CONCEPT_FROM_TRAIT(DeclaresStorageDuration, has_storage_duration_category_v)

NS_END  // djinterp


#endif  // D_ENV_LANG_IS_CPP20_OR_HIGHER && D_ENV_CPP_FEATURE_LANG_CONCEPTS


#endif  // DJINTERP_META_STORAGE_CONCEPTS_
