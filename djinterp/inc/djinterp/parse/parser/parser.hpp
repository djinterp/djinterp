/*******************************************************************************
* djinterp [parse]                                                    parser.hpp
*
* The parser carrier: a parsing function P A, presented as a CRTP expression.
*   Per ch-parsing.tex the parser carrier is fixed as
*
*       P A = Σ* → maybe⟨A × Σ*⟩          (or result⟨A × Σ*, E⟩)
*
* and it carries four protocols of the functional companion without
* further invention: Functor, Applicative, Alternative (left-biased PEG
* ordered choice), and Monad.
*
*   STATIC PRESENTATION.  parser_expr<Derived> is a CRTP base.  Every
* parser — every leaf (succeed, any, literal, satisfy, eof, ...) and
* every combinator (alt, seq, many, optional, sep_by, ...) — inherits
* from parser_expr<itself> and supplies a parse_impl(state&) member.
* parser_expr provides parse() and operator() that delegate to the
* derived implementation via static_cast.  Composition is fully
* visible to the compiler, fully inlinable, zero virtual dispatch.
*
*   ERASED PRESENTATION.  parser<R, E> is the value-semantic, type-
* erasing handle.  It also inherits from parser_expr<parser<R, E>>,
* so it slots into static composition; under the hood it owns a
* std::function with the parsing signature.  Any parser_expr-derived
* value is implicitly convertible to the handle (one std::function
* construction at the boundary); the handle is the canonical type
* for storage, recursion, late binding, and protocol participation.
*
*   PROTOCOLS.  The four protocol specialisations live on parser<R, E>
* (the handle), at djinterp:: scope.  Static CRTP expressions can use
* the named free-function combinators in combinators.hpp directly;
* protocol entry points (monad_bind, alternative_choice, the pipeline
* combinators) are reached through the handle.
*
* CONTENTS
*   I.    parser_expr<Derived>           CRTP base
*   II.   is_parser  /  parser concept    structural and concept surface
*   III.  parser_input_type /             SFINAE-safe member-type
*         parser_result_type              extractors
*   IV.   is_text_parser /                input-domain classification
*         is_binary_parser
*   V.    parsers_compatible /            composability over a shared
*         parser_state_type               alphabet
*   VI.   parser<R, E>                    type-erasing handle
*   VII.  monad_traits<parser<R, E>>      Monad instance
*   VIII. applicative_traits<...>         Applicative
*   IX.   alternative_traits<...>         Alternative — PEG ordered choice
*   X.    operator|  /  pipe              syntactic sugar over the protocols
*
*
* path:      /inc/djinterp/parse/parser/parser.hpp
* link(s):   ch-parsing.tex
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.06.29
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef DJINTERP_PARSE_PARSER_PARSER_HPP
#define DJINTERP_PARSE_PARSER_PARSER_HPP 1

// FLOOR, FOR NOW: below C++11 this file is empty, rather than an error (README
// rule 5). The owner's ruling: compile at every level first; port to C++98
// only where something needs it.
#include "../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <cstddef>
#include <functional>
#include <type_traits>
#include <utility>
// djinterp
#include "../../djinterp.hpp"
#include "../../core/meta/type_utility.hpp"  // clean_t
#include "../../core/meta/member_traits.hpp"
#include "../../core/functional/functor.hpp"
#include "../../core/functional/applicative.hpp"
#include "../../core/functional/alternative.hpp"
#include "../../core/functional/monad.hpp"
#include "../parse.hpp"


NS_DJINTERP
NS_PARSE


// ================================================================
//  I.   parser_expr<Derived>
// ================================================================

// parser_expr
//   class: CRTP base for every parser.  A conforming derived type
// Derived must supply
//
//     using input_type   = ...                       (Σ element)
//     using result_type  = ...                       (A produced)
//     output_type parse_impl(state_type&) const      (the function)
//
// where output_type defaults to parse_result<result_type> and
// state_type to parse_state<input_type> unless Derived overrides
// either alias.  The CRTP base exposes parse() and operator() that
// forward to parse_impl via static_cast — no virtual dispatch, full
// inlinability through the entire composition tree.
//
//   The parser IS its application: parse() and operator() are the
// same call; both names are kept so call sites can read either as
// the formal notation does (f(s)) or as the parser literature does
// (p.parse(s)).
template<typename Derived>
class parser_expr
{
public:
    using derived_type = Derived;

    // derived
    //   method: static_cast-down to the concrete parser.  Used by
    // combinators that need to read members the base doesn't see.
    D_NODISCARD
    const Derived&
    derived() const D_NOEXCEPT
    {
        return static_cast<const Derived&>(*this);
    }

    D_NODISCARD
    Derived&
    derived() D_NOEXCEPT
    {
        return static_cast<Derived&>(*this);
    }

    // parse
    //   method: runs the parser against _state, returning the
    // derived's output.  Delegated to Derived::parse_impl via the
    // CRTP downcast.  The return type names Self, which defaults to
    // Derived, so that parse_impl is looked up only when parse is used:
    // here, as the base of Derived, Derived is still incomplete.
    template<typename State,
             typename Self = Derived>
    D_NODISCARD
    auto parse(
        State& _state
    ) const
    -> decltype(std::declval<const Self&>().parse_impl(_state))
    {
        return derived().parse_impl(_state);
    }

    // operator()
    //   method: alias for parse().  The parser IS a function.
    template<typename State,
             typename Self = Derived>
    D_NODISCARD
    auto operator()(
        State& _state
    ) const
    -> decltype(std::declval<const Self&>().parse_impl(_state))
    {
        return derived().parse_impl(_state);
    }

protected:
    parser_expr() = default;
    ~parser_expr() = default;
    parser_expr(const parser_expr&) = default;
    parser_expr(parser_expr&&) = default;
    parser_expr& operator=(const parser_expr&) = default;
    parser_expr& operator=(parser_expr&&) = default;
};


// ================================================================
//  II.  is_parser  /  parser concept
// ================================================================

// is_parser
//   trait: structural check.  A type T is a parser iff it derives
// from parser_expr<T> (CRTP) — that contract requires T to expose
// input_type, result_type, and parse_impl as a side condition of
// the inheritance, since the base's parse() instantiation reads
// them.
template<typename T>
struct is_parser
    : std::is_base_of<
          parser_expr<typename std::decay<T>::type>,
          typename std::decay<T>::type>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename T>
    static constexpr bool is_parser_v = is_parser<T>::value;
#endif


#if D_ENV_CPP_FEATURE_LANG_CONCEPTS

    // parser_concept
    //   concept: structurally conforming parser.
    template<typename T>
    concept parser_concept = is_parser<T>::value;

    // text_parser_concept
    //   concept: a parser whose input_type is char.
    template<typename T>
    concept text_parser_concept =
        ( parser_concept<T> &&
          std::is_same<
              typename std::decay<T>::type::input_type,
              char>::value );

    // binary_parser_concept
    //   concept: a parser whose input_type is unsigned char.
    template<typename T>
    concept binary_parser_concept =
        ( parser_concept<T> &&
          std::is_same<
              typename std::decay<T>::type::input_type,
              unsigned char>::value );

#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS


// ================================================================
//  III. SFINAE-safe member-type extractors
// ================================================================

// parser_input_type / parser_input_type_t
//   trait/type: SFINAE-safe extraction of a parser's input_type;
// yields `void` when absent.
D_DEFINE_MEMBER_TYPE_OR(parser_input_type, input_type, void)

// parser_result_type / parser_result_type_t
//   trait/type: SFINAE-safe extraction of a parser's result_type.
D_DEFINE_MEMBER_TYPE_OR(parser_result_type, result_type, void)


// ================================================================
//  IV.  is_text_parser  /  is_binary_parser
// ================================================================

NS_INTERNAL

    // is_text_parser_helper
    //   trait: primary template (failure case).
    template<typename T,
             bool     IsParser = is_parser<T>::value,
             typename           = void>
    struct is_text_parser_helper : std::false_type
    {};

    // is_text_parser_helper (success case)
    //   trait: a parser whose input_type is char.
    template<typename T>
    struct is_text_parser_helper<
        T,
        true,
        typename std::enable_if<
            std::is_same<typename clean_t<T>::input_type,
                         char>::value>::type
    > : std::true_type
    {};

    // is_binary_parser_helper
    //   trait: primary template (failure case).
    template<typename T,
             bool     IsParser = is_parser<T>::value,
             typename           = void>
    struct is_binary_parser_helper : std::false_type
    {};

    // is_binary_parser_helper (success case)
    //   trait: a parser whose input_type is unsigned char.
    template<typename T>
    struct is_binary_parser_helper<
        T,
        true,
        typename std::enable_if<
            std::is_same<typename clean_t<T>::input_type,
                         unsigned char>::value>::type
    > : std::true_type
    {};

NS_END  // internal

// is_text_parser
//   trait: a structurally conforming parser whose input_type is char.
template<typename T>
struct is_text_parser : internal::is_text_parser_helper<T>
{};

// is_binary_parser
//   trait: a structurally conforming parser whose input_type is
// unsigned char.
template<typename T>
struct is_binary_parser : internal::is_binary_parser_helper<T>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename T>
    static constexpr bool is_text_parser_v =
        is_text_parser<T>::value;

    template<typename T>
    static constexpr bool is_binary_parser_v =
        is_binary_parser<T>::value;
#endif


// ================================================================
//  V.   parsers_compatible
// ================================================================

NS_INTERNAL

    // parsers_compatible_helper
    //   trait: primary template (failure case).
    template<typename A,
             typename B,
             bool     BothParsers = ( is_parser<A>::value &&
                                       is_parser<B>::value ),
             typename = void>
    struct parsers_compatible_helper : std::false_type
    {};

    // parsers_compatible_helper (success case)
    //   trait: both are parsers sharing input_type.
    template<typename A,
             typename B>
    struct parsers_compatible_helper<
        A,
        B,
        true,
        typename std::enable_if<
            std::is_same<
                typename clean_t<A>::input_type,
                typename clean_t<B>::input_type>::value>::type
    > : std::true_type
    {};

NS_END  // internal

// parsers_compatible
//   trait: two parsers share input_type and are therefore composable.
template<typename A,
         typename B>
struct parsers_compatible
    : internal::parsers_compatible_helper<A, B>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename A,
             typename B>
    static constexpr bool parsers_compatible_v =
        parsers_compatible<A, B>::value;
#endif


// ================================================================
//  VI.  parser<R, E>  —  the type-erasing handle
// ================================================================

// parser
//   class: the value-semantic handle.  Holds any parser_expr-derived
// value via std::function, presenting a uniform type for storage,
// recursion, return values, and protocol participation.  Itself a
// parser_expr (CRTP self-referent), so it composes statically with
// other parser_exprs as well — at the cost of the std::function
// indirection per call.
//
//   The single non-default constructor is templated on any callable
// matching the parsing-function signature.  Since every parser_expr
// is callable (operator() is inherited from the base), passing one
// in just works — the parser_expr is copied into the std::function's
// storage and its operator() is the per-call entry point.  A SFINAE
// guard excludes `parser` itself so copy/move take their respective
// special members.
//
//   Result    A   — the value produced on success.
//   Element   Σ   — the surface stream element type; char by default.
template<typename Result,
         typename Element = char>
class parser : public parser_expr<parser<Result, Element>>
{
public:
    using input_type    = Element;
    using element_type  = Element;
    using result_type   = Result;
    using value_type    = Result;
    using state_type    = parse_state<Element>;
    using output_type   = parse_result<Result>;
    using function_type =
        std::function<output_type(state_type&)>;

    parser()
        : m_fn()
    {}

    // parser (from any matching callable)
    //   constructor: wraps a callable — including any parser_expr —
    // into the type-erasing handle.  SFINAE excludes `parser` itself
    // so the copy and move constructors aren't shadowed.
    template<typename Fn,
             typename = typename std::enable_if<
                 ( !std::is_same<
                       typename std::decay<Fn>::type,
                       parser>::value )                       &&
                 ( std::is_constructible<
                       function_type, Fn>::value )>::type>
    parser(
        Fn _fn
    )
        : m_fn(static_cast<Fn&&>(_fn))
    {}

    parser(const parser&) = default;
    parser(parser&&)      = default;
    parser& operator=(const parser&) = default;
    parser& operator=(parser&&)      = default;
    ~parser() = default;


    // parse_impl
    //   method: the CRTP hook the parser_expr base dispatches to.
    // An uninitialised handle (constructed by `parser()`) yields an
    // error — distinct from a holder of an always-failing parser.
    output_type
    parse_impl(
        state_type& _state
    ) const
    {
        if (!m_fn)
        {
            return output_type::make_error(
                DParseStatusFailure,
                _state.offset,
                "parser: uninitialised");
        }

        return m_fn(_state);
    }

    // ok
    //   method: true iff this handle holds a function.
    D_NODISCARD
    bool
    ok() const D_NOEXCEPT
    {
        return static_cast<bool>(m_fn);
    }

    // explicit operator bool
    //   method: same as ok(), for `if (p)` idioms.
    D_NODISCARD
    explicit
    operator bool() const D_NOEXCEPT
    {
        return static_cast<bool>(m_fn);
    }

private:
    function_type m_fn;
};


NS_END  // parse


// ================================================================
//  VII.  monad_traits<parser<R, E>>
//  VIII. applicative_traits<parser<R, E>>
//  IX.   alternative_traits<parser<R, E>>
// ================================================================
//   The four protocol specialisations live at djinterp:: scope —
// the same namespace as the primary templates — so the parse::
// namespace closes above and reopens below.  These instances act on
// the erased handle; CRTP expressions reach the protocols either by
// implicit erasure or via the named free-function combinators in
// combinators.hpp.

// monad_traits<parser<R, E>>
//   specialisation: parser is a Monad.  unit lifts a value into a
// parser that succeeds without consuming input; bind runs the first
// parser, threads its result through f, and runs the resulting
// parser at the advanced state.  An error from the first short-
// circuits.
template<typename Result,
         typename Element>
struct monad_traits<parse::parser<Result, Element>>
{
    using is_specialized = std::true_type;
    using value_type     = Result;

    template<typename U>
    using rebind = parse::parser<U, Element>;

    // unit
    //   lifts a value into the parser monad.
    static
    parse::parser<Result, Element>
    unit(
        Result _value
    )
    {
        using state_type  = parse::parse_state<Element>;
        using output_type = parse::parse_result<Result>;

        return parse::parser<Result, Element>(
            [_value](state_type& /*_state*/) -> output_type
            {
                return output_type(_value);
            });
    }

    // bind
    //   monadic bind.  Runs _p; on success, applies _f to the
    // produced value (which must yield a parser) and runs that at
    // the state _p left behind; on failure, propagates the error.
    template<typename Function>
    static
    auto bind(
        const parse::parser<Result, Element>& _p,
        Function                                _f
    )
    -> typename std::decay<decltype(
        _f(std::declval<const Result&>()))>::type
    {
        using next_parser_t =
            typename std::decay<decltype(
                _f(std::declval<const Result&>()))>::type;
        using next_result_t = typename next_parser_t::result_type;
        using state_type    = parse::parse_state<Element>;
        using output_type   = parse::parse_result<next_result_t>;

        return next_parser_t(
            [_p, _f](state_type& _state) -> output_type
            {
                parse::parse_result<Result> r = _p.parse(_state);

                if (!r.ok())
                {
                    return output_type(r.error());
                }

                next_parser_t next = _f(r.value());
                return next.parse(_state);
            });
    }
};


// applicative_traits<parser<R, E>>
//   specialisation: parser is an Applicative.  pure is the monad's
// unit; ap runs the function-producing parser, then the value-
// producing parser, and applies the former to the latter — both run
// in sequence, threading the residual via the state.
template<typename Result,
         typename Element>
struct applicative_traits<parse::parser<Result, Element>>
{
    using is_specialized = std::true_type;
    using value_type     = Result;

    template<typename U>
    using rebind = parse::parser<U, Element>;

    // pure
    //   lifts a value into the parser applicative.  Equivalent to
    // monad_traits::unit.
    static
    parse::parser<Result, Element>
    pure(
        Result _value
    )
    {
        return monad_traits<
                   parse::parser<Result, Element>
               >::unit(static_cast<Result&&>(_value));
    }

    // ap
    //   applicative apply.  Runs the function-parser, then the
    // value-parser, and combines them.  Either failure short-
    // circuits to the error.
    template<typename Pf>
    static
    auto ap(
        const Pf&                              _pf,
        const parse::parser<Result, Element>& _pa
    )
    -> parse::parser<
           typename std::decay<decltype(
               std::declval<typename Pf::result_type>()(
                   std::declval<Result>()))>::type,
           Element>
    {
        using fn_type     = typename Pf::result_type;
        using out_type    =
            typename std::decay<decltype(
                std::declval<fn_type>()(
                    std::declval<Result>()))>::type;
        using state_type  = parse::parse_state<Element>;
        using output_type = parse::parse_result<out_type>;

        return parse::parser<out_type, Element>(
            [_pf, _pa](state_type& _state) -> output_type
            {
                parse::parse_result<fn_type> rf = _pf.parse(_state);

                if (!rf.ok())
                {
                    return output_type(rf.error());
                }

                parse::parse_result<Result> ra = _pa.parse(_state);

                if (!ra.ok())
                {
                    return output_type(ra.error());
                }

                return output_type(rf.value()(ra.value()));
            });
    }
};


// alternative_traits<parser<R, E>>
//   specialisation: parser is an Alternative.  empty() always fails;
// choice(p, q) is PEG ordered choice — try p first, commit on
// success, otherwise restore the offset and try q.  Left-biased and
// leftmost-wins, exactly as ch-parsing prescribes.
template<typename Result,
         typename Element>
struct alternative_traits<parse::parser<Result, Element>>
{
    using is_specialized = std::true_type;
    using value_type     = Result;

    // empty
    //   the failure / identity for choice.
    static
    parse::parser<Result, Element>
    empty()
    {
        using state_type  = parse::parse_state<Element>;
        using output_type = parse::parse_result<Result>;

        return parse::parser<Result, Element>(
            [](state_type& _state) -> output_type
            {
                return output_type::make_error(
                    parse::DParseStatusFailure,
                    _state.offset,
                    "alternative::empty");
            });
    }

    // choice
    //   PEG ordered choice — try _a, restore the offset and try _b
    // on failure.
    static
    parse::parser<Result, Element>
    choice(
        const parse::parser<Result, Element>& _a,
        const parse::parser<Result, Element>& _b
    )
    {
        using state_type  = parse::parse_state<Element>;
        using output_type = parse::parse_result<Result>;

        return parse::parser<Result, Element>(
            [_a, _b](state_type& _state) -> output_type
            {
                std::size_t saved = _state.offset;

                output_type r = _a.parse(_state);

                if (r.ok())
                {
                    return r;
                }

                _state.offset = saved;

                return _b.parse(_state);
            });
    }
};



NS_END  // djinterp

#endif  // floor, for now


#endif  // DJINTERP_PARSE_PARSER_PARSER_HPP
