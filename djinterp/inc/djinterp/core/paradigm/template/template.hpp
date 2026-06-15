/******************************************************************************
* djinterp [meta]                                                 template.hpp
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
* path:      /inc/djinterp/core/meta/template.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.06.13
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    TRANSFORMATION TRAITS
      i.   transformation_sink (sigma deduction) + _t
      ii.  is_transformation (F : tau x tau -> sigma) + _v
      iii. source_transformer_sink + _t; is_source_transformer + _v

II.   THE ALGEBRA
      i.   evaluate                 -- ev : (tau -> sigma) x tau -> sigma
      ii.  transformer_helper       -- F_t = F(t, .) (internal)
      iii. instantiate              -- F_hat at a point
      iv.  template_system          -- packages (tau, sigma, F)
      v.   make_template_system

III.  STAGE PIPELINES
      i.   unary_chain_helper       -- g_n . ... . g_1 (internal)
      ii.  stage_chain_helper       -- f_n . ... . f_1, f_1 binary (internal)
      iii. stages                   -- template consumed at stage 1
      iv.  reader_chain_helper      -- ambient template (internal)
      v.   reader_stages            -- the Reader / environment variant

IV.   PARSERS  (sigma = (rho x tau) + E)
      i.   parse_success_tag / parse_failure_tag (internal)
      ii.  parse_outcome            -- the parser sink
      iii. parse_success / parse_failure
      iv.  is_parse_outcome + _v
      v.   parser_system            -- alias pinning sigma
      vi.  kleisli_then_helper / kleisli_bind_helper (internal)
      vii. kleisli_then / kleisli_bind

V.    CONCEPTS  (C++20)
*/

#ifndef DJINTERP_META_TEMPLATE_
#define DJINTERP_META_TEMPLATE_ 1

// std
#include <cstddef>
#include <type_traits>
#include <utility>
// djinterp
#include "../../djinterp.hpp"      // NS_*, D_CONSTEXPR, D_NODISCARD, clean_t
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
    //   trait: tagless detector for `F(const _Tau&, const _Tau&)`.  The leading
    // overload's return type is the deduced sink; on substitution failure the
    // variadic overload yields `nonesuch`.  Declared, never defined -- used
    // only in unevaluated context.
    template<typename _Fn,
             typename _Tau>
    auto transformation_sink_probe(int)
        -> invoke_result_t<_Fn, const _Tau&, const _Tau&>;

    template<typename,
             typename>
    auto transformation_sink_probe(...) -> nonesuch;

    // source_transformer_probe
    //   trait: tagless detector for `g(const _Tau&)` -- the shape of a
    // source-transformer F_t.  Same overload structure as above.
    template<typename _Fn,
             typename _Tau>
    auto source_transformer_probe(int)
        -> invoke_result_t<_Fn, const _Tau&>;

    template<typename,
             typename>
    auto source_transformer_probe(...) -> nonesuch;

NS_END  // internal


// transformation_sink
//   trait: the sink type sigma produced by a transformation _Fn applied to a
// template and a source, both of carrier type _Tau (i.e. the result of
// `F(const _Tau&, const _Tau&)`), or `nonesuch` when _Fn is not so callable.
template<typename _Fn,
         typename _Tau>
struct transformation_sink
{
    using type = decltype(internal::transformation_sink_probe<_Fn, _Tau>(0));
};

// transformation_sink_t
//   type: convenience alias for transformation_sink<_Fn, _Tau>::type.
template<typename _Fn,
         typename _Tau>
using transformation_sink_t = typename transformation_sink<_Fn, _Tau>::type;


// is_transformation
//   trait: true iff _Fn is a transformation over carrier _Tau -- callable as
// `F(const _Tau&, const _Tau&)`, i.e. its sink is detectable.  Parallel
// concept: `transformation_for`.
template<typename _Fn,
         typename _Tau>
struct is_transformation
    : std::integral_constant<bool,
          !std::is_same<transformation_sink_t<_Fn, _Tau>, nonesuch>::value>
{};

// is_transformation_v
//   value: variable-template shorthand for is_transformation<_Fn, _Tau>::value.
// (Two-parameter trait, so the unary D_TYPE_TRAIT_VALUE_BOOL sugar does not
// apply; the standard gating is reproduced here by hand.)
#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    template<typename _Fn,
             typename _Tau>
    inline constexpr bool is_transformation_v =
        is_transformation<_Fn, _Tau>::value;
#elif D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Fn,
             typename _Tau>
    constexpr bool is_transformation_v = is_transformation<_Fn, _Tau>::value;
#endif


// source_transformer_sink
//   trait: the sink sigma produced by a source-transformer F_t applied to a
// single source of carrier _Tau (i.e. the result of `g(const _Tau&)`), or
// `nonesuch` when _Fn is not so callable.
template<typename _Fn,
         typename _Tau>
struct source_transformer_sink
{
    using type = decltype(internal::source_transformer_probe<_Fn, _Tau>(0));
};

// source_transformer_sink_t
//   type: convenience alias for source_transformer_sink<_Fn, _Tau>::type.
template<typename _Fn,
         typename _Tau>
using source_transformer_sink_t =
    typename source_transformer_sink<_Fn, _Tau>::type;


// is_source_transformer
//   trait: true iff _Fn is a source-transformer F_t : tau -> sigma over carrier
// _Tau -- callable as `g(const _Tau&)`, i.e. its sink is detectable.  Parallel
// concept: `source_transformer_for`.
template<typename _Fn,
         typename _Tau>
struct is_source_transformer
    : std::integral_constant<bool,
          !std::is_same<source_transformer_sink_t<_Fn, _Tau>, nonesuch>::value>
{};

// is_source_transformer_v
//   value: variable-template shorthand for the trait above.
#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    template<typename _Fn,
             typename _Tau>
    inline constexpr bool is_source_transformer_v =
        is_source_transformer<_Fn, _Tau>::value;
#elif D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Fn,
             typename _Tau>
    constexpr bool is_source_transformer_v =
        is_source_transformer<_Fn, _Tau>::value;
#endif


// ===========================================================================
// II.  THE ALGEBRA
// ===========================================================================

// evaluate
//   function: the universal evaluation map ev : (tau -> sigma) x tau -> sigma.
// Applies a source-transformer _g (an F_t) to a source _alpha and returns the
// sink.  The whole module exists to make `evaluate(instantiate(fn, t), alpha)`
// equal `fn(t, alpha)` -- the factorization F = ev . (F_hat x id).
template<typename _Transformer,
         typename _Source>
D_NODISCARD D_CONSTEXPR auto
evaluate(
    _Transformer&& _g,
    _Source&&      _alpha
)
D_NOEXCEPT_IF(noexcept(static_cast<_Transformer&&>(_g)(
                           static_cast<_Source&&>(_alpha))))
{
    return static_cast<_Transformer&&>(_g)(static_cast<_Source&&>(_alpha));
}


NS_INTERNAL

    // transformer_helper
    //   class: the curried transformer F_t = F(t, .) : tau -> sigma -- the image
    // of a template t under F_hat.  Holds the transformation F (decayed) and the
    // bound template; calling it with a source applies F.  Returned by
    // `instantiate` and by `template_system::instantiate`.
    template<typename _Fn,
             typename _Tau>
    class transformer_helper
    {
    public:
        using carrier_type = _Tau;
        using sink_type    = transformation_sink_t<_Fn, _Tau>;

        // bind a template to a transformation
        D_CONSTEXPR transformer_helper(
            const _Fn&  _fn,
            const _Tau& _bound_template
        )
            : m_fn(_fn),
              m_template(_bound_template)
        {}

        // apply F_t to a source: F(t, alpha)
        D_NODISCARD D_CONSTEXPR sink_type
        operator()(
            const _Tau& _source
        ) const
        {
            return m_fn(m_template, _source);
        }

    private:
        _Fn  m_fn;
        _Tau m_template;
    };

NS_END  // internal


// instantiate
//   function: F_hat at a point.  Decodes a template _t under a bare
// transformation _fn into its source-transformer F_t : tau -> sigma, so that
// `evaluate(instantiate(fn, t), alpha) == fn(t, alpha)`.  The carrier tau is
// deduced from the template argument.
template<typename _Fn,
         typename _Tau>
D_NODISCARD D_CONSTEXPR
internal::transformer_helper<clean_t<_Fn>, clean_t<_Tau>>
instantiate(
    _Fn&&  _fn,
    _Tau&& _t
)
{
    return internal::transformer_helper<clean_t<_Fn>, clean_t<_Tau>>(
        static_cast<_Fn&&>(_fn),
        static_cast<_Tau&&>(_t));
}


// template_system
//   class: a template-source-sink system (tau, sigma, F) realized as a value.
// Holds the transformation F : tau x tau -> sigma.  `apply` (and operator()) is
// the uncurried F; `instantiate` is F_hat (it returns the transformer F_t); and
// the identity
//       apply(t, alpha) == evaluate(instantiate(t), alpha)
// is the factorization F = ev . (F_hat x id), true here by construction.  The
// sink type defaults to the deduced sigma but may be pinned (see parser_system).
template<typename _Fn,
         typename _Tau,
         typename _Sigma = transformation_sink_t<_Fn, _Tau>>
class template_system
{
public:
    using carrier_type        = _Tau;
    using sink_type           = _Sigma;
    using transformation_type = _Fn;
    using transformer_type    = internal::transformer_helper<_Fn, _Tau>;

    // wrap a transformation
    D_CONSTEXPR explicit template_system(
        const _Fn& _fn
    )
        : m_fn(_fn)
    {}

    // apply -- the uncurried F(t, alpha) = beta
    D_NODISCARD D_CONSTEXPR sink_type
    apply(
        const _Tau& _bound_template,
        const _Tau& _source
    ) const
    {
        return m_fn(_bound_template, _source);
    }

    // operator() -- alias for apply
    D_NODISCARD D_CONSTEXPR sink_type
    operator()(
        const _Tau& _bound_template,
        const _Tau& _source
    ) const
    {
        return m_fn(_bound_template, _source);
    }

    // instantiate -- F_hat: decode a template into its transformer F_t
    D_NODISCARD D_CONSTEXPR transformer_type
    instantiate(
        const _Tau& _bound_template
    ) const
    {
        return transformer_type(m_fn, _bound_template);
    }

    // transformation -- the underlying F
    D_NODISCARD D_CONSTEXPR const _Fn&
    transformation() const
    {
        return m_fn;
    }

private:
    _Fn m_fn;
};


// make_template_system
//   function: build a template_system over carrier _Tau (explicit, since it is
// not deducible from a generic binary F) from a transformation _fn; the sink
// type sigma is deduced from `F(tau, tau)`.
template<typename _Tau,
         typename _Fn>
D_NODISCARD D_CONSTEXPR
template_system<clean_t<_Fn>, _Tau>
make_template_system(
    _Fn&& _fn
)
{
    return template_system<clean_t<_Fn>, _Tau>(static_cast<_Fn&&>(_fn));
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
    template<typename... _Gs>
    class unary_chain_helper;

    // unary_chain_helper<> (identity)
    //   class: the empty chain -- returns its argument unchanged.
    template<>
    class unary_chain_helper<>
    {
    public:
        D_CONSTEXPR unary_chain_helper()
        {}

        template<typename _X>
        D_NODISCARD D_CONSTEXPR _X
        operator()(
            _X _x
        ) const
        {
            return _x;
        }
    };

    // unary_chain_helper<_G, _Gs...>
    //   class: peel the head stage _G, then run the remaining chain on its
    // output.
    template<typename    _G,
             typename... _Gs>
    class unary_chain_helper<_G, _Gs...>
    {
    public:
        D_CONSTEXPR explicit unary_chain_helper(
            const _G&     _g,
            const _Gs&... _gs
        )
            : m_g(_g),
              m_rest(_gs...)
        {}

        template<typename _X>
        D_NODISCARD D_CONSTEXPR auto
        operator()(
            _X _x
        ) const
        {
            return m_rest(m_g(static_cast<_X&&>(_x)));
        }

    private:
        _G                         m_g;
        unary_chain_helper<_Gs...> m_rest;
    };


    // stage_chain_helper
    //   class: a binary first stage f_1 : tau x tau -> tau_1 followed by the
    // unary chain f_2 .. f_n.  Calling with (t, alpha) runs f_1 on the product
    // and threads its result through the tail; the whole is a transformation.
    template<typename    _First,
             typename... _Rest>
    class stage_chain_helper
    {
    public:
        D_CONSTEXPR explicit stage_chain_helper(
            const _First&   _first,
            const _Rest&... _rest
        )
            : m_first(_first),
              m_tail(_rest...)
        {}

        template<typename _Tau>
        D_NODISCARD D_CONSTEXPR auto
        operator()(
            const _Tau& _bound_template,
            const _Tau& _source
        ) const
        {
            return m_tail(m_first(_bound_template, _source));
        }

    private:
        _First                       m_first;
        unary_chain_helper<_Rest...> m_tail;
    };

NS_END  // internal


// stages
//   function: assemble a transformation F = f_n . ... . f_1 from a binary first
// stage f_1 : tau x tau -> tau_1 and zero or more unary stages f_2 .. f_n.  The
// template is consumed at stage 1 and is invisible thereafter; see
// `reader_stages` for the ambient-template variant.
template<typename    _First,
         typename... _Rest>
D_NODISCARD D_CONSTEXPR
internal::stage_chain_helper<clean_t<_First>, clean_t<_Rest>...>
stages(
    _First&&   _first,
    _Rest&&... _rest
)
{
    return internal::stage_chain_helper<clean_t<_First>, clean_t<_Rest>...>(
        static_cast<_First&&>(_first),
        static_cast<_Rest&&>(_rest)...);
}


NS_INTERNAL

    // reader_chain_helper
    //   class: the ambient-template ("Reader") stage chain.  Every stage is
    // binary (template, x) -> y; the template is threaded into all of them, so
    // F_t = (f_n)_t . ... . (f_1)_t.  The first stage's x is the source.
    template<typename... _Fs>
    class reader_chain_helper;

    // reader_chain_helper<_F> (last stage)
    //   class: a single ambient stage -- apply f(template, x).
    template<typename _F>
    class reader_chain_helper<_F>
    {
    public:
        D_CONSTEXPR explicit reader_chain_helper(
            const _F& _f
        )
            : m_f(_f)
        {}

        template<typename _Tau,
                 typename _X>
        D_NODISCARD D_CONSTEXPR auto
        operator()(
            const _Tau& _bound_template,
            const _X&   _x
        ) const
        {
            return m_f(_bound_template, _x);
        }

    private:
        _F m_f;
    };

    // reader_chain_helper<_F, _Next, _Rest...>
    //   class: run the head ambient stage, then the rest -- both fed the same
    // template.
    template<typename    _F,
             typename    _Next,
             typename... _Rest>
    class reader_chain_helper<_F, _Next, _Rest...>
    {
    public:
        D_CONSTEXPR reader_chain_helper(
            const _F&     _f,
            const _Next&  _next,
            const _Rest&... _rest
        )
            : m_f(_f),
              m_rest(_next, _rest...)
        {}

        template<typename _Tau,
                 typename _X>
        D_NODISCARD D_CONSTEXPR auto
        operator()(
            const _Tau& _bound_template,
            const _X&   _x
        ) const
        {
            return m_rest(_bound_template, m_f(_bound_template, _x));
        }

    private:
        _F                                   m_f;
        reader_chain_helper<_Next, _Rest...> m_rest;
    };

NS_END  // internal


// reader_stages
//   function: assemble a transformation in the Reader / environment convention,
// F_t = (f_n)_t . ... . (f_1)_t, where the template is ambient to every stage.
// Each stage is binary (template, x) -> y; the first stage's second argument is
// the source.  Like `stages`, the result is a transformation usable as F.
template<typename    _First,
         typename... _Rest>
D_NODISCARD D_CONSTEXPR
internal::reader_chain_helper<clean_t<_First>, clean_t<_Rest>...>
reader_stages(
    _First&&   _first,
    _Rest&&... _rest
)
{
    return internal::reader_chain_helper<clean_t<_First>, clean_t<_Rest>...>(
        static_cast<_First&&>(_first),
        static_cast<_Rest&&>(_rest)...);
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
// _Result and the remaining source of type _Source; on failure it carries an
// _Error.  Build it with `parse_success` / `parse_failure`.
//   Didactic storage: the three components share the object and the inactive
// ones are value-initialized, so _Result, _Source and _Error must be
// default-constructible.  A production parser would instead pin sigma to a true
// sum such as `result<pair<rho, tau>, E>` (union storage, no such requirement).
template<typename _Result,
         typename _Source,
         typename _Error>
class parse_outcome
{
public:
    using result_type = _Result;
    using source_type = _Source;
    using error_type  = _Error;

    // success construction: ((value, remaining), ok)
    D_CONSTEXPR parse_outcome(
        internal::parse_success_tag,
        const _Result& _value,
        const _Source& _remaining
    )
        : m_value(_value),
          m_remaining(_remaining),
          m_error(),
          m_ok(true)
    {}

    // failure construction: (error, !ok)
    D_CONSTEXPR parse_outcome(
        internal::parse_failure_tag,
        const _Error& _error
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
    D_NODISCARD D_CONSTEXPR const _Result&
    value() const
    {
        return m_value;
    }

    // remaining -- the leftover source tau threaded to the next stage
    D_NODISCARD D_CONSTEXPR const _Source&
    remaining() const
    {
        return m_remaining;
    }

    // error -- the failure E (defined only when is_err())
    D_NODISCARD D_CONSTEXPR const _Error&
    error() const
    {
        return m_error;
    }

private:
    _Result m_value;
    _Source m_remaining;
    _Error  m_error;
    bool    m_ok;
};


// parse_success
//   function: build a successful parse_outcome carrying a result _value and the
// remaining source _rem.  The error type _Error is explicit -- on success no E
// is otherwise present to deduce it from.
template<typename _Error,
         typename _Result,
         typename _Source>
D_NODISCARD D_CONSTEXPR
parse_outcome<clean_t<_Result>, clean_t<_Source>, _Error>
parse_success(
    _Result&& _value,
    _Source&& _rem
)
{
    return parse_outcome<clean_t<_Result>, clean_t<_Source>, _Error>(
        internal::parse_success_tag{},
        static_cast<_Result&&>(_value),
        static_cast<_Source&&>(_rem));
}

// parse_failure
//   function: build a failed parse_outcome carrying an _error.  The result and
// source types are explicit -- on failure no rho / tau value is present.
template<typename _Result,
         typename _Source,
         typename _Error>
D_NODISCARD D_CONSTEXPR
parse_outcome<_Result, _Source, clean_t<_Error>>
parse_failure(
    _Error&& _error
)
{
    return parse_outcome<_Result, _Source, clean_t<_Error>>(
        internal::parse_failure_tag{},
        static_cast<_Error&&>(_error));
}


// is_parse_outcome
//   trait: true iff _Type is a parse_outcome<...> (+ is_parse_outcome_v).
D_TYPE_TRAIT_IS_SPECIALIZATION_OF(is_parse_outcome, parse_outcome)


// parser_system
//   type: a template_system whose sink is pinned to the parser shape
// (rho x tau) + E.  The transformation _Fn is the grammar-and-input map
// P : tau x tau -> parse_outcome<rho, tau, E>; `instantiate` yields P_t.
template<typename _Fn,
         typename _Tau,
         typename _Result,
         typename _Error>
using parser_system =
    template_system<_Fn, _Tau, parse_outcome<_Result, _Tau, _Error>>;


NS_INTERNAL

    // kleisli_then_helper
    //   class: sequences two instantiated parsers P_t, Q_t : tau -> parse_outcome
    // sharing a source type and error type.  Runs P; on success it threads P's
    // leftover source into Q and keeps Q's result; on failure it short-circuits,
    // re-tagging P's error as Q's outcome type.  This is the note's `Q <> P`.
    template<typename _P,
             typename _Q>
    class kleisli_then_helper
    {
    public:
        D_CONSTEXPR kleisli_then_helper(
            const _P& _p,
            const _Q& _q
        )
            : m_p(_p),
              m_q(_q)
        {}

        template<typename _Source>
        D_NODISCARD D_CONSTEXPR auto
        operator()(
            const _Source& _source
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
        _P m_p;
        _Q m_q;
    };


    // kleisli_bind_helper
    //   class: monadic bind for parsers.  _K is a continuation
    // (const result& -> parser): runs P, and on success applies K to the result
    // and runs the resulting parser on P's leftover source; failure
    // short-circuits.  This is the general `P >>= k`.
    template<typename _P,
             typename _K>
    class kleisli_bind_helper
    {
    public:
        D_CONSTEXPR kleisli_bind_helper(
            const _P& _p,
            const _K& _k
        )
            : m_p(_p),
              m_k(_k)
        {}

        template<typename _Source>
        D_NODISCARD D_CONSTEXPR auto
        operator()(
            const _Source& _source
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
        _P m_p;
        _K m_k;
    };

NS_END  // internal


// kleisli_then
//   function: sequence two parsers, threading the remaining source and keeping
// the second parser's result (the note's `Q <> P`).  Both must share the source
// and error types; the result type may differ.
template<typename _P,
         typename _Q>
D_NODISCARD D_CONSTEXPR
internal::kleisli_then_helper<clean_t<_P>, clean_t<_Q>>
kleisli_then(
    _P&& _p,
    _Q&& _q
)
{
    return internal::kleisli_then_helper<clean_t<_P>, clean_t<_Q>>(
        static_cast<_P&&>(_p),
        static_cast<_Q&&>(_q));
}

// kleisli_bind
//   function: monadic bind `P >>= k`, where _k maps the result of _p to the
// next parser; the leftover source is threaded and failure short-circuits.
template<typename _P,
         typename _K>
D_NODISCARD D_CONSTEXPR
internal::kleisli_bind_helper<clean_t<_P>, clean_t<_K>>
kleisli_bind(
    _P&& _p,
    _K&& _k
)
{
    return internal::kleisli_bind_helper<clean_t<_P>, clean_t<_K>>(
        static_cast<_P&&>(_p),
        static_cast<_K&&>(_k));
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
//   concept: _Fn is a transformation F : tau x tau -> sigma over carrier _Tau.
template<typename _Fn,
         typename _Tau>
concept transformation_for = is_transformation<_Fn, _Tau>::value;

// source_transformer_for
//   concept: _Fn is a source-transformer F_t : tau -> sigma over carrier _Tau.
template<typename _Fn,
         typename _Tau>
concept source_transformer_for = is_source_transformer<_Fn, _Tau>::value;

// parse_outcome_c
//   concept: _Type is a parse_outcome<...> -- a (rho x tau) + E parser sink.
template<typename _Type>
concept parse_outcome_c = is_parse_outcome<_Type>::value;

NS_END  // djinterp

#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS


#endif  // D_ENV_LANG_IS_CPP14_OR_HIGHER


#endif  // DJINTERP_META_TEMPLATE_
