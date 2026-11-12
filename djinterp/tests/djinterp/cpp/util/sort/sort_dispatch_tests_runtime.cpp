/*******************************************************************************
* djinterp [cpp]                                 sort_dispatch_tests_runtime.cpp
*
*
* path:      /tests/djinterp/cpp/util/sort/sort_dispatch_tests_runtime.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.11.12
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

    DSortAlgorithm every[6];

    every[0] = DSortAlgorithm::bubble;
    every[1] = DSortAlgorithm::selection;
    every[2] = DSortAlgorithm::insertion;
    every[3] = DSortAlgorithm::merge;
    every[4] = DSortAlgorithm::quick;
    every[5] = DSortAlgorithm::heap;

    //   One case of the switch per enumerator, all six visited.  The input
    // is long enough that quicksort passes its insertion-sort threshold, so
    // no case is quietly answered by the same code as another.
    expected = shape_random(64);
    std::stable_sort(expected.begin(), expected.end(), std::less<int>());

    for (i = 0; i < 6; ++i)
    {
        data = shape_random(64);

        ::djinterp::internal::dispatch_runtime(every[i],
                                               data.begin(),
                                               data.end(),
                                               std::less<int>());

        result.check(data == expected);
    }

    //   The comparator is forwarded, not defaulted: the same six cases under
    // a descending comparator.
    expected = shape_random(64);
    std::stable_sort(expected.begin(), expected.end(), std::greater<int>());

    for (i = 0; i < 6; ++i)
    {
        data = shape_random(64);

        ::djinterp::internal::dispatch_runtime(every[i],
                                               data.begin(),
                                               data.end(),
                                               std::greater<int>());

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
        make_counting(std::less<int>(), &comparisons));

    result.check(ranges_equal(data, original));
    result.check(comparisons == 0);

    //   An unsorted range stays unsorted, which is the observable
    // consequence a caller has to be ready for.
    result.check(!is_sorted_range(data.begin(), data.end(), std::less<int>()));

    return result.ok();
}


// tests_sort_dispatch_runtime_battery
bool tests_sort_dispatch_runtime_battery()
{
    verdict result;

    //   Every algorithm, over every shape and length in the corpus, reached
    // through the run-time switch rather than through its own entry point.
    verify_battery_reference(
        &runtime_dispatch_ints<DSortAlgorithm::bubble>,
        std::less<int>(),
        result);
    verify_battery_reference(
        &runtime_dispatch_ints<DSortAlgorithm::selection>,
        std::less<int>(),
        result);
    verify_battery_reference(
        &runtime_dispatch_ints<DSortAlgorithm::insertion>,
        std::less<int>(),
        result);
    verify_battery_reference(
        &runtime_dispatch_ints<DSortAlgorithm::merge>,
        std::less<int>(),
        result);
    verify_battery_reference(
        &runtime_dispatch_ints<DSortAlgorithm::quick>,
        std::less<int>(),
        result);
    verify_battery_reference(
        &runtime_dispatch_ints<DSortAlgorithm::heap>,
        std::less<int>(),
        result);

    return result.ok();
}


NS_END  // testing
NS_END  // djinterp
