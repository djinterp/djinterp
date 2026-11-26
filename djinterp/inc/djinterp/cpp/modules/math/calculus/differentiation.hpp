/*******************************************************************************
* djinterp [cpp][math]                                       differentiation.hpp
*
* Symbolic and numerical differentiation.
*   Symbolic differentiation rewrites the expression AST of expression.hpp
* into a new AST representing the exact derivative - one overload per
* operation, applying the sum, product, quotient and chain rules. Because the
* result is itself an expression, it evaluates at compile time or run time and
* can be differentiated again.
*
*   derivative<I>(e)        - partial derivative wrt argument I (exact)
*   nth_derivative<I,N>(e)  - the N-th partial derivative wrt argument I
*   second_derivative<I>(e) - convenience for N = 2
*   gradient<N>(e)          - vector of the first N partials (a vector_node)
*   numeric_derivative(f,x) - central-difference derivative of a callable
*   numeric_partial<I>(e,p) - central-difference partial of e at a point
*
* Supported operations: + - * / and unary -, exp, sin, cos, tan, sqrt, abs,
* log, atan. Everything composed from these (sec, sinh, asin, pow, ...) is
* differentiated automatically.
*
* LEVELS (step 31, 5c)
*   C++11 is the floor, as for expression.hpp, and the header is empty below
* it. From C++11 each derivative, the higher derivatives and the gradient
* build their trees as constant expressions, each overload one return and
* its result the type of that return (-> decltype), so that a new node's
* overload still joins by argument-dependent lookup; a derivative evaluates
* as a constant expression wherever its tree does (a polynomial from C++11;
* exp, sin, cos, tan, sqrt, log and atan from C++14, where the C core's
* kernels are). numeric_derivative is a constant expression from C++11 with
* its callable; numeric_partial from C++14, where std::array's operator[]
* is. The index sequences are re_std's.
*
* HOW TO USE
* ----------
*   Include it, or calculus.hpp, from C++11. derivative<I>(e) is e's exact
* partial derivative by argument I, itself an expression to evaluate, print
* through reify or differentiate again; nth_derivative, gradient and
* numeric_partial are beside it.
*
* OPTIONS
* -------
*   The numerical rules take their step as an argument; the symbolic ones
* choose nothing at run time.
*
* CONFIGURATION
* -------------
*   None of its own: it compiles from C++11 and is empty below it (N1).
*
*
* path:      /inc/djinterp/cpp/modules/math/calculus/differentiation.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.06.20
*                                                            revised: 2026.11.24
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  SYMBOLIC DERIVATIVE
    -------------------
2.  HIGHER-ORDER DERIVATIVES
    ------------------------
3.  GRADIENT
    --------
4.  NUMERICAL DIFFERENTIATION
    -------------------------
*/

#ifndef DJINTERP_MATH_CALCULUS_DIFFERENTIATION_HPP
#define DJINTERP_MATH_CALCULUS_DIFFERENTIATION_HPP 1

// djinterp
#include "../../../djinterp.hpp"  // framework root

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <array>
#include <cstddef>
// djinterp
#include "../expression.hpp"
#include "../function/function.hpp"
#include "./elementary.hpp"
// re_std
#include "../../../../../re_std/cpp/utility/integer_sequence.hpp"       // index_
                                                                        // sequence
#include "../../../../../re_std/cpp/utility/make_integer_sequence.hpp"  // make_index_
                                                                        // sequence


NS_DJINTERP
NS_MATH

//==============================================================================
// 1.  SYMBOLIC DERIVATIVE
//==============================================================================
// one overload per operation.

// wrt
//   tag: "with respect to argument I", each rule's first parameter. A rule
// is the overload derivative(wrt<I>, const node&) of its node's type, found
// by argument-dependent lookup; its recursive calls name no template
// argument, so that each rule's type (-> decltype) is deduced from its
// arguments alone, as C++11 needs. derivative<I>(e) below is the spelling
// to call.
template<std::size_t I>
struct wrt
{
    static constexpr std::size_t index = I;
};

// ---- leaves ----------------------------------------------------------------

// d/dx of a constant is zero.
template<std::size_t I, typename T>
D_CONSTEXPR
constant_node<double> derivative(
    wrt<I>,
    const constant_node<T>&
)
{
    return constant(0.0);
}

// d/dx_I of x_J is 1 when I == J, else 0.
template<std::size_t I, std::size_t J, typename T>
D_CONSTEXPR
constant_node<double> derivative(
    wrt<I>,
    const variable_node<J, T>&
)
{
    return constant((I == J) ? 1.0 : 0.0);
}

// ---- binary operations -----------------------------------------------------

template<std::size_t I, typename L, typename R>
D_CONSTEXPR
auto derivative(
    wrt<I> _w,
    const binary_node<internal::op_add, L, R>& _n
) -> decltype(derivative(_w, _n.left) + derivative(_w, _n.right))
{
    return derivative(_w, _n.left) + derivative(_w, _n.right);
}

template<std::size_t I, typename L, typename R>
D_CONSTEXPR
auto derivative(
    wrt<I> _w,
    const binary_node<internal::op_sub, L, R>& _n
) -> decltype(derivative(_w, _n.left) - derivative(_w, _n.right))
{
    return derivative(_w, _n.left) - derivative(_w, _n.right);
}

// product rule:  (uv)' = u'v + uv'
template<std::size_t I, typename L, typename R>
D_CONSTEXPR
auto derivative(
    wrt<I> _w,
    const binary_node<internal::op_mul, L, R>& _n
) -> decltype(derivative(_w, _n.left) * _n.right
            + _n.left * derivative(_w, _n.right))
{
    return derivative(_w, _n.left) * _n.right
         + _n.left * derivative(_w, _n.right);
}

// quotient rule:  (u/v)' = (u'v - uv') / v^2
template<std::size_t I, typename L, typename R>
D_CONSTEXPR
auto derivative(
    wrt<I> _w,
    const binary_node<internal::op_div, L, R>& _n
) -> decltype(( derivative(_w, _n.left) * _n.right
              - _n.left * derivative(_w, _n.right) )
            / (_n.right * _n.right))
{
    return ( derivative(_w, _n.left) * _n.right
           - _n.left * derivative(_w, _n.right) )
         / (_n.right * _n.right);
}

// ---- unary operations (chain rule:  f(u)' = f'(u) * u') ---------------------

template<std::size_t I, typename A>
D_CONSTEXPR
auto derivative(
    wrt<I> _w,
    const unary_node<internal::op_neg, A>& _n
) -> decltype(-derivative(_w, _n.arg))
{
    return -derivative(_w, _n.arg);
}

template<std::size_t I, typename A>
D_CONSTEXPR
auto derivative(
    wrt<I> _w,
    const unary_node<internal::fn_exp, A>& _n
) -> decltype(exp(_n.arg) * derivative(_w, _n.arg))
{
    return exp(_n.arg) * derivative(_w, _n.arg);
}

template<std::size_t I, typename A>
D_CONSTEXPR
auto derivative(
    wrt<I> _w,
    const unary_node<internal::fn_sin, A>& _n
) -> decltype(cos(_n.arg) * derivative(_w, _n.arg))
{
    return cos(_n.arg) * derivative(_w, _n.arg);
}

template<std::size_t I, typename A>
D_CONSTEXPR
auto derivative(
    wrt<I> _w,
    const unary_node<internal::fn_cos, A>& _n
) -> decltype(-sin(_n.arg) * derivative(_w, _n.arg))
{
    return -sin(_n.arg) * derivative(_w, _n.arg);
}

// d/dx tan(u) = u' / cos(u)^2
template<std::size_t I, typename A>
D_CONSTEXPR
auto derivative(
    wrt<I> _w,
    const unary_node<internal::fn_tan, A>& _n
) -> decltype(derivative(_w, _n.arg) / squared(cos(_n.arg)))
{
    return derivative(_w, _n.arg) / squared(cos(_n.arg));
}

// d/dx sqrt(u) = u' / (2 sqrt(u))
template<std::size_t I, typename A>
D_CONSTEXPR
auto derivative(
    wrt<I> _w,
    const unary_node<internal::fn_sqrt, A>& _n
) -> decltype(derivative(_w, _n.arg) / (constant(2.0) * sqrt(_n.arg)))
{
    return derivative(_w, _n.arg) / (constant(2.0) * sqrt(_n.arg));
}

// d/dx |u| = (u / |u|) u'
template<std::size_t I, typename A>
D_CONSTEXPR
auto derivative(
    wrt<I> _w,
    const unary_node<internal::fn_abs, A>& _n
) -> decltype((_n.arg / abs(_n.arg)) * derivative(_w, _n.arg))
{
    return (_n.arg / abs(_n.arg)) * derivative(_w, _n.arg);
}

// d/dx ln(u) = u' / u
template<std::size_t I, typename A>
D_CONSTEXPR
auto derivative(
    wrt<I> _w,
    const unary_node<internal::fn_log, A>& _n
) -> decltype(derivative(_w, _n.arg) / _n.arg)
{
    return derivative(_w, _n.arg) / _n.arg;
}

// d/dx atan(u) = u' / (1 + u^2)
template<std::size_t I, typename A>
D_CONSTEXPR
auto derivative(
    wrt<I> _w,
    const unary_node<internal::fn_atan, A>& _n
) -> decltype(derivative(_w, _n.arg) / (constant(1.0) + _n.arg * _n.arg))
{
    return derivative(_w, _n.arg) / (constant(1.0) + _n.arg * _n.arg);
}


// ---- the spelling to call ----------------------------------------------------

// derivative<I>(e): the partial derivative of e with respect to argument I,
// by the rules above.
template<std::size_t I, typename E>
D_CONSTEXPR
auto derivative(
    const E& _e
) -> decltype(derivative(wrt<I>(), _e))
{
    return derivative(wrt<I>(), _e);
}


//==============================================================================
// 2.  HIGHER-ORDER DERIVATIVES
//==============================================================================

NS_INTERNAL

    template<std::size_t I, std::size_t N>
    struct nth_deriv_helper
    {
        template<typename E>
        static D_CONSTEXPR
        auto apply(
            const E& _e
        ) -> decltype(nth_deriv_helper<I, N - 1>::apply(
                          derivative(wrt<I>(), _e)))
        {
            return nth_deriv_helper<I, N - 1>::apply(
                derivative(wrt<I>(), _e));
        }
    };

    template<std::size_t I>
    struct nth_deriv_helper<I, 0>
    {
        template<typename E>
        static D_CONSTEXPR
        E apply(
            const E& _e
        )
        {
            return _e;
        }
    };

NS_END  // internal

// nth_derivative<I, N>(e): the N-th partial derivative wrt argument I.
template<std::size_t I, std::size_t N, typename E>
D_CONSTEXPR
auto nth_derivative(
    const E& _e
) -> decltype(internal::nth_deriv_helper<I, N>::apply(_e))
{
    return internal::nth_deriv_helper<I, N>::apply(_e);
}

// second_derivative<I>(e): convenience for N = 2.
template<std::size_t I, typename E>
D_CONSTEXPR
auto second_derivative(
    const E& _e
) -> decltype(nth_derivative<I, 2>(_e))
{
    return nth_derivative<I, 2>(_e);
}


//==============================================================================
// 3.  GRADIENT
//==============================================================================

NS_INTERNAL

    template<typename E, std::size_t... Is>
    D_CONSTEXPR
    auto gradient_impl(
        const E& _e,
        re_std::index_sequence<Is...>
    ) -> decltype(vec(derivative(wrt<Is>(), _e)...))
    {
        return vec(derivative(wrt<Is>(), _e)...);
    }

NS_END  // internal

// gradient<N>(e): a vector_node whose components are the first N partials.
template<std::size_t N, typename E>
D_CONSTEXPR
auto gradient(
    const E& _e
) -> decltype(internal::gradient_impl(_e, re_std::make_index_sequence<N>{}))
{
    return internal::gradient_impl(_e, re_std::make_index_sequence<N>{});
}


//==============================================================================
// 4.  NUMERICAL DIFFERENTIATION
//==============================================================================

NS_INTERNAL

    // eval_shifted
    //   _e at _p with its I-th coordinate moved _h up (or down), the
    // others as they are: one return over std::array's const operator[], a
    // constant expression from C++14, where the copy and += numeric_partial
    // made would need C++17's non-const operator[].
    template<std::size_t I, typename E, std::size_t N, std::size_t... Is>
    D_CONSTEXPR_CPP14
    double eval_shifted(
        const E&                      _e,
        const std::array<double, N>&  _p,
        double                        _h,
        bool                          _up,
        re_std::index_sequence<Is...>
    )
    {
        return static_cast<double>(
            _e(((Is == I) ? (_up ? (_p[Is] + _h) : (_p[Is] - _h))
                            : _p[Is])...));
    }

NS_END  // internal

// numeric_derivative(f, x): central-difference derivative of a unary callable.
template<typename F>
D_CONSTEXPR
double numeric_derivative(
    F      _f,
    double _x,
    double _h = 1e-6
)
{
    return (_f(_x + _h) - _f(_x - _h)) / (2.0 * _h);
}

// numeric_partial<I>(e, point): central-difference partial of an expression
// evaluated at a coordinate point (a std::array of arguments).
template<std::size_t I, typename E, std::size_t N>
D_CONSTEXPR_CPP14
double numeric_partial(
    const E&               _e,
    std::array<double, N>  _point,
    double                 _h = 1e-6
)
{
    return ( internal::eval_shifted<I>(_e, _point, _h, true,
                                        re_std::make_index_sequence<N>{})
           - internal::eval_shifted<I>(_e, _point, _h, false,
                                        re_std::make_index_sequence<N>{}) )
         / (2.0 * _h);
}

NS_END  // math
NS_END  // djinterp


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_MATH_CALCULUS_DIFFERENTIATION_HPP
