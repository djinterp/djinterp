/*******************************************************************************
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
* path:      /inc/re_std/algorithm/search.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.13
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_ALGORITHM_SEARCH_HPP
#define RE_STD_ALGORITHM_SEARCH_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


// ===========================================================================
// 0.   COMPATIBILITY MACROS
// ===========================================================================


namespace re_std
{


// ===========================================================================
// I.   SEARCH (DEFAULT ==)
// ===========================================================================

// search
//   function: returns an iterator to the first element of the first
// occurrence of [_first2, _last2) within [_first1, _last1), comparing
// via operator==. Returns _first1 for an empty needle, _last1 on
// no-match.
template<typename ForwardIt1,
         typename ForwardIt2>
RE_STD_CONSTEXPR_CPP14 ForwardIt1
search(
    ForwardIt1 _first1,
    ForwardIt1 _last1,
    ForwardIt2 _first2,
    ForwardIt2 _last2
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
        ForwardIt1 _it1 = _first1;
        ForwardIt2 _it2 = _first2;

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
template<typename ForwardIt1,
         typename ForwardIt2,
         typename BinaryPred>
RE_STD_CONSTEXPR_CPP14 ForwardIt1
search(
    ForwardIt1 _first1,
    ForwardIt1 _last1,
    ForwardIt2 _first2,
    ForwardIt2 _last2,
    BinaryPred _pred
)
{
    if (_first2 == _last2)
    {
        return _first1;
    }

    while (_first1 != _last1)
    {
        ForwardIt1 _it1 = _first1;
        ForwardIt2 _it2 = _first2;

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
template<typename ForwardIt,
         typename Searcher>
RE_STD_CONSTEXPR_CPP14 ForwardIt
search(
    ForwardIt       _first,
    ForwardIt       _last,
    const Searcher& _searcher
)
{
    return _searcher(_first, _last).first;
}


}  // re_std


#endif  // RE_STD_ALGORITHM_SEARCH_HPP
