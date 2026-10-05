/*******************************************************************************
* djinterp [re_std]                                              make_unique.hpp
*
* factory functions for unique_ptr:
*   make_unique<T>(_args...)       single T,         value-initialized
*   make_unique<T[]>(_n)           dynamic array,     value-initialized
*   make_unique_for_overwrite<T>()    single T,      default-init (C++20+)
*   make_unique_for_overwrite<T[]>(_n) array,         default-init (C++20+)
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
* path:      /inc/re_std/memory/make_unique.hpp
* link(s):   TBA
* author(s): re_std contributors                             created: 2026.05.02
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_MEMORY_MAKE_UNIQUE_HPP
#define RE_STD_MEMORY_MAKE_UNIQUE_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


#if RE_STD_LANG_IS_CPP11_OR_HIGHER

    // std
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

// make_unique<T>(_args...)
//   function: build a unique_ptr<T> by forwarding _args to T's ctor.
//   T must NOT be an array type — array forms have their own overload.
template<typename T, typename... Args>
typename enable_if
<
    !is_array<T>::value,
    unique_ptr<T>
>::type
make_unique(Args&&... _args)
{
    return unique_ptr<T>(new T(re_std::forward<Args>(_args)...));
}


// =============================================================================
// make_unique  -  unbounded-array form (value-initialised)
// =============================================================================

// make_unique<T[]>(_n)
//   function: allocate an array of _n elements with value-init
//             (parenthesised new).
//   T must be an unbounded array type (e.g. int[]); the bounded-array
//   form (e.g. int[5]) has no overload.
template<typename T>
typename enable_if
<
    is_unbounded_array<T>::value,
    unique_ptr<T>
>::type
make_unique(std::size_t _n)
{
    typedef typename remove_extent<T>::type _U;
    return unique_ptr<T>(new _U[_n]());
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

#if RE_STD_LANG_IS_CPP11_OR_HIGHER  // (already true here, kept for clarity)

    // make_unique_for_overwrite<T>()
    //   function: single T, default-initialised.
    template<typename T>
    typename enable_if
    <
        !is_array<T>::value,
        unique_ptr<T>
    >::type
    make_unique_for_overwrite()
    {
        return unique_ptr<T>(new T);
    }

    // make_unique_for_overwrite<T[]>(_n)
    //   function: array of _n elements, default-initialised.
    template<typename T>
    typename enable_if
    <
        is_unbounded_array<T>::value,
        unique_ptr<T>
    >::type
    make_unique_for_overwrite(std::size_t _n)
    {
        typedef typename remove_extent<T>::type _U;
        return unique_ptr<T>(new _U[_n]);
    }

#endif


}  // re_std
#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER

#endif  // RE_STD_MEMORY_MAKE_UNIQUE_HPP
