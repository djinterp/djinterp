/******************************************************************************
* djinterp [container] write_only_container_concepts.hpp C++20 concepts for the
* ACCESS (write_only) axis -- the `requires`-facing view of
* write_only_container_traits.hpp. THE CONCEPTS ADD NO POLICY. Each is exactly
* its trait, spelled so it can constrain a template instead of gating one
* through enable_if. The trait stays the single source of truth. NAMES. Where
* the obvious name is taken by a CONTAINER CLASS in this namespace, the concept
* takes an adjective form instead. A concept and a class of the same name in one
* namespace is a hard redeclaration, and this framework has already been bitten
* by that three times. PORTABILITY: Gated on C++20 + concepts. Below that the
* header is empty and callers use the `::value` / `_v` forms directly. path:
* /inc/djinterp/core/container/concepts/write_only_container_concepts.hpp
* link(s): TBA author(s): Samuel 'teer' Neal-Blim created: 2026.07.14
* *****************************************************************************/

#ifndef DJINTERP_WRITE_ONLY_CONTAINER_CONCEPTS_
#define DJINTERP_WRITE_ONLY_CONTAINER_CONCEPTS_ 1

// djinterp
#include "../../djinterp.hpp"
#include "../../meta/concepts.hpp"   // D_CONCEPT_FROM_TRAIT
#include "../traits/write_only_container_traits.hpp"


#if D_ENV_LANG_IS_CPP20_OR_HIGHER && D_ENV_CPP_FEATURE_LANG_CONCEPTS


NS_DJINTERP

// ==========================================================================
//  RESTRICTION: A SINK
// ==========================================================================


// WriteOnlyContainer
// concept: a handle grants mutation but NOT observation of element VALUES. The
// value-free metadata size() / empty() is not an observer, so a sink exposing
// only those plus an append surface still reads write_only.
D_CONCEPT_FROM_TRAIT(WriteOnlyContainer, is_write_only_container_v)

NS_END  // djinterp


#endif  // D_ENV_LANG_IS_CPP20_OR_HIGHER && D_ENV_CPP_FEATURE_LANG_CONCEPTS


#endif  // DJINTERP_WRITE_ONLY_CONTAINER_CONCEPTS_
