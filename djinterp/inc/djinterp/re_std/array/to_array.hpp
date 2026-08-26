/******************************************************************************
* djinterp [re_std]                                               to_array.hpp
*
* to_array factory header:
*   Provides the two C++20 to_array overloads:
*
*     to_array(_Type (&)[_N])    -> array<remove_cv_t<_Type>, _N>
*     to_array(_Type (&&)[_N])   -> array<remove_cv_t<_Type>, _N>
*
*   Both overloads strip cv-qualification from the element type and
* construct an array<remove_cv_t<_Type>, _N> by copying (lvalue
* overload) or moving (rvalue overload) each element.
*
*   PORTABILITY:
*   to_array entered the standard in C++20; re_std back-ports it to
* C++11 via index_sequence expansion. Requires:
*   - rvalue references (C++11+)
*   - variadic templates (C++11+)
*   - index_sequence + make_index_sequence (C++14+ in std; re_std
*     ships these in <utility> back-ported to C++11)
*
*   The implementation is constexpr from C++11 (matching std's C++20
* introduction-as-constexpr) — re_std is ahead of std on tier
* availability but offers the same constexpr-ness from intro.
*
*   MULTIDIMENSIONAL ARRAYS:
*   _Type may not itself be an array type — to_array on a 2-D array
* is ill-formed per [array.creation]. re_std enforces this via
* static_assert.
*
*   Uses:
*     array.hpp                    - the array class
*     type_traits/remove_cv.hpp    - remove_cv trait
*     type_traits/is_array.hpp     - is_array trait (for the assert)
*     utility/integer_sequence.hpp - index_sequence machinery
*
*
* path:      /inc/djinterp/re_std/array/to_array.hpp
* link(s):   TBA
* author(s): TBA                                           created: 2026.05.19
******************************************************************************/

#ifndef DJINTERP_RE_STD_TO_ARRAY_
#define DJINTERP_RE_STD_TO_ARRAY_ 1

#include "../../core/djinterp.hpp"

// gate: to_array requires variadic templates + rvalue references +
// index_sequence — effectively the same set as re_std::make_any.
#if ( D_ENV_CPP_FEATURE_LANG_VARIADIC_TEMPLATES &&                            \
      D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES )

#include <cstddef>

#include "./array.hpp"
#include "../type_traits/remove_cv.hpp"
#include "../type_traits/is_array.hpp"
#include "../utility/integer_sequence.hpp"
#include "../utility/make_integer_sequence.hpp"   // make_index_sequence


NS_RESTD


NS_INTERNAL

    // to_array_lvalue
    //   function: index-sequence-expansion helper for the lvalue
    // to_array overload. Copy-initialises each element.
    template<typename    _Type,
             std::size_t _N,
             std::size_t... _Is>
    D_CONSTEXPR array<typename re_std::remove_cv<_Type>::type, _N>
    to_array_lvalue(
        _Type (&_src)[_N],
        re_std::index_sequence<_Is...>
    )
    {
        return array<typename re_std::remove_cv<_Type>::type, _N>{
            { _src[_Is]... }
        };
    }

    // to_array_rvalue
    //   function: index-sequence-expansion helper for the rvalue
    // to_array overload. Move-initialises each element.
    template<typename    _Type,
             std::size_t _N,
             std::size_t... _Is>
    D_CONSTEXPR array<typename re_std::remove_cv<_Type>::type, _N>
    to_array_rvalue(
        _Type (&&_src)[_N],
        re_std::index_sequence<_Is...>
    )
    {
        return array<typename re_std::remove_cv<_Type>::type, _N>{
            { static_cast<_Type&&>(_src[_Is])... }
        };
    }

NS_END  // internal


// ===========================================================================
// I.   TO_ARRAY (lvalue)
// ===========================================================================

// to_array (lvalue)
//   function: creates an array<remove_cv_t<_Type>, _N> from a
// C-style array, copying each element.
template<typename    _Type,
         std::size_t _N>
D_CONSTEXPR array<typename re_std::remove_cv<_Type>::type, _N>
to_array(
    _Type (&_src)[_N]
)
{
    // multidimensional input forbidden per [array.creation]/p2.
    static_assert(!re_std::is_array<_Type>::value,
        "re_std::to_array: source array element type may not itself be an array");

    return internal::to_array_lvalue(
        _src,
        re_std::make_index_sequence<_N>{});
}


// ===========================================================================
// II.  TO_ARRAY (rvalue)
// ===========================================================================

// to_array (rvalue)
//   function: creates an array<remove_cv_t<_Type>, _N> from a
// C-style array rvalue, moving each element.
template<typename    _Type,
         std::size_t _N>
D_CONSTEXPR array<typename re_std::remove_cv<_Type>::type, _N>
to_array(
    _Type (&&_src)[_N]
)
{
    static_assert(!re_std::is_array<_Type>::value,
        "re_std::to_array: source array element type may not itself be an array");

    return internal::to_array_rvalue(
        static_cast<_Type (&&)[_N]>(_src),
        re_std::make_index_sequence<_N>{});
}


NS_END  // re_std


#endif  // VARIADIC_TEMPLATES && RVALUE_REFERENCES


#endif  // DJINTERP_RE_STD_TO_ARRAY_
