/*******************************************************************************
* djinterp [re_std]                                               destroy_at.hpp
*
* explicit destructor call, normalised to look like a function call:
*   re_std::destroy_at(_p) calls _p->~T(). For arrays (C++20+),
* destroys each element in turn, in undefined order, then unwinds.
*
* portability:
*   re_std back-ports the function to C++11+. constexpr from C++20+
* (matches std). The array overload is only meaningful when
* re_std::is_array is available and the call expression `_p[i]` is
* well-formed for array element access on a pointer-to-array, which
* requires the C++20 array-overload semantics.
*
*
* path:      /inc/re_std/memory/destroy_at.hpp
* link(s):   TBA
* author(s): re_std contributors                             created: 2026.05.01
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_MEMORY_DESTROY_AT_HPP
#define RE_STD_MEMORY_DESTROY_AT_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


#if RE_STD_LANG_IS_CPP11_OR_HIGHER

    #include "re_std/memory/addressof.hpp"
    #include "re_std/type_traits/is_array.hpp"
    #include "re_std/type_traits/enable_if.hpp"


namespace re_std
{

// =============================================================================
// destroy_at  -  scalar overload
// =============================================================================

// destroy_at(_p)
//   function: calls _p->~T(). For non-array T. constexpr from C++20, as
// the banner says and as std's is: a constexpr body that is a statement
// returning void needs C++14, and a destructor call in a constant
// expression needs C++20.
template<typename T>
RE_STD_CONSTEXPR_CPP20
typename enable_if<!is_array<T>::value, void>::type
destroy_at(T* _p)
{
    _p->~T();
}


// =============================================================================
// destroy_at  -  array overload (C++20+)
// =============================================================================

// On C++20+ the standard adds an array overload that destroys each
// element of *_p, in some order. The implementation is a forward loop;
// element-wise destruction by `addressof(elem)->~U()` is itself a
// `destroy_at` call, so this is recursive on remove_extent<T>.
#if RE_STD_LANG_IS_CPP20_OR_HIGHER

    template<typename T>
    constexpr
    typename enable_if<is_array<T>::value, void>::type
    destroy_at(T* _p)
    {
        for (auto& _elem : *_p)
        {
            re_std::destroy_at(re_std::addressof(_elem));
        }
    }

#endif


}  // re_std
#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER

#endif  // RE_STD_MEMORY_DESTROY_AT_HPP
