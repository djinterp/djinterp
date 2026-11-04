/*******************************************************************************
* djinterp [cpp/math]                                          number_traits.hpp
*
* The static side of the generic value: what generic code needs to know of a
* number type, and which type two numbers combine to.
*   NUMBER_TRAITS<T> answers, from C++98: whether it was written for T
* (is_specialized); the field's traits -- whether T's arithmetic never rounds
* (is_exact), whether T holds integers alone (is_integer), whether its
* quotients are a field's (is_field) and its products commute
* (is_commutative), its zero and one; the order's -- whether < orders it
* (is_ordered) and |x|; the magnitude generic code pivots by (magnitude),
* |x| for an ordered type and, for one with no order, a nonnegative number
* of an ordered type (magnitude_type); its square root, exact for an integer
* type; its conversions to and from double, which roots and angles go
* through where T has none of its own; and its tolerance: the precision a
* value carries (epsilon), the relative tolerance approximate comparisons
* use when given none, a few epsilons (tolerance), and the tolerance for two
* given values, 0 where both are exact (S157, S158). The primary template
* serves any type made from an int and converted to and from double, as the
* linear-algebra subframework's generic path always took them, and calls an
* integral type exact and integer; the standard integer and floating-point
* types have their own; math::value and math::inline_value theirs, in
* value.hpp; any other type, a bignum, a complex number or a quaternion
* among them, joins by specializing it.
*   NUMBER_MODULUS_SQUARED(x) is |x|^2, x * x for a number of a real value;
* a type that holds numbers of no real value overloads it, as value.hpp
* does for a value's complex numbers, so that generic norms are of moduli.
*   COMMON_NUMBER<A, B> is the type of a + b: for two arithmetic types their
* usual arithmetic conversions, from C++98; for a value or an inline value
* with each other or an arithmetic type, value.hpp's, from C++98; for any
* other pair, the type a + b has, from C++11, or a specialization at any
* level.
*   From C++20 the concept number names the types generic code computes
* with.
*
*
* path:      /inc/djinterp/cpp/modules/math/number_traits.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.18
*                                                            revised: 2026.11.02
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  NUMBER TRAITS
    -------------
    1.  Roots and epsilons, chosen by level and type
    2.  number_traits
    3.  The arithmetic types
    4.  The modulus squared
2.  COMMON NUMBER
    -------------
    1.  The arithmetic types
    2.  common_number
3.  NUMBERS
    -------
    1.  number
*/

#ifndef DJINTERP_MATH_NUMBER_TRAITS_HPP
#define DJINTERP_MATH_NUMBER_TRAITS_HPP 1

// std
#include <float.h>                    // FLT_EPSILON, DBL_EPSILON,
                                      // LDBL_EPSILON
// djinterp
#include "../../djinterp.hpp"                    // framework root
#include "../../../c/modules/math/elementary.h"  // d_math_sqrt, d_math_sqrt_c
#include "./math_common.hpp"                     // NS_MATH
// re_std
#include "../../../../re_std/cpp/type_traits/is_arithmetic.hpp"  // is_arithmetic
#include "../../../../re_std/cpp/type_traits/is_integral.hpp"    // is_integral
// std, by level (the level is known only once the root is in)
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    #include <type_traits>            // std::decay, std::is_constant_evaluated
    #include <utility>                // std::declval
#endif
#if D_ENV_LANG_IS_CPP20_OR_HIGHER
    #include <concepts>               // std::copyable, std::convertible_to,
                                      // std::same_as
#endif


NS_DJINTERP
NS_MATH


//==============================================================================
// 1.  NUMBER TRAITS
//==============================================================================


// 1.1    Roots and epsilons, chosen by level and type
//------------------------------------------------------------------------------
NS_INTERNAL

    // number_root
    //   function: the correctly rounded square root of _x, the C core's: its
    // constant-expression form from C++14, the C library's below, and from
    // C++20 whichever the evaluation asks for. The two are the same bit for
    // bit.
    template<typename Type>
    D_CONSTEXPR_CPP14 Type
    number_root(
        Type _x
    ) D_NOEXCEPT
    {
#if D_ENV_LANG_IS_CPP20_OR_HIGHER
        if (std::is_constant_evaluated())
        {
            return d_math_sqrt_c(_x);
        }

        return d_math_sqrt(_x);
#elif D_ENV_LANG_IS_CPP14_OR_HIGHER
        return d_math_sqrt_c(_x);
#else
        return d_math_sqrt(_x);
#endif
    }

    // integer_root
    //   function: the exact square root of the integer _x, rounded down: the
    // largest r with r r <= _x, found from the double's root and then
    // corrected, a step at a time, by r > _x / r and r + 1 <= _x / (r + 1),
    // which no product overflows (S157). 0 for a negative _x, which has no
    // root an integer type holds.
    template<typename Type>
    D_CONSTEXPR_CPP14 Type
    integer_root(
        Type _x
    ) D_NOEXCEPT
    {
        Type r = static_cast<Type>(0);

        // no root, or the root itself
        if (!(_x > static_cast<Type>(1)))
        {
            return (_x < static_cast<Type>(0)) ? static_cast<Type>(0) : _x;
        }

        r = static_cast<Type>(number_root(static_cast<double>(_x)));

        // the double's root at least 1, and corrected to the floor
        if (r < static_cast<Type>(1))
        {
            r = static_cast<Type>(1);
        }

        while (r > static_cast<Type>(_x / r))
        {
            --r;
        }

        while (static_cast<Type>(r + 1) <=
               static_cast<Type>(_x / static_cast<Type>(r + 1)))
        {
            ++r;
        }

        return r;
    }

    // primary_root
    //   trait: the primary template's root: an integral type's exact
    // (integer_root), any other's through double.
    template<typename Type,
             bool     Integral = re_std::is_integral<Type>::value>
    struct primary_root
    {
        static D_CONSTEXPR_CPP14 Type
        of(
            const Type& _x
        )
        {
            return static_cast<Type>(number_root(static_cast<double>(_x)));
        }
    };

    template<typename Type>
    struct primary_root<Type, true>
    {
        static D_CONSTEXPR_CPP14 Type
        of(
            const Type& _x
        )
        {
            return integer_root(_x);
        }
    };

    // floating_epsilon
    //   trait: a floating-point type's epsilon, <float.h>'s: the distance
    // from 1 to the next value up.
    template<typename Type>
    struct floating_epsilon;

    template<>
    struct floating_epsilon<float>
    {
        static D_CONSTEXPR float
        value() D_NOEXCEPT
        {
            return FLT_EPSILON;
        }
    };

    template<>
    struct floating_epsilon<double>
    {
        static D_CONSTEXPR double
        value() D_NOEXCEPT
        {
            return DBL_EPSILON;
        }
    };

    template<>
    struct floating_epsilon<long double>
    {
        static D_CONSTEXPR long double
        value() D_NOEXCEPT
        {
            return LDBL_EPSILON;
        }
    };

NS_END  // internal

// 1.2    number_traits
//------------------------------------------------------------------------------
// number_traits
//   trait: what generic code needs to know of a number type. This primary
// template serves a type nothing was written for, as the linear-algebra
// subframework's generic path always took one: made from an int, converted
// to and from double, its root taken through double, ordered by <, and as
// precise as a double. An integral type it serves, a character type or bool,
// is exact and holds integers, as the language has it, and its root is
// exact.
//   The field's traits: is_exact, is_integer, is_field (x / y times y is x,
// y not 0, for every x: not for integers), is_commutative, zero() and one().
// The order's: is_ordered and abs(x). magnitude(x), of magnitude_type, is
// what generic code pivots by and scales a tolerance by: |x| for an ordered
// type, Type itself; a type with no order (a complex number, a quaternion)
// gives a nonnegative number of an ordered type, a modulus or a sum of
// moduli, and says so. epsilon(x) is the precision x carries, 0 for an
// exact value, and epsilon() the type's; tolerance() is the relative
// tolerance of approximate comparisons given none, a few epsilons -- four
// -- of the type, 0 for an exact one; tolerance(a, b) that times the larger
// magnitude, with the larger epsilon of the two, 0 where both are exact.
// The four are of magnitude_type, as the magnitudes they scale are.
template<typename Type>
struct number_traits
{
    typedef Type magnitude_type;

    static const bool is_specialized = false;
    static const bool is_exact       = re_std::is_integral<Type>::value;
    static const bool is_integer     = re_std::is_integral<Type>::value;
    static const bool is_field       = !re_std::is_integral<Type>::value;
    static const bool is_commutative = true;
    static const bool is_ordered     = true;

    // the additive and multiplicative identities.
    static D_CONSTEXPR Type
    zero()
    {
        return static_cast<Type>(0);
    }

    static D_CONSTEXPR Type
    one()
    {
        return static_cast<Type>(1);
    }

    // |_x|.
    static D_CONSTEXPR Type
    abs(
        const Type& _x
    )
    {
        return (_x < static_cast<Type>(0)) ? -_x : _x;
    }

    // what a pivot is chosen by: |_x|.
    static D_CONSTEXPR Type
    magnitude(
        const Type& _x
    )
    {
        return abs(_x);
    }

    // the nearest double, and back.
    static D_CONSTEXPR double
    to_double(
        const Type& _x
    )
    {
        return static_cast<double>(_x);
    }

    static D_CONSTEXPR Type
    from_double(
        double _x
    )
    {
        return static_cast<Type>(_x);
    }

    // the square root: an integral type's exact, rounded down; any other's
    // through double.
    static D_CONSTEXPR_CPP14 Type
    sqrt(
        const Type& _x
    )
    {
        return internal::primary_root<Type>::of(_x);
    }

    // the type's precision, and a value's: a double's, through which the
    // type goes, or 0 for an integral type.
    static D_CONSTEXPR Type
    epsilon()
    {
        return re_std::is_integral<Type>::value ? static_cast<Type>(0)
                                                : static_cast<Type>(
                                                      DBL_EPSILON);
    }

    static D_CONSTEXPR Type
    epsilon(
        const Type& _x
    )
    {
        return (static_cast<void>(_x), epsilon());
    }

    // the relative tolerance of a comparison given none: four epsilons.
    static D_CONSTEXPR Type
    tolerance()
    {
        return static_cast<Type>(4) * epsilon();
    }

    // the tolerance for comparing _a and _b: four of the larger epsilon,
    // times the larger magnitude.
    static D_CONSTEXPR_CPP14 Type
    tolerance(
        const Type& _a,
        const Type& _b
    )
    {
        const Type ma = magnitude(_a);
        const Type mb = magnitude(_b);

        return tolerance() * ((ma < mb) ? mb : ma);
    }
};

// 1.3    The arithmetic types
//------------------------------------------------------------------------------
NS_INTERNAL

    // integer_number_traits
    //   trait: number_traits for an integer type: exact and ordered, its
    // quotients no field's, its root exact and rounded down (S157), its
    // epsilon and tolerance 0. Signed says whether |x| may negate.
    template<typename Type,
             bool     Signed>
    struct integer_number_traits
    {
        typedef Type magnitude_type;

        static const bool is_specialized = true;
        static const bool is_exact       = true;
        static const bool is_integer     = true;
        static const bool is_field       = false;
        static const bool is_commutative = true;
        static const bool is_ordered     = true;

        static D_CONSTEXPR Type
        zero() D_NOEXCEPT
        {
            return static_cast<Type>(0);
        }

        static D_CONSTEXPR Type
        one() D_NOEXCEPT
        {
            return static_cast<Type>(1);
        }

        static D_CONSTEXPR Type
        abs(
            Type _x
        ) D_NOEXCEPT
        {
            return Signed ? ((_x < static_cast<Type>(0))
                                 ? static_cast<Type>(-_x) : _x)
                          : _x;
        }

        static D_CONSTEXPR Type
        magnitude(
            Type _x
        ) D_NOEXCEPT
        {
            return abs(_x);
        }

        static D_CONSTEXPR double
        to_double(
            Type _x
        ) D_NOEXCEPT
        {
            return static_cast<double>(_x);
        }

        static D_CONSTEXPR Type
        from_double(
            double _x
        ) D_NOEXCEPT
        {
            return static_cast<Type>(_x);
        }

        static D_CONSTEXPR_CPP14 Type
        sqrt(
            Type _x
        ) D_NOEXCEPT
        {
            return integer_root(_x);
        }

        static D_CONSTEXPR Type
        epsilon() D_NOEXCEPT
        {
            return static_cast<Type>(0);
        }

        static D_CONSTEXPR Type
        epsilon(
            Type _x
        ) D_NOEXCEPT
        {
            return (static_cast<void>(_x), static_cast<Type>(0));
        }

        static D_CONSTEXPR Type
        tolerance() D_NOEXCEPT
        {
            return static_cast<Type>(0);
        }

        static D_CONSTEXPR Type
        tolerance(
            Type _a,
            Type _b
        ) D_NOEXCEPT
        {
            return (static_cast<void>(_a),
                    static_cast<void>(_b),
                    static_cast<Type>(0));
        }
    };

    // floating_number_traits
    //   trait: number_traits for float, double and long double: inexact,
    // ordered, a field; their own correctly rounded root, the C core's; their
    // own epsilon, <float.h>'s, and four of it for a relative tolerance
    // (S157).
    template<typename Type>
    struct floating_number_traits
    {
        typedef Type magnitude_type;

        static const bool is_specialized = true;
        static const bool is_exact       = false;
        static const bool is_integer     = false;
        static const bool is_field       = true;
        static const bool is_commutative = true;
        static const bool is_ordered     = true;

        static D_CONSTEXPR Type
        zero() D_NOEXCEPT
        {
            return static_cast<Type>(0);
        }

        static D_CONSTEXPR Type
        one() D_NOEXCEPT
        {
            return static_cast<Type>(1);
        }

        static D_CONSTEXPR Type
        abs(
            Type _x
        ) D_NOEXCEPT
        {
            return (_x < static_cast<Type>(0)) ? -_x : _x;
        }

        static D_CONSTEXPR Type
        magnitude(
            Type _x
        ) D_NOEXCEPT
        {
            return abs(_x);
        }

        static D_CONSTEXPR double
        to_double(
            Type _x
        ) D_NOEXCEPT
        {
            return static_cast<double>(_x);
        }

        static D_CONSTEXPR Type
        from_double(
            double _x
        ) D_NOEXCEPT
        {
            return static_cast<Type>(_x);
        }

        static D_CONSTEXPR_CPP14 Type
        sqrt(
            Type _x
        ) D_NOEXCEPT
        {
            return number_root(_x);
        }

        static D_CONSTEXPR Type
        epsilon() D_NOEXCEPT
        {
            return floating_epsilon<Type>::value();
        }

        static D_CONSTEXPR Type
        epsilon(
            Type _x
        ) D_NOEXCEPT
        {
            return (static_cast<void>(_x), floating_epsilon<Type>::value());
        }

        static D_CONSTEXPR Type
        tolerance() D_NOEXCEPT
        {
            return static_cast<Type>(4) * floating_epsilon<Type>::value();
        }

        static D_CONSTEXPR_CPP14 Type
        tolerance(
            Type _a,
            Type _b
        ) D_NOEXCEPT
        {
            const Type ma = abs(_a);
            const Type mb = abs(_b);

            return tolerance() * ((ma < mb) ? mb : ma);
        }
    };

NS_END  // internal

template<>
struct number_traits<signed char>
    : internal::integer_number_traits<signed char, true>
{};

template<>
struct number_traits<unsigned char>
    : internal::integer_number_traits<unsigned char, false>
{};

template<>
struct number_traits<short>
    : internal::integer_number_traits<short, true>
{};

template<>
struct number_traits<unsigned short>
    : internal::integer_number_traits<unsigned short, false>
{};

template<>
struct number_traits<int>
    : internal::integer_number_traits<int, true>
{};

template<>
struct number_traits<unsigned>
    : internal::integer_number_traits<unsigned, false>
{};

template<>
struct number_traits<long>
    : internal::integer_number_traits<long, true>
{};

template<>
struct number_traits<unsigned long>
    : internal::integer_number_traits<unsigned long, false>
{};

#if D_ENV_HAS_LONG_LONG
D_LONG_LONG_DIAG_PUSH
template<>
struct number_traits<long long>
    : internal::integer_number_traits<long long, true>
{};

template<>
struct number_traits<unsigned long long>
    : internal::integer_number_traits<unsigned long long, false>
{};
D_LONG_LONG_DIAG_POP
#endif

template<>
struct number_traits<float>
    : internal::floating_number_traits<float>
{};

template<>
struct number_traits<double>
    : internal::floating_number_traits<double>
{};

template<>
struct number_traits<long double>
    : internal::floating_number_traits<long double>
{};


// 1.4    The modulus squared
//------------------------------------------------------------------------------
// number_modulus_squared
//   function: |_x|^2, of _x's type, Julia's abs2: _x * _x, which is |_x|^2
// for every number of a real value. A type that holds numbers of no real
// value, as math::value holds complex ones, gives an overload of this name
// beside it, which argument-dependent lookup finds; so generic code's norms
// and distances, which sum it, are of the moduli for every such type (S355).
template<typename Type>
D_CONSTEXPR Type
number_modulus_squared(
    const Type& _x
)
{
    return (_x * _x);
}


//==============================================================================
// 2.  COMMON NUMBER
//==============================================================================


// 2.1    The arithmetic types
//------------------------------------------------------------------------------
// C++98 has no decltype; the type of a + b is told by which overload of
// number_rank it would choose, through sizeof, which does not evaluate it.
// After the usual arithmetic conversions a + b of two arithmetic types is
// one of the nine types below, each an exact match.
NS_INTERNAL

    // number_rank_of
    //   type: a character array whose size names one arithmetic type.
    template<int Rank>
    struct number_rank_of
    {
        typedef char (&type)[Rank];
    };

    // number_rank
    //   function: declared only, for sizeof: the rank of a + b's type.
    number_rank_of<1>::type number_rank(int);
    number_rank_of<2>::type number_rank(unsigned);
    number_rank_of<3>::type number_rank(long);
    number_rank_of<4>::type number_rank(unsigned long);
#if D_ENV_HAS_LONG_LONG
D_LONG_LONG_DIAG_PUSH
    number_rank_of<5>::type number_rank(long long);
    number_rank_of<6>::type number_rank(unsigned long long);
D_LONG_LONG_DIAG_POP
#endif
    number_rank_of<7>::type number_rank(float);
    number_rank_of<8>::type number_rank(double);
    number_rank_of<9>::type number_rank(long double);

    // number_of_rank
    //   trait: the arithmetic type a rank names.
    template<int Rank>
    struct number_of_rank;

    template<>
    struct number_of_rank<1>
    {
        typedef int type;
    };

    template<>
    struct number_of_rank<2>
    {
        typedef unsigned type;
    };

    template<>
    struct number_of_rank<3>
    {
        typedef long type;
    };

    template<>
    struct number_of_rank<4>
    {
        typedef unsigned long type;
    };

#if D_ENV_HAS_LONG_LONG
D_LONG_LONG_DIAG_PUSH
    template<>
    struct number_of_rank<5>
    {
        typedef long long type;
    };

    template<>
    struct number_of_rank<6>
    {
        typedef unsigned long long type;
    };
D_LONG_LONG_DIAG_POP
#endif

    template<>
    struct number_of_rank<7>
    {
        typedef float type;
    };

    template<>
    struct number_of_rank<8>
    {
        typedef double type;
    };

    template<>
    struct number_of_rank<9>
    {
        typedef long double type;
    };

    // number_made
    //   function: declared only, for sizeof: a Type to add.
    template<typename Type>
    Type number_made();

    // arithmetic_number
    //   trait: 1 for an arithmetic type, long long and unsigned long long
    // among them wherever the build has them, which re_std's is_arithmetic
    // leaves out below C++11, where they are no standard type.
    template<typename Type>
    struct arithmetic_number
    {
        enum
        {
            value = (re_std::is_arithmetic<Type>::value) ? 1 : 0
        };
    };

#if D_ENV_HAS_LONG_LONG
D_LONG_LONG_DIAG_PUSH
    template<>
    struct arithmetic_number<long long>
    {
        enum
        {
            value = 1
        };
    };

    template<>
    struct arithmetic_number<unsigned long long>
    {
        enum
        {
            value = 1
        };
    };
D_LONG_LONG_DIAG_POP
#endif

    // arithmetic_common_number
    //   trait: the type of a + b for arithmetic A and B, at every level.
    template<typename A,
             typename B>
    struct arithmetic_common_number
    {
        typedef typename number_of_rank<
            sizeof(number_rank(number_made<A>() + number_made<B>()))>::type
            type;
    };

    // common_number_of
    //   trait: common_number's answer: the arithmetic types' at every level;
    // from C++11, for any other pair, the type of a + b, decayed; below,
    // none, and a type that needs one specializes common_number.
    template<typename A,
             typename B,
             bool     Arithmetic = ( (arithmetic_number<A>::value != 0) &&
                                     (arithmetic_number<B>::value != 0) )>
    struct common_number_of
    {
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
        typedef typename std::decay<
            decltype(std::declval<A>() + std::declval<B>())>::type type;
#endif
    };

    template<typename A,
             typename B>
    struct common_number_of<A, B, true>
        : arithmetic_common_number<A, B>
    {};

NS_END  // internal

// 2.2    common_number
//------------------------------------------------------------------------------
// common_number
//   trait: the type of a + b for numbers of types A and B. For two
// arithmetic types their usual arithmetic conversions at every level; from
// C++11 for any other pair the type a + b has; and wherever a type's
// author specializes it, the type it says.
template<typename A,
         typename B>
struct common_number
    : internal::common_number_of<A, B>
{};

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
// common_number_t
//   type: common_number's type (C++11).
template<typename A,
         typename B>
using common_number_t = typename common_number<A, B>::type;
#endif


//==============================================================================
// 3.  NUMBERS
//==============================================================================


#if D_ENV_LANG_IS_CPP20_OR_HIGHER

// 3.1    number
//------------------------------------------------------------------------------
// number
//   concept: a type that generic code computes with as with a number:
// copyable, with the four operations and negation giving the type back,
// and comparison for equality and order. The arithmetic types but bool are
// numbers, and math::inline_value and math::value; a boolean is a kind of
// value, not a number, as value.h has it.
template<typename Type>
concept number =
    ( std::copyable<Type> &&
      (!std::same_as<std::remove_cv_t<Type>, bool>) &&
      requires(const Type& _a,
               const Type& _b)
      {
          { _a + _b }  -> std::convertible_to<Type>;
          { _a - _b }  -> std::convertible_to<Type>;
          { _a * _b }  -> std::convertible_to<Type>;
          { _a / _b }  -> std::convertible_to<Type>;
          { -_a }      -> std::convertible_to<Type>;
          { _a == _b } -> std::convertible_to<bool>;
          { _a < _b }  -> std::convertible_to<bool>;
      } );

#endif  // D_ENV_LANG_IS_CPP20_OR_HIGHER


NS_END  // math
NS_END  // djinterp


#endif  // DJINTERP_MATH_NUMBER_TRAITS_HPP
