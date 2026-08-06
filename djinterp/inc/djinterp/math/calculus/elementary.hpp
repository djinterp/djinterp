/******************************************************************************
* djinterp [math]                                     calculus/elementary.hpp
*
* Elementary functions, identities, and scalar helpers.
*   Two layers:
*
*   1. AST combinators (djinterp::math) that extend the expression vocabulary
*      of expression.hpp. Two new primitive operations are introduced -- the
*      natural logarithm (fn_log) and the arctangent (fn_atan) -- each with a
*      constexpr kernel and a libm fast-path, exactly mirroring fn_exp/fn_sin.
*      Everything else (sec, csc, cot, sinh, cosh, tanh, asin, acos, pow,
*      cbrt, exp2, log2, log10) is composed from the existing primitives, so
*      each one differentiates automatically through differentiation.hpp.
*
*   2. Scalar helpers (djinterp::math::fn) for direct numeric use: the
*      transcendentals above evaluated at a point, plus the algebraic and
*      combinatorial functions (abs, sign, floor, ceil, round, clamp, lerp,
*      radians, degrees, factorial, binomial, gcd, lcm, horner, ...).
*
* path:      /inc/djinterp/math/calculus/elementary.hpp
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                       date: 2026.06.20
******************************************************************************/

#ifndef DJINTERP_MATH_CALCULUS_ELEMENTARY_
#define DJINTERP_MATH_CALCULUS_ELEMENTARY_ 1

#include <cstddef>
#include <cmath>
#include <array>
#include <type_traits>

#include "../../djinterp.hpp"
#include "../expression.hpp"
#include "./constants.hpp"


NS_DJINTERP
NS_MATH

// ============================================================================
// I.    NEW PRIMITIVE KERNELS + OPERATION FUNCTORS (internal)
// ============================================================================

NS_INTERNAL

    // clog: natural logarithm. Range-reduce by factors of e into ~[2/3, 3/2],
    // then the fast atanh series  ln(x) = 2 * sum_{k>=0} y^(2k+1)/(2k+1),
    // y = (x-1)/(x+1).
    inline D_CONSTEXPR double
    clog(double _x) noexcept
    {
        if (_x <= 0.0)
        {
            return 0.0;   // domain error; caller's responsibility
        }

        const double E = 2.718281828459045235360287;
        int          k = 0;

        while (_x > 1.5)    { _x /= E; ++k; }
        while (_x < 0.6667) { _x *= E; --k; }

        const double y  = (_x - 1.0) / (_x + 1.0);
        const double y2 = y * y;
        double       term = y;
        double       sum  = y;

        for (int n = 1; n < 24; ++n)
        {
            term *= y2;
            sum  += term / static_cast<double>(2 * n + 1);
        }

        return 2.0 * sum + static_cast<double>(k);
    }

    // catan: arctangent. Fold to x>=0 and x<=1 via sign and reciprocal
    // identities, halve the argument twice (atan x = 2 atan(x/(1+sqrt(1+x^2))))
    // to accelerate convergence, then the Maclaurin series.
    inline D_CONSTEXPR double
    catan(double _x) noexcept
    {
        const double half_pi = 1.570796326794896619231322;
        bool         neg     = false;
        bool         recip   = false;

        if (_x < 0.0) { _x = -_x; neg = true; }
        if (_x > 1.0) { _x = 1.0 / _x; recip = true; }

        _x = _x / (1.0 + csqrt(1.0 + _x * _x));
        _x = _x / (1.0 + csqrt(1.0 + _x * _x));

        const double x2 = _x * _x;
        double       term = _x;
        double       sum  = _x;

        for (int n = 1; n < 16; ++n)
        {
            term *= -x2;
            sum  += term / static_cast<double>(2 * n + 1);
        }

        double result = 4.0 * sum;          // undo the two halvings

        if (recip) { result = half_pi - result; }

        return neg ? -result : result;
    }

#if D_ENV_CPP_FEATURE_STL_IS_CONSTANT_EVALUATED
    #define D_CALC_DISPATCH(KERNEL, STDFN, ARG) \
        (std::is_constant_evaluated() ? KERNEL(ARG) : STDFN(ARG))
#else
    #define D_CALC_DISPATCH(KERNEL, STDFN, ARG) (KERNEL(ARG))
#endif

    struct fn_log
    {
        template<typename _X>
        static D_CONSTEXPR double apply(_X _x)
        {
            return D_CALC_DISPATCH(clog, std::log, static_cast<double>(_x));
        }
    };

    struct fn_atan
    {
        template<typename _X>
        static D_CONSTEXPR double apply(_X _x)
        {
            return D_CALC_DISPATCH(catan, std::atan, static_cast<double>(_x));
        }
    };

#undef D_CALC_DISPATCH

NS_END  // internal


// ============================================================================
// II.   AST COMBINATORS  (extend the expression vocabulary)
// ============================================================================

// log / ln  -- natural logarithm (new primitive)
template<typename _A>
D_CONSTEXPR auto log(_A _a)
{
    auto e = internal::as_expr(_a);
    return unary_node<internal::fn_log, decltype(e)>(e);
}

template<typename _A>
D_CONSTEXPR auto ln(_A _a)
{
    return log(_a);
}

// atan  -- arctangent (new primitive)
template<typename _A>
D_CONSTEXPR auto atan(_A _a)
{
    auto e = internal::as_expr(_a);
    return unary_node<internal::fn_atan, decltype(e)>(e);
}

// log2 / log10  -- change of base
template<typename _A>
D_CONSTEXPR auto log2(_A _a)
{
    return log(_a) / constant(constants::ln2<double>);
}

template<typename _A>
D_CONSTEXPR auto log10(_A _a)
{
    return log(_a) / constant(constants::ln10<double>);
}

// reciprocal trig  -- sec, csc, cot
template<typename _A>
D_CONSTEXPR auto sec(_A _a)
{
    return constant(1.0) / cos(_a);
}

template<typename _A>
D_CONSTEXPR auto csc(_A _a)
{
    return constant(1.0) / sin(_a);
}

template<typename _A>
D_CONSTEXPR auto cot(_A _a)
{
    auto e = internal::as_expr(_a);
    return cos(e) / sin(e);
}

// hyperbolic  -- sinh, cosh, tanh  (composed from exp)
template<typename _A>
D_CONSTEXPR auto sinh(_A _a)
{
    auto e = internal::as_expr(_a);
    return (exp(e) - exp(-e)) / constant(2.0);
}

template<typename _A>
D_CONSTEXPR auto cosh(_A _a)
{
    auto e = internal::as_expr(_a);
    return (exp(e) + exp(-e)) / constant(2.0);
}

template<typename _A>
D_CONSTEXPR auto tanh(_A _a)
{
    auto e = internal::as_expr(_a);
    return (exp(e) - exp(-e)) / (exp(e) + exp(-e));
}

// inverse trig  -- asin, acos  (composed from atan and sqrt)
template<typename _A>
D_CONSTEXPR auto asin(_A _a)
{
    auto e = internal::as_expr(_a);
    return atan(e / sqrt(constant(1.0) - e * e));
}

template<typename _A>
D_CONSTEXPR auto acos(_A _a)
{
    auto e = internal::as_expr(_a);
    return constant(constants::half_pi<double>) - asin(e);
}

// powers  -- pow, cbrt, exp2  (composed from exp and log)
template<typename _Base, typename _Exp>
D_CONSTEXPR auto pow(_Base _b, _Exp _e)
{
    auto base = internal::as_expr(_b);
    auto expo = internal::as_expr(_e);
    return exp(expo * log(base));
}

template<typename _A>
D_CONSTEXPR auto cbrt(_A _a)
{
    return pow(_a, constant(1.0 / 3.0));
}

template<typename _A>
D_CONSTEXPR auto exp2(_A _a)
{
    auto e = internal::as_expr(_a);
    return exp(e * constant(constants::ln2<double>));
}


// ============================================================================
// III.  SCALAR HELPERS  (djinterp::math::fn)
// ============================================================================

namespace fn
{
    // ---- transcendentals: evaluate the AST combinators at a point ----------
    // (constexpr-capable: the kernels run during constant evaluation, libm at
    //  run time, through the same dispatch as the expression layer.)

    inline D_CONSTEXPR double sin (double _x) { return ::djinterp::math::sin (constant(_x))(); }
    inline D_CONSTEXPR double cos (double _x) { return ::djinterp::math::cos (constant(_x))(); }
    inline D_CONSTEXPR double tan (double _x) { return ::djinterp::math::tan (constant(_x))(); }
    inline D_CONSTEXPR double exp (double _x) { return ::djinterp::math::exp (constant(_x))(); }
    inline D_CONSTEXPR double sqrt(double _x) { return ::djinterp::math::sqrt(constant(_x))(); }
    inline D_CONSTEXPR double log (double _x) { return ::djinterp::math::log (constant(_x))(); }
    inline D_CONSTEXPR double atan(double _x) { return ::djinterp::math::atan(constant(_x))(); }
    inline D_CONSTEXPR double log2 (double _x) { return ::djinterp::math::log2 (constant(_x))(); }
    inline D_CONSTEXPR double log10(double _x) { return ::djinterp::math::log10(constant(_x))(); }
    inline D_CONSTEXPR double sec (double _x) { return ::djinterp::math::sec (constant(_x))(); }
    inline D_CONSTEXPR double csc (double _x) { return ::djinterp::math::csc (constant(_x))(); }
    inline D_CONSTEXPR double cot (double _x) { return ::djinterp::math::cot (constant(_x))(); }
    inline D_CONSTEXPR double sinh(double _x) { return ::djinterp::math::sinh(constant(_x))(); }
    inline D_CONSTEXPR double cosh(double _x) { return ::djinterp::math::cosh(constant(_x))(); }
    inline D_CONSTEXPR double tanh(double _x) { return ::djinterp::math::tanh(constant(_x))(); }
    inline D_CONSTEXPR double asin(double _x) { return ::djinterp::math::asin(constant(_x))(); }
    inline D_CONSTEXPR double acos(double _x) { return ::djinterp::math::acos(constant(_x))(); }
    inline D_CONSTEXPR double cbrt(double _x) { return ::djinterp::math::cbrt(constant(_x))(); }
    inline D_CONSTEXPR double exp2(double _x) { return ::djinterp::math::exp2(constant(_x))(); }

    inline D_CONSTEXPR double pow(double _b, double _e)
    { return ::djinterp::math::pow(constant(_b), constant(_e))(); }

    // atan2: full-quadrant arctangent (scalar only).
    inline D_CONSTEXPR double
    atan2(double _y, double _x)
    {
        if (_x > 0.0) { return atan(_y / _x); }
        if (_x < 0.0)
        {
            return (_y >= 0.0) ? atan(_y / _x) + constants::pi<double>
                               : atan(_y / _x) - constants::pi<double>;
        }
        if (_y > 0.0) { return  constants::half_pi<double>; }
        if (_y < 0.0) { return -constants::half_pi<double>; }
        return 0.0;
    }

    // ---- algebraic / rounding ---------------------------------------------

    inline D_CONSTEXPR double abs(double _x)   { return (_x < 0.0) ? -_x : _x; }

    template<typename _T>
    D_CONSTEXPR int sign(_T _x)
    { return (_T(0) < _x) - (_x < _T(0)); }

    inline D_CONSTEXPR double
    floor(double _x)
    {
        long long i = static_cast<long long>(_x);
        double    f = static_cast<double>(i);
        return (f > _x) ? (f - 1.0) : f;
    }

    inline D_CONSTEXPR double
    ceil(double _x)
    {
        long long i = static_cast<long long>(_x);
        double    f = static_cast<double>(i);
        return (f < _x) ? (f + 1.0) : f;
    }

    inline D_CONSTEXPR double trunc(double _x)
    { return static_cast<double>(static_cast<long long>(_x)); }

    inline D_CONSTEXPR double round(double _x)
    { return (_x >= 0.0) ? floor(_x + 0.5) : ceil(_x - 0.5); }

    inline D_CONSTEXPR double fract(double _x)
    { return _x - floor(_x); }

    template<typename _T>
    D_CONSTEXPR _T clamp(_T _v, _T _lo, _T _hi)
    { return (_v < _lo) ? _lo : ((_v > _hi) ? _hi : _v); }

    inline D_CONSTEXPR double saturate(double _x)
    { return clamp(_x, 0.0, 1.0); }

    template<typename _T>
    D_CONSTEXPR _T lerp(_T _a, _T _b, _T _t)
    { return _a + (_b - _a) * _t; }

    inline D_CONSTEXPR double radians(double _deg)
    { return _deg * constants::rad_per_deg<double>; }

    inline D_CONSTEXPR double degrees(double _rad)
    { return _rad * constants::deg_per_rad<double>; }

    // ---- combinatorial / number-theoretic ---------------------------------

    inline D_CONSTEXPR unsigned long long
    factorial(unsigned _n)
    {
        unsigned long long r = 1;
        for (unsigned i = 2; i <= _n; ++i) { r *= i; }
        return r;
    }

    inline D_CONSTEXPR unsigned long long
    binomial(unsigned _n, unsigned _k)
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

    template<typename _T>
    D_CONSTEXPR _T gcd(_T _a, _T _b)
    {
        if (_a < 0) { _a = -_a; }
        if (_b < 0) { _b = -_b; }
        while (_b != 0) { _T t = _a % _b; _a = _b; _b = t; }
        return _a;
    }

    template<typename _T>
    D_CONSTEXPR _T lcm(_T _a, _T _b)
    {
        if (_a == 0 || _b == 0) { return 0; }
        _T g = gcd(_a, _b);
        return (_a / g) * _b;
    }

    // horner: evaluate a polynomial whose coefficients are highest-degree
    // first, i.e. c[0]*x^(N-1) + c[1]*x^(N-2) + ... + c[N-1].
    template<std::size_t _N, typename _T>
    D_CONSTEXPR _T
    horner(const std::array<_T, _N>& _coeffs, _T _x)
    {
        _T acc = _T(0);
        for (std::size_t i = 0; i < _N; ++i)
        {
            acc = acc * _x + _coeffs[i];
        }
        return acc;
    }
}  // fn

NS_END  // math
NS_END  // djinterp


#endif  // DJINTERP_MATH_CALCULUS_ELEMENTARY_
