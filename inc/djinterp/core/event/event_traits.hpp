/******************************************************************************
* djinterp [event]                                            event_traits.hpp
*
* Event type traits:
*   Compile-time introspection for event tag types. Provides the foundational
* type machinery for the event system: tag-type detection, argument
* introspection, propagation control, and event declaration macros.
*
* COMPONENTS:
*   djinterp::event_traits<_Event>
*     - args_type   : the std::tuple of argument types
*     - arity       : number of arguments
*     - has_name    : whether the event has a static name() member
*     - has_args    : whether the event carries arguments
*
*   djinterp::event_context
*     - propagation control passed to listeners during dispatch
*
*   D_EVENT(_name, ...)       - declares an event tag with arguments
*   D_EVENT_EMPTY(_name)      - declares an event tag with no arguments
*
*   djinterp::is_event            (C++20 concept)
*
* INTERNAL COMPONENTS:
*   djinterp::internal::index_sequence         - C++11 polyfill
*   djinterp::internal::make_index_sequence    - C++11 polyfill
*   djinterp::internal::has_args_type          - tag detection
*   djinterp::internal::has_event_name         - name detection
*   djinterp::internal::is_tuple               - tuple detection
*   djinterp::internal::apply_impl             - tuple-apply helper
*   djinterp::internal::apply_tuple            - tuple-apply convenience
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
* path:      /inc/djinterp/core/event/event_traits.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.03.11
******************************************************************************/

#ifndef DJINTERP_EVENT_TRAITS_
#define DJINTERP_EVENT_TRAITS_ 1

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

    // use standard library versions
    template<std::size_t... _I>
    using index_sequence = std::index_sequence<_I...>;

    template<std::size_t _N>
    using make_index_sequence = std::make_index_sequence<_N>;

#endif  // !D_ENV_LANG_IS_CPP14_OR_HIGHER

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
    {};

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
// VI.  CONVENIENCE MACROS FOR EVENT DECLARATION
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
// VII. CONCEPT CONSTRAINTS (C++20+)
// =========================================================================

#if D_ENV_CPP_FEATURE_LANG_CONCEPTS

// is_event
//   concept: constrains types that satisfy the event tag requirements:
// must have a nested args_type that is a std::tuple specialization.
template<typename _Event>
concept is_event =
    internal::has_args_type<_Event>::value &&
    internal::is_tuple<typename _Event::args_type>::value;

#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS


NS_END  // djinterp


#endif  // DJINTERP_EVENT_TRAITS_