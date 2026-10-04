/***********************************************************************
* re_std                                                    destroy_at.hpp
*
* explicit destructor call, normalised to look like a function call:
*   re_std::destroy_at(_p) calls _p->~_T(). For arrays (C++20+),
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
* path:      /inc/djinterp/re_std/memory/destroy_at.hpp
* link(s):   TBA
* author(s): re_std contributors                         date: 2026.05.01
***********************************************************************/

#ifndef DJINTERP_RE_STD_MEMORY_DESTROY_AT_
#define DJINTERP_RE_STD_MEMORY_DESTROY_AT_ 1

#include "djinterp.hpp"


#if D_ENV_LANG_IS_CPP11_OR_HIGHER

    #include "re_std/memory/addressof.hpp"
    #include "re_std/type_traits/is_array.hpp"
    #include "re_std/type_traits/enable_if.hpp"


namespace re_std
{

// =============================================================================
// destroy_at  -  scalar overload
// =============================================================================

// destroy_at(_p)
//   function: calls _p->~_T(). For non-array _T.
template<typename _T>
D_CONSTEXPR
typename enable_if<!is_array<_T>::value, void>::type
destroy_at(_T* _p)
{
    _p->~_T();
}


// =============================================================================
// destroy_at  -  array overload (C++20+)
// =============================================================================

// On C++20+ the standard adds an array overload that destroys each
// element of *_p, in some order. The implementation is a forward loop;
// element-wise destruction by `addressof(elem)->~U()` is itself a
// `destroy_at` call, so this is recursive on remove_extent<_T>.
#if D_ENV_LANG_IS_CPP20_OR_HIGHER

    template<typename _T>
    constexpr
    typename enable_if<is_array<_T>::value, void>::type
    destroy_at(_T* _p)
    {
        for (auto& _elem : *_p)
        {
            re_std::destroy_at(re_std::addressof(_elem));
        }
    }

#endif


}  // namespace re_std

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#endif  // DJINTERP_RE_STD_MEMORY_DESTROY_AT_
