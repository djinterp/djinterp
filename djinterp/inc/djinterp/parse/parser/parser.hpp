/******************************************************************************
* djinterp [parse]                                            parser/parser.hpp
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
*   STATIC PRESENTATION.  parser_expr<_Derived> is a CRTP base.  Every
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
*   I.    parser_expr<_Derived>           CRTP base
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
* path:      /inc/djinterp/parse/parser/parser.hpp
* link(s):   ch-parsing.tex
* author(s): Samuel 'teer' Neal-Blim                          date: 2026.06.29
******************************************************************************/

#ifndef DJINTERP_PARSE_PARSER_
#define DJINTERP_PARSE_PARSER_ 1

// std
#include <cstddef>
#include <functional>
#include <type_traits>
#include <utility>
// djinterp
#include "../../core/djinterp.hpp"
#include "../../core/meta/member_traits.hpp"
#include "../../core/functional/functor.hpp"
#include "../../core/functional/applicative.hpp"
#include "../../core/functional/alternative.hpp"
#include "../../core/functional/monad.hpp"
#include "../parse.hpp"


NS_DJINTERP
NS_PARSE


// ================================================================
//  I.   parser_expr<_Derived>
// ================================================================

// parser_expr
//   class: CRTP base for every parser.  A conforming derived type
// _Derived must supply
//
//     using input_type   = ...                       (Σ element)
//     using result_type  = ...                       (A produced)
//     output_type parse_impl(state_type&) const      (the function)
//
// where output_type defaults to parse_result<result_type> and
// state_type to parse_state<input_type> unless _Derived overrides
// either alias.  The CRTP base exposes parse() and operator() that
// forward to parse_impl via static_cast — no virtual dispatch, full
// inlinability through the entire composition tree.
//
//   The parser IS its application: parse() and operator() are the
// same call; both names are kept so call sites can read either as
// the formal notation does (f(s)) or as the parser literature does
// (p.parse(s)).
template<typename _Derived>
class parser_expr
{
public:
    using derived_type = _Derived;

    // derived
    //   method: static_cast-down to the concrete parser.  Used by
    // combinators that need to read members the base doesn't see.
    D_NODISCARD
    const _Derived&
    derived() const D_NOEXCEPT
    {
        return static_cast<const _Derived&>(*this);
    }

    D_NODISCARD
    _Derived&
    derived() D_NOEXCEPT
    {
        return static_cast<_Derived&>(*this);
    }

    // parse
    //   method: runs the parser against _state, returning the
    // derived's output.  Delegated to _Derived::parse_impl via the
    // CRTP downcast.
    template<typename _State>
    D_NODISCARD
    auto parse(
        _State& _state
    ) const
    -> decltype(std::declval<const _Derived&>().parse_impl(_state))
    {
        return derived().parse_impl(_state);
    }

    // operator()
    //   method: alias for parse().  The parser IS a function.
    template<typename _State>
    D_NODISCARD
    auto operator()(
        _State& _state
    ) const
    -> decltype(std::declval<const _Derived&>().parse_impl(_state))
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
//   trait: structural check.  A type _T is a parser iff it derives
// from parser_expr<_T> (CRTP) — that contract requires _T to expose
// input_type, result_type, and parse_impl as a side condition of
// the inheritance, since the base's parse() instantiation reads
// them.
template<typename _T>
struct is_parser
    : std::is_base_of<
          parser_expr<typename std::decay<_T>::type>,
          typename std::decay<_T>::type>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _T>
    static constexpr bool is_parser_v = is_parser<_T>::value;
#endif


#if D_ENV_CPP_FEATURE_LANG_CONCEPTS

    // parser_concept
    //   concept: structurally conforming parser.
    template<typename _T>
    concept parser_concept = is_parser<_T>::value;

    // text_parser_concept
    //   concept: a parser whose input_type is char.
    template<typename _T>
    concept text_parser_concept =
        ( parser_concept<_T> &&
          std::is_same<
              typename std::decay<_T>::type::input_type,
              char>::value );

    // binary_parser_concept
    //   concept: a parser whose input_type is unsigned char.
    template<typename _T>
    concept binary_parser_concept =
        ( parser_concept<_T> &&
          std::is_same<
              typename std::decay<_T>::type::input_type,
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
    template<typename _T,
             bool     _IsParser = is_parser<_T>::value,
             typename           = void>
    struct is_text_parser_helper : std::false_type
    {};

    // is_text_parser_helper (success case)
    //   trait: a parser whose input_type is char.
    template<typename _T>
    struct is_text_parser_helper<
        _T,
        true,
        typename std::enable_if<
            std::is_same<typename clean_t<_T>::input_type,
                         char>::value>::type
    > : std::true_type
    {};

    // is_binary_parser_helper
    //   trait: primary template (failure case).
    template<typename _T,
             bool     _IsParser = is_parser<_T>::value,
             typename           = void>
    struct is_binary_parser_helper : std::false_type
    {};

    // is_binary_parser_helper (success case)
    //   trait: a parser whose input_type is unsigned char.
    template<typename _T>
    struct is_binary_parser_helper<
        _T,
        true,
        typename std::enable_if<
            std::is_same<typename clean_t<_T>::input_type,
                         unsigned char>::value>::type
    > : std::true_type
    {};

NS_END  // internal

// is_text_parser
//   trait: a structurally conforming parser whose input_type is char.
template<typename _T>
struct is_text_parser : internal::is_text_parser_helper<_T>
{};

// is_binary_parser
//   trait: a structurally conforming parser whose input_type is
// unsigned char.
template<typename _T>
struct is_binary_parser : internal::is_binary_parser_helper<_T>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _T>
    static constexpr bool is_text_parser_v =
        is_text_parser<_T>::value;

    template<typename _T>
    static constexpr bool is_binary_parser_v =
        is_binary_parser<_T>::value;
#endif


// ================================================================
//  V.   parsers_compatible
// ================================================================

NS_INTERNAL

    // parsers_compatible_helper
    //   trait: primary template (failure case).
    template<typename _A,
             typename _B,
             bool     _BothParsers = ( is_parser<_A>::value &&
                                       is_parser<_B>::value ),
             typename = void>
    struct parsers_compatible_helper : std::false_type
    {};

    // parsers_compatible_helper (success case)
    //   trait: both are parsers sharing input_type.
    template<typename _A,
             typename _B>
    struct parsers_compatible_helper<
        _A,
        _B,
        true,
        typename std::enable_if<
            std::is_same<
                typename clean_t<_A>::input_type,
                typename clean_t<_B>::input_type>::value>::type
    > : std::true_type
    {};

NS_END  // internal

// parsers_compatible
//   trait: two parsers share input_type and are therefore composable.
template<typename _A,
         typename _B>
struct parsers_compatible
    : internal::parsers_compatible_helper<_A, _B>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _A,
             typename _B>
    static constexpr bool parsers_compatible_v =
        parsers_compatible<_A, _B>::value;
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
//   _Result   A   — the value produced on success.
//   _Element  Σ   — the surface stream element type; char by default.
template<typename _Result,
         typename _Element = char>
class parser : public parser_expr<parser<_Result, _Element>>
{
public:
    using input_type    = _Element;
    using element_type  = _Element;
    using result_type   = _Result;
    using value_type    = _Result;
    using state_type    = parse_state<_Element>;
    using output_type   = parse_result<_Result>;
    using function_type =
        std::function<output_type(state_type&)>;

    parser()
        : m_fn()
    {}

    // parser (from any matching callable)
    //   constructor: wraps a callable — including any parser_expr —
    // into the type-erasing handle.  SFINAE excludes `parser` itself
    // so the copy and move constructors aren't shadowed.
    template<typename _Fn,
             typename = typename std::enable_if<
                 ( !std::is_same<
                       typename std::decay<_Fn>::type,
                       parser>::value )                       &&
                 ( std::is_constructible<
                       function_type, _Fn>::value )>::type>
    parser(
        _Fn _fn
    )
        : m_fn(static_cast<_Fn&&>(_fn))
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
template<typename _Result,
         typename _Element>
struct monad_traits<parse::parser<_Result, _Element>>
{
    using is_specialized = std::true_type;
    using value_type     = _Result;

    template<typename _U>
    using rebind = parse::parser<_U, _Element>;

    // unit
    //   lifts a value into the parser monad.
    static
    parse::parser<_Result, _Element>
    unit(
        _Result _value
    )
    {
        using state_type  = parse::parse_state<_Element>;
        using output_type = parse::parse_result<_Result>;

        return parse::parser<_Result, _Element>(
            [_value](state_type& /*_state*/) -> output_type
            {
                return output_type(_value);
            });
    }

    // bind
    //   monadic bind.  Runs _p; on success, applies _f to the
    // produced value (which must yield a parser) and runs that at
    // the state _p left behind; on failure, propagates the error.
    template<typename _Function>
    static
    auto bind(
        const parse::parser<_Result, _Element>& _p,
        _Function                               _f
    )
    -> typename std::decay<decltype(
        _f(std::declval<const _Result&>()))>::type
    {
        using next_parser_t =
            typename std::decay<decltype(
                _f(std::declval<const _Result&>()))>::type;
        using next_result_t = typename next_parser_t::result_type;
        using state_type    = parse::parse_state<_Element>;
        using output_type   = parse::parse_result<next_result_t>;

        return next_parser_t(
            [_p, _f](state_type& _state) -> output_type
            {
                parse::parse_result<_Result> r = _p.parse(_state);

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
template<typename _Result,
         typename _Element>
struct applicative_traits<parse::parser<_Result, _Element>>
{
    using is_specialized = std::true_type;
    using value_type     = _Result;

    template<typename _U>
    using rebind = parse::parser<_U, _Element>;

    // pure
    //   lifts a value into the parser applicative.  Equivalent to
    // monad_traits::unit.
    static
    parse::parser<_Result, _Element>
    pure(
        _Result _value
    )
    {
        return monad_traits<
                   parse::parser<_Result, _Element>
               >::unit(static_cast<_Result&&>(_value));
    }

    // ap
    //   applicative apply.  Runs the function-parser, then the
    // value-parser, and combines them.  Either failure short-
    // circuits to the error.
    template<typename _Pf>
    static
    auto ap(
        const _Pf&                              _pf,
        const parse::parser<_Result, _Element>& _pa
    )
    -> parse::parser<
           typename std::decay<decltype(
               std::declval<typename _Pf::result_type>()(
                   std::declval<_Result>()))>::type,
           _Element>
    {
        using fn_type     = typename _Pf::result_type;
        using out_type    =
            typename std::decay<decltype(
                std::declval<fn_type>()(
                    std::declval<_Result>()))>::type;
        using state_type  = parse::parse_state<_Element>;
        using output_type = parse::parse_result<out_type>;

        return parse::parser<out_type, _Element>(
            [_pf, _pa](state_type& _state) -> output_type
            {
                parse::parse_result<fn_type> rf = _pf.parse(_state);

                if (!rf.ok())
                {
                    return output_type(rf.error());
                }

                parse::parse_result<_Result> ra = _pa.parse(_state);

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
template<typename _Result,
         typename _Element>
struct alternative_traits<parse::parser<_Result, _Element>>
{
    using is_specialized = std::true_type;
    using value_type     = _Result;

    // empty
    //   the failure / identity for choice.
    static
    parse::parser<_Result, _Element>
    empty()
    {
        using state_type  = parse::parse_state<_Element>;
        using output_type = parse::parse_result<_Result>;

        return parse::parser<_Result, _Element>(
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
    parse::parser<_Result, _Element>
    choice(
        const parse::parser<_Result, _Element>& _a,
        const parse::parser<_Result, _Element>& _b
    )
    {
        using state_type  = parse::parse_state<_Element>;
        using output_type = parse::parse_result<_Result>;

        return parse::parser<_Result, _Element>(
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

// functor_traits<parser<R, E>>
//   Not specialised here: the monad bridge in functor.hpp picks up
// any monad as a functor, so parser<R, E> participates in functor
// automatically via the monad_traits specialisation above.


#endif  // DJINTERP_PARSE_PARSER_