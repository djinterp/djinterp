/****************************************************************************
* djinterp [event]                                event_handler_concepts.hpp
*
*  djinterp event handler classification concepts
*   C++20 concepts layered on top of event_handler_traits.hpp.  These
* concepts provide readable `requires` constraints for event handler types,
* including core structural handler identity, typed bind/fire/queue support,
* and handler classification by dispatch capability.
*
*   This header is intentionally thin: it does not re-implement detection.
* Instead, each concept forwards to the corresponding public trait,
* variable, or concept from the event handler trait layer.
*
* TABLE OF CONTENTS
* =================
* 1.   Feature Gate
* 2.   Core Handler Concepts
* 3.   Typed Operation Concepts
* 4.   Dispatch Capability Concepts
*
* 
* path:      /inc/event/event_handler_concepts.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                        date: 2026.04.07
****************************************************************************/

#ifndef DJINTERP_EVENT_HANDLER_CONCEPTS_
#define DJINTERP_EVENT_HANDLER_CONCEPTS_ 1

#include <cstddef>
#include "event_listener_concepts.hpp"
#include "event_handler_traits.hpp"

#if !defined(__cpp_concepts) || (__cpp_concepts < 201907L)
    #error "event_handler_concepts.hpp requires C++20 concepts support."
#endif


NS_DJINTERP

// =========================================================================
// I.   CORE HANDLER CONCEPTS
// =========================================================================

// event_handler_type
//   concept: constrains types satisfying the structural event handler
// protocol.
template<typename _Type>
concept event_handler_type =
    is_event_handler_type<clean_t<_Type>>;

// non_event_handler_type
//   concept: constrains types that do not satisfy the structural event
// handler protocol.
template<typename _Type>
concept non_event_handler_type =
    !event_handler_type<_Type>;

// processing_event_handler_type
//   concept: constrains handler types supporting bounded processing.
template<typename _Type>
concept processing_event_handler_type =
    event_handler_type<_Type> &&
    event_handler_traits<clean_t<_Type>>::has_process;

// draining_event_handler_type
//   concept: constrains handler types supporting full queue draining.
template<typename _Type>
concept draining_event_handler_type =
    event_handler_type<_Type> &&
    event_handler_traits<clean_t<_Type>>::has_process_all;


// =========================================================================
// II.  TYPED OPERATION CONCEPTS
// =========================================================================

// event_handler_bindable_to
//   concept: constrains handlers that can bind a callable as a listener
// for the given event type.
template<typename _Handler,
         typename _Event,
         typename _Callable>
concept event_handler_bindable_to =
    event_handler_type<_Handler> &&
    event_type<_Event> &&
    event_listener_for<_Callable, _Event> &&
    event_handler_has_bind<clean_t<_Handler>, clean_t<_Event>, _Callable>::value;

// event_handler_fireable_for
//   concept: constrains handlers that can immediately dispatch the
// given event with the supplied argument types.
template<typename _Handler,
         typename _Event,
         typename... _Args>
concept event_handler_fireable_for =
    event_handler_type<_Handler> &&
    event_type<_Event> &&
    event_handler_has_fire<clean_t<_Handler>, clean_t<_Event>, _Args...>::value;

// event_handler_queueable_for
//   concept: constrains handlers that can defer dispatch of the given
// event with the supplied argument types.
template<typename _Handler,
         typename _Event,
         typename... _Args>
concept event_handler_queueable_for =
    event_handler_type<_Handler> &&
    event_type<_Event> &&
    event_handler_has_queue<clean_t<_Handler>, clean_t<_Event>, _Args...>::value;


// =========================================================================
// III. DISPATCH CAPABILITY CONCEPTS
// =========================================================================

// firing_event_handler_for
//   concept: constrains handlers able to bind and immediately fire a
// given event type.
template<typename _Handler,
         typename _Event,
         typename _Callable,
         typename... _Args>
concept firing_event_handler_for =
    event_handler_bindable_to<_Handler, _Event, _Callable> &&
    event_handler_fireable_for<_Handler, _Event, _Args...>;

// queueing_event_handler_for
//   concept: constrains handlers able to bind and queue a given event
// type.
template<typename _Handler,
         typename _Event,
         typename _Callable,
         typename... _Args>
concept queueing_event_handler_for =
    event_handler_bindable_to<_Handler, _Event, _Callable> &&
    event_handler_queueable_for<_Handler, _Event, _Args...>;

// full_event_handler_for
//   concept: constrains handlers able to bind, fire, and queue a given
// event type.
template<typename _Handler,
         typename _Event,
         typename _Callable,
         typename... _Args>
concept full_event_handler_for =
    event_handler_bindable_to<_Handler, _Event, _Callable> &&
    event_handler_fireable_for<_Handler, _Event, _Args...> &&
    event_handler_queueable_for<_Handler, _Event, _Args...>;


NS_END  // djinterp


#endif  // DJINTERP_EVENT_HANDLER_CONCEPTS_
