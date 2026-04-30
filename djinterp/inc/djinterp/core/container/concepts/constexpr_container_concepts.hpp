/******************************************************************************
* djinterp [container]                        constexpr_container_concepts.hpp
*
* Constexpr container concepts:
*   C++20 concepts layered over constexpr_container_traits.hpp. These concepts
* provide readable constraints for compile-time-usable containers without
* replacing the existing SFINAE trait surface.
*
*   The concepts mirror the verified public trait surface from
* constexpr_container_traits.hpp:
*   - constexpr opt-in tag, extent, size-expression, and iteration signals
*   - constexpr vs not-constexpr classification
*   - shorthand concepts over constexpr_container_class<T>
*
* 
* path:      /inc/djinterp/core/container/concepts/
*                constexpr_container_concepts.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.28
******************************************************************************/

#ifndef DJINTERP_CONSTEXPR_CONTAINER_CONCEPTS_
#define DJINTERP_CONSTEXPR_CONTAINER_CONCEPTS_ 1

#ifndef __cplusplus
    #error "constexpr_container_concepts.hpp requires C++ compilation"
#endif

//djinterp
#include "../../djinterp.hpp"
#include "../traits/constexpr_container_traits.hpp"


NS_DJINTERP


#if D_ENV_CPP_FEATURE_LANG_CONCEPTS

// ===========================================================================
// I.   Primitive signal concepts
// ===========================================================================

// constexpr_tagged_container_surface
//   concept: constrains types exposing the opt-in constexpr container tag.
template<typename _Type>
concept constexpr_tagged_container_surface =
    has_constexpr_container_tag<_Type>::value;

// constexpr_extent_container_surface
//   concept: constrains types exposing a compile-time extent signal.
template<typename _Type>
concept constexpr_extent_container_surface =
    has_constexpr_extent<_Type>::value;

// constexpr_size_expression_surface
//   concept: constrains types whose default-constructed size() is usable in
// constant evaluation.
template<typename _Type>
concept constexpr_size_expression_surface =
    has_constexpr_size_expression<_Type>::value;

// constexpr_iteration_container_surface
//   concept: constrains types classified as having constexpr iteration.
template<typename _Type>
concept constexpr_iteration_container_surface =
    has_constexpr_iteration<_Type>::value;


// ===========================================================================
// II.  Axis classification concepts
// ===========================================================================

// constexpr_container_type
//   concept: constrains types classified as constexpr-capable containers.
template<typename _Type>
concept constexpr_container_type =
    is_constexpr_container<_Type>::value;

// not_constexpr_container_type
//   concept: constrains types explicitly classified as not constexpr-capable.
template<typename _Type>
concept not_constexpr_container_type =
    is_not_constexpr_container<_Type>::value;

// structurally_constexpr_container_type
//   concept: constrains constexpr containers classified through structural
// rather than opt-in tagging alone.
template<typename _Type>
concept structurally_constexpr_container_type =
    constexpr_container_type<_Type> &&
    ( has_constexpr_size_expression<_Type>::value ||
      has_constexpr_extent<_Type>::value          ||
      has_constexpr_iteration<_Type>::value );

// opt_in_constexpr_container_type
//   concept: constrains constexpr containers classified through the
// explicit is_constexpr_container opt-in tag.
template<typename _Type>
concept opt_in_constexpr_container_type =
    constexpr_container_type<_Type> &&
    has_constexpr_container_tag<_Type>::value;


// ===========================================================================
// III. Classification-based shorthand concepts
// ===========================================================================

// classified_constexpr_container
//   concept: shorthand for any type recognized as constexpr-capable by the
// aggregate classification struct.
template<typename _Type>
concept classified_constexpr_container =
    constexpr_container_class<_Type>::is_constexpr;

// extent_constexpr_container
//   concept: constexpr-capable container with compile-time extent metadata.
template<typename _Type>
concept extent_constexpr_container =
    constexpr_container_type<_Type> &&
    constexpr_container_class<_Type>::has_extent;

// iterable_constexpr_container
//   concept: constexpr-capable container with constexpr iteration support.
template<typename _Type>
concept iterable_constexpr_container =
    constexpr_container_type<_Type> &&
    constexpr_container_class<_Type>::has_iteration;

// size_constexpr_container
//   concept: constexpr-capable container with constexpr-valid size().
template<typename _Type>
concept size_constexpr_container =
    constexpr_container_type<_Type> &&
    constexpr_container_class<_Type>::has_size_expr;

#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS


NS_END  // djinterp


#endif  // DJINTERP_CONSTEXPR_CONTAINER_CONCEPTS_