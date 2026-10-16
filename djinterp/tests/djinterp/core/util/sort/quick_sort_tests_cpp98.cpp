/*******************************************************************************
* djinterp [core]                                     quick_sort_tests_cpp98.cpp
*
*
* path:      /tests/djinterp/core/util/sort/quick_sort_tests_cpp98.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
// djinterp [test]
#include "quick_sort_tests.hpp"


NS_DJINTERP
NS_TESTING


// tests_quick_sort_cpp98_ordering_battery
bool tests_quick_sort_cpp98_ordering_battery()
{
    verdict          result;
    std::vector<int> empty;
    std::vector<int> single;

    //   quick_sort_apply_98 names the element type through a trailing null
    // pointer instead of std::iterator_traits.  It is compiled into every
    // build, so it is driven directly here rather than left to a dialect the
    // harness cannot select.
    quick_apply_98_ints(empty.begin(), empty.end());
    result.check(empty.empty());

    single.push_back(42);
    quick_apply_98_ints(single.begin(), single.end());
    result.check(single.size() == 1);
    result.check(single[0] == 42);

    verify_battery_reference(&quick_apply_98_ints, less<int>(), result);

    return result.ok();
}


// tests_quick_sort_cpp98_parity_with_apply
bool tests_quick_sort_cpp98_parity_with_apply()
{
    verdict          result;
    std::vector<int> traits_path;
    std::vector<int> hint_path;
    std::size_t      traits_comparisons;
    std::size_t      hint_comparisons;
    std::size_t      size_index;
    std::size_t      shape_index;
    std::size_t      length;

    std::vector<keyed_battery_case> keyed_cases;
    std::vector<keyed_value>        keyed_traits;
    std::vector<keyed_value>        keyed_hint;
    std::size_t                     case_index;
    std::size_t                     element_index;

    //   Quicksort is not stable, so identical OUTPUT is a much stronger
    // claim here than it is for the stable algorithms: two variants that
    // partitioned differently would still both be "sorted" yet would place
    // equivalent elements differently.  Element-for-element agreement plus
    // an identical comparison count pins the two bodies together.
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

            ::djinterp::internal::quick_sort_apply(
                traits_path.begin(),
                traits_path.end(),
                make_counting(less<int>(), &traits_comparisons));

            ::djinterp::internal::quick_sort_apply_98(
                hint_path.begin(),
                hint_path.end(),
                make_counting(less<int>(), &hint_comparisons),
                static_cast<int*>(nullptr));

            result.check(ranges_equal(traits_path, hint_path));
            result.check(traits_comparisons == hint_comparisons);
        }
    }

    //   The same parity under a key-only comparator.  Ordering alone cannot
    // catch a divergence here -- only the tags can, because they are the
    // part of an element that a key comparison cannot see.
    keyed_cases = keyed_battery();

    for (case_index = 0; case_index < keyed_cases.size(); ++case_index)
    {
        keyed_traits = keyed_cases[case_index].data;
        keyed_hint   = keyed_traits;

        ::djinterp::internal::quick_sort_apply(keyed_traits.begin(),
                                               keyed_traits.end(),
                                               keyed_value_less());

        quick_apply_98_keyed(keyed_hint.begin(), keyed_hint.end());

        result.check(keyed_traits.size() == keyed_hint.size());

        for (element_index = 0;
             element_index < keyed_traits.size();
             ++element_index)
        {
            result.check(keyed_traits[element_index] ==
                         keyed_hint[element_index]);
        }
    }

    return result.ok();
}


NS_END  // testing
NS_END  // djinterp
