/******************************************************************************
* re_std [functional]                                       default_searcher.hpp
*
*   default_searcher - the naive substring search, packaged as a searcher
* object for use with re_std::search(first, last, searcher).
*
*   STD IS C++17; re_std IS C++98 - a 19-year back-port.
*   Nothing here needs a language feature past C++98: the class stores two
* iterators and a predicate, and operator() is a pair of nested loops
* returning re_std::pair.  std introduced it in C++17 only because the
* SEARCHER PROTOCOL is a C++17 idea, not because the algorithm needed
* anything.
*
*   WHY IT EXISTS AT ALL, given search() already does this.
*   The point of a searcher is that the pattern is bound ONCE and the object
* reused across many haystacks.  For default_searcher there is no
* precomputation to amortise, so it is the baseline: it makes the searcher
* protocol usable without forcing a Boyer-Moore table on a caller whose
* pattern is three characters long, where building the table costs more than
* the search saves.
*
*   COMPLEXITY: O(n*m) worst case, and that is by design - the constant factor
* is tiny and no memory is allocated.
*
* path:      /inc/djinterp/re_std/functional/default_searcher.hpp
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.08.13
******************************************************************************/

#ifndef DJINTERP_RE_STD_FUNCTIONAL_DEFAULT_SEARCHER_
#define DJINTERP_RE_STD_FUNCTIONAL_DEFAULT_SEARCHER_ 1

#include "../../core/djinterp.hpp"
#include "../type_traits/type_traits.hpp"
#include "../utility/pair.hpp"
#include "./equal_to.hpp"

NS_RESTD

// default_searcher
//   class: binds a pattern and searches for it naively.
template<typename _ForwardIt, typename _BinaryPred = equal_to<> >
class default_searcher
{
    _ForwardIt  m_first;
    _ForwardIt  m_last;
    _BinaryPred m_pred;

public:
    D_CONSTEXPR default_searcher(_ForwardIt pat_first,
                                 _ForwardIt pat_last,
                                 _BinaryPred pred = _BinaryPred())
        : m_first(pat_first), m_last(pat_last), m_pred(pred)
    {}

    //   Returns [begin, end) of the first match, or [last, last) when there
    // is none.  Note an EMPTY pattern matches at first, returning
    // [first, first) - std specifies that and callers rely on it.
    template<typename _ForwardIt2>
    D_CONSTEXPR_CPP14 pair<_ForwardIt2, _ForwardIt2>
    operator()(_ForwardIt2 first, _ForwardIt2 last) const
    {
        for (;; ++first)
        {
            _ForwardIt2 hay = first;
            _ForwardIt  pat = m_first;
            for (;; ++hay, ++pat)
            {
                if (pat == m_last)
                {
                    return pair<_ForwardIt2, _ForwardIt2>(first, hay);
                }
                if (hay == last)
                {
                    return pair<_ForwardIt2, _ForwardIt2>(last, last);
                }
                if (!m_pred(*hay, *pat))
                {
                    break;
                }
            }
        }
    }
};

NS_END

#endif  // DJINTERP_RE_STD_FUNCTIONAL_DEFAULT_SEARCHER_
