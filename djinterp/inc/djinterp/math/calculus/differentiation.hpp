/******************************************************************************
* djinterp [math]                                 calculus/differentiation.hpp
*
* Symbolic and numerical differentiation.
*   Symbolic differentiation rewrites the expression AST of expression.hpp
* into a new AST representing the exact derivative -- one overload per
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
* path:      /inc/djinterp/math/calculus/differentiation.hpp
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                       date: 2026.06.20
******************************************************************************/

#ifndef DJINTERP_MATH_CALCULUS_DIFFERENTIATION_
#define DJINTERP_MATH_CALCULUS_DIFFERENTIATION_ 1

#include <cstddef>
#include <array>
#include <utility>

#include "../../djinterp.hpp"
#include "../expression.hpp"
#include "../function.hpp"
#include "./elementary.hpp"


NS_DJINTERP
NS_MATH

// ============================================================================
// I.    SYMBOLIC DERIVATIVE  (one overload per operation)
// ============================================================================

// ---- leaves ----------------------------------------------------------------

// d/dx of a constant is zero.
template<std::size_t _I, typename _T>
D_CONSTEXPR auto
derivative(const constant_node<_T>&)
{
    return constant(0.0);
}

// d/dx_I of x_J is 1 when I == J, else 0.
template<std::size_t _I, std::size_t _J, typename _T>
D_CONSTEXPR auto
derivative(const variable_node<_J, _T>&)
{
    return constant((_I == _J) ? 1.0 : 0.0);
}

// ---- binary operations -----------------------------------------------------

template<std::size_t _I, typename _L, typename _R>
D_CONSTEXPR auto
derivative(const binary_node<internal::op_add, _L, _R>& _n)
{
    return derivative<_I>(_n.left) + derivative<_I>(_n.right);
}

template<std::size_t _I, typename _L, typename _R>
D_CONSTEXPR auto
derivative(const binary_node<internal::op_sub, _L, _R>& _n)
{
    return derivative<_I>(_n.left) - derivative<_I>(_n.right);
}

// product rule:  (uv)' = u'v + uv'
template<std::size_t _I, typename _L, typename _R>
D_CONSTEXPR auto
derivative(const binary_node<internal::op_mul, _L, _R>& _n)
{
    return derivative<_I>(_n.left) * _n.right
         + _n.left * derivative<_I>(_n.right);
}

// quotient rule:  (u/v)' = (u'v - uv') / v^2
template<std::size_t _I, typename _L, typename _R>
D_CONSTEXPR auto
derivative(const binary_node<internal::op_div, _L, _R>& _n)
{
    return ( derivative<_I>(_n.left) * _n.right
           - _n.left * derivative<_I>(_n.right) )
         / (_n.right * _n.right);
}

// ---- unary operations (chain rule:  f(u)' = f'(u) * u') ---------------------

template<std::size_t _I, typename _A>
D_CONSTEXPR auto
derivative(const unary_node<internal::op_neg, _A>& _n)
{
    return -derivative<_I>(_n.arg);
}

template<std::size_t _I, typename _A>
D_CONSTEXPR auto
derivative(const unary_node<internal::fn_exp, _A>& _n)
{
    return exp(_n.arg) * derivative<_I>(_n.arg);
}

template<std::size_t _I, typename _A>
D_CONSTEXPR auto
derivative(const unary_node<internal::fn_sin, _A>& _n)
{
    return cos(_n.arg) * derivative<_I>(_n.arg);
}

template<std::size_t _I, typename _A>
D_CONSTEXPR auto
derivative(const unary_node<internal::fn_cos, _A>& _n)
{
    return -sin(_n.arg) * derivative<_I>(_n.arg);
}

// d/dx tan(u) = u' / cos(u)^2
template<std::size_t _I, typename _A>
D_CONSTEXPR auto
derivative(const unary_node<internal::fn_tan, _A>& _n)
{
    return derivative<_I>(_n.arg) / squared(cos(_n.arg));
}

// d/dx sqrt(u) = u' / (2 sqrt(u))
template<std::size_t _I, typename _A>
D_CONSTEXPR auto
derivative(const unary_node<internal::fn_sqrt, _A>& _n)
{
    return derivative<_I>(_n.arg) / (constant(2.0) * sqrt(_n.arg));
}

// d/dx |u| = (u / |u|) u'
template<std::size_t _I, typename _A>
D_CONSTEXPR auto
derivative(const unary_node<internal::fn_abs, _A>& _n)
{
    return (_n.arg / abs(_n.arg)) * derivative<_I>(_n.arg);
}

// d/dx ln(u) = u' / u
template<std::size_t _I, typename _A>
D_CONSTEXPR auto
derivative(const unary_node<internal::fn_log, _A>& _n)
{
    return derivative<_I>(_n.arg) / _n.arg;
}

// d/dx atan(u) = u' / (1 + u^2)
template<std::size_t _I, typename _A>
D_CONSTEXPR auto
derivative(const unary_node<internal::fn_atan, _A>& _n)
{
    return derivative<_I>(_n.arg) / (constant(1.0) + _n.arg * _n.arg);
}


// ============================================================================
// II.   HIGHER-ORDER DERIVATIVES
// ============================================================================

NS_INTERNAL

    template<std::size_t _I, std::size_t _N>
    struct nth_deriv_helper
    {
        template<typename _E>
        static D_CONSTEXPR auto apply(const _E& _e)
        {
            return nth_deriv_helper<_I, _N - 1>::apply(derivative<_I>(_e));
        }
    };

    template<std::size_t _I>
    struct nth_deriv_helper<_I, 0>
    {
        template<typename _E>
        static D_CONSTEXPR auto apply(const _E& _e)
        {
            return _e;
        }
    };

NS_END  // internal

// nth_derivative<I, N>(e): the N-th partial derivative wrt argument I.
template<std::size_t _I, std::size_t _N, typename _E>
D_CONSTEXPR auto
nth_derivative(const _E& _e)
{
    return internal::nth_deriv_helper<_I, _N>::apply(_e);
}

// second_derivative<I>(e): convenience for N = 2.
template<std::size_t _I, typename _E>
D_CONSTEXPR auto
second_derivative(const _E& _e)
{
    return nth_derivative<_I, 2>(_e);
}


// ============================================================================
// III.  GRADIENT
// ============================================================================

NS_INTERNAL

    template<typename _E, std::size_t... _Is>
    D_CONSTEXPR auto
    gradient_impl(const _E& _e, std::index_sequence<_Is...>)
    {
        return vec(derivative<_Is>(_e)...);
    }

NS_END  // internal

// gradient<N>(e): a vector_node whose components are the first N partials.
template<std::size_t _N, typename _E>
D_CONSTEXPR auto
gradient(const _E& _e)
{
    return internal::gradient_impl(_e, std::make_index_sequence<_N>{});
}


// ============================================================================
// IV.   NUMERICAL DIFFERENTIATION
// ============================================================================

NS_INTERNAL

    template<typename _E, std::size_t _N, std::size_t... _Is>
    D_CONSTEXPR double
    eval_point(const _E& _e, const std::array<double, _N>& _p,
               std::index_sequence<_Is...>)
    {
        return static_cast<double>(_e(_p[_Is]...));
    }

NS_END  // internal

// numeric_derivative(f, x): central-difference derivative of a unary callable.
template<typename _F>
D_CONSTEXPR double
numeric_derivative(_F _f, double _x, double _h = 1e-6)
{
    return (_f(_x + _h) - _f(_x - _h)) / (2.0 * _h);
}

// numeric_partial<I>(e, point): central-difference partial of an expression
// evaluated at a coordinate point (a std::array of arguments).
template<std::size_t _I, typename _E, std::size_t _N>
D_CONSTEXPR double
numeric_partial(const _E& _e, std::array<double, _N> _point, double _h = 1e-6)
{
    std::array<double, _N> hi = _point;
    std::array<double, _N> lo = _point;
    hi[_I] += _h;
    lo[_I] -= _h;

    return ( internal::eval_point(_e, hi, std::make_index_sequence<_N>{})
           - internal::eval_point(_e, lo, std::make_index_sequence<_N>{}) )
         / (2.0 * _h);
}

NS_END  // math
NS_END  // djinterp


#endif  // DJINTERP_MATH_CALCULUS_DIFFERENTIATION_
