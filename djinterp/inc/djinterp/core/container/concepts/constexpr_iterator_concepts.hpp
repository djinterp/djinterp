/*******************************************************************************
* djinterp [core]                                constexpr_iterator_concepts.hpp
*
* djinterp constexpr_iterator_concepts.hpp
*
* Compile-time iterability concepts.
*   C++20 concepts layered on top of the constexpr_iterator trait
* layer.  Provides readable `requires` constraints for:
*
*     ConstexprIterable        - the umbrella concept
*     ConstexprBeginCapable   - has constexpr_begin()
*     ConstexprEndCapable     - has constexpr_end()
*     ConstexprIterAlias      - has nested constexpr_iterator
*
*   This header is intentionally thin: it does not re-implement
* detection.  Each concept forwards to the corresponding public
* trait or variable template from
* constexpr_iterator_traits.hpp.
*
*   PORTABILITY:
*   The whole header is a no-op when concepts are unavailable.
* On C++17 and earlier, callers should constrain templates with
* the underlying SFINAE traits directly (e.g. via std::enable_if
* on is_constexpr_iterable<Type>::value).
*
*
*            constexpr_iterator_concepts.hpp
*
*
* path:      /inc/djinterp/core/container/concepts/constexpr_iterator_concepts.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.04.25
*                                                            revised: 2026.10.01
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    Feature Gate
      ------------

II.   Method-Level Concepts
      ---------------------

III.  Type-Alias Concept
      ------------------

IV.   Aggregate Concept
      -----------------
*/

#ifndef DJINTERP_CONTAINER_CONCEPTS_CONSTEXPR_ITERATOR_CONCEPTS_HPP
#define DJINTERP_CONTAINER_CONCEPTS_CONSTEXPR_ITERATOR_CONCEPTS_HPP 1

// FLOOR, FOR NOW: below C++17 this file is empty, rather than an error (round
// 2's rule). The owner's ruling: compile at every level first; port down only
// where something needs it.
#include "../../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// std
#include <type_traits>
// djinterp
#include "../../../djinterp.hpp"
#include "../iterator/constexpr_iterator_traits.hpp"


// ===========================================================================
// I.   Feature Gate
// ===========================================================================
// Concepts are a C++20 language feature.  This entire header
// is empty in older standards; callers should fall back to
// SFINAE traits directly.
#if D_ENV_LANG_IS_CPP20_OR_HIGHER && D_ENV_CPP_FEATURE_LANG_CONCEPTS


NS_DJINTERP
NS_CONCEPTS


// ===========================================================================
// II.  Method-Level Concepts
// ===========================================================================

// ConstexprBeginCapable
//   concept: constrains types exposing a constexpr_begin() const member.
template<typename Type>
concept ConstexprBeginCapable =
    has_constexpr_begin_method_v<clean_t<Type>>;

// ConstexprEndCapable
//   concept: constrains types exposing a constexpr_end() const member.
template<typename Type>
concept ConstexprEndCapable =
    has_constexpr_end_method_v<clean_t<Type>>;


// ===========================================================================
// III. Type-Alias Concept
// ===========================================================================

// ConstexprIterAlias
//   concept: constrains types declaring a nested `constexpr_iterator` type
// alias.
template<typename Type>
concept ConstexprIterAlias =
    has_constexpr_iterator_alias_v<clean_t<Type>>;


// ===========================================================================
// IV.  Aggregate Concept
// ===========================================================================

// ConstexprIterable
//   concept: the umbrella concept. A type is constexpr-iterable when it is
// iterable AND supports compile-time iteration (per is_constexpr_iterable).
template<typename Type>
concept ConstexprIterable =
    is_constexpr_iterable_v<clean_t<Type>>;

// HasConstexprIterationConcept
//   concept: constrains types that expose any compile-time iteration entry
// point (alias OR begin/end pair).
template<typename Type>
concept HasConstexprIterationConcept =
    has_constexpr_iteration_v<clean_t<Type>>;


NS_END  // concepts
NS_END  // djinterp


#endif  // C++20 + concepts

#endif  // floor, for now

#endif  // DJINTERP_CONTAINER_CONCEPTS_CONSTEXPR_ITERATOR_CONCEPTS_HPP
