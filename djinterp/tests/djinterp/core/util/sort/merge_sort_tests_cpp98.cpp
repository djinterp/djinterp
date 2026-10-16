/*******************************************************************************
* djinterp [core]                                     merge_sort_tests_cpp98.cpp
*
*
* path:      /tests/djinterp/core/util/sort/merge_sort_tests_cpp98.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
// djinterp [test]
#include "merge_sort_tests.hpp"


NS_DJINTERP
NS_TESTING


// tests_merge_sort_cpp98_degenerate_ranges
bool tests_merge_sort_cpp98_degenerate_ranges()
{
    verdict          result;
    std::vector<int> empty;
    std::vector<int> single;
    std::vector<int> reversed_pair;

    //   merge_sort_apply_98 names both the element type and the difference
    // type through trailing null pointers instead of std::iterator_traits.
    // It is compiled into every build, so it is driven directly here.
    merge_apply_98_ints(empty.begin(), empty.end());
    result.check(empty.empty());

    single.push_back(42);
    merge_apply_98_ints(single.begin(), single.end());
    result.check(single.size() == 1);
    result.check(single[0] == 42);

    reversed_pair.push_back(2);
    reversed_pair.push_back(1);
    merge_apply_98_ints(reversed_pair.begin(), reversed_pair.end());
    result.check(reversed_pair[0] == 1);
    result.check(reversed_pair[1] == 2);

    return result.ok();
}


// tests_merge_sort_cpp98_ordering_battery
bool tests_merge_sort_cpp98_ordering_battery()
{
    verdict result;

    verify_battery_reference(&merge_apply_98_ints, less<int>(), result);

    return result.ok();
}


// tests_merge_sort_cpp98_parity_with_apply
bool tests_merge_sort_cpp98_parity_with_apply()
{
    verdict          result;
    std::vector<int> traits_path;
    std::vector<int> hint_path;
    std::size_t      traits_comparisons;
    std::size_t      hint_comparisons;
    std::size_t      size_index;
    std::size_t      shape_index;
    std::size_t      length;

    //   Same algorithm, two spellings.  Parity is checked on the output AND
    // the comparison count, so a divergence in the merge condition shows up
    // even where both spellings still happen to produce ordered output.
    for (size_index = 0; size_index < battery_size_count; ++size_index)
    {
        length = battery_size_at(size_index);

        for (shape_index = 0;
             shape_index < battery_shape_count;
             ++shape_index)
        {
            traits_path        = make_shape(shape_index, length);
            hint_path          = traits_path;
            traits_comparisons = 0;
            hint_comparisons   = 0;

            ::djinterp::internal::merge_sort_apply(
                traits_path.begin(),
                traits_path.end(),
                make_counting(less<int>(), &traits_comparisons));

            ::djinterp::internal::merge_sort_apply_98(
                hint_path.begin(),
                hint_path.end(),
                make_counting(less<int>(), &hint_comparisons),
                static_cast<int*>(nullptr),
                static_cast<std::ptrdiff_t*>(nullptr));

            result.check(ranges_equal(traits_path, hint_path));
            result.check(traits_comparisons == hint_comparisons);
        }
    }

    return result.ok();
}


// tests_merge_sort_cpp98_stability
bool tests_merge_sort_cpp98_stability()
{
    verdict result;

    verify_stability_battery(&merge_apply_98_keyed, true, result);

    return result.ok();
}


NS_END  // testing
NS_END  // djinterp
