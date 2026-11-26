/*******************************************************************************
* djinterp [cpp][math]                                            functional.hpp
*
* Bridge between the math module and the functional subframework.
*   Math expressions, math_function, and math relations all expose operator(),
* so they already model the functional vocabulary:
*
*     - a single-variable expression  is a unary callable  (double -> double)
*       and thus an is_unary_transformer for compose / pipe / pipeline.map;
*     - an N-variable expression / math_function is an N-ary is_callable, so it
*       can be curry()'d, flip()'d, and partially applied;
*     - a relation (x > 0, x == y, ...) returns bool, so it is an is_predicate
*       for predicate combinators and pipeline.filter.
*
*   The one rough edge: an expression's operator() is a *template*, so the
* declared-shape introspection in function_traits.hpp (arity / return type)
* cannot read it. The adapters below pin an expression to a concrete,
* non-template signature, making it introspectable and std::function-free while
* still composing through the functional combinators.
*
* LAYOUT NOTE:
*   The functional subframework is inc/djinterp/cpp/functional/, which the
* includes below reach as "../../functional/...". Define
* DJINTERP_MATH_FUNCTIONAL_NO_DEPS before including this header to take only
* the adapter layer (e.g. to use the adapters without pulling the whole
* functional subframework).
*
* LEVELS (step 30, 5b)
*   C++11 is the floor, as for expression.hpp and the functional
* subframework, and the header is empty below it. From C++11 each adapter's
* call is a constant expression wherever its expression's evaluation is.
*
*
* path:      /inc/djinterp/cpp/modules/math/functional.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.06.20
*                                                            revised: 2026.11.24
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  FIXED-SIGNATURE ADAPTERS
    ------------------------
2.  ADAPTER FACTORIES
    -----------------
*/

#ifndef DJINTERP_MATH_FUNCTIONAL_HPP
#define DJINTERP_MATH_FUNCTIONAL_HPP 1

// djinterp
#include "../../djinterp.hpp"  // framework root

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// djinterp - math
#include "./expression.hpp"           // expressions
#include "./function/function.hpp"    // relations, math_function

// djinterp - functional subframework (see LAYOUT NOTE)
#ifndef DJINTERP_MATH_FUNCTIONAL_NO_DEPS
    #include "../../functional/compose.hpp"   // compose / pipe / pipe_all
    #include "../../functional/curry.hpp"     // curry / flip / uncurry
    #include "../../functional/pipeline.hpp"  // DFunctionPipeline
#endif


NS_DJINTERP
NS_MATH

namespace functional
{
    //==========================================================================
    // 1.  FIXED-SIGNATURE ADAPTERS
    //==========================================================================
    // Each adapter stores an expression by value and exposes a concrete,
    // non-template operator(), so the result is introspectable by
    // function_traits.hpp and usable anywhere a plain callable is expected.
    // The value type defaults to double, matching the expression kernel.

    // unary_adapter
    //   wraps an expression as a concrete T(T) callable.
    template<typename Expr,
             typename T = double>
    struct unary_adapter
    {
        Expr expression;

        D_CONSTEXPR T
        operator()(T _x) const
        {
            return static_cast<T>(expression(_x));
        }
    };

    // binary_adapter
    //   wraps an expression as a concrete T(T, T) callable.
    template<typename Expr,
             typename T = double>
    struct binary_adapter
    {
        Expr expression;

        D_CONSTEXPR T
        operator()(T _x, T _y) const
        {
            return static_cast<T>(expression(_x, _y));
        }
    };

    // ternary_adapter
    //   wraps an expression as a concrete T(T, T, T) callable (e.g. a
    // cylindrical or spherical scalar field).
    template<typename Expr,
             typename T = double>
    struct ternary_adapter
    {
        Expr expression;

        D_CONSTEXPR T
        operator()(T _x, T _y, T _z) const
        {
            return static_cast<T>(expression(_x, _y, _z));
        }
    };

    // predicate_adapter
    //   wraps a relation/expression as a concrete bool(T) predicate.
    template<typename Rel,
             typename T = double>
    struct predicate_adapter
    {
        Rel relation;

        D_CONSTEXPR bool
        operator()(T _x) const
        {
            return static_cast<bool>(relation(_x));
        }
    };

    //==========================================================================
    // 2.  ADAPTER FACTORIES
    //==========================================================================
    // By-value parameters give a fully decayed Expr, so the adapter stores a
    // clean value with no dangling references.

    template<typename T = double, typename Expr>
    D_CONSTEXPR unary_adapter<Expr, T>
    as_unary(Expr _e)
    {
        return unary_adapter<Expr, T>{ _e };
    }

    template<typename T = double, typename Expr>
    D_CONSTEXPR binary_adapter<Expr, T>
    as_binary(Expr _e)
    {
        return binary_adapter<Expr, T>{ _e };
    }

    template<typename T = double, typename Expr>
    D_CONSTEXPR ternary_adapter<Expr, T>
    as_ternary(Expr _e)
    {
        return ternary_adapter<Expr, T>{ _e };
    }

    template<typename T = double, typename Rel>
    D_CONSTEXPR predicate_adapter<Rel, T>
    as_predicate(Rel _r)
    {
        return predicate_adapter<Rel, T>{ _r };
    }

}  // functional

NS_END  // math
NS_END  // djinterp

/******************************************************************************
* INTEGRATION PATTERNS (production, with the functional subframework present):
*
*   using namespace djinterp;
*   using namespace djinterp::math;
*   using namespace djinterp::math::variables;  // x to phi (S403)
*   namespace mf = djinterp::math::functional;
*
*   // expression as a unary transformer in a pipeline:
*   auto sq = x * x;
*   auto out = pipeline_from(xs).map(mf::as_unary(sq)).to_vector();
*
*   // math relation as a filter predicate:
*   auto positive = (x > constant(0.0));
*   auto kept = pipeline_from(xs).filter(mf::as_predicate(positive)).to_vector();
*
*   // curry / partially apply a 2-variable expression:
*   auto plane = x + y;
*   auto g     = curry(mf::as_binary(plane));   // g(1.0)(2.0) == 3.0
*
*   // compose a math expression with an arbitrary callable (math order):
*   auto h = compose(mf::as_unary(sq), [](double v){ return v + 1.0; });
*
* Raw (un-adapted) expressions also satisfy is_callable / is_predicate, so they
* work directly with the combinators; the adapters add fixed-signature
* introspectability and a stable value type.
******************************************************************************/

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#endif  // DJINTERP_MATH_FUNCTIONAL_HPP
