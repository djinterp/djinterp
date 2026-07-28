/******************************************************************************
* djinterp [restd]                                             find_first_of.hpp
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
*   - constexpr in std from C++20 (P0202); restd lifts to C++14.
*
*
* path:      /inc/djinterp/restd/algorithm/find_first_of.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.13
******************************************************************************/

#ifndef DJINTERP_RESTD_ALGORITHM_FIND_FIRST_OF_
#define DJINTERP_RESTD_ALGORITHM_FIND_FIRST_OF_ 1

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
// I.   FIND_FIRST_OF (DEFAULT ==)
// ===========================================================================

// find_first_of
//   function: returns the first iterator it in [_first1, _last1) for
// which *it equals some element of [_first2, _last2). Returns _last1 on
// no-match.
template<typename _InputIt,
         typename _ForwardIt>
D_CONSTEXPR_CPP14 _InputIt
find_first_of(
    _InputIt   _first1,
    _InputIt   _last1,
    _ForwardIt _first2,
    _ForwardIt _last2
)
{
    for (; _first1 != _last1; ++_first1)
    {
        for (_ForwardIt _it = _first2; _it != _last2; ++_it)
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
template<typename _InputIt,
         typename _ForwardIt,
         typename _BinaryPred>
D_CONSTEXPR_CPP14 _InputIt
find_first_of(
    _InputIt    _first1,
    _InputIt    _last1,
    _ForwardIt  _first2,
    _ForwardIt  _last2,
    _BinaryPred _pred
)
{
    for (; _first1 != _last1; ++_first1)
    {
        for (_ForwardIt _it = _first2; _it != _last2; ++_it)
        {
            if (_pred(*_first1, *_it))
            {
                return _first1;
            }
        }
    }

    return _last1;
}


NS_END  // restd


#endif  // DJINTERP_RESTD_ALGORITHM_FIND_FIRST_OF_
