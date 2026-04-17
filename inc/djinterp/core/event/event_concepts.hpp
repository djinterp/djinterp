/******************************************************************************
* djinterp [event]                                          event_concepts.hpp
*
*  djinterp event classification concepts
*   C++20 concepts layered on top of event_traits.hpp.  These concepts
* provide readable `requires` constraints for event tag types, including
* generic event identity, naming, argument presence, and fixed-arity event
* classification.
*
*   This header is intentionally thin: it does not re-implement detection.
* Instead, each concept forwards to the corresponding public trait,
* variable, or concept from the event trait layer.
*
* TABLE OF CONTENTS
* =================
* 1.   Feature Gate
* 2.   Core Event Concepts
* 3.   Event Name Concepts
* 4.   Event Arity Concepts
*
* 
* path:      /inc/djinterp/core/event/event_concepts.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2026.04.07
******************************************************************************/

#ifndef DJINTERP_EVENT_CONCEPTS_
#define DJINTERP_EVENT_CONCEPTS_ 1

#include <cstddef>
#include <type_traits>
#include "event_traits.hpp"

#if !defined(__cpp_concepts) || (__cpp_concepts < 201907L)
    #error "event_concepts.hpp requires C++20 concepts support."
#endif


NS_DJINTERP

// =========================================================================
// I.   CORE EVENT CONCEPTS
// =========================================================================

// event_type
//   concept: constrains types satisfying the event tag protocol.
template<typename _Type>
concept event_type =
    is_event<clean_t<_Type>>;

// non_event_type
//   concept: constrains types that do not satisfy the event tag protocol.
template<typename _Type>
concept non_event_type =
    !event_type<_Type>;

// empty_event_type
//   concept: constrains event types carrying no payload arguments.
template<typename _Type>
concept empty_event_type =
    event_type<_Type> &&
    !event_traits<clean_t<_Type>>::has_args;

// argument_event_type
//   concept: constrains event types carrying one or more payload
// arguments.
template<typename _Type>
concept argument_event_type =
    event_type<_Type> &&
    event_traits<clean_t<_Type>>::has_args;


// =========================================================================
// II.  EVENT NAME CONCEPTS
// =========================================================================

// named_event_type
//   concept: constrains event types exposing a static name() member.
template<typename _Type>
concept named_event_type =
    event_type<_Type> &&
    event_traits<clean_t<_Type>>::has_name;

// unnamed_event_type
//   concept: constrains event types without a static name() member.
template<typename _Type>
concept unnamed_event_type =
    event_type<_Type> &&
    !event_traits<clean_t<_Type>>::has_name;


// =========================================================================
// III. EVENT ARITY CONCEPTS
// =========================================================================

// event_of_arity
//   concept: constrains event types with exactly _Arity payload
// arguments.
template<typename _Type,
         std::size_t _Arity>
concept event_of_arity =
    event_type<_Type> &&
    (event_traits<clean_t<_Type>>::arity == _Arity);

// nullary_event_type
//   concept: constrains event types with zero payload arguments.
template<typename _Type>
concept nullary_event_type =
    event_of_arity<_Type, 0>;

// unary_event_type
//   concept: constrains event types with one payload argument.
template<typename _Type>
concept unary_event_type =
    event_of_arity<_Type, 1>;

// binary_event_type
//   concept: constrains event types with two payload arguments.
template<typename _Type>
concept binary_event_type =
    event_of_arity<_Type, 2>;

// ternary_event_type
//   concept: constrains event types with three payload arguments.
template<typename _Type>
concept ternary_event_type =
    event_of_arity<_Type, 3>;

// variadic_event_type
//   concept: constrains event types with four or more payload
// arguments.
template<typename _Type>
concept variadic_event_type =
    event_type<_Type> &&
    (event_traits<clean_t<_Type>>::arity > 3);


NS_END  // djinterp


#endif  // DJINTERP_EVENT_CONCEPTS_
