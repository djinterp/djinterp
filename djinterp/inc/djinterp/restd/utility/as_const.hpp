/***********************************************************************
* restd                                                      as_const.hpp
*
* const-view cast utility:
*   Provides restd::as_const, which returns a const reference to its
* argument. Useful for triggering const-qualified overloads of member
* functions or for capturing-by-const-ref in a way that's explicit at
* the call site.
*
*   Two overloads:
*     as_const(T&)          -> const T&    (returns the const view)
*     as_const(const T&&)   = delete       (banned: would dangle)
*
*   STANDARD STATUS:
*   Introduced in C++17. restd back-ports to C++11+ (the implementation
* needs only `= delete` and add_const, both available since C++11).
*
*
* path:      /inc/restd/utility/as_const.hpp
* link(s):   TBA
* author(s): restd team                                  date: 2026.05.02
***********************************************************************/

#ifndef RESTD_UTILITY_AS_CONST_
#define RESTD_UTILITY_AS_CONST_ 1

#include "djinterp.hpp"

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

#include "../type_traits/add_const.hpp"

NS_RESTD

// =============================================================================
// AS_CONST
// =============================================================================

// as_const (lvalue overload)
//   function: returns a const reference to the argument. Single-
//   statement; constexpr-eligible from C++11.
template<typename _Type>
D_CONSTEXPR
typename add_const<_Type>::type& as_const(_Type& _value) noexcept
{
    return _value;
}

// as_const (rvalue overload, deleted)
//   function: forbidden -- as_const on an rvalue would return a
//   reference to a soon-to-die temporary. Deletion is part of the
//   standard's interface.
template<typename _Type>
void as_const(const _Type&&) = delete;

NS_END  // restd

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#endif  // RESTD_UTILITY_AS_CONST_
