/******************************************************************************
* djinterp [container] mutable_container_concepts.hpp C++20 concepts for the
* MUTABILITY axis -- the `requires`-facing view of mutable_container_traits.hpp.
* THE CONCEPTS ADD NO POLICY. Each is exactly its trait, spelled so it can
* constrain a template instead of gating one through enable_if. The trait stays
* the single source of truth. NAMES. Where the obvious name is taken by a
* CONTAINER CLASS in this namespace, the concept takes an adjective form
* instead. A concept and a class of the same name in one namespace is a hard
* redeclaration, and this framework has already been bitten by that three times.
* PORTABILITY: Gated on C++20 + concepts. Below that the header is empty and
* callers use the `::value` / `_v` forms directly. path:
* /inc/djinterp/core/container/concepts/mutable_container_concepts.hpp link(s):
* TBA author(s): Samuel 'teer' Neal-Blim created: 2026.07.14
* *****************************************************************************/

#ifndef DJINTERP_MUTABLE_CONTAINER_CONCEPTS_
#define DJINTERP_MUTABLE_CONTAINER_CONCEPTS_ 1

// djinterp
#include "../../djinterp.hpp"
#include "../../meta/concepts.hpp"   // D_CONCEPT_FROM_TRAIT
#include "../traits/mutable_container_traits.hpp"


#if D_ENV_LANG_IS_CPP20_OR_HIGHER && D_ENV_CPP_FEATURE_LANG_CONCEPTS


NS_DJINTERP

// ==========================================================================
//  WHAT MAY CHANGE
// ==========================================================================


// MutableContainerType
//   concept: something may change -- values, structure, or both.
D_CONCEPT_FROM_TRAIT(MutableContainerType, is_mutable_container_v)


// ImmutableContainerType
//   concept: container-shaped and nothing may change.
D_CONCEPT_FROM_TRAIT(ImmutableContainerType, is_immutable_container_v)


// ElementMutableContainerType
// concept: an EXISTING element may be overwritten in place. The probe is
// sequence- shaped (a settable operator[] or data()), so an associative
// container -- std::map included -- does not read as this. Not a bug: it is
// what the structural probe can see.
D_CONCEPT_FROM_TRAIT(ElementMutableContainerType,
                     is_element_mutable_container_v)


// StructurallyMutableContainerType
//   concept: the SET of elements may change -- insert, erase, clear, resize.
D_CONCEPT_FROM_TRAIT(StructurallyMutableContainerType,
                     is_structurally_mutable_container_v)

NS_END  // djinterp


#endif  // D_ENV_LANG_IS_CPP20_OR_HIGHER && D_ENV_CPP_FEATURE_LANG_CONCEPTS


#endif  // DJINTERP_MUTABLE_CONTAINER_CONCEPTS_
