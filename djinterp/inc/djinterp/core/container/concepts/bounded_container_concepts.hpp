/******************************************************************************
* djinterp [container]                          bounded_container_concepts.hpp
*
* Bounded container concepts:
*   C++20 concepts layered over bounded_container_traits.hpp. These concepts
* provide readable constraints for the bounded / unbounded axis without
* replacing the existing SFINAE trait surface.
*   The concepts mirror the verified public trait surface from
* bounded_container_traits.hpp:
*   - fixed extent, max_size, capacity, reserve, and size signals
*   - bounded vs unbounded classification
*   - shorthand concepts over bounded_container_class<T>
*
* 
* path:      /inc/djinterp/core/container/concepts/
*                bounded_container_concepts.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.28
******************************************************************************/

#ifndef DJINTERP_BOUNDED_CONTAINER_CONCEPTS_
#define DJINTERP_BOUNDED_CONTAINER_CONCEPTS_ 1

#ifndef __cplusplus
    #error "bounded_container_concepts.hpp requires C++ compilation"
#endif

//djinterp
#include "../../djinterp.hpp"
#include "../traits/bounded_container_traits.hpp"


NS_DJINTERP


#if D_ENV_CPP_FEATURE_LANG_CONCEPTS

// ===========================================================================
// I.   Primitive signal concepts
// ===========================================================================

// fixed_extent_bounded_surface
//   concept: constrains types exposing a compile-time extent signal.
template<typename _Type>
concept fixed_extent_bounded_surface =
    has_fixed_extent_signal<_Type>::value;

// max_size_bounded_surface
//   concept: constrains types exposing max_size().
template<typename _Type>
concept max_size_bounded_surface =
    has_max_size_signal<_Type>::value;

// capacity_bounded_surface
//   concept: constrains types exposing capacity().
template<typename _Type>
concept capacity_bounded_surface =
    has_capacity_signal<_Type>::value;

// reserve_capable_bounded_surface
//   concept: constrains types exposing reserve(size_t).
template<typename _Type>
concept reserve_capable_bounded_surface =
    has_reserve_signal<_Type>::value;

// sized_bounded_surface
//   concept: constrains types exposing size().
template<typename _Type>
concept sized_bounded_surface =
    has_size_signal<_Type>::value;


// ===========================================================================
// II.  Axis classification concepts
// ===========================================================================

// bounded_container_type
//   concept: constrains types classified as bounded.
template<typename _Type>
concept bounded_container_type =
    is_bounded_container<_Type>::value;

// unbounded_container_type
//   concept: constrains types classified as unbounded.
template<typename _Type>
concept unbounded_container_type =
    is_unbounded_container<_Type>::value;

// fixed_capacity_container_type
//   concept: constrains bounded containers whose bound is implied by
// capacity() without reserve().
template<typename _Type>
concept fixed_capacity_container_type =
    has_capacity_signal<_Type>::value &&
    !has_reserve_signal<_Type>::value;

// extent_bounded_container_type
//   concept: constrains bounded containers whose bound is signaled by extent.
template<typename _Type>
concept extent_bounded_container_type =
    bounded_container_type<_Type> &&
    has_fixed_extent_signal<_Type>::value;

// max_size_bounded_container_type
//   concept: constrains bounded containers whose bound is signaled by max_size().
template<typename _Type>
concept max_size_bounded_container_type =
    bounded_container_type<_Type> &&
    has_max_size_signal<_Type>::value;


// ===========================================================================
// III. Classification-based shorthand concepts
// ===========================================================================

// classified_bounded_container
//   concept: shorthand for any type recognized as bounded by the aggregate
// classification struct.
template<typename _Type>
concept classified_bounded_container =
    bounded_container_class<_Type>::is_bounded;

// classified_unbounded_container
//   concept: shorthand for any type recognized as unbounded by the aggregate
// classification struct.
template<typename _Type>
concept classified_unbounded_container =
    bounded_container_class<_Type>::is_unbounded;

// fully_signaled_bounded_container
//   concept: bounded container exposing one or more explicit bounding signals.
template<typename _Type>
concept fully_signaled_bounded_container =
    bounded_container_type<_Type> &&
    ( bounded_container_class<_Type>::has_extent   ||
      bounded_container_class<_Type>::has_max_size ||
      bounded_container_class<_Type>::has_capacity );

#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS


NS_END  // djinterp


#endif  // DJINTERP_BOUNDED_CONTAINER_CONCEPTS_