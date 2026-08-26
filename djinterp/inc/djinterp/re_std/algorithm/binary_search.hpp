/******************************************************************************
* djinterp [re_std]                                            binary_search.hpp
*
* binary_search algorithm header:
*   Returns true if _value occurs in the sorted/partitioned range
* [_first, _last) per operator< (or _comp). Built on lower_bound; the
* second comparison checks that the located position holds a value
* equivalent to _value (not just one not-less-than).
*
*   PORTABILITY:
*   - std::binary_search is C++98.
*   - constexpr in std from C++20 (P0202); re_std lifts to C++14.
*   - Two overloads: default operator< and custom comparator.
*
*
* path:      /inc/djinterp/re_std/algorithm/binary_search.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.13
******************************************************************************/

#ifndef DJINTERP_RE_STD_ALGORITHM_BINARY_SEARCH_
#define DJINTERP_RE_STD_ALGORITHM_BINARY_SEARCH_ 1

// djinterp
#include "../../core/djinterp.hpp"
// re_std
#include "./lower_bound.hpp"


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
// I.   BINARY_SEARCH (DEFAULT operator<)
// ===========================================================================

// binary_search
//   function: returns true if some element of [_first, _last) is
// equivalent to _value per operator< (i.e., neither x < _value nor
// _value < x).
template<typename _ForwardIt,
         typename _Type>
D_CONSTEXPR_CPP14 bool
binary_search(
    _ForwardIt   _first,
    _ForwardIt   _last,
    const _Type& _value
)
{
    _first = re_std::lower_bound(_first, _last, _value);
    return (_first != _last) && !(_value < *_first);
}


// ===========================================================================
// II.  BINARY_SEARCH (COMPARATOR)
// ===========================================================================

// binary_search (comparator)
//   function: as above but comparison is via _comp.
template<typename _ForwardIt,
         typename _Type,
         typename _Compare>
D_CONSTEXPR_CPP14 bool
binary_search(
    _ForwardIt   _first,
    _ForwardIt   _last,
    const _Type& _value,
    _Compare     _comp
)
{
    _first = re_std::lower_bound(_first, _last, _value, _comp);
    return (_first != _last) && !_comp(_value, *_first);
}


NS_END  // re_std


#endif  // DJINTERP_RE_STD_ALGORITHM_BINARY_SEARCH_
