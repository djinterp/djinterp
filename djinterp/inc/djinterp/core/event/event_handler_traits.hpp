/******************************************************************************
* djinterp [event]                                     event_handler_traits.hpp
*
* Event handler type traits:
*   Compile-time structural detection for types that satisfy the event
* handler interface. Verifies that a type provides the operations expected
* by code that consumes an event handler: listener management, immediate
* and deferred dispatch, and query functions.
*
*   Non-template operations (unbind, enable, disable, is_enabled, contains,
* listener_count, enabled_count, pending_events, process, process_all) are
* detected directly. Template operations (bind, fire, queue) require a
* concrete event type for detection and are provided as separate typed
* checks: has_bind<_Handler, _Event, _Callable>, has_fire<_Handler, _Event>,
* and has_queue<_Handler, _Event>.
*
* COMPONENTS:
*   djinterp::event_handler_traits<_Handler>
*     - has_unbind           : unbind(listener_id) → bool
*     - has_enable           : enable(listener_id) → bool
*     - has_disable          : disable(listener_id) → bool
*     - has_is_enabled       : is_enabled(listener_id) const → bool
*     - has_contains         : contains(listener_id) const → bool
*     - has_listener_count   : listener_count() const → size_t
*     - has_enabled_count    : enabled_count() const → size_t
*     - has_pending_events   : pending_events() const → size_t
*     - has_process          : process(size_t) → size_t
*     - has_process_all      : process_all() → size_t
*     - is_event_handler     : all required non-template ops present
*
*   djinterp::event_handler_has_bind<_Handler, _Event, _Callable>
*   djinterp::event_handler_has_fire<_Handler, _Event, _Args...>
*   djinterp::event_handler_has_queue<_Handler, _Event, _Args...>
*
*   djinterp::is_event_handler_type   (C++20 concept)
*
* FEATURE DEPENDENCIES:
*   D_ENV_CPP_FEATURE_LANG_VARIADIC_TEMPLATES - parameter packs
*   D_ENV_CPP_FEATURE_LANG_ALIAS_TEMPLATES    - using aliases
*   D_ENV_CPP_FEATURE_LANG_CONCEPTS           - concept constraints (C++20)
*
* PORTABLE ACROSS:
*   C++11, C++14, C++17, C++20, C++23, C++26
*
* 
* path:      /inc/djinterp/core/event/event_handler_traits.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.03.12
******************************************************************************/

#ifndef DJINTERP_EVENT_HANDLER_TRAITS_
#define DJINTERP_EVENT_HANDLER_TRAITS_ 1

// require the C++ framework header
#ifndef DJINTERP_
    #error "event_handler_traits.hpp requires djinterp.h to be included first"
#endif

#ifndef __cplusplus
    #error "event_handler_traits.hpp can only be used in C++ compilation mode"
#endif

#if !D_ENV_LANG_IS_CPP11_OR_HIGHER
    #error "event_handler_traits.hpp requires C++11 or higher"
#endif

#include <cstddef>
#include <type_traits>

#include "event_listener_traits.hpp"


NS_DJINTERP


// =========================================================================
// I.   NON-TEMPLATE OPERATION DETECTION
// =========================================================================

NS_INTERNAL

    // ---- listener management ops ----

    // has_handler_unbind
    //   trait: detects unbind(listener_id) returning bool.
    template<typename _Handler,
             typename = void>
    struct has_handler_unbind
    {
        static constexpr bool value = false;
    };

    template<typename _Handler>
    struct has_handler_unbind<_Handler,
        typename std::enable_if<
            std::is_same<
                decltype(std::declval<_Handler&>().unbind(
                    std::declval<listener_id>())),
                bool
            >::value
        >::type>
    {
        static constexpr bool value = true;
    };

    // has_handler_enable
    //   trait: detects enable(listener_id) returning bool.
    template<typename _Handler,
             typename = void>
    struct has_handler_enable
    {
        static constexpr bool value = false;
    };

    template<typename _Handler>
    struct has_handler_enable<_Handler,
        typename std::enable_if<
            std::is_same<
                decltype(std::declval<_Handler&>().enable(
                    std::declval<listener_id>())),
                bool
            >::value
        >::type>
    {
        static constexpr bool value = true;
    };

    // has_handler_disable
    //   trait: detects disable(listener_id) returning bool.
    template<typename _Handler,
             typename = void>
    struct has_handler_disable
    {
        static constexpr bool value = false;
    };

    template<typename _Handler>
    struct has_handler_disable<_Handler,
        typename std::enable_if<
            std::is_same<
                decltype(std::declval<_Handler&>().disable(
                    std::declval<listener_id>())),
                bool
            >::value
        >::type>
    {
        static constexpr bool value = true;
    };

    // has_handler_is_enabled
    //   trait: detects is_enabled(listener_id) const returning bool.
    template<typename _Handler,
             typename = void>
    struct has_handler_is_enabled
    {
        static constexpr bool value = false;
    };

    template<typename _Handler>
    struct has_handler_is_enabled<_Handler,
        typename std::enable_if<
            std::is_same<
                decltype(std::declval<const _Handler&>().is_enabled(
                    std::declval<listener_id>())),
                bool
            >::value
        >::type>
    {
        static constexpr bool value = true;
    };

    // has_handler_contains
    //   trait: detects contains(listener_id) const returning bool.
    template<typename _Handler,
             typename = void>
    struct has_handler_contains
    {
        static constexpr bool value = false;
    };

    template<typename _Handler>
    struct has_handler_contains<_Handler,
        typename std::enable_if<
            std::is_same<
                decltype(std::declval<const _Handler&>().contains(
                    std::declval<listener_id>())),
                bool
            >::value
        >::type>
    {
        static constexpr bool value = true;
    };

    // ---- query ops ----

    // has_handler_listener_count
    //   trait: detects listener_count() const returning size_t.
    template<typename _Handler,
             typename = void>
    struct has_handler_listener_count
    {
        static constexpr bool value = false;
    };

    template<typename _Handler>
    struct has_handler_listener_count<_Handler,
        typename std::enable_if<
            std::is_same<
                decltype(std::declval<const _Handler&>().listener_count()),
                std::size_t
            >::value
        >::type>
    {
        static constexpr bool value = true;
    };

    // has_handler_enabled_count
    //   trait: detects enabled_count() const returning size_t.
    template<typename _Handler,
             typename = void>
    struct has_handler_enabled_count
    {
        static constexpr bool value = false;
    };

    template<typename _Handler>
    struct has_handler_enabled_count<_Handler,
        typename std::enable_if<
            std::is_same<
                decltype(std::declval<const _Handler&>().enabled_count()),
                std::size_t
            >::value
        >::type>
    {
        static constexpr bool value = true;
    };

    // has_handler_pending_events
    //   trait: detects pending_events() const returning size_t.
    template<typename _Handler,
             typename = void>
    struct has_handler_pending_events
    {
        static constexpr bool value = false;
    };

    template<typename _Handler>
    struct has_handler_pending_events<_Handler,
        typename std::enable_if<
            std::is_same<
                decltype(std::declval<const _Handler&>().pending_events()),
                std::size_t
            >::value
        >::type>
    {
        static constexpr bool value = true;
    };

    // ---- dispatch ops ----

    // has_handler_process
    //   trait: detects process(size_t) returning size_t.
    template<typename _Handler,
             typename = void>
    struct has_handler_process
    {
        static constexpr bool value = false;
    };

    template<typename _Handler>
    struct has_handler_process<_Handler,
        typename std::enable_if<
            std::is_same<
                decltype(std::declval<_Handler&>().process(
                    std::declval<std::size_t>())),
                std::size_t
            >::value
        >::type>
    {
        static constexpr bool value = true;
    };

    // has_handler_process_all
    //   trait: detects process_all() returning size_t.
    template<typename _Handler,
             typename = void>
    struct has_handler_process_all
    {
        static constexpr bool value = false;
    };

    template<typename _Handler>
    struct has_handler_process_all<_Handler,
        typename std::enable_if<
            std::is_same<
                decltype(std::declval<_Handler&>().process_all()),
                std::size_t
            >::value
        >::type>
    {
        static constexpr bool value = true;
    };

NS_END  // internal


// =========================================================================
// II.  EVENT HANDLER TRAITS
// =========================================================================

// event_handler_traits
//   trait: compile-time structural detection for types that satisfy
// the event handler interface. Validates non-template operations;
// template operations (bind, fire, queue) require typed detection
// via the separate helpers in section III.
//
// note: structural check only; does not verify semantics.
template<typename _Handler>
struct event_handler_traits
{
    // ---- listener management detection ----

    // has_unbind
    //   constant: true if unbind(listener_id) → bool.
    static constexpr bool has_unbind =
        internal::has_handler_unbind<_Handler>::value;

    // has_enable
    //   constant: true if enable(listener_id) → bool.
    static constexpr bool has_enable =
        internal::has_handler_enable<_Handler>::value;

    // has_disable
    //   constant: true if disable(listener_id) → bool.
    static constexpr bool has_disable =
        internal::has_handler_disable<_Handler>::value;

    // has_is_enabled
    //   constant: true if is_enabled(listener_id) const → bool.
    static constexpr bool has_is_enabled =
        internal::has_handler_is_enabled<_Handler>::value;

    // has_contains
    //   constant: true if contains(listener_id) const → bool.
    static constexpr bool has_contains =
        internal::has_handler_contains<_Handler>::value;

    // ---- query detection ----

    // has_listener_count
    //   constant: true if listener_count() const → size_t.
    static constexpr bool has_listener_count =
        internal::has_handler_listener_count<_Handler>::value;

    // has_enabled_count
    //   constant: true if enabled_count() const → size_t.
    static constexpr bool has_enabled_count =
        internal::has_handler_enabled_count<_Handler>::value;

    // has_pending_events
    //   constant: true if pending_events() const → size_t.
    static constexpr bool has_pending_events =
        internal::has_handler_pending_events<_Handler>::value;

    // ---- dispatch detection ----

    // has_process
    //   constant: true if process(size_t) → size_t.
    static constexpr bool has_process =
        internal::has_handler_process<_Handler>::value;

    // has_process_all
    //   constant: true if process_all() → size_t.
    static constexpr bool has_process_all =
        internal::has_handler_process_all<_Handler>::value;

    // ---- composite detection ----

    // is_event_handler
    //   constant: true if _Handler provides all required non-template
    // operations for use as an event handler.
    static constexpr bool is_event_handler =
        ( has_unbind         &&
          has_enable         &&
          has_disable        &&
          has_is_enabled     &&
          has_contains       &&
          has_listener_count &&
          has_enabled_count  &&
          has_pending_events &&
          has_process        &&
          has_process_all );
};


// =========================================================================
// III. TYPED TEMPLATE OPERATION DETECTION
// =========================================================================
// Template methods (bind, fire, queue) cannot be detected without
// concrete types. These helpers accept the handler, an event type,
// and (for bind) a callable type.

NS_INTERNAL

    // has_handler_bind
    //   trait: detects handler.bind<_Event>(callable) returning
    // listener_id.
    template<typename _Void,
             typename _Handler,
             typename _Event,
             typename _Callable>
    struct has_handler_bind
    {
        static constexpr bool value = false;
    };

    template<typename _Handler,
             typename _Event,
             typename _Callable>
    struct has_handler_bind<
        typename std::enable_if<
            std::is_same<
                decltype(std::declval<_Handler&>().template bind<_Event>(
                    std::declval<_Callable>())),
                listener_id
            >::value
        >::type,
        _Handler,
        _Event,
        _Callable>
    {
        static constexpr bool value = true;
    };

    // has_handler_fire
    //   trait: detects handler.fire<_Event>(args...) returning
    // size_t.
    template<typename _Void,
             typename _Handler,
             typename _Event,
             typename... _Args>
    struct has_handler_fire
    {
        static constexpr bool value = false;
    };

    template<typename _Handler,
             typename _Event,
             typename... _Args>
    struct has_handler_fire<
        typename std::enable_if<
            std::is_same<
                decltype(std::declval<_Handler&>().template fire<_Event>(
                    std::declval<_Args>()...)),
                std::size_t
            >::value
        >::type,
        _Handler,
        _Event,
        _Args...>
    {
        static constexpr bool value = true;
    };

    // has_handler_queue
    //   trait: detects handler.queue<_Event>(args...) as a
    // well-formed expression.
    template<typename _Void,
             typename _Handler,
             typename _Event,
             typename... _Args>
    struct has_handler_queue
    {
        static constexpr bool value = false;
    };

    template<typename _Handler,
             typename _Event,
             typename... _Args>
    struct has_handler_queue<
        decltype(static_cast<void>(
            std::declval<_Handler&>().template queue<_Event>(
                std::declval<_Args>()...)
        )),
        _Handler,
        _Event,
        _Args...>
    {
        static constexpr bool value = true;
    };

NS_END  // internal

// event_handler_has_bind
//   trait: evaluates to true if _Handler.bind<_Event>(_Callable)
// is well-formed and returns listener_id.
template<typename _Handler,
         typename _Event,
         typename _Callable>
struct event_handler_has_bind
{
    static constexpr bool value =
        internal::has_handler_bind<
            void, _Handler, _Event, _Callable>::value;
};

// event_handler_has_fire
//   trait: evaluates to true if _Handler.fire<_Event>(_Args...)
// is well-formed and returns size_t.
template<typename _Handler,
         typename _Event,
         typename... _Args>
struct event_handler_has_fire
{
    static constexpr bool value =
        internal::has_handler_fire<
            void, _Handler, _Event, _Args...>::value;
};

// event_handler_has_queue
//   trait: evaluates to true if _Handler.queue<_Event>(_Args...)
// is well-formed.
template<typename _Handler,
         typename _Event,
         typename... _Args>
struct event_handler_has_queue
{
    static constexpr bool value =
        internal::has_handler_queue<
            void, _Handler, _Event, _Args...>::value;
};


// =========================================================================
// IV.  CONCEPT CONSTRAINTS (C++20+)
// =========================================================================

#if D_ENV_CPP_FEATURE_LANG_CONCEPTS

// is_event_handler_type
//   concept: constrains types that satisfy the event handler
// structural requirements (non-template operations).
template<typename _Handler>
concept is_event_handler_type =
    event_handler_traits<_Handler>::is_event_handler;

#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS


NS_END  // djinterp


#endif  // DJINTERP_EVENT_HANDLER_TRAITS_
