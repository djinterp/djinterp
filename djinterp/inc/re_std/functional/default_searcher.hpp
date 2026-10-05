/*******************************************************************************
* djinterp [re_std]                                         default_searcher.hpp
*
* default_searcher class header:
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
*
* path:      /inc/re_std/functional/default_searcher.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.08.13
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_FUNCTIONAL_DEFAULT_SEARCHER_HPP
#define RE_STD_FUNCTIONAL_DEFAULT_SEARCHER_HPP 1

// FLOOR, FOR NOW: below C++11 this header is empty rather than an error
// (README rule 5; re_std omits rather than degrades). The owner's ruling:
// compile at every level first; port to C++98 only where something needs it.
#include "../config.hpp"  // RE_STD_* configuration
#if RE_STD_LANG_IS_CPP11_OR_HIGHER

// re_std
#include "../type_traits/type_traits.hpp"
#include "../utility/pair.hpp"
#include "./equal_to.hpp"


namespace re_std
{

// default_searcher
//   class: binds a pattern and searches for it naively.
template<typename ForwardIt, typename BinaryPred = equal_to<> >
class default_searcher
{
    ForwardIt  m_first;
    ForwardIt  m_last;
    BinaryPred m_pred;

public:
    RE_STD_CONSTEXPR default_searcher(ForwardIt pat_first,
                                 ForwardIt pat_last,
                                 BinaryPred pred = BinaryPred())
        : m_first(pat_first), m_last(pat_last), m_pred(pred)
    {}

    //   Returns [begin, end) of the first match, or [last, last) when there
    // is none.  Note an EMPTY pattern matches at first, returning
    // [first, first) - std specifies that and callers rely on it.
    template<typename ForwardIt2>
    RE_STD_CONSTEXPR_CPP14 pair<ForwardIt2, ForwardIt2>
    operator()(ForwardIt2 first, ForwardIt2 last) const
    {
        for (;; ++first)
        {
            ForwardIt2 hay = first;
            ForwardIt  pat = m_first;
            for (;; ++hay, ++pat)
            {
                if (pat == m_last)
                {
                    return pair<ForwardIt2, ForwardIt2>(first, hay);
                }
                if (hay == last)
                {
                    return pair<ForwardIt2, ForwardIt2>(last, last);
                }
                if (!m_pred(*hay, *pat))
                {
                    break;
                }
            }
        }
    }
};

}

#endif  // floor, for now


#endif  // RE_STD_FUNCTIONAL_DEFAULT_SEARCHER_HPP
