/******************************************************************************
* djinterp [re_std]                                              is_signed.hpp
*
* is_signed trait header:
*   Detects whether a type, ignoring cv-qualifiers, is a signed
* arithmetic type. Floating-point types are always signed. Integral
* signedness is determined via the constant-expression test
* `static_cast<T>(-1) < static_cast<T>(0)`.
*
*     is_signed<int>::value            -> true
*     is_signed<unsigned int>::value   -> false
*     is_signed<float>::value          -> true   (floats are signed)
*     is_signed<bool>::value           -> false  (-1 wraps to true=1, not <0)
*     is_signed<char>::value           -> implementation-defined
*     is_signed<MyEnum>::value         -> false  (enums are not arithmetic;
*                                                use is_signed on
*                                                underlying_type<MyEnum>::type)
*     is_signed<int*>::value           -> false
*
*
* path:      /inc/djinterp/re_std/type_traits/is_signed.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.28
******************************************************************************/

#ifndef DJINTERP_RE_STD_TYPE_TRAITS_IS_SIGNED_
#define DJINTERP_RE_STD_TYPE_TRAITS_IS_SIGNED_ 1

// djinterp
#include "../../core/djinterp.hpp"
#include "./integral_constant.hpp"
#include "./true_type.hpp"
#include "./false_type.hpp"
#include "./is_integral.hpp"
#include "./is_floating_point.hpp"
#include "./remove_cv.hpp"


NS_RESTD


// =============================================================================
// I.   IS_SIGNED
// =============================================================================

NS_INTERNAL

    // is_signed_helper
    //   trait: false for non-arithmetic types (primary template).
    template<typename _Type,
             bool      _IsArithmetic>
    struct is_signed_helper : false_type
    {};

    // is_signed_helper<_Type, true>
    //   trait: arithmetic types - test via comparison.
    // Floating-point: comparison is true (e.g. -1.0 < 0.0).
    // Signed integral: comparison is true.
    // Unsigned integral: -1 wraps to max value, comparison is false.
    // bool: -1 -> true (1), 0 -> false (0); 1 < 0 is false.
    template<typename _Type>
    struct is_signed_helper<_Type, true>
        : integral_constant<bool,
            ( static_cast<_Type>(-1) < static_cast<_Type>(0) )>
    {};

NS_END  // internal

// is_signed
//   trait: true if _Type (cv-stripped) is a signed arithmetic type.
template<typename _Type>
struct is_signed
    : internal::is_signed_helper<
          typename remove_cv<_Type>::type,
          ( is_integral<_Type>::value ||
            is_floating_point<_Type>::value )>
{};


// =============================================================================
// II.  IS_SIGNED_V (C++14+ variable)
// =============================================================================

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    // is_signed_v
    //   variable: convenience for is_signed<_Type>::value.
    template<typename _Type>
    D_CONSTEXPR bool is_signed_v = is_signed<_Type>::value;

#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES


NS_END  // re_std


#endif  // DJINTERP_RE_STD_TYPE_TRAITS_IS_SIGNED_
