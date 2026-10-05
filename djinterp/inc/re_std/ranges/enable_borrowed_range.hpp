/*******************************************************************************
* djinterp [re_std]                                    enable_borrowed_range.hpp
*
* enable_borrowed_range customization point header:
*   Provides the customisation-point variable template that classifies a
* type as a borrowed_range (C++20) — a range whose iterators remain valid
* after the range itself has been destroyed (the canonical examples being
* span, string_view, ref_view, owning_view-of-pointers, and subrange).
* The default for every type is false; users opt their own types in.
*
*   PORTABILITY:
*   - C++14+: real variable template (RE_STD_HAS_ENABLE_BORROWED_VAR == 1).
*   - C++98/03/11: trait-struct fallback. enable_borrowed_range<T>::value
*     is the equivalent boolean. The trait works on any conforming
*     compiler.
*
*
* path:      /inc/re_std/ranges/enable_borrowed_range.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.13
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_RANGES_ENABLE_BORROWED_RANGE_HPP
#define RE_STD_RANGES_ENABLE_BORROWED_RANGE_HPP 1

// FLOOR, FOR NOW: below C++11 this header is empty rather than an error
// (README rule 5; re_std omits rather than degrades). The owner's ruling:
// compile at every level first; port to C++98 only where something needs it.
#include "../config.hpp"  // RE_STD_* configuration
#if RE_STD_LANG_IS_CPP11_OR_HIGHER

// re_std
#include "../type_traits/type_traits.hpp"


namespace re_std
{


// ===========================================================================
// 0.   DETECTION MACRO
// ===========================================================================

// RE_STD_HAS_ENABLE_BORROWED_VAR
//   constant: 1 when enable_borrowed_range is exposed as a constexpr
// bool variable template. 0 when only the trait-struct form is
// available.
#ifndef RE_STD_HAS_ENABLE_BORROWED_VAR
    #if RE_STD_LANG_HAS_VARIABLE_TEMPLATES
        #define RE_STD_HAS_ENABLE_BORROWED_VAR  1
    #else
        #define RE_STD_HAS_ENABLE_BORROWED_VAR  0
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
// handful of other library types specialise it to true. Re_std
// matches this — the primary always reports false.
template<typename Type>
struct enable_borrowed_range
    : false_type
{};


// ===========================================================================
// II.  ENABLE_BORROWED_RANGE_V (variable template, C++14+)
// ===========================================================================

#if RE_STD_HAS_ENABLE_BORROWED_VAR

// enable_borrowed_range_v
//   variable: convenience constexpr accessor. Matches the C++20
// std::ranges::enable_borrowed_range variable-template form (which
// in C++20 is itself the customisation point; the trait struct is
// re_std-specific for back-portability).
// note: users who need to opt their type in on C++14+ should
// specialise this variable template:
//
//     namespace re_std {
//         template<>
//         constexpr bool enable_borrowed_range_v<my_range> = true;
//     }
//
// On C++98/03/11, specialise the enable_borrowed_range trait struct
// instead.
template<typename Type>
RE_STD_CONSTEXPR bool enable_borrowed_range_v =
    enable_borrowed_range<Type>::value;

#endif  // RE_STD_HAS_ENABLE_BORROWED_VAR


}  // re_std

#endif  // floor, for now


#endif  // RE_STD_RANGES_ENABLE_BORROWED_RANGE_HPP
