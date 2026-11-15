/*******************************************************************************
* djinterp [cpp/math]                                             arithmetic.hpp
*
* The arithmetic of the run-time expression language beyond value.h's: the
* standard functions' evaluators in a floating-point type, in a complex type
* and as a value, which the function table (function_table.hpp) holds, and
* the powers and remainders of the operators ^ and mod, which evaluate.hpp's
* folds apply.
*   IN A TYPE, a function is the C core's plain form in it, the C library's, as
* ^ is (S194): fast, and within an ulp of the _c forms (gate G6); abs is |x|.
* factorial(n) is n! for a whole n >= 0, the product rounded to the type as it
* is made, an infinity past the type's range; a NaN for any other n, as the C
* core has no gamma function. sign(x) is 1 or -1 by x's sign, a 0 or a NaN
* itself, as Julia's is; min and max IEEE 754-2019's minimum and maximum, a NaN
* where any argument is one and -0 below +0 (S338), where C's fmin and fmax give
* the other argument for a NaN, as IEEE 754-2008's minNum and maxNum did, and
* either of two zeros. Every number in a real type is real: real(x) is x,
* imag(x) 0, conj(x) x, arg(x) carg(x + 0i), pi where x is negative or -0 and +0
* otherwise, complex(x, y) x where y is 0 and a NaN otherwise, and im a NaN, as
* sqrt(-1) is (S271, S332); so too quat(w, x, y, z) w where x, y and z are 0,
* and a NaN otherwise, and jm and km NaNs; parts, rotate, rotmat, rotquat and
* slerp, which give or take vectors, matrices and quaternions, are NaNs, as
* cross is (S275, S391).
*   IN A COMPLEX TYPE, math::complex<T> of float, double or long double (S339),
* a function with a principal complex value is the face's by its own name, so
* cplx.h's plain form at run time, on the C library's real functions, as the
* owner ruled (S336), as a real type takes the C library's: sqrt, exp, log, pow,
* the trigonometric and hyperbolic functions and their inverses, and exp2, log2,
* log10, log1p and expm1, whose complex forms cplx.h has from chunk 42 (S335,
* S342), the last five's plain forms their _c forms' bits; one of the real
* line's order is its real form of the real parts, plus +0i, where every
* argument is real, and a NaN in both parts otherwise; abs, real, imag and arg
* are the face's, plus +0i, conj its conjugate, complex(x, y) of real x and y
* the number x + yi, sign(z) z / |z| and a 0 itself, and min and max of real
* numbers alone; quat(w, x, y, z) of real arguments w + xi where y and z are 0,
* and a NaN in both parts otherwise; abs and arg there are the plain forms too.
* The constants are their real values plus +0i, im 0 + 1i, and jm and km NaNs in
* both parts, as no complex number is a unit past im. evaluate.hpp's fold there
* keeps a real number real, as the value does.
*   WHERE A VALUE AND math::complex<T> MAY DIFFER (S336): a function with a
* principal complex value, computed densely as a value, is cplx.h's _c form,
* one long double kernel rounded once, the same bits at every level and on
* every compiler, within a step of the correctly rounded parts for float and
* double; computed in a complex type it is the plain form, its bits the C
* library's, within the plain bounds cplx.h's banner states, up to 6 steps.
* So sqrt(z), exp(z) and the rest of one inexact z may differ in their last
* bits between a value and the fold in math::complex<T>, each within its
* bound; an exact value, a Gaussian rational (S337), is the value's alone;
* the modulus and the argument of a dense number as their own forms; and
* the arithmetic + - * /, which comes once, and exp2, log2, log10, log1p and
* expm1, whose plain forms are their _c forms, never differ.
*   AS A VALUE, a function is exact wherever its value is a rational number of
* the family: abs, floor, ceil, round, trunc, fmod and remainder always; sqrt
* and cbrt where the argument's terms are whole squares or cubes, hypot where
* the sum of the squares is a rational square, pow as ^ is; factorial for a
* whole n >= 0; exp2 at a whole number, log2 and log10 at a power of 2 or 10;
* and the others at the one point each where their value is rational: sin, tan,
* asin, atan, sinh, tanh, log1p, expm1, asinh and atanh at 0, which is 0; cos,
* cosh and exp at 0, which is 1; acos, log and acosh at 1, which is 0; atan2(0,
* x) for x > 0, which is 0; and sign, min and max everywhere. Nowhere else is
* any of them rational (Lindemann and Weierstrass), so nowhere else is a value
* lost. An exact argument outside a function's domain, where it has no complex
* value either, is a domain error: factorial of a negative number or a fraction,
* fmod or remainder by 0, atan2(0, 0); the logarithm of 0, log1p(-1), and
* atanh(1) and atanh(-1) are poles. Elsewhere a function's value is the C core's
* in the kind the tower gives: the widest of the inexact arguments' kinds, a
* long double where all are exact, as ^'s is. Errors pass, the first; a boolean
* or none is a domain error; every result carries its arguments' flags.
*   COMPLEX NUMBERS (complex_kind.hpp, step 23, 3e) are values too. complex(x,
* y) is the number of two real parts, exact where both are, a real number where
* they are exact and y is 0; real, imag and conj give a number's parts and
* conjugate, a real number's imaginary part a 0 of its kind; arg its angle in
* (-pi, pi], exactly 0 for an exact real number not negative, and otherwise
* cplx.h's arg_c in the tower's kind; abs and the bars its modulus, exact where
* it is rational, |3 + 4im| being 5. Each function of a complex argument gives
* its principal value, with C99 Annex G's cuts (S271): exact where it is a
* Gaussian rational, sqrt(2im) being 1 + im, sqrt(3 + 4im) 2 + im and sign(3 +
* 4im) 3/5 + 4/5im, an exact number to a whole power by repeated squaring and to
* a rational p/q where its principal q-th root is a Gaussian rational, (2im) ^
* (1/2) being 1 + im (S337); and otherwise the face's _c form in the tower's
* type, a long double where every argument is exact, so cplx.h's _c form, dense
* and never made real (S336). So too a real argument where the function has no
* real value but a complex one (S272): sqrt and log of a negative number,
* sqrt(-4) being 2im and log(-1) pi im; asin, acos and atanh past -1 or 1, and
* acosh below 1, each taken as x + 0i, so the value Annex G gives on the upper
* side of the cut, asin(2) being pi/2 + 1.317...im and acos(2) -1.317...im, as
* C, Python, NumPy and Scilab give them, where MATLAB takes asin's and acos's
* cuts past 1 from below, its asin(2) 1.5708 - 1.3170i and acos(2) 0 + 1.3170i,
* the imaginary parts' signs the opposite (S341); and a negative base to a
* finite fractional power, (-8) ^ (1/3) being 1 + 1.732...im, where cbrt(-8) is
* -2. Of a complex argument, the functions of the real line's order, cbrt,
* floor, ceil, round, trunc, fmod, remainder, mod, atan2, hypot, the factorial,
* min and max, are a domain error (S271). exp2, log2, log10, log1p and expm1 of
* a complex argument, and log2 and log10 of a negative number and log1p of one
* below -1, are their principal values with cplx.h's forms of them (S342),
* dense, none being a Gaussian rational: log2(-8) is 3 + 4.532...im, its real
* part exactly 3, the real function's on the axis (S343). real, imag, conj and
* arg of a matrix are its elements', each element's, and complex(A, B) of two
* matrices of one shape the matrix of complex(a_ij, b_ij), of a matrix and a
* number a domain error (S350).
*   QUATERNIONS (quaternion_kind.hpp, step 24, 3f) are values too. real and
* conj give a quaternion's real part and conjugate; abs and the bars its
* norm, exact where it is rational, |1 + 2im + 2jm + 4km| being 5; and
* sign(q) q / |q|, exact where |q| is rational. exp, log and sqrt of a
* quaternion are their principal values, the complex ones in q's plane, w +
* |v| u for u = v / |v| (S364), in the tower's type, quat.h's _c forms,
* dense; sqrt exact where the root is a quaternion of rationals, sqrt(-2 +
* 2im + 2jm + 2km) being 1 + im + jm + km. q ^ r takes a real exponent: an
* exact whole r by repeated squaring, inv(q)'s for a negative one; an exact
* rational p/s where q's principal s-th root is a quaternion of rationals,
* (-46 + 3im + 6jm + 6km) ^ (1/3) being 2 + im + 2jm + 2km; otherwise
* quat.h's pow_c in the tower's type. A quaternion exponent, of any base, and
* a complex one of a quaternion are a domain error, as their two orders give
* two values; so are imag, arg and complex(x, y) of a quaternion, whose
* parts parts gives, the functions of the real line's order, and the
* trigonometric and hyperbolic functions and their inverses, which the
* language does not give a quaternion. quat, parts, inv, rotate, rotmat,
* rotquat and slerp are the function table's (function_table.hpp).
*   THE ARITHMETIC of ^ and mod, which value.h does not have, is here too,
* beside the functions': evaluate.hpp's folds apply it. A ^ n of a matrix is the
* matrix kind's power, a square matrix to a whole n, a negative one its
* inverse's (matrix_kind.hpp, S248, S259), and so is pow(A, n); a matrix in mod,
* or an argument of any other function but a value of piecewise's, an argument
* of linear algebra's and one of complex, real, imag, conj and arg (S350), is a
* domain error.
*   ONE HEADER FOR IT. Until chunk 48 this was section 3 of function.hpp,
* with the tables and the logic; now the table (function_table.hpp) and the
* operators (evaluate.hpp) each include it (S401).
*   STEPS. Step 15 (2c) filled the standard functions' evaluators; step 20
* (3b) gave ^ matrices; step 23 (3e), its first part, complex, real, imag,
* conj and arg, and complex numbers' powers, its second, in chunk 41, each
* function's principal complex value where cplx.h has its complex form,
* log1p, expm1, asinh, acosh, atanh, sign, min and max, and the complex
* types' evaluators, and in chunk 42 exp2, log2, log10, log1p and expm1 with
* cplx.h's own (S335, S342), its third, in chunk 43, the parts of a matrix
* and complex(A, B) (S350); step 24 (3f), its second part, in chunk 45, the
* functions and powers of quaternions (S275).
*
*
* path:      /inc/djinterp/cpp/modules/math/expr/arithmetic.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.14
*                                                            revised: 2026.11.14
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  THE ARITHMETIC
    --------------
    1.  The operations
         1.  expr_fn
    2.  In a floating-point type
         1.  A value in a type
         2.  The power, the remainder and the factorial
         3.  The parts
         4.  expr_fn_in
         5.  In a complex type
    3.  As a value
         1.  Exact powers and roots
         2.  Exact integers and remainders
         3.  Exact functions
         4.  In the tower's kind
         5.  expr_value_pow, expr_value_mod, expr_value_fn
         6.  expr_holds_complex, expr_real_only
*/

#ifndef DJINTERP_MATH_EXPR_ARITHMETIC_HPP
#define DJINTERP_MATH_EXPR_ARITHMETIC_HPP 1

// djinterp
#include "../../../djinterp.hpp"  // framework root

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <cmath>    // std::fabs, std::isinf, std::signbit, ...
#include <cstddef>  // std::size_t
#include <limits>   // std::numeric_limits
#include <vector>   // std::vector
// djinterp
#include "../../../../c/modules/math/elementary.h"       // d_math_sqrt,
                                                         // _sin, _cos, ...
#include "../../../../c/modules/math/exponential.h"      // d_math_exp,
                                                         // _exp2, _log, ...
#include "../../../../c/modules/math/hyperbolic.h"       // d_math_sinh,
                                                         // _cosh, _tanh
#include "../../../../c/modules/math/nearest_integer.h"  // d_math_floor,
                                                         // _ceil, ...
#include "../../../../c/modules/math/power.h"            // d_math_pow,
                                                         // _cbrt, _hypot
#include "../../../../c/modules/math/remainder.h"        // d_math_fmod,
                                                         // _remainder
#include "../../../../c/modules/math/tangent.h"          // d_math_tan,
                                                         // _atan, _atan2
#include "../math_common.hpp"                            // NS_MATH
#include "../value.hpp"                                  // value
#include "./complex_kind.hpp"                            // expr_complex,
                                                         // expr_real, ...
#include "./expr_common.hpp"                             // expr_context
#include "./matrix_kind.hpp"                             // expr_is_matrix,
                                                         // the powers
#include "./quaternion_kind.hpp"                         // expr_quaternion_fn,
                                                         // ...


NS_DJINTERP
NS_MATH


//==============================================================================
// 1.  THE ARITHMETIC
//==============================================================================
// What the standard functions and the operators ^ and mod compute, beyond
// value.h's + - * / and |x|: in a floating-point type, and as a value.


// 1.1    The operations
//------------------------------------------------------------------------------
NS_INTERNAL

    // 1.1.1
    // expr_fn
    //   enum: each operation of this section: the standard functions, the
    // complex numbers' parts and complex(x, y) among them (S271), the five
    // the C core gave in chunk 37 and min, max and sign (S203), and the
    // floored remainder of the operator mod, which no function is.
    enum class expr_fn
    {
        sqrt      = 1,
        cbrt      = 2,
        sin       = 3,
        cos       = 4,
        tan       = 5,
        asin      = 6,
        acos      = 7,
        atan      = 8,
        sinh      = 9,
        cosh      = 10,
        tanh      = 11,
        exp       = 12,
        exp2      = 13,
        log       = 14,
        log2      = 15,
        log10     = 16,
        floor     = 17,
        ceil      = 18,
        round     = 19,
        trunc     = 20,
        abs       = 21,
        factorial = 22,
        atan2     = 23,
        hypot     = 24,
        pow       = 25,
        fmod      = 26,
        remainder = 27,
        mod       = 28,
        real      = 29,
        imag      = 30,
        conj      = 31,
        arg       = 32,
        complex   = 33,
        log1p     = 34,
        expm1     = 35,
        asinh     = 36,
        acosh     = 37,
        atanh     = 38,
        sign      = 39,
        min       = 40,
        max       = 41
    };

NS_END  // internal

// 1.2    In a floating-point type
//------------------------------------------------------------------------------
NS_INTERNAL

    // 1.2.1
    // A value in a type
    //   functions: a NaN of Type, real or complex, the complex one's parts
    // both NaNs; whether a number of Type is a NaN, a complex one where
    // either part is; and a value's nearest Type, a boolean's 1 or 0, as
    // true's and false's, a NaN for an error or none, which have neither.
    template<typename Type>
    struct expr_nan_of
    {
        static Type
        make() D_NOEXCEPT
        {
            return std::numeric_limits<Type>::quiet_NaN();
        }
    };

    template<typename Type>
    struct expr_nan_of<math::complex<Type> >
    {
        static math::complex<Type>
        make() D_NOEXCEPT
        {
            return math::complex<Type>(std::numeric_limits<Type>::quiet_NaN(),
                                       std::numeric_limits<Type>::quiet_NaN());
        }
    };

    template<typename Type>
    bool
    expr_is_nan_of(
        Type _x
    )
    {
        return std::isnan(_x);
    }

    template<typename Type>
    bool
    expr_is_nan_of(
        const math::complex<Type>& _z
    )
    {
        return ( (std::isnan(_z.real())) ||
                 (std::isnan(_z.imag())) );
    }

    template<typename Type>
    Type
    expr_value_in(
        const value& _value
    )
    {
        Type nearest = Type(0);
        bool truth   = false;

        // a number: its nearest
        if (_value.to(nearest))
        {
            return nearest;
        }

        // a boolean: 1 or 0
        if (_value.to(truth))
        {
            return ( truth ? Type(1) : Type(0) );
        }

        return std::numeric_limits<Type>::quiet_NaN();
    }

    // 1.2.2
    // The power, the remainder and the factorial
    //   the C core's pow in Type, the library's; the floored remainder, fmod
    // plus the divisor where their signs differ, 0 with its sign; and n!,
    // as the banner says.
    template<typename Type>
    Type
    expr_pow_in(
        Type _base,
        Type _exponent
    )
    {
        return d_math_pow(_base, _exponent);
    }

    template<typename Type>
    Type
    expr_mod_in(
        Type _a,
        Type _b
    )
    {
        const Type rest = d_math_fmod(_a, _b);

        // a remainder of the other sign: one divisor more
        if ( (rest != Type(0)) &&
             ( (rest < Type(0)) != (_b < Type(0)) ) )
        {
            return rest + _b;
        }

        // none: 0, with the divisor's sign
        if (rest == Type(0))
        {
            return ( (_b < Type(0)) ? -Type(0) : Type(0) );
        }

        return rest;
    }

    template<typename Type>
    Type
    expr_factorial_in(
        Type _n
    )
    {
        Type product = Type(1);

        // a whole number, not negative: a NaN fails both
        if ( (!(_n >= Type(0))) ||
             (d_math_trunc(_n) != _n) )
        {
            return std::numeric_limits<Type>::quiet_NaN();
        }

        // each factor in turn, until the product passes the type's range,
        // and at the latest until the type's largest exponent: n! is past
        // 2^n from n = 4, and so past the range by then, where std::isinf
        // does not say so, as under valgrind's x87; each product stored, so
        // that the x87 rounds it to Type
        for (Type factor = Type(2);
             ( (factor <= _n) &&
               (factor <= Type(std::numeric_limits<Type>::max_exponent)) );
             factor += Type(1))
        {
            volatile Type stored = product * factor;

            product = stored;

            if (std::isinf(product))
            {
                break;
            }
        }

        return product;
    }

    // 1.2.3
    // The parts
    //   expr_arg_in, expr_complex_in: a real number's argument in Type,
    // carg(x + 0i), pi where x is negative or -0, a NaN for a NaN and +0
    // otherwise, pi the constant table's in Type; and complex(x, y), which
    // in Type is x where y is 0, of either sign, and otherwise a NaN, as a
    // number that is no real one is there (S332).
    template<typename Type>
    Type
    expr_arg_in(
        Type _x
    )
    {
        // a NaN: no angle
        if (_x != _x)
        {
            return _x;
        }

        return ( (std::signbit(_x))
                     ? static_cast<Type>(
                           3.141592653589793238462643383279502884197L)
                     : Type(0) );
    }

    template<typename Type>
    Type
    expr_complex_in(
        Type _re,
        Type _im
    )
    {
        return ( (_im == Type(0)) ? _re
                                  : std::numeric_limits<Type>::quiet_NaN() );
    }

    // expr_sign_in, expr_extreme_in, expr_min_in, expr_max_in
    //   functions: sign(x) in Type, 1 or -1 by x's sign, x itself for a 0 of
    // either sign and for a NaN, as Julia's sign is; and the least or the
    // greatest of _count numbers, the first at _x, as IEEE 754-2019's
    // minimum and maximum: a NaN, the first, where any is one, and -0 below
    // +0 (S338).
    template<typename Type>
    Type
    expr_sign_in(
        Type _x
    )
    {
        // a 0, of either sign, or a NaN: itself
        if ( (_x == Type(0)) ||
             (std::isnan(_x)) )
        {
            return _x;
        }

        return ( (_x < Type(0)) ? Type(-1) : Type(1) );
    }

    template<typename Type>
    Type
    expr_extreme_in(
        const Type* _x,
        std::size_t _count,
        bool        _greatest
    )
    {
        Type best = _x[0];

        // a NaN: the first
        for (std::size_t i = 0; i < _count; ++i)
        {
            if (std::isnan(_x[i]))
            {
                return _x[i];
            }
        }

        // each number past the best, or a 0 of the sign the extreme takes
        for (std::size_t i = 1; i < _count; ++i)
        {
            const bool past  = ( _greatest ? (_x[i] > best) : (_x[i] < best) );
            const bool other = ( (_x[i] == best) &&
                                 (std::signbit(_x[i]) != std::signbit(best)) &&
                                 (std::signbit(_x[i]) != _greatest) );

            if ( (past) ||
                 (other) )
            {
                best = _x[i];
            }
        }

        return best;
    }

    template<typename Type>
    Type
    expr_min_in(
        const Type*         _x,
        std::size_t         _count,
        const expr_context& _context
    )
    {
        static_cast<void>(_context);

        return expr_extreme_in(_x, _count, false);
    }

    template<typename Type>
    Type
    expr_max_in(
        const Type*         _x,
        std::size_t         _count,
        const expr_context& _context
    )
    {
        static_cast<void>(_context);

        return expr_extreme_in(_x, _count, true);
    }

    // 1.2.4
    // expr_fn_in
    //   function: an operation in Type, of the arguments it takes, the first
    // at _x: a real number's parts its own and 0, its conjugate itself. min
    // and max, of any number of arguments, are expr_min_in's and
    // expr_max_in's.
    template<typename Type>
    Type
    expr_fn_in(
        expr_fn     _fn,
        const Type* _x
    )
    {
        // by operation
        switch (_fn)
        {
            case expr_fn::sqrt:      return d_math_sqrt(_x[0]);
            case expr_fn::cbrt:      return d_math_cbrt(_x[0]);
            case expr_fn::sin:       return d_math_sin(_x[0]);
            case expr_fn::cos:       return d_math_cos(_x[0]);
            case expr_fn::tan:       return d_math_tan(_x[0]);
            case expr_fn::asin:      return d_math_asin(_x[0]);
            case expr_fn::acos:      return d_math_acos(_x[0]);
            case expr_fn::atan:      return d_math_atan(_x[0]);
            case expr_fn::sinh:      return d_math_sinh(_x[0]);
            case expr_fn::cosh:      return d_math_cosh(_x[0]);
            case expr_fn::tanh:      return d_math_tanh(_x[0]);
            case expr_fn::exp:       return d_math_exp(_x[0]);
            case expr_fn::exp2:      return d_math_exp2(_x[0]);
            case expr_fn::log:       return d_math_log(_x[0]);
            case expr_fn::log2:      return d_math_log2(_x[0]);
            case expr_fn::log10:     return d_math_log10(_x[0]);
            case expr_fn::floor:     return d_math_floor(_x[0]);
            case expr_fn::ceil:      return d_math_ceil(_x[0]);
            case expr_fn::round:     return d_math_round(_x[0]);
            case expr_fn::trunc:     return d_math_trunc(_x[0]);
            case expr_fn::abs:       return std::fabs(_x[0]);
            case expr_fn::factorial: return expr_factorial_in(_x[0]);
            case expr_fn::atan2:     return d_math_atan2(_x[0], _x[1]);
            case expr_fn::hypot:     return d_math_hypot(_x[0], _x[1]);
            case expr_fn::pow:       return expr_pow_in(_x[0], _x[1]);
            case expr_fn::fmod:      return d_math_fmod(_x[0], _x[1]);
            case expr_fn::remainder: return d_math_remainder(_x[0], _x[1]);
            case expr_fn::real:      return _x[0];
            case expr_fn::imag:      return Type(0);
            case expr_fn::conj:      return _x[0];
            case expr_fn::arg:       return expr_arg_in(_x[0]);
            case expr_fn::complex:   return expr_complex_in(_x[0], _x[1]);
            case expr_fn::log1p:     return d_math_log1p(_x[0]);
            case expr_fn::expm1:     return d_math_expm1(_x[0]);
            case expr_fn::asinh:     return d_math_asinh(_x[0]);
            case expr_fn::acosh:     return d_math_acosh(_x[0]);
            case expr_fn::atanh:     return d_math_atanh(_x[0]);
            case expr_fn::sign:      return expr_sign_in(_x[0]);
            case expr_fn::mod:
            default:
                break;
        }

        return expr_mod_in(_x[0], _x[1]);
    }

    // 1.2.5
    // In a complex type
    //   functions: an operation in math::complex<Type>, of the arguments it
    // takes, the first at _z (S339). A function with a principal complex
    // value is math::complex<Type>'s: by its own name in a complex type, so
    // cplx.h's plain form at run time, and by its _c name, cplx.h's _c form,
    // as a value (S336); a function of the real line's order its real form
    // of the real parts, plus +0i, where every argument is real, its
    // imaginary part 0 of either sign, and a NaN in both parts otherwise;
    // the parts, the modulus and the argument as the face's, in the same
    // form, complex(x, y) of two real numbers x + yi, and sign(z) z / |z|, a
    // 0 itself.

    // expr_is_real_of
    //   function: whether a complex number is real, its imaginary part 0 of
    // either sign.
    template<typename Type>
    bool
    expr_is_real_of(
        const math::complex<Type>& _z
    )
    {
        return (_z.imag() == Type(0));
    }

    // expr_real_line_complex
    //   function: _fn of the real line in math::complex<Type>: its real form
    // of the real parts, plus +0i, where each of the _count arguments is
    // real; a NaN otherwise.
    template<typename Type>
    math::complex<Type>
    expr_real_line_complex(
        expr_fn                    _fn,
        const math::complex<Type>* _z,
        std::size_t                _count
    )
    {
        Type x[2] = { Type(0), Type(0) };

        // each argument real, its real part taken
        for (std::size_t i = 0; i < _count; ++i)
        {
            if (!expr_is_real_of(_z[i]))
            {
                return expr_nan_of<math::complex<Type> >::make();
            }

            x[i] = _z[i].real();
        }

        return math::complex<Type>(expr_fn_in<Type>(_fn, x));
    }

    // expr_fn_complex
    //   function: an operation in math::complex<Type>, as 3.2.5 says: in
    // cplx.h's plain form, as a complex type takes it, or where _exact in
    // its _c form, as a value takes it (S336).
    template<typename Type>
    math::complex<Type>
    expr_fn_complex(
        expr_fn                    _fn,
        const math::complex<Type>* _z,
        bool                       _exact = false
    )
    {
        typedef math::complex<Type> number;

        // by operation
        switch (_fn)
        {
            case expr_fn::sqrt:  return ( _exact ? sqrt_c(_z[0])
                                                 : sqrt(_z[0]) );
            case expr_fn::sin:   return ( _exact ? sin_c(_z[0])
                                                 : sin(_z[0]) );
            case expr_fn::cos:   return ( _exact ? cos_c(_z[0])
                                                 : cos(_z[0]) );
            case expr_fn::tan:   return ( _exact ? tan_c(_z[0])
                                                 : tan(_z[0]) );
            case expr_fn::asin:  return ( _exact ? asin_c(_z[0])
                                                 : asin(_z[0]) );
            case expr_fn::acos:  return ( _exact ? acos_c(_z[0])
                                                 : acos(_z[0]) );
            case expr_fn::atan:  return ( _exact ? atan_c(_z[0])
                                                 : atan(_z[0]) );
            case expr_fn::sinh:  return ( _exact ? sinh_c(_z[0])
                                                 : sinh(_z[0]) );
            case expr_fn::cosh:  return ( _exact ? cosh_c(_z[0])
                                                 : cosh(_z[0]) );
            case expr_fn::tanh:  return ( _exact ? tanh_c(_z[0])
                                                 : tanh(_z[0]) );
            case expr_fn::asinh: return ( _exact ? asinh_c(_z[0])
                                                 : asinh(_z[0]) );
            case expr_fn::acosh: return ( _exact ? acosh_c(_z[0])
                                                 : acosh(_z[0]) );
            case expr_fn::atanh: return ( _exact ? atanh_c(_z[0])
                                                 : atanh(_z[0]) );
            case expr_fn::exp:   return ( _exact ? exp_c(_z[0])
                                                 : exp(_z[0]) );
            case expr_fn::log:   return ( _exact ? log_c(_z[0])
                                                 : log(_z[0]) );
            case expr_fn::exp2:  return ( _exact ? exp2_c(_z[0])
                                                 : exp2(_z[0]) );
            case expr_fn::log2:  return ( _exact ? log2_c(_z[0])
                                                 : log2(_z[0]) );
            case expr_fn::log10: return ( _exact ? log10_c(_z[0])
                                                 : log10(_z[0]) );
            case expr_fn::log1p: return ( _exact ? log1p_c(_z[0])
                                                 : log1p(_z[0]) );
            case expr_fn::expm1: return ( _exact ? expm1_c(_z[0])
                                                 : expm1(_z[0]) );
            case expr_fn::pow:   return ( _exact ? pow_c(_z[0], _z[1])
                                                 : pow(_z[0], _z[1]) );
            case expr_fn::abs:   return number( _exact ? abs_c(_z[0])
                                                       : abs(_z[0]) );
            case expr_fn::real:  return number(_z[0].real());
            case expr_fn::imag:  return number(_z[0].imag());
            case expr_fn::conj:  return conj(_z[0]);
            case expr_fn::arg:   return number( _exact ? arg_c(_z[0])
                                                       : arg(_z[0]) );
            case expr_fn::complex:
                // two real parts
                if ( (!expr_is_real_of(_z[0])) ||
                     (!expr_is_real_of(_z[1])) )
                {
                    return expr_nan_of<number>::make();
                }

                return number(_z[0].real(), _z[1].real());
            case expr_fn::sign:
                // a 0 itself, and otherwise z / |z|
                if (_z[0] == number(Type(0)))
                {
                    return _z[0];
                }

                return ( _z[0] / ( _exact ? abs_c(_z[0])
                                          : abs(_z[0]) ) );
            case expr_fn::atan2:
            case expr_fn::hypot:
            case expr_fn::fmod:
            case expr_fn::remainder:
            case expr_fn::mod:
                return expr_real_line_complex(_fn, _z, 2);
            default:
                break;
        }

        return expr_real_line_complex(_fn, _z, 1);
    }

    // expr_extreme_complex, expr_min_complex, expr_max_complex
    //   functions: min and max in math::complex<Type>: of real numbers the
    // real one's, plus +0i; a NaN where any is not real.
    template<typename Type>
    math::complex<Type>
    expr_extreme_complex(
        const math::complex<Type>* _z,
        std::size_t                _count,
        bool                       _greatest
    )
    {
        std::vector<Type> x;

        // each real, its real part taken
        for (std::size_t i = 0; i < _count; ++i)
        {
            if (!expr_is_real_of(_z[i]))
            {
                return expr_nan_of<math::complex<Type> >::make();
            }

            x.push_back(_z[i].real());
        }

        return math::complex<Type>(expr_extreme_in(x.data(),
                                                   _count,
                                                   _greatest));
    }

    template<typename Type>
    math::complex<Type>
    expr_min_complex(
        const math::complex<Type>* _z,
        std::size_t                _count,
        const expr_context&        _context
    )
    {
        static_cast<void>(_context);

        return expr_extreme_complex(_z, _count, false);
    }

    template<typename Type>
    math::complex<Type>
    expr_max_complex(
        const math::complex<Type>* _z,
        std::size_t                _count,
        const expr_context&        _context
    )
    {
        static_cast<void>(_context);

        return expr_extreme_complex(_z, _count, true);
    }

    // expr_complex_function
    //   struct: the operation fn as expr_complex_apply takes it
    // (complex_kind.hpp), expr_fn_complex's in each type in the _c forms, as a
    // value takes them: one answer at every level, where the fold in
    // math::complex<T> takes the plain forms (S336).
    struct expr_complex_function
    {
        expr_fn fn;

        template<typename Type>
        math::complex<Type>
        operator()(
            const math::complex<Type>* _z
        ) const
        {
            return expr_fn_complex<Type>(fn,
                                         _z,
                                         true);
        }
    };

NS_END  // internal

// 1.3    As a value
//------------------------------------------------------------------------------
NS_INTERNAL

    // 1.3.1
    // Exact powers and roots
    //   a whole power by repeated squaring, exact as value.h's products and
    // quotients are; a whole root, for a rational exponent and the roots;
    // and whether a whole number is a power of another.

    // expr_power_capped
    //   function: whether _root ^ _k is at most _n, for _root >= 1, and if so
    // the power, in _power: each product checked before it is made.
    inline bool
    expr_power_capped(
        dj_math_imax  _root,
        dj_math_imax  _k,
        dj_math_imax  _n,
        dj_math_imax& _power
    )
    {
        dj_math_imax product = 1;

        // _k factors, none past _n
        for (dj_math_imax i = 0; i < _k; ++i)
        {
            if (product > _n / _root)
            {
                return false;
            }

            product *= _root;
        }

        _power = product;

        return true;
    }

    // expr_whole_root
    //   function: whether _n >= 0 is the _k-th power of a whole number, for
    // _k >= 1, and if so that number, in _root: the greatest whose power is
    // at most _n, by bisection, then its power compared.
    inline bool
    expr_whole_root(
        dj_math_imax  _n,
        dj_math_imax  _k,
        dj_math_imax& _root
    )
    {
        // 0 and 1 are their own roots
        if (_n < 2)
        {
            _root = _n;

            return true;
        }

        // past the word's width, no whole number's power is _n
        if (_k >= static_cast<dj_math_imax>(
                      std::numeric_limits<dj_math_imax>::digits))
        {
            return false;
        }

        dj_math_imax low   = 1;
        dj_math_imax high  = _n;
        dj_math_imax power = 0;

        // the greatest root whose power is at most _n
        while (low < high)
        {
            const dj_math_imax middle = low + (high - low + 1) / 2;

            if (expr_power_capped(middle, _k, _n, power))
            {
                low = middle;
            }
            else
            {
                high = middle - 1;
            }
        }

        _root = low;

        return ( (expr_power_capped(low, _k, _n, power)) &&
                 (power == _n) );
    }

    // expr_ratio_root
    //   function: whether the rational _q, its terms whole _k-th powers, has
    // an exact _k-th root, and if so the root, in _out: of a negative _q
    // only for an odd _k, and the negative of its magnitude's.
    inline bool
    expr_ratio_root(
        const dj_rational& _q,
        dj_math_imax       _k,
        value&             _out
    )
    {
        dj_math_imax num_root = 0;
        dj_math_imax den_root = 1;

        // a negative number: an odd root, of a magnitude a word holds
        if (_q.num < 0)
        {
            if ( ((_k % 2) == 0) ||
                 (_q.num == std::numeric_limits<dj_math_imax>::min()) ||
                 (!expr_whole_root(-_q.num, _k, num_root)) ||
                 (!expr_whole_root(_q.den, _k, den_root)) )
            {
                return false;
            }

            _out = value::ratio(-num_root, den_root);

            return true;
        }

        // terms that are whole _k-th powers
        if ( (expr_whole_root(_q.num, _k, num_root)) &&
             (expr_whole_root(_q.den, _k, den_root)) )
        {
            _out = value::ratio(num_root, den_root);

            return true;
        }

        return false;
    }

    // expr_whole_power
    //   function: _base ^ _n for an exact base and a whole _n: by repeated
    // squaring, exact; 1 for _n = 0, and the reciprocal for _n < 0, a pole
    // for a base of 0.
    inline value
    expr_whole_power(
        const value& _base,
        dj_math_imax _n
    )
    {
        dj_math_umax left = ( (_n < 0)
                                  ? (dj_math_umax(0) - dj_math_umax(_n))
                                  : dj_math_umax(_n) );
        value        power(1);
        value        square = _base;

        // a factor for each bit of the exponent
        while (left != 0)
        {
            if ((left & 1u) != 0)
            {
                power = power * square;
            }

            left >>= 1;

            if (left != 0)
            {
                square = square * square;
            }
        }

        // a negative exponent: the reciprocal
        if (_n < 0)
        {
            return (value(1) / power);
        }

        return power;
    }

    // expr_ratio_power
    //   function: whether _base ^ (p / q), for an exact base not negative and
    // an exact rational exponent, is exact, where the base's terms are whole
    // q-th powers; and if so its value, in _out. Where it is not, its value
    // is the tower's kind's. A negative base's power is complex (S272),
    // expr_value_pow's.
    inline bool
    expr_ratio_power(
        const value&       _base,
        const dj_rational& _exponent,
        value&             _out
    )
    {
        dj_rational  terms    = { 0, 1 };
        dj_math_imax num_root = 0;
        dj_math_imax den_root = 1;

        // terms that are whole q-th powers
        if ( (_base.to(terms)) &&
             (expr_whole_root(terms.num, _exponent.den, num_root)) &&
             (expr_whole_root(terms.den, _exponent.den, den_root)) )
        {
            _out = expr_whole_power(value::ratio(num_root, den_root),
                                    _exponent.num);

            return true;
        }

        return false;
    }

    // expr_gcd
    //   function: the greatest common divisor of two positive whole numbers,
    // Euclid's.
    inline dj_math_imax
    expr_gcd(
        dj_math_imax _a,
        dj_math_imax _b
    )
    {
        // each remainder in turn
        while (_b != 0)
        {
            const dj_math_imax rest = _a % _b;

            _a = _b;
            _b = rest;
        }

        return _a;
    }

    // expr_gaussian_root
    //   function: whether the exact number _z, real or complex, its parts
    // rationals of the family, has a principal _q-th root that is a Gaussian
    // rational, for 2 <= _q below the word's width, and if so that root, in
    // _out (S337). With D the least common denominator of z's parts, D times
    // a root w of z is a Gaussian integer: w = g / e in lowest terms, e^q
    // divides D's power and e divides D, so D w = (D / e) g. So the
    // principal root, exp(log z / q), is found in long double, D times it is
    // rounded part by part to whole numbers G, and w = G / D is taken where
    // its q-th power is z exactly, every step of it exact, and it lies
    // within 2^-10 of its modulus of the long double root, as each other
    // root lies 2 sin(pi / q), 2^-4 or more, of its modulus away. Where D w
    // passes the family, or the long double cannot round it, there is none.
    inline bool
    expr_gaussian_root(
        const value& _z,
        dj_math_imax _q,
        value&       _out
    )
    {
        dj_rational re  = { 0, 1 };
        dj_rational im  = { 0, 1 };
        long double zr  = 0.0L;
        long double zi  = 0.0L;

        // exact parts of the family, and a root of the word's width
        if ( (!_z.is_exact()) ||
             (!expr_real(_z).to(re)) ||
             (!expr_imag(_z).to(im)) ||
             (_q < 2) ||
             (_q >= static_cast<dj_math_imax>(
                        std::numeric_limits<dj_math_imax>::digits)) )
        {
            return false;
        }

        const value den = value(re.den / expr_gcd(re.den, im.den)) *
                          value(im.den);
        long double d   = 0.0L;

        static_cast<void>(expr_real(_z).to(zr));
        static_cast<void>(expr_imag(_z).to(zi));
        static_cast<void>(den.to(d));

        const math::complex<long double> principal =
            exp_c(log_c(math::complex<long double>(zr, zi)) /
                static_cast<long double>(_q));
        const long double gr    = std::floor((d * principal.real()) + 0.5L);
        const long double gi    = std::floor((d * principal.imag()) + 0.5L);
        const long double limit = 9.0e18L;

        // D w within the family
        if ( (!den.is_exact()) ||
             (!(std::fabs(gr) < limit)) ||
             (!(std::fabs(gi) < limit)) )
        {
            return false;
        }

        const value       root  = expr_complex(
                                      value(static_cast<dj_math_imax>(gr)),
                                      value(static_cast<dj_math_imax>(gi))) /
                                  den;
        const value       power = expr_whole_power(root, _q);
        const long double dr    = (gr / d) - principal.real();
        const long double di    = (gi / d) - principal.imag();
        const long double size  = ((gr * gr) + (gi * gi)) / (d * d);

        // the root's q-th power z, exactly, and the root the principal one
        if ( (!root.is_exact()) ||
             (!power.is_exact()) ||
             (!(power == _z)) ||
             ( ((dr * dr) + (di * di)) * 1048576.0L > size ) )
        {
            return false;
        }

        _out = root;

        return true;
    }

    // expr_power_of
    //   function: whether _n >= 1 is a whole power of _base > 1, and if so
    // the exponent, in _k: _n divided by _base while it divides, then
    // compared with 1, the power 0.
    inline bool
    expr_power_of(
        dj_math_imax  _n,
        dj_math_imax  _base,
        dj_math_imax& _k
    )
    {
        dj_math_imax exponent = 0;

        while ( (_n % _base) == 0 )
        {
            _n /= _base;
            ++exponent;
        }

        _k = exponent;

        return (_n == 1);
    }

    // 1.3.2
    // Exact integers and remainders
    //   a rational rounded to a whole number each way, and the remainder of
    // a quotient rounded so: floored for mod, truncated for fmod, to the
    // nearest, ties to even, for remainder.
    enum class expr_rounding
    {
        floor = 1,   // toward minus infinity
        ceil  = 2,   // toward plus infinity
        trunc = 3,   // toward 0
        away  = 4,   // to the nearest, halves away from 0, as C's round
        even  = 5    // to the nearest, halves to the even, as IEEE's
    };

    // expr_whole_of
    //   function: the rational _q rounded to a whole number by _how; the
    // quotient of terms of the family, so that no step leaves the word.
    inline dj_math_imax
    expr_whole_of(
        const dj_rational& _q,
        expr_rounding      _how
    )
    {
        const dj_math_imax whole = _q.num / _q.den;
        const dj_math_imax rest  = _q.num % _q.den;
        const dj_math_imax step  = ( (_q.num < 0) ? -1 : 1 );
        const dj_math_imax size  = ( (rest < 0) ? -rest : rest );

        // a whole number already
        if (rest == 0)
        {
            return whole;
        }

        // by the rounding: the truncated quotient, or one step from it
        switch (_how)
        {
            case expr_rounding::floor:
                return ( (_q.num < 0) ? (whole - 1) : whole );

            case expr_rounding::ceil:
                return ( (_q.num > 0) ? (whole + 1) : whole );

            case expr_rounding::away:
                return ( (size >= _q.den - size) ? (whole + step) : whole );

            case expr_rounding::even:
                // past the half, or at it from an odd quotient
                if ( (size > _q.den - size) ||
                     ( (size == _q.den - size) &&
                       ((whole % 2) != 0) ) )
                {
                    return whole + step;
                }

                return whole;

            case expr_rounding::trunc:
            default:
                break;
        }

        return whole;
    }

    // expr_exact_rest
    //   function: whether _a less _b times their quotient rounded by _how
    // is exact, and if so its value, in _out: for exact operands, a domain
    // error for b = 0.
    inline bool
    expr_exact_rest(
        const value&  _a,
        const value&  _b,
        expr_rounding _how,
        value&        _out
    )
    {
        dj_rational quotient = { 0, 1 };

        // an exact zero divisor: no remainder
        if (_b == value(0))
        {
            _out = value(D_MATH_VALUE_ERROR_DOMAIN);

            return true;
        }

        // an exact quotient, rounded
        if ((_a / _b).to(quotient))
        {
            _out = _a - (_b * value(expr_whole_of(quotient, _how)));

            return true;
        }

        return false;
    }

    // expr_exact_factorial
    //   function: n! for an exact whole n >= 0: value.h's products, past the
    // family G2's long double, until it passes long double's range.
    inline value
    expr_exact_factorial(
        dj_math_imax _n
    )
    {
        value       product(1);
        long double nearest = 0.0L;

        // each factor in turn, and at the latest until long double's largest
        // exponent, as expr_factorial_in counts
        for (dj_math_imax factor = 2;
             ( (factor <= _n) &&
               (factor <= std::numeric_limits<long double>::max_exponent) );
             ++factor)
        {
            product = product * value(factor);

            // past the family, and past long double's range
            if ( (!product.is_exact()) &&
                 (product.to(nearest)) &&
                 (std::isinf(nearest)) )
            {
                break;
            }
        }

        return product;
    }

    // 1.3.3
    // Exact functions
    //   function: whether _fn of exact arguments has an exact value, or an
    // error, as the banner says; and if so it, in _out. Where it has
    // neither, its value is the tower's kind's.

    // expr_exact_at
    //   function: whether the exact _x is the rational _at, and if so the
    // exact _value, in _out: the one point where a function is rational.
    inline bool
    expr_exact_at(
        const dj_rational& _x,
        dj_math_imax       _at,
        dj_math_imax       _value,
        value&             _out
    )
    {
        // the point itself
        if ( (_x.num == _at) &&
             (_x.den == 1) )
        {
            _out = value(_value);

            return true;
        }

        return false;
    }

    // expr_exact_log
    //   function: the logarithm to _base of the exact _x, where it is exact
    // or an error: a pole at 0, a domain error below it; and for a base of
    // 2 or 10, the exponent of a whole power, 1/_base^k among them.
    inline bool
    expr_exact_log(
        const dj_rational& _x,
        dj_math_imax       _base,
        value&             _out
    )
    {
        dj_math_imax k = 0;

        // 0, and below it
        if (_x.num <= 0)
        {
            _out = value( (_x.num == 0) ? D_MATH_VALUE_ERROR_POLE
                                        : D_MATH_VALUE_ERROR_DOMAIN );

            return true;
        }

        // a power of the base: a whole one, or its reciprocal
        if ( (_x.den == 1) &&
             (expr_power_of(_x.num, _base, k)) )
        {
            _out = value(k);

            return true;
        }

        if ( (_x.num == 1) &&
             (expr_power_of(_x.den, _base, k)) )
        {
            _out = value(-k);

            return true;
        }

        return false;
    }

    // expr_exact_one
    //   function: a function of one exact argument, a rational of the
    // family, by the banner's rules.
    inline bool
    expr_exact_one(
        expr_fn            _fn,
        const dj_rational& _x,
        value&             _out
    )
    {
        const bool past_one = ( (_x.num > _x.den) ||
                                (_x.num < -_x.den) );

        // by function
        switch (_fn)
        {
            case expr_fn::sqrt:
                // no real root of a negative number
                if (_x.num < 0)
                {
                    _out = value(D_MATH_VALUE_ERROR_DOMAIN);

                    return true;
                }

                return expr_ratio_root(_x, 2, _out);

            case expr_fn::cbrt:
                return expr_ratio_root(_x, 3, _out);

            case expr_fn::asin:
            case expr_fn::acos:
                // past 1, no angle
                if (past_one)
                {
                    _out = value(D_MATH_VALUE_ERROR_DOMAIN);

                    return true;
                }

                return ( (_fn == expr_fn::asin) ? expr_exact_at(_x, 0, 0, _out)
                                                : expr_exact_at(_x, 1, 0,
                                                                _out) );

            case expr_fn::sin:
            case expr_fn::tan:
            case expr_fn::atan:
            case expr_fn::sinh:
            case expr_fn::tanh:
                return expr_exact_at(_x, 0, 0, _out);

            case expr_fn::cos:
            case expr_fn::cosh:
            case expr_fn::exp:
                return expr_exact_at(_x, 0, 1, _out);

            case expr_fn::exp2:
                // a whole exponent
                if (_x.den == 1)
                {
                    _out = expr_whole_power(value(2), _x.num);

                    return true;
                }

                return false;

            case expr_fn::log:
                // the pole and the domain, then 1
                if (_x.num <= 0)
                {
                    return expr_exact_log(_x, 2, _out);
                }

                return expr_exact_at(_x, 1, 0, _out);

            case expr_fn::log2:
                return expr_exact_log(_x, 2, _out);

            case expr_fn::log10:
                return expr_exact_log(_x, 10, _out);

            case expr_fn::floor:
                _out = value(expr_whole_of(_x, expr_rounding::floor));

                return true;

            case expr_fn::ceil:
                _out = value(expr_whole_of(_x, expr_rounding::ceil));

                return true;

            case expr_fn::round:
                _out = value(expr_whole_of(_x, expr_rounding::away));

                return true;

            case expr_fn::trunc:
                _out = value(expr_whole_of(_x, expr_rounding::trunc));

                return true;

            case expr_fn::log1p:
                // -1 a pole, and below it no real value: the complex one,
                // which expr_value_fn takes first (S272)
                if (_x.num <= -_x.den)
                {
                    _out = value( (_x.num == -_x.den)
                                      ? D_MATH_VALUE_ERROR_POLE
                                      : D_MATH_VALUE_ERROR_DOMAIN );

                    return true;
                }

                return expr_exact_at(_x, 0, 0, _out);

            case expr_fn::atanh:
                // -1 and 1 poles; past them a complex value, taken before
                if ( (_x.num == _x.den) ||
                     (_x.num == -_x.den) )
                {
                    _out = value(D_MATH_VALUE_ERROR_POLE);

                    return true;
                }

                return expr_exact_at(_x, 0, 0, _out);

            case expr_fn::expm1:
            case expr_fn::asinh:
                return expr_exact_at(_x, 0, 0, _out);

            case expr_fn::acosh:
                return expr_exact_at(_x, 1, 0, _out);

            case expr_fn::sign:
                _out = value( (_x.num < 0) ? -1 : ( (_x.num > 0) ? 1 : 0 ) );

                return true;

            case expr_fn::factorial:
            default:
                break;
        }

        // the factorial: of a whole number, not negative
        if ( (_x.den != 1) ||
             (_x.num < 0) )
        {
            _out = value(D_MATH_VALUE_ERROR_DOMAIN);

            return true;
        }

        _out = expr_exact_factorial(_x.num);

        return true;
    }

    // expr_exact_two
    //   function: a function of two exact arguments, by the banner's rules:
    // atan2 and hypot of rationals of the family; fmod, remainder and mod
    // of any exact numbers.
    inline bool
    expr_exact_two(
        expr_fn      _fn,
        const value& _a,
        const value& _b,
        value&       _out
    )
    {
        dj_rational x   = { 0, 1 };
        dj_rational y   = { 0, 1 };
        dj_rational sum = { 0, 1 };

        // by function
        switch (_fn)
        {
            case expr_fn::atan2:
                // the angle of (b, a): 0 on the positive axis, none at 0
                if ( (!_a.to(y)) ||
                     (!_b.to(x)) ||
                     (y.num != 0) ||
                     (x.num < 0) )
                {
                    return false;
                }

                _out = ( (x.num == 0) ? value(D_MATH_VALUE_ERROR_DOMAIN)
                                      : value(0) );

                return true;

            case expr_fn::hypot:
                // the root of the sum of the squares, where it is exact
                return ( (((_a * _a) + (_b * _b)).to(sum)) &&
                         (expr_ratio_root(sum, 2, _out)) );

            case expr_fn::fmod:
                return expr_exact_rest(_a, _b, expr_rounding::trunc, _out);

            case expr_fn::remainder:
                return expr_exact_rest(_a, _b, expr_rounding::even, _out);

            case expr_fn::mod:
            default:
                break;
        }

        return expr_exact_rest(_a, _b, expr_rounding::floor, _out);
    }

    // 1.3.4
    // In the tower's kind
    //   an operation in the kind of the tower its arguments give: the widest
    // inexact kind of theirs, a long double where all are exact; each
    // argument its nearest there; the answer stored in that kind before it
    // becomes a value, as the x87 may hold more of it than the kind has;
    // and the result carrying every argument's flags.

    // expr_inexact_kind
    //   function: a number's kind, where it is an inline inexact one; a long
    // double for a boxed inexact kind; none for an exact one.
    inline dj_math_value_kind
    expr_inexact_kind(
        const value& _x
    )
    {
        // an inline inexact kind: its own
        if ( (_x.kind() == D_MATH_VALUE_FLOAT)  ||
             (_x.kind() == D_MATH_VALUE_DOUBLE) ||
             (_x.kind() == D_MATH_VALUE_LONG_DOUBLE) )
        {
            return _x.kind();
        }

        return ( (_x.is_exact()) ? D_MATH_VALUE_NONE
                                 : D_MATH_VALUE_LONG_DOUBLE );
    }

    template<typename Type>
    value
    expr_tower_as(
        expr_fn      _fn,
        const value* _x,
        std::size_t  _count
    )
    {
        Type     x[2]  = { Type(0), Type(0) };
        unsigned flags = 0u;

        // each argument its nearest Type, its flags kept
        for (std::size_t i = 0; i < _count; ++i)
        {
            x[i]   = expr_value_in<Type>(_x[i]);
            flags |= _x[i].flags();
        }

        // the answer stored, so that the x87 rounds it to Type: the C
        // library may hand it back with more precision than Type has
        volatile Type answer = expr_fn_in<Type>(_fn, x);
        value         result(static_cast<Type>(answer));
        dj_math_value held = result.detach();

        held.flags |= flags;

        return value::adopt(held);
    }

    inline value
    expr_tower(
        expr_fn      _fn,
        const value* _x,
        std::size_t  _count
    )
    {
        dj_math_value_kind widest = D_MATH_VALUE_NONE;

        // the widest inexact kind of the arguments
        for (std::size_t i = 0; i < _count; ++i)
        {
            const dj_math_value_kind kind = expr_inexact_kind(_x[i]);

            if (kind > widest)
            {
                widest = kind;
            }
        }

        // that kind; a long double for exact arguments alone
        if (widest == D_MATH_VALUE_FLOAT)
        {
            return expr_tower_as<float>(_fn, _x, _count);
        }

        if (widest == D_MATH_VALUE_DOUBLE)
        {
            return expr_tower_as<double>(_fn, _x, _count);
        }

        return expr_tower_as<long double>(_fn, _x, _count);
    }

    // 1.3.5
    // expr_value_pow, expr_value_mod, expr_value_fn
    //   functions: _a ^ _b, _a mod _b, and any operation of this section, as
    // values, as the banner says: an error passes, the first; a boolean or
    // none is a domain error; exact where the operation is; otherwise in
    // the tower's kind. A matrix to a power is the matrix kind's
    // (matrix_kind.hpp); real, imag, conj and arg of a matrix its elements',
    // and complex(A, B) of two the matrix of complex(a_ij, b_ij) (S350); and in
    // any other operation here a matrix is a domain error. A complex
    // number's parts, its conjugate and argument and complex(x, y) are
    // complex_kind.hpp's. A function of a complex argument, or of a real one
    // where it has no real value but a complex one (S272), is its principal
    // complex value, exact where it is a Gaussian rational (S337), and
    // otherwise expr_fn_complex's in the tower's type (S336), exp2, log2,
    // log10, log1p and expm1 among them (S342); of the real line's order, a
    // domain error (S271). min and max are exact by value.h's order, the
    // least or the greatest argument itself (S338).

    // expr_value_failed
    //   function: whether an argument is an error, which passes, the first,
    // or no number, a domain error; and if so that, in _out.
    inline bool
    expr_value_failed(
        const value* _x,
        std::size_t  _count,
        value&       _out
    )
    {
        // an error passes, the first
        for (std::size_t i = 0; i < _count; ++i)
        {
            if (_x[i].kind() == D_MATH_VALUE_ERROR)
            {
                _out = _x[i];

                return true;
            }
        }

        // what is no number has no value here
        for (std::size_t i = 0; i < _count; ++i)
        {
            if (!_x[i].is_number())
            {
                _out = value(D_MATH_VALUE_ERROR_DOMAIN);

                return true;
            }
        }

        return false;
    }

    // expr_value_negative, expr_value_fraction
    //   functions: whether a real number is below 0, by value.h's exact
    // order, a NaN not; and whether one is finite and not whole: an exact
    // rational not an integer, or an inexact number finite and not whole, a
    // NaN and an infinity not, as their powers are real.
    inline bool
    expr_value_negative(
        const value& _x
    )
    {
        return ( d_math_value_cmp(_x.c_value(),
                                  d_math_value_from_int(0)) ==
                     D_MATH_VALUE_LESS );
    }

    inline bool
    expr_value_fraction(
        const value& _x
    )
    {
        dj_rational q       = { 0, 1 };
        long double nearest = 0.0L;

        // exact: a rational of a denominator past 1
        if (_x.is_exact())
        {
            return ( (_x.to(q)) &&
                     (q.den != 1) );
        }

        return ( (_x.to(nearest)) &&
                 (std::isfinite(nearest)) &&
                 (std::floor(nearest) != nearest) );
    }

    // expr_beyond_real
    //   function: whether the real _x is where _fn has no real value but a
    // complex one (S272): sqrt, log, log2 and log10 below 0, log1p below -1,
    // asin, acos and atanh past -1 or 1, acosh below 1; a NaN nowhere.
    inline bool
    expr_beyond_real(
        expr_fn      _fn,
        const value& _x
    )
    {
        const dj_math_value_order below =
            d_math_value_cmp(_x.c_value(), d_math_value_from_int(-1));
        const dj_math_value_order above =
            d_math_value_cmp(_x.c_value(), d_math_value_from_int(1));

        // by function
        switch (_fn)
        {
            case expr_fn::sqrt:
            case expr_fn::log:
            case expr_fn::log2:
            case expr_fn::log10:
                return expr_value_negative(_x);

            case expr_fn::log1p:
                return (below == D_MATH_VALUE_LESS);

            case expr_fn::asin:
            case expr_fn::acos:
            case expr_fn::atanh:
                return ( (below == D_MATH_VALUE_LESS) ||
                         (above == D_MATH_VALUE_GREATER) );

            case expr_fn::acosh:
                return (above == D_MATH_VALUE_LESS);

            default:
                break;
        }

        return false;
    }

    // expr_complex_sign
    //   function: sign(z) of a complex number: z / |z|, exact where the
    // modulus is; otherwise expr_fn_complex's in the tower's type.
    inline value
    expr_complex_sign(
        const value& _z
    )
    {
        const value modulus = abs(_z);

        // an exact modulus: an exact quotient
        if (modulus.is_exact())
        {
            return (_z / modulus);
        }

        const expr_complex_function op = { expr_fn::sign };

        return expr_complex_apply(&_z, 1, op);
    }

    // expr_complex_value_fn
    //   function: _fn of _count arguments, one of them complex or a real one
    // where _fn has a complex value alone, as 1.3.5 says.
    inline value
    expr_complex_value_fn(
        expr_fn      _fn,
        const value* _x,
        std::size_t  _count
    )
    {
        const expr_complex_function op = { _fn };
        value                       root;

        // by function
        switch (_fn)
        {
            case expr_fn::sqrt:
                // an exact root, where it is a Gaussian rational
                if (expr_gaussian_root(_x[0], 2, root))
                {
                    return expr_complex_flagged(root, _x[0].flags());
                }

                return expr_complex_apply(_x, _count, op);

            case expr_fn::sin:
            case expr_fn::cos:
            case expr_fn::tan:
            case expr_fn::asin:
            case expr_fn::acos:
            case expr_fn::atan:
            case expr_fn::sinh:
            case expr_fn::cosh:
            case expr_fn::tanh:
            case expr_fn::asinh:
            case expr_fn::acosh:
            case expr_fn::atanh:
            case expr_fn::exp:
            case expr_fn::log:
            case expr_fn::exp2:
            case expr_fn::log2:
            case expr_fn::log10:
            case expr_fn::log1p:
            case expr_fn::expm1:
                return expr_complex_apply(_x, _count, op);

            case expr_fn::sign:
                return expr_complex_sign(_x[0]);

            default:
                break;
        }

        // the real line's order
        return value(D_MATH_VALUE_ERROR_DOMAIN);
    }

    // expr_quaternion_exact_root
    //   function: whether the principal _den-th root of the exact quaternion
    // _q is a quaternion of rationals, and if so it, in _out: the square
    // root by quaternion_kind.hpp's expr_quaternion_root; any other where |v|
    // is rational, in q's plane, the Gaussian root of w + |v| im (S337) taken
    // back along u = v / |v|, as each function of a quaternion is the
    // complex one in its plane (S275).
    inline bool
    expr_quaternion_exact_root(
        const value& _q,
        dj_math_imax _den,
        value&       _out
    )
    {
        // the square root
        if (_den == 2)
        {
            return expr_quaternion_root(_q, _out);
        }

        const expr_quaternion_parts_held parts =
            expr_quaternion_parts_of(_q.c_value());
        const value r = value_root_of( ( (parts.x * parts.x) +
                                         (parts.y * parts.y) ) +
                                       (parts.z * parts.z) );
        value       root;

        // |v| irrational, or no Gaussian root in the plane
        if ( (!expr_is_quaternion(_q)) ||
             (!r.is_exact()) ||
             (!expr_gaussian_root(expr_complex(parts.w, r), _den, root)) )
        {
            return false;
        }

        const value c = expr_real(root);
        const value d = expr_imag(root);

        _out = expr_complex_flagged(
            expr_quaternion_made(c,
                                 (d * parts.x) / r,
                                 (d * parts.y) / r,
                                 (d * parts.z) / r,
                                 0u),
            _q.flags());

        return true;
    }

    // expr_quaternion_value_fn
    //   function: _fn of _count arguments, one of them a quaternion, as 1.3.5
    // says: exp and log their principal values, the complex ones in q's
    // plane, in the tower's type; sqrt too, but exact where the root is a
    // quaternion of rationals; and sign q / |q|; any other function a domain
    // error, of the real line's order or none a quaternion has (S275).
    inline value
    expr_quaternion_value_fn(
        expr_fn      _fn,
        const value* _x,
        std::size_t  _count
    )
    {
        value root;

        // of one argument: exp, log, sqrt and sign
        if (_count == 1)
        {
            switch (_fn)
            {
                case expr_fn::exp:
                    return expr_quaternion_apply(expr_quaternion_fn::exp,
                                                 _x[0],
                                                 value(0));

                case expr_fn::log:
                    return expr_quaternion_apply(expr_quaternion_fn::log,
                                                 _x[0],
                                                 value(0));

                case expr_fn::sqrt:
                    // an exact root, where it is a quaternion of rationals
                    if (expr_quaternion_root(_x[0], root))
                    {
                        return root;
                    }

                    return expr_quaternion_apply(expr_quaternion_fn::sqrt,
                                                 _x[0],
                                                 value(0));

                case expr_fn::sign:
                    return expr_quaternion_sign(_x[0]);

                default:
                    break;
            }
        }

        // none of a quaternion
        return value(D_MATH_VALUE_ERROR_DOMAIN);
    }

    // expr_value_signbit
    //   function: whether a real number's sign bit is set: an inexact
    // one's, -0 among them; never an exact one's.
    inline bool
    expr_value_signbit(
        const value& _x
    )
    {
        long double nearest = 0.0L;

        return ( (!_x.is_exact()) &&
                 (_x.to(nearest)) &&
                 (std::signbit(nearest)) );
    }

    // expr_extreme_value
    //   function: min or max of _count values, the first at _x: an error,
    // the first; a domain error for what is no number, and for a complex
    // number or a quaternion, as no order of them is a field's (S271,
    // S275); a NaN, the first,
    // where any is one; and otherwise the least or the greatest argument
    // itself, by value.h's exact order, the first of equal ones but -0
    // below 0 (S338); with every argument's flags.
    inline value
    expr_extreme_value(
        const value* _x,
        std::size_t  _count,
        bool         _greatest
    )
    {
        unsigned    flags = 0u;
        std::size_t best  = 0;
        value       failed;

        // an error, or what is no number
        if (expr_value_failed(_x, _count, failed))
        {
            return failed;
        }

        // a complex number or a quaternion: no order
        for (std::size_t i = 0; i < _count; ++i)
        {
            if ( (expr_is_complex(_x[i])) ||
                 (expr_is_quaternion(_x[i])) )
            {
                return value(D_MATH_VALUE_ERROR_DOMAIN);
            }

            flags |= _x[i].flags();
        }

        // a NaN, the first
        for (std::size_t i = 0; i < _count; ++i)
        {
            if (d_math_value_cmp(_x[i].c_value(), _x[i].c_value()) ==
                    D_MATH_VALUE_UNORDERED)
            {
                return expr_complex_flagged(_x[i], flags);
            }
        }

        // each argument past the best, or a 0 of the sign the extreme takes
        for (std::size_t i = 1; i < _count; ++i)
        {
            const dj_math_value_order order =
                d_math_value_cmp(_x[i].c_value(), _x[best].c_value());
            const bool past  = ( _greatest
                                     ? (order == D_MATH_VALUE_GREATER)
                                     : (order == D_MATH_VALUE_LESS) );
            const bool other =
                ( (order == D_MATH_VALUE_EQUAL) &&
                  (expr_value_signbit(_x[i]) !=
                       expr_value_signbit(_x[best])) &&
                  (expr_value_signbit(_x[i]) != _greatest) );

            if ( (past) ||
                 (other) )
            {
                best = i;
            }
        }

        return expr_complex_flagged(_x[best], flags);
    }

    inline value
    expr_min_value(
        const value*        _x,
        std::size_t         _count,
        const expr_context& _context
    )
    {
        static_cast<void>(_context);

        return expr_extreme_value(_x, _count, false);
    }

    inline value
    expr_max_value(
        const value*        _x,
        std::size_t         _count,
        const expr_context& _context
    )
    {
        static_cast<void>(_context);

        return expr_extreme_value(_x, _count, true);
    }

    // expr_quaternion_value_pow
    //   function: _a ^ _b, one of them a quaternion: to a real exponent
    // alone, a complex or quaternion one a domain error, as its two orders
    // give two values (S275); exact operands to a whole exponent exactly, by
    // repeated squaring, and to a rational one where the root is a
    // quaternion of rationals; otherwise the principal value, quat.h's in
    // the tower's type.
    inline value
    expr_quaternion_value_pow(
        const value& _a,
        const value& _b
    )
    {
        dj_math_imax whole    = 0;
        dj_rational  exponent = { 0, 1 };
        value        root;

        // no real exponent
        if (!expr_complex_real_number(_b))
        {
            return value(D_MATH_VALUE_ERROR_DOMAIN);
        }

        // exact operands: a whole exponent, or a rational one
        if ( (_a.is_exact()) &&
             (_b.is_exact()) )
        {
            if (_b.to(whole))
            {
                return expr_whole_power(_a, whole);
            }

            if ( (_b.to(exponent)) &&
                 (expr_quaternion_exact_root(_a, exponent.den, root)) )
            {
                return expr_whole_power(root, exponent.num);
            }
        }

        return expr_quaternion_apply(expr_quaternion_fn::pow, _a, _b);
    }

    inline value
    expr_value_pow(
        const value& _a,
        const value& _b
    )
    {
        const value  x[2]     = { _a, _b };
        dj_math_imax whole    = 0;
        dj_rational  exponent = { 0, 1 };
        value        exact;

        // a matrix: an error passes, the first; then a matrix's power
        if ( (expr_is_matrix(_a)) ||
             (expr_is_matrix(_b)) )
        {
            if (_a.kind() == D_MATH_VALUE_ERROR)
            {
                return _a;
            }

            if (_b.kind() == D_MATH_VALUE_ERROR)
            {
                return _b;
            }

            return expr_matrix_power(_a,
                                     _b);
        }

        // an error, or what is no number
        if (expr_value_failed(x, 2, exact))
        {
            return exact;
        }

        // a quaternion: expr_quaternion_value_pow's (S275)
        if ( (expr_is_quaternion(_a)) ||
             (expr_is_quaternion(_b)) )
        {
            return expr_quaternion_value_pow(_a, _b);
        }

        // a complex operand, or a negative real base to a power not whole
        // (S272): exact operands to a whole exponent exactly, by repeated
        // squaring, and to a rational one where the base's principal root is
        // a Gaussian rational (S337); otherwise the principal value,
        // expr_fn_complex's in the tower's type (S336)
        if ( (expr_is_complex(_a)) ||
             (expr_is_complex(_b)) ||
             ( (expr_value_negative(_a)) &&
               (expr_value_fraction(_b)) ) )
        {
            const expr_complex_function op = { expr_fn::pow };
            value                       root;

            if ( (_a.is_exact()) &&
                 (_b.is_exact()) )
            {
                if (_b.to(whole))
                {
                    return expr_whole_power(_a, whole);
                }

                if ( (_b.to(exponent)) &&
                     (expr_gaussian_root(_a, exponent.den, root)) )
                {
                    return expr_whole_power(root, exponent.num);
                }
            }

            return expr_complex_apply(x, 2, op);
        }

        // exact operands: a whole exponent, or a rational one
        if ( (_a.is_exact()) &&
             (_b.is_exact()) )
        {
            if (_b.to(whole))
            {
                return expr_whole_power(_a, whole);
            }

            if ( (_b.to(exponent)) &&
                 (expr_ratio_power(_a, exponent, exact)) )
            {
                return exact;
            }
        }

        return expr_tower(expr_fn::pow, x, 2);
    }

    // expr_value_fn_beyond_real
    //   function: whether _fn of the _count arguments at _x takes its value
    // past the real numbers: of a quaternion argument its principal value,
    // or none (S275); of a complex one likewise, none of the real line's
    // order (S271); and of a real one where its value is complex alone
    // (S272); and if so that value, in _out.
    inline bool
    expr_value_fn_beyond_real(
        expr_fn      _fn,
        const value* _x,
        std::size_t  _count,
        value&       _out
    )
    {
        // a quaternion argument
        for (std::size_t i = 0; i < _count; ++i)
        {
            if (expr_is_quaternion(_x[i]))
            {
                _out = expr_quaternion_value_fn(_fn, _x, _count);

                return true;
            }
        }

        // a complex argument
        for (std::size_t i = 0; i < _count; ++i)
        {
            if (expr_is_complex(_x[i]))
            {
                _out = expr_complex_value_fn(_fn, _x, _count);

                return true;
            }
        }

        // a real argument where the function's value is complex alone
        if ( (_count == 1) &&
             (expr_beyond_real(_fn, _x[0])) )
        {
            _out = expr_complex_value_fn(_fn, _x, _count);

            return true;
        }

        return false;
    }

    inline value
    expr_value_fn(
        expr_fn      _fn,
        const value* _x,
        std::size_t  _count
    )
    {
        dj_rational one   = { 0, 1 };
        bool        exact = true;
        value       answer;

        // of a matrix, each element's, element by element: matrix_kind.hpp's
        // (S350), real and conj quaternion_kind.hpp's, which take every number
        if ( ( (_fn == expr_fn::real) ||
               (_fn == expr_fn::imag) ||
               (_fn == expr_fn::conj) ||
               (_fn == expr_fn::arg) ) &&
             (expr_is_matrix(_x[0])) )
        {
            return expr_matrix_each(
                _x[0],
                ( (_fn == expr_fn::real) ? &expr_quaternion_real
                : (_fn == expr_fn::imag) ? &expr_imag
                : (_fn == expr_fn::conj) ? &expr_quaternion_conj
                                         : &expr_arg ));
        }

        // complex(A, B) of matrices: the matrix of complex(a_ij, b_ij)
        if ( (_fn == expr_fn::complex) &&
             ( (expr_is_matrix(_x[0])) ||
               (expr_is_matrix(_x[1])) ) )
        {
            return expr_matrix_complex_of(_x[0],
                                          _x[1]);
        }

        // a number's real part and conjugate, a quaternion's among them,
        // quaternion_kind.hpp's; a complex number's imaginary part and
        // argument, and complex(x, y), complex_kind.hpp's, of a quaternion a
        // domain error
        switch (_fn)
        {
            case expr_fn::real:    return expr_quaternion_real(_x[0]);
            case expr_fn::imag:    return expr_imag(_x[0]);
            case expr_fn::conj:    return expr_quaternion_conj(_x[0]);
            case expr_fn::arg:     return expr_arg(_x[0]);
            case expr_fn::complex: return expr_complex(_x[0], _x[1]);
            default:
                break;
        }

        // |x| is value.h's, the modulus of a complex number, ^'s power
        // pow's, and min and max the least and the greatest
        if (_fn == expr_fn::abs)
        {
            return abs(_x[0]);
        }

        if (_fn == expr_fn::pow)
        {
            return expr_value_pow(_x[0], _x[1]);
        }

        if ( (_fn == expr_fn::min) ||
             (_fn == expr_fn::max) )
        {
            return expr_extreme_value(_x, _count, (_fn == expr_fn::max));
        }

        // an error, or what is no number
        if (expr_value_failed(_x, _count, answer))
        {
            return answer;
        }

        // a quaternion or complex argument, or a real one where the value
        // is complex alone: its principal value, or none
        if (expr_value_fn_beyond_real(_fn, _x, _count, answer))
        {
            return answer;
        }

        for (std::size_t i = 0; i < _count; ++i)
        {
            exact = ( (exact) && (_x[i].is_exact()) );
        }

        // exact arguments, exact where the operation is
        if ( (exact) &&
             ( (_count == 1) ? ( (_x[0].to(one)) &&
                                 (expr_exact_one(_fn, one, answer)) )
                             : expr_exact_two(_fn, _x[0], _x[1], answer) ) )
        {
            return answer;
        }

        return expr_tower(_fn, _x, _count);
    }

    inline value
    expr_value_mod(
        const value& _a,
        const value& _b
    )
    {
        const value x[2] = { _a, _b };

        return expr_value_fn(expr_fn::mod, x, 2);
    }

    // 1.3.6
    // expr_holds_complex, expr_real_only
    //   functions: whether a value is or holds a number not real, a complex
    // number, a quaternion, or a matrix holding either; and an
    // answer under _context: a domain error where it asks real results, none
    // of the _count arguments at _arguments holds a number not real, and the
    // answer does (S272, S340), so that sqrt(-4) and eig of a real matrix
    // with a complex pair (S352) are domain errors there, and a function of
    // a complex argument keeps its complex value. Each standard evaluator
    // as a value that can stray so applies it to its own answer.
    inline bool
    expr_holds_complex(
        const value& _x
    )
    {
        return ( (expr_is_complex(_x)) ||
                 (expr_is_quaternion(_x)) ||
                 ( (expr_is_matrix(_x)) &&
                   ( (expr_matrix_has_complex(_x)) ||
                     (expr_matrix_has_quaternion(_x)) ) ) );
    }

    inline value
    expr_real_only(
        const expr_context& _context,
        const value*        _arguments,
        std::size_t         _count,
        const value&        _answer
    )
    {
        // real results not asked
        if (!_context.real_results)
        {
            return _answer;
        }

        // an argument not real: a complex answer is its own
        for (std::size_t i = 0; i < _count; ++i)
        {
            if (expr_holds_complex(_arguments[i]))
            {
                return _answer;
            }
        }

        return ( (expr_holds_complex(_answer))
                     ? value(D_MATH_VALUE_ERROR_DOMAIN)
                     : _answer );
    }

NS_END  // internal


NS_END  // math
NS_END  // djinterp


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_MATH_EXPR_ARITHMETIC_HPP
