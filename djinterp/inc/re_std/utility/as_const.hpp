/*******************************************************************************
* djinterp [re_std]                                                 as_const.hpp
*
* const-view cast utility:
*   Provides re_std::as_const, which returns a const reference to its
* argument. Useful for triggering const-qualified overloads of member
* functions or for capturing-by-const-ref in a way that's explicit at
* the call site.
*
*   Two overloads:
*     as_const(T&)          -> const T&    (returns the const view)
*     as_const(const T&&)   = delete       (banned: would dangle)
*
*   STANDARD STATUS:
*   Introduced in C++17. re_std back-ports to C++11+ (the implementation
* needs only `= delete` and add_const, both available since C++11).
*
*
* path:      /inc/re_std/utility/as_const.hpp
* link(s):   TBA
* author(s): re_std team                                     created: 2026.05.02
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_UTILITY_AS_CONST_HPP
#define RE_STD_UTILITY_AS_CONST_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration

#if RE_STD_LANG_IS_CPP11_OR_HIGHER

#include "../type_traits/add_const.hpp"

namespace re_std
{

// =============================================================================
// AS_CONST
// =============================================================================

// as_const (lvalue overload)
//   function: returns a const reference to the argument. Single-
//   statement; constexpr-eligible from C++11.
template<typename Type>
RE_STD_CONSTEXPR
typename add_const<Type>::type& as_const(Type& _value) noexcept
{
    return _value;
}

// as_const (rvalue overload, deleted)
//   function: forbidden -- as_const on an rvalue would return a
//   reference to a soon-to-die temporary. Deletion is part of the
//   standard's interface.
template<typename Type>
void as_const(const Type&&) = delete;

}  // re_std

#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER

#endif  // RE_STD_UTILITY_AS_CONST_HPP
