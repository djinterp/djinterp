/******************************************************************************
* djinterp [restd]                                                   is_heap.hpp
*
* is_heap algorithm header:
*   Returns true if [_first, _last) is a max-heap under operator< (or
* _comp). Built on is_heap_until: the range is a heap iff the
* until-iterator equals _last.
*
*   PORTABILITY:
*   - std::is_heap is C++11; restd back-ports to C++98.
*   - constexpr in std from C++20 (P0202); restd lifts to C++14.
*   - Requires RandomAccessIterator.
*   - Two overloads: default operator< and custom comparator.
*
*
* path:      /inc/djinterp/re_std/algorithm/is_heap.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.13
******************************************************************************/

#ifndef DJINTERP_RESTD_ALGORITHM_IS_HEAP_
#define DJINTERP_RESTD_ALGORITHM_IS_HEAP_ 1

// djinterp
#include "../../core/djinterp.hpp"
// restd
#include "./is_heap_until.hpp"


// ===========================================================================
// 0.   COMPATIBILITY MACROS
// ===========================================================================

#ifndef D_CONSTEXPR_CPP14
    #if D_ENV_LANG_IS_CPP14_OR_HIGHER
        #define D_CONSTEXPR_CPP14 constexpr
    #else
        #define D_CONSTEXPR_CPP14
    #endif
#endif


NS_RESTD


// ===========================================================================
// I.   IS_HEAP (DEFAULT operator<)
// ===========================================================================

// is_heap
//   function: returns true if [_first, _last) is a max-heap.
template<typename _RandomIt>
D_CONSTEXPR_CPP14 bool
is_heap(
    _RandomIt _first,
    _RandomIt _last
)
{
    return restd::is_heap_until(_first, _last) == _last;
}


// ===========================================================================
// II.  IS_HEAP (COMPARATOR)
// ===========================================================================

// is_heap (comparator)
//   function: as above but using _comp for parent-child comparisons.
template<typename _RandomIt,
         typename _Compare>
D_CONSTEXPR_CPP14 bool
is_heap(
    _RandomIt _first,
    _RandomIt _last,
    _Compare  _comp
)
{
    return restd::is_heap_until(_first, _last, _comp) == _last;
}


NS_END  // restd


#endif  // DJINTERP_RESTD_ALGORITHM_IS_HEAP_
