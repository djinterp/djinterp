/******************************************************************************
* djinterp [math]                                               functional.hpp
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
*   The functional-framework includes below assume math/ and functional/ are
* sibling directories under the same include root (inc/math/, inc/functional/).
* Adjust the "../functional/..." paths if your tree differs. Define
* DJINTERP_MATH_FUNCTIONAL_NO_DEPS before including this header to take only the
* std-only adapter layer (e.g. to use the adapters without pulling the whole
* functional subframework).
*
* path:      /inc/djinterp/math/functional.hpp
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.06.20
******************************************************************************/

#ifndef DJINTERP_MATH_FUNCTIONAL_
#define DJINTERP_MATH_FUNCTIONAL_ 1

// std
#include <type_traits>
#include <utility>
// djinterp -- math
#include "../djinterp.hpp"
#include "./expression.hpp"
#include "./function.hpp"

// djinterp -- functional subframework (see LAYOUT NOTE)
#ifndef DJINTERP_MATH_FUNCTIONAL_NO_DEPS
#  include "../functional/functional_traits.hpp"   // is_callable / is_predicate
#  include "../functional/compose.hpp"             // compose / pipe / pipe_all
#  include "../functional/curry.hpp"               // curry / flip / uncurry
#  include "../functional/pipeline.hpp"            // function_pipeline
#endif


NS_DJINTERP
NS_MATH

namespace functional
{
    // ========================================================================
    // I.    FIXED-SIGNATURE ADAPTERS
    // ========================================================================
    // Each adapter stores an expression by value and exposes a concrete,
    // non-template operator(), so the result is introspectable by
    // function_traits.hpp and usable anywhere a plain callable is expected.
    // The value type defaults to double, matching the expression kernel.

    // unary_adapter
    //   wraps an expression as a concrete _T(_T) callable.
    template<typename _Expr,
             typename _T = double>
    struct unary_adapter
    {
        _Expr expression;

        D_CONSTEXPR _T
        operator()(_T _x) const
        {
            return static_cast<_T>(expression(_x));
        }
    };

    // binary_adapter
    //   wraps an expression as a concrete _T(_T, _T) callable.
    template<typename _Expr,
             typename _T = double>
    struct binary_adapter
    {
        _Expr expression;

        D_CONSTEXPR _T
        operator()(_T _x, _T _y) const
        {
            return static_cast<_T>(expression(_x, _y));
        }
    };

    // ternary_adapter
    //   wraps an expression as a concrete _T(_T, _T, _T) callable (e.g. a
    // cylindrical or spherical scalar field).
    template<typename _Expr,
             typename _T = double>
    struct ternary_adapter
    {
        _Expr expression;

        D_CONSTEXPR _T
        operator()(_T _x, _T _y, _T _z) const
        {
            return static_cast<_T>(expression(_x, _y, _z));
        }
    };

    // predicate_adapter
    //   wraps a relation/expression as a concrete bool(_T) predicate.
    template<typename _Rel,
             typename _T = double>
    struct predicate_adapter
    {
        _Rel relation;

        D_CONSTEXPR bool
        operator()(_T _x) const
        {
            return static_cast<bool>(relation(_x));
        }
    };

    // ========================================================================
    // II.   ADAPTER FACTORIES
    // ========================================================================
    // By-value parameters give a fully decayed _Expr, so the adapter stores a
    // clean value with no dangling references.

    template<typename _T = double, typename _Expr>
    D_CONSTEXPR unary_adapter<_Expr, _T>
    as_unary(_Expr _e)
    {
        return unary_adapter<_Expr, _T>{ _e };
    }

    template<typename _T = double, typename _Expr>
    D_CONSTEXPR binary_adapter<_Expr, _T>
    as_binary(_Expr _e)
    {
        return binary_adapter<_Expr, _T>{ _e };
    }

    template<typename _T = double, typename _Expr>
    D_CONSTEXPR ternary_adapter<_Expr, _T>
    as_ternary(_Expr _e)
    {
        return ternary_adapter<_Expr, _T>{ _e };
    }

    template<typename _T = double, typename _Rel>
    D_CONSTEXPR predicate_adapter<_Rel, _T>
    as_predicate(_Rel _r)
    {
        return predicate_adapter<_Rel, _T>{ _r };
    }

}  // functional

NS_END  // math
NS_END  // djinterp

/******************************************************************************
* INTEGRATION PATTERNS (production, with the functional subframework present):
*
*   using namespace djinterp;
*   using namespace djinterp::math;
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

#endif  // DJINTERP_MATH_FUNCTIONAL_
