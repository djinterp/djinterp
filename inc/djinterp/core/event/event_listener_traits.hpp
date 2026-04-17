/******************************************************************************
* djinterp [event]                                    event_listener_traits.hpp
*
* Event listener traits:
*   Compile-time introspection for callable compatibility with event listener
* signatures. Provides SFINAE-based detection of whether a callable can serve
* as a listener for a given event type, noexcept analysis, and return-type
* extraction. Also defines the listener_id opaque handle used throughout the
* event system.
*
* COMPONENTS:
*   djinterp::listener_id
*     - opaque handle for listener management; value 0 is the null sentinel
*
*   djinterp::event_listener_traits<_Callable, _Event>
*     - is_compatible    : callable accepts (event_context&, Args...)
*     - is_nothrow       : invocation is noexcept
*     - expected_arity   : number of event arguments (excluding context)
*     - returns_void     : whether the callable returns void
*     - return_type      : the return type of the callable
*
*   djinterp::is_event_listener           (C++20 concept)
*   djinterp::is_nothrow_event_listener   (C++20 concept)
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
* path:      /inc/djinterp/core/event/event_listener_traits.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.03.11
******************************************************************************/

#ifndef DJINTERP_EVENT_LISTENER_TRAITS_
#define DJINTERP_EVENT_LISTENER_TRAITS_ 1

// require the C++ framework header
#ifndef DJINTERP_
    #error "event_listener_traits.hpp requires djinterp.h to be included first"
#endif

#ifndef __cplusplus
    #error "event_listener_traits.hpp can only be used in C++ compilation mode"
#endif

#if !D_ENV_LANG_IS_CPP11_OR_HIGHER
    #error "event_listener_traits.hpp requires C++11 or higher"
#endif

#include <cstddef>
#include <cstdint>
#include <tuple>
#include <type_traits>

#include "event_traits.hpp"


NS_DJINTERP


// =========================================================================
// I.   LISTENER IDENTIFICATION
// =========================================================================

// listener_id
//   struct: opaque handle returned from bind(), used for unbind(),
// enable(), and disable() operations. The value 0 is reserved as
// an invalid/null sentinel.
struct listener_id
{
    std::uint64_t value;

    bool operator==(const listener_id& _other) const
    {
        return (value == _other.value);
    };

    bool operator!=(const listener_id& _other) const
    {
        return (value != _other.value);
    };

    // is_valid
    //   returns true if this id refers to a real listener (non-zero).
    bool is_valid() const
    {
        return (value != 0);
    };

    // null
    //   returns an invalid listener_id sentinel.
    static listener_id null()
    {
        listener_id id;
        id.value = 0;

        return id;
    };
};


// =========================================================================
// II.  LISTENER COMPATIBILITY DETECTION
// =========================================================================

NS_INTERNAL

    // listener_compatible_helper
    //   trait: detects if _Callable can be invoked with
    // (event_context&, Args...) where Args... are the elements of
    // the event's args_type tuple.
    // primary template: incompatible (SFINAE failure case).
    template<typename _Void,
             typename _Callable,
             typename _ArgsTuple,
             typename _Indices>
    struct listener_compatible_helper
    {
        static constexpr bool value = false;
    };

    // listener_compatible_helper (success specialization)
    //   trait: well-formed when the callable accepts
    // (event_context&, tuple-element-types...).
    template<typename _Callable,
             typename... _Args,
             std::size_t... _I>
    struct listener_compatible_helper<
        decltype(static_cast<void>(
            std::declval<_Callable>()(
                std::declval<event_context&>(),
                std::declval<_Args>()...)
        )),
        _Callable,
        std::tuple<_Args...>,
        index_sequence<_I...>>
    {
        static constexpr bool value = true;
    };

    // listener_nothrow_helper
    //   trait: detects if _Callable invocation with
    // (event_context&, Args...) is noexcept.
    // primary template: not noexcept (SFINAE failure or non-noexcept).
    template<typename _Void,
             typename _Callable,
             typename _ArgsTuple,
             typename _Indices>
    struct listener_nothrow_helper
    {
        static constexpr bool value = false;
    };

    // listener_nothrow_helper (success specialization)
    //   trait: evaluates noexcept for the callable invocation.
    template<typename _Callable,
             typename... _Args,
             std::size_t... _I>
    struct listener_nothrow_helper<
        typename std::enable_if<
            noexcept(
                std::declval<_Callable>()(
                    std::declval<event_context&>(),
                    std::declval<_Args>()...))
        >::type,
        _Callable,
        std::tuple<_Args...>,
        index_sequence<_I...>>
    {
        static constexpr bool value = true;
    };

    // listener_return_type_helper
    //   trait: extracts the return type of calling _Callable with
    // the event's listener signature.
    // primary template: return type is void (fallback).
    template<typename _Void,
             typename _Callable,
             typename _ArgsTuple>
    struct listener_return_type_helper
    {
        using type = void;
        static constexpr bool is_detected = false;
    };

    // listener_return_type_helper (success specialization)
    //   trait: extracts the actual return type when the callable is
    // well-formed.
    template<typename _Callable,
             typename... _Args>
    struct listener_return_type_helper<
        decltype(static_cast<void>(
            std::declval<_Callable>()(
                std::declval<event_context&>(),
                std::declval<_Args>()...)
        )),
        _Callable,
        std::tuple<_Args...>>
    {
        using type = decltype(
            std::declval<_Callable>()(
                std::declval<event_context&>(),
                std::declval<_Args>()...));
        static constexpr bool is_detected = true;
    };

NS_END  // internal


// =========================================================================
// III. EVENT LISTENER TRAITS
// =========================================================================

// event_listener_traits
//   trait: compile-time introspection for a callable's compatibility
// with a given event type's listener signature.
// requires: _Event must satisfy event_traits requirements.
//
// provides:
//   is_compatible    - true if _Callable(event_context&, Args...) is
//                      well-formed
//   is_nothrow       - true if the invocation is noexcept
//   expected_arity   - number of event arguments (excluding context)
//   returns_void     - true if the callable returns void
//   return_type      - the return type of the callable (void if
//                      incompatible)
template<typename _Callable,
         typename _Event>
struct event_listener_traits
{
private:
    using traits    = event_traits<_Event>;
    using args_type = typename traits::args_type;
    using indices   = internal::make_index_sequence<traits::arity>;

public:
    // is_compatible
    //   constant: true if _Callable can be invoked with
    // (event_context&, Args...) matching the event's args_type.
    static constexpr bool is_compatible =
        internal::listener_compatible_helper<
            void, _Callable, args_type, indices>::value;

    // is_nothrow
    //   constant: true if the listener invocation is noexcept.
    // only meaningful when is_compatible is true.
    static constexpr bool is_nothrow =
        internal::listener_nothrow_helper<
            void, _Callable, args_type, indices>::value;

    // expected_arity
    //   constant: number of event arguments the listener must accept
    // (excluding the event_context& parameter).
    static constexpr std::size_t expected_arity = traits::arity;

    // return_type
    //   type: the return type of the callable when invoked with the
    // listener signature. Defaults to void if incompatible.
    using return_type = typename internal::listener_return_type_helper<
        void, _Callable, args_type>::type;

    // returns_void
    //   constant: true if the callable returns void.
    static constexpr bool returns_void =
        std::is_void<return_type>::value;
};


// =========================================================================
// IV.  CONCEPT CONSTRAINTS (C++20+)
// =========================================================================

#if D_ENV_CPP_FEATURE_LANG_CONCEPTS

// is_event_listener
//   concept: constrains callables that can serve as listeners for a
// given event type. The callable must accept (event_context&, Args...)
// where Args... are the elements of _Event::args_type.
template<typename _Callable,
         typename _Event>
concept is_event_listener =
    is_event<_Event> &&
    event_listener_traits<_Callable, _Event>::is_compatible;

// is_nothrow_event_listener
//   concept: constrains callables that are noexcept-compatible
// listeners for a given event type.
template<typename _Callable,
         typename _Event>
concept is_nothrow_event_listener =
    is_event_listener<_Callable, _Event> &&
    event_listener_traits<_Callable, _Event>::is_nothrow;

#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS


NS_END  // djinterp


#endif  // DJINTERP_EVENT_LISTENER_TRAITS_
