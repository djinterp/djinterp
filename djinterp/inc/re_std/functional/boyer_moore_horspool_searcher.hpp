/*******************************************************************************
* djinterp [re_std]                            boyer_moore_horspool_searcher.hpp
*
* boyer_moore_horspool_searcher class header:
*   Boyer-Moore-Horspool: Boyer-Moore with the bad-character rule only.
*
*   THE TRADE AGAINST FULL BOYER-MOORE.
*   Horspool drops the good-suffix table entirely.  That costs worst-case
* guarantees - it degrades to O(n*m) on adversarial input where full
* Boyer-Moore stays O(n) - but it halves the preprocessing and, on ordinary
* text, is usually FASTER because the good-suffix rule rarely fires and the
* smaller table has better cache behaviour.  std ships both for exactly this
* reason; neither dominates.
*
*   THE SHIFT RULE.
*   Align the pattern at the window, compare from the RIGHT.  On a mismatch,
* look up the haystack character at the window's LAST position - not at the
* mismatch position, which is what plain Boyer-Moore uses - and shift so that
* character lines up with its last occurrence in the pattern.  Using the last
* window position is what makes Horspool's table independent of where the
* mismatch happened.
*
*   STD IS C++17; re_std IS C++11.  The floor is re_std::hash, needed by the
* general bad-character table; see searcher_detail.hpp.
*
*
* path:      /inc/re_std/functional/boyer_moore_horspool_searcher.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.08.13
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_FUNCTIONAL_BOYER_MOORE_HORSPOOL_SEARCHER_HPP
#define RE_STD_FUNCTIONAL_BOYER_MOORE_HORSPOOL_SEARCHER_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration

#if RE_STD_LANG_IS_CPP11_OR_HIGHER

#include "../type_traits/type_traits.hpp"
#include "../utility/pair.hpp"
#include "../iterator/iterator_traits.hpp"
#include "./hash.hpp"
#include "./equal_to.hpp"
#include "./searcher_detail.hpp"

namespace re_std
{

// boyer_moore_horspool_searcher
//   class: binds a pattern and searches using the bad-character rule.
template<typename RandomIt,
         typename Hash = hash<
             typename iterator_traits<RandomIt>::value_type>,
         typename Pred = equal_to<> >
class boyer_moore_horspool_searcher
{
    typedef typename iterator_traits<RandomIt>::value_type _Value;
    typedef internal::bad_char_table<_Value, Hash, Pred>  _Table;

    RandomIt m_first;
    RandomIt m_last;
    Pred     m_pred;
    ptrdiff_t m_size;
    _Table    m_bad;

public:
    boyer_moore_horspool_searcher(RandomIt pat_first,
                                  RandomIt pat_last,
                                  Hash h = Hash(),
                                  Pred p = Pred())
        : m_first(pat_first), m_last(pat_last), m_pred(p),
          m_size(pat_last - pat_first),
          m_bad(static_cast<size_t>(pat_last - pat_first), h, p)
    {
        //   Every position EXCEPT the last: the last character's own entry
        // would make the shift zero and the search would not advance.
        for (ptrdiff_t i = 0; i + 1 < m_size; ++i)
        {
            m_bad.set(m_first[i], i);
        }
        return;
    }

    template<typename RandomIt2>
    pair<RandomIt2, RandomIt2>
    operator()(RandomIt2 first, RandomIt2 last) const
    {
        const ptrdiff_t n = last - first;
        const ptrdiff_t m = m_size;

        if (m == 0)      { return pair<RandomIt2, RandomIt2>(first, first); }
        if (n < m)       { return pair<RandomIt2, RandomIt2>(last, last); }

        ptrdiff_t pos = 0;
        while (pos <= n - m)
        {
            ptrdiff_t j = m - 1;
            while (j >= 0 && m_pred(first[pos + j], m_first[j])) { --j; }
            if (j < 0)
            {
                return pair<RandomIt2, RandomIt2>(first + pos,
                                                    first + pos + m);
            }
            //   Shift on the LAST window character, not the mismatch.
            const ptrdiff_t occ   = m_bad.get(first[pos + m - 1]);
            const ptrdiff_t shift = m - 1 - occ;
            pos += (shift > 0 ? shift : 1);
        }
        return pair<RandomIt2, RandomIt2>(last, last);
    }
};

}

#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER

#endif  // RE_STD_FUNCTIONAL_BOYER_MOORE_HORSPOOL_SEARCHER_HPP
