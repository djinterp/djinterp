/*******************************************************************************
* djinterp [re_std]                                  lexicographical_compare.hpp
*
* lexicographical_compare algorithm header:
*   Dictionary-order strict-less over two ranges: true iff
* [_first1, _last1) compares less than [_first2, _last2).
*
*   The first position at which the ranges differ decides the result.
* If one range is a proper prefix of the other, the SHORTER compares
* less. Two equal ranges compare false, as strict-less requires.
*
*   PORTABILITY:
*   - std::lexicographical_compare is C++98.
*   - constexpr in std from C++20 (P0202); re_std lifts to C++14.
*   - Two overloads: default operator< and custom comparator.
*   - Input iterators suffice: each range is traversed once.
*
*
* path:      /inc/re_std/algorithm/lexicographical_compare.hpp
* link(s):   TBA
* author(s): TBA                                             created: 2026.08.24
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_ALGORITHM_LEXICOGRAPHICAL_COMPARE_HPP
#define RE_STD_ALGORITHM_LEXICOGRAPHICAL_COMPARE_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


// ===========================================================================
// 0.   COMPATIBILITY MACROS
// ===========================================================================


namespace re_std
{


// ===========================================================================
// I.   LEXICOGRAPHICAL_COMPARE (DEFAULT operator<)
// ===========================================================================

// lexicographical_compare
//   function: true iff range 1 precedes range 2 in dictionary order.
template<typename InputIt1,
         typename InputIt2>
RE_STD_CONSTEXPR_CPP14 bool
lexicographical_compare(
    InputIt1 _first1,
    InputIt1 _last1,
    InputIt2 _first2,
    InputIt2 _last2
)
{
    for (; (_first1 != _last1) && (_first2 != _last2);
         ++_first1, (void)++_first2)
    {
        if (*_first1 < *_first2)
        {
            return true;
        }
        if (*_first2 < *_first1)
        {
            return false;
        }
    }

    // common prefix exhausted: range 1 is less exactly when it ran out
    // first and range 2 did not.
    return (_first1 == _last1) && (_first2 != _last2);
}


// ===========================================================================
// II.  LEXICOGRAPHICAL_COMPARE (COMPARATOR)
// ===========================================================================

// lexicographical_compare (comparator)
//   function: as above but ordering is decided by _comp.
template<typename InputIt1,
         typename InputIt2,
         typename Compare>
RE_STD_CONSTEXPR_CPP14 bool
lexicographical_compare(
    InputIt1 _first1,
    InputIt1 _last1,
    InputIt2 _first2,
    InputIt2 _last2,
    Compare  _comp
)
{
    for (; (_first1 != _last1) && (_first2 != _last2);
         ++_first1, (void)++_first2)
    {
        if (_comp(*_first1, *_first2))
        {
            return true;
        }
        if (_comp(*_first2, *_first1))
        {
            return false;
        }
    }

    return (_first1 == _last1) && (_first2 != _last2);
}


}  // re_std


#endif  // RE_STD_ALGORITHM_LEXICOGRAPHICAL_COMPARE_HPP
