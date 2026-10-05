/*******************************************************************************
* djinterp [re_std]                                           numeric_limits.hpp
*
* the numeric_limits<T> trait for every fundamental arithmetic type:
*   a self-contained numeric_limits with per-type specialisations for bool, the
*   character types, every signed/unsigned integer, and float / double / long
*   double, plus cv-qualified passthroughs. Integer limits are derived from the
*   type itself (a generic internal base) so signedness-implementation-defined
*   char / wchar_t are handled automatically with no <climits> value dependency;
*   floating-point limits read the compiler-predefined macros (falling back to
*   <cfloat>), and infinity / NaN use compiler builtins. Works on C++98 up; the
*   C++11 lowest() observer is back-ported to every tier (RE_STD AHEAD OF STD).
*
*
* path:      /inc/re_std/limits/numeric_limits.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.06.05
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_LIMITS_NUMERIC_LIMITS_HPP
#define RE_STD_LIMITS_NUMERIC_LIMITS_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration
// re_std
#include "float_round_style.hpp"
#include "float_denorm_style.hpp"

// ---- value sources -------------------------------------------------------
//   Primary path: compiler-predefined macros (zero standard headers, the
// re_std ideal). Fallback path: the C fundamental-limits headers
// <climits> / <cfloat> on compilers that do not predefine them (e.g. MSVC).
#if defined(__CHAR_BIT__)
    #define RE_STD_CHAR_BIT __CHAR_BIT__
#else
    // std
    #include <climits>
    #define RE_STD_CHAR_BIT CHAR_BIT
#endif

#if defined(__FLT_MANT_DIG__)
    #define RE_STD_FLT_MANT_DIG  __FLT_MANT_DIG__
    #define RE_STD_FLT_DIG  __FLT_DIG__
    #define RE_STD_FLT_MIN_EXP  __FLT_MIN_EXP__
    #define RE_STD_FLT_MIN_10_EXP  __FLT_MIN_10_EXP__
    #define RE_STD_FLT_MAX_EXP  __FLT_MAX_EXP__
    #define RE_STD_FLT_MAX_10_EXP  __FLT_MAX_10_EXP__
    #define RE_STD_FLT_MAX  __FLT_MAX__
    #define RE_STD_FLT_MIN  __FLT_MIN__
    #define RE_STD_FLT_EPSILON  __FLT_EPSILON__
    #define RE_STD_FLT_DENORM_MIN  __FLT_DENORM_MIN__
    #if defined(__FLT_HAS_INFINITY__)
        #define RE_STD_FLT_HAS_INF  __FLT_HAS_INFINITY__
    #else
        #define RE_STD_FLT_HAS_INF  1
    #endif
    #if defined(__FLT_HAS_QUIET_NAN__)
        #define RE_STD_FLT_HAS_QNAN  __FLT_HAS_QUIET_NAN__
    #else
        #define RE_STD_FLT_HAS_QNAN  1
    #endif
    #if defined(__FLT_HAS_DENORM__)
        #define RE_STD_FLT_HAS_DENORM  __FLT_HAS_DENORM__
    #else
        #define RE_STD_FLT_HAS_DENORM  1
    #endif
    #define RE_STD_DBL_MANT_DIG  __DBL_MANT_DIG__
    #define RE_STD_DBL_DIG  __DBL_DIG__
    #define RE_STD_DBL_MIN_EXP  __DBL_MIN_EXP__
    #define RE_STD_DBL_MIN_10_EXP  __DBL_MIN_10_EXP__
    #define RE_STD_DBL_MAX_EXP  __DBL_MAX_EXP__
    #define RE_STD_DBL_MAX_10_EXP  __DBL_MAX_10_EXP__
    #define RE_STD_DBL_MAX  __DBL_MAX__
    #define RE_STD_DBL_MIN  __DBL_MIN__
    #define RE_STD_DBL_EPSILON  __DBL_EPSILON__
    #define RE_STD_DBL_DENORM_MIN  __DBL_DENORM_MIN__
    #if defined(__DBL_HAS_INFINITY__)
        #define RE_STD_DBL_HAS_INF  __DBL_HAS_INFINITY__
    #else
        #define RE_STD_DBL_HAS_INF  1
    #endif
    #if defined(__DBL_HAS_QUIET_NAN__)
        #define RE_STD_DBL_HAS_QNAN  __DBL_HAS_QUIET_NAN__
    #else
        #define RE_STD_DBL_HAS_QNAN  1
    #endif
    #if defined(__DBL_HAS_DENORM__)
        #define RE_STD_DBL_HAS_DENORM  __DBL_HAS_DENORM__
    #else
        #define RE_STD_DBL_HAS_DENORM  1
    #endif
    #define RE_STD_LDBL_MANT_DIG  __LDBL_MANT_DIG__
    #define RE_STD_LDBL_DIG  __LDBL_DIG__
    #define RE_STD_LDBL_MIN_EXP  __LDBL_MIN_EXP__
    #define RE_STD_LDBL_MIN_10_EXP  __LDBL_MIN_10_EXP__
    #define RE_STD_LDBL_MAX_EXP  __LDBL_MAX_EXP__
    #define RE_STD_LDBL_MAX_10_EXP  __LDBL_MAX_10_EXP__
    #define RE_STD_LDBL_MAX  __LDBL_MAX__
    #define RE_STD_LDBL_MIN  __LDBL_MIN__
    #define RE_STD_LDBL_EPSILON  __LDBL_EPSILON__
    #define RE_STD_LDBL_DENORM_MIN  __LDBL_DENORM_MIN__
    #if defined(__LDBL_HAS_INFINITY__)
        #define RE_STD_LDBL_HAS_INF  __LDBL_HAS_INFINITY__
    #else
        #define RE_STD_LDBL_HAS_INF  1
    #endif
    #if defined(__LDBL_HAS_QUIET_NAN__)
        #define RE_STD_LDBL_HAS_QNAN  __LDBL_HAS_QUIET_NAN__
    #else
        #define RE_STD_LDBL_HAS_QNAN  1
    #endif
    #if defined(__LDBL_HAS_DENORM__)
        #define RE_STD_LDBL_HAS_DENORM  __LDBL_HAS_DENORM__
    #else
        #define RE_STD_LDBL_HAS_DENORM  1
    #endif
#else
    // std
    #include <cfloat>
    #define RE_STD_FLT_MANT_DIG  FLT_MANT_DIG
    #define RE_STD_FLT_DIG  FLT_DIG
    #define RE_STD_FLT_MIN_EXP  FLT_MIN_EXP
    #define RE_STD_FLT_MIN_10_EXP  FLT_MIN_10_EXP
    #define RE_STD_FLT_MAX_EXP  FLT_MAX_EXP
    #define RE_STD_FLT_MAX_10_EXP  FLT_MAX_10_EXP
    #define RE_STD_FLT_MAX  FLT_MAX
    #define RE_STD_FLT_MIN  FLT_MIN
    #define RE_STD_FLT_EPSILON  FLT_EPSILON
    #if defined(FLT_TRUE_MIN)
        #define RE_STD_FLT_DENORM_MIN  FLT_TRUE_MIN
    #else
        #define RE_STD_FLT_DENORM_MIN  FLT_MIN  // degraded: no subnormal min
    #endif
    #define RE_STD_FLT_HAS_INF     1
    #define RE_STD_FLT_HAS_QNAN    1
    #define RE_STD_FLT_HAS_DENORM  1
    #define RE_STD_DBL_MANT_DIG  DBL_MANT_DIG
    #define RE_STD_DBL_DIG  DBL_DIG
    #define RE_STD_DBL_MIN_EXP  DBL_MIN_EXP
    #define RE_STD_DBL_MIN_10_EXP  DBL_MIN_10_EXP
    #define RE_STD_DBL_MAX_EXP  DBL_MAX_EXP
    #define RE_STD_DBL_MAX_10_EXP  DBL_MAX_10_EXP
    #define RE_STD_DBL_MAX  DBL_MAX
    #define RE_STD_DBL_MIN  DBL_MIN
    #define RE_STD_DBL_EPSILON  DBL_EPSILON
    #if defined(DBL_TRUE_MIN)
        #define RE_STD_DBL_DENORM_MIN  DBL_TRUE_MIN
    #else
        #define RE_STD_DBL_DENORM_MIN  DBL_MIN  // degraded: no subnormal min
    #endif
    #define RE_STD_DBL_HAS_INF     1
    #define RE_STD_DBL_HAS_QNAN    1
    #define RE_STD_DBL_HAS_DENORM  1
    #define RE_STD_LDBL_MANT_DIG  LDBL_MANT_DIG
    #define RE_STD_LDBL_DIG  LDBL_DIG
    #define RE_STD_LDBL_MIN_EXP  LDBL_MIN_EXP
    #define RE_STD_LDBL_MIN_10_EXP  LDBL_MIN_10_EXP
    #define RE_STD_LDBL_MAX_EXP  LDBL_MAX_EXP
    #define RE_STD_LDBL_MAX_10_EXP  LDBL_MAX_10_EXP
    #define RE_STD_LDBL_MAX  LDBL_MAX
    #define RE_STD_LDBL_MIN  LDBL_MIN
    #define RE_STD_LDBL_EPSILON  LDBL_EPSILON
    #if defined(LDBL_TRUE_MIN)
        #define RE_STD_LDBL_DENORM_MIN  LDBL_TRUE_MIN
    #else
        #define RE_STD_LDBL_DENORM_MIN  LDBL_MIN  // degraded: no subnormal min
    #endif
    #define RE_STD_LDBL_HAS_INF     1
    #define RE_STD_LDBL_HAS_QNAN    1
    #define RE_STD_LDBL_HAS_DENORM  1
#endif

// infinity / NaN need compiler builtins (no portable header source).
#if defined(__has_builtin)
    #if __has_builtin(__builtin_huge_valf)
        #define RE_STD_LIMITS_BUILTINS 1
    #endif
#endif
#if !defined(RE_STD_LIMITS_BUILTINS)
    #if ( defined(__GNUC__) || defined(__clang__) )
        #define RE_STD_LIMITS_BUILTINS 1
    #else
        #define RE_STD_LIMITS_BUILTINS 0
    #endif
#endif

namespace re_std
{


    // numeric_limits
    //   trait: primary template. For every non-arithmetic type, is_specialized
    // is false and all members are zero / false (matching std).
    template<typename Type>
    struct numeric_limits
    {
        static RE_STD_CONSTEXPR const bool is_specialized = false;
        static RE_STD_CONSTEXPR const bool is_signed      = false;
        static RE_STD_CONSTEXPR const bool is_integer     = false;
        static RE_STD_CONSTEXPR const bool is_exact       = false;
        static RE_STD_CONSTEXPR const int  radix          = 0;
        static RE_STD_CONSTEXPR const int  digits         = 0;
        static RE_STD_CONSTEXPR const int  digits10       = 0;
        static RE_STD_CONSTEXPR const int  max_digits10   = 0;
        static RE_STD_CONSTEXPR const int  min_exponent   = 0;
        static RE_STD_CONSTEXPR const int  min_exponent10 = 0;
        static RE_STD_CONSTEXPR const int  max_exponent   = 0;
        static RE_STD_CONSTEXPR const int  max_exponent10 = 0;
        static RE_STD_CONSTEXPR const bool has_infinity      = false;
        static RE_STD_CONSTEXPR const bool has_quiet_NaN     = false;
        static RE_STD_CONSTEXPR const bool has_signaling_NaN = false;
        static RE_STD_CONSTEXPR const float_denorm_style has_denorm = denorm_absent;
        static RE_STD_CONSTEXPR const bool has_denorm_loss   = false;
        static RE_STD_CONSTEXPR const bool is_iec559  = false;
        static RE_STD_CONSTEXPR const bool is_bounded = false;
        static RE_STD_CONSTEXPR const bool is_modulo  = false;
        static RE_STD_CONSTEXPR const bool traps      = false;
        static RE_STD_CONSTEXPR const bool tinyness_before = false;
        static RE_STD_CONSTEXPR const float_round_style round_style = round_toward_zero;

        static RE_STD_CONSTEXPR Type min()           RE_STD_NOEXCEPT { return Type(); }
        static RE_STD_CONSTEXPR Type max()           RE_STD_NOEXCEPT { return Type(); }
        static RE_STD_CONSTEXPR Type lowest()        RE_STD_NOEXCEPT { return Type(); }
        static RE_STD_CONSTEXPR Type epsilon()       RE_STD_NOEXCEPT { return Type(); }
        static RE_STD_CONSTEXPR Type round_error()   RE_STD_NOEXCEPT { return Type(); }
        static RE_STD_CONSTEXPR Type infinity()      RE_STD_NOEXCEPT { return Type(); }
        static RE_STD_CONSTEXPR Type quiet_NaN()     RE_STD_NOEXCEPT { return Type(); }
        static RE_STD_CONSTEXPR Type signaling_NaN() RE_STD_NOEXCEPT { return Type(); }
        static RE_STD_CONSTEXPR Type denorm_min()    RE_STD_NOEXCEPT { return Type(); }
    };

namespace internal
{

    // limits_widest_uint
    //   typedef: the widest unsigned integer available at this tier; used to
    // build the all-value-bits mask for max() without per-type constants.
#if RE_STD_LANG_IS_CPP11_OR_HIGHER
    typedef unsigned long long limits_widest_uint;
#else
    typedef unsigned long limits_widest_uint;
#endif

    // integer_limits_base
    //   trait: the shared numeric_limits body for every fundamental integer
    // type. Signedness, digit counts and min()/max() are all derived from Type
    // itself (so it is correct for char / wchar_t, whose signedness is
    // implementation-defined) with no dependency on <climits> values.
    template<typename Type>
    struct integer_limits_base
    {
        static RE_STD_CONSTEXPR const bool is_specialized = true;
        // signed iff -1 compares below 1 (avoids the always-false `< 0` warning).
        static RE_STD_CONSTEXPR const bool is_signed =
            ( static_cast<Type>(-1) < static_cast<Type>(1) );
        static RE_STD_CONSTEXPR const bool is_integer = true;
        static RE_STD_CONSTEXPR const bool is_exact   = true;
        static RE_STD_CONSTEXPR const int  radix      = 2;
        static RE_STD_CONSTEXPR const int  digits =
            ( static_cast<int>(sizeof(Type) * RE_STD_CHAR_BIT) - (is_signed ? 1 : 0) );
        static RE_STD_CONSTEXPR const int  digits10     = digits * 643 / 2136;
        static RE_STD_CONSTEXPR const int  max_digits10 = 0;
        static RE_STD_CONSTEXPR const int  min_exponent   = 0;
        static RE_STD_CONSTEXPR const int  min_exponent10 = 0;
        static RE_STD_CONSTEXPR const int  max_exponent   = 0;
        static RE_STD_CONSTEXPR const int  max_exponent10 = 0;
        static RE_STD_CONSTEXPR const bool has_infinity      = false;
        static RE_STD_CONSTEXPR const bool has_quiet_NaN     = false;
        static RE_STD_CONSTEXPR const bool has_signaling_NaN = false;
        static RE_STD_CONSTEXPR const float_denorm_style has_denorm = denorm_absent;
        static RE_STD_CONSTEXPR const bool has_denorm_loss = false;
        static RE_STD_CONSTEXPR const bool is_iec559  = false;
        static RE_STD_CONSTEXPR const bool is_bounded = true;
        static RE_STD_CONSTEXPR const bool is_modulo  = !is_signed;
        static RE_STD_CONSTEXPR const bool traps      = true;
        static RE_STD_CONSTEXPR const bool tinyness_before = false;
        static RE_STD_CONSTEXPR const float_round_style round_style = round_toward_zero;

        static RE_STD_CONSTEXPR Type max() RE_STD_NOEXCEPT
        {
            // all `digits` value-bits set, masked out of an all-ones widest uint.
            return static_cast<Type>(
                ( ~static_cast<limits_widest_uint>(0) ) >>
                ( static_cast<int>(sizeof(limits_widest_uint)) * RE_STD_CHAR_BIT - digits ) );
        }
        static RE_STD_CONSTEXPR Type min() RE_STD_NOEXCEPT
        {
            return is_signed ? static_cast<Type>( -max() - 1 )
                             : static_cast<Type>(0);
        }
        static RE_STD_CONSTEXPR Type lowest()        RE_STD_NOEXCEPT { return min(); }
        static RE_STD_CONSTEXPR Type epsilon()       RE_STD_NOEXCEPT { return static_cast<Type>(0); }
        static RE_STD_CONSTEXPR Type round_error()   RE_STD_NOEXCEPT { return static_cast<Type>(0); }
        static RE_STD_CONSTEXPR Type infinity()      RE_STD_NOEXCEPT { return static_cast<Type>(0); }
        static RE_STD_CONSTEXPR Type quiet_NaN()     RE_STD_NOEXCEPT { return static_cast<Type>(0); }
        static RE_STD_CONSTEXPR Type signaling_NaN() RE_STD_NOEXCEPT { return static_cast<Type>(0); }
        static RE_STD_CONSTEXPR Type denorm_min()    RE_STD_NOEXCEPT { return static_cast<Type>(0); }
    };

}  // internal

    // numeric_limits<bool>
    //   trait: specialisation for bool (digits = 1, not modulo).
    template<>
    struct numeric_limits<bool>
    {
        static RE_STD_CONSTEXPR const bool is_specialized = true;
        static RE_STD_CONSTEXPR const bool is_signed   = false;
        static RE_STD_CONSTEXPR const bool is_integer  = true;
        static RE_STD_CONSTEXPR const bool is_exact    = true;
        static RE_STD_CONSTEXPR const int  radix       = 2;
        static RE_STD_CONSTEXPR const int  digits      = 1;
        static RE_STD_CONSTEXPR const int  digits10    = 0;
        static RE_STD_CONSTEXPR const int  max_digits10 = 0;
        static RE_STD_CONSTEXPR const int  min_exponent   = 0;
        static RE_STD_CONSTEXPR const int  min_exponent10 = 0;
        static RE_STD_CONSTEXPR const int  max_exponent   = 0;
        static RE_STD_CONSTEXPR const int  max_exponent10 = 0;
        static RE_STD_CONSTEXPR const bool has_infinity      = false;
        static RE_STD_CONSTEXPR const bool has_quiet_NaN     = false;
        static RE_STD_CONSTEXPR const bool has_signaling_NaN = false;
        static RE_STD_CONSTEXPR const float_denorm_style has_denorm = denorm_absent;
        static RE_STD_CONSTEXPR const bool has_denorm_loss = false;
        static RE_STD_CONSTEXPR const bool is_iec559  = false;
        static RE_STD_CONSTEXPR const bool is_bounded = true;
        static RE_STD_CONSTEXPR const bool is_modulo  = false;
        static RE_STD_CONSTEXPR const bool traps      = true;
        static RE_STD_CONSTEXPR const bool tinyness_before = false;
        static RE_STD_CONSTEXPR const float_round_style round_style = round_toward_zero;

        static RE_STD_CONSTEXPR bool min()           RE_STD_NOEXCEPT { return false; }
        static RE_STD_CONSTEXPR bool max()           RE_STD_NOEXCEPT { return true; }
        static RE_STD_CONSTEXPR bool lowest()        RE_STD_NOEXCEPT { return false; }
        static RE_STD_CONSTEXPR bool epsilon()       RE_STD_NOEXCEPT { return false; }
        static RE_STD_CONSTEXPR bool round_error()   RE_STD_NOEXCEPT { return false; }
        static RE_STD_CONSTEXPR bool infinity()      RE_STD_NOEXCEPT { return false; }
        static RE_STD_CONSTEXPR bool quiet_NaN()     RE_STD_NOEXCEPT { return false; }
        static RE_STD_CONSTEXPR bool signaling_NaN() RE_STD_NOEXCEPT { return false; }
        static RE_STD_CONSTEXPR bool denorm_min()    RE_STD_NOEXCEPT { return false; }
    };
    // numeric_limits<char>
    //   trait: specialisation for char (via integer_limits_base).
    template<>
    struct numeric_limits<char> : internal::integer_limits_base<char>
    {
    };
    // numeric_limits<signed char>
    //   trait: specialisation for signed char (via integer_limits_base).
    template<>
    struct numeric_limits<signed char> : internal::integer_limits_base<signed char>
    {
    };
    // numeric_limits<unsigned char>
    //   trait: specialisation for unsigned char (via integer_limits_base).
    template<>
    struct numeric_limits<unsigned char> : internal::integer_limits_base<unsigned char>
    {
    };
    // numeric_limits<wchar_t>
    //   trait: specialisation for wchar_t (via integer_limits_base).
    template<>
    struct numeric_limits<wchar_t> : internal::integer_limits_base<wchar_t>
    {
    };
    // numeric_limits<short>
    //   trait: specialisation for short (via integer_limits_base).
    template<>
    struct numeric_limits<short> : internal::integer_limits_base<short>
    {
    };
    // numeric_limits<unsigned short>
    //   trait: specialisation for unsigned short (via integer_limits_base).
    template<>
    struct numeric_limits<unsigned short> : internal::integer_limits_base<unsigned short>
    {
    };
    // numeric_limits<int>
    //   trait: specialisation for int (via integer_limits_base).
    template<>
    struct numeric_limits<int> : internal::integer_limits_base<int>
    {
    };
    // numeric_limits<unsigned int>
    //   trait: specialisation for unsigned int (via integer_limits_base).
    template<>
    struct numeric_limits<unsigned int> : internal::integer_limits_base<unsigned int>
    {
    };
    // numeric_limits<long>
    //   trait: specialisation for long (via integer_limits_base).
    template<>
    struct numeric_limits<long> : internal::integer_limits_base<long>
    {
    };
    // numeric_limits<unsigned long>
    //   trait: specialisation for unsigned long (via integer_limits_base).
    template<>
    struct numeric_limits<unsigned long> : internal::integer_limits_base<unsigned long>
    {
    };
#if RE_STD_LANG_IS_CPP11_OR_HIGHER
    // numeric_limits<long long>
    //   trait: specialisation for long long (via integer_limits_base).
    template<>
    struct numeric_limits<long long> : internal::integer_limits_base<long long>
    {
    };
    // numeric_limits<unsigned long long>
    //   trait: specialisation for unsigned long long (via integer_limits_base).
    template<>
    struct numeric_limits<unsigned long long> : internal::integer_limits_base<unsigned long long>
    {
    };
#endif  // C++11 (long long)
#if RE_STD_LANG_IS_CPP11_OR_HIGHER
    // numeric_limits<char16_t>
    //   trait: specialisation for char16_t (via integer_limits_base).
    template<>
    struct numeric_limits<char16_t> : internal::integer_limits_base<char16_t>
    {
    };
    // numeric_limits<char32_t>
    //   trait: specialisation for char32_t (via integer_limits_base).
    template<>
    struct numeric_limits<char32_t> : internal::integer_limits_base<char32_t>
    {
    };
#endif  // C++11 (char16_t / char32_t)
#if RE_STD_LANG_IS_CPP20_OR_HIGHER
    // numeric_limits<char8_t>
    //   trait: specialisation for char8_t (via integer_limits_base).
    template<>
    struct numeric_limits<char8_t> : internal::integer_limits_base<char8_t>
    {
    };
#endif  // C++20 (char8_t)

    // numeric_limits<float>
    //   trait: specialisation for float. Constant values come from the FLT_*
    // macros (compiler-predefined, else <cfloat>); infinity / NaN come from
    // compiler builtins when available, degrading to a documented best effort.
    template<>
    struct numeric_limits<float>
    {
        static RE_STD_CONSTEXPR const bool is_specialized = true;
        static RE_STD_CONSTEXPR const bool is_signed   = true;
        static RE_STD_CONSTEXPR const bool is_integer  = false;
        static RE_STD_CONSTEXPR const bool is_exact    = false;
        static RE_STD_CONSTEXPR const int  radix       = 2;
        static RE_STD_CONSTEXPR const int  digits       = RE_STD_FLT_MANT_DIG;
        static RE_STD_CONSTEXPR const int  digits10     = RE_STD_FLT_DIG;
        static RE_STD_CONSTEXPR const int  max_digits10 = 2 + RE_STD_FLT_MANT_DIG * 643 / 2136;
        static RE_STD_CONSTEXPR const int  min_exponent   = RE_STD_FLT_MIN_EXP;
        static RE_STD_CONSTEXPR const int  min_exponent10 = RE_STD_FLT_MIN_10_EXP;
        static RE_STD_CONSTEXPR const int  max_exponent   = RE_STD_FLT_MAX_EXP;
        static RE_STD_CONSTEXPR const int  max_exponent10 = RE_STD_FLT_MAX_10_EXP;
        static RE_STD_CONSTEXPR const bool has_infinity      = ( RE_STD_FLT_HAS_INF  != 0 );
        static RE_STD_CONSTEXPR const bool has_quiet_NaN     = ( RE_STD_FLT_HAS_QNAN != 0 );
        static RE_STD_CONSTEXPR const bool has_signaling_NaN = ( RE_STD_FLT_HAS_QNAN != 0 );
        static RE_STD_CONSTEXPR const float_denorm_style has_denorm =
            ( RE_STD_FLT_HAS_DENORM != 0 ) ? denorm_present : denorm_absent;
        static RE_STD_CONSTEXPR const bool has_denorm_loss = false;
        static RE_STD_CONSTEXPR const bool is_iec559  = true;
        static RE_STD_CONSTEXPR const bool is_bounded = true;
        static RE_STD_CONSTEXPR const bool is_modulo  = false;
        static RE_STD_CONSTEXPR const bool traps      = false;
        static RE_STD_CONSTEXPR const bool tinyness_before = false;
        static RE_STD_CONSTEXPR const float_round_style round_style = round_to_nearest;

        static RE_STD_CONSTEXPR float min()         RE_STD_NOEXCEPT { return RE_STD_FLT_MIN; }
        static RE_STD_CONSTEXPR float max()         RE_STD_NOEXCEPT { return RE_STD_FLT_MAX; }
        static RE_STD_CONSTEXPR float lowest()      RE_STD_NOEXCEPT { return -RE_STD_FLT_MAX; }
        static RE_STD_CONSTEXPR float epsilon()     RE_STD_NOEXCEPT { return RE_STD_FLT_EPSILON; }
        static RE_STD_CONSTEXPR float round_error() RE_STD_NOEXCEPT { return static_cast<float>(0.5); }
        static RE_STD_CONSTEXPR float denorm_min()  RE_STD_NOEXCEPT { return RE_STD_FLT_DENORM_MIN; }
#if RE_STD_LIMITS_BUILTINS
        static RE_STD_CONSTEXPR float infinity()      RE_STD_NOEXCEPT { return __builtin_huge_valf(); }
        static RE_STD_CONSTEXPR float quiet_NaN()     RE_STD_NOEXCEPT { return __builtin_nanf(""); }
        static RE_STD_CONSTEXPR float signaling_NaN() RE_STD_NOEXCEPT { return __builtin_nansf(""); }
#else
        static RE_STD_CONSTEXPR float infinity()      RE_STD_NOEXCEPT { return RE_STD_FLT_MAX; }
        static RE_STD_CONSTEXPR float quiet_NaN()     RE_STD_NOEXCEPT { return static_cast<float>(0); }
        static RE_STD_CONSTEXPR float signaling_NaN() RE_STD_NOEXCEPT { return static_cast<float>(0); }
#endif
    };

    // numeric_limits<double>
    //   trait: specialisation for double. Constant values come from the DBL_*
    // macros (compiler-predefined, else <cfloat>); infinity / NaN come from
    // compiler builtins when available, degrading to a documented best effort.
    template<>
    struct numeric_limits<double>
    {
        static RE_STD_CONSTEXPR const bool is_specialized = true;
        static RE_STD_CONSTEXPR const bool is_signed   = true;
        static RE_STD_CONSTEXPR const bool is_integer  = false;
        static RE_STD_CONSTEXPR const bool is_exact    = false;
        static RE_STD_CONSTEXPR const int  radix       = 2;
        static RE_STD_CONSTEXPR const int  digits       = RE_STD_DBL_MANT_DIG;
        static RE_STD_CONSTEXPR const int  digits10     = RE_STD_DBL_DIG;
        static RE_STD_CONSTEXPR const int  max_digits10 = 2 + RE_STD_DBL_MANT_DIG * 643 / 2136;
        static RE_STD_CONSTEXPR const int  min_exponent   = RE_STD_DBL_MIN_EXP;
        static RE_STD_CONSTEXPR const int  min_exponent10 = RE_STD_DBL_MIN_10_EXP;
        static RE_STD_CONSTEXPR const int  max_exponent   = RE_STD_DBL_MAX_EXP;
        static RE_STD_CONSTEXPR const int  max_exponent10 = RE_STD_DBL_MAX_10_EXP;
        static RE_STD_CONSTEXPR const bool has_infinity      = ( RE_STD_DBL_HAS_INF  != 0 );
        static RE_STD_CONSTEXPR const bool has_quiet_NaN     = ( RE_STD_DBL_HAS_QNAN != 0 );
        static RE_STD_CONSTEXPR const bool has_signaling_NaN = ( RE_STD_DBL_HAS_QNAN != 0 );
        static RE_STD_CONSTEXPR const float_denorm_style has_denorm =
            ( RE_STD_DBL_HAS_DENORM != 0 ) ? denorm_present : denorm_absent;
        static RE_STD_CONSTEXPR const bool has_denorm_loss = false;
        static RE_STD_CONSTEXPR const bool is_iec559  = true;
        static RE_STD_CONSTEXPR const bool is_bounded = true;
        static RE_STD_CONSTEXPR const bool is_modulo  = false;
        static RE_STD_CONSTEXPR const bool traps      = false;
        static RE_STD_CONSTEXPR const bool tinyness_before = false;
        static RE_STD_CONSTEXPR const float_round_style round_style = round_to_nearest;

        static RE_STD_CONSTEXPR double min()         RE_STD_NOEXCEPT { return RE_STD_DBL_MIN; }
        static RE_STD_CONSTEXPR double max()         RE_STD_NOEXCEPT { return RE_STD_DBL_MAX; }
        static RE_STD_CONSTEXPR double lowest()      RE_STD_NOEXCEPT { return -RE_STD_DBL_MAX; }
        static RE_STD_CONSTEXPR double epsilon()     RE_STD_NOEXCEPT { return RE_STD_DBL_EPSILON; }
        static RE_STD_CONSTEXPR double round_error() RE_STD_NOEXCEPT { return static_cast<double>(0.5); }
        static RE_STD_CONSTEXPR double denorm_min()  RE_STD_NOEXCEPT { return RE_STD_DBL_DENORM_MIN; }
#if RE_STD_LIMITS_BUILTINS
        static RE_STD_CONSTEXPR double infinity()      RE_STD_NOEXCEPT { return __builtin_huge_val(); }
        static RE_STD_CONSTEXPR double quiet_NaN()     RE_STD_NOEXCEPT { return __builtin_nan(""); }
        static RE_STD_CONSTEXPR double signaling_NaN() RE_STD_NOEXCEPT { return __builtin_nans(""); }
#else
        static RE_STD_CONSTEXPR double infinity()      RE_STD_NOEXCEPT { return RE_STD_DBL_MAX; }
        static RE_STD_CONSTEXPR double quiet_NaN()     RE_STD_NOEXCEPT { return static_cast<double>(0); }
        static RE_STD_CONSTEXPR double signaling_NaN() RE_STD_NOEXCEPT { return static_cast<double>(0); }
#endif
    };

    // numeric_limits<long double>
    //   trait: specialisation for long double. Constant values come from the LDBL_*
    // macros (compiler-predefined, else <cfloat>); infinity / NaN come from
    // compiler builtins when available, degrading to a documented best effort.
    template<>
    struct numeric_limits<long double>
    {
        static RE_STD_CONSTEXPR const bool is_specialized = true;
        static RE_STD_CONSTEXPR const bool is_signed   = true;
        static RE_STD_CONSTEXPR const bool is_integer  = false;
        static RE_STD_CONSTEXPR const bool is_exact    = false;
        static RE_STD_CONSTEXPR const int  radix       = 2;
        static RE_STD_CONSTEXPR const int  digits       = RE_STD_LDBL_MANT_DIG;
        static RE_STD_CONSTEXPR const int  digits10     = RE_STD_LDBL_DIG;
        static RE_STD_CONSTEXPR const int  max_digits10 = 2 + RE_STD_LDBL_MANT_DIG * 643 / 2136;
        static RE_STD_CONSTEXPR const int  min_exponent   = RE_STD_LDBL_MIN_EXP;
        static RE_STD_CONSTEXPR const int  min_exponent10 = RE_STD_LDBL_MIN_10_EXP;
        static RE_STD_CONSTEXPR const int  max_exponent   = RE_STD_LDBL_MAX_EXP;
        static RE_STD_CONSTEXPR const int  max_exponent10 = RE_STD_LDBL_MAX_10_EXP;
        static RE_STD_CONSTEXPR const bool has_infinity      = ( RE_STD_LDBL_HAS_INF  != 0 );
        static RE_STD_CONSTEXPR const bool has_quiet_NaN     = ( RE_STD_LDBL_HAS_QNAN != 0 );
        static RE_STD_CONSTEXPR const bool has_signaling_NaN = ( RE_STD_LDBL_HAS_QNAN != 0 );
        static RE_STD_CONSTEXPR const float_denorm_style has_denorm =
            ( RE_STD_LDBL_HAS_DENORM != 0 ) ? denorm_present : denorm_absent;
        static RE_STD_CONSTEXPR const bool has_denorm_loss = false;
        static RE_STD_CONSTEXPR const bool is_iec559  = true;
        static RE_STD_CONSTEXPR const bool is_bounded = true;
        static RE_STD_CONSTEXPR const bool is_modulo  = false;
        static RE_STD_CONSTEXPR const bool traps      = false;
        static RE_STD_CONSTEXPR const bool tinyness_before = false;
        static RE_STD_CONSTEXPR const float_round_style round_style = round_to_nearest;

        static RE_STD_CONSTEXPR long double min()         RE_STD_NOEXCEPT { return RE_STD_LDBL_MIN; }
        static RE_STD_CONSTEXPR long double max()         RE_STD_NOEXCEPT { return RE_STD_LDBL_MAX; }
        static RE_STD_CONSTEXPR long double lowest()      RE_STD_NOEXCEPT { return -RE_STD_LDBL_MAX; }
        static RE_STD_CONSTEXPR long double epsilon()     RE_STD_NOEXCEPT { return RE_STD_LDBL_EPSILON; }
        static RE_STD_CONSTEXPR long double round_error() RE_STD_NOEXCEPT { return static_cast<long double>(0.5); }
        static RE_STD_CONSTEXPR long double denorm_min()  RE_STD_NOEXCEPT { return RE_STD_LDBL_DENORM_MIN; }
#if RE_STD_LIMITS_BUILTINS
        static RE_STD_CONSTEXPR long double infinity()      RE_STD_NOEXCEPT { return __builtin_huge_vall(); }
        static RE_STD_CONSTEXPR long double quiet_NaN()     RE_STD_NOEXCEPT { return __builtin_nanl(""); }
        static RE_STD_CONSTEXPR long double signaling_NaN() RE_STD_NOEXCEPT { return __builtin_nansl(""); }
#else
        static RE_STD_CONSTEXPR long double infinity()      RE_STD_NOEXCEPT { return RE_STD_LDBL_MAX; }
        static RE_STD_CONSTEXPR long double quiet_NaN()     RE_STD_NOEXCEPT { return static_cast<long double>(0); }
        static RE_STD_CONSTEXPR long double signaling_NaN() RE_STD_NOEXCEPT { return static_cast<long double>(0); }
#endif
    };

    // numeric_limits<const Type> / <volatile Type> / <const volatile Type>
    //   trait: cv-qualified passthroughs (inherit the unqualified specialisation).
    template<typename Type>
    struct numeric_limits<const Type> : numeric_limits<Type>
    {
    };

    template<typename Type>
    struct numeric_limits<volatile Type> : numeric_limits<Type>
    {
    };

    template<typename Type>
    struct numeric_limits<const volatile Type> : numeric_limits<Type>
    {
    };

}  // re_std

#undef RE_STD_CHAR_BIT
#undef RE_STD_FLT_MANT_DIG
#undef RE_STD_FLT_DIG
#undef RE_STD_FLT_MIN_EXP
#undef RE_STD_FLT_MIN_10_EXP
#undef RE_STD_FLT_MAX_EXP
#undef RE_STD_FLT_MAX_10_EXP
#undef RE_STD_FLT_MAX
#undef RE_STD_FLT_MIN
#undef RE_STD_FLT_EPSILON
#undef RE_STD_FLT_DENORM_MIN
#undef RE_STD_FLT_HAS_INF
#undef RE_STD_FLT_HAS_QNAN
#undef RE_STD_FLT_HAS_DENORM
#undef RE_STD_DBL_MANT_DIG
#undef RE_STD_DBL_DIG
#undef RE_STD_DBL_MIN_EXP
#undef RE_STD_DBL_MIN_10_EXP
#undef RE_STD_DBL_MAX_EXP
#undef RE_STD_DBL_MAX_10_EXP
#undef RE_STD_DBL_MAX
#undef RE_STD_DBL_MIN
#undef RE_STD_DBL_EPSILON
#undef RE_STD_DBL_DENORM_MIN
#undef RE_STD_DBL_HAS_INF
#undef RE_STD_DBL_HAS_QNAN
#undef RE_STD_DBL_HAS_DENORM
#undef RE_STD_LDBL_MANT_DIG
#undef RE_STD_LDBL_DIG
#undef RE_STD_LDBL_MIN_EXP
#undef RE_STD_LDBL_MIN_10_EXP
#undef RE_STD_LDBL_MAX_EXP
#undef RE_STD_LDBL_MAX_10_EXP
#undef RE_STD_LDBL_MAX
#undef RE_STD_LDBL_MIN
#undef RE_STD_LDBL_EPSILON
#undef RE_STD_LDBL_DENORM_MIN
#undef RE_STD_LDBL_HAS_INF
#undef RE_STD_LDBL_HAS_QNAN
#undef RE_STD_LDBL_HAS_DENORM
#undef RE_STD_LIMITS_BUILTINS

#endif  // RE_STD_LIMITS_NUMERIC_LIMITS_HPP
