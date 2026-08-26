/******************************************************************************
* djinterp [re_std]                                    pair_compare_three_way.hpp
*
* pair operator<=> header:
*   Adds the C++20 three-way comparison operator to re_std::pair.
* Returns the common_comparison_category of the two element-wise
* <=> results; lexicographic semantics (first element first, then
* second on equivalence).
*
*     pair<int, int>(1, 2) <=> pair<int, int>(1, 3)
*       -> strong_ordering::less
*     pair<int, double>(1, 1.0) <=> pair<int, double>(1, 2.0)
*       -> partial_ordering::less
*       (common category clamps to partial because of the double)
*
*   IMPLEMENTATION:
*   Two-step lexicographic compare written inline (no recursive
* helper needed — pair has fixed arity 2). The result type is
* precomputed via common_comparison_category_t over the two
* per-element <=> result types. Each per-element <=> result is
* static_cast'd to the common type before comparison-vs-0 and
* return.
*
*   PORTABILITY:
*   Entire file gated on D_ENV_LANG_IS_CPP20_OR_HIGHER. On C++11-17
* the existing classic six comparison operators on pair (shipped
* as part of the original re_std::pair) remain the only comparison
* surface.
*
*
* path:      /inc/djinterp/re_std/utility/pair_compare_three_way.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.17
******************************************************************************/

#ifndef DJINTERP_RE_STD_UTILITY_PAIR_COMPARE_THREE_WAY_
#define DJINTERP_RE_STD_UTILITY_PAIR_COMPARE_THREE_WAY_ 1

// djinterp
#include "../../core/djinterp.hpp"


#if D_ENV_LANG_IS_CPP20_OR_HIGHER


// djinterp
#include "./pair.hpp"
#include "./declval.hpp"
#include "../compare/strong_ordering.hpp"
#include "../compare/common_comparison_category.hpp"


NS_RESTD


// =============================================================================
// I.   OPERATOR<=>
// =============================================================================

// operator<=>
//   function: lexicographic three-way comparison of two pairs. The
// return type is common_comparison_category of the first-element
// and second-element <=> result types.
template<typename _T1,
         typename _T2,
         typename _U1,
         typename _U2>
constexpr common_comparison_category_t<
              decltype(re_std::declval<const _T1&>() <=> re_std::declval<const _U1&>()),
              decltype(re_std::declval<const _T2&>() <=> re_std::declval<const _U2&>())>
operator<=>(
    const pair<_T1, _T2>& _lhs,
    const pair<_U1, _U2>& _rhs
)
{
    typedef common_comparison_category_t<
                decltype(re_std::declval<const _T1&>() <=> re_std::declval<const _U1&>()),
                decltype(re_std::declval<const _T2&>() <=> re_std::declval<const _U2&>())
            > _R;
    _R _r = static_cast<_R>(_lhs.first <=> _rhs.first);
    if (_r != 0)
    {
        return _r;
    }
    return static_cast<_R>(_lhs.second <=> _rhs.second);
}


NS_END  // re_std


#endif  // D_ENV_LANG_IS_CPP20_OR_HIGHER


#endif  // DJINTERP_RE_STD_UTILITY_PAIR_COMPARE_THREE_WAY_
