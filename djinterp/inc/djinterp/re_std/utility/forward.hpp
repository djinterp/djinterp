/***********************************************************************
* restd                                                       forward.hpp
*
* perfect-forwarding cast utility:
*   Provides restd::forward, the canonical cast used in forwarding
* references to preserve value category. Two overloads:
*
*     forward<T>(lvalue_ref) -> static_cast<T&&>(arg)        // lvalue overload
*     forward<T>(rvalue_ref) -> static_cast<T&&>(arg)        // rvalue overload
*
*   The rvalue overload static_asserts that T is not an lvalue
* reference; forwarding a real rvalue as an lvalue would produce a
* dangling reference.
*
*   Requires rvalue references (C++11+). On standards without rvalue
* references, no symbol is defined; callers must gate their use of
* restd::forward on D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES.
*
*   marked constexpr on C++11+ (single-statement bodies).
*
*
* path:      /inc/djinterp/re_std/utility/forward.hpp
* link(s):   TBA
* author(s): restd team                                 date: 2026.04.30
***********************************************************************/

#ifndef RESTD_UTILITY_FORWARD_
#define RESTD_UTILITY_FORWARD_ 1

#include "djinterp.hpp"

#if D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES

#include "../type_traits/remove_reference.hpp"
#include "../type_traits/is_lvalue_reference.hpp"

NS_RESTD

// =============================================================================
// FORWARD
// =============================================================================

// forward (lvalue overload)
//   function: forwards an lvalue as either an lvalue or an rvalue,
//   depending on the deduced template argument _Type.
template<typename _Type>
D_CONSTEXPR
_Type&& forward(typename remove_reference<_Type>::type& _value) noexcept
{
    return static_cast<_Type&&>(_value);
}

// forward (rvalue overload)
//   function: forwards an rvalue. static_asserts that _Type is not an
//   lvalue reference -- forwarding an rvalue as an lvalue would yield
//   a dangling reference.
template<typename _Type>
D_CONSTEXPR
_Type&& forward(typename remove_reference<_Type>::type&& _value) noexcept
{
    static_assert(!is_lvalue_reference<_Type>::value,
                  "restd::forward: cannot forward an rvalue as an lvalue");
    return static_cast<_Type&&>(_value);
}

NS_END  // restd

#endif  // D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES

#endif  // RESTD_UTILITY_FORWARD_
