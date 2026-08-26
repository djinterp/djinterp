/******************************************************************************
* re_std [numeric]                                        saturation_arith.hpp
*
*   C++26 saturation arithmetic:
*   `add_sat`, `sub_sat`, `mul_sat`, `div_sat` and `saturate_cast` - integer
* arithmetic that CLAMPS at the type's limits instead of wrapping or invoking
* undefined behaviour.
*
*     add_sat<int8_t>(120, 120)   ->  127   (not -16)
*     sub_sat(0u, 1u)             ->  0     (not 4294967295)
*     saturate_cast<int8_t>(300)  ->  127
*
*   EVERY OPERATION MUST AVOID THE OVERFLOW IT IS DETECTING.
*   That is the whole difficulty: you cannot compute a + b and then look at
* the result, because for signed types that computation is already UB before
* you get to inspect it.  Each function therefore tests the operands BEFORE
* operating:
*
*     add_sat  signed:   overflow iff b > 0 && a > max - b   (and the mirror)
*     sub_sat  signed:   overflow iff b < 0 && a > max + b   (and the mirror)
*              unsigned: underflow iff a < b  - a single comparison
*     mul_sat            done in the widest unsigned magnitude space with a
*                        division check; sign handled separately
*     div_sat            only ONE case can overflow: min / -1
*
*   THE div_sat SURPRISE.
*   Division looks incapable of overflow, but min / -1 is |min| which is one
* past max in two's complement - so it saturates to max.  It is also the case
* that traps at runtime on x86 (SIGFPE), so getting it wrong is not a quiet
* bug.  Division by zero remains UB in std and here: saturation is defined for
* overflow, not for an undefined operand.
*
*   STD IS C++26; re_std IS C++98 (constexpr from C++11).
*   Nothing needs a language feature past C++98 - comparisons, casts, and
* numeric_limits.  D_CONSTEXPR lifts everything to constant expressions from
* C++11, twenty-two years earlier than std.
*
*   TYPE RESTRICTION.
*   Per [numeric.sat], the operand type must be an INTEGER type: bool, the
* character types, and the extended char types are excluded even though they
* are technically integral.  is_sat_integer below enforces that via SFINAE, so
* add_sat('a', 'b') does not compile rather than silently doing something
* surprising with an implementation-defined signedness.
*
*
* path:      /inc/djinterp/re_std/numeric/saturation_arith.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.08.13
******************************************************************************/

#ifndef DJINTERP_RE_STD_NUMERIC_SATURATION_ARITH_
#define DJINTERP_RE_STD_NUMERIC_SATURATION_ARITH_ 1

// re_std
#include "../limits/limits"             // numeric_limits
#include "../type_traits/type_traits.hpp"   // enable_if, is_same, is_signed,
                                            // make_unsigned, remove_cv

NS_RESTD

NS_INTERNAL

    // is_sat_integer
    //   trait: true for the types [numeric.sat] admits - the standard signed
    // and unsigned integer types only.  bool and the character types are
    // integral but NOT integer types for this purpose, so they are excluded
    // by listing what is allowed rather than by negating what is not.
    template<typename _Type> struct is_sat_integer_base            { enum { value = 0 }; };
    template<> struct is_sat_integer_base<signed char>             { enum { value = 1 }; };
    template<> struct is_sat_integer_base<short>                   { enum { value = 1 }; };
    template<> struct is_sat_integer_base<int>                     { enum { value = 1 }; };
    template<> struct is_sat_integer_base<long>                    { enum { value = 1 }; };
    template<> struct is_sat_integer_base<unsigned char>           { enum { value = 1 }; };
    template<> struct is_sat_integer_base<unsigned short>          { enum { value = 1 }; };
    template<> struct is_sat_integer_base<unsigned int>            { enum { value = 1 }; };
    template<> struct is_sat_integer_base<unsigned long>           { enum { value = 1 }; };
#if D_ENV_HAS_LONG_LONG
    //   D_ENV_HAS_LONG_LONG is 1 on every mainstream compiler even at C++98,
    // where `long long` is a documented extension rather than a standard type.
    // That is a deliberate project choice, but it means -Wpedantic emits
    // "ISO C++ 1998 does not support 'long long'" for every specialisation
    // like the two below - noise that trains people to stop reading warnings.
    // Suppressed locally here.
    //
    //   TODO(djinterp): this belongs in djinterp.hpp as a shared
    // D_LONG_LONG_DIAG_PUSH / D_LONG_LONG_DIAG_POP pair - EVERY re_std header
    // that specialises on long long (numeric_limits, make_unsigned,
    // is_integral, ...) hits the same warning and should not each carry its
    // own copy of this block.
    #if defined(__GNUC__) || defined(__clang__)
        #pragma GCC diagnostic push
        #pragma GCC diagnostic ignored "-Wlong-long"
    #endif

    template<> struct is_sat_integer_base<long long>               { enum { value = 1 }; };
    template<> struct is_sat_integer_base<unsigned long long>      { enum { value = 1 }; };

    #if defined(__GNUC__) || defined(__clang__)
        #pragma GCC diagnostic pop
    #endif
#endif

    template<typename _Type>
    struct is_sat_integer
        : is_sat_integer_base<typename remove_cv<_Type>::type>
    {};

    // sat_enable
    //   alias-ish: SFINAE guard used on every public entry point.
    template<typename _Int, typename _Result>
    struct sat_enable
        : enable_if<is_sat_integer<_Int>::value != 0, _Result>
    {};

NS_END  // internal


// =============================================================================
// saturate_cast
// =============================================================================

// saturate_cast
//   function: convert value to _To, clamping to _To's range rather than
// wrapping or truncating.
//
//   The comparisons are written so that no operand is ever converted into a
// type that cannot hold it: each side is compared in a space where both
// values are representable, which is why the signed/unsigned cases are split
// rather than folded into one expression.
template<typename _To, typename _From>
D_NODISCARD D_CONSTEXPR
typename internal::sat_enable<_From, _To>::type
saturate_cast(_From value) D_NOEXCEPT
{
    //   Both trait values are cast to bool before comparison.  A trait whose
    // ::value is an unnamed enum (the C++98-compatible spelling) would
    // otherwise make this a comparison between two DIFFERENT enum types,
    // which -Wenum-compare rightly flags.  Casting also keeps the expression
    // correct if a trait ever reports a non-bool integral.
    return
        // signed -> signed, or unsigned -> unsigned: plain range clamp
        (static_cast<bool>(is_signed<_From>::value)
             == static_cast<bool>(is_signed<_To>::value))
            ? ( (value > static_cast<_From>((numeric_limits<_To>::max)()))
                    ? (numeric_limits<_To>::max)()
              : (value < static_cast<_From>((numeric_limits<_To>::min)()))
                    ? (numeric_limits<_To>::min)()
                    : static_cast<_To>(value) )
        // signed -> unsigned: negatives clamp to 0, then clamp the top
        : (static_cast<bool>(is_signed<_From>::value))
            ? ( (value < static_cast<_From>(0))
                    ? static_cast<_To>(0)
              : (static_cast<typename make_unsigned<_From>::type>(value)
                     > static_cast<typename make_unsigned<_From>::type>(
                           (numeric_limits<_To>::max)()))
                    ? (numeric_limits<_To>::max)()
                    : static_cast<_To>(value) )
        // unsigned -> signed: only the top can overflow
            : ( (value > static_cast<_From>((numeric_limits<_To>::max)()))
                    ? (numeric_limits<_To>::max)()
                    : static_cast<_To>(value) );
}


// =============================================================================
// add_sat / sub_sat
// =============================================================================

// add_sat
//   function: a + b, clamped to _Int's range.
//
//   Signed overflow is detected BEFORE the addition happens - testing the sum
// afterwards would already be undefined behaviour.
template<typename _Int>
D_NODISCARD D_CONSTEXPR
typename internal::sat_enable<_Int, _Int>::type
add_sat(_Int a, _Int b) D_NOEXCEPT
{
    return static_cast<bool>(is_signed<_Int>::value)
        ? ( (b > static_cast<_Int>(0)
             && a > static_cast<_Int>((numeric_limits<_Int>::max)() - b))
                ? (numeric_limits<_Int>::max)()
          : (b < static_cast<_Int>(0)
             && a < static_cast<_Int>((numeric_limits<_Int>::min)() - b))
                ? (numeric_limits<_Int>::min)()
                : static_cast<_Int>(a + b) )
        // unsigned: wrap is defined, so a single post-check is safe and exact
        : ( (static_cast<_Int>(a + b) < a)
                ? (numeric_limits<_Int>::max)()
                : static_cast<_Int>(a + b) );
}

// sub_sat
//   function: a - b, clamped to _Int's range.
template<typename _Int>
D_NODISCARD D_CONSTEXPR
typename internal::sat_enable<_Int, _Int>::type
sub_sat(_Int a, _Int b) D_NOEXCEPT
{
    return static_cast<bool>(is_signed<_Int>::value)
        ? ( (b < static_cast<_Int>(0)
             && a > static_cast<_Int>((numeric_limits<_Int>::max)() + b))
                ? (numeric_limits<_Int>::max)()
          : (b > static_cast<_Int>(0)
             && a < static_cast<_Int>((numeric_limits<_Int>::min)() + b))
                ? (numeric_limits<_Int>::min)()
                : static_cast<_Int>(a - b) )
        // unsigned: the only failure is going below zero
        : ( (a < b) ? static_cast<_Int>(0) : static_cast<_Int>(a - b) );
}


// =============================================================================
// mul_sat / div_sat
// =============================================================================

NS_INTERNAL

    // mul_sat_unsigned
    //   function: unsigned multiply with a division-based overflow check.
    // b == 0 is handled first so the division is never by zero.
    template<typename _Uint>
    D_CONSTEXPR _Uint mul_sat_unsigned(_Uint a, _Uint b, _Uint limit) D_NOEXCEPT
    {
        return (b == static_cast<_Uint>(0))
                   ? static_cast<_Uint>(0)
             : (a > static_cast<_Uint>(limit / b))
                   ? limit
                   : static_cast<_Uint>(a * b);
    }

NS_END  // internal

// mul_sat
//   function: a * b, clamped to _Int's range.
//
//   The signed path works in unsigned MAGNITUDE space, because the magnitude
// of the most negative value is not representable as a positive signed value -
// so negating first would be the very overflow being guarded against.  The
// sign of the result selects which limit applies, and the magnitude bound is
// asymmetric: |min| is one larger than max.
template<typename _Int>
D_NODISCARD D_CONSTEXPR
typename internal::sat_enable<_Int, _Int>::type
mul_sat(_Int a, _Int b) D_NOEXCEPT
{
    typedef typename make_unsigned<_Int>::type _U;
    return static_cast<bool>(is_signed<_Int>::value)
        ? ( (a == static_cast<_Int>(0) || b == static_cast<_Int>(0))
                ? static_cast<_Int>(0)
          // result is negative when exactly one operand is negative
          : ( ((a < static_cast<_Int>(0)) != (b < static_cast<_Int>(0)))
                ? ( ( (a < static_cast<_Int>(0)
                           ? static_cast<_U>(static_cast<_U>(0)
                                             - static_cast<_U>(a))
                           : static_cast<_U>(a))
                      > static_cast<_U>(
                            static_cast<_U>(static_cast<_U>(0)
                                - static_cast<_U>(
                                    (numeric_limits<_Int>::min)()))
                            / (b < static_cast<_Int>(0)
                                   ? static_cast<_U>(static_cast<_U>(0)
                                                     - static_cast<_U>(b))
                                   : static_cast<_U>(b))) )
                        ? (numeric_limits<_Int>::min)()
                        : static_cast<_Int>(a * b) )
                : ( ( (a < static_cast<_Int>(0)
                           ? static_cast<_U>(static_cast<_U>(0)
                                             - static_cast<_U>(a))
                           : static_cast<_U>(a))
                      > static_cast<_U>(
                            static_cast<_U>((numeric_limits<_Int>::max)())
                            / (b < static_cast<_Int>(0)
                                   ? static_cast<_U>(static_cast<_U>(0)
                                                     - static_cast<_U>(b))
                                   : static_cast<_U>(b))) )
                        ? (numeric_limits<_Int>::max)()
                        : static_cast<_Int>(a * b) ) ) )
        : static_cast<_Int>(internal::mul_sat_unsigned<_Int>(
              a, b, (numeric_limits<_Int>::max)()));
}

// div_sat
//   function: a / b, clamped to _Int's range.
//
//   Exactly one division can overflow: min / -1, whose true value is |min| =
// max + 1.  It saturates to max.  Division by zero is undefined here as it is
// in std - saturation covers overflow, not undefined operands.
template<typename _Int>
D_NODISCARD D_CONSTEXPR
typename internal::sat_enable<_Int, _Int>::type
div_sat(_Int a, _Int b) D_NOEXCEPT
{
    return (   static_cast<bool>(is_signed<_Int>::value)
            && a == (numeric_limits<_Int>::min)()
            && b == static_cast<_Int>(-1))
               ? (numeric_limits<_Int>::max)()
               : static_cast<_Int>(a / b);
}

NS_END  // re_std
#endif  // DJINTERP_RE_STD_NUMERIC_SATURATION_ARITH_
