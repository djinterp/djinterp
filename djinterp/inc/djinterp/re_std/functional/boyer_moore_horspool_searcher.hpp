/******************************************************************************
* re_std [functional]                          boyer_moore_horspool_searcher.hpp
*
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
* path:      /inc/djinterp/re_std/functional/boyer_moore_horspool_searcher.hpp
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.08.13
******************************************************************************/

#ifndef RESTD_FUNCTIONAL_BM_HORSPOOL_SEARCHER_
#define RESTD_FUNCTIONAL_BM_HORSPOOL_SEARCHER_ 1

#include "../../djinterp.hpp"

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

#include "../type_traits/type_traits.hpp"
#include "../utility/pair.hpp"
#include "../iterator/iterator_traits.hpp"
#include "./hash.hpp"
#include "./equal_to.hpp"
#include "./searcher_detail.hpp"

NS_DJINTERP
NS_RESTD

// boyer_moore_horspool_searcher
//   class: binds a pattern and searches using the bad-character rule.
template<typename _RandomIt,
         typename _Hash = hash<
             typename iterator_traits<_RandomIt>::value_type>,
         typename _Pred = equal_to<> >
class boyer_moore_horspool_searcher
{
    typedef typename iterator_traits<_RandomIt>::value_type _Value;
    typedef internal::bad_char_table<_Value, _Hash, _Pred>  _Table;

    _RandomIt m_first;
    _RandomIt m_last;
    _Pred     m_pred;
    ptrdiff_t m_size;
    _Table    m_bad;

public:
    boyer_moore_horspool_searcher(_RandomIt pat_first,
                                  _RandomIt pat_last,
                                  _Hash h = _Hash(),
                                  _Pred p = _Pred())
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

    template<typename _RandomIt2>
    pair<_RandomIt2, _RandomIt2>
    operator()(_RandomIt2 first, _RandomIt2 last) const
    {
        const ptrdiff_t n = last - first;
        const ptrdiff_t m = m_size;

        if (m == 0)      { return pair<_RandomIt2, _RandomIt2>(first, first); }
        if (n < m)       { return pair<_RandomIt2, _RandomIt2>(last, last); }

        ptrdiff_t pos = 0;
        while (pos <= n - m)
        {
            ptrdiff_t j = m - 1;
            while (j >= 0 && m_pred(first[pos + j], m_first[j])) { --j; }
            if (j < 0)
            {
                return pair<_RandomIt2, _RandomIt2>(first + pos,
                                                    first + pos + m);
            }
            //   Shift on the LAST window character, not the mismatch.
            const ptrdiff_t occ   = m_bad.get(first[pos + m - 1]);
            const ptrdiff_t shift = m - 1 - occ;
            pos += (shift > 0 ? shift : 1);
        }
        return pair<_RandomIt2, _RandomIt2>(last, last);
    }
};

NS_END
NS_END

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#endif  // RESTD_FUNCTIONAL_BM_HORSPOOL_SEARCHER_
