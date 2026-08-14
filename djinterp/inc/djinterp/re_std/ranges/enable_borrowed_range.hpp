/******************************************************************************
* djinterp [restd]                                   enable_borrowed_range.hpp
*
* enable_borrowed_range customization point header:
*   Provides the customisation-point variable template that classifies a
* type as a borrowed_range (C++20) — a range whose iterators remain valid
* after the range itself has been destroyed (the canonical examples being
* span, string_view, ref_view, owning_view-of-pointers, and subrange).
* The default for every type is false; users opt their own types in.
*
*   PORTABILITY:
*   - C++14+: real variable template (D_RESTD_HAS_ENABLE_BORROWED_VAR == 1).
*   - C++98/03/11: trait-struct fallback. enable_borrowed_range<T>::value
*     is the equivalent boolean. The trait works on any conforming
*     compiler.
*
*
* path:      /inc/djinterp/re_std/ranges/enable_borrowed_range.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.13
******************************************************************************/

#ifndef DJINTERP_RESTD_RANGES_ENABLE_BORROWED_RANGE_
#define DJINTERP_RESTD_RANGES_ENABLE_BORROWED_RANGE_ 1

#include "../../core/djinterp.hpp"
#include "../type_traits/type_traits.hpp"


NS_RESTD


// ===========================================================================
// 0.   DETECTION MACRO
// ===========================================================================

// D_RESTD_HAS_ENABLE_BORROWED_VAR
//   constant: 1 when enable_borrowed_range is exposed as a constexpr
// bool variable template. 0 when only the trait-struct form is
// available.
#ifndef D_RESTD_HAS_ENABLE_BORROWED_VAR
    #if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
        #define D_RESTD_HAS_ENABLE_BORROWED_VAR  1
    #else
        #define D_RESTD_HAS_ENABLE_BORROWED_VAR  0
    #endif
#endif


// ===========================================================================
// I.   ENABLE_BORROWED_RANGE (primary trait)
// ===========================================================================

// enable_borrowed_range (trait)
//   trait: false_type by default. Specialise for a user range type
// to declare that iterators obtained from rvalues of that type
// remain valid after the rvalue has been destroyed.
// note: the C++20 standard defaults this to false for every type;
// only span, string_view, subrange, ref_view, iota_view, and a
// handful of other library types specialise it to true. Restd
// matches this — the primary always reports false.
template<typename _Type>
struct enable_borrowed_range
    : false_type
{};


// ===========================================================================
// II.  ENABLE_BORROWED_RANGE_V (variable template, C++14+)
// ===========================================================================

#if D_RESTD_HAS_ENABLE_BORROWED_VAR

// enable_borrowed_range_v
//   variable: convenience constexpr accessor. Matches the C++20
// std::ranges::enable_borrowed_range variable-template form (which
// in C++20 is itself the customisation point; the trait struct is
// restd-specific for back-portability).
// note: users who need to opt their type in on C++14+ should
// specialise this variable template:
//
//     namespace restd {
//         template<>
//         constexpr bool enable_borrowed_range_v<my_range> = true;
//     }
//
// On C++98/03/11, specialise the enable_borrowed_range trait struct
// instead.
template<typename _Type>
D_CONSTEXPR bool enable_borrowed_range_v =
    enable_borrowed_range<_Type>::value;

#endif  // D_RESTD_HAS_ENABLE_BORROWED_VAR


NS_END  // restd


#endif  // DJINTERP_RESTD_RANGES_ENABLE_BORROWED_RANGE_
