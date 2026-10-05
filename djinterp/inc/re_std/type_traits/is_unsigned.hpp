/*******************************************************************************
* djinterp [re_std]                                              is_unsigned.hpp
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
* path:      /inc/re_std/type_traits/is_unsigned.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.04.28
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_TYPE_TRAITS_IS_UNSIGNED_HPP
#define RE_STD_TYPE_TRAITS_IS_UNSIGNED_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration
#include "./integral_constant.hpp"
#include "./true_type.hpp"
#include "./false_type.hpp"
#include "./is_integral.hpp"
#include "./remove_cv.hpp"


namespace re_std
{


// =============================================================================
// I.   IS_UNSIGNED
// =============================================================================

namespace internal
{

    // is_unsigned_helper
    //   trait: false for non-integral types (primary template).
    template<typename Type,
             bool      IsIntegral>
    struct is_unsigned_helper : false_type
    {};

    // is_unsigned_helper<Type, true>
    //   trait: integral types - test via comparison.
    // Unsigned: -1 wraps to max value -> 0 < max is true.
    // Signed:   -1 stays negative     -> 0 < -1 is false.
    template<typename Type>
    struct is_unsigned_helper<Type, true>
        : integral_constant<bool,
            ( static_cast<Type>(0) < static_cast<Type>(-1) )>
    {};

}  // internal

// is_unsigned
//   trait: true if Type (cv-stripped) is an unsigned integral type.
template<typename Type>
struct is_unsigned
    : internal::is_unsigned_helper<
          typename remove_cv<Type>::type,
          is_integral<Type>::value>
{};


// =============================================================================
// II.  IS_UNSIGNED_V (C++14+ variable)
// =============================================================================

#if RE_STD_LANG_HAS_VARIABLE_TEMPLATES

    // is_unsigned_v
    //   variable: convenience for is_unsigned<Type>::value.
    template<typename Type>
    RE_STD_CONSTEXPR bool is_unsigned_v = is_unsigned<Type>::value;

#endif  // RE_STD_LANG_HAS_VARIABLE_TEMPLATES


}  // re_std


#endif  // RE_STD_TYPE_TRAITS_IS_UNSIGNED_HPP
