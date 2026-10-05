/*******************************************************************************
* djinterp [re_std]                                              search_sort.hpp
*
* qsort and bsearch (re-exports):
*   The C type-erased sort and binary search, taking a comparison through
* a function pointer over void*.
*
*   PREFER re_std::sort AND re_std::lower_bound:
*   <algorithm> shipped in this library and its versions are better on
* every axis that matters. They are type-safe, so a mismatched element
* size cannot compile; they inline the comparison instead of calling
* through a pointer; they work on any random-access range rather than a
* contiguous array; and re_std::sort is O(n log n) worst case, where
* qsort's complexity is unspecified. These two are surfaced for C
* interoperation -- passing a comparator to a C library, or sorting a
* block a C API handed over -- not as a general recommendation.
*
*   THE TRAP THAT MAKES qsort WORTH A COMMENT:
*   The comparator must return an int whose SIGN encodes the ordering.
* Writing `return *(const int*)a - *(const int*)b;` is the classic bug:
* it is correct for small values and overflows into a wrong sign for
* large ones, so the sort silently produces a wrong order rather than
* failing. Compare and return -1 / 0 / 1 instead.
*
*   qsort is also not stable, and both functions are undefined behaviour
* if the comparator is inconsistent.
*
*
* path:      /inc/re_std/cstdlib/search_sort.hpp
* link(s):   TBA
* author(s): TBA                                             created: 2026.08.25
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_CSTDLIB_SEARCH_SORT_HPP
#define RE_STD_CSTDLIB_SEARCH_SORT_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


#if RE_STD_LANG_IS_CPP11_OR_HIGHER

// std
#include <cstdlib>


namespace re_std
{

    // qsort
    //   function: sort a contiguous block through a type-erased
    // comparator. Not stable; complexity unspecified.
    using ::std::qsort;

    // bsearch
    //   function: binary search a sorted block. Returns a pointer to a
    // matching element, or null. Which match, when there are several, is
    // unspecified.
    using ::std::bsearch;

}  // re_std


#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER


#endif  // RE_STD_CSTDLIB_SEARCH_SORT_HPP
