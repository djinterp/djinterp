/*******************************************************************************
* djinterp [cpp/math]                                              constants.hpp
*
* Mathematical constants, compile-time rationals, and number bases.
*   The single source of truth for numeric constants across the math module.
* Every constant is a static member function of constants::of<Type>, in every
* build from C++98, and a constant expression from C++11 -- the form
* std::numeric_limits gives its floating-point values, a class holding no
* floating-point constant before C++11:
*
*     constants::of<double>::pi()    constants::of<float>::ln2()
*
* From C++14 each is also a variable template, its type defaulting to double:
* constants::pi<Type> is constants::of<Type>::pi(). Use them with the
* expression core via constant(constants::pi<double>), etc. The geometry and
* calculus subframeworks draw their constants from here. Each literal has 40
* significant digits, so that it rounds as the constant itself does in every
* format up to a quad's 113 bits.
*
*   rational, a compile-time reduced rational number that is a leaf of an
* expression, exists where the expression core does: from C++14, until
* expression.hpp is ported. number_base and the number traits compile from
* C++98; the alias templates over number_base exist from C++11 (below,
* number_base<Type, 2> is what binary_base<Type> names) and the traits' _v
* variable templates from C++14.
*
*
* path:      /inc/djinterp/cpp/modules/math/constants.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.02.04
*                                                            revised: 2026.10.08
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  NAMED CONSTANTS
    ---------------
    1.  of
    2.  Variable templates (C++14)
2.  COMPILE-TIME RATIONAL (C++14)
    -----------------------------
    1.  rational
    2.  ratio_to_rational
3.  NUMBER BASE
    -----------
    1.  number_base
    2.  Static member definitions
    3.  The common bases (C++11)
4.  NUMBER AND BASE TRAITS
    ----------------------
    1.  Detection helpers
    2.  Traits
    3.  Variable templates (C++14)
*/

#ifndef DJINTERP_MATH_CONSTANTS_HPP
#define DJINTERP_MATH_CONSTANTS_HPP 1

// std
#include <cstddef>                       // std::size_t
// djinterp
#include "../../djinterp.hpp"               // framework root
#include "../../meta/trait_detect.hpp"  // D_TYPE_TRAIT_HAS_STATIC_MEMBER
#if D_ENV_LANG_IS_CPP14_OR_HIGHER
    #include "./expression.hpp"          // expression_base
#endif  // D_ENV_LANG_IS_CPP14_OR_HIGHER
// re_std
#include "../../../../re_std/cpp/cstdint/cstdint.hpp"                // re_std::intmax_t
#include "../../../../re_std/cpp/type_traits/enable_if.hpp"          // enable_if
#include "../../../../re_std/cpp/type_traits/integral_constant.hpp"  // integral_
                                                           // constant
#include "../../../../re_std/cpp/type_traits/is_arithmetic.hpp"      // is_arithmetic
#include "../../../../re_std/cpp/type_traits/is_floating_point.hpp"  // is_floating_
                                                           // point
#include "../../../../re_std/cpp/type_traits/is_integral.hpp"        // is_integral


NS_DJINTERP
NS_MATH


//==============================================================================
// 1.  NAMED CONSTANTS
//==============================================================================
// In the nested namespace djinterp::math::constants. These do not collide
// with the geometry subframework's flat pi_v family, which can be expressed
// through them (pi_v<Type> == constants::pi<Type>).

namespace constants
{

// 1.1    of
//------------------------------------------------------------------------------
// of
//   struct: every named constant, as Type: each a static member function
// returning its 40-digit long double literal converted to Type once.
template<typename Type = double>
struct of
{
    // ---- pi and its multiples ----------------------------------------------

    static D_CONSTEXPR Type
    pi() D_NOEXCEPT
    {
        return static_cast<Type>(3.141592653589793238462643383279502884197L);
    }

    static D_CONSTEXPR Type
    two_pi() D_NOEXCEPT
    {
        return static_cast<Type>(6.283185307179586476925286766559005768394L);
    }

    static D_CONSTEXPR Type
    half_pi() D_NOEXCEPT
    {
        return static_cast<Type>(1.570796326794896619231321691639751442099L);
    }

    static D_CONSTEXPR Type
    quarter_pi() D_NOEXCEPT
    {
        return static_cast<Type>(0.7853981633974483096156608458198757210493L);
    }

    static D_CONSTEXPR Type
    inv_pi() D_NOEXCEPT
    {
        return static_cast<Type>(0.3183098861837906715377675267450287240689L);
    }

    // tau
    //   2 pi, two_pi's other name.
    static D_CONSTEXPR Type
    tau() D_NOEXCEPT
    {
        return static_cast<Type>(6.283185307179586476925286766559005768394L);
    }

    // ---- e and the logarithms ----------------------------------------------

    static D_CONSTEXPR Type
    e() D_NOEXCEPT
    {
        return static_cast<Type>(2.718281828459045235360287471352662497757L);
    }

    static D_CONSTEXPR Type
    ln2() D_NOEXCEPT
    {
        return static_cast<Type>(0.6931471805599453094172321214581765680755L);
    }

    static D_CONSTEXPR Type
    ln10() D_NOEXCEPT
    {
        return static_cast<Type>(2.302585092994045684017991454684364207601L);
    }

    static D_CONSTEXPR Type
    log2e() D_NOEXCEPT
    {
        return static_cast<Type>(1.442695040888963407359924681001892137427L);
    }

    static D_CONSTEXPR Type
    log10e() D_NOEXCEPT
    {
        return static_cast<Type>(0.4342944819032518276511289189166050822944L);
    }

    // ---- roots -------------------------------------------------------------

    static D_CONSTEXPR Type
    sqrt2() D_NOEXCEPT
    {
        return static_cast<Type>(1.414213562373095048801688724209698078570L);
    }

    static D_CONSTEXPR Type
    sqrt3() D_NOEXCEPT
    {
        return static_cast<Type>(1.732050807568877293527446341505872366943L);
    }

    static D_CONSTEXPR Type
    inv_sqrt2() D_NOEXCEPT
    {
        return static_cast<Type>(0.7071067811865475244008443621048490392848L);
    }

    // ---- others ------------------------------------------------------------

    static D_CONSTEXPR Type
    euler_gamma() D_NOEXCEPT
    {
        return static_cast<Type>(0.5772156649015328606065120900824024310422L);
    }

    static D_CONSTEXPR Type
    golden_ratio() D_NOEXCEPT
    {
        return static_cast<Type>(1.618033988749894848204586834365638117720L);
    }

    // phi
    //   the golden ratio, golden_ratio's other name.
    static D_CONSTEXPR Type
    phi() D_NOEXCEPT
    {
        return static_cast<Type>(1.618033988749894848204586834365638117720L);
    }

    // ---- angle conversion factors ------------------------------------------

    static D_CONSTEXPR Type
    rad_per_deg() D_NOEXCEPT
    {
        return static_cast<Type>(0.01745329251994329576923690768488612713443L);
    }

    static D_CONSTEXPR Type
    deg_per_rad() D_NOEXCEPT
    {
        return static_cast<Type>(57.29577951308232087679815481410517033241L);
    }
};

// 1.2    Variable templates (C++14)
//------------------------------------------------------------------------------
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    // pi ... deg_per_rad
    //   variable template: of<Type>'s constants, Type defaulting to double.
    template<typename Type = double>
    D_INLINE_VAR D_CONSTEXPR_VAR Type pi = of<Type>::pi();

    template<typename Type = double>
    D_INLINE_VAR D_CONSTEXPR_VAR Type two_pi = of<Type>::two_pi();

    template<typename Type = double>
    D_INLINE_VAR D_CONSTEXPR_VAR Type half_pi = of<Type>::half_pi();

    template<typename Type = double>
    D_INLINE_VAR D_CONSTEXPR_VAR Type quarter_pi = of<Type>::quarter_pi();

    template<typename Type = double>
    D_INLINE_VAR D_CONSTEXPR_VAR Type inv_pi = of<Type>::inv_pi();

    template<typename Type = double>
    D_INLINE_VAR D_CONSTEXPR_VAR Type tau = of<Type>::tau();

    template<typename Type = double>
    D_INLINE_VAR D_CONSTEXPR_VAR Type e = of<Type>::e();

    template<typename Type = double>
    D_INLINE_VAR D_CONSTEXPR_VAR Type ln2 = of<Type>::ln2();

    template<typename Type = double>
    D_INLINE_VAR D_CONSTEXPR_VAR Type ln10 = of<Type>::ln10();

    template<typename Type = double>
    D_INLINE_VAR D_CONSTEXPR_VAR Type log2e = of<Type>::log2e();

    template<typename Type = double>
    D_INLINE_VAR D_CONSTEXPR_VAR Type log10e = of<Type>::log10e();

    template<typename Type = double>
    D_INLINE_VAR D_CONSTEXPR_VAR Type sqrt2 = of<Type>::sqrt2();

    template<typename Type = double>
    D_INLINE_VAR D_CONSTEXPR_VAR Type sqrt3 = of<Type>::sqrt3();

    template<typename Type = double>
    D_INLINE_VAR D_CONSTEXPR_VAR Type inv_sqrt2 = of<Type>::inv_sqrt2();

    template<typename Type = double>
    D_INLINE_VAR D_CONSTEXPR_VAR Type euler_gamma = of<Type>::euler_gamma();

    template<typename Type = double>
    D_INLINE_VAR D_CONSTEXPR_VAR Type golden_ratio = of<Type>::golden_ratio();

    template<typename Type = double>
    D_INLINE_VAR D_CONSTEXPR_VAR Type phi = of<Type>::phi();

    template<typename Type = double>
    D_INLINE_VAR D_CONSTEXPR_VAR Type rad_per_deg = of<Type>::rad_per_deg();

    template<typename Type = double>
    D_INLINE_VAR D_CONSTEXPR_VAR Type deg_per_rad = of<Type>::deg_per_rad();

#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

}  // namespace constants


//==============================================================================
// 2.  COMPILE-TIME RATIONAL (C++14)
//==============================================================================
// A leaf of an expression, so it exists where the expression core does: from
// C++14, which is where expression.hpp compiles until it is ported.

#if D_ENV_LANG_IS_CPP14_OR_HIGHER


// 2.1    rational
//------------------------------------------------------------------------------
// rational
//   struct: a compile-time reduced rational number that models the expression
// protocol (arity 0, value_type, operator()), so it composes directly into
// expression trees alongside constant_node. The fraction is reduced by gcd and
// the sign normalised onto the numerator at instantiation.
template<re_std::intmax_t Numerator,
         re_std::intmax_t Denominator = 1>
struct rational : expression_base<rational<Numerator, Denominator>>
{
    static_assert(Denominator != 0, "rational: denominator cannot be zero.");

    using value_type = double;

private:
    static constexpr re_std::intmax_t
    gcd_of(re_std::intmax_t _a, re_std::intmax_t _b)
    {
        return (_b == 0) ? _a : gcd_of(_b, _a % _b);
    }

    static constexpr re_std::intmax_t
    abs_of(re_std::intmax_t _x)
    {
        return (_x < 0) ? -_x : _x;
    }

    static constexpr re_std::intmax_t common =
        gcd_of(abs_of(Numerator), abs_of(Denominator));
    static constexpr re_std::intmax_t den_sign = (Denominator < 0) ? -1 : 1;

public:
    static constexpr re_std::intmax_t numerator   =
        den_sign * Numerator / common;
    static constexpr re_std::intmax_t denominator =
        abs_of(Denominator) / common;

    static constexpr value_type value =
        static_cast<value_type>(numerator) /
        static_cast<value_type>(denominator);

    static constexpr std::size_t arity = 0;

    template<typename... Args>
    D_CONSTEXPR value_type
    operator()(Args&&...) const D_NOEXCEPT
    {
        return value;
    }

    // compile-time fraction arithmetic (reduced on instantiation of the
    // result)
    template<re_std::intmax_t OtherNumerator,
             re_std::intmax_t OtherDenominator>
    using add = rational<numerator * OtherDenominator +
                             OtherNumerator * denominator,
                         denominator * OtherDenominator>;

    template<re_std::intmax_t OtherNumerator,
             re_std::intmax_t OtherDenominator>
    using subtract = rational<numerator * OtherDenominator -
                                  OtherNumerator * denominator,
                              denominator * OtherDenominator>;

    template<re_std::intmax_t OtherNumerator,
             re_std::intmax_t OtherDenominator>
    using multiply = rational<numerator * OtherNumerator,
                              denominator * OtherDenominator>;

    template<re_std::intmax_t OtherNumerator,
             re_std::intmax_t OtherDenominator>
    using divide = rational<numerator * OtherDenominator,
                            denominator * OtherNumerator>;
};

// 2.2    ratio_to_rational
//------------------------------------------------------------------------------
// ratio_to_rational
//   type: a ratio -- std::ratio, re_std::ratio, anything with num and den --
// as a rational.
template<typename Ratio>
using ratio_to_rational = rational<Ratio::num, Ratio::den>;


#endif  // D_ENV_LANG_IS_CPP14_OR_HIGHER


//==============================================================================
// 3.  NUMBER BASE
//==============================================================================


// 3.1    number_base
//------------------------------------------------------------------------------
// number_base
//   struct: a number base, or radix, from 2 to 36, for digit interpretation.
template<typename    ValueType,
         std::size_t Radix>
struct number_base
{
    D_STATIC_ASSERT((Radix >= 2),  "number_base: radix must be at least 2.");
    D_STATIC_ASSERT((Radix <= 36), "number_base: radix must be at most 36.");

    typedef ValueType value_type;

    static D_CONSTEXPR_VAR std::size_t radix = Radix;

    // max_digit
    //   query: the largest digit, radix - 1.
    static D_CONSTEXPR
    value_type max_digit() D_NOEXCEPT
    {
        return static_cast<value_type>(Radix - 1);
    }

    // is_valid_digit
    //   query: whether _digit is a digit of this base, 0 to radix - 1.
    static D_CONSTEXPR
    bool is_valid_digit(
        value_type _digit
    ) D_NOEXCEPT
    {
        return ( (_digit >= 0) &&
                 (static_cast<std::size_t>(_digit) < Radix) );
    }
};

// 3.2    Static member definitions
//------------------------------------------------------------------------------
// As interval.hpp's: a definition below C++17, a redeclaration from it.
template<typename    ValueType,
         std::size_t Radix>
D_CONSTEXPR_VAR std::size_t number_base<ValueType, Radix>::radix;

// 3.3    The common bases (C++11)
//------------------------------------------------------------------------------
// Alias templates, so from C++11; below, number_base<Type, 2> is what
// binary_base<Type> names, and number_base<Type, Base> what base does.
#if D_ENV_LANG_IS_CPP11_OR_HIGHER

    // binary_base ... hexadecimal_base, base
    //   type: number_base with radix 2, 8, 10, 16, or Base.
    template<typename Type = std::size_t>
    using binary_base = number_base<Type, 2>;

    template<typename Type = std::size_t>
    using octal_base = number_base<Type, 8>;

    template<typename Type = std::size_t>
    using decimal_base = number_base<Type, 10>;

    template<typename Type = std::size_t>
    using hexadecimal_base = number_base<Type, 16>;

    template<typename    Type,
             std::size_t Base>
    using base = number_base<Type, Base>;

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


//==============================================================================
// 4.  NUMBER AND BASE TRAITS
//==============================================================================
// Folded in from the retired math_traits.hpp. Only the traits whose target
// types still exist are reproduced here (rational, number_base). The old-model
// traits are not carried over; they are superseded as follows:
//   is_evaluable / is_constant / is_mathematical_constant  -> is_expression
//                                                     (expression.hpp)
//   is_function / is_unary_function / is_binary_function    -> is_function and
//                                                     function_arity
//                                                     (function.hpp)
//   is_polynomial / is_linear / is_quadratic / is_cubic /
//     is_constant_function                                 -> none: the
//                                                     value-holding model has
//                                                     no degree-based
//                                                     polynomial type
//   is_differentiable / is_integrable                      -> the calculus free
//                                                     functions derivative<>(),
//                                                     integrate(), ...
//                                                     (calculus/)
//   has_coordinate_system / is_cartesian / is_polar         -> is_coord_system
//                                                     and are_same_system
//                                                     (coordinate.hpp), and
//                                                     is_coordinate_system
//                                                     (function.hpp)
//   interval traits                                        -> interval.hpp's
//   Each trait below has the same ::value at every level, a non-static member
// counting as absent.


// 4.1    Detection helpers
//------------------------------------------------------------------------------
// A member is found by name with trait_detect.hpp's engines, then counted only
// if it is static -- a pointer to it is an ordinary pointer, not a pointer to
// member -- as interval.hpp's are: by name alone, a non-static member is
// found from C++11 and by GCC at C++98, but not by Clang at C++98.
NS_INTERNAL

    // constants_probe_yes / constants_probe_no
    //   type: the two answers a probe can give, told apart by size.
    typedef char constants_probe_yes;

    struct constants_probe_no
    {
        char answer[2];
    };

    // constants_static_probe
    //   function: declared only, for sizeof: a pointer to an object or a
    // function answers yes, a pointer to member no.
    template<typename Pointee>
    constants_probe_yes constants_static_probe(
        Pointee*
    );
    constants_probe_no constants_static_probe(
        ...
    );

    // constants_names_radix ... constants_names_denominator
    //   helper: whether Type names a radix, numerator or denominator.
    D_TYPE_TRAIT_HAS_STATIC_MEMBER(constants_names_radix, radix)
    D_TYPE_TRAIT_HAS_STATIC_MEMBER(constants_names_numerator, numerator)
    D_TYPE_TRAIT_HAS_STATIC_MEMBER(constants_names_denominator, denominator)

    // constants_has_radix
    //   helper: whether Type has a static member radix.
    template<typename Type,
             bool     Named = constants_names_radix<Type>::value>
    struct constants_has_radix
        : re_std::integral_constant<bool, false>
    {};

    template<typename Type>
    struct constants_has_radix<Type, true>
        : re_std::integral_constant<bool,
              ( sizeof(constants_static_probe(&Type::radix)) ==
                sizeof(constants_probe_yes) )>
    {};

    // constants_has_numerator
    //   helper: whether Type has a static member numerator.
    template<typename Type,
             bool     Named = constants_names_numerator<Type>::value>
    struct constants_has_numerator
        : re_std::integral_constant<bool, false>
    {};

    template<typename Type>
    struct constants_has_numerator<Type, true>
        : re_std::integral_constant<bool,
              ( sizeof(constants_static_probe(&Type::numerator)) ==
                sizeof(constants_probe_yes) )>
    {};

    // constants_has_denominator
    //   helper: whether Type has a static member denominator.
    template<typename Type,
             bool     Named = constants_names_denominator<Type>::value>
    struct constants_has_denominator
        : re_std::integral_constant<bool, false>
    {};

    template<typename Type>
    struct constants_has_denominator<Type, true>
        : re_std::integral_constant<bool,
              ( sizeof(constants_static_probe(&Type::denominator)) ==
                sizeof(constants_probe_yes) )>
    {};

    // constants_using_base
    //   helper: whether Type's radix is a constant equal to Base: false where
    // Type has none, or one that is not a constant comparable with Base (a
    // function, a non-static member), the comparison failing substitution.
    template<typename    Type,
             std::size_t Base,
             typename    Enable = void>
    struct constants_using_base
        : re_std::integral_constant<bool, false>
    {};

    template<typename    Type,
             std::size_t Base>
    struct constants_using_base<Type,
                                Base,
                                typename re_std::enable_if<
                                    (Type::radix == Base)>::type>
        : re_std::integral_constant<bool, true>
    {};

NS_END  // internal

// 4.2    Traits
//------------------------------------------------------------------------------

// is_rational
//   trait: Type models a rational number (has static members numerator and
// denominator).
template<typename Type>
struct is_rational
    : re_std::integral_constant<bool,
          ( (internal::constants_has_numerator<Type>::value) &&
            (internal::constants_has_denominator<Type>::value) )>
{};

// has_number_base
//   trait: Type specifies a number base (has a static member radix).
template<typename Type>
struct has_number_base
    : re_std::integral_constant<bool,
          internal::constants_has_radix<Type>::value>
{};

// is_using_base
//   trait: Type uses the specific number base Base.
template<typename    Type,
         std::size_t Base>
struct is_using_base
    : re_std::integral_constant<bool,
          internal::constants_using_base<Type, Base>::value>
{};

// is_binary / is_octal / is_decimal / is_hexadecimal
//   trait: Type uses base 2 / 8 / 10 / 16 respectively.
template<typename Type> struct is_binary      : is_using_base<Type, 2>  {};
template<typename Type> struct is_octal       : is_using_base<Type, 8>  {};
template<typename Type> struct is_decimal     : is_using_base<Type, 10> {};
template<typename Type> struct is_hexadecimal : is_using_base<Type, 16> {};

// is_integer_type / is_real_number / is_floating_point_type
//   trait: number-domain vocabulary, thin re_std wrappers kept for API
// parity: integral, arithmetic, floating-point.
template<typename Type>
struct is_integer_type : re_std::is_integral<Type>
{};

template<typename Type>
struct is_real_number : re_std::is_arithmetic<Type>
{};

template<typename Type>
struct is_floating_point_type : re_std::is_floating_point<Type>
{};

// 4.3    Variable templates (C++14)
//------------------------------------------------------------------------------
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    // is_rational_v ... is_floating_point_type_v
    //   variable template: the value of each trait above.
    template<typename Type>
    D_INLINE_VAR D_CONSTEXPR_VAR bool is_rational_v = is_rational<Type>::value;

    template<typename Type>
    D_INLINE_VAR D_CONSTEXPR_VAR bool has_number_base_v =
        has_number_base<Type>::value;

    template<typename    Type,
             std::size_t Base>
    D_INLINE_VAR D_CONSTEXPR_VAR bool is_using_base_v =
        is_using_base<Type, Base>::value;

    template<typename Type>
    D_INLINE_VAR D_CONSTEXPR_VAR bool is_binary_v = is_binary<Type>::value;

    template<typename Type>
    D_INLINE_VAR D_CONSTEXPR_VAR bool is_octal_v = is_octal<Type>::value;

    template<typename Type>
    D_INLINE_VAR D_CONSTEXPR_VAR bool is_decimal_v = is_decimal<Type>::value;

    template<typename Type>
    D_INLINE_VAR D_CONSTEXPR_VAR bool is_hexadecimal_v =
        is_hexadecimal<Type>::value;

    template<typename Type>
    D_INLINE_VAR D_CONSTEXPR_VAR bool is_integer_type_v =
        is_integer_type<Type>::value;

    template<typename Type>
    D_INLINE_VAR D_CONSTEXPR_VAR bool is_real_number_v =
        is_real_number<Type>::value;

    template<typename Type>
    D_INLINE_VAR D_CONSTEXPR_VAR bool is_floating_point_type_v =
        is_floating_point_type<Type>::value;

#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES


NS_END  // math
NS_END  // djinterp


#endif  // DJINTERP_MATH_CONSTANTS_HPP
