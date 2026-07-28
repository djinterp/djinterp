/***********************************************************************
* restd                                                        destroy.hpp
*
* range destruction:
*   restd::destroy(_first, _last) calls destroy_at on each element of
* the half-open range [_first, _last).
*
*   restd::destroy_n(_first, _n) calls destroy_at on each of the
* first _n elements starting at _first, and returns the iterator just
* past the last destroyed element.
*
* portability:
*   Both functions are C++17 in std. restd back-ports to C++11+
* (the implementations dereference forward iterators and take
* addresses, which is fine on any C++ compiler, but destroy_at itself
* needs C++11+).
*
* constexpr:
*   Both functions are constexpr from C++20+ (matches std). On C++11
* and C++14 the bodies are non-constexpr because they iterate.
*
*
* path:      /inc/restd/memory/destroy.hpp
* link(s):   TBA
* author(s): restd contributors                          date: 2026.05.01
***********************************************************************/

#ifndef RESTD_MEMORY_DESTROY_
#define RESTD_MEMORY_DESTROY_ 1

#include "djinterp.hpp"


#if D_ENV_LANG_IS_CPP11_OR_HIGHER

    #include "restd/memory/addressof.hpp"
    #include "restd/memory/destroy_at.hpp"


namespace restd
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
            restd::destroy_at(restd::addressof(*_first));
        }
    }

#else

    template<typename _ForwardIt>
    void destroy(_ForwardIt _first, _ForwardIt _last)
    {
        for (; _first != _last; ++_first)
        {
            restd::destroy_at(restd::addressof(*_first));
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
            restd::destroy_at(restd::addressof(*_first));
        }
        return _first;
    }

#else

    template<typename _ForwardIt, typename _Size>
    _ForwardIt destroy_n(_ForwardIt _first, _Size _n)
    {
        for (; _n > 0; (void)++_first, --_n)
        {
            restd::destroy_at(restd::addressof(*_first));
        }
        return _first;
    }

#endif


}  // namespace restd

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#endif  // RESTD_MEMORY_DESTROY_
