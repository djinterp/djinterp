/******************************************************************************
* re_std [algorithm]                       lexicographical_compare_three_way.hpp
*
*   lexicographical_compare_three_way - the ordering counterpart of
* lexicographical_compare, returning a comparison CATEGORY rather than a bool.
*
*   WHY THE BOOL VERSION IS NOT ENOUGH.
*   lexicographical_compare answers "is a < b", so deciding between less,
* equal and greater takes TWO passes: compare(a, b) then compare(b, a). This
* returns the whole answer in one pass, which is what makes it the right
* primitive for a container's operator<=>.
*
*   THE RETURN TYPE COMES FROM THE COMPARATOR, not from the elements, so the
* caller controls the strength. compare_three_way gives the elements' own
* category; passing compare_partial_order_fallback yields partial_ordering even
* for elements that could manage strong. The trailing return type is spelled as
* decltype of the comparator call for exactly that reason.
*
*   THE TAIL CASES ARE WHERE THIS GOES WRONG IF WRITTEN CARELESSLY. Once one
* range is exhausted the answer is decided by LENGTH, not by any element, and
* the result must still be expressible in the comparator's category. The
* strong_ordering values below convert implicitly to weak_ordering and
* partial_ordering, so the same three returns are correct at every strength.
*
*   STD IS C++20; re_std IS C++20 - a hard ceiling, since the return type is a
* comparison category and those do not exist below C++20.
*
* path:      /inc/djinterp/re_std/algorithm/lexicographical_compare_three_way.hpp
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.08.13
******************************************************************************/

#ifndef DJINTERP_RE_STD_ALGORITHM_LEX_COMPARE_THREE_WAY_
#define DJINTERP_RE_STD_ALGORITHM_LEX_COMPARE_THREE_WAY_ 1

#include "../../core/djinterp.hpp"

#if D_ENV_LANG_IS_CPP20_OR_HIGHER

#include "../type_traits/type_traits.hpp"
#include "../compare/compare"
#include "../functional/compare_three_way.hpp"

NS_RESTD

// lexicographical_compare_three_way
//   function: three-way lexicographical comparison under a supplied
// comparator.
template<typename _InputIt1, typename _InputIt2, typename _Compare>
D_CONSTEXPR auto lexicographical_compare_three_way(_InputIt1 first1,
                                                   _InputIt1 last1,
                                                   _InputIt2 first2,
                                                   _InputIt2 last2,
                                                   _Compare  comp)
    -> decltype(comp(*first1, *first2))
{
    typedef decltype(comp(*first1, *first2)) _Category;

    static_assert(
        is_convertible<_Category, strong_ordering>::value
            || is_convertible<_Category, weak_ordering>::value
            || is_convertible<_Category, partial_ordering>::value,
        "the comparator must return a comparison category type");

    for (; first1 != last1 && first2 != last2; ++first1, ++first2)
    {
        const _Category result = comp(*first1, *first2);
        if (result != 0)
        {
            return result;
        }
    }

    //   Both ranges agree as far as the shorter one goes, so length decides.
    // These strong_ordering values convert to whatever _Category is.
    return first1 != last1 ? static_cast<_Category>(strong_ordering::greater)
         : first2 != last2 ? static_cast<_Category>(strong_ordering::less)
                           : static_cast<_Category>(strong_ordering::equal);
}

// lexicographical_compare_three_way
//   function: as above, comparing elements with compare_three_way.
template<typename _InputIt1, typename _InputIt2>
D_CONSTEXPR auto lexicographical_compare_three_way(_InputIt1 first1,
                                                   _InputIt1 last1,
                                                   _InputIt2 first2,
                                                   _InputIt2 last2)
    -> decltype(compare_three_way()(*first1, *first2))
{
    return re_std::lexicographical_compare_three_way(
        first1, last1, first2, last2, compare_three_way());
}

NS_END

#endif  // D_ENV_LANG_IS_CPP20_OR_HIGHER

#endif  // DJINTERP_RE_STD_ALGORITHM_LEX_COMPARE_THREE_WAY_
