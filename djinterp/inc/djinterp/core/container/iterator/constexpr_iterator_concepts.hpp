/******************************************************************************
* djinterp                                       constexpr_iterator_concepts.hpp
*
* Compile-time iterability concepts.
*   C++20 concepts layered on top of the constexpr_iterator trait
* layer.  Provides readable `requires` constraints for:
*
*     constexpr_iterable        - the umbrella concept
*     constexpr_begin_capable   - has constexpr_begin()
*     constexpr_end_capable     - has constexpr_end()
*     constexpr_iter_alias      - has nested constexpr_iterator
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
* on is_constexpr_iterable<_Type>::value).
*
* TABLE OF CONTENTS
* =================
* I.    Feature Gate
* II.   Method-Level Concepts
* III.  Type-Alias Concept
* IV.   Aggregate Concept
*
*
* path:      /inc/djinterp/container/iterator/constexpr_iterator_concepts.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.25
******************************************************************************/

#ifndef DJINTERP_CONSTEXPR_ITERATOR_CONCEPTS_
#define DJINTERP_CONSTEXPR_ITERATOR_CONCEPTS_ 1

// std
#include <type_traits>
// djinterp
#include "../core/djinterp.hpp"
#include "./constexpr_iterator_traits.hpp"


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

// constexpr_begin_capable
//   concept: constrains types exposing a constexpr_begin()
// const member.
template<typename _Type>
concept constexpr_begin_capable =
    has_constexpr_begin_method_v<clean_t<_Type>>;

// constexpr_end_capable
//   concept: constrains types exposing a constexpr_end()
// const member.
template<typename _Type>
concept constexpr_end_capable =
    has_constexpr_end_method_v<clean_t<_Type>>;


// ===========================================================================
// III. Type-Alias Concept
// ===========================================================================

// constexpr_iter_alias
//   concept: constrains types declaring a nested
// `constexpr_iterator` type alias.
template<typename _Type>
concept constexpr_iter_alias =
    has_constexpr_iterator_alias_v<clean_t<_Type>>;


// ===========================================================================
// IV.  Aggregate Concept
// ===========================================================================

// constexpr_iterable
//   concept: the umbrella concept.  A type is
// constexpr-iterable when it is iterable AND supports
// compile-time iteration (per is_constexpr_iterable).
template<typename _Type>
concept constexpr_iterable =
    is_constexpr_iterable_v<clean_t<_Type>>;

// has_constexpr_iteration_concept
//   concept: constrains types that expose any compile-time
// iteration entry point (alias OR begin/end pair).
template<typename _Type>
concept has_constexpr_iteration_concept =
    has_constexpr_iteration_v<clean_t<_Type>>;


NS_END  // concepts
NS_END  // djinterp


#endif  // C++20 + concepts


#endif  // DJINTERP_CONSTEXPR_ITERATOR_CONCEPTS_
