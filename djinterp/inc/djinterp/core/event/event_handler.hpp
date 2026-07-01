/******************************************************************************
* djinterp [event]                                                 handler.hpp
*
* The handler -- a step with a verdict:
*   A handler is the single primitive of the event layer: given the (ambient)
* state and an occurrence's payload, it yields a verdict in P = {pass,
* consume}. This header provides the opaque handler_id handle, compile-time
* introspection of callable/handler compatibility (handler_traits), the
* sequencing monoid (seq with unit skip and left zero consume), and the
* invocation-normalization helpers the registry uses to drive type-erased
* handlers.
*
*   This header absorbs the former event_listener_traits.hpp and
* event_listener_concepts.hpp. The most significant change from the
* pre-refactor "listener" model is that a handler now *returns* its verdict
* rather than mutating a context object: the signature is verdict(payload...)
* (a void-returning callable is accepted and treated as an always-pass
* handler, since the monoid unit skip yields pass).
*
* FORMAL CORRESPONDENCE ("Definition of an Event"):
*   handler  h : S x A_e -> S x P   -- a callable verdict(payload...); state S
*                                     is ambient (captured), the verdict is
*                                     returned (see handler_traits).
*   sequencing  h1 ; h2             -- seq(h1, h2)
*   unit        skip(s,a)=(s,pass)  -- skip() / skip_t
*   left zero   consume             -- a handler returning verdict::consume
*                                     short-circuits the remainder.
*
* COMPONENTS:
*   djinterp::handler_id                       - opaque handler handle
*   djinterp::handler_traits<_Callable,_Event> - compatibility introspection
*   djinterp::skip_t / djinterp::skip()        - the monoid unit
*   djinterp::seq(h1, h2)                       - handler sequencing
*   djinterp::is_handler / handler_for ...      (C++20 concepts)
*
* INTERNAL COMPONENTS:
*   djinterp::internal::invoke_normalized   - call a handler, void -> pass
*   djinterp::internal::apply_handler       - unpack a payload tuple and call
*   djinterp::internal::handler_invoke_result / handler_nothrow_helper
*   djinterp::internal::seq_handler         - the sequenced-handler functor
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
* path:      /inc/djinterp/core/event/handler.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.03.11
******************************************************************************/

#ifndef DJINTERP_EVENT_HANDLER_PRIMITIVE_
#define DJINTERP_EVENT_HANDLER_PRIMITIVE_ 1

// require the C++ framework header
//#ifndef DJINTERP_
//    #error "handler.hpp requires djinterp.h to be included first"
//#endif
//
//#ifndef __cplusplus
//    #error "handler.hpp can only be used in C++ compilation mode"
//#endif
//
//#if !D_ENV_LANG_IS_CPP11_OR_HIGHER
//    #error "handler.hpp requires C++11 or higher"
//#endif

// std
#include <cstddef>
#include <cstdint>
#include <tuple>
#include <type_traits>
#include <utility>
// djinterp
#include "../djinterp.hpp"
#include "./event_common.hpp"


NS_DJINTERP


// =========================================================================
// I.   HANDLER IDENTIFICATION
// =========================================================================

// handler_id
//   struct: opaque handle returned from registry bind(), used for unbind(),
// enable(), and disable() operations (the named letter ell of the registry
// word). The value 0 is reserved as an invalid/null sentinel.
struct handler_id
{
    std::uint64_t value;

    bool operator==(const handler_id& _other) const
    {
        return (value == _other.value);
    };

    bool operator!=(const handler_id& _other) const
    {
        return (value != _other.value);
    };

    // is_valid
    //   returns true if this id refers to a real handler (non-zero).
    bool is_valid() const
    {
        return (value != 0);
    };

    // null
    //   returns an invalid handler_id sentinel.
    static handler_id null()
    {
        handler_id id;
        id.value = 0;

        return id;
    };
};


// =========================================================================
// II.  HANDLER INVOCATION (verdict normalization)
// =========================================================================

NS_INTERNAL

    // invoke_normalized (void-returning overload)
    //   function: invokes a void-returning handler and reports the unit
    // verdict pass. This is the adapter that lets a plain void(payload...)
    // callable serve as an always-pass handler.
    template<typename _H,
             typename... _A>
    typename std::enable_if<
        std::is_void<decltype(std::declval<_H&>()(std::declval<_A>()...))>::value,
        verdict>::type
    invoke_normalized(_H&     _h,
                      _A&&... _a)
    {
        _h(std::forward<_A>(_a)...);

        return verdict::pass;
    }

    // invoke_normalized (verdict-returning overload)
    //   function: invokes a verdict-returning handler and forwards its
    // verdict unchanged.
    template<typename _H,
             typename... _A>
    typename std::enable_if<
        !std::is_void<
            decltype(std::declval<_H&>()(std::declval<_A>()...))
        >::value,
        verdict>::type
    invoke_normalized(_H&     _h,
                      _A&&... _a)
    {
        return _h(std::forward<_A>(_a)...);
    }

    // apply_handler
    //   function: unpacks a payload tuple and invokes the handler with its
    // elements, normalizing the result to a verdict. Used by the registry's
    // type-erased dispatch closure.
    template<typename _H,
             typename _Tuple,
             std::size_t... _I>
    verdict apply_handler(_H&             _h,
                          _Tuple&         _payload,
                          index_sequence<_I...>)
    {
        return invoke_normalized(_h, std::get<_I>(_payload)...);
    }

NS_END  // internal


// =========================================================================
// III. HANDLER COMPATIBILITY DETECTION
// =========================================================================

NS_INTERNAL

    // handler_invoke_result
    //   trait: detects whether _Callable is invocable with the payload's
    // value domains and, if so, extracts the result type.
    // primary template: not invocable (SFINAE failure case).
    template<typename _Void,
             typename _Callable,
             typename _Payload>
    struct handler_invoke_result
    {
        using type = void;
        static constexpr bool value = false;
    };

    // handler_invoke_result (success specialization)
    //   trait: well-formed when the callable accepts the payload elements.
    template<typename _Callable,
             typename... _Args>
    struct handler_invoke_result<
        decltype(static_cast<void>(
            std::declval<_Callable&>()(std::declval<_Args>()...)
        )),
        _Callable,
        std::tuple<_Args...>>
    {
        using type = decltype(
            std::declval<_Callable&>()(std::declval<_Args>()...));
        static constexpr bool value = true;
    };

    // handler_nothrow_helper
    //   trait: detects whether invoking _Callable with the payload's value
    // domains is noexcept.
    // primary template: not noexcept (SFINAE failure or throwing).
    template<typename _Void,
             typename _Callable,
             typename _Payload>
    struct handler_nothrow_helper
    {
        static constexpr bool value = false;
    };

    // handler_nothrow_helper (success specialization)
    //   trait: evaluates noexcept for the payload invocation.
    template<typename _Callable,
             typename... _Args>
    struct handler_nothrow_helper<
        typename std::enable_if<
            noexcept(
                std::declval<_Callable&>()(std::declval<_Args>()...))
        >::type,
        _Callable,
        std::tuple<_Args...>>
    {
        static constexpr bool value = true;
    };

NS_END  // internal


// =========================================================================
// IV.  HANDLER TRAITS
// =========================================================================

// handler_traits
//   trait: compile-time introspection for a callable's suitability as a
// handler of a given event type. A compatible handler is invocable with the
// event's payload value domains and returns either void (always-pass) or a
// verdict.
// requires: _Event must satisfy event_traits requirements.
//
// provides:
//   is_invocable     - true if _Callable(payload...) is well-formed
//   is_compatible    - is_invocable and the result is void or verdict
//   is_nothrow       - true if the invocation is noexcept
//   expected_arity   - number of payload value domains of the event
//   return_type      - the callable's return type (void if not invocable)
//   returns_void     - true if the callable returns void (always-pass)
//   returns_verdict  - true if the callable returns a verdict
template<typename _Callable,
         typename _Event>
struct handler_traits
{
private:
    using event_t    = event_traits<clean_t<_Event>>;
    using payload_t  = typename event_t::payload_type;
    using callable_t = clean_t<_Callable>;
    using invoke_t   =
        internal::handler_invoke_result<void, callable_t, payload_t>;

public:
    // expected_arity
    //   constant: number of payload value domains the handler must accept.
    static constexpr std::size_t expected_arity = event_t::arity;

    // is_invocable
    //   constant: true if _Callable can be invoked with the payload's
    // value domains.
    static constexpr bool is_invocable = invoke_t::value;

    // return_type
    //   type: the return type of the payload invocation (void if the
    // callable is not invocable with the payload).
    using return_type = typename invoke_t::type;

    // returns_void
    //   constant: true if the callable returns void. Only meaningful when
    // is_compatible is true; a void return denotes an always-pass handler.
    static constexpr bool returns_void =
        std::is_void<return_type>::value;

    // returns_verdict
    //   constant: true if the callable returns a verdict.
    static constexpr bool returns_verdict =
        std::is_same<return_type, verdict>::value;

    // is_compatible
    //   constant: true if _Callable is invocable with the payload and its
    // result is interpretable as a verdict (void or verdict).
    static constexpr bool is_compatible =
        ( is_invocable &&
          ( returns_void || returns_verdict ) );

    // is_nothrow
    //   constant: true if the handler invocation is noexcept. Only
    // meaningful when is_compatible is true.
    static constexpr bool is_nothrow =
        internal::handler_nothrow_helper<void, callable_t, payload_t>::value;
};


// =========================================================================
// V.   THE HANDLER MONOID (seq, skip)
// =========================================================================
// (H_e, seq, skip) is a monoid: seq is associative, skip is a two-sided
// unit, and any unconditionally consuming handler is a left zero. The
// registry's dispatch (registry.hpp) is the fold of seq over the effective
// word; these combinators make the monoid first-class for composition and
// for testing its laws.

// skip_t
//   struct: the unit handler. Ignores the payload and always yields
// verdict::pass. Folding a handler word is invariant under inserting or
// removing skip -- the algebraic reason masking a handler is well-defined.
struct skip_t
{
    template<typename... _Args>
    verdict operator()(_Args&&...) const
    {
        return verdict::pass;
    };
};

// skip
//   function: returns the monoid unit skip_t.
inline skip_t skip()
{
    return skip_t{};
}

NS_INTERNAL

    // seq_handler
    //   struct: the sequenced composition of two handlers. Runs the first;
    // if it consumes, the second is skipped and consume is returned;
    // otherwise the second runs and its verdict is returned. Arguments are
    // passed as lvalues to both stages (never moved) so the shared payload
    // survives the first invocation.
    template<typename _H1,
             typename _H2>
    struct seq_handler
    {
        _H1 first;
        _H2 second;

        template<typename... _Args>
        verdict operator()(_Args&&... _args)
        {
            verdict v = invoke_normalized(first, _args...);

            // left zero: consume cuts off the remainder of the word
            if (consumed(v))
            {
                return v;
            }

            return invoke_normalized(second, _args...);
        };
    };

NS_END  // internal

// seq
//   function: sequences two handlers into one (h1 ; h2), realizing the
// monoid operation. With skip as unit and consume as left zero, repeated
// seq folds a whole handler word into a single handler.
template<typename _H1,
         typename _H2>
internal::seq_handler<clean_t<_H1>, clean_t<_H2>>
seq(_H1&& _h1,
    _H2&& _h2)
{
    return internal::seq_handler<clean_t<_H1>, clean_t<_H2>>{
        clean_t<_H1>(std::forward<_H1>(_h1)),
        clean_t<_H2>(std::forward<_H2>(_h2))};
}


// =========================================================================
// VI.  CONCEPT CONSTRAINTS (C++20+)
// =========================================================================

#if D_ENV_CPP_FEATURE_LANG_CONCEPTS

// ---- core handler concepts ----

// is_handler
//   concept: constrains callables that can serve as a handler for a given
// event type (invocable with the payload, returning void or verdict).
template<typename _Callable,
         typename _Event>
concept is_handler =
    is_event<clean_t<_Event>> &&
    handler_traits<clean_t<_Callable>, clean_t<_Event>>::is_compatible;

// is_nothrow_handler
//   concept: constrains noexcept-compatible handlers for an event type.
template<typename _Callable,
         typename _Event>
concept is_nothrow_handler =
    is_handler<_Callable, _Event> &&
    handler_traits<clean_t<_Callable>, clean_t<_Event>>::is_nothrow;

// handler_for
//   concept: readable spelling of is_handler.
template<typename _Callable,
         typename _Event>
concept handler_for =
    is_handler<_Callable, _Event>;

// nothrow_handler_for
//   concept: readable spelling of is_nothrow_handler.
template<typename _Callable,
         typename _Event>
concept nothrow_handler_for =
    is_nothrow_handler<_Callable, _Event>;

// throwing_handler_for
//   concept: constrains compatible handlers whose invocation is not
// statically known to be noexcept.
template<typename _Callable,
         typename _Event>
concept throwing_handler_for =
    handler_for<_Callable, _Event> &&
    !handler_traits<clean_t<_Callable>, clean_t<_Event>>::is_nothrow;


// ---- return-type handler concepts ----

// void_handler_for
//   concept: constrains handlers returning void (always-pass handlers).
template<typename _Callable,
         typename _Event>
concept void_handler_for =
    handler_for<_Callable, _Event> &&
    handler_traits<clean_t<_Callable>, clean_t<_Event>>::returns_void;

// verdict_handler_for
//   concept: constrains handlers returning an explicit verdict.
template<typename _Callable,
         typename _Event>
concept verdict_handler_for =
    handler_for<_Callable, _Event> &&
    handler_traits<clean_t<_Callable>, clean_t<_Event>>::returns_verdict;


// ---- event-arity handler concepts ----

// handler_for_event_of_arity
//   concept: constrains handlers for an event carrying exactly _Arity
// payload value domains.
template<typename _Callable,
         typename _Event,
         std::size_t _Arity>
concept handler_for_event_of_arity =
    handler_for<_Callable, _Event> &&
    (handler_traits<clean_t<_Callable>, clean_t<_Event>>::expected_arity ==
     _Arity);

// nullary_handler_for
//   concept: constrains handlers for empty events.
template<typename _Callable,
         typename _Event>
concept nullary_handler_for =
    handler_for_event_of_arity<_Callable, _Event, 0>;

// unary_handler_for
//   concept: constrains handlers for unary events.
template<typename _Callable,
         typename _Event>
concept unary_handler_for =
    handler_for_event_of_arity<_Callable, _Event, 1>;

// binary_handler_for
//   concept: constrains handlers for binary events.
template<typename _Callable,
         typename _Event>
concept binary_handler_for =
    handler_for_event_of_arity<_Callable, _Event, 2>;

// ternary_handler_for
//   concept: constrains handlers for ternary events.
template<typename _Callable,
         typename _Event>
concept ternary_handler_for =
    handler_for_event_of_arity<_Callable, _Event, 3>;

// variadic_handler_for
//   concept: constrains handlers for events carrying four or more payload
// value domains.
template<typename _Callable,
         typename _Event>
concept variadic_handler_for =
    handler_for<_Callable, _Event> &&
    (handler_traits<clean_t<_Callable>, clean_t<_Event>>::expected_arity > 3);

#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS


NS_END  // djinterp


#endif  // DJINTERP_EVENT_HANDLER_PRIMITIVE_
