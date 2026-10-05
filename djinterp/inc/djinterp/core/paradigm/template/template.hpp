/*******************************************************************************
* djinterp [core]                                                   template.hpp
*
*   The template-source-sink algebra: a programming-agnostic formalization,
* rendered in C++.  A *system* is a carrier type tau, a sink type sigma, and a
* transformation F : tau x tau -> sigma whose first argument is a *template* t
* and whose second is a *source* alpha; F(t, alpha) = beta is the *sink*.  The
* defining identity is the curry / evaluate factorization
*
*       F  =  ev . (F_hat x id) ,        F(t, alpha) = ev(F_hat(t), alpha) ,
*
* where F_hat(t) = F_t : tau -> sigma is the *source-transformer* that a
* template names, and ev is the universal evaluation map.  `instantiate` is
* F_hat at a point, `evaluate` is ev, and `template_system` packages F so the
* identity holds by construction.
*
*   The pipeline section composes a transformation from stages
* F = f_n . ... . f_1 in which only the first stage is binary (it alone
* consumes the template-source product); every later stage is unary.
* `reader_stages` is the ambient-template variant, threading t to every stage.
*
*   The parser section is the special case sigma = (rho x tau) + E:
* `parse_outcome` is that sink (a result rho paired with the remaining source
* tau, summed with an error E), and `kleisli_then` / `kleisli_bind` sequence
* parsers by threading the remaining source forward and short-circuiting on
* failure -- the note's `Q <> P` and `P >>= k`.
*
*   Stands on the core / meta headers only (djinterp.hpp, trait_detect.hpp,
* type_traits.hpp; concepts.hpp under C++20).  Requires C++14+ (higher-order
* return-type deduction); self-suppresses on C++11, mirroring transducer.hpp.
*
*   Trait-triple convention: each `is_X` carries an `is_X_v` (C++14+) and a
* parallel concept (C++20).  Implementation classes are `internal::*_helper`
* and model a role; the public factories return them.
*
*
* path:      /inc/djinterp/core/paradigm/template/template.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.06.13
*                                                            revised: 2026.10.01
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    TRANSFORMATION TRAITS
      ---------------------
      i.    transformation_sink (sigma deduction) + _t
      ii.   is_transformation (F : tau x tau -> sigma) + _v
      iii.  source_transformer_sink + _t; is_source_transformer + _v

II.   THE ALGEBRA
      -----------
      i.    evaluate                 -- ev : (tau -> sigma) x tau -> sigma
      ii.   transformer_helper       -- F_t = F(t, .) (internal)
      iii.  instantiate              -- F_hat at a point
      iv.   template_system          -- packages (tau, sigma, F)
      v.    make_template_system

III.  STAGE PIPELINES
      ---------------
      i.    unary_chain_helper       -- g_n . ... . g_1 (internal)
      ii.   stage_chain_helper       -- f_n . ... . f_1, f_1 binary (internal)
      iii.  stages                   -- template consumed at stage 1
      iv.   reader_chain_helper      -- ambient template (internal)
      v.    reader_stages            -- the Reader / environment variant

IV.   PARSERS  (sigma = (rho x tau) + E)
      ----------------------------------
      i.    parse_success_tag / parse_failure_tag (internal)
      ii.   parse_outcome            -- the parser sink
      iii.  parse_success / parse_failure
      iv.   is_parse_outcome + _v
      v.    parser_system            -- alias pinning sigma
      vi.   kleisli_then_helper / kleisli_bind_helper (internal)
      vii.  kleisli_then / kleisli_bind

V.    CONCEPTS  (C++20)
      -----------------
*/

#ifndef DJINTERP_PARADIGM_TEMPLATE_TEMPLATE_HPP
#define DJINTERP_PARADIGM_TEMPLATE_TEMPLATE_HPP 1

// FLOOR, FOR NOW: below C++11 this file is empty, rather than an error (README
// rule 5). The owner's ruling: compile at every level first; port to C++98
// only where something needs it.
#include "../../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <cstddef>
#include <type_traits>
#include <utility>
// djinterp
#include "../../../djinterp.hpp"      // NS_*, D_CONSTEXPR, D_NODISCARD, clean_t
#include "../../meta/type_utility.hpp"  // clean_t
#include "../../meta/trait_detect.hpp"   // D_TYPE_TRAIT_VALUE_BOOL, _IS_SPECIALIZATION_OF
#include "../../meta/type_traits.hpp"    // nonesuch, invoke_result_t


#if D_ENV_CPP_FEATURE_LANG_CONCEPTS
    #include "../../meta/concepts.hpp"
#endif


// This module is higher-order: it leans on C++14 return-type deduction.  On a
// strict C++11 toolchain it contributes nothing rather than failing to compile
// (the same self-suppression transducer.hpp uses).
#if D_ENV_LANG_IS_CPP14_OR_HIGHER


NS_DJINTERP


// ===========================================================================
// I.   TRANSFORMATION TRAITS
// ===========================================================================
//   A *source* and a *template* are both inhabitants of the carrier type tau;
// they are roles, not distinct types, so there is nothing type-level to detect
// for them (this is the homogeneity the formalization rests on).  What is
// detectable is the shape of a transformation and of the transformers it
// curries to.

//   The detectors below are *tagless*: each forms its detection expression in
// the return type of an overloaded probe and reads it back with `decltype`.
// The `(int)` overload is preferred when the expression type-checks; on
// substitution failure the `(...)` fallback supplies `nonesuch`.  No
// `typename = void` slot and no void_t-keyed partial specialization -- the same
// structural style the core `is_invocable` uses.  Each `is_X` is then just the
// statement that the corresponding sink is detectable, so the detection
// expression lives in exactly one place.

NS_INTERNAL

    // transformation_sink_probe
    //   trait: tagless detector for `F(const Tau&, const Tau&)`.  The leading
    // overload's return type is the deduced sink; on substitution failure the
    // variadic overload yields `nonesuch`.  Declared, never defined -- used
    // only in unevaluated context.
    //
    //   Fn is probed as `const Fn&`, NOT as an rvalue.  This is what the
    // algebra actually does: `transformer_helper` and `template_system` store F
    // by value and invoke it from const member functions, so a transformation
    // must be const-callable.  Probing the rvalue instead would admit a functor
    // whose operator() is non-const -- `is_transformation` (and the
    // `transformation_for` concept built on it) would report true, and the
    // failure would then surface deep inside the algebra rather than at the
    // constraint.  F is a mathematical map; const-callability is the contract.
    template<typename Fn,
             typename Tau>
    auto transformation_sink_probe(int)
        -> invoke_result_t<const Fn&, const Tau&, const Tau&>;

    template<typename,
             typename>
    auto transformation_sink_probe(...) -> nonesuch;

    // source_transformer_probe
    //   trait: tagless detector for `g(const Tau&)` -- the shape of a
    // source-transformer F_t.  Same overload structure, and the same const-
    // callability contract: the kleisli helpers store parsers by value and
    // invoke them from const member functions, and the canonical F_t
    // (`transformer_helper`) is itself const-callable.
    template<typename Fn,
             typename Tau>
    auto source_transformer_probe(int)
        -> invoke_result_t<const Fn&, const Tau&>;

    template<typename,
             typename>
    auto source_transformer_probe(...) -> nonesuch;

NS_END  // internal


// transformation_sink
//   trait: the sink type sigma produced by a transformation Fn applied to a
// template and a source, both of carrier type Tau (i.e. the result of
// `F(const Tau&, const Tau&)`), or `nonesuch` when Fn is not so callable.
template<typename Fn,
         typename Tau>
struct transformation_sink
{
    using type = decltype(internal::transformation_sink_probe<Fn, Tau>(0));
};

// transformation_sink_t
//   type: convenience alias for transformation_sink<Fn, Tau>::type.
template<typename Fn,
         typename Tau>
using transformation_sink_t = typename transformation_sink<Fn, Tau>::type;


// is_transformation
//   trait: true iff Fn is a transformation over carrier Tau -- callable as
// `F(const Tau&, const Tau&)`, i.e. its sink is detectable.  Parallel
// concept: `transformation_for`.
template<typename Fn,
         typename Tau>
struct is_transformation
    : std::integral_constant<bool,
          !std::is_same<transformation_sink_t<Fn, Tau>, nonesuch>::value>
{};

// is_transformation_v
//   value: variable-template shorthand for is_transformation<Fn, Tau>::value.
// (Two-parameter trait, so the unary D_TYPE_TRAIT_VALUE_BOOL sugar does not
// apply; the standard gating is reproduced here by hand.)
#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    template<typename Fn,
             typename Tau>
    D_CONSTEXPR_INLINE_VAR bool is_transformation_v =
        is_transformation<Fn, Tau>::value;
#elif D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename Fn,
             typename Tau>
    constexpr bool is_transformation_v = is_transformation<Fn, Tau>::value;
#endif


// source_transformer_sink
//   trait: the sink sigma produced by a source-transformer F_t applied to a
// single source of carrier Tau (i.e. the result of `g(const Tau&)`), or
// `nonesuch` when Fn is not so callable.
template<typename Fn,
         typename Tau>
struct source_transformer_sink
{
    using type = decltype(internal::source_transformer_probe<Fn, Tau>(0));
};

// source_transformer_sink_t
//   type: convenience alias for source_transformer_sink<Fn, Tau>::type.
template<typename Fn,
         typename Tau>
using source_transformer_sink_t =
    typename source_transformer_sink<Fn, Tau>::type;


// is_source_transformer
//   trait: true iff Fn is a source-transformer F_t : tau -> sigma over carrier
// Tau -- callable as `g(const Tau&)`, i.e. its sink is detectable.  Parallel
// concept: `source_transformer_for`.
template<typename Fn,
         typename Tau>
struct is_source_transformer
    : std::integral_constant<bool,
          !std::is_same<source_transformer_sink_t<Fn, Tau>, nonesuch>::value>
{};

// is_source_transformer_v
//   value: variable-template shorthand for the trait above.
#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    template<typename Fn,
             typename Tau>
    D_CONSTEXPR_INLINE_VAR bool is_source_transformer_v =
        is_source_transformer<Fn, Tau>::value;
#elif D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename Fn,
             typename Tau>
    constexpr bool is_source_transformer_v =
        is_source_transformer<Fn, Tau>::value;
#endif


// ===========================================================================
// II.  THE ALGEBRA
// ===========================================================================

// evaluate
//   function: the universal evaluation map ev : (tau -> sigma) x tau -> sigma.
// Applies a source-transformer _g (an F_t) to a source _alpha and returns the
// sink.  The whole module exists to make `evaluate(instantiate(fn, t), alpha)`
// equal `fn(t, alpha)` -- the factorization F = ev . (F_hat x id).
template<typename Transformer,
         typename Source>
D_NODISCARD D_CONSTEXPR auto
evaluate(
    Transformer&& _g,
    Source&&      _alpha
)
D_NOEXCEPT_IF(noexcept(static_cast<Transformer&&>(_g)(
                           static_cast<Source&&>(_alpha))))
{
    return static_cast<Transformer&&>(_g)(static_cast<Source&&>(_alpha));
}


NS_INTERNAL

    // transformer_helper
    //   class: the curried transformer F_t = F(t, .) : tau -> sigma -- the image
    // of a template t under F_hat.  Holds the transformation F (decayed) and the
    // bound template; calling it with a source applies F.  Returned by
    // `instantiate` and by `template_system::instantiate`.
    template<typename Fn,
             typename Tau>
    class transformer_helper
    {
    public:
        using carrier_type = Tau;
        using sink_type    = transformation_sink_t<Fn, Tau>;

        // bind a template to a transformation
        D_CONSTEXPR transformer_helper(
            const Fn&  _fn,
            const Tau& _bound_template
        )
            : m_fn(_fn),
              m_template(_bound_template)
        {}

        // apply F_t to a source: F(t, alpha)
        D_NODISCARD D_CONSTEXPR sink_type
        operator()(
            const Tau& _source
        ) const
        {
            return m_fn(m_template, _source);
        }

    private:
        Fn   m_fn;
        Tau m_template;
    };

NS_END  // internal


// instantiate
//   function: F_hat at a point.  Decodes a template _t under a bare
// transformation _fn into its source-transformer F_t : tau -> sigma, so that
// `evaluate(instantiate(fn, t), alpha) == fn(t, alpha)`.  The carrier tau is
// deduced from the template argument.
template<typename Fn,
         typename Tau>
D_NODISCARD D_CONSTEXPR
internal::transformer_helper<clean_t<Fn>, clean_t<Tau>>
instantiate(
    Fn&&  _fn,
    Tau&& _t
)
{
    return internal::transformer_helper<clean_t<Fn>, clean_t<Tau>>(
        static_cast<Fn&&>(_fn),
        static_cast<Tau&&>(_t));
}


// template_system
//   class: a template-source-sink system (tau, sigma, F) realized as a value.
// Holds the transformation F : tau x tau -> sigma.  `apply` (and operator()) is
// the uncurried F; `instantiate` is F_hat (it returns the transformer F_t); and
// the identity
//       apply(t, alpha) == evaluate(instantiate(t), alpha)
// is the factorization F = ev . (F_hat x id), true here by construction.  The
// sink type defaults to the deduced sigma but may be pinned (see parser_system).
template<typename Fn,
         typename Tau,
         typename Sigma = transformation_sink_t<Fn, Tau>>
class template_system
{
public:
    using carrier_type        = Tau;
    using sink_type           = Sigma;
    using transformation_type = Fn;
    using transformer_type    = internal::transformer_helper<Fn, Tau>;

    // wrap a transformation
    D_CONSTEXPR explicit template_system(
        const Fn& _fn
    )
        : m_fn(_fn)
    {}

    // apply -- the uncurried F(t, alpha) = beta
    D_NODISCARD D_CONSTEXPR sink_type
    apply(
        const Tau& _bound_template,
        const Tau& _source
    ) const
    {
        return m_fn(_bound_template, _source);
    }

    // operator() -- alias for apply
    D_NODISCARD D_CONSTEXPR sink_type
    operator()(
        const Tau& _bound_template,
        const Tau& _source
    ) const
    {
        return m_fn(_bound_template, _source);
    }

    // instantiate -- F_hat: decode a template into its transformer F_t
    D_NODISCARD D_CONSTEXPR transformer_type
    instantiate(
        const Tau& _bound_template
    ) const
    {
        return transformer_type(m_fn, _bound_template);
    }

    // transformation -- the underlying F
    D_NODISCARD D_CONSTEXPR const Fn&
    transformation() const
    {
        return m_fn;
    }

private:
    Fn m_fn;
};


// make_template_system
//   function: build a template_system over carrier Tau (explicit, since it is
// not deducible from a generic binary F) from a transformation _fn; the sink
// type sigma is deduced from `F(tau, tau)`.
template<typename Tau,
         typename Fn>
D_NODISCARD D_CONSTEXPR
template_system<clean_t<Fn>, Tau>
make_template_system(
    Fn&& _fn
)
{
    return template_system<clean_t<Fn>, Tau>(static_cast<Fn&&>(_fn));
}


// ===========================================================================
// III. STAGE PIPELINES
// ===========================================================================
//   A transformation may be assembled from stages F = f_n . ... . f_1.  Only
// f_1 is binary -- it alone consumes the (template, source) product; every
// later stage is unary.  The result is itself a transformation, pluggable as
// the F of a template_system.

NS_INTERNAL

    // unary_chain_helper
    //   class: left-to-right composition of unary stages, g_n . ... . g_1.  The
    // empty chain is the identity.  Stages held by value (decayed).
    template<typename... Gs>
    class unary_chain_helper;

    // unary_chain_helper<> (identity)
    //   class: the empty chain -- returns its argument unchanged.
    template<>
    class unary_chain_helper<>
    {
    public:
        D_CONSTEXPR unary_chain_helper()
        {}

        template<typename X>
        D_NODISCARD D_CONSTEXPR X
        operator()(
            X _x
        ) const
        {
            return _x;
        }
    };

    // unary_chain_helper<G, Gs...>
    //   class: peel the head stage G, then run the remaining chain on its
    // output.
    template<typename    G,
             typename... Gs>
    class unary_chain_helper<G, Gs...>
    {
    public:
        D_CONSTEXPR explicit unary_chain_helper(
            const G&     _g,
            const Gs&... _gs
        )
            : m_g(_g),
              m_rest(_gs...)
        {}

        template<typename X>
        D_NODISCARD D_CONSTEXPR auto
        operator()(
            X _x
        ) const
        {
            return m_rest(m_g(static_cast<X&&>(_x)));
        }

    private:
        G                          m_g;
        unary_chain_helper<Gs...> m_rest;
    };


    // stage_chain_helper
    //   class: a binary first stage f_1 : tau x tau -> tau_1 followed by the
    // unary chain f_2 .. f_n.  Calling with (t, alpha) runs f_1 on the product
    // and threads its result through the tail; the whole is a transformation.
    template<typename    First,
             typename... Rest>
    class stage_chain_helper
    {
    public:
        D_CONSTEXPR explicit stage_chain_helper(
            const First&   _first,
            const Rest&... _rest
        )
            : m_first(_first),
              m_tail(_rest...)
        {}

        template<typename Tau>
        D_NODISCARD D_CONSTEXPR auto
        operator()(
            const Tau& _bound_template,
            const Tau& _source
        ) const
        {
            return m_tail(m_first(_bound_template, _source));
        }

    private:
        First                        m_first;
        unary_chain_helper<Rest...> m_tail;
    };

NS_END  // internal


// stages
//   function: assemble a transformation F = f_n . ... . f_1 from a binary first
// stage f_1 : tau x tau -> tau_1 and zero or more unary stages f_2 .. f_n.  The
// template is consumed at stage 1 and is invisible thereafter; see
// `reader_stages` for the ambient-template variant.
template<typename    First,
         typename... Rest>
D_NODISCARD D_CONSTEXPR
internal::stage_chain_helper<clean_t<First>, clean_t<Rest>...>
stages(
    First&&   _first,
    Rest&&... _rest
)
{
    return internal::stage_chain_helper<clean_t<First>, clean_t<Rest>...>(
        static_cast<First&&>(_first),
        static_cast<Rest&&>(_rest)...);
}


NS_INTERNAL

    // reader_chain_helper
    //   class: the ambient-template ("Reader") stage chain.  Every stage is
    // binary (template, x) -> y; the template is threaded into all of them, so
    // F_t = (f_n)_t . ... . (f_1)_t.  The first stage's x is the source.
    template<typename... Fs>
    class reader_chain_helper;

    // reader_chain_helper<F> (last stage)
    //   class: a single ambient stage -- apply f(template, x).
    template<typename F>
    class reader_chain_helper<F>
    {
    public:
        D_CONSTEXPR explicit reader_chain_helper(
            const F& _f
        )
            : m_f(_f)
        {}

        template<typename Tau,
                 typename X>
        D_NODISCARD D_CONSTEXPR auto
        operator()(
            const Tau& _bound_template,
            const X&   _x
        ) const
        {
            return m_f(_bound_template, _x);
        }

    private:
        F m_f;
    };

    // reader_chain_helper<F, Next, Rest...>
    //   class: run the head ambient stage, then the rest -- both fed the same
    // template.
    template<typename    F,
             typename    Next,
             typename... Rest>
    class reader_chain_helper<F, Next, Rest...>
    {
    public:
        D_CONSTEXPR reader_chain_helper(
            const F&     _f,
            const Next&  _next,
            const Rest&... _rest
        )
            : m_f(_f),
              m_rest(_next, _rest...)
        {}

        template<typename Tau,
                 typename X>
        D_NODISCARD D_CONSTEXPR auto
        operator()(
            const Tau& _bound_template,
            const X&   _x
        ) const
        {
            return m_rest(_bound_template, m_f(_bound_template, _x));
        }

    private:
        F                                    m_f;
        reader_chain_helper<Next, Rest...> m_rest;
    };

NS_END  // internal


// reader_stages
//   function: assemble a transformation in the Reader / environment convention,
// F_t = (f_n)_t . ... . (f_1)_t, where the template is ambient to every stage.
// Each stage is binary (template, x) -> y; the first stage's second argument is
// the source.  Like `stages`, the result is a transformation usable as F.
template<typename    First,
         typename... Rest>
D_NODISCARD D_CONSTEXPR internal::reader_chain_helper<clean_t<First>, clean_t<Rest>...>
reader_stages(
    First&&   _first,
    Rest&&... _rest
)
{
    return internal::reader_chain_helper<clean_t<First>, clean_t<Rest>...>(
        static_cast<First&&>(_first),
        static_cast<Rest&&>(_rest)...);
}


// ===========================================================================
// IV.  PARSERS   (sigma = (rho x tau) + E)
// ===========================================================================
//   A parser is just a transformation whose sink has the shape (rho x tau) + E:
// a result rho paired with the remaining source tau on success, or an error E.
// Nothing in the schema changes -- only the choice of sigma.  An instantiated
// parser is the source-transformer P_t : tau -> parse_outcome.

NS_INTERNAL

    // parse_success_tag
    //   tag: selects parse_outcome's success constructor.
    struct parse_success_tag
    {};

    // parse_failure_tag
    //   tag: selects parse_outcome's failure constructor.
    struct parse_failure_tag
    {};

NS_END  // internal

// parse_outcome
//   class: a concrete (rho x tau) + E.  On success it carries a result of type
// Result and the remaining source of type Source; on failure it carries an
// Error.  Build it with `parse_success` / `parse_failure`.
//   Didactic storage: the three components share the object and the inactive
// ones are value-initialized, so Result, Source and Error must be
// default-constructible.  A production parser would instead pin sigma to a true
// sum such as `result<pair<rho, tau>, E>` (union storage, no such requirement).
template<typename Result,
         typename Source,
         typename Error>
class parse_outcome
{
public:
    using result_type = Result;
    using source_type = Source;
    using error_type  = Error;

    // success construction: ((value, remaining), ok)
    D_CONSTEXPR parse_outcome(
        internal::parse_success_tag,
        const Result& _value,
        const Source& _remaining
    )
        : m_value(_value),
          m_remaining(_remaining),
          m_error(),
          m_ok(true)
    {}

    // failure construction: (error, !ok)
    D_CONSTEXPR parse_outcome(
        internal::parse_failure_tag,
        const Error& _error
    )
        : m_value(),
          m_remaining(),
          m_error(_error),
          m_ok(false)
    {}

    D_NODISCARD D_CONSTEXPR bool
    is_ok() const
    {
        return m_ok;
    }

    D_NODISCARD D_CONSTEXPR bool
    is_err() const
    {
        return !m_ok;
    }

    D_NODISCARD D_CONSTEXPR explicit
    operator bool() const
    {
        return m_ok;
    }

    // value -- the produced result rho (defined only when is_ok())
    D_NODISCARD D_CONSTEXPR const Result&
    value() const
    {
        return m_value;
    }

    // remaining -- the leftover source tau threaded to the next stage
    D_NODISCARD D_CONSTEXPR const Source&
    remaining() const
    {
        return m_remaining;
    }

    // error -- the failure E (defined only when is_err())
    D_NODISCARD D_CONSTEXPR const Error&
    error() const
    {
        return m_error;
    }

private:
    Result m_value;
    Source m_remaining;
    Error   m_error;
    bool    m_ok;
};


// parse_success
//   function: build a successful parse_outcome carrying a result _value and the
// remaining source _rem.  The error type Error is explicit -- on success no E
// is otherwise present to deduce it from.
template<typename Error,
         typename Result,
         typename Source>
D_NODISCARD D_CONSTEXPR
parse_outcome<clean_t<Result>, clean_t<Source>, Error>
parse_success(
    Result&& _value,
    Source&& _rem
)
{
    return parse_outcome<clean_t<Result>, clean_t<Source>, Error>(
        internal::parse_success_tag{},
        static_cast<Result&&>(_value),
        static_cast<Source&&>(_rem));
}

// parse_failure
//   function: build a failed parse_outcome carrying an _error.  The result and
// source types are explicit -- on failure no rho / tau value is present.
template<typename Result,
         typename Source,
         typename Error>
D_NODISCARD D_CONSTEXPR
parse_outcome<Result, Source, clean_t<Error>>
parse_failure(
    Error&& _error
)
{
    return parse_outcome<Result, Source, clean_t<Error>>(
        internal::parse_failure_tag{},
        static_cast<Error&&>(_error));
}


// is_parse_outcome
//   trait: true iff Type is a parse_outcome<...> (+ is_parse_outcome_v).
D_TYPE_TRAIT_IS_SPECIALIZATION_OF(is_parse_outcome, parse_outcome)


// parser_system
//   type: a template_system whose sink is pinned to the parser shape
// (rho x tau) + E.  The transformation Fn is the grammar-and-input map
// P : tau x tau -> parse_outcome<rho, tau, E>; `instantiate` yields P_t.
template<typename Fn,
         typename Tau,
         typename Result,
         typename Error>
using parser_system =
    template_system<Fn, Tau, parse_outcome<Result, Tau, Error>>;


NS_INTERNAL

    // kleisli_then_helper
    //   class: sequences two instantiated parsers P_t, Q_t : tau -> parse_outcome
    // sharing a source type and error type.  Runs P; on success it threads P's
    // leftover source into Q and keeps Q's result; on failure it short-circuits,
    // re-tagging P's error as Q's outcome type.  This is the note's `Q <> P`.
    template<typename P,
             typename Q>
    class kleisli_then_helper
    {
    public:
        D_CONSTEXPR kleisli_then_helper(
            const P& _p,
            const Q& _q
        )
            : m_p(_p),
              m_q(_q)
        {}

        template<typename Source>
        D_NODISCARD D_CONSTEXPR_CPP14 auto
        operator()(
            const Source& _source
        ) const
        {
            using out_t = decltype(m_q(_source));

            auto first = m_p(_source);

            // success: feed P's leftover into Q
            if (first.is_ok())
            {
                return m_q(first.remaining());
            }

            return parse_failure<typename out_t::result_type,
                                 typename out_t::source_type>(first.error());
        }

    private:
        P m_p;
        Q m_q;
    };


    // kleisli_bind_helper
    //   class: monadic bind for parsers.  K is a continuation
    // (const result& -> parser): runs P, and on success applies K to the result
    // and runs the resulting parser on P's leftover source; failure
    // short-circuits.  This is the general `P >>= k`.
    template<typename P,
             typename K>
    class kleisli_bind_helper
    {
    public:
        D_CONSTEXPR kleisli_bind_helper(
            const P& _p,
            const K& _k
        )
            : m_p(_p),
              m_k(_k)
        {}

        template<typename Source>
        D_NODISCARD D_CONSTEXPR_CPP14 auto
        operator()(
            const Source& _source
        ) const
        {
            auto first = m_p(_source);

            using cont_t = decltype(m_k(first.value()));
            using out_t  = decltype(std::declval<cont_t>()(_source));

            // success: run the continuation parser on P's leftover
            if (first.is_ok())
            {
                return m_k(first.value())(first.remaining());
            }

            return parse_failure<typename out_t::result_type,
                                 typename out_t::source_type>(first.error());
        }

    private:
        P m_p;
        K m_k;
    };

NS_END  // internal


// kleisli_then
//   function: sequence two parsers, threading the remaining source and keeping
// the second parser's result (the note's `Q <> P`).  Both must share the source
// and error types; the result type may differ.
template<typename P,
         typename Q>
D_NODISCARD D_CONSTEXPR
internal::kleisli_then_helper<clean_t<P>, clean_t<Q>>
kleisli_then(
    P&& _p,
    Q&& _q
)
{
    return internal::kleisli_then_helper<clean_t<P>, clean_t<Q>>(
        static_cast<P&&>(_p),
        static_cast<Q&&>(_q));
}

// kleisli_bind
//   function: monadic bind `P >>= k`, where _k maps the result of _p to the
// next parser; the leftover source is threaded and failure short-circuits.
template<typename P,
         typename K>
D_NODISCARD D_CONSTEXPR
internal::kleisli_bind_helper<clean_t<P>, clean_t<K>>
kleisli_bind(
    P&& _p,
    K&& _k
)
{
    return internal::kleisli_bind_helper<clean_t<P>, clean_t<K>>(
        static_cast<P&&>(_p),
        static_cast<K&&>(_k));
}


NS_END  // djinterp


// ===========================================================================
// V.   CONCEPTS   (C++20)
// ===========================================================================
//   Concept parallels of the section-I / section-IV traits, following the
// trait-triple convention: where an `is_X` trait exists, a concept named `X`,
// `X_for`, or `X_c` lives alongside it.

#if D_ENV_CPP_FEATURE_LANG_CONCEPTS

NS_DJINTERP

// transformation_for
//   concept: Fn is a transformation F : tau x tau -> sigma over carrier Tau.
template<typename Fn,
         typename Tau>
concept transformation_for = is_transformation<Fn, Tau>::value;

// source_transformer_for
//   concept: Fn is a source-transformer F_t : tau -> sigma over carrier Tau.
template<typename Fn,
         typename Tau>
concept source_transformer_for = is_source_transformer<Fn, Tau>::value;

// parse_outcome_c
//   concept: Type is a parse_outcome<...> -- a (rho x tau) + E parser sink.
template<typename Type>
concept parse_outcome_c = is_parse_outcome<Type>::value;

NS_END  // djinterp

#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS


#endif  // D_ENV_LANG_IS_CPP14_OR_HIGHER

#endif  // floor, for now


#endif  // DJINTERP_PARADIGM_TEMPLATE_TEMPLATE_HPP
