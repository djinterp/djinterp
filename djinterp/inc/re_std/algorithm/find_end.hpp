/******************************************************************************
* djinterp [re_std]                                                 find_end.hpp
*
* find_end algorithm header:
*   Returns an iterator to the beginning of the LAST occurrence of
* [_first2, _last2) as a subsequence of [_first1, _last1). Two
* overloads:
*   - default operator==
*   - custom binary predicate
*
*   PORTABILITY:
*   - std::find_end is C++98.
*   - Empty-needle behaviour: per std (C++11+ clarification), an empty
*     needle returns _last1. Note the asymmetry with std::search, which
*     returns _first1 on an empty needle.
*   - O(N*M) naive scan. Boyer-Moore-style preprocessing is a separate
*     facility (default_searcher / boyer_moore_searcher in <functional>).
*   - constexpr in std from C++20 (P0202); re_std lifts to C++14.
*
*
* path:      /inc/djinterp/re_std/algorithm/find_end.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.13
******************************************************************************/

#ifndef DJINTERP_RE_STD_ALGORITHM_FIND_END_
#define DJINTERP_RE_STD_ALGORITHM_FIND_END_ 1

#include "../../core/djinterp.hpp"


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
// I.   FIND_END (DEFAULT ==)
// ===========================================================================

// find_end
//   function: returns an iterator to the first element of the LAST
// occurrence of [_first2, _last2) within [_first1, _last1), comparing
// elements via operator==. Returns _last1 if the needle does not occur
// or if the needle is empty.
template<typename _ForwardIt1,
         typename _ForwardIt2>
D_CONSTEXPR_CPP14 _ForwardIt1
find_end(
    _ForwardIt1 _first1,
    _ForwardIt1 _last1,
    _ForwardIt2 _first2,
    _ForwardIt2 _last2
)
{
    // empty needle: return _last1 (matches std)
    if (_first2 == _last2)
    {
        return _last1;
    }

    _ForwardIt1 _result = _last1;

    while (_first1 != _last1)
    {
        // try to match needle starting at _first1
        _ForwardIt1 _it1 = _first1;
        _ForwardIt2 _it2 = _first2;

        while ( (_it1 != _last1) &&
                (_it2 != _last2) &&
                (*_it1 == *_it2) )
        {
            ++_it1;
            ++_it2;
        }

        if (_it2 == _last2)
        {
            // full needle matched at _first1; remember and keep scanning
            _result = _first1;
        }

        ++_first1;
    }

    return _result;
}


// ===========================================================================
// II.  FIND_END (CUSTOM PRED)
// ===========================================================================

// find_end (predicate)
//   function: as above but element comparison is via the user-supplied
// binary predicate _pred.
template<typename _ForwardIt1,
         typename _ForwardIt2,
         typename _BinaryPred>
D_CONSTEXPR_CPP14 _ForwardIt1
find_end(
    _ForwardIt1 _first1,
    _ForwardIt1 _last1,
    _ForwardIt2 _first2,
    _ForwardIt2 _last2,
    _BinaryPred _pred
)
{
    if (_first2 == _last2)
    {
        return _last1;
    }

    _ForwardIt1 _result = _last1;

    while (_first1 != _last1)
    {
        _ForwardIt1 _it1 = _first1;
        _ForwardIt2 _it2 = _first2;

        while ( (_it1 != _last1) &&
                (_it2 != _last2) &&
                _pred(*_it1, *_it2) )
        {
            ++_it1;
            ++_it2;
        }

        if (_it2 == _last2)
        {
            _result = _first1;
        }

        ++_first1;
    }

    return _result;
}


NS_END  // re_std


#endif  // DJINTERP_RE_STD_ALGORITHM_FIND_END_
