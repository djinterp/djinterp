/***********************************************************************
* restd                                               numeric_limits.hpp
*
* the numeric_limits<T> trait for every fundamental arithmetic type:
*   a self-contained numeric_limits with per-type specialisations for bool, the
*   character types, every signed/unsigned integer, and float / double / long
*   double, plus cv-qualified passthroughs. Integer limits are derived from the
*   type itself (a generic internal base) so signedness-implementation-defined
*   char / wchar_t are handled automatically with no <climits> value dependency;
*   floating-point limits read the compiler-predefined macros (falling back to
*   <cfloat>), and infinity / NaN use compiler builtins. Works on C++98 up; the
*   C++11 lowest() observer is back-ported to every tier (RESTD AHEAD OF STD).
*
*
* path:      /inc/djinterp/re_std/limits/numeric_limits.hpp
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                       date: 2026.06.05
***********************************************************************/

#ifndef RESTD_LIMITS_NUMERIC_LIMITS_
#define RESTD_LIMITS_NUMERIC_LIMITS_ 1

// djinterp
#include "djinterp.hpp"
// restd
#include "float_round_style.hpp"
#include "float_denorm_style.hpp"

// ---- value sources -------------------------------------------------------
//   Primary path: compiler-predefined macros (zero standard headers, the
// restd ideal). Fallback path: the C fundamental-limits headers
// <climits> / <cfloat> on compilers that do not predefine them (e.g. MSVC).
#if defined(__CHAR_BIT__)
    #define D_RESTD_CHAR_BIT __CHAR_BIT__
#else
    #include <climits>
    #define D_RESTD_CHAR_BIT CHAR_BIT
#endif

#if defined(__FLT_MANT_DIG__)
    #define D_RESTD_FLT_MANT_DIG  __FLT_MANT_DIG__
    #define D_RESTD_FLT_DIG  __FLT_DIG__
    #define D_RESTD_FLT_MIN_EXP  __FLT_MIN_EXP__
    #define D_RESTD_FLT_MIN_10_EXP  __FLT_MIN_10_EXP__
    #define D_RESTD_FLT_MAX_EXP  __FLT_MAX_EXP__
    #define D_RESTD_FLT_MAX_10_EXP  __FLT_MAX_10_EXP__
    #define D_RESTD_FLT_MAX  __FLT_MAX__
    #define D_RESTD_FLT_MIN  __FLT_MIN__
    #define D_RESTD_FLT_EPSILON  __FLT_EPSILON__
    #define D_RESTD_FLT_DENORM_MIN  __FLT_DENORM_MIN__
    #if defined(__FLT_HAS_INFINITY__)
        #define D_RESTD_FLT_HAS_INF  __FLT_HAS_INFINITY__
    #else
        #define D_RESTD_FLT_HAS_INF  1
    #endif
    #if defined(__FLT_HAS_QUIET_NAN__)
        #define D_RESTD_FLT_HAS_QNAN  __FLT_HAS_QUIET_NAN__
    #else
        #define D_RESTD_FLT_HAS_QNAN  1
    #endif
    #if defined(__FLT_HAS_DENORM__)
        #define D_RESTD_FLT_HAS_DENORM  __FLT_HAS_DENORM__
    #else
        #define D_RESTD_FLT_HAS_DENORM  1
    #endif
    #define D_RESTD_DBL_MANT_DIG  __DBL_MANT_DIG__
    #define D_RESTD_DBL_DIG  __DBL_DIG__
    #define D_RESTD_DBL_MIN_EXP  __DBL_MIN_EXP__
    #define D_RESTD_DBL_MIN_10_EXP  __DBL_MIN_10_EXP__
    #define D_RESTD_DBL_MAX_EXP  __DBL_MAX_EXP__
    #define D_RESTD_DBL_MAX_10_EXP  __DBL_MAX_10_EXP__
    #define D_RESTD_DBL_MAX  __DBL_MAX__
    #define D_RESTD_DBL_MIN  __DBL_MIN__
    #define D_RESTD_DBL_EPSILON  __DBL_EPSILON__
    #define D_RESTD_DBL_DENORM_MIN  __DBL_DENORM_MIN__
    #if defined(__DBL_HAS_INFINITY__)
        #define D_RESTD_DBL_HAS_INF  __DBL_HAS_INFINITY__
    #else
        #define D_RESTD_DBL_HAS_INF  1
    #endif
    #if defined(__DBL_HAS_QUIET_NAN__)
        #define D_RESTD_DBL_HAS_QNAN  __DBL_HAS_QUIET_NAN__
    #else
        #define D_RESTD_DBL_HAS_QNAN  1
    #endif
    #if defined(__DBL_HAS_DENORM__)
        #define D_RESTD_DBL_HAS_DENORM  __DBL_HAS_DENORM__
    #else
        #define D_RESTD_DBL_HAS_DENORM  1
    #endif
    #define D_RESTD_LDBL_MANT_DIG  __LDBL_MANT_DIG__
    #define D_RESTD_LDBL_DIG  __LDBL_DIG__
    #define D_RESTD_LDBL_MIN_EXP  __LDBL_MIN_EXP__
    #define D_RESTD_LDBL_MIN_10_EXP  __LDBL_MIN_10_EXP__
    #define D_RESTD_LDBL_MAX_EXP  __LDBL_MAX_EXP__
    #define D_RESTD_LDBL_MAX_10_EXP  __LDBL_MAX_10_EXP__
    #define D_RESTD_LDBL_MAX  __LDBL_MAX__
    #define D_RESTD_LDBL_MIN  __LDBL_MIN__
    #define D_RESTD_LDBL_EPSILON  __LDBL_EPSILON__
    #define D_RESTD_LDBL_DENORM_MIN  __LDBL_DENORM_MIN__
    #if defined(__LDBL_HAS_INFINITY__)
        #define D_RESTD_LDBL_HAS_INF  __LDBL_HAS_INFINITY__
    #else
        #define D_RESTD_LDBL_HAS_INF  1
    #endif
    #if defined(__LDBL_HAS_QUIET_NAN__)
        #define D_RESTD_LDBL_HAS_QNAN  __LDBL_HAS_QUIET_NAN__
    #else
        #define D_RESTD_LDBL_HAS_QNAN  1
    #endif
    #if defined(__LDBL_HAS_DENORM__)
        #define D_RESTD_LDBL_HAS_DENORM  __LDBL_HAS_DENORM__
    #else
        #define D_RESTD_LDBL_HAS_DENORM  1
    #endif
#else
    #include <cfloat>
    #define D_RESTD_FLT_MANT_DIG  FLT_MANT_DIG
    #define D_RESTD_FLT_DIG  FLT_DIG
    #define D_RESTD_FLT_MIN_EXP  FLT_MIN_EXP
    #define D_RESTD_FLT_MIN_10_EXP  FLT_MIN_10_EXP
    #define D_RESTD_FLT_MAX_EXP  FLT_MAX_EXP
    #define D_RESTD_FLT_MAX_10_EXP  FLT_MAX_10_EXP
    #define D_RESTD_FLT_MAX  FLT_MAX
    #define D_RESTD_FLT_MIN  FLT_MIN
    #define D_RESTD_FLT_EPSILON  FLT_EPSILON
    #if defined(FLT_TRUE_MIN)
        #define D_RESTD_FLT_DENORM_MIN  FLT_TRUE_MIN
    #else
        #define D_RESTD_FLT_DENORM_MIN  FLT_MIN  // degraded: no subnormal min
    #endif
    #define D_RESTD_FLT_HAS_INF     1
    #define D_RESTD_FLT_HAS_QNAN    1
    #define D_RESTD_FLT_HAS_DENORM  1
    #define D_RESTD_DBL_MANT_DIG  DBL_MANT_DIG
    #define D_RESTD_DBL_DIG  DBL_DIG
    #define D_RESTD_DBL_MIN_EXP  DBL_MIN_EXP
    #define D_RESTD_DBL_MIN_10_EXP  DBL_MIN_10_EXP
    #define D_RESTD_DBL_MAX_EXP  DBL_MAX_EXP
    #define D_RESTD_DBL_MAX_10_EXP  DBL_MAX_10_EXP
    #define D_RESTD_DBL_MAX  DBL_MAX
    #define D_RESTD_DBL_MIN  DBL_MIN
    #define D_RESTD_DBL_EPSILON  DBL_EPSILON
    #if defined(DBL_TRUE_MIN)
        #define D_RESTD_DBL_DENORM_MIN  DBL_TRUE_MIN
    #else
        #define D_RESTD_DBL_DENORM_MIN  DBL_MIN  // degraded: no subnormal min
    #endif
    #define D_RESTD_DBL_HAS_INF     1
    #define D_RESTD_DBL_HAS_QNAN    1
    #define D_RESTD_DBL_HAS_DENORM  1
    #define D_RESTD_LDBL_MANT_DIG  LDBL_MANT_DIG
    #define D_RESTD_LDBL_DIG  LDBL_DIG
    #define D_RESTD_LDBL_MIN_EXP  LDBL_MIN_EXP
    #define D_RESTD_LDBL_MIN_10_EXP  LDBL_MIN_10_EXP
    #define D_RESTD_LDBL_MAX_EXP  LDBL_MAX_EXP
    #define D_RESTD_LDBL_MAX_10_EXP  LDBL_MAX_10_EXP
    #define D_RESTD_LDBL_MAX  LDBL_MAX
    #define D_RESTD_LDBL_MIN  LDBL_MIN
    #define D_RESTD_LDBL_EPSILON  LDBL_EPSILON
    #if defined(LDBL_TRUE_MIN)
        #define D_RESTD_LDBL_DENORM_MIN  LDBL_TRUE_MIN
    #else
        #define D_RESTD_LDBL_DENORM_MIN  LDBL_MIN  // degraded: no subnormal min
    #endif
    #define D_RESTD_LDBL_HAS_INF     1
    #define D_RESTD_LDBL_HAS_QNAN    1
    #define D_RESTD_LDBL_HAS_DENORM  1
#endif

// infinity / NaN need compiler builtins (no portable header source).
#if defined(__has_builtin)
    #if __has_builtin(__builtin_huge_valf)
        #define D_RESTD_LIMITS_BUILTINS 1
    #endif
#endif
#if !defined(D_RESTD_LIMITS_BUILTINS)
    #if ( defined(__GNUC__) || defined(__clang__) )
        #define D_RESTD_LIMITS_BUILTINS 1
    #else
        #define D_RESTD_LIMITS_BUILTINS 0
    #endif
#endif

NS_RESTD


    // numeric_limits
    //   trait: primary template. For every non-arithmetic type, is_specialized
    // is false and all members are zero / false (matching std).
    template<typename _Type>
    struct numeric_limits
    {
        static D_CONSTEXPR const bool is_specialized = false;
        static D_CONSTEXPR const bool is_signed      = false;
        static D_CONSTEXPR const bool is_integer     = false;
        static D_CONSTEXPR const bool is_exact       = false;
        static D_CONSTEXPR const int  radix          = 0;
        static D_CONSTEXPR const int  digits         = 0;
        static D_CONSTEXPR const int  digits10       = 0;
        static D_CONSTEXPR const int  max_digits10   = 0;
        static D_CONSTEXPR const int  min_exponent   = 0;
        static D_CONSTEXPR const int  min_exponent10 = 0;
        static D_CONSTEXPR const int  max_exponent   = 0;
        static D_CONSTEXPR const int  max_exponent10 = 0;
        static D_CONSTEXPR const bool has_infinity      = false;
        static D_CONSTEXPR const bool has_quiet_NaN     = false;
        static D_CONSTEXPR const bool has_signaling_NaN = false;
        static D_CONSTEXPR const float_denorm_style has_denorm = denorm_absent;
        static D_CONSTEXPR const bool has_denorm_loss   = false;
        static D_CONSTEXPR const bool is_iec559  = false;
        static D_CONSTEXPR const bool is_bounded = false;
        static D_CONSTEXPR const bool is_modulo  = false;
        static D_CONSTEXPR const bool traps      = false;
        static D_CONSTEXPR const bool tinyness_before = false;
        static D_CONSTEXPR const float_round_style round_style = round_toward_zero;

        static D_CONSTEXPR _Type min()           D_NOEXCEPT { return _Type(); }
        static D_CONSTEXPR _Type max()           D_NOEXCEPT { return _Type(); }
        static D_CONSTEXPR _Type lowest()        D_NOEXCEPT { return _Type(); }
        static D_CONSTEXPR _Type epsilon()       D_NOEXCEPT { return _Type(); }
        static D_CONSTEXPR _Type round_error()   D_NOEXCEPT { return _Type(); }
        static D_CONSTEXPR _Type infinity()      D_NOEXCEPT { return _Type(); }
        static D_CONSTEXPR _Type quiet_NaN()     D_NOEXCEPT { return _Type(); }
        static D_CONSTEXPR _Type signaling_NaN() D_NOEXCEPT { return _Type(); }
        static D_CONSTEXPR _Type denorm_min()    D_NOEXCEPT { return _Type(); }
    };

NS_INTERNAL

    // limits_widest_uint
    //   typedef: the widest unsigned integer available at this tier; used to
    // build the all-value-bits mask for max() without per-type constants.
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    typedef unsigned long long limits_widest_uint;
#else
    typedef unsigned long limits_widest_uint;
#endif

    // integer_limits_base
    //   trait: the shared numeric_limits body for every fundamental integer
    // type. Signedness, digit counts and min()/max() are all derived from _Type
    // itself (so it is correct for char / wchar_t, whose signedness is
    // implementation-defined) with no dependency on <climits> values.
    template<typename _Type>
    struct integer_limits_base
    {
        static D_CONSTEXPR const bool is_specialized = true;
        // signed iff -1 compares below 1 (avoids the always-false `< 0` warning).
        static D_CONSTEXPR const bool is_signed =
            ( static_cast<_Type>(-1) < static_cast<_Type>(1) );
        static D_CONSTEXPR const bool is_integer = true;
        static D_CONSTEXPR const bool is_exact   = true;
        static D_CONSTEXPR const int  radix      = 2;
        static D_CONSTEXPR const int  digits =
            ( static_cast<int>(sizeof(_Type) * D_RESTD_CHAR_BIT) - (is_signed ? 1 : 0) );
        static D_CONSTEXPR const int  digits10     = digits * 643 / 2136;
        static D_CONSTEXPR const int  max_digits10 = 0;
        static D_CONSTEXPR const int  min_exponent   = 0;
        static D_CONSTEXPR const int  min_exponent10 = 0;
        static D_CONSTEXPR const int  max_exponent   = 0;
        static D_CONSTEXPR const int  max_exponent10 = 0;
        static D_CONSTEXPR const bool has_infinity      = false;
        static D_CONSTEXPR const bool has_quiet_NaN     = false;
        static D_CONSTEXPR const bool has_signaling_NaN = false;
        static D_CONSTEXPR const float_denorm_style has_denorm = denorm_absent;
        static D_CONSTEXPR const bool has_denorm_loss = false;
        static D_CONSTEXPR const bool is_iec559  = false;
        static D_CONSTEXPR const bool is_bounded = true;
        static D_CONSTEXPR const bool is_modulo  = !is_signed;
        static D_CONSTEXPR const bool traps      = true;
        static D_CONSTEXPR const bool tinyness_before = false;
        static D_CONSTEXPR const float_round_style round_style = round_toward_zero;

        static D_CONSTEXPR _Type max() D_NOEXCEPT
        {
            // all `digits` value-bits set, masked out of an all-ones widest uint.
            return static_cast<_Type>(
                ( ~static_cast<limits_widest_uint>(0) ) >>
                ( static_cast<int>(sizeof(limits_widest_uint)) * D_RESTD_CHAR_BIT - digits ) );
        }
        static D_CONSTEXPR _Type min() D_NOEXCEPT
        {
            return is_signed ? static_cast<_Type>( -max() - 1 )
                             : static_cast<_Type>(0);
        }
        static D_CONSTEXPR _Type lowest()        D_NOEXCEPT { return min(); }
        static D_CONSTEXPR _Type epsilon()       D_NOEXCEPT { return static_cast<_Type>(0); }
        static D_CONSTEXPR _Type round_error()   D_NOEXCEPT { return static_cast<_Type>(0); }
        static D_CONSTEXPR _Type infinity()      D_NOEXCEPT { return static_cast<_Type>(0); }
        static D_CONSTEXPR _Type quiet_NaN()     D_NOEXCEPT { return static_cast<_Type>(0); }
        static D_CONSTEXPR _Type signaling_NaN() D_NOEXCEPT { return static_cast<_Type>(0); }
        static D_CONSTEXPR _Type denorm_min()    D_NOEXCEPT { return static_cast<_Type>(0); }
    };

NS_END  // internal

    // numeric_limits<bool>
    //   trait: specialisation for bool (digits = 1, not modulo).
    template<>
    struct numeric_limits<bool>
    {
        static D_CONSTEXPR const bool is_specialized = true;
        static D_CONSTEXPR const bool is_signed   = false;
        static D_CONSTEXPR const bool is_integer  = true;
        static D_CONSTEXPR const bool is_exact    = true;
        static D_CONSTEXPR const int  radix       = 2;
        static D_CONSTEXPR const int  digits      = 1;
        static D_CONSTEXPR const int  digits10    = 0;
        static D_CONSTEXPR const int  max_digits10 = 0;
        static D_CONSTEXPR const int  min_exponent   = 0;
        static D_CONSTEXPR const int  min_exponent10 = 0;
        static D_CONSTEXPR const int  max_exponent   = 0;
        static D_CONSTEXPR const int  max_exponent10 = 0;
        static D_CONSTEXPR const bool has_infinity      = false;
        static D_CONSTEXPR const bool has_quiet_NaN     = false;
        static D_CONSTEXPR const bool has_signaling_NaN = false;
        static D_CONSTEXPR const float_denorm_style has_denorm = denorm_absent;
        static D_CONSTEXPR const bool has_denorm_loss = false;
        static D_CONSTEXPR const bool is_iec559  = false;
        static D_CONSTEXPR const bool is_bounded = true;
        static D_CONSTEXPR const bool is_modulo  = false;
        static D_CONSTEXPR const bool traps      = true;
        static D_CONSTEXPR const bool tinyness_before = false;
        static D_CONSTEXPR const float_round_style round_style = round_toward_zero;

        static D_CONSTEXPR bool min()           D_NOEXCEPT { return false; }
        static D_CONSTEXPR bool max()           D_NOEXCEPT { return true; }
        static D_CONSTEXPR bool lowest()        D_NOEXCEPT { return false; }
        static D_CONSTEXPR bool epsilon()       D_NOEXCEPT { return false; }
        static D_CONSTEXPR bool round_error()   D_NOEXCEPT { return false; }
        static D_CONSTEXPR bool infinity()      D_NOEXCEPT { return false; }
        static D_CONSTEXPR bool quiet_NaN()     D_NOEXCEPT { return false; }
        static D_CONSTEXPR bool signaling_NaN() D_NOEXCEPT { return false; }
        static D_CONSTEXPR bool denorm_min()    D_NOEXCEPT { return false; }
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
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
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
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
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
#if D_ENV_LANG_IS_CPP20_OR_HIGHER
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
        static D_CONSTEXPR const bool is_specialized = true;
        static D_CONSTEXPR const bool is_signed   = true;
        static D_CONSTEXPR const bool is_integer  = false;
        static D_CONSTEXPR const bool is_exact    = false;
        static D_CONSTEXPR const int  radix       = 2;
        static D_CONSTEXPR const int  digits       = D_RESTD_FLT_MANT_DIG;
        static D_CONSTEXPR const int  digits10     = D_RESTD_FLT_DIG;
        static D_CONSTEXPR const int  max_digits10 = 2 + D_RESTD_FLT_MANT_DIG * 643 / 2136;
        static D_CONSTEXPR const int  min_exponent   = D_RESTD_FLT_MIN_EXP;
        static D_CONSTEXPR const int  min_exponent10 = D_RESTD_FLT_MIN_10_EXP;
        static D_CONSTEXPR const int  max_exponent   = D_RESTD_FLT_MAX_EXP;
        static D_CONSTEXPR const int  max_exponent10 = D_RESTD_FLT_MAX_10_EXP;
        static D_CONSTEXPR const bool has_infinity      = ( D_RESTD_FLT_HAS_INF  != 0 );
        static D_CONSTEXPR const bool has_quiet_NaN     = ( D_RESTD_FLT_HAS_QNAN != 0 );
        static D_CONSTEXPR const bool has_signaling_NaN = ( D_RESTD_FLT_HAS_QNAN != 0 );
        static D_CONSTEXPR const float_denorm_style has_denorm =
            ( D_RESTD_FLT_HAS_DENORM != 0 ) ? denorm_present : denorm_absent;
        static D_CONSTEXPR const bool has_denorm_loss = false;
        static D_CONSTEXPR const bool is_iec559  = true;
        static D_CONSTEXPR const bool is_bounded = true;
        static D_CONSTEXPR const bool is_modulo  = false;
        static D_CONSTEXPR const bool traps      = false;
        static D_CONSTEXPR const bool tinyness_before = false;
        static D_CONSTEXPR const float_round_style round_style = round_to_nearest;

        static D_CONSTEXPR float min()         D_NOEXCEPT { return D_RESTD_FLT_MIN; }
        static D_CONSTEXPR float max()         D_NOEXCEPT { return D_RESTD_FLT_MAX; }
        static D_CONSTEXPR float lowest()      D_NOEXCEPT { return -D_RESTD_FLT_MAX; }
        static D_CONSTEXPR float epsilon()     D_NOEXCEPT { return D_RESTD_FLT_EPSILON; }
        static D_CONSTEXPR float round_error() D_NOEXCEPT { return static_cast<float>(0.5); }
        static D_CONSTEXPR float denorm_min()  D_NOEXCEPT { return D_RESTD_FLT_DENORM_MIN; }
#if D_RESTD_LIMITS_BUILTINS
        static D_CONSTEXPR float infinity()      D_NOEXCEPT { return __builtin_huge_valf(); }
        static D_CONSTEXPR float quiet_NaN()     D_NOEXCEPT { return __builtin_nanf(""); }
        static D_CONSTEXPR float signaling_NaN() D_NOEXCEPT { return __builtin_nansf(""); }
#else
        static D_CONSTEXPR float infinity()      D_NOEXCEPT { return D_RESTD_FLT_MAX; }
        static D_CONSTEXPR float quiet_NaN()     D_NOEXCEPT { return static_cast<float>(0); }
        static D_CONSTEXPR float signaling_NaN() D_NOEXCEPT { return static_cast<float>(0); }
#endif
    };

    // numeric_limits<double>
    //   trait: specialisation for double. Constant values come from the DBL_*
    // macros (compiler-predefined, else <cfloat>); infinity / NaN come from
    // compiler builtins when available, degrading to a documented best effort.
    template<>
    struct numeric_limits<double>
    {
        static D_CONSTEXPR const bool is_specialized = true;
        static D_CONSTEXPR const bool is_signed   = true;
        static D_CONSTEXPR const bool is_integer  = false;
        static D_CONSTEXPR const bool is_exact    = false;
        static D_CONSTEXPR const int  radix       = 2;
        static D_CONSTEXPR const int  digits       = D_RESTD_DBL_MANT_DIG;
        static D_CONSTEXPR const int  digits10     = D_RESTD_DBL_DIG;
        static D_CONSTEXPR const int  max_digits10 = 2 + D_RESTD_DBL_MANT_DIG * 643 / 2136;
        static D_CONSTEXPR const int  min_exponent   = D_RESTD_DBL_MIN_EXP;
        static D_CONSTEXPR const int  min_exponent10 = D_RESTD_DBL_MIN_10_EXP;
        static D_CONSTEXPR const int  max_exponent   = D_RESTD_DBL_MAX_EXP;
        static D_CONSTEXPR const int  max_exponent10 = D_RESTD_DBL_MAX_10_EXP;
        static D_CONSTEXPR const bool has_infinity      = ( D_RESTD_DBL_HAS_INF  != 0 );
        static D_CONSTEXPR const bool has_quiet_NaN     = ( D_RESTD_DBL_HAS_QNAN != 0 );
        static D_CONSTEXPR const bool has_signaling_NaN = ( D_RESTD_DBL_HAS_QNAN != 0 );
        static D_CONSTEXPR const float_denorm_style has_denorm =
            ( D_RESTD_DBL_HAS_DENORM != 0 ) ? denorm_present : denorm_absent;
        static D_CONSTEXPR const bool has_denorm_loss = false;
        static D_CONSTEXPR const bool is_iec559  = true;
        static D_CONSTEXPR const bool is_bounded = true;
        static D_CONSTEXPR const bool is_modulo  = false;
        static D_CONSTEXPR const bool traps      = false;
        static D_CONSTEXPR const bool tinyness_before = false;
        static D_CONSTEXPR const float_round_style round_style = round_to_nearest;

        static D_CONSTEXPR double min()         D_NOEXCEPT { return D_RESTD_DBL_MIN; }
        static D_CONSTEXPR double max()         D_NOEXCEPT { return D_RESTD_DBL_MAX; }
        static D_CONSTEXPR double lowest()      D_NOEXCEPT { return -D_RESTD_DBL_MAX; }
        static D_CONSTEXPR double epsilon()     D_NOEXCEPT { return D_RESTD_DBL_EPSILON; }
        static D_CONSTEXPR double round_error() D_NOEXCEPT { return static_cast<double>(0.5); }
        static D_CONSTEXPR double denorm_min()  D_NOEXCEPT { return D_RESTD_DBL_DENORM_MIN; }
#if D_RESTD_LIMITS_BUILTINS
        static D_CONSTEXPR double infinity()      D_NOEXCEPT { return __builtin_huge_val(); }
        static D_CONSTEXPR double quiet_NaN()     D_NOEXCEPT { return __builtin_nan(""); }
        static D_CONSTEXPR double signaling_NaN() D_NOEXCEPT { return __builtin_nans(""); }
#else
        static D_CONSTEXPR double infinity()      D_NOEXCEPT { return D_RESTD_DBL_MAX; }
        static D_CONSTEXPR double quiet_NaN()     D_NOEXCEPT { return static_cast<double>(0); }
        static D_CONSTEXPR double signaling_NaN() D_NOEXCEPT { return static_cast<double>(0); }
#endif
    };

    // numeric_limits<long double>
    //   trait: specialisation for long double. Constant values come from the LDBL_*
    // macros (compiler-predefined, else <cfloat>); infinity / NaN come from
    // compiler builtins when available, degrading to a documented best effort.
    template<>
    struct numeric_limits<long double>
    {
        static D_CONSTEXPR const bool is_specialized = true;
        static D_CONSTEXPR const bool is_signed   = true;
        static D_CONSTEXPR const bool is_integer  = false;
        static D_CONSTEXPR const bool is_exact    = false;
        static D_CONSTEXPR const int  radix       = 2;
        static D_CONSTEXPR const int  digits       = D_RESTD_LDBL_MANT_DIG;
        static D_CONSTEXPR const int  digits10     = D_RESTD_LDBL_DIG;
        static D_CONSTEXPR const int  max_digits10 = 2 + D_RESTD_LDBL_MANT_DIG * 643 / 2136;
        static D_CONSTEXPR const int  min_exponent   = D_RESTD_LDBL_MIN_EXP;
        static D_CONSTEXPR const int  min_exponent10 = D_RESTD_LDBL_MIN_10_EXP;
        static D_CONSTEXPR const int  max_exponent   = D_RESTD_LDBL_MAX_EXP;
        static D_CONSTEXPR const int  max_exponent10 = D_RESTD_LDBL_MAX_10_EXP;
        static D_CONSTEXPR const bool has_infinity      = ( D_RESTD_LDBL_HAS_INF  != 0 );
        static D_CONSTEXPR const bool has_quiet_NaN     = ( D_RESTD_LDBL_HAS_QNAN != 0 );
        static D_CONSTEXPR const bool has_signaling_NaN = ( D_RESTD_LDBL_HAS_QNAN != 0 );
        static D_CONSTEXPR const float_denorm_style has_denorm =
            ( D_RESTD_LDBL_HAS_DENORM != 0 ) ? denorm_present : denorm_absent;
        static D_CONSTEXPR const bool has_denorm_loss = false;
        static D_CONSTEXPR const bool is_iec559  = true;
        static D_CONSTEXPR const bool is_bounded = true;
        static D_CONSTEXPR const bool is_modulo  = false;
        static D_CONSTEXPR const bool traps      = false;
        static D_CONSTEXPR const bool tinyness_before = false;
        static D_CONSTEXPR const float_round_style round_style = round_to_nearest;

        static D_CONSTEXPR long double min()         D_NOEXCEPT { return D_RESTD_LDBL_MIN; }
        static D_CONSTEXPR long double max()         D_NOEXCEPT { return D_RESTD_LDBL_MAX; }
        static D_CONSTEXPR long double lowest()      D_NOEXCEPT { return -D_RESTD_LDBL_MAX; }
        static D_CONSTEXPR long double epsilon()     D_NOEXCEPT { return D_RESTD_LDBL_EPSILON; }
        static D_CONSTEXPR long double round_error() D_NOEXCEPT { return static_cast<long double>(0.5); }
        static D_CONSTEXPR long double denorm_min()  D_NOEXCEPT { return D_RESTD_LDBL_DENORM_MIN; }
#if D_RESTD_LIMITS_BUILTINS
        static D_CONSTEXPR long double infinity()      D_NOEXCEPT { return __builtin_huge_vall(); }
        static D_CONSTEXPR long double quiet_NaN()     D_NOEXCEPT { return __builtin_nanl(""); }
        static D_CONSTEXPR long double signaling_NaN() D_NOEXCEPT { return __builtin_nansl(""); }
#else
        static D_CONSTEXPR long double infinity()      D_NOEXCEPT { return D_RESTD_LDBL_MAX; }
        static D_CONSTEXPR long double quiet_NaN()     D_NOEXCEPT { return static_cast<long double>(0); }
        static D_CONSTEXPR long double signaling_NaN() D_NOEXCEPT { return static_cast<long double>(0); }
#endif
    };

    // numeric_limits<const _Type> / <volatile _Type> / <const volatile _Type>
    //   trait: cv-qualified passthroughs (inherit the unqualified specialisation).
    template<typename _Type>
    struct numeric_limits<const _Type> : numeric_limits<_Type>
    {
    };

    template<typename _Type>
    struct numeric_limits<volatile _Type> : numeric_limits<_Type>
    {
    };

    template<typename _Type>
    struct numeric_limits<const volatile _Type> : numeric_limits<_Type>
    {
    };

NS_END  // restd

#undef D_RESTD_CHAR_BIT
#undef D_RESTD_FLT_MANT_DIG
#undef D_RESTD_FLT_DIG
#undef D_RESTD_FLT_MIN_EXP
#undef D_RESTD_FLT_MIN_10_EXP
#undef D_RESTD_FLT_MAX_EXP
#undef D_RESTD_FLT_MAX_10_EXP
#undef D_RESTD_FLT_MAX
#undef D_RESTD_FLT_MIN
#undef D_RESTD_FLT_EPSILON
#undef D_RESTD_FLT_DENORM_MIN
#undef D_RESTD_FLT_HAS_INF
#undef D_RESTD_FLT_HAS_QNAN
#undef D_RESTD_FLT_HAS_DENORM
#undef D_RESTD_DBL_MANT_DIG
#undef D_RESTD_DBL_DIG
#undef D_RESTD_DBL_MIN_EXP
#undef D_RESTD_DBL_MIN_10_EXP
#undef D_RESTD_DBL_MAX_EXP
#undef D_RESTD_DBL_MAX_10_EXP
#undef D_RESTD_DBL_MAX
#undef D_RESTD_DBL_MIN
#undef D_RESTD_DBL_EPSILON
#undef D_RESTD_DBL_DENORM_MIN
#undef D_RESTD_DBL_HAS_INF
#undef D_RESTD_DBL_HAS_QNAN
#undef D_RESTD_DBL_HAS_DENORM
#undef D_RESTD_LDBL_MANT_DIG
#undef D_RESTD_LDBL_DIG
#undef D_RESTD_LDBL_MIN_EXP
#undef D_RESTD_LDBL_MIN_10_EXP
#undef D_RESTD_LDBL_MAX_EXP
#undef D_RESTD_LDBL_MAX_10_EXP
#undef D_RESTD_LDBL_MAX
#undef D_RESTD_LDBL_MIN
#undef D_RESTD_LDBL_EPSILON
#undef D_RESTD_LDBL_DENORM_MIN
#undef D_RESTD_LDBL_HAS_INF
#undef D_RESTD_LDBL_HAS_QNAN
#undef D_RESTD_LDBL_HAS_DENORM
#undef D_RESTD_LIMITS_BUILTINS

#endif  // RESTD_LIMITS_NUMERIC_LIMITS_
