/******************************************************************************
* djinterp [event]                                                   event.hpp
*
* Event foundations -- the alphabet layer:
*   The base vocabulary of the event system, formalized after the companion
* note "Definition of an Event". Defines event-type introspection
* (event_traits), the verdict set P = {pass, consume} returned by every
* handler, the event-declaration macros, and the C++20 concept layer for
* classifying event tags. This is the C++11-portable root of the module:
* every other event header builds on it.
*
*   This header absorbs the former event_traits.hpp and event_concepts.hpp
* into a single foundations unit. The concept layer is compiled only where
* C++20 concepts are available; on older standards the traits remain fully
* usable and the concepts are simply omitted (no hard #error).
*
* FORMAL CORRESPONDENCE ("Definition of an Event"):
*   event type e, payload A_e   -- an event tag with a nested `payload_type`
*                                  (a std::tuple of value domains); the legacy
*                                  spelling `args_type` is still accepted.
*   alphabet  Sigma             -- the (open) set of declared event tags.
*   verdict   P = {pass,consume} -- enum class verdict; `consume` is the left
*                                  zero of handler sequencing (see handler.hpp).
*   D_EVENT / D_EVENT_EMPTY      -- declare one summand of the alphabet.
*
* COMPONENTS:
*   djinterp::verdict                 - the two-point verdict set P
*   djinterp::consumed                - true if a verdict halts propagation
*   djinterp::event_traits<_Event>    - payload_type, arity, has_name, has_args
*   D_EVENT(_name, ...)               - declare an event tag with a payload
*   D_EVENT_EMPTY(_name)              - declare an event tag with empty payload
*   djinterp::is_event                (C++20 concept)
*   djinterp::event_type, ...         (C++20 classification concepts)
*
* INTERNAL COMPONENTS:
*   djinterp::internal::index_sequence / make_index_sequence  - C++11 polyfill
*   djinterp::internal::has_payload_type / has_args_type      - tag detection
*   djinterp::internal::has_event_payload / event_payload     - payload picker
*   djinterp::internal::has_event_name                        - name detection
*   djinterp::internal::is_tuple                              - tuple detection
*   djinterp::internal::apply_impl / apply_tuple              - tuple-apply
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
* path:      /inc/djinterp/core/event/event.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.03.11
******************************************************************************/

#ifndef DJINTERP_EVENT_COMMON_
#define DJINTERP_EVENT_COMMON_ 1

// require the C++ framework header
//#ifndef DJINTERP_
//    #error "event.hpp requires djinterp.h to be included first"
//#endif
//
//#ifndef __cplusplus
//    #error "event.hpp can only be used in C++ compilation mode"
//#endif
//
//#if !D_ENV_LANG_IS_CPP11_OR_HIGHER
//    #error "event.hpp requires C++11 or higher"
//#endif

// std
#include <cstddef>
#include <tuple>
#include <type_traits>
#include <utility>
// djinterp
#include "../djinterp.hpp"


NS_DJINTERP


// =========================================================================
// I.   INDEX SEQUENCE POLYFILL (C++11)
// =========================================================================
// std::index_sequence and std::make_index_sequence are C++14. For C++11
// portability, an internal implementation is provided and used when the
// standard version is not available.

NS_INTERNAL

#if ( !D_ENV_LANG_IS_CPP14_OR_HIGHER )

    // index_sequence
    //   type: compile-time integer sequence (C++11 polyfill).
    template<std::size_t... _I>
    struct index_sequence
    {};

    // make_index_sequence_helper
    //   trait: recursive builder for index_sequence.
    template<std::size_t _N,
             std::size_t... _I>
    struct make_index_sequence_helper
        : make_index_sequence_helper<_N - 1, _N - 1, _I...>
    {};

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

    // index_sequence
    //   type: alias for the standard library sequence (C++14+).
    template<std::size_t... _I>
    using index_sequence = std::index_sequence<_I...>;

    // make_index_sequence
    //   type: alias for the standard library builder (C++14+).
    template<std::size_t _N>
    using make_index_sequence = std::make_index_sequence<_N>;

#endif  // !D_ENV_LANG_IS_CPP14_OR_HIGHER

NS_END  // internal


// =========================================================================
// II.  VERDICT (the set P)
// =========================================================================

// verdict
//   enum: the two-point verdict set P = {pass, consume} returned by every
// handler step. `pass` lets dispatch continue to the next handler in the
// effective word; `consume` cuts off the remainder for the current
// occurrence. In the handler monoid (handler.hpp) `consume` is the left
// zero and the unit `skip` always yields `pass`.
enum class verdict
{
    pass,
    consume
};

// consumed
//   function: true if the verdict halts propagation (i.e. equals
// verdict::consume). Provided for readable dispatch loops.
inline bool consumed(verdict _v)
{
    return (_v == verdict::consume);
}


// =========================================================================
// III. EVENT TAG DETECTION
// =========================================================================

NS_INTERNAL

    // has_payload_type
    //   trait: detects if _Event has a nested `payload_type` typedef
    // (the canonical spelling of the event payload A_e).
    template<typename _Event,
             typename = void>
    struct has_payload_type
    {
        static constexpr bool value = false;
    };

    template<typename _Event>
    struct has_payload_type<_Event,
        decltype(static_cast<void>(
            std::declval<typename clean_t<_Event>::payload_type>()
        ))>
    {
        static constexpr bool value = true;
    };

    // has_args_type
    //   trait: detects if _Event has a nested `args_type` typedef (the
    // legacy spelling of the payload, retained for backward compatibility).
    template<typename _Event,
             typename = void>
    struct has_args_type
    {
        static constexpr bool value = false;
    };

    template<typename _Event>
    struct has_args_type<_Event,
        decltype(static_cast<void>(
            std::declval<typename clean_t<_Event>::args_type>()
        ))>
    {
        static constexpr bool value = true;
    };

    // has_event_payload
    //   trait: true if _Event declares a payload under either the canonical
    // `payload_type` spelling or the legacy `args_type` spelling.
    template<typename _Event>
    struct has_event_payload
    {
        static constexpr bool value =
            ( has_payload_type<_Event>::value ||
              has_args_type<_Event>::value );
    };

    // event_payload
    //   trait: selects the event's payload tuple, preferring the canonical
    // `payload_type` and falling back to the legacy `args_type`.
    // primary template: canonical spelling present.
    template<typename _Event,
             bool _HasPayload = has_payload_type<_Event>::value>
    struct event_payload
    {
        using type = typename clean_t<_Event>::payload_type;
    };

    // event_payload (legacy fallback)
    //   trait: used when only the legacy `args_type` spelling is present.
    template<typename _Event>
    struct event_payload<_Event, false>
    {
        using type = typename clean_t<_Event>::args_type;
    };

    // has_event_name
    //   trait: detects if _Event has a static `name()` member returning
    // const char*.
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
                decltype(clean_t<_Event>::name()),
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
    //   function: applies a callable to a tuple of arguments (C++11/14
    // fallback for std::apply).
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
    //   function: convenience wrapper that deduces the index sequence from
    // the tuple size.
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
//   trait: compile-time introspection for event tag types. Provides access
// to the event's payload tuple A_e, its arity, and its name.
// requires: _Event must define a nested `payload_type` (or legacy
// `args_type`) that is a std::tuple specialization.
template<typename _Event>
struct event_traits
{
    static_assert(internal::has_event_payload<_Event>::value,
                  "Event type must define a nested `payload_type` (a "
                  "std::tuple of its payload value types). The legacy "
                  "spelling `args_type` is also accepted.");

    // payload_type
    //   type: the tuple of payload value domains for this event (A_e).
    using payload_type =
        typename internal::event_payload<_Event>::type;

    static_assert(internal::is_tuple<payload_type>::value,
                  "Event `payload_type` must be a std::tuple "
                  "specialization.");

    // args_type
    //   type: legacy alias for payload_type, retained for backward
    // compatibility with code written against the pre-refactor vocabulary.
    using args_type = payload_type;

    // arity
    //   constant: number of value domains the payload carries.
    static constexpr std::size_t arity =
        std::tuple_size<payload_type>::value;

    // has_name
    //   constant: true if the event provides a static name() member.
    static constexpr bool has_name =
        internal::has_event_name<_Event>::value;

    // has_args
    //   constant: true if the event carries a non-empty payload.
    static constexpr bool has_args = (arity > 0);
};


// =========================================================================
// V.   CONVENIENCE MACROS FOR EVENT DECLARATION
// =========================================================================

// D_EVENT
//   macro: declares an event tag type with the given name and payload
// value domains. Usage:
//   D_EVENT(on_resize, int, int)
// expands to:
//   struct on_resize {
//       using payload_type = std::tuple<int, int>;
//       static const char* name() { return "on_resize"; }
//   };
#define D_EVENT(_name, ...)                                                \
    struct _name                                                           \
    {                                                                      \
        using payload_type = std::tuple<__VA_ARGS__>;                      \
        static const char* name() { return #_name; }                      \
    }

// D_EVENT_EMPTY
//   macro: declares an event tag type carrying an empty payload.
// Usage:
//   D_EVENT_EMPTY(on_close)
#define D_EVENT_EMPTY(_name)                                               \
    struct _name                                                           \
    {                                                                      \
        using payload_type = std::tuple<>;                                 \
        static const char* name() { return #_name; }                      \
    }


// =========================================================================
// VI.  CONCEPT CONSTRAINTS (C++20+)
// =========================================================================
// The concept layer (formerly event_concepts.hpp) is compiled only when
// C++20 concepts are available. On earlier standards these definitions are
// omitted and the traits above remain the portable interface.

#if D_ENV_CPP_FEATURE_LANG_CONCEPTS

// ---- core event identity ----

// is_event
//   concept: constrains types that satisfy the event tag requirements:
// a nested payload (payload_type or legacy args_type) that is a std::tuple
// specialization.
template<typename _Event>
concept is_event =
    internal::has_event_payload<_Event>::value &&
    internal::is_tuple<
        typename internal::event_payload<_Event>::type>::value;

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
//   concept: constrains event types carrying one or more payload arguments.
template<typename _Type>
concept argument_event_type =
    event_type<_Type> &&
    event_traits<clean_t<_Type>>::has_args;


// ---- event name ----

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


// ---- event arity ----

// event_of_arity
//   concept: constrains event types with exactly _Arity payload arguments.
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
//   concept: constrains event types with four or more payload arguments.
template<typename _Type>
concept variadic_event_type =
    event_type<_Type> &&
    (event_traits<clean_t<_Type>>::arity > 3);

#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS


NS_END  // djinterp


#endif  // DJINTERP_EVENT_COMMON_