/***********************************************************************
* re_std                                                    make_unique.hpp
*
* factory functions for unique_ptr:
*   make_unique<_T>(_args...)       single _T,         value-initialized
*   make_unique<_T[]>(_n)           dynamic array,     value-initialized
*   make_unique_for_overwrite<_T>()    single _T,      default-init (C++20+)
*   make_unique_for_overwrite<_T[]>(_n) array,         default-init (C++20+)
*
* the bounded-array form (e.g. make_unique<int[5]>) is intentionally
* not provided. With both the non-array overload (1) and the
* unbounded-array overload (2) constrained out, calls of that form
* fail overload resolution at the call site — same end result as the
* standard's = delete spec, with less code.
*
* C++11+ floor:
*   make_unique was added in C++14, re_std back-ports to C++11.
*   make_unique_for_overwrite was added in C++20, re_std back-ports
*   to C++11+.
*
*
* path:      /inc/djinterp/re_std/memory/make_unique.hpp
* link(s):   TBA
* author(s): re_std contributors                         date: 2026.05.02
***********************************************************************/

#ifndef DJINTERP_RE_STD_MEMORY_MAKE_UNIQUE_
#define DJINTERP_RE_STD_MEMORY_MAKE_UNIQUE_ 1

#include "djinterp.hpp"


#if D_ENV_LANG_IS_CPP11_OR_HIGHER

    #include <cstddef>  // size_t

    #include "re_std/memory/unique_ptr.hpp"
    #include "re_std/type_traits/enable_if.hpp"
    #include "re_std/type_traits/is_array.hpp"
    #include "re_std/type_traits/is_unbounded_array.hpp"
    #include "re_std/type_traits/remove_extent.hpp"
    #include "re_std/utility/forward.hpp"


namespace re_std
{

// =============================================================================
// make_unique  -  non-array form (value-initialised)
// =============================================================================

// make_unique<_T>(_args...)
//   function: build a unique_ptr<_T> by forwarding _args to _T's ctor.
//   _T must NOT be an array type — array forms have their own overload.
template<typename _T, typename... _Args>
typename enable_if
<
    !is_array<_T>::value,
    unique_ptr<_T>
>::type
make_unique(_Args&&... _args)
{
    return unique_ptr<_T>(new _T(re_std::forward<_Args>(_args)...));
}


// =============================================================================
// make_unique  -  unbounded-array form (value-initialised)
// =============================================================================

// make_unique<_T[]>(_n)
//   function: allocate an array of _n elements with value-init
//             (parenthesised new).
//   _T must be an unbounded array type (e.g. int[]); the bounded-array
//   form (e.g. int[5]) has no overload.
template<typename _T>
typename enable_if
<
    is_unbounded_array<_T>::value,
    unique_ptr<_T>
>::type
make_unique(std::size_t _n)
{
    typedef typename remove_extent<_T>::type _U;
    return unique_ptr<_T>(new _U[_n]());
}


// =============================================================================
// make_unique_for_overwrite  -  C++20+
// =============================================================================
//
// The "for_overwrite" forms produce default-initialised storage (no
// parens on the new-expression), which for trivial types means
// uninitialised memory. The expectation is that the caller will
// immediately overwrite every element.
//
// std added this in C++20. re_std back-ports to C++11+.

#if D_ENV_LANG_IS_CPP11_OR_HIGHER  // (already true here, kept for clarity)

    // make_unique_for_overwrite<_T>()
    //   function: single _T, default-initialised.
    template<typename _T>
    typename enable_if
    <
        !is_array<_T>::value,
        unique_ptr<_T>
    >::type
    make_unique_for_overwrite()
    {
        return unique_ptr<_T>(new _T);
    }

    // make_unique_for_overwrite<_T[]>(_n)
    //   function: array of _n elements, default-initialised.
    template<typename _T>
    typename enable_if
    <
        is_unbounded_array<_T>::value,
        unique_ptr<_T>
    >::type
    make_unique_for_overwrite(std::size_t _n)
    {
        typedef typename remove_extent<_T>::type _U;
        return unique_ptr<_T>(new _U[_n]);
    }

#endif


}  // namespace re_std

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#endif  // DJINTERP_RE_STD_MEMORY_MAKE_UNIQUE_
