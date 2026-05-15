/***********************************************************************
* restd                                                       as_const.hpp
*
* as_const(_v):
*   Returns a const lvalue reference to _v. Used to force selection
* of const overloads, particularly to avoid accidentally invoking
* mutating range-based-for begin()/end() on containers that have a
* const overload distinct from the non-const one.
*
*   The deleted rvalue overload prevents `as_const(temporary())` --
* binding a const& to a temporary in a way that escapes the
* expression's lifetime is a common bug.
*
* added in std C++17.
*
*
* path:      /inc/restd/utility/as_const.hpp
* link(s):   TBA
* author(s): restd team                                 date: 2026.05.09
***********************************************************************/

#ifndef RESTD_UTILITY_AS_CONST_
#define RESTD_UTILITY_AS_CONST_ 1

#include "djinterp.hpp"


#if D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES

namespace restd
{

template<typename _T>
D_CONSTEXPR const _T& as_const(_T& _v) D_NOEXCEPT
{
    return _v;
}

// Disable rvalue arg: would otherwise bind a const& to a temporary
// that dies at the end of the full-expression.
template<typename _T>
void as_const(const _T&&) = delete;


}  // namespace restd

#endif  // D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES

#endif  // RESTD_UTILITY_AS_CONST_
