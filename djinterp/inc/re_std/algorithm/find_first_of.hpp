/*******************************************************************************
* djinterp [re_std]                                            find_first_of.hpp
*
* find_first_of algorithm header:
*   Returns the first iterator in [_first1, _last1) whose element
* matches any element of [_first2, _last2). Two overloads:
*   - default operator==
*   - custom binary predicate
*
*   PORTABILITY:
*   - std::find_first_of is C++98.
*   - O(N*M) naive scan. The needle is not preprocessed; callers wanting
*     better complexity should use a searcher (default_searcher /
*     boyer_moore_searcher in <functional>) with the search overload.
*   - constexpr in std from C++20 (P0202); re_std lifts to C++14.
*
*
* path:      /inc/re_std/algorithm/find_first_of.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.13
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_ALGORITHM_FIND_FIRST_OF_HPP
#define RE_STD_ALGORITHM_FIND_FIRST_OF_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


// ===========================================================================
// 0.   COMPATIBILITY MACROS
// ===========================================================================


namespace re_std
{


// ===========================================================================
// I.   FIND_FIRST_OF (DEFAULT ==)
// ===========================================================================

// find_first_of
//   function: returns the first iterator it in [_first1, _last1) for
// which *it equals some element of [_first2, _last2). Returns _last1 on
// no-match.
template<typename InputIt,
         typename ForwardIt>
RE_STD_CONSTEXPR_CPP14 InputIt
find_first_of(
    InputIt   _first1,
    InputIt   _last1,
    ForwardIt _first2,
    ForwardIt _last2
)
{
    for (; _first1 != _last1; ++_first1)
    {
        for (ForwardIt _it = _first2; _it != _last2; ++_it)
        {
            if (*_first1 == *_it)
            {
                return _first1;
            }
        }
    }

    return _last1;
}


// ===========================================================================
// II.  FIND_FIRST_OF (CUSTOM PRED)
// ===========================================================================

// find_first_of (predicate)
//   function: as above but element comparison is via the user-supplied
// binary predicate _pred.
template<typename InputIt,
         typename ForwardIt,
         typename BinaryPred>
RE_STD_CONSTEXPR_CPP14 InputIt
find_first_of(
    InputIt    _first1,
    InputIt    _last1,
    ForwardIt  _first2,
    ForwardIt  _last2,
    BinaryPred _pred
)
{
    for (; _first1 != _last1; ++_first1)
    {
        for (ForwardIt _it = _first2; _it != _last2; ++_it)
        {
            if (_pred(*_first1, *_it))
            {
                return _first1;
            }
        }
    }

    return _last1;
}


}  // re_std


#endif  // RE_STD_ALGORITHM_FIND_FIRST_OF_HPP
