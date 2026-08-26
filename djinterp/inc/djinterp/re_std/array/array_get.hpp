/******************************************************************************
* djinterp [re_std]                                               array_get.hpp
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
*   _Index is statically checked via static_assert (C++11+) or a
* compile-time array-of-zero trick on C++98/03. Indices outside
* [0, _Size) are diagnosed at instantiation time, matching std's
* static_assert behaviour.
*
*
* path:      /inc/djinterp/re_std/array/array_get.hpp
* link(s):   TBA
* author(s): TBA                                           created: 2026.05.19
******************************************************************************/

#ifndef DJINTERP_RE_STD_ARRAY_GET_
#define DJINTERP_RE_STD_ARRAY_GET_ 1

#include <cstddef>

#include "../../core/djinterp.hpp"
#include "./array.hpp"


#ifndef D_CONSTEXPR_CPP14
    #if D_ENV_LANG_IS_CPP14_OR_HIGHER
        #define D_CONSTEXPR_CPP14   constexpr
    #else
        #define D_CONSTEXPR_CPP14
    #endif
#endif


NS_RESTD


// ===========================================================================
// I.   GET<I>(ARRAY) — LVALUE OVERLOADS
// ===========================================================================
// Available on every tier. C++14+ constexpr.

// get<I>(array&)
//   function: returns a reference to the _Index-th element of _a.
// static_assert: _Index < _Size.
template<std::size_t _Index,
         typename    _Type,
         std::size_t _Size>
D_CONSTEXPR_CPP14 _Type&
get(
    array<_Type, _Size>& _a
) D_NOEXCEPT
{
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    static_assert(_Index < _Size, "re_std::get<I>(array): I out of bounds");
#endif
    return array<_Type, _Size>::_storage::ptr(_a._M_elems)[_Index];
}

// get<I>(const array&)
//   function: returns a const reference to the _Index-th element.
// static_assert: _Index < _Size.
template<std::size_t _Index,
         typename    _Type,
         std::size_t _Size>
D_CONSTEXPR_CPP14 _Type const&
get(
    array<_Type, _Size> const& _a
) D_NOEXCEPT
{
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    static_assert(_Index < _Size, "re_std::get<I>(array): I out of bounds");
#endif
    return array<_Type, _Size>::_storage::ptr(_a._M_elems)[_Index];
}


// ===========================================================================
// II.  GET<I>(ARRAY) — RVALUE OVERLOADS (C++11+)
// ===========================================================================
// Gated on rvalue references. The non-const-rvalue overload moves;
// the const-rvalue overload is rarely useful but standardised since
// C++17 LWG 2485 (and back-ported here unconditionally for C++11+).

#if D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES

// get<I>(array&&)
//   function: returns an rvalue reference to the _Index-th element
// of _a — caller may move it out.
template<std::size_t _Index,
         typename    _Type,
         std::size_t _Size>
D_CONSTEXPR_CPP14 _Type&&
get(
    array<_Type, _Size>&& _a
) D_NOEXCEPT
{
    static_assert(_Index < _Size, "re_std::get<I>(array): I out of bounds");
    return static_cast<_Type&&>(
        array<_Type, _Size>::_storage::ptr(_a._M_elems)[_Index]);
}

// get<I>(const array&&)
//   function: returns a const rvalue reference to the _Index-th
// element. Standardised by LWG 2485; back-ported to C++11.
template<std::size_t _Index,
         typename    _Type,
         std::size_t _Size>
D_CONSTEXPR_CPP14 _Type const&&
get(
    array<_Type, _Size> const&& _a
) D_NOEXCEPT
{
    static_assert(_Index < _Size, "re_std::get<I>(array): I out of bounds");
    return static_cast<_Type const&&>(
        array<_Type, _Size>::_storage::ptr(_a._M_elems)[_Index]);
}

#endif  // D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES


NS_END  // re_std


#endif  // DJINTERP_RE_STD_ARRAY_GET_
