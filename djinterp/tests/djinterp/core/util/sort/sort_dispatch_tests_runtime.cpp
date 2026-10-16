/*******************************************************************************
* djinterp [core]                                sort_dispatch_tests_runtime.cpp
*
*
* path:      /tests/djinterp/core/util/sort/sort_dispatch_tests_runtime.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
// djinterp [test]
#include "sort_dispatch_tests.hpp"


NS_DJINTERP
NS_TESTING


// tests_sort_dispatch_runtime_every_case
bool tests_sort_dispatch_runtime_every_case()
{
    verdict          result;
    std::vector<int> data;
    std::vector<int> expected;
    std::size_t      i;

    sort_algorithm every[6];

    every[0] = sort_algorithm::bubble;
    every[1] = sort_algorithm::selection;
    every[2] = sort_algorithm::insertion;
    every[3] = sort_algorithm::merge;
    every[4] = sort_algorithm::quick;
    every[5] = sort_algorithm::heap;

    //   One case of the switch per enumerator, all six visited.  The input
    // is long enough that quicksort passes its insertion-sort threshold, so
    // no case is quietly answered by the same code as another.
    expected = shape_random(64);
    std::stable_sort(expected.begin(), expected.end(), less<int>());

    for (i = 0; i < 6; ++i)
    {
        data = shape_random(64);

        ::djinterp::internal::dispatch_runtime(every[i],
                                               data.begin(),
                                               data.end(),
                                               less<int>());

        result.check(data == expected);
    }

    //   The comparator is forwarded, not defaulted: the same six cases under
    // a descending comparator.
    expected = shape_random(64);
    std::stable_sort(expected.begin(), expected.end(), greater<int>());

    for (i = 0; i < 6; ++i)
    {
        data = shape_random(64);

        ::djinterp::internal::dispatch_runtime(every[i],
                                               data.begin(),
                                               data.end(),
                                               greater<int>());

        result.check(data == expected);
    }

    return result.ok();
}


// tests_sort_dispatch_runtime_unrecognised_is_noop
bool tests_sort_dispatch_runtime_unrecognised_is_noop()
{
    verdict          result;
    std::vector<int> data;
    std::vector<int> original;
    std::size_t      comparisons;

    //   The switch has no default label, so an enumerator outside the six
    // falls through and the range is left exactly as it was.  The header
    // documents this as a no-op; "no-op" is checked strictly -- not merely
    // "still a permutation", but byte-for-byte unchanged, with the
    // comparator never consulted.
    data        = shape_random(64);
    original    = data;
    comparisons = 0;

    ::djinterp::internal::dispatch_runtime(
        unrecognised_algorithm(),
        data.begin(),
        data.end(),
        make_counting(less<int>(), &comparisons));

    result.check(ranges_equal(data, original));
    result.check(comparisons == 0);

    //   An unsorted range stays unsorted, which is the observable
    // consequence a caller has to be ready for.
    result.check(!is_sorted_range(data.begin(), data.end(), less<int>()));

    return result.ok();
}


// tests_sort_dispatch_runtime_battery
bool tests_sort_dispatch_runtime_battery()
{
    verdict result;

    //   Every algorithm, over every shape and length in the corpus, reached
    // through the run-time switch rather than through its own entry point.
    verify_battery_reference(
        &runtime_dispatch_ints<sort_algorithm::bubble>,
        less<int>(),
        result);
    verify_battery_reference(
        &runtime_dispatch_ints<sort_algorithm::selection>,
        less<int>(),
        result);
    verify_battery_reference(
        &runtime_dispatch_ints<sort_algorithm::insertion>,
        less<int>(),
        result);
    verify_battery_reference(
        &runtime_dispatch_ints<sort_algorithm::merge>,
        less<int>(),
        result);
    verify_battery_reference(
        &runtime_dispatch_ints<sort_algorithm::quick>,
        less<int>(),
        result);
    verify_battery_reference(
        &runtime_dispatch_ints<sort_algorithm::heap>,
        less<int>(),
        result);

    return result.ok();
}


NS_END  // testing
NS_END  // djinterp
