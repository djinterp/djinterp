/*******************************************************************************
* djinterp [cpp]                                bubble_sort_tests_primitives.cpp
*
*
* path:      /tests/djinterp/cpp/util/sort/bubble_sort_tests_primitives.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.12
*                                                            revised: 2026.11.12
*******************************************************************************/
// std
#include <algorithm>
// djinterp
// djinterp [test]
#include "bubble_sort_tests.hpp"


NS_DJINTERP
NS_TESTING


// bubble_pass_ints
//   helper: bubble_pass over vector<int>, ascending, on the pairs (i-1, i) for
// i in [_begin, _end).
static
std::ptrdiff_t bubble_pass_ints(
    std::vector<int>& _data,
    std::ptrdiff_t    _begin,
    std::ptrdiff_t    _end
)
{
    return ::djinterp::internal::bubble_pass(_data.begin(),
                                             _begin,
                                             _end,
                                             std::less<int>());
}


// tests_bubble_sort_compare_exchange_is_strict
bool tests_bubble_sort_compare_exchange_is_strict()
{
    verdict                  result;
    std::vector<int>         pair;
    std::vector<DKeyedValue> keyed;

    //   Out of order: the later element strictly precedes, so the pair is
    // exchanged and the call says so.
    pair.push_back(2);
    pair.push_back(1);

    result.check(::djinterp::internal::bubble_compare_exchange(
        pair.begin(), pair.begin() + 1, std::less<int>()));
    result.check((pair[0] == 1) && (pair[1] == 2));

    //   In order: nothing moves.
    result.check(!::djinterp::internal::bubble_compare_exchange(
        pair.begin(), pair.begin() + 1, std::less<int>()));
    result.check((pair[0] == 1) && (pair[1] == 2));

    //   Equivalent: STRICT precedence is required, so two elements with equal
    // keys are never exchanged.  This is the whole of the family's stability.
    keyed.push_back(make_keyed(5, 0));
    keyed.push_back(make_keyed(5, 1));

    result.check(!::djinterp::internal::bubble_compare_exchange(
        keyed.begin(), keyed.begin() + 1, keyed_value_less()));
    result.check((keyed[0].tag == 0) && (keyed[1].tag == 1));

    return result.ok();
}


// tests_bubble_sort_pass_reports_last_exchange
bool tests_bubble_sort_pass_reports_last_exchange()
{
    verdict          result;
    std::vector<int> data;

    //   [3, 1, 2]: the carried 3 travels to the end, exchanging at 1 and 2.
    data.push_back(3);
    data.push_back(1);
    data.push_back(2);

    result.check(bubble_pass_ints(data, 1, 3) == 2);
    result.check((data[0] == 1) && (data[1] == 2) && (data[2] == 3));

    //   Ordered now, so a second pass exchanges nothing and returns 0 -- the
    // value that ends a driver's loop without a separate flag.
    result.check(bubble_pass_ints(data, 1, 3) == 0);

    //   [2, 1, 3, 4, 5]: the only exchange is at the front, so the pass
    // reports 1 although it swept to the end.
    data.clear();
    data.push_back(2);
    data.push_back(1);
    data.push_back(3);
    data.push_back(4);
    data.push_back(5);

    result.check(bubble_pass_ints(data, 1, 5) == 1);
    result.check(is_sorted_range(data.begin(), data.end(), std::less<int>()));

    //   [1, 2, 5, 3, 4]: the 5 is carried from index 2 and exchanged at 3
    // and 4, so the last exchange is the last pair.
    data.clear();
    data.push_back(1);
    data.push_back(2);
    data.push_back(5);
    data.push_back(3);
    data.push_back(4);

    result.check(bubble_pass_ints(data, 1, 5) == 4);
    result.check(is_sorted_range(data.begin(), data.end(), std::less<int>()));

    return result.ok();
}


// tests_bubble_sort_pass_degenerate_runs
bool tests_bubble_sort_pass_degenerate_runs()
{
    verdict          result;
    std::vector<int> data;
    std::vector<int> original;

    data.push_back(9);
    data.push_back(3);
    data.push_back(2);
    data.push_back(1);
    data.push_back(0);
    original = data;

    //   A pass that starts at 0 would read the element before the range, so
    // it is refused: no pairs, nothing moved.
    result.check(bubble_pass_ints(data, 0, 5) == 0);
    result.check(ranges_equal(data, original));

    //   An empty or inverted run has no pairs.
    result.check(bubble_pass_ints(data, 3, 3) == 0);
    result.check(bubble_pass_ints(data, 4, 2) == 0);
    result.check(ranges_equal(data, original));

    //   A sub-run [2, 4) sweeps the pairs (1, 2) and (2, 3) and nothing
    // else: the elements at 0 and 4 stay where they are.  A concurrent
    // driver handing each worker a sub-run relies on exactly this.
    result.check(bubble_pass_ints(data, 2, 4) == 3);
    result.check(data[0] == 9);
    result.check((data[1] == 2) && (data[2] == 1) && (data[3] == 3));
    result.check(data[4] == 0);

    return result.ok();
}


// tests_bubble_sort_pass_bound_invariant
bool tests_bubble_sort_pass_bound_invariant()
{
    verdict          result;
    std::vector<int> data;
    std::vector<int> sorted;
    std::size_t      size_index;
    std::size_t      shape_index;
    std::size_t      passes;
    std::ptrdiff_t   bound;
    std::ptrdiff_t   next;
    std::ptrdiff_t   k;

    //   The driver's shrinking bound is sound only if everything at or after
    // a pass's last exchange is already in its final place.  That is
    // asserted directly, pass by pass, over the whole battery: the bound
    // strictly falls, the suffix it leaves behind matches the sorted
    // reference, and the number of passes never exceeds the length.
    for (size_index = 0; size_index < battery_size_count; ++size_index)
    {
        for (shape_index = 0;
             shape_index < battery_shape_count;
             ++shape_index)
        {
            data   = make_shape(shape_index, battery_size_at(size_index));
            sorted = data;
            std::sort(sorted.begin(), sorted.end());

            bound  = static_cast<std::ptrdiff_t>(data.size());
            passes = 0;

            while (bound > 1)
            {
                next = bubble_pass_ints(data, 1, bound);

                result.check(next < bound);

                for (k = next; k < static_cast<std::ptrdiff_t>(data.size());
                     ++k)
                {
                    result.check(data[static_cast<std::size_t>(k)] ==
                                 sorted[static_cast<std::size_t>(k)]);
                }

                bound = next;
                ++passes;
            }

            result.check(passes <= data.size());
            result.check(ranges_equal(data, sorted));
        }
    }

    return result.ok();
}


NS_END  // testing
NS_END  // djinterp
