/******************************************************************************
* djinterp [container]                           iterator_category_concepts.hpp
*
* C++20 concepts for the runtime ITERATOR-CATEGORY layer -- the
* `requires`-facing view of iterator/iterator_category_traits.hpp. THE
* CONCEPTS ADD NO POLICY. Each is
* exactly its trait, spelled so it can constrain a template instead of gating
* one through enable_if. The trait stays the single source of truth. NAMES.
* meta/concepts.hpp already owns the general type-level concepts (the `_c`
* family), and constexpr_iterator_concepts.hpp the constexpr-iteration ones;
* neither is duplicated here. Where an obvious name is otherwise taken, the
* concept takes a form that cannot collide -- a concept and a class of one name
* in one namespace is a hard redeclaration. PORTABILITY: Gated on C++20 +
* concepts. Below that the header is empty and callers use the `::value` / `_v`
* forms directly. path:
* /inc/djinterp/core/container/iterator/iterator_category_concepts.hpp link(s):
* TBA author(s): Samuel 'teer' Neal-Blim created: 2026.07.14
* *****************************************************************************/

#ifndef DJINTERP_ITERATOR_CATEGORY_CONCEPTS_
#define DJINTERP_ITERATOR_CATEGORY_CONCEPTS_ 1

// djinterp
#include "../../djinterp.hpp"
#include "../../meta/concepts.hpp"   // D_CONCEPT_FROM_TRAIT
#include "../iterator/iterator_category_traits.hpp"


#if D_ENV_LANG_IS_CPP20_OR_HIGHER && D_ENV_CPP_FEATURE_LANG_CONCEPTS


NS_DJINTERP

// ==========================================================================
//  STANDARD CATEGORY, AT THE CONTAINER LEVEL
// ==========================================================================


// ForwardIterableContainer
// concept: its begin() iterator is at least forward. These are CONTAINER-level
// -- they read the category of the container's iterator -- as against the
// iterator-level is_forward_iterator<It>, a separate concern. The
// sequential-layout classifier and the filter strategy both consume these.
D_CONCEPT_FROM_TRAIT(ForwardIterableContainer, is_forward_iterable_v)


// BidirectionalIterableContainer
//   concept: its iterator can step backward as well as forward.
D_CONCEPT_FROM_TRAIT(BidirectionalIterableContainer,
                     is_bidirectional_iterable_v)


// RandomAccessIterableContainer
//   concept: its iterator supports O(1) jumps -- indexing, not just stepping.
D_CONCEPT_FROM_TRAIT(RandomAccessIterableContainer, is_random_access_iterable_v)


// ContiguousIterableContainer
// concept: random-access AND its elements are contiguous in memory -- it
// exposes data(). The strongest category, and the one that licenses treating
// the range as a buffer.
D_CONCEPT_FROM_TRAIT(ContiguousIterableContainer, is_contiguous_iterable_v)

NS_END  // djinterp


#endif  // D_ENV_LANG_IS_CPP20_OR_HIGHER && D_ENV_CPP_FEATURE_LANG_CONCEPTS


#endif  // DJINTERP_ITERATOR_CATEGORY_CONCEPTS_
