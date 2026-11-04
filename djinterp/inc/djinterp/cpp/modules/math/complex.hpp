/*******************************************************************************
* djinterp [cpp/math]                                                complex.hpp
*
* Complex numbers in C++: math::complex<T>, of float, double and long double,
* the face of the C core's struct d_cplx_f, _d and _ld (c/math/cplx.h), as
* math::value is the value's (S270).
*   ONE ANSWER. Every operation is cplx.h's: its arithmetic, pure, each
* part correctly rounded or within a step of it; and C99's functions, each
* in its _c form, one long double kernel rounded once to the type, at every
* level and on every compiler, so that a result has the same bits wherever
* it is computed, in a constant expression or not (G6). The plain forms,
* faster on the C library's real functions, are not taken: their bits are
* each library's. An operation with a real operand takes it as itself, not
* as x + 0i (C99's G.5), so that a zero's sign survives and no infinity
* meets a zero.
*   CONSTRUCTION is from two parts, the imaginary +0 when it is left out;
* implicitly from a real number, and from a std::complex<T>, so that a
* standard value meeting this one becomes this one; and explicitly from the
* C core's struct, and from a complex<U> of another part type. OUT are
* real() and imag(); c_value(), the C core's struct, lent; and to_std(), a
* std::complex<T>, by name only, so that no expression of the two types is
* ambiguous.
*   OPERATIONS are found by argument-dependent lookup only, as math::value's
* are: + - * / and their assignments, with a complex or a real operand on
* either side; unary - and +; == and !=, part by part, a -0 equal to a 0;
* and real, imag, conj, proj, abs, arg, exp, log, sqrt, pow, sin, cos, tan,
* asin, acos, atan, sinh, cosh, tanh, asinh, acosh and atanh, each with C99
* Annex G's branch cuts and special values and the bounds cplx.h states;
* and exp2, log2, log10, log1p and expm1, cplx.h's of a complex number, with
* the values at the special parts of the functions each is a form of, and
* on the real axis, where the value is real, the real function's.
* pow with a real operand is pow of it plus 0i, as C's <tgmath.h> takes it.
* There is no order, and no < to ask one of.
*   NUMBERS. number_traits<complex<T> > is a commutative field, inexact and
* unordered; its magnitude, a T, is |re| + |im|, which LAPACK pivots by, and
* its tolerance four of T's epsilons, so that linear_algebra's vectors and
* matrices of it compute: their sums and products, determinant, inverse and
* solution the bits of cmat.h's. common_number of a complex number and its
* real type, or of two, is the complex type, at every level.
*   LEVELS. Everything from C++98; a literal type whose operations are
* constant expressions from C++14, as cplx.h's _c forms are.
*
*
* path:      /inc/djinterp/cpp/modules/math/complex.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.30
*                                                            revised: 2026.11.02
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  THE COMPLEX NUMBER
    ------------------
    1.  The C core's struct, by type
    2.  complex
2.  NUMBERS
    -------
    1.  number_traits
    2.  common_number
*/

#ifndef DJINTERP_MATH_COMPLEX_HPP
#define DJINTERP_MATH_COMPLEX_HPP 1

// std
#include <complex>                    // std::complex
// djinterp
#include "../../djinterp.hpp"              // framework root
#include "../../../c/modules/math/cplx.h"  // struct d_cplx_*, d_cplx_make and
                                           // the one name of every function
#include "./math_common.hpp"               // NS_MATH
#include "./number_traits.hpp"             // number_traits, common_number


NS_DJINTERP
NS_MATH


//==============================================================================
// 1.  THE COMPLEX NUMBER
//==============================================================================


// 1.1    The C core's struct, by type
//------------------------------------------------------------------------------
NS_INTERNAL

    // complex_core
    //   trait: the C core's complex struct of Type's parts, struct d_cplx_f,
    // _d or _ld, and whether Type has one: float, double and long double.
    template<typename Type>
    struct complex_core
    {
        enum
        {
            supported = 0
        };
    };

    template<>
    struct complex_core<float>
    {
        typedef struct d_cplx_f type;

        enum
        {
            supported = 1
        };
    };

    template<>
    struct complex_core<double>
    {
        typedef struct d_cplx_d type;

        enum
        {
            supported = 1
        };
    };

    template<>
    struct complex_core<long double>
    {
        typedef struct d_cplx_ld type;

        enum
        {
            supported = 1
        };
    };

NS_END  // internal

// 1.2    complex
//------------------------------------------------------------------------------
// complex
//   class: a complex number re + im i of Type's parts, float, double or long
// double: the C core's struct and its operations, found by argument-
// dependent lookup. The default value is 0, both parts +0.
template<typename Type>
class complex
{
    D_STATIC_ASSERT((internal::complex_core<Type>::supported == 1),
                    "complex: its parts are float, double or long double, "
                    "the C core's.");

public:
    typedef Type                                        value_type;
    typedef typename internal::complex_core<Type>::type core_type;

    // ---- construction ------------------------------------------------------

    // 0.
    D_CONSTEXPR_CPP14
    complex() D_NOEXCEPT
        : m_core(d_cplx_make(static_cast<Type>(0),
                             static_cast<Type>(0)))
    {}

    // _re + _im i; a real number, implicitly, with +0 its imaginary part.
    D_CONSTEXPR_CPP14
    complex(
        Type _re,
        Type _im = static_cast<Type>(0)
    ) D_NOEXCEPT
        : m_core(d_cplx_make(_re,
                             _im))
    {}

    // a standard library's complex number, implicitly: its two parts.
    D_CONSTEXPR_CPP14
    complex(
        const std::complex<Type>& _z
    ) D_NOEXCEPT
        : m_core(d_cplx_make(_z.real(),
                             _z.imag()))
    {}

    // the C core's struct, explicitly.
    D_CONSTEXPR_CPP14 explicit
    complex(
        const core_type& _z
    ) D_NOEXCEPT
        : m_core(_z)
    {}

    // a complex number of another part type, explicitly: each part
    // converted, as a number of that type converts to Type.
    template<typename Other>
    D_CONSTEXPR_CPP14 explicit
    complex(
        const complex<Other>& _z
    ) D_NOEXCEPT
        : m_core(d_cplx_make(static_cast<Type>(_z.real()),
                             static_cast<Type>(_z.imag())))
    {}

    // ---- the parts ---------------------------------------------------------

    // the real part, and the imaginary.
    D_CONSTEXPR_CPP14 Type
    real() const D_NOEXCEPT
    {
        return d_cplx_real(m_core);
    }

    D_CONSTEXPR_CPP14 Type
    imag() const D_NOEXCEPT
    {
        return d_cplx_imag(m_core);
    }

    // the real part set to _re, and the imaginary to _im, as
    // std::complex's are.
    D_CONSTEXPR_CPP14 void
    real(
        Type _re
    ) D_NOEXCEPT
    {
        m_core.part[0] = _re;

        return;
    }

    D_CONSTEXPR_CPP14 void
    imag(
        Type _im
    ) D_NOEXCEPT
    {
        m_core.part[1] = _im;

        return;
    }

    // ---- conversions -------------------------------------------------------

    // the C core's struct, lent.
    D_CONSTEXPR_CPP14 const core_type&
    c_value() const D_NOEXCEPT
    {
        return m_core;
    }

    // the standard library's complex number of the same parts.
    D_CONSTEXPR_CPP14 std::complex<Type>
    to_std() const D_NOEXCEPT
    {
        return std::complex<Type>(m_core.part[0],
                                  m_core.part[1]);
    }

    // ---- assignment by an operation ----------------------------------------

    // this number plus, less, times or over _w, or the real _x.
    D_CONSTEXPR_CPP14 complex&
    operator+=(
        const complex& _w
    ) D_NOEXCEPT
    {
        m_core = d_cplx_add(m_core, _w.m_core);

        return *this;
    }

    D_CONSTEXPR_CPP14 complex&
    operator+=(
        Type _x
    ) D_NOEXCEPT
    {
        m_core = d_cplx_add_real(m_core, _x);

        return *this;
    }

    D_CONSTEXPR_CPP14 complex&
    operator-=(
        const complex& _w
    ) D_NOEXCEPT
    {
        m_core = d_cplx_sub(m_core, _w.m_core);

        return *this;
    }

    D_CONSTEXPR_CPP14 complex&
    operator-=(
        Type _x
    ) D_NOEXCEPT
    {
        m_core = d_cplx_sub_real(m_core, _x);

        return *this;
    }

    D_CONSTEXPR_CPP14 complex&
    operator*=(
        const complex& _w
    ) D_NOEXCEPT
    {
        m_core = d_cplx_mul(m_core, _w.m_core);

        return *this;
    }

    D_CONSTEXPR_CPP14 complex&
    operator*=(
        Type _x
    ) D_NOEXCEPT
    {
        m_core = d_cplx_mul_real(m_core, _x);

        return *this;
    }

    D_CONSTEXPR_CPP14 complex&
    operator/=(
        const complex& _w
    ) D_NOEXCEPT
    {
        m_core = d_cplx_div(m_core, _w.m_core);

        return *this;
    }

    D_CONSTEXPR_CPP14 complex&
    operator/=(
        Type _x
    ) D_NOEXCEPT
    {
        m_core = d_cplx_div_real(m_core, _x);

        return *this;
    }

    // ---- arithmetic --------------------------------------------------------

    // +_z, and -_z, each part negated.
    friend D_CONSTEXPR_CPP14 complex
    operator+(
        const complex& _z
    ) D_NOEXCEPT
    {
        return _z;
    }

    friend D_CONSTEXPR_CPP14 complex
    operator-(
        const complex& _z
    ) D_NOEXCEPT
    {
        return complex(d_cplx_neg(_z.m_core));
    }

    // _z + _w; a real operand added to the real part alone.
    friend D_CONSTEXPR_CPP14 complex
    operator+(
        const complex& _z,
        const complex& _w
    ) D_NOEXCEPT
    {
        return complex(d_cplx_add(_z.m_core, _w.m_core));
    }

    friend D_CONSTEXPR_CPP14 complex
    operator+(
        const complex& _z,
        Type           _x
    ) D_NOEXCEPT
    {
        return complex(d_cplx_add_real(_z.m_core, _x));
    }

    friend D_CONSTEXPR_CPP14 complex
    operator+(
        Type           _x,
        const complex& _z
    ) D_NOEXCEPT
    {
        return complex(d_cplx_add_real(_z.m_core, _x));
    }

    // _z - _w; a real operand on the real part alone, x - z negating the
    // imaginary part.
    friend D_CONSTEXPR_CPP14 complex
    operator-(
        const complex& _z,
        const complex& _w
    ) D_NOEXCEPT
    {
        return complex(d_cplx_sub(_z.m_core, _w.m_core));
    }

    friend D_CONSTEXPR_CPP14 complex
    operator-(
        const complex& _z,
        Type           _x
    ) D_NOEXCEPT
    {
        return complex(d_cplx_sub_real(_z.m_core, _x));
    }

    friend D_CONSTEXPR_CPP14 complex
    operator-(
        Type           _x,
        const complex& _z
    ) D_NOEXCEPT
    {
        return complex(d_cplx_real_sub(_x, _z.m_core));
    }

    // _z _w; a real operand times each part.
    friend D_CONSTEXPR_CPP14 complex
    operator*(
        const complex& _z,
        const complex& _w
    ) D_NOEXCEPT
    {
        return complex(d_cplx_mul(_z.m_core, _w.m_core));
    }

    friend D_CONSTEXPR_CPP14 complex
    operator*(
        const complex& _z,
        Type           _x
    ) D_NOEXCEPT
    {
        return complex(d_cplx_mul_real(_z.m_core, _x));
    }

    friend D_CONSTEXPR_CPP14 complex
    operator*(
        Type           _x,
        const complex& _z
    ) D_NOEXCEPT
    {
        return complex(d_cplx_mul_real(_z.m_core, _x));
    }

    // _z / _w; each part over a real divisor; a real over _w as cplx.h's
    // real_div gives it.
    friend D_CONSTEXPR_CPP14 complex
    operator/(
        const complex& _z,
        const complex& _w
    ) D_NOEXCEPT
    {
        return complex(d_cplx_div(_z.m_core, _w.m_core));
    }

    friend D_CONSTEXPR_CPP14 complex
    operator/(
        const complex& _z,
        Type           _x
    ) D_NOEXCEPT
    {
        return complex(d_cplx_div_real(_z.m_core, _x));
    }

    friend D_CONSTEXPR_CPP14 complex
    operator/(
        Type           _x,
        const complex& _z
    ) D_NOEXCEPT
    {
        return complex(d_cplx_real_div(_x, _z.m_core));
    }

    // ---- comparison --------------------------------------------------------

    // whether _z and _w are equal part by part, a -0 equal to a 0 and a NaN
    // to nothing; a real operand a number whose imaginary part is 0.
    friend D_CONSTEXPR_CPP14 bool
    operator==(
        const complex& _z,
        const complex& _w
    ) D_NOEXCEPT
    {
        return d_cplx_equal(_z.m_core, _w.m_core);
    }

    friend D_CONSTEXPR_CPP14 bool
    operator==(
        const complex& _z,
        Type           _x
    ) D_NOEXCEPT
    {
        return d_cplx_equal(_z.m_core,
                            d_cplx_make(_x, static_cast<Type>(0)));
    }

    friend D_CONSTEXPR_CPP14 bool
    operator==(
        Type           _x,
        const complex& _z
    ) D_NOEXCEPT
    {
        return d_cplx_equal(_z.m_core,
                            d_cplx_make(_x, static_cast<Type>(0)));
    }

    friend D_CONSTEXPR_CPP14 bool
    operator!=(
        const complex& _z,
        const complex& _w
    ) D_NOEXCEPT
    {
        return !d_cplx_equal(_z.m_core, _w.m_core);
    }

    friend D_CONSTEXPR_CPP14 bool
    operator!=(
        const complex& _z,
        Type           _x
    ) D_NOEXCEPT
    {
        return !d_cplx_equal(_z.m_core,
                             d_cplx_make(_x, static_cast<Type>(0)));
    }

    friend D_CONSTEXPR_CPP14 bool
    operator!=(
        Type           _x,
        const complex& _z
    ) D_NOEXCEPT
    {
        return !d_cplx_equal(_z.m_core,
                             d_cplx_make(_x, static_cast<Type>(0)));
    }

    // ---- C99's functions, exact ------------------------------------------

    // the real part and the imaginary, as creal and cimag.
    friend D_CONSTEXPR_CPP14 Type
    real(
        const complex& _z
    ) D_NOEXCEPT
    {
        return d_cplx_real(_z.m_core);
    }

    friend D_CONSTEXPR_CPP14 Type
    imag(
        const complex& _z
    ) D_NOEXCEPT
    {
        return d_cplx_imag(_z.m_core);
    }

    // the conjugate, as conj; the projection on the Riemann sphere, as
    // cproj: an infinity in either part gives +inf + 0i, the 0 of the
    // imaginary part's sign.
    friend D_CONSTEXPR_CPP14 complex
    conj(
        const complex& _z
    ) D_NOEXCEPT
    {
        return complex(d_cplx_conj(_z.m_core));
    }

    friend D_CONSTEXPR_CPP14 complex
    proj(
        const complex& _z
    ) D_NOEXCEPT
    {
        return complex(d_cplx_proj(_z.m_core));
    }

    // ---- C99's functions, the _c forms -----------------------------------

    // the modulus, as cabs, and the argument, in [-pi, pi], as carg.
    friend D_CONSTEXPR_CPP14 Type
    abs(
        const complex& _z
    ) D_NOEXCEPT
    {
        return d_cplx_abs_c(_z.m_core);
    }

    friend D_CONSTEXPR_CPP14 Type
    arg(
        const complex& _z
    ) D_NOEXCEPT
    {
        return d_cplx_arg_c(_z.m_core);
    }

    // e^_z, the principal logarithm, the principal square root.
    friend D_CONSTEXPR_CPP14 complex
    exp(
        const complex& _z
    ) D_NOEXCEPT
    {
        return complex(d_cplx_exp_c(_z.m_core));
    }

    friend D_CONSTEXPR_CPP14 complex
    log(
        const complex& _z
    ) D_NOEXCEPT
    {
        return complex(d_cplx_log_c(_z.m_core));
    }

    friend D_CONSTEXPR_CPP14 complex
    sqrt(
        const complex& _z
    ) D_NOEXCEPT
    {
        return complex(d_cplx_sqrt_c(_z.m_core));
    }

    // _z ^ _w, the principal value, as cpow: exp(_w log _z); a real operand
    // that number plus 0i.
    friend D_CONSTEXPR_CPP14 complex
    pow(
        const complex& _z,
        const complex& _w
    ) D_NOEXCEPT
    {
        return complex(d_cplx_pow_c(_z.m_core, _w.m_core));
    }

    friend D_CONSTEXPR_CPP14 complex
    pow(
        const complex& _z,
        Type           _x
    ) D_NOEXCEPT
    {
        return complex(d_cplx_pow_c(_z.m_core,
                                    d_cplx_make(_x, static_cast<Type>(0))));
    }

    friend D_CONSTEXPR_CPP14 complex
    pow(
        Type           _x,
        const complex& _w
    ) D_NOEXCEPT
    {
        return complex(d_cplx_pow_c(d_cplx_make(_x, static_cast<Type>(0)),
                                    _w.m_core));
    }

    // the trigonometric functions and their principal inverses.
    friend D_CONSTEXPR_CPP14 complex
    sin(
        const complex& _z
    ) D_NOEXCEPT
    {
        return complex(d_cplx_sin_c(_z.m_core));
    }

    friend D_CONSTEXPR_CPP14 complex
    cos(
        const complex& _z
    ) D_NOEXCEPT
    {
        return complex(d_cplx_cos_c(_z.m_core));
    }

    friend D_CONSTEXPR_CPP14 complex
    tan(
        const complex& _z
    ) D_NOEXCEPT
    {
        return complex(d_cplx_tan_c(_z.m_core));
    }

    friend D_CONSTEXPR_CPP14 complex
    asin(
        const complex& _z
    ) D_NOEXCEPT
    {
        return complex(d_cplx_asin_c(_z.m_core));
    }

    friend D_CONSTEXPR_CPP14 complex
    acos(
        const complex& _z
    ) D_NOEXCEPT
    {
        return complex(d_cplx_acos_c(_z.m_core));
    }

    friend D_CONSTEXPR_CPP14 complex
    atan(
        const complex& _z
    ) D_NOEXCEPT
    {
        return complex(d_cplx_atan_c(_z.m_core));
    }

    // the hyperbolic functions and their principal inverses.
    friend D_CONSTEXPR_CPP14 complex
    sinh(
        const complex& _z
    ) D_NOEXCEPT
    {
        return complex(d_cplx_sinh_c(_z.m_core));
    }

    friend D_CONSTEXPR_CPP14 complex
    cosh(
        const complex& _z
    ) D_NOEXCEPT
    {
        return complex(d_cplx_cosh_c(_z.m_core));
    }

    friend D_CONSTEXPR_CPP14 complex
    tanh(
        const complex& _z
    ) D_NOEXCEPT
    {
        return complex(d_cplx_tanh_c(_z.m_core));
    }

    friend D_CONSTEXPR_CPP14 complex
    asinh(
        const complex& _z
    ) D_NOEXCEPT
    {
        return complex(d_cplx_asinh_c(_z.m_core));
    }

    friend D_CONSTEXPR_CPP14 complex
    acosh(
        const complex& _z
    ) D_NOEXCEPT
    {
        return complex(d_cplx_acosh_c(_z.m_core));
    }

    friend D_CONSTEXPR_CPP14 complex
    atanh(
        const complex& _z
    ) D_NOEXCEPT
    {
        return complex(d_cplx_atanh_c(_z.m_core));
    }

    // the five C99 has only for real arguments, of a complex one

    friend D_CONSTEXPR_CPP14 complex
    exp2(
        const complex& _z
    ) D_NOEXCEPT
    {
        return complex(d_cplx_exp2_c(_z.m_core));
    }

    friend D_CONSTEXPR_CPP14 complex
    log2(
        const complex& _z
    ) D_NOEXCEPT
    {
        return complex(d_cplx_log2_c(_z.m_core));
    }

    friend D_CONSTEXPR_CPP14 complex
    log10(
        const complex& _z
    ) D_NOEXCEPT
    {
        return complex(d_cplx_log10_c(_z.m_core));
    }

    friend D_CONSTEXPR_CPP14 complex
    log1p(
        const complex& _z
    ) D_NOEXCEPT
    {
        return complex(d_cplx_log1p_c(_z.m_core));
    }

    friend D_CONSTEXPR_CPP14 complex
    expm1(
        const complex& _z
    ) D_NOEXCEPT
    {
        return complex(d_cplx_expm1_c(_z.m_core));
    }

private:
    core_type m_core;
};


//==============================================================================
// 2.  NUMBERS
//==============================================================================


// 2.1    number_traits
//------------------------------------------------------------------------------
// number_traits<complex<Type> >
//   trait: a complex number is a commutative field, inexact, with no order;
// its magnitude, of Type, is |re| + |im|, the magnitude LAPACK's izamax and
// cmat.h pivot by, no larger than sqrt 2 times the modulus and needing no
// root; its epsilon Type's, its tolerance four of them, and the tolerance
// of a pair that times the larger magnitude. Having no order, it has no
// abs of its own type and no root through double: the subframework's code
// that would order the numbers it is given, or take their root, is refused
// at compile time.
template<typename Type>
struct number_traits<complex<Type> >
{
    typedef Type magnitude_type;

    static const bool is_specialized = true;
    static const bool is_exact       = false;
    static const bool is_integer     = false;
    static const bool is_field       = true;
    static const bool is_commutative = true;
    static const bool is_ordered     = false;

    // the additive and multiplicative identities.
    static D_CONSTEXPR_CPP14 complex<Type>
    zero() D_NOEXCEPT
    {
        return complex<Type>();
    }

    static D_CONSTEXPR_CPP14 complex<Type>
    one() D_NOEXCEPT
    {
        return complex<Type>(static_cast<Type>(1));
    }

    // what a pivot is chosen by: |re| + |im|.
    static D_CONSTEXPR_CPP14 Type
    magnitude(
        const complex<Type>& _z
    ) D_NOEXCEPT
    {
        return number_traits<Type>::abs(_z.real()) +
               number_traits<Type>::abs(_z.imag());
    }

    // the type's precision, and a value's: Type's epsilon.
    static D_CONSTEXPR Type
    epsilon() D_NOEXCEPT
    {
        return number_traits<Type>::epsilon();
    }

    static D_CONSTEXPR Type
    epsilon(
        const complex<Type>& _z
    ) D_NOEXCEPT
    {
        return (static_cast<void>(_z), number_traits<Type>::epsilon());
    }

    // the relative tolerance of a comparison given none: four epsilons.
    static D_CONSTEXPR Type
    tolerance() D_NOEXCEPT
    {
        return number_traits<Type>::tolerance();
    }

    // the tolerance for comparing _a and _b: four epsilons times the larger
    // magnitude.
    static D_CONSTEXPR_CPP14 Type
    tolerance(
        const complex<Type>& _a,
        const complex<Type>& _b
    ) D_NOEXCEPT
    {
        const Type ma = magnitude(_a);
        const Type mb = magnitude(_b);

        return tolerance() * ((ma < mb) ? mb : ma);
    }
};

// 2.2    common_number
//------------------------------------------------------------------------------
// common_number of a complex number and its real type, either way round, or
// of two: the complex type, at every level, as a + b is.
template<typename Type>
struct common_number<complex<Type>, complex<Type> >
{
    typedef complex<Type> type;
};

template<typename Type>
struct common_number<complex<Type>, Type>
{
    typedef complex<Type> type;
};

template<typename Type>
struct common_number<Type, complex<Type> >
{
    typedef complex<Type> type;
};


NS_END  // math
NS_END  // djinterp


#endif  // DJINTERP_MATH_COMPLEX_HPP
