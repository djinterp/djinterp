/******************************************************************************
* djinterp [event]                                 event_listener_concepts.hpp
*
*  djinterp event listener classification concepts
*   C++20 concepts layered on top of event_listener_traits.hpp.  These
* concepts provide readable `requires` constraints for listener callables,
* including listener compatibility, noexcept listeners, void-returning
* listeners, and listener classification by the arity of the bound event.
*
*   This header is intentionally thin: it does not re-implement detection.
* Instead, each concept forwards to the corresponding public trait,
* variable, or concept from the event listener trait layer.
*
* TABLE OF CONTENTS
* =================
* 1.   Feature Gate
* 2.   Core Listener Concepts
* 3.   Return-Type Listener Concepts
* 4.   Event-Arity Listener Concepts
*
* 
* path:      /inc/djinterp/core/event/event_listener_concepts.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2026.04.07
******************************************************************************/

#ifndef DJINTERP_EVENT_LISTENER_CONCEPTS_
#define DJINTERP_EVENT_LISTENER_CONCEPTS_ 1

#include <cstddef>
#include <type_traits>
#include "event_concepts.hpp"
#include "event_listener_traits.hpp"

#if !defined(__cpp_concepts) || (__cpp_concepts < 201907L)
    #error "event_listener_concepts.hpp requires C++20 concepts support."
#endif


NS_DJINTERP

// =========================================================================
// I.   CORE LISTENER CONCEPTS
// =========================================================================

// event_listener_for
//   concept: constrains callables compatible with an event's listener
// signature.
template<typename _Callable,
         typename _Event>
concept event_listener_for =
    is_event_listener<_Callable, clean_t<_Event>>;

// nothrow_event_listener_for
//   concept: constrains noexcept-compatible listeners for a given
// event type.
template<typename _Callable,
         typename _Event>
concept nothrow_event_listener_for =
    is_nothrow_event_listener<_Callable, clean_t<_Event>>;

// throwing_event_listener_for
//   concept: constrains compatible listeners whose invocation is not
// statically known to be noexcept.
template<typename _Callable,
         typename _Event>
concept throwing_event_listener_for =
    event_listener_for<_Callable, _Event> &&
    !event_listener_traits<_Callable, clean_t<_Event>>::is_nothrow;


// =========================================================================
// II.  RETURN-TYPE LISTENER CONCEPTS
// =========================================================================

// void_event_listener_for
//   concept: constrains listeners returning void.
template<typename _Callable,
         typename _Event>
concept void_event_listener_for =
    event_listener_for<_Callable, _Event> &&
    event_listener_traits<_Callable, clean_t<_Event>>::returns_void;

// value_event_listener_for
//   concept: constrains listeners returning a non-void value.
template<typename _Callable,
         typename _Event>
concept value_event_listener_for =
    event_listener_for<_Callable, _Event> &&
    !event_listener_traits<_Callable, clean_t<_Event>>::returns_void;

// listener_returning
//   concept: constrains listeners whose return type exactly matches
// _Return.
template<typename _Callable,
         typename _Event,
         typename _Return>
concept listener_returning =
    event_listener_for<_Callable, _Event> &&
    std::is_same_v<
        typename event_listener_traits<
            _Callable,
            clean_t<_Event>>::return_type,
        _Return>;


// =========================================================================
// III. EVENT-ARITY LISTENER CONCEPTS
// =========================================================================

// listener_for_event_of_arity
//   concept: constrains listeners for an event carrying exactly
// _Arity payload arguments.
template<typename _Callable,
         typename _Event,
         std::size_t _Arity>
concept listener_for_event_of_arity =
    event_listener_for<_Callable, _Event> &&
    (event_listener_traits<_Callable, clean_t<_Event>>::expected_arity ==
     _Arity);

// nullary_event_listener_for
//   concept: constrains listeners for empty events.
template<typename _Callable,
         typename _Event>
concept nullary_event_listener_for =
    listener_for_event_of_arity<_Callable, _Event, 0>;

// unary_event_listener_for
//   concept: constrains listeners for unary events.
template<typename _Callable,
         typename _Event>
concept unary_event_listener_for =
    listener_for_event_of_arity<_Callable, _Event, 1>;

// binary_event_listener_for
//   concept: constrains listeners for binary events.
template<typename _Callable,
         typename _Event>
concept binary_event_listener_for =
    listener_for_event_of_arity<_Callable, _Event, 2>;

// ternary_event_listener_for
//   concept: constrains listeners for ternary events.
template<typename _Callable,
         typename _Event>
concept ternary_event_listener_for =
    listener_for_event_of_arity<_Callable, _Event, 3>;

// variadic_event_listener_for
//   concept: constrains listeners for events carrying four or more
// payload arguments.
template<typename _Callable,
         typename _Event>
concept variadic_event_listener_for =
    event_listener_for<_Callable, _Event> &&
    (event_listener_traits<_Callable, clean_t<_Event>>::expected_arity > 3);


NS_END  // djinterp


#endif  // DJINTERP_EVENT_LISTENER_CONCEPTS_
