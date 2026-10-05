/*******************************************************************************
* djinterp [re_std]                                                     move.hpp
*
* rvalue cast utility:
*   Provides re_std::move, the canonical cast-to-rvalue-reference used
* to enable move construction and move assignment. Equivalent to
* static_cast<remove_reference<T>::type&&>(value).
*
*   Requires rvalue references (C++11+). On standards without rvalue
* references, no symbol is defined; callers must gate their use of
* re_std::move on RE_STD_LANG_HAS_RVALUE_REFERENCES.
*
*   marked constexpr on C++11+ (single-statement body).
*
*
* path:      /inc/re_std/utility/move.hpp
* link(s):   TBA
* author(s): re_std team                                     created: 2026.04.30
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_UTILITY_MOVE_HPP
#define RE_STD_UTILITY_MOVE_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration

#if RE_STD_LANG_HAS_RVALUE_REFERENCES

#include "../type_traits/remove_reference.hpp"

namespace re_std
{

// =============================================================================
// MOVE
// =============================================================================

// move
//   function: produces an rvalue reference to _value, signalling that
//   _value's resources may be pilfered. Single-statement body so it is
//   constexpr-eligible on C++11.
template<typename Type>
RE_STD_CONSTEXPR
typename remove_reference<Type>::type&&
move(Type&& _value) noexcept
{
    return static_cast<typename remove_reference<Type>::type&&>(_value);
}

}  // re_std

#endif  // RE_STD_LANG_HAS_RVALUE_REFERENCES

#endif  // RE_STD_UTILITY_MOVE_HPP
