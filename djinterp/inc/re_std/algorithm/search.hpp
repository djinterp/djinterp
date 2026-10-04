/******************************************************************************
* djinterp [re_std]                                                   search.hpp
*
* search algorithm header:
*   Finds the first occurrence of [_first2, _last2) as a subsequence
* of [_first1, _last1). Three overloads:
*   - default operator==                              (C++98)
*   - custom binary predicate                         (C++98)
*   - generic Searcher callable (C++17 form)          (back-ported)
*
*   PORTABILITY:
*   - std::search (the two C++98 forms) is C++98.
*   - Empty-needle behaviour: per std, an empty needle returns _first1.
*     Note the asymmetry with find_end, which returns _last1.
*   - The C++17 Searcher overload delegates to _searcher(_first, _last)
*     and returns the first iterator of the resulting pair. Per C++17+
*     [func.search.default], any Searcher's operator() returns
*     pair<It, It> — the concrete searcher types (default_searcher,
*     boyer_moore_searcher, boyer_moore_horspool_searcher) live in
*     <functional> and are deferred there; users may supply their own
*     conforming searcher today.
*   - O(N*M) naive scan in the equality / predicate forms.
*   - constexpr in std from C++20 (P0202); re_std lifts to C++14.
*
*
* path:      /inc/djinterp/re_std/algorithm/search.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.13
******************************************************************************/

#ifndef DJINTERP_RE_STD_ALGORITHM_SEARCH_
#define DJINTERP_RE_STD_ALGORITHM_SEARCH_ 1

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
// I.   SEARCH (DEFAULT ==)
// ===========================================================================

// search
//   function: returns an iterator to the first element of the first
// occurrence of [_first2, _last2) within [_first1, _last1), comparing
// via operator==. Returns _first1 for an empty needle, _last1 on
// no-match.
template<typename _ForwardIt1,
         typename _ForwardIt2>
D_CONSTEXPR_CPP14 _ForwardIt1
search(
    _ForwardIt1 _first1,
    _ForwardIt1 _last1,
    _ForwardIt2 _first2,
    _ForwardIt2 _last2
)
{
    // empty needle: return _first1 (matches std; note asymmetry with
    // find_end which returns _last1)
    if (_first2 == _last2)
    {
        return _first1;
    }

    while (_first1 != _last1)
    {
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
            return _first1;
        }

        if (_it1 == _last1)
        {
            return _last1;
        }

        ++_first1;
    }

    return _last1;
}


// ===========================================================================
// II.  SEARCH (CUSTOM PRED)
// ===========================================================================

// search (predicate)
//   function: as above but element comparison is via the user-supplied
// binary predicate _pred.
template<typename _ForwardIt1,
         typename _ForwardIt2,
         typename _BinaryPred>
D_CONSTEXPR_CPP14 _ForwardIt1
search(
    _ForwardIt1 _first1,
    _ForwardIt1 _last1,
    _ForwardIt2 _first2,
    _ForwardIt2 _last2,
    _BinaryPred _pred
)
{
    if (_first2 == _last2)
    {
        return _first1;
    }

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
            return _first1;
        }

        if (_it1 == _last1)
        {
            return _last1;
        }

        ++_first1;
    }

    return _last1;
}


// ===========================================================================
// III. SEARCH (C++17 SEARCHER FORM)
// ===========================================================================

// search (searcher)
//   function: delegates to _searcher(_first, _last) and returns the
// first iterator of the resulting pair (per C++17+ [func.search]).
// The Searcher contract: operator() taking [first, last) and returning
// pair<It, It>. Concrete searcher types live in <functional>.
template<typename _ForwardIt,
         typename _Searcher>
D_CONSTEXPR_CPP14 _ForwardIt
search(
    _ForwardIt       _first,
    _ForwardIt       _last,
    const _Searcher& _searcher
)
{
    return _searcher(_first, _last).first;
}


NS_END  // re_std


#endif  // DJINTERP_RE_STD_ALGORITHM_SEARCH_
