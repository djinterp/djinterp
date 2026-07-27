/******************************************************************************
* djinterp [container] sorted_container_concepts.hpp C++20 concepts for the
* SORTEDNESS axis -- the `requires`-facing view of sorted_container_traits.hpp.
* THE CONCEPTS ADD NO POLICY. Each is exactly its trait, spelled so it can
* constrain a template instead of gating one through enable_if. The trait stays
* the single source of truth. NAMES. Where the obvious name is taken by a
* CONTAINER CLASS in this namespace, the concept takes an adjective form
* instead. A concept and a class of the same name in one namespace is a hard
* redeclaration, and this framework has already been bitten by that three times.
* PORTABILITY: Gated on C++20 + concepts. Below that the header is empty and
* callers use the `::value` / `_v` forms directly. path:
* /inc/djinterp/core/container/concepts/sorted_container_concepts.hpp link(s):
* TBA author(s): Samuel 'teer' Neal-Blim created: 2026.07.14
* *****************************************************************************/

#ifndef DJINTERP_SORTED_CONTAINER_CONCEPTS_
#define DJINTERP_SORTED_CONTAINER_CONCEPTS_ 1

// djinterp
#include "../../djinterp.hpp"
#include "../../meta/concepts.hpp"   // D_CONCEPT_FROM_TRAIT
#include "../traits/sorted_container_traits.hpp"


#if D_ENV_LANG_IS_CPP20_OR_HIGHER && D_ENV_CPP_FEATURE_LANG_CONCEPTS


NS_DJINTERP

// ==========================================================================
//  THE PROMISE THE TYPE MAKES
// ==========================================================================


// SortedEnumerable
//   concept: enumerating it is GUARANTEED to yield comparator order -- monotone
// (keyed, comparator-equipped) or sorted (positional, invariant-held).
// This is the property that lets a container be PRESENTED in order without
// sorting it, and it is the whole point of a radix tree.
D_CONCEPT_FROM_TRAIT(SortedEnumerable, admits_sorted_enumeration_v)


// MonotoneContainer
// concept: keyed AND comparator-equipped: no positions, but the enumeration is
// sorted by construction. A hash-ordered container has no comparator and is not
// this.
D_CONCEPT_FROM_TRAIT(MonotoneContainer, is_monotone_container_v)


// SortedContainer
// concept: positional, and the positions are held in comparator order by
// invariant. Named for the guarantee -- `sorted_container` is a header and a
// mixin here.
D_CONCEPT_FROM_TRAIT(SortedContainer, is_sorted_container_v)


// UnsortedContainer
// concept: a container whose positions are NOT guaranteed in comparator order.
// Gated on being a container, so a non-container reports false rather than a
// vacuous true -- which is what separates this from a bare negation.
D_CONCEPT_FROM_TRAIT(UnsortedContainer, is_unsorted_container_v)

NS_END  // djinterp


#endif  // D_ENV_LANG_IS_CPP20_OR_HIGHER && D_ENV_CPP_FEATURE_LANG_CONCEPTS


#endif  // DJINTERP_SORTED_CONTAINER_CONCEPTS_
