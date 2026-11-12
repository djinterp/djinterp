/*******************************************************************************
* djinterp [cpp]                                     merge_sort_tests_traits.cpp
*
*
* path:      /tests/djinterp/cpp/util/sort/merge_sort_tests_traits.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.11.12
*******************************************************************************/
// std
#include <type_traits>
// djinterp
// djinterp [test]
#include "merge_sort_tests.hpp"


NS_DJINTERP
NS_TESTING


// tests_merge_sort_traits_values
bool tests_merge_sort_traits_values()
{
    verdict result;

    //   merge_sort_traits pins the three properties a caller can branch on,
    // with the values the C module publishes as D_MERGE_SORT_IS_*.  Merge
    // sort buys stability with a buffer, and is adaptive in comparisons: an
    // ordered pair of runs is detected in one comparison and copied.
    result.check(merge_sort_traits::is_stable);
    result.check(!merge_sort_traits::is_in_place);
    result.check(merge_sort_traits::is_adaptive);

    //   The members are usable in constant expressions, which is the point of
    // publishing them as static const bool.  The dispatch tags inherit this
    // struct, so the dispatch suite checks that link.
    result.check(static_cast<bool>(
        std::integral_constant<bool, merge_sort_traits::is_stable>::value));

    return result.ok();
}


// tests_merge_sort_traits_adaptivity
bool tests_merge_sort_traits_adaptivity()
{
    verdict          result;
    std::vector<int> data;
    std::size_t      size_index;
    std::size_t      length;
    std::size_t      comparisons;

    //   is_adaptive is a claim about cost, so it is checked as one.  On
    // ordered input every pair of runs is already in order and costs one
    // comparison; there are at most n - 1 such pairs across all the passes
    // (n/2 + n/4 + ... + 1), so the whole sort makes at most n - 1
    // comparisons -- where random input costs on the order of n log n.
    for (size_index = 0; size_index < battery_size_count; ++size_index)
    {
        length = battery_size_at(size_index);

        if (length < 2)
        {
            continue;
        }

        data        = shape_ascending(length);
        comparisons = 0;

        merge_sort(data.begin(),
                   data.end(),
                   make_counting(std::less<int>(), &comparisons));

        result.check(comparisons <= (length - 1));
        result.check(is_sorted_range(data.begin(),
                                     data.end(),
                                     std::less<int>()));

        //   all-equal input is ordered too: no right element ever strictly
        // precedes a left one
        data        = shape_constant(length);
        comparisons = 0;

        merge_sort(data.begin(),
                   data.end(),
                   make_counting(std::less<int>(), &comparisons));

        result.check(comparisons <= (length - 1));
    }

    //   and the claim has teeth: shuffled input of the same length costs
    // more than the ordered bound
    data        = shape_random(512);
    comparisons = 0;

    merge_sort(data.begin(),
               data.end(),
               make_counting(std::less<int>(), &comparisons));

    result.check(comparisons > 511);

    return result.ok();
}


NS_END  // testing
NS_END  // djinterp
