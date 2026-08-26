/******************************************************************************
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
* path:      /inc/djinterp/re_std/cstdlib/search_sort.hpp
* link(s):   TBA
* author(s): TBA                                           created: 2026.08.25
******************************************************************************/

#ifndef DJINTERP_RE_STD_CSTDLIB_SEARCH_SORT_
#define DJINTERP_RE_STD_CSTDLIB_SEARCH_SORT_ 1

// djinterp
#include "../../core/djinterp.hpp"


#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <cstdlib>


NS_RESTD

    // qsort
    //   function: sort a contiguous block through a type-erased
    // comparator. Not stable; complexity unspecified.
    using ::std::qsort;

    // bsearch
    //   function: binary search a sorted block. Returns a pointer to a
    // matching element, or null. Which match, when there are several, is
    // unspecified.
    using ::std::bsearch;

NS_END  // re_std


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_RE_STD_CSTDLIB_SEARCH_SORT_
