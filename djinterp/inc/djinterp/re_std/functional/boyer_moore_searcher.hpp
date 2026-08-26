/******************************************************************************
* re_std [functional]                                   boyer_moore_searcher.hpp
*
*   full Boyer-Moore: bad-character rule AND good-suffix rule.
*
*   THE GOOD-SUFFIX RULE IS WHAT BUYS THE WORST-CASE BOUND.
*   Horspool shifts only on the mismatched character and degrades to O(n*m) on
* adversarial input.  Adding the good-suffix rule - which shifts based on what
* ALREADY MATCHED before the mismatch - is what keeps the search sublinear in
* the bad cases.  The cost is a second preprocessing pass and a table the size
* of the pattern.
*
*   THE TABLE CONSTRUCTION IS THE SUBTLE PART, and it is subtle in a specific
* way worth spelling out.  It runs in two phases over an intermediate `suffix`
* array where suffix[i] is the length of the longest suffix of pat[0..i] that
* is also a suffix of the whole pattern:
*
*     phase 1  handles the case where a matched suffix reappears elsewhere in
*              the pattern - shift to that reappearance.
*     phase 2  handles the case where it does not, but a PREFIX of the pattern
*              matches a suffix of what matched - shift to align that prefix.
*
*   WHAT IS ACTUALLY LOAD-BEARING HERE was settled by mutation testing, not by
* reading, because an earlier draft of this comment asserted the opposite and
* was wrong.  Three targeted mutations, each run against the 12000-case fuzz:
*
*     drop phase 1 entirely           -> FAILS (4 cases).  Phase 1 is the
*                                        essential half; without it the table
*                                        over-shifts and silently SKIPS
*                                        matches, returning a well-formed
*                                        "not found".
*     swap the two phases             -> passes.  They are order-independent,
*                                        because phase 2 only writes entries
*                                        still holding the default.
*     drop phase 2's default guard    -> passes.  Redundant in this order,
*                                        since phase 1 runs after it and
*                                        writes unconditionally.  Kept anyway:
*                                        it is what makes the two phases
*                                        order-independent, and removing it
*                                        turns a safe reordering into a bug.
*
*   A fourth mutation - taking the SMALLER of the two shift rules instead of
* the larger - also passes, which is the expected result and confirms that
* each rule is independently safe: the smaller shift only costs speed, never
* correctness.
*
*   The silent-skip failure mode is why this module is fuzzed against a naive
* search rather than spot-checked.  A wrong table does not crash or hang.
*
*   This is the Charras-Lecroq formulation, chosen over the textbook
* double-loop because it is O(m) rather than O(m^2) in the preprocessing.
*
*   STD IS C++17; re_std IS C++11 (floor is re_std::hash - see
* searcher_detail.hpp).
*
* path:      /inc/djinterp/re_std/functional/boyer_moore_searcher.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.08.13
******************************************************************************/

#ifndef DJINTERP_RE_STD_FUNCTIONAL_BOYER_MOORE_SEARCHER_
#define DJINTERP_RE_STD_FUNCTIONAL_BOYER_MOORE_SEARCHER_ 1

// re_std
#include "../../core/djinterp.hpp"

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

#include "../type_traits/type_traits.hpp"
#include "../utility/pair.hpp"
#include "../iterator/iterator_traits.hpp"
#include "./hash.hpp"
#include "./equal_to.hpp"
#include "./searcher_detail.hpp"

NS_RESTD

// boyer_moore_searcher
//   class: binds a pattern and searches using both Boyer-Moore rules.
template<typename _RandomIt,
         typename _Hash = hash<
             typename iterator_traits<_RandomIt>::value_type>,
         typename _Pred = equal_to<> >
class boyer_moore_searcher
{
    typedef typename iterator_traits<_RandomIt>::value_type _Value;
    typedef internal::bad_char_table<_Value, _Hash, _Pred>  _Table;

    _RandomIt                            m_first;
    _RandomIt                            m_last;
    _Pred                                m_pred;
    ptrdiff_t                            m_size;
    _Table                               m_bad;
    internal::searcher_buffer<ptrdiff_t> m_good;

    // build_good_suffix
    //   function: fill m_good, the shift to apply when a mismatch occurs
    // having already matched the pattern's last (m - 1 - j) characters.
    void build_good_suffix()
    {
        const ptrdiff_t m = m_size;
        if (m == 0) { return; }

        internal::searcher_buffer<ptrdiff_t> suffix(
            static_cast<size_t>(m), 0);

        // ---- intermediate: longest suffix of pat[0..i] that is also a
        //      suffix of the whole pattern
        suffix[static_cast<size_t>(m - 1)] = m;
        ptrdiff_t g = m - 1;
        ptrdiff_t f = m - 1;
        for (ptrdiff_t i = m - 2; i >= 0; --i)
        {
            if (i > g
                && suffix[static_cast<size_t>(i + m - 1 - f)] < i - g)
            {
                suffix[static_cast<size_t>(i)] =
                    suffix[static_cast<size_t>(i + m - 1 - f)];
            }
            else
            {
                if (i < g) { g = i; }
                f = i;
                while (g >= 0
                       && m_pred(m_first[g], m_first[g + m - 1 - f]))
                {
                    --g;
                }
                suffix[static_cast<size_t>(i)] = f - g;
            }
        }

        for (ptrdiff_t i = 0; i < m; ++i)
        {
            m_good[static_cast<size_t>(i)] = m;
        }

        // ---- phase 2 first in index order, but it only writes entries still
        //      holding the default, so phase 1 below can overwrite freely
        ptrdiff_t j = 0;
        for (ptrdiff_t i = m - 1; i >= 0; --i)
        {
            if (suffix[static_cast<size_t>(i)] == i + 1)
            {
                for (; j < m - 1 - i; ++j)
                {
                    if (m_good[static_cast<size_t>(j)] == m)
                    {
                        m_good[static_cast<size_t>(j)] = m - 1 - i;
                    }
                }
            }
        }

        // ---- phase 1: a reappearance of the matched suffix elsewhere in the
        //      pattern gives a smaller, always-safe shift, so it wins
        for (ptrdiff_t i = 0; i <= m - 2; ++i)
        {
            m_good[static_cast<size_t>(
                m - 1 - suffix[static_cast<size_t>(i)])] = m - 1 - i;
        }
        return;
    }

public:
    boyer_moore_searcher(_RandomIt pat_first,
                         _RandomIt pat_last,
                         _Hash h = _Hash(),
                         _Pred p = _Pred())
        : m_first(pat_first), m_last(pat_last), m_pred(p),
          m_size(pat_last - pat_first),
          m_bad(static_cast<size_t>(pat_last - pat_first), h, p),
          m_good(static_cast<size_t>(pat_last - pat_first), 0)
    {
        for (ptrdiff_t i = 0; i < m_size; ++i)
        {
            m_bad.set(m_first[i], i);
        }
        build_good_suffix();
        return;
    }

    template<typename _RandomIt2>
    pair<_RandomIt2, _RandomIt2>
    operator()(_RandomIt2 first, _RandomIt2 last) const
    {
        const ptrdiff_t n = last - first;
        const ptrdiff_t m = m_size;

        if (m == 0) { return pair<_RandomIt2, _RandomIt2>(first, first); }
        if (n < m)  { return pair<_RandomIt2, _RandomIt2>(last, last); }

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

            //   Take the LARGER of the two rules' shifts. Both are safe on
            // their own; the larger simply skips more.
            const ptrdiff_t bad_shift =
                j - m_bad.get(first[pos + j]);
            const ptrdiff_t good_shift = m_good[static_cast<size_t>(j)];
            ptrdiff_t shift = bad_shift > good_shift ? bad_shift : good_shift;
            if (shift < 1) { shift = 1; }
            pos += shift;
        }
        return pair<_RandomIt2, _RandomIt2>(last, last);
    }
};

NS_END  // re_std
#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#endif  // DJINTERP_RE_STD_FUNCTIONAL_BOYER_MOORE_SEARCHER_
