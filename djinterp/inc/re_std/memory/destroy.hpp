/***********************************************************************
* re_std                                                       destroy.hpp
*
* range destruction:
*   re_std::destroy(_first, _last) calls destroy_at on each element of
* the half-open range [_first, _last).
*
*   re_std::destroy_n(_first, _n) calls destroy_at on each of the
* first _n elements starting at _first, and returns the iterator just
* past the last destroyed element.
*
* portability:
*   Both functions are C++17 in std. re_std back-ports to C++11+
* (the implementations dereference forward iterators and take
* addresses, which is fine on any C++ compiler, but destroy_at itself
* needs C++11+).
*
* constexpr:
*   Both functions are constexpr from C++20+ (matches std). On C++11
* and C++14 the bodies are non-constexpr because they iterate.
*
*
* path:      /inc/djinterp/re_std/memory/destroy.hpp
* link(s):   TBA
* author(s): re_std contributors                         date: 2026.05.01
***********************************************************************/

#ifndef DJINTERP_RE_STD_MEMORY_DESTROY_
#define DJINTERP_RE_STD_MEMORY_DESTROY_ 1

#include "djinterp.hpp"


#if D_ENV_LANG_IS_CPP11_OR_HIGHER

    #include "re_std/memory/addressof.hpp"
    #include "re_std/memory/destroy_at.hpp"


namespace re_std
{

// =============================================================================
// destroy
// =============================================================================

// destroy(_first, _last)
//   function: destroys every element in the half-open range
//             [_first, _last) by calling destroy_at on its address.
#if D_ENV_LANG_IS_CPP20_OR_HIGHER

    template<typename _ForwardIt>
    constexpr void destroy(_ForwardIt _first, _ForwardIt _last)
    {
        for (; _first != _last; ++_first)
        {
            re_std::destroy_at(re_std::addressof(*_first));
        }
    }

#else

    template<typename _ForwardIt>
    void destroy(_ForwardIt _first, _ForwardIt _last)
    {
        for (; _first != _last; ++_first)
        {
            re_std::destroy_at(re_std::addressof(*_first));
        }
    }

#endif


// =============================================================================
// destroy_n
// =============================================================================

// destroy_n(_first, _n)
//   function: destroys _n elements starting at _first. Returns the
//             iterator just past the last destroyed element.
#if D_ENV_LANG_IS_CPP20_OR_HIGHER

    template<typename _ForwardIt, typename _Size>
    constexpr _ForwardIt destroy_n(_ForwardIt _first, _Size _n)
    {
        for (; _n > 0; (void)++_first, --_n)
        {
            re_std::destroy_at(re_std::addressof(*_first));
        }
        return _first;
    }

#else

    template<typename _ForwardIt, typename _Size>
    _ForwardIt destroy_n(_ForwardIt _first, _Size _n)
    {
        for (; _n > 0; (void)++_first, --_n)
        {
            re_std::destroy_at(re_std::addressof(*_first));
        }
        return _first;
    }

#endif


}  // namespace re_std

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#endif  // DJINTERP_RE_STD_MEMORY_DESTROY_
