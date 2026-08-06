/******************************************************************************
* djinterp [math]                                                constants.hpp
*
* Mathematical constants, compile-time rationals, and number bases.
*   This is the single source of truth for numeric constants across the math
* module. Constants live in the nested djinterp::math::constants namespace as
* variable templates parameterised on their floating-point type; use them with
* the expression core via constant(constants::pi<double>), etc. The geometry
* and calculus subframeworks both draw their constants from here.
*
*   Also provides a value-holding compile-time rational number (an expression
* leaf) and the number_base / radix system.
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
#include "../djinterp.hpp"
#include "./expression.hpp"


NS_DJINTERP
NS_MATH

// ============================================================================
// I.    NAMED CONSTANTS  (djinterp::math::constants)
// ============================================================================
// Variable templates, parameterised on the floating-point type. These do not
// collide with the geometry subframework's flat pi_v family; that family can
// be expressed as aliases of these (e.g. pi_v<T> == constants::pi<T>).

namespace constants
{
    // pi and friends
    template<typename _T = double>
    D_INLINE_VAR constexpr _T pi =
        static_cast<_T>(3.141592653589793238462643383279502884L);

    template<typename _T = double>
    D_INLINE_VAR constexpr _T two_pi =
        static_cast<_T>(6.283185307179586476925286766559005768L);

    template<typename _T = double>
    D_INLINE_VAR constexpr _T half_pi =
        static_cast<_T>(1.570796326794896619231321691639751442L);

    template<typename _T = double>
    D_INLINE_VAR constexpr _T quarter_pi =
        static_cast<_T>(0.785398163397448309615660845819875721L);

    template<typename _T = double>
    D_INLINE_VAR constexpr _T inv_pi =
        static_cast<_T>(0.318309886183790671537767526745028724L);

    template<typename _T = double>
    D_INLINE_VAR constexpr _T tau =
        static_cast<_T>(6.283185307179586476925286766559005768L);

    // e and logarithms
    template<typename _T = double>
    D_INLINE_VAR constexpr _T e =
        static_cast<_T>(2.718281828459045235360287471352662498L);

    template<typename _T = double>
    D_INLINE_VAR constexpr _T ln2 =
        static_cast<_T>(0.693147180559945309417232121458176568L);

    template<typename _T = double>
    D_INLINE_VAR constexpr _T ln10 =
        static_cast<_T>(2.302585092994045684017991454684364208L);

    template<typename _T = double>
    D_INLINE_VAR constexpr _T log2e =
        static_cast<_T>(1.442695040888963407359924681001892137L);

    template<typename _T = double>
    D_INLINE_VAR constexpr _T log10e =
        static_cast<_T>(0.434294481903251827651128918916605082L);

    // roots
    template<typename _T = double>
    D_INLINE_VAR constexpr _T sqrt2 =
        static_cast<_T>(1.414213562373095048801688724209698079L);

    template<typename _T = double>
    D_INLINE_VAR constexpr _T sqrt3 =
        static_cast<_T>(1.732050807568877293527446341505872367L);

    template<typename _T = double>
    D_INLINE_VAR constexpr _T inv_sqrt2 =
        static_cast<_T>(0.707106781186547524400844362104849039L);

    // other
    template<typename _T = double>
    D_INLINE_VAR constexpr _T euler_gamma =
        static_cast<_T>(0.577215664901532860606512090082402431L);

    template<typename _T = double>
    D_INLINE_VAR constexpr _T golden_ratio =
        static_cast<_T>(1.618033988749894848204586834365638118L);

    // phi: alias spelling of golden_ratio.
    template<typename _T = double>
    D_INLINE_VAR constexpr _T phi =
        static_cast<_T>(1.618033988749894848204586834365638118L);

    // angle conversion factors
    template<typename _T = double>
    D_INLINE_VAR constexpr _T rad_per_deg =
        static_cast<_T>(0.017453292519943295769236907684886127L);

    template<typename _T = double>
    D_INLINE_VAR constexpr _T deg_per_rad =
        static_cast<_T>(57.29577951308232087679815481410517033L);
}  // constants


// ============================================================================
// II.   COMPILE-TIME RATIONAL  (value-holding expression leaf)
// ============================================================================

// rational
//   struct: a compile-time reduced rational number that models the expression
// protocol (arity 0, value_type, operator()), so it composes directly into
// expression trees alongside constant_node. The fraction is reduced by gcd and
// the sign normalised onto the numerator at instantiation.
template<std::intmax_t _Numerator,
         std::intmax_t _Denominator = 1>
struct rational : expression_base<rational<_Numerator, _Denominator>>
{
    static_assert(_Denominator != 0, "rational: denominator cannot be zero.");

    using value_type = double;

private:
    static constexpr std::intmax_t
    gcd_of(std::intmax_t _a, std::intmax_t _b)
    {
        return (_b == 0) ? _a : gcd_of(_b, _a % _b);
    }

    static constexpr std::intmax_t
    abs_of(std::intmax_t _x)
    {
        return (_x < 0) ? -_x : _x;
    }

    static constexpr std::intmax_t common =
        gcd_of(abs_of(_Numerator), abs_of(_Denominator));
    static constexpr std::intmax_t den_sign = (_Denominator < 0) ? -1 : 1;

public:
    static constexpr std::intmax_t numerator   = den_sign * _Numerator / common;
    static constexpr std::intmax_t denominator = abs_of(_Denominator) / common;

    static constexpr value_type value =
        static_cast<value_type>(numerator) /
        static_cast<value_type>(denominator);

    static constexpr std::size_t arity = 0;

    template<typename... _Args>
    D_CONSTEXPR value_type
    operator()(_Args&&...) const noexcept
    {
        return value;
    }

    // compile-time fraction arithmetic (reduced on instantiation of the result)
    template<std::intmax_t _On, std::intmax_t _Od>
    using add = rational<numerator * _Od + _On * denominator,
                         denominator * _Od>;

    template<std::intmax_t _On, std::intmax_t _Od>
    using subtract = rational<numerator * _Od - _On * denominator,
                             denominator * _Od>;

    template<std::intmax_t _On, std::intmax_t _Od>
    using multiply = rational<numerator * _On, denominator * _Od>;

    template<std::intmax_t _On, std::intmax_t _Od>
    using divide = rational<numerator * _Od, denominator * _On>;
};

// ratio_to_rational
//   type: converts a std::ratio to a rational.
template<typename _Ratio>
using ratio_to_rational = rational<_Ratio::num, _Ratio::den>;


// ============================================================================
// III.  NUMBER BASE / RADIX SYSTEM
// ============================================================================

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

template<typename _Type = std::size_t>
using binary_base = number_base<_Type, 2>;

template<typename _Type = std::size_t>
using octal_base = number_base<_Type, 8>;

template<typename _Type = std::size_t>
using decimal_base = number_base<_Type, 10>;

template<typename _Type = std::size_t>
using hexadecimal_base = number_base<_Type, 16>;

template<typename    _Type,
         std::size_t _Base>
using base = number_base<_Type, _Base>;


// ============================================================================
// IV.   NUMBER & BASE TRAITS
// ============================================================================
// Folded in from the retired math_traits.hpp. Only the traits whose target
// types still exist are reproduced here (rational, number_base). The old-model
// traits are NOT carried over -- they are superseded as follows:
//   is_evaluable / is_constant / is_mathematical_constant   -> is_expression
//                                                              (expression.hpp)
//   is_function / is_unary_function / is_binary_function     -> is_function +
//                                                       function_arity (function.hpp)
//   is_polynomial / is_linear / is_quadratic / is_cubic /
//     is_constant_function                                   -> (no degree-based
//                                                       polynomial type in the
//                                                       value-holding model)
//   is_differentiable / is_integrable                        -> the calculus
//                                                       free functions derivative<>(),
//                                                       integrate(), ... (calculus/)
//   has_coordinate_system / is_cartesian / is_polar          -> is_coord_system /
//                                                       are_same_system (coordinate.hpp)
//                                                       and is_coordinate_system (function.hpp)
//   is_differentiable, interval traits                       -> interval.hpp folded traits

NS_INTERNAL

    // constants-local well-formedness probe, scoped under cdetail so it stays
    // distinct from the module's other internal void_t definitions. C++14-safe,
    // keeping this header compilable at C++14.
    namespace cdetail
    {
        template<typename...>
        struct make_void { using type = void; };
        template<typename... _Ts>
        using void_t = typename make_void<_Ts...>::type;
    }

    // member detectors
    template<typename _Type, typename = void>
    struct has_radix : std::false_type {};
    template<typename _Type>
    struct has_radix<_Type, cdetail::void_t<decltype(_Type::radix)>>
        : std::true_type {};

    template<typename _Type, typename = void>
    struct has_numerator : std::false_type {};
    template<typename _Type>
    struct has_numerator<_Type, cdetail::void_t<decltype(_Type::numerator)>>
        : std::true_type {};

    template<typename _Type, typename = void>
    struct has_denominator : std::false_type {};
    template<typename _Type>
    struct has_denominator<_Type, cdetail::void_t<decltype(_Type::denominator)>>
        : std::true_type {};

    template<typename _Type, typename = void>
    struct rational_check : std::false_type {};
    template<typename _Type>
    struct rational_check<_Type, std::enable_if_t<
        has_numerator<_Type>::value && has_denominator<_Type>::value
    >> : std::true_type {};

    template<typename _Type, std::size_t _Base, typename = void>
    struct using_base_check : std::false_type {};
    template<typename _Type, std::size_t _Base>
    struct using_base_check<_Type, _Base, std::enable_if_t<
        has_radix<_Type>::value && (_Type::radix == _Base)
    >> : std::true_type {};

NS_END  // internal

// is_rational
//   trait: _Type models a rational number (exposes numerator and denominator).
template<typename _Type>
struct is_rational : internal::rational_check<_Type> {};

// has_number_base
//   trait: _Type specifies a number base (exposes radix).
template<typename _Type>
struct has_number_base : internal::has_radix<_Type> {};

// is_using_base
//   trait: _Type uses the specific number base _Base.
template<typename    _Type,
         std::size_t _Base>
struct is_using_base : internal::using_base_check<_Type, _Base> {};

// is_binary / is_octal / is_decimal / is_hexadecimal
//   trait: _Type uses base 2 / 8 / 10 / 16 respectively.
template<typename _Type> struct is_binary      : is_using_base<_Type, 2>  {};
template<typename _Type> struct is_octal       : is_using_base<_Type, 8>  {};
template<typename _Type> struct is_decimal     : is_using_base<_Type, 10> {};
template<typename _Type> struct is_hexadecimal : is_using_base<_Type, 16> {};

// number-domain vocabulary (thin std wrappers, kept for API parity).
template<typename _Type> struct is_integer_type        : std::is_integral<_Type>       {};
template<typename _Type> struct is_real_number         : std::is_arithmetic<_Type>     {};
template<typename _Type> struct is_floating_point_type : std::is_floating_point<_Type> {};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
template<typename _Type>
D_INLINE_VAR constexpr bool is_rational_v = is_rational<_Type>::value;
template<typename _Type>
D_INLINE_VAR constexpr bool has_number_base_v = has_number_base<_Type>::value;
template<typename    _Type,
         std::size_t _Base>
D_INLINE_VAR constexpr bool is_using_base_v = is_using_base<_Type, _Base>::value;
template<typename _Type>
D_INLINE_VAR constexpr bool is_binary_v = is_binary<_Type>::value;
template<typename _Type>
D_INLINE_VAR constexpr bool is_octal_v = is_octal<_Type>::value;
template<typename _Type>
D_INLINE_VAR constexpr bool is_decimal_v = is_decimal<_Type>::value;
template<typename _Type>
D_INLINE_VAR constexpr bool is_hexadecimal_v = is_hexadecimal<_Type>::value;
template<typename _Type>
D_INLINE_VAR constexpr bool is_integer_type_v = is_integer_type<_Type>::value;
template<typename _Type>
D_INLINE_VAR constexpr bool is_real_number_v = is_real_number<_Type>::value;
template<typename _Type>
D_INLINE_VAR constexpr bool is_floating_point_type_v = is_floating_point_type<_Type>::value;
#endif

NS_END  // math
NS_END  // djinterp


#endif  // DJINTERP_MATH_CONSTANTS_
