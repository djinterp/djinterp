/******************************************************************************
* djinterp [event]                                            event_traits.hpp
*
* Event type traits and listener compatibility traits:
*   Compile-time introspection for event tag types and callable compatibility
* with event listener signatures. Provides the foundational type machinery
* for the event system: tag-type detection, argument introspection, listener
* signature verification, and propagation control.
*
* COMPONENTS:
*   djinterp::event_traits<_Event>
*     - args_type   : the std::tuple of argument types
*     - arity       : number of arguments
*     - has_name    : whether the event has a static name() member
*
*   djinterp::event_listener_traits<_Callable, _Event>
*     - is_compatible    : callable accepts (event_context&, Args...)
*     - is_nothrow       : invocation is noexcept (C++11 detection)
*     - expected_arity   : number of event arguments (excluding context)
*
*   djinterp::event_context
*     - propagation control passed to listeners during dispatch
*
*   D_EVENT(_name, ...)       - declares an event tag with arguments
*   D_EVENT_EMPTY(_name)      - declares an event tag with no arguments
*
*   djinterp::is_event            (C++20 concept)
*   djinterp::is_event_listener   (C++20 concept)
*
* FEATURE DEPENDENCIES:
*   D_ENV_CPP_FEATURE_LANG_VARIADIC_TEMPLATES - parameter packs
*   D_ENV_CPP_FEATURE_LANG_ALIAS_TEMPLATES    - using aliases
*   D_ENV_CPP_FEATURE_LANG_CONCEPTS           - concept constraints (C++20)
*   D_ENV_CPP_FEATURE_LANG_NOEXCEPT_FUNCTION_TYPE - noexcept detection
*
* PORTABLE ACROSS:
*   C++11, C++14, C++17, C++20, C++23, C++26
*
* path:      \inc\event\event_traits.hpp
* author(s): Samuel 'teer' Neal-Blim                          date: 2026.03.11
******************************************************************************/

#ifndef DJINTERP_CPP_EVENT_TRAITS_
#define DJINTERP_CPP_EVENT_TRAITS_ 1

// require the C++ framework header
#ifndef DJINTERP_
    #error "event_traits.hpp requires djinterp.h to be included first"
#endif

#ifndef __cplusplus
    #error "event_traits.hpp can only be used in C++ compilation mode"
#endif

#if !D_ENV_LANG_IS_CPP11_OR_HIGHER
    #error "event_traits.hpp requires C++11 or higher"
#endif

#include <cstddef>
#include <tuple>
#include <type_traits>
#include <utility>


NS_DJINTERP


// =========================================================================
// I.   INDEX SEQUENCE POLYFILL (C++11)
// =========================================================================
// std::index_sequence and std::make_index_sequence are C++14. For
// C++11 portability, we provide an internal implementation that is
// used when the standard version is not available.

NS_INTERNAL

#if (__cplusplus < 201402L)

    // index_sequence
    //   type: compile-time integer sequence (C++11 polyfill).
    template<std::size_t... _I>
    struct index_sequence
    {
    };

    // make_index_sequence_helper
    //   trait: recursive builder for index_sequence.
    template<std::size_t _N,
             std::size_t... _I>
    struct make_index_sequence_helper
        : make_index_sequence_helper<_N - 1, _N - 1, _I...>
    {
    };

    // make_index_sequence_helper<0, ...>
    //   trait: base case; produces the final index_sequence.
    template<std::size_t... _I>
    struct make_index_sequence_helper<0, _I...>
    {
        using type = index_sequence<_I...>;
    };

    // make_index_sequence
    //   type: alias for the constructed index_sequence.
    template<std::size_t _N>
    using make_index_sequence =
        typename make_index_sequence_helper<_N>::type;

#else

    // use standard library versions
    template<std::size_t... _I>
    using index_sequence = std::index_sequence<_I...>;

    template<std::size_t _N>
    using make_index_sequence = std::make_index_sequence<_N>;

#endif  // __cplusplus < 201402L

NS_END  // internal


// =========================================================================
// II.  FORWARD DECLARATIONS
// =========================================================================

class event_context;


// =========================================================================
// III. EVENT TAG DETECTION
// =========================================================================

NS_INTERNAL

    // has_args_type
    //   trait: detects if _Event has a nested `args_type` typedef.
    template<typename _Event,
             typename = void>
    struct has_args_type
    {
        static constexpr bool value = false;
    };

    template<typename _Event>
    struct has_args_type<_Event,
        decltype(static_cast<void>(
            std::declval<typename _Event::args_type>()
        ))>
    {
        static constexpr bool value = true;
    };

    // has_event_name
    //   trait: detects if _Event has a static `name()` member
    // returning const char*.
    template<typename _Event,
             typename = void>
    struct has_event_name
    {
        static constexpr bool value = false;
    };

    template<typename _Event>
    struct has_event_name<_Event,
        typename std::enable_if<
            std::is_same<
                decltype(_Event::name()),
                const char*
            >::value
        >::type>
    {
        static constexpr bool value = true;
    };

    // is_tuple
    //   trait: detects if _T is a std::tuple specialization.
    template<typename _T>
    struct is_tuple
    {
        static constexpr bool value = false;
    };

    template<typename... _Types>
    struct is_tuple<std::tuple<_Types...>>
    {
        static constexpr bool value = true;
    };

    // apply_impl
    //   function: applies a callable to a tuple of arguments
    // (C++11/14 fallback for std::apply).
    template<typename _F,
             typename _Tuple,
             std::size_t... _I>
    auto apply_impl(_F&&    _f,
                    _Tuple& _t,
                    index_sequence<_I...>)
        -> decltype(_f(std::get<_I>(_t)...))
    {
        return _f(std::get<_I>(_t)...);
    }

    // apply_tuple
    //   function: convenience wrapper that deduces the index sequence
    // from the tuple size.
    template<typename _F,
             typename _Tuple>
    auto apply_tuple(_F&&    _f,
                     _Tuple& _t)
        -> decltype(apply_impl(
            std::forward<_F>(_f),
            _t,
            make_index_sequence<
                std::tuple_size<
                    typename std::remove_reference<_Tuple>::type
                >::value>{}))
    {
        return apply_impl(
            std::forward<_F>(_f),
            _t,
            make_index_sequence<
                std::tuple_size<
                    typename std::remove_reference<_Tuple>::type
                >::value>{});
    }

NS_END  // internal


// =========================================================================
// IV.  EVENT TRAITS
// =========================================================================

// event_traits
//   trait: compile-time introspection for event tag types. Provides
// access to the event's argument types, arity, and name.
// requires: _Event must define a nested `args_type` as a std::tuple.
template<typename _Event>
struct event_traits
{
    static_assert(internal::has_args_type<_Event>::value,
                  "Event type must define a nested `args_type` "
                  "as a std::tuple of its argument types.");

    // args_type
    //   type: the tuple of argument types for this event.
    using args_type = typename _Event::args_type;

    static_assert(internal::is_tuple<args_type>::value,
                  "Event `args_type` must be a std::tuple "
                  "specialization.");

    // arity
    //   constant: number of arguments this event carries.
    static constexpr std::size_t arity =
        std::tuple_size<args_type>::value;

    // has_name
    //   constant: true if the event provides a static name() member.
    static constexpr bool has_name =
        internal::has_event_name<_Event>::value;

    // has_args
    //   constant: true if the event carries one or more arguments.
    static constexpr bool has_args = (arity > 0);
};


// =========================================================================
// V.   PROPAGATION CONTROL
// =========================================================================

// event_context
//   class: passed to each listener during dispatch. Allows a listener
// to stop propagation to subsequent listeners for the same event.
class event_context
{
public:
    event_context()
        : m_consumed(false)
    {
    };

    // consume
    //   marks this event as consumed; subsequent listeners will not
    // be invoked for this dispatch.
    void consume()
    {
        m_consumed = true;
    };

    // is_consumed
    //   returns true if a listener has consumed this event.
    bool is_consumed() const
    {
        return m_consumed;
    };

    // reset
    //   clears consumed state for reuse.
    void reset()
    {
        m_consumed = false;
    };

private:
    bool m_consumed;
};


// =========================================================================
// VI.  EVENT LISTENER TRAITS
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
// VII. CONVENIENCE MACROS FOR EVENT DECLARATION
// =========================================================================

// D_EVENT
//   macro: declares an event tag type with the given name and argument
// types. Usage:
//   D_EVENT(on_resize, int, int)
// expands to:
//   struct on_resize {
//       using args_type = std::tuple<int, int>;
//       static const char* name() { return "on_resize"; }
//   };
#define D_EVENT(_name, ...)                                                \
    struct _name                                                           \
    {                                                                      \
        using args_type = std::tuple<__VA_ARGS__>;                         \
        static const char* name() { return #_name; }                       \
    }

// D_EVENT_EMPTY
//   macro: declares an event tag type with no arguments.
// Usage:
//   D_EVENT_EMPTY(on_close)
#define D_EVENT_EMPTY(_name)                                               \
    struct _name                                                           \
    {                                                                      \
        using args_type = std::tuple<>;                                    \
        static const char* name() { return #_name; }                       \
    }


// =========================================================================
// VIII. CONCEPT CONSTRAINTS (C++20+)
// =========================================================================

#if D_ENV_CPP_FEATURE_LANG_CONCEPTS

// is_event
//   concept: constrains types that satisfy the event tag requirements:
// must have a nested args_type that is a std::tuple specialization.
template<typename _Event>
concept is_event =
    internal::has_args_type<_Event>::value &&
    internal::is_tuple<typename _Event::args_type>::value;

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


#endif  // DJINTERP_CPP_EVENT_TRAITS_
