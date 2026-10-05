/*******************************************************************************
* djinterp [re_std]                                                  forward.hpp
*
* perfect-forwarding cast utility:
*   Provides re_std::forward, the canonical cast used in forwarding
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
* re_std::forward on RE_STD_LANG_HAS_RVALUE_REFERENCES.
*
*   marked constexpr on C++11+ (single-statement bodies).
*
*
* path:      /inc/re_std/utility/forward.hpp
* link(s):   TBA
* author(s): re_std team                                     created: 2026.04.30
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_UTILITY_FORWARD_HPP
#define RE_STD_UTILITY_FORWARD_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration

#if RE_STD_LANG_HAS_RVALUE_REFERENCES

#include "../type_traits/remove_reference.hpp"
#include "../type_traits/is_lvalue_reference.hpp"

namespace re_std
{

// =============================================================================
// FORWARD
// =============================================================================

// forward (lvalue overload)
//   function: forwards an lvalue as either an lvalue or an rvalue,
//   depending on the deduced template argument Type.
template<typename Type>
RE_STD_CONSTEXPR
Type&& forward(typename remove_reference<Type>::type& _value) noexcept
{
    return static_cast<Type&&>(_value);
}

// forward (rvalue overload)
//   function: forwards an rvalue. static_asserts that Type is not an
//   lvalue reference -- forwarding an rvalue as an lvalue would yield
//   a dangling reference.
template<typename Type>
RE_STD_CONSTEXPR
Type&& forward(typename remove_reference<Type>::type&& _value) noexcept
{
    static_assert(!is_lvalue_reference<Type>::value,
                  "re_std::forward: cannot forward an rvalue as an lvalue");
    return static_cast<Type&&>(_value);
}

}  // re_std

#endif  // RE_STD_LANG_HAS_RVALUE_REFERENCES

#endif  // RE_STD_UTILITY_FORWARD_HPP
