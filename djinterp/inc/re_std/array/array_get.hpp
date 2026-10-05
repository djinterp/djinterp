/*******************************************************************************
* djinterp [re_std]                                                array_get.hpp
*
* array get header:
*   Provides the four value-category overloads of re_std::get<I>(array):
*
*     get<I>(      array<T, N>&  )  ->       T&
*     get<I>(const array<T, N>&  )  -> const T&
*     get<I>(      array<T, N>&& )  ->       T&&
*     get<I>(const array<T, N>&& )  -> const T&&
*
*   The C++11–C++17 standard required only the lvalue overloads; the
* rvalue overloads were added in C++11 as well per [array.tuple]. The
* const-rvalue overload arrived later. All four are provided here on
* C++11+; on C++98/03 only the two lvalue overloads ship (no rvalue
* references).
*
*   CONSTEXPR:
*   - C++11: not constexpr (constexpr function bodies were restricted
*     to a single return statement, and the return type — _T& — could
*     not be returned through a constexpr function under the C++11
*     implicit-const rule).
*   - C++14+: constexpr (LWG 2185 — same easement that made the
*     const overloads of array's accessors constexpr).
*
*   BOUNDS CHECK:
*   Index is statically checked via static_assert (C++11+) or a
* compile-time array-of-zero trick on C++98/03. Indices outside
* [0, Size) are diagnosed at instantiation time, matching std's
* static_assert behaviour.
*
*
* path:      /inc/re_std/array/array_get.hpp
* link(s):   TBA
* author(s): TBA                                             created: 2026.05.19
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_ARRAY_ARRAY_GET_HPP
#define RE_STD_ARRAY_ARRAY_GET_HPP 1

// FLOOR, FOR NOW: below C++11 this header is empty rather than an error
// (README rule 5; re_std omits rather than degrades). The owner's ruling:
// compile at every level first; port to C++98 only where something needs it.
#include "../config.hpp"  // RE_STD_* configuration
#if RE_STD_LANG_IS_CPP11_OR_HIGHER

// std
#include <cstddef>
// re_std
#include "./array.hpp"


namespace re_std
{


// ===========================================================================
// I.   GET<I>(ARRAY) — LVALUE OVERLOADS
// ===========================================================================
// Available on every tier. C++14+ constexpr.

// get<I>(array&)
//   function: returns a reference to the Index-th element of _a.
// static_assert: Index < Size.
template<std::size_t Index,
         typename    Type,
         std::size_t Size>
RE_STD_CONSTEXPR_CPP14 Type&
get(
    array<Type, Size>& _a
) RE_STD_NOEXCEPT
{
#if RE_STD_LANG_IS_CPP11_OR_HIGHER
    static_assert(Index < Size, "re_std::get<I>(array): I out of bounds");
#endif
    return array<Type, Size>::_storage::ptr(_a._M_elems)[Index];
}

// get<I>(const array&)
//   function: returns a const reference to the Index-th element.
// static_assert: Index < Size.
template<std::size_t Index,
         typename    Type,
         std::size_t Size>
RE_STD_CONSTEXPR_CPP14 Type const&
get(
    array<Type, Size> const& _a
) RE_STD_NOEXCEPT
{
#if RE_STD_LANG_IS_CPP11_OR_HIGHER
    static_assert(Index < Size, "re_std::get<I>(array): I out of bounds");
#endif
    return array<Type, Size>::_storage::ptr(_a._M_elems)[Index];
}


// ===========================================================================
// II.  GET<I>(ARRAY) — RVALUE OVERLOADS (C++11+)
// ===========================================================================
// Gated on rvalue references. The non-const-rvalue overload moves;
// the const-rvalue overload is rarely useful but standardised since
// C++17 LWG 2485 (and back-ported here unconditionally for C++11+).

#if RE_STD_LANG_HAS_RVALUE_REFERENCES

// get<I>(array&&)
//   function: returns an rvalue reference to the Index-th element
// of _a — caller may move it out.
template<std::size_t Index,
         typename    Type,
         std::size_t Size>
RE_STD_CONSTEXPR_CPP14 Type&&
get(
    array<Type, Size>&& _a
) RE_STD_NOEXCEPT
{
    static_assert(Index < Size, "re_std::get<I>(array): I out of bounds");
    return static_cast<Type&&>(
        array<Type, Size>::_storage::ptr(_a._M_elems)[Index]);
}

// get<I>(const array&&)
//   function: returns a const rvalue reference to the Index-th
// element. Standardised by LWG 2485; back-ported to C++11.
template<std::size_t Index,
         typename    Type,
         std::size_t Size>
RE_STD_CONSTEXPR_CPP14 Type const&&
get(
    array<Type, Size> const&& _a
) RE_STD_NOEXCEPT
{
    static_assert(Index < Size, "re_std::get<I>(array): I out of bounds");
    return static_cast<Type const&&>(
        array<Type, Size>::_storage::ptr(_a._M_elems)[Index]);
}

#endif  // RE_STD_LANG_HAS_RVALUE_REFERENCES


}  // re_std

#endif  // floor, for now


#endif  // RE_STD_ARRAY_ARRAY_GET_HPP
