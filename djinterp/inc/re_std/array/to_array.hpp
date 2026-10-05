/*******************************************************************************
* djinterp [re_std]                                                 to_array.hpp
*
* to_array factory header:
*   Provides the two C++20 to_array overloads:
*
*     to_array(Type (&)[N])    -> array<remove_cv_t<Type>, N>
*     to_array(Type (&&)[N])   -> array<remove_cv_t<Type>, N>
*
*   Both overloads strip cv-qualification from the element type and
* construct an array<remove_cv_t<Type>, N> by copying (lvalue
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
*   Type may not itself be an array type — to_array on a 2-D array
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
* path:      /inc/re_std/array/to_array.hpp
* link(s):   TBA
* author(s): TBA                                             created: 2026.05.19
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_ARRAY_TO_ARRAY_HPP
#define RE_STD_ARRAY_TO_ARRAY_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration

// gate: to_array requires variadic templates + rvalue references +
// index_sequence — effectively the same set as re_std::make_any.
#if ( RE_STD_LANG_HAS_VARIADIC_TEMPLATES &&                            \
      RE_STD_LANG_HAS_RVALUE_REFERENCES )

// std
#include <cstddef>

#include "./array.hpp"
#include "../type_traits/remove_cv.hpp"
#include "../type_traits/is_array.hpp"
#include "../utility/integer_sequence.hpp"
#include "../utility/make_integer_sequence.hpp"   // make_index_sequence


namespace re_std
{


namespace internal
{

    // to_array_lvalue
    //   function: index-sequence-expansion helper for the lvalue
    // to_array overload. Copy-initialises each element.
    template<typename    Type,
             std::size_t N,
             std::size_t... Is>
    RE_STD_CONSTEXPR array<typename re_std::remove_cv<Type>::type, N>
    to_array_lvalue(
        Type (&_src)[N],
        re_std::index_sequence<Is...>
    )
    {
        return array<typename re_std::remove_cv<Type>::type, N>{
            { _src[Is]... }
        };
    }

    // to_array_rvalue
    //   function: index-sequence-expansion helper for the rvalue
    // to_array overload. Move-initialises each element.
    template<typename    Type,
             std::size_t N,
             std::size_t... Is>
    RE_STD_CONSTEXPR array<typename re_std::remove_cv<Type>::type, N>
    to_array_rvalue(
        Type (&&_src)[N],
        re_std::index_sequence<Is...>
    )
    {
        return array<typename re_std::remove_cv<Type>::type, N>{
            { static_cast<Type&&>(_src[Is])... }
        };
    }

}  // internal


// ===========================================================================
// I.   TO_ARRAY (lvalue)
// ===========================================================================

// to_array (lvalue)
//   function: creates an array<remove_cv_t<Type>, N> from a
// C-style array, copying each element.
template<typename    Type,
         std::size_t N>
RE_STD_CONSTEXPR array<typename re_std::remove_cv<Type>::type, N>
to_array(
    Type (&_src)[N]
)
{
    // multidimensional input forbidden per [array.creation]/p2.
    static_assert(!re_std::is_array<Type>::value,
        "re_std::to_array: source array element type may not itself be an array");

    return internal::to_array_lvalue(
        _src,
        re_std::make_index_sequence<N>{});
}


// ===========================================================================
// II.  TO_ARRAY (rvalue)
// ===========================================================================

// to_array (rvalue)
//   function: creates an array<remove_cv_t<Type>, N> from a
// C-style array rvalue, moving each element.
template<typename    Type,
         std::size_t N>
RE_STD_CONSTEXPR array<typename re_std::remove_cv<Type>::type, N>
to_array(
    Type (&&_src)[N]
)
{
    static_assert(!re_std::is_array<Type>::value,
        "re_std::to_array: source array element type may not itself be an array");

    return internal::to_array_rvalue(
        static_cast<Type (&&)[N]>(_src),
        re_std::make_index_sequence<N>{});
}


}  // re_std


#endif  // VARIADIC_TEMPLATES && RVALUE_REFERENCES


#endif  // RE_STD_ARRAY_TO_ARRAY_HPP
