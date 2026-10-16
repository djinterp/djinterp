/*******************************************************************************
* djinterp [core]                                 insertion_sort_tests_cpp98.cpp
*
*
* path:      /tests/djinterp/core/util/sort/insertion_sort_tests_cpp98.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
// djinterp [test]
#include "insertion_sort_tests.hpp"


NS_DJINTERP
NS_TESTING


// tests_insertion_sort_cpp98_degenerate_ranges
bool tests_insertion_sort_cpp98_degenerate_ranges()
{
    verdict          result;
    std::vector<int> empty;
    std::vector<int> single;
    std::vector<int> reversed_pair;

    //   insertion_sort_apply_98 takes the element type through a trailing
    // null pointer instead of std::iterator_traits.  It is compiled into
    // every build, not only C++98 ones, so it is tested directly here rather
    // than left to a dialect the harness cannot select.
    insertion_apply_98_ints(empty.begin(), empty.end());
    result.check(empty.empty());

    single.push_back(42);
    insertion_apply_98_ints(single.begin(), single.end());
    result.check(single.size() == 1);
    result.check(single[0] == 42);

    reversed_pair.push_back(2);
    reversed_pair.push_back(1);
    insertion_apply_98_ints(reversed_pair.begin(), reversed_pair.end());
    result.check(reversed_pair[0] == 1);
    result.check(reversed_pair[1] == 2);

    return result.ok();
}


// tests_insertion_sort_cpp98_ordering_battery
bool tests_insertion_sort_cpp98_ordering_battery()
{
    verdict result;

    verify_battery_reference(&insertion_apply_98_ints, less<int>(), result);

    return result.ok();
}


// tests_insertion_sort_cpp98_parity_with_apply
bool tests_insertion_sort_cpp98_parity_with_apply()
{
    verdict          result;
    std::vector<int> traits_path;
    std::vector<int> hint_path;
    std::size_t      traits_comparisons;
    std::size_t      hint_comparisons;
    std::size_t      size_index;
    std::size_t      shape_index;
    std::size_t      length;

    //   The two drivers are meant to be the same algorithm reached two ways.
    // Parity is asserted on BOTH the output and the comparison count: equal
    // results with different counts would mean the bodies had drifted apart
    // even though they still agree on this corpus.
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

            ::djinterp::internal::insertion_sort_apply(
                traits_path.begin(),
                traits_path.end(),
                make_counting(less<int>(), &traits_comparisons));

            ::djinterp::internal::insertion_sort_apply_98(
                hint_path.begin(),
                hint_path.end(),
                make_counting(less<int>(), &hint_comparisons),
                static_cast<int*>(nullptr));

            result.check(ranges_equal(traits_path, hint_path));
            result.check(traits_comparisons == hint_comparisons);
        }
    }

    return result.ok();
}


// tests_insertion_sort_cpp98_stability
bool tests_insertion_sort_cpp98_stability()
{
    verdict result;

    //   The hint variant keeps the same strict `if (!comp(key, *prev))
    // break;` shift condition, so it must be stable exactly as the
    // traits-deducing one is.
    verify_stability_battery(&insertion_apply_98_keyed, true, result);

    return result.ok();
}


NS_END  // testing
NS_END  // djinterp
