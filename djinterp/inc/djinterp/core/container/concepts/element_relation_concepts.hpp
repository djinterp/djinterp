/******************************************************************************
* djinterp [container] element_relation_concepts.hpp C++20 concepts for the
* ELEMENT RELATIONS axis -- the `requires`-facing view of
* element_relation_traits.hpp. THE CONCEPTS ADD NO POLICY. Each is exactly its
* trait, spelled so it can constrain a template instead of gating one through
* enable_if. The trait stays the single source of truth. NAMES. Where the
* obvious name is taken by a CONTAINER CLASS in this namespace, the concept
* takes an adjective form instead. A concept and a class of the same name in one
* namespace is a hard redeclaration, and this framework has already been bitten
* by that three times. PORTABILITY: Gated on C++20 + concepts. Below that the
* header is empty and callers use the `::value` / `_v` forms directly. path:
* /inc/djinterp/core/container/concepts/element_relation_concepts.hpp link(s):
* TBA author(s): Samuel 'teer' Neal-Blim created: 2026.07.14
* *****************************************************************************/

#ifndef DJINTERP_ELEMENT_RELATION_CONCEPTS_
#define DJINTERP_ELEMENT_RELATION_CONCEPTS_ 1

// djinterp
#include "../../djinterp.hpp"
#include "../../meta/concepts.hpp"   // D_CONCEPT_FROM_TRAIT
#include "../traits/element_relation_traits.hpp"


#if D_ENV_LANG_IS_CPP20_OR_HIGHER && D_ENV_CPP_FEATURE_LANG_CONCEPTS


NS_DJINTERP

// ==========================================================================
//  WHAT THE ELEMENTS SUPPORT
// ==========================================================================


// EqualityComparableElements
// concept: the ELEMENTS compare for equality. This is the precondition for
// content equality of the containers -- not the same thing as the containers
// themselves being comparable.
D_CONCEPT_FROM_TRAIT(EqualityComparableElements,
                     has_equality_comparable_elements_v)


// OrderedComparableElements
//   concept: the elements admit <, so the container can be ordered by content.
D_CONCEPT_FROM_TRAIT(OrderedComparableElements,
                     has_less_than_comparable_elements_v)


// TotallyOrderedElements
// concept: the elements are TOTALLY ordered -- every pair is comparable, which
// is what a sort actually needs and what < alone does not promise.
D_CONCEPT_FROM_TRAIT(TotallyOrderedElements, has_totally_ordered_elements_v)


// ==========================================================================
//  WHAT THE CONTAINER ITSELF SUPPORTS
// ==========================================================================


// EqualityComparableContainer
//   concept: the CONTAINER compares for equality, directly.
D_CONCEPT_FROM_TRAIT(EqualityComparableContainer,
                     is_equality_comparable_container_v)


// OrderedComparableContainer
//   concept: the container compares with <.
D_CONCEPT_FROM_TRAIT(OrderedComparableContainer,
                     is_ordered_comparable_container_v)

NS_END  // djinterp


#endif  // D_ENV_LANG_IS_CPP20_OR_HIGHER && D_ENV_CPP_FEATURE_LANG_CONCEPTS


#endif  // DJINTERP_ELEMENT_RELATION_CONCEPTS_
