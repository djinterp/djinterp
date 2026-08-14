/***********************************************************************
* restd                                                          move.hpp
*
* rvalue cast utility:
*   Provides restd::move, the canonical cast-to-rvalue-reference used
* to enable move construction and move assignment. Equivalent to
* static_cast<remove_reference<T>::type&&>(value).
*
*   Requires rvalue references (C++11+). On standards without rvalue
* references, no symbol is defined; callers must gate their use of
* restd::move on D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES.
*
*   marked constexpr on C++11+ (single-statement body).
*
*
* path:      /inc/djinterp/re_std/utility/move.hpp
* link(s):   TBA
* author(s): restd team                                 date: 2026.04.30
***********************************************************************/

#ifndef RESTD_UTILITY_MOVE_
#define RESTD_UTILITY_MOVE_ 1

#include "djinterp.hpp"

#if D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES

#include "../type_traits/remove_reference.hpp"

NS_RESTD

// =============================================================================
// MOVE
// =============================================================================

// move
//   function: produces an rvalue reference to _value, signalling that
//   _value's resources may be pilfered. Single-statement body so it is
//   constexpr-eligible on C++11.
template<typename _Type>
D_CONSTEXPR
typename remove_reference<_Type>::type&&
move(_Type&& _value) noexcept
{
    return static_cast<typename remove_reference<_Type>::type&&>(_value);
}

NS_END  // restd

#endif  // D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES

#endif  // RESTD_UTILITY_MOVE_
