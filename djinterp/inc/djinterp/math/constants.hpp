/******************************************************************************
* djinterp [math]                                                constants.hpp
*
* Compile-time numeric constants and number-base representation.
*   Harvested from the former math.hpp during consolidation. Provides the
* named mathematical constants (pi, e, phi, sqrt2), a compile-time reduced
* rational number type, and the radix / number-base system. Each constant
* type models the expression protocol from expression.hpp, so it composes
* directly into expression trees.
*
* 
* path:      /inc/djinterp/math/constants.hpp
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.02.04
******************************************************************************/

#ifndef DJINTERP_MATH_CONSTANTS_
#define DJINTERP_MATH_CONSTANTS_ 1

// std
#include <cstddef>
#include <cstdint>
#include <ratio>
// djinterp
#include "../core/djinterp.hpp"
#include "./expression.hpp"


NS_DJINTERP  // djinterp
NS_MATH      // math

// ===========================================================================
// I.   Named Mathematical Constants
// ===========================================================================

// pi_constant
//   struct: the mathematical constant pi as an expression node.
struct pi_constant : expression_base<pi_constant>
{
    using value_type = double;

    static constexpr const char* name             = "pi";
    static constexpr value_type  value            = 3.14159265358979323846;
    static constexpr std::size_t arity            = 0;
    static constexpr std::size_t degree           = 0;
    static constexpr bool        is_constant_expr = true;

    template<typename _InputType>
    static constexpr value_type
    evaluate(_InputType) noexcept
    {
        return value;
    }

    using derivative = constant<value_type, static_cast<value_type>(0)>;
};

// e_constant
//   struct: Euler's number e as an expression node.
struct e_constant : expression_base<e_constant>
{
    using value_type = double;

    static constexpr const char* name             = "e";
    static constexpr value_type  value            = 2.71828182845904523536;
    static constexpr std::size_t arity            = 0;
    static constexpr std::size_t degree           = 0;
    static constexpr bool        is_constant_expr = true;

    template<typename _InputType>
    static constexpr value_type
    evaluate(_InputType) noexcept
    {
        return value;
    }

    using derivative = constant<value_type, static_cast<value_type>(0)>;
};

// phi_constant
//   struct: the golden ratio phi as an expression node.
struct phi_constant : expression_base<phi_constant>
{
    using value_type = double;

    static constexpr const char* name             = "phi";
    static constexpr value_type  value            = 1.61803398874989484820;
    static constexpr std::size_t arity            = 0;
    static constexpr std::size_t degree           = 0;
    static constexpr bool        is_constant_expr = true;

    template<typename _InputType>
    static constexpr value_type
    evaluate(_InputType) noexcept
    {
        return value;
    }

    using derivative = constant<value_type, static_cast<value_type>(0)>;
};

// sqrt2_constant
//   struct: the square root of two as an expression node.
struct sqrt2_constant : expression_base<sqrt2_constant>
{
    using value_type = double;

    static constexpr const char* name             = "sqrt2";
    static constexpr value_type  value            = 1.41421356237309504880;
    static constexpr std::size_t arity            = 0;
    static constexpr std::size_t degree           = 0;
    static constexpr bool        is_constant_expr = true;

    template<typename _InputType>
    static constexpr value_type
    evaluate(_InputType) noexcept
    {
        return value;
    }

    using derivative = constant<value_type, static_cast<value_type>(0)>;
};

// pi
//   type: convenience alias for pi_constant.
using pi = pi_constant;

// e
//   type: convenience alias for e_constant.
using e = e_constant;

// phi
//   type: convenience alias for phi_constant.
using phi = phi_constant;

// sqrt2
//   type: convenience alias for sqrt2_constant.
using sqrt2 = sqrt2_constant;


// ===========================================================================
// II.  Typed Constant Value Templates
// ===========================================================================
// Value-level constants for use where a plain numeric literal is wanted in
// a chosen precision (e.g. coordinate-system scale factors). Centralizes
// the pi literal that was previously inlined across the coordinate headers.

// pi_v
//   constant: pi in the requested precision.
template<typename _T = double>
inline constexpr _T pi_v = static_cast<_T>(3.14159265358979323846L);

// e_v
//   constant: Euler's number in the requested precision.
template<typename _T = double>
inline constexpr _T e_v = static_cast<_T>(2.71828182845904523536L);

// phi_v
//   constant: the golden ratio in the requested precision.
template<typename _T = double>
inline constexpr _T phi_v = static_cast<_T>(1.61803398874989484820L);

// sqrt2_v
//   constant: the square root of two in the requested precision.
template<typename _T = double>
inline constexpr _T sqrt2_v = static_cast<_T>(1.41421356237309504880L);

// two_pi_v
//   constant: 2*pi in the requested precision.
template<typename _T = double>
inline constexpr _T two_pi_v = static_cast<_T>(2) * pi_v<_T>;


// ===========================================================================
// III. Compile-Time Rational Number
// ===========================================================================

// rational
//   struct: compile-time reduced rational number (a fraction) as an
// expression node. The fraction is reduced by gcd and the sign is
// normalized onto the numerator at instantiation.
template<std::intmax_t _Numerator,
         std::intmax_t _Denominator = 1>
struct rational : expression_base<rational<_Numerator, _Denominator>>
{
    static_assert(_Denominator != 0,
                  "rational: denominator cannot be zero.");

    using value_type = double;

private:
    static constexpr std::intmax_t
    gcd(std::intmax_t _a, std::intmax_t _b)
    {
        return (_b == 0) ? _a : gcd(_b, _a % _b);
    }

    static constexpr std::intmax_t
    abs_val(std::intmax_t _x)
    {
        return (_x < 0) ? -_x : _x;
    }

    static constexpr std::intmax_t
    sign_val(std::intmax_t _x)
    {
        return (_x < 0) ? -1 : 1;
    }

    static constexpr std::intmax_t common =
        gcd(abs_val(_Numerator), abs_val(_Denominator));
    static constexpr std::intmax_t sign = sign_val(_Denominator);

public:
    static constexpr std::intmax_t numerator   = sign * _Numerator / common;
    static constexpr std::intmax_t denominator = abs_val(_Denominator) / common;

    static constexpr value_type value =
        static_cast<value_type>(numerator) /
        static_cast<value_type>(denominator);

    static constexpr std::size_t arity            = 0;
    static constexpr std::size_t degree           = 0;
    static constexpr bool        is_constant_expr = true;

    template<typename _InputType>
    static constexpr value_type
    evaluate(_InputType) noexcept
    {
        return value;
    }

    using derivative = rational<0, 1>;

    // compile-time fraction arithmetic
    template<std::intmax_t _OtherNum, std::intmax_t _OtherDen>
    using add = rational<numerator * _OtherDen + _OtherNum * denominator,
                         denominator * _OtherDen>;

    template<std::intmax_t _OtherNum, std::intmax_t _OtherDen>
    using subtract = rational<( (numerator * _OtherDen) -
                                (_OtherNum * denominator) ),
                             denominator * _OtherDen>;

    template<std::intmax_t _OtherNum, std::intmax_t _OtherDen>
    using multiply = rational<numerator * _OtherNum,
                             denominator * _OtherDen>;

    template<std::intmax_t _OtherNum, std::intmax_t _OtherDen>
    using divide = rational<numerator * _OtherDen,
                           denominator * _OtherNum>;
};

// ratio_to_rational
//   type: converts a std::ratio to a rational.
template<typename _Ratio>
using ratio_to_rational = rational<_Ratio::num, _Ratio::den>;


// ===========================================================================
// IV.  Number Base / Radix System
// ===========================================================================

// number_base
//   struct: specifies a number base/radix for digit interpretation.
template<typename    _ValueType,
         std::size_t _Radix>
struct number_base
{
    static_assert(_Radix >= 2,  "number_base: radix must be at least 2.");
    static_assert(_Radix <= 36, "number_base: radix must be at most 36.");

    using value_type = _ValueType;

    static constexpr std::size_t radix = _Radix;

    static constexpr value_type
    max_digit() noexcept
    {
        return static_cast<value_type>(_Radix - 1);
    }

    static constexpr bool
    is_valid_digit(value_type _digit) noexcept
    {
        return ( (_digit >= 0) &&
                 (static_cast<std::size_t>(_digit) < _Radix) );
    }
};

// binary_base
//   type: number_base with radix 2.
template<typename _Type = std::size_t>
using binary_base = number_base<_Type, 2>;

// octal_base
//   type: number_base with radix 8.
template<typename _Type = std::size_t>
using octal_base = number_base<_Type, 8>;

// decimal_base
//   type: number_base with radix 10.
template<typename _Type = std::size_t>
using decimal_base = number_base<_Type, 10>;

// hexadecimal_base
//   type: number_base with radix 16.
template<typename _Type = std::size_t>
using hexadecimal_base = number_base<_Type, 16>;

// base
//   type: number_base with an explicit radix.
template<typename    _Type,
         std::size_t _Base>
using base = number_base<_Type, _Base>;


NS_END  // math
NS_END  // djinterp


#endif  // DJINTERP_MATH_CONSTANTS_
