/******************************************************************************
* djinterp [restd]                                             is_unsigned.hpp
*
* is_unsigned trait header:
*   Detects whether a type, ignoring cv-qualifiers, is an unsigned
* integral type. Floating-point types are NEVER unsigned. The integral
* test is `static_cast<T>(0) < static_cast<T>(-1)`, which is true when
* -1 wraps to a large positive value (unsigned semantics).
*
*     is_unsigned<unsigned int>::value -> true
*     is_unsigned<int>::value          -> false
*     is_unsigned<bool>::value         -> true  (bool stores as 0/1; -1 -> 1)
*     is_unsigned<float>::value        -> false (no unsigned floats)
*     is_unsigned<unsigned char>::value -> true
*     is_unsigned<int*>::value         -> false
*
*
* path:      /inc/djinterp/restd/type_traits/is_unsigned.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.28
******************************************************************************/

#ifndef DJINTERP_RESTD_TYPE_TRAITS_IS_UNSIGNED_
#define DJINTERP_RESTD_TYPE_TRAITS_IS_UNSIGNED_ 1

// djinterp
#include "../../core/djinterp.hpp"
#include "./integral_constant.hpp"
#include "./true_type.hpp"
#include "./false_type.hpp"
#include "./is_integral.hpp"
#include "./remove_cv.hpp"


NS_RESTD


// =============================================================================
// I.   IS_UNSIGNED
// =============================================================================

NS_INTERNAL

    // is_unsigned_helper
    //   trait: false for non-integral types (primary template).
    template<typename _Type,
             bool      _IsIntegral>
    struct is_unsigned_helper : false_type
    {};

    // is_unsigned_helper<_Type, true>
    //   trait: integral types - test via comparison.
    // Unsigned: -1 wraps to max value -> 0 < max is true.
    // Signed:   -1 stays negative     -> 0 < -1 is false.
    template<typename _Type>
    struct is_unsigned_helper<_Type, true>
        : integral_constant<bool,
            ( static_cast<_Type>(0) < static_cast<_Type>(-1) )>
    {};

NS_END  // internal

// is_unsigned
//   trait: true if _Type (cv-stripped) is an unsigned integral type.
template<typename _Type>
struct is_unsigned
    : internal::is_unsigned_helper<
          typename remove_cv<_Type>::type,
          is_integral<_Type>::value>
{};


// =============================================================================
// II.  IS_UNSIGNED_V (C++14+ variable)
// =============================================================================

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    // is_unsigned_v
    //   variable: convenience for is_unsigned<_Type>::value.
    template<typename _Type>
    D_CONSTEXPR bool is_unsigned_v = is_unsigned<_Type>::value;

#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES


NS_END  // restd


#endif  // DJINTERP_RESTD_TYPE_TRAITS_IS_UNSIGNED_
