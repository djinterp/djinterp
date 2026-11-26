/*******************************************************************************
* djinterp [cpp][math]                                            elementary.hpp
*
* Elementary functions, identities, and scalar helpers.
*   Two layers:
*
*   1. AST combinators (djinterp::math) that extend the expression vocabulary
*      of expression.hpp. Two new primitive operations are introduced - the
*      natural logarithm (fn_log) and the arctangent (fn_atan) - each the C
*      core's `_c` kernel, d_math_log_c and d_math_atan_c, exactly as
*      fn_exp and fn_sin are expression.hpp's (N3). Everything else (sec,
*      csc, cot, sinh, cosh, tanh, asin, acos, pow, cbrt, exp2, log2, log10)
*      is composed from the existing primitives, so each one differentiates
*      automatically through differentiation.hpp.
*
*   2. Scalar helpers (djinterp::math::fn) for direct numeric use: the
*      transcendentals above evaluated at a point, plus the algebraic and
*      combinatorial functions (abs, sign, floor, ceil, round, clamp, lerp,
*      radians, degrees, factorial, binomial, gcd, lcm, horner, ...).
*
* LEVELS (step 31, 5c)
*   C++11 is the floor, as for expression.hpp, and the header is empty below
* it. From C++11 every combinator is as above and builds its tree as a
* constant expression; log and atan evaluate by the C core's kernels, d_math_
* log_c and d_math_atan_c, constant expressions from C++14 where those
* kernels are, and the same calls at run time at every level, so that one
* call has one answer at every level, as expression.hpp's exp, sin, cos, tan
* and sqrt have since step 29 (S430). Where this header's own kernels took
* the logarithm by dividing by e as many times as the argument's size asked,
* and the C library's at run time from C++20 only, the logarithm is now the
* double family's correctly rounded one. On a 32-bit x87, where double is
* evaluated wider, log refuses to compile, as exp does (S418, S426). Of the
* scalar helpers, the transcendentals are constant expressions from C++14,
* with the kernels they call; abs, sign, floor, ceil, trunc, round, fract,
* clamp, saturate, lerp, radians, degrees, factorial, gcd and lcm from
* C++11, each one return; binomial and horner from C++14, the first looping
* as many times as its argument says and the second reading a std::array,
* whose operator[] is a constant expression from C++14. The constants are
* constants::of<double>'s, one spelling at every level.
*
*
* path:      /inc/djinterp/cpp/modules/math/calculus/elementary.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.06.20
*                                                            revised: 2026.11.24
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  NEW PRIMITIVE OPERATION FUNCTORS (INTERNAL)
    -------------------------------------------
2.  AST COMBINATORS
    ---------------
3.  SCALAR HELPERS
    --------------
*/

#ifndef DJINTERP_MATH_CALCULUS_ELEMENTARY_HPP
#define DJINTERP_MATH_CALCULUS_ELEMENTARY_HPP 1

// djinterp
#include "../../../djinterp.hpp"

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <array>
#include <cstddef>
#include <type_traits>
// djinterp
#include "../../../../c/modules/math/exponential.h"  // d_math_log_c
#include "../../../../c/modules/math/tangent.h"      // d_math_atan_c
#include "../expression.hpp"
#include "../constants.hpp"


NS_DJINTERP
NS_MATH

//==============================================================================
// 1.  NEW PRIMITIVE OPERATION FUNCTORS (INTERNAL)
//==============================================================================

NS_INTERNAL

    // fn_log, fn_atan
    //   the C core's kernels, as expression.hpp's fn_exp and fn_sin: constant
    // expressions from C++14, where the kernels are (D_MATH_FN), and run-time
    // calls of the same kernels below it.
    struct fn_log
    {
        template<typename X>
        static D_CONSTEXPR_CPP14
        double apply(
            X _x
        )
        {
            return d_math_log_c(static_cast<double>(_x));
        }
    };

    struct fn_atan
    {
        template<typename X>
        static D_CONSTEXPR_CPP14
        double apply(
            X _x
        )
        {
            return d_math_atan_c(static_cast<double>(_x));
        }
    };

    // calc_factorial_of
    //   the product _lo * ... * _hi, halved until each half is one factor,
    // so that the recursion is as deep as the bits of _hi: its value modulo
    // 2^64, as the loop's was, a product of unsigned factors being the same
    // in any order.
    inline D_CONSTEXPR
    unsigned long long calc_factorial_of(
        unsigned long long _lo,
        unsigned long long _hi
    )
    {
        return (_lo > _hi)  ? 1ull
             : (_lo == _hi) ? _lo
             : ( calc_factorial_of(_lo, _lo + (_hi - _lo) / 2)
               * calc_factorial_of(_lo + (_hi - _lo) / 2 + 1, _hi) );
    }

    // calc_gcd_of
    //   Euclid's steps on two magnitudes, as the loop took them: as many as
    // a type's Fibonacci numbers allow, 92 for 64 bits.
    template<typename T>
    D_CONSTEXPR
    T calc_gcd_of(
        T _a,
        T _b
    )
    {
        return (_b != 0) ? calc_gcd_of(_b, static_cast<T>(_a % _b)) : _a;
    }

NS_END  // internal


//==============================================================================
// 2.  AST COMBINATORS
//==============================================================================
// extend the expression vocabulary.

// log / ln  - natural logarithm (new primitive)
template<typename A>
D_CONSTEXPR
unary_node<internal::fn_log, typename internal::expr_of<A>::type>
log(
    A _a
)
{
    return unary_node<internal::fn_log,
                      typename internal::expr_of<A>::type>(
        internal::as_expr(_a));
}

template<typename A>
D_CONSTEXPR
auto ln(
    A _a
) -> decltype(log(_a))
{
    return log(_a);
}

// atan  - arctangent (new primitive)
template<typename A>
D_CONSTEXPR
unary_node<internal::fn_atan, typename internal::expr_of<A>::type>
atan(
    A _a
)
{
    return unary_node<internal::fn_atan,
                      typename internal::expr_of<A>::type>(
        internal::as_expr(_a));
}

// log2 / log10  - change of base
template<typename A>
D_CONSTEXPR
auto log2(
    A _a
) -> decltype(log(_a) / constant(constants::of<double>::ln2()))
{
    return log(_a) / constant(constants::of<double>::ln2());
}

template<typename A>
D_CONSTEXPR
auto log10(
    A _a
) -> decltype(log(_a) / constant(constants::of<double>::ln10()))
{
    return log(_a) / constant(constants::of<double>::ln10());
}

// reciprocal trig  - sec, csc, cot
template<typename A>
D_CONSTEXPR
auto sec(
    A _a
) -> decltype(constant(1.0) / cos(_a))
{
    return constant(1.0) / cos(_a);
}

template<typename A>
D_CONSTEXPR
auto csc(
    A _a
) -> decltype(constant(1.0) / sin(_a))
{
    return constant(1.0) / sin(_a);
}

template<typename A>
D_CONSTEXPR
auto cot(
    A _a
) -> decltype(cos(internal::as_expr(_a)) / sin(internal::as_expr(_a)))
{
    return cos(internal::as_expr(_a)) / sin(internal::as_expr(_a));
}

// hyperbolic  - sinh, cosh, tanh  (composed from exp)
template<typename A>
D_CONSTEXPR
auto sinh(
    A _a
) -> decltype((exp(internal::as_expr(_a)) - exp(-internal::as_expr(_a)))
              / constant(2.0))
{
    return (exp(internal::as_expr(_a)) - exp(-internal::as_expr(_a)))
         / constant(2.0);
}

template<typename A>
D_CONSTEXPR
auto cosh(
    A _a
) -> decltype((exp(internal::as_expr(_a)) + exp(-internal::as_expr(_a)))
              / constant(2.0))
{
    return (exp(internal::as_expr(_a)) + exp(-internal::as_expr(_a)))
         / constant(2.0);
}

template<typename A>
D_CONSTEXPR
auto tanh(
    A _a
) -> decltype((exp(internal::as_expr(_a)) - exp(-internal::as_expr(_a)))
              / (exp(internal::as_expr(_a)) + exp(-internal::as_expr(_a))))
{
    return (exp(internal::as_expr(_a)) - exp(-internal::as_expr(_a)))
         / (exp(internal::as_expr(_a)) + exp(-internal::as_expr(_a)));
}

// inverse trig  - asin, acos  (composed from atan and sqrt)
template<typename A>
D_CONSTEXPR
auto asin(
    A _a
) -> decltype(atan(internal::as_expr(_a)
                   / sqrt(constant(1.0)
                          - internal::as_expr(_a) * internal::as_expr(_a))))
{
    return atan(internal::as_expr(_a)
                / sqrt(constant(1.0)
                       - internal::as_expr(_a) * internal::as_expr(_a)));
}

template<typename A>
D_CONSTEXPR
auto acos(
    A _a
) -> decltype(constant(constants::of<double>::half_pi())
              - asin(internal::as_expr(_a)))
{
    return constant(constants::of<double>::half_pi())
         - asin(internal::as_expr(_a));
}

// powers  - pow, cbrt, exp2  (composed from exp and log)
template<typename Base, typename Exp>
D_CONSTEXPR
auto pow(
    Base _b,
    Exp  _e
) -> decltype(exp(internal::as_expr(_e) * log(internal::as_expr(_b))))
{
    return exp(internal::as_expr(_e) * log(internal::as_expr(_b)));
}

template<typename A>
D_CONSTEXPR
auto cbrt(
    A _a
) -> decltype(pow(_a, constant(1.0 / 3.0)))
{
    return pow(_a, constant(1.0 / 3.0));
}

template<typename A>
D_CONSTEXPR
auto exp2(
    A _a
) -> decltype(exp(internal::as_expr(_a)
                  * constant(constants::of<double>::ln2())))
{
    return exp(internal::as_expr(_a) * constant(constants::of<double>::ln2()));
}


//==============================================================================
// 3.  SCALAR HELPERS
//==============================================================================
// djinterp::math::fn.

namespace fn
{
    // ---- transcendentals: evaluate the AST combinators at a point ----------
    // (constant expressions from C++14, where the C core's kernels are: sin,
    //  cos, tan, exp and sqrt expression.hpp's, log and atan this header's,
    //  each the C core's `_c` kernel at compile time and at run time alike.)

    inline D_CONSTEXPR_CPP14 double sin (double _x) { return ::djinterp::math::sin (constant(_x))(); }
    inline D_CONSTEXPR_CPP14 double cos (double _x) { return ::djinterp::math::cos (constant(_x))(); }
    inline D_CONSTEXPR_CPP14 double tan (double _x) { return ::djinterp::math::tan (constant(_x))(); }
    inline D_CONSTEXPR_CPP14 double exp (double _x) { return ::djinterp::math::exp (constant(_x))(); }
    inline D_CONSTEXPR_CPP14 double sqrt(double _x) { return ::djinterp::math::sqrt(constant(_x))(); }
    inline D_CONSTEXPR_CPP14 double log (double _x) { return ::djinterp::math::log (constant(_x))(); }
    inline D_CONSTEXPR_CPP14 double atan(double _x) { return ::djinterp::math::atan(constant(_x))(); }
    inline D_CONSTEXPR_CPP14 double log2 (double _x) { return ::djinterp::math::log2 (constant(_x))(); }
    inline D_CONSTEXPR_CPP14 double log10(double _x) { return ::djinterp::math::log10(constant(_x))(); }
    inline D_CONSTEXPR_CPP14 double sec (double _x) { return ::djinterp::math::sec (constant(_x))(); }
    inline D_CONSTEXPR_CPP14 double csc (double _x) { return ::djinterp::math::csc (constant(_x))(); }
    inline D_CONSTEXPR_CPP14 double cot (double _x) { return ::djinterp::math::cot (constant(_x))(); }
    inline D_CONSTEXPR_CPP14 double sinh(double _x) { return ::djinterp::math::sinh(constant(_x))(); }
    inline D_CONSTEXPR_CPP14 double cosh(double _x) { return ::djinterp::math::cosh(constant(_x))(); }
    inline D_CONSTEXPR_CPP14 double tanh(double _x) { return ::djinterp::math::tanh(constant(_x))(); }
    inline D_CONSTEXPR_CPP14 double asin(double _x) { return ::djinterp::math::asin(constant(_x))(); }
    inline D_CONSTEXPR_CPP14 double acos(double _x) { return ::djinterp::math::acos(constant(_x))(); }
    inline D_CONSTEXPR_CPP14 double cbrt(double _x) { return ::djinterp::math::cbrt(constant(_x))(); }
    inline D_CONSTEXPR_CPP14 double exp2(double _x) { return ::djinterp::math::exp2(constant(_x))(); }

    inline D_CONSTEXPR_CPP14 double pow(double _b, double _e)
    { return ::djinterp::math::pow(constant(_b), constant(_e))(); }

    // atan2: full-quadrant arctangent (scalar only).
    inline D_CONSTEXPR_CPP14
    double atan2(
        double _y,
        double _x
    )
    {
        if (_x > 0.0) { return atan(_y / _x); }
        if (_x < 0.0)
        {
            return (_y >= 0.0) ? atan(_y / _x) + constants::of<double>::pi()
                               : atan(_y / _x) - constants::of<double>::pi();
        }
        if (_y > 0.0) { return  constants::of<double>::half_pi(); }
        if (_y < 0.0) { return -constants::of<double>::half_pi(); }
        return 0.0;
    }

    // ---- algebraic / rounding (one return each, from C++11) ----------------

    inline D_CONSTEXPR double abs(double _x)   { return (_x < 0.0) ? -_x : _x; }

    template<typename T>
    D_CONSTEXPR int sign(T _x)
    { return (T(0) < _x) - (_x < T(0)); }

    // floor, ceil: the truncation, one less or one more where it passed _x,
    // as the locals of the C++14 forms had it.
    inline D_CONSTEXPR
    double floor(
        double _x
    )
    {
        return (static_cast<double>(static_cast<long long>(_x)) > _x)
                   ? (static_cast<double>(static_cast<long long>(_x)) - 1.0)
                   : static_cast<double>(static_cast<long long>(_x));
    }

    inline D_CONSTEXPR
    double ceil(
        double _x
    )
    {
        return (static_cast<double>(static_cast<long long>(_x)) < _x)
                   ? (static_cast<double>(static_cast<long long>(_x)) + 1.0)
                   : static_cast<double>(static_cast<long long>(_x));
    }

    inline D_CONSTEXPR double trunc(double _x)
    { return static_cast<double>(static_cast<long long>(_x)); }

    inline D_CONSTEXPR double round(double _x)
    { return (_x >= 0.0) ? floor(_x + 0.5) : ceil(_x - 0.5); }

    inline D_CONSTEXPR double fract(double _x)
    { return _x - floor(_x); }

    template<typename T>
    D_CONSTEXPR T clamp(T _v, T _lo, T _hi)
    { return (_v < _lo) ? _lo : ((_v > _hi) ? _hi : _v); }

    inline D_CONSTEXPR double saturate(double _x)
    { return clamp(_x, 0.0, 1.0); }

    template<typename T>
    D_CONSTEXPR T lerp(T _a, T _b, T _t)
    { return _a + (_b - _a) * _t; }

    inline D_CONSTEXPR double radians(double _deg)
    { return _deg * constants::of<double>::rad_per_deg(); }

    inline D_CONSTEXPR double degrees(double _rad)
    { return _rad * constants::of<double>::deg_per_rad(); }

    // ---- combinatorial / number-theoretic ---------------------------------

    inline D_CONSTEXPR
    unsigned long long factorial(
        unsigned _n
    )
    {
        return internal::calc_factorial_of(
            2ull, static_cast<unsigned long long>(_n));
    }

    inline D_CONSTEXPR_CPP14
    unsigned long long binomial(
        unsigned _n,
        unsigned _k
    )
    {
        if (_k > _n) { return 0; }
        if (_k > _n - _k) { _k = _n - _k; }

        unsigned long long r = 1;
        for (unsigned i = 0; i < _k; ++i)
        {
            r = r * (_n - i) / (i + 1);   // exact at each step for binomials
        }
        return r;
    }

    template<typename T>
    D_CONSTEXPR
    T gcd(
        T _a,
        T _b
    )
    {
        return internal::calc_gcd_of(static_cast<T>((_a < 0) ? -_a : _a),
                              static_cast<T>((_b < 0) ? -_b : _b));
    }

    template<typename T>
    D_CONSTEXPR
    T lcm(
        T _a,
        T _b
    )
    {
        return (_a == 0 || _b == 0) ? T(0) : (_a / gcd(_a, _b)) * _b;
    }

    // horner: evaluate a polynomial whose coefficients are highest-degree
    // first, i.e. c[0]*x^(N-1) + c[1]*x^(N-2) + ... + c[N-1].
    template<std::size_t N, typename T>
    D_CONSTEXPR_CPP14
    T horner(
        const std::array<T, N>&  _coeffs,
        T                        _x
    )
    {
        T acc = T(0);
        for (std::size_t i = 0; i < N; ++i)
        {
            acc = acc * _x + _coeffs[i];
        }
        return acc;
    }
}  // fn

NS_END  // math
NS_END  // djinterp


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_MATH_CALCULUS_ELEMENTARY_HPP
