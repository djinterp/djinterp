/*******************************************************************************
* djinterp [core]                                      heap_sort_tests_apply.cpp
*
*
* path:      /tests/djinterp/core/util/sort/heap_sort_tests_apply.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
// djinterp [test]
#include "heap_sort_tests.hpp"


NS_DJINTERP
NS_TESTING


// tests_heap_sort_apply_degenerate_ranges
bool tests_heap_sort_apply_degenerate_ranges()
{
    verdict          result;
    std::vector<int> empty;
    std::vector<int> single;
    std::vector<int> ordered_pair;
    std::vector<int> reversed_pair;
    std::vector<int> populated;

    //   The size <= 1 guard is load-bearing here in a way it is not for the
    // other algorithms: phase 1 starts at heap_parent(size - 1), so an empty
    // range would compute the parent of index -1 and sift from a negative
    // root.
    heap_apply_ints(empty.begin(), empty.end());
    result.check(empty.empty());

    single.push_back(42);
    heap_apply_ints(single.begin(), single.end());
    result.check(single.size() == 1);
    result.check(single[0] == 42);

    //   Two elements is the smallest range that heapifies: parent(1) is 0,
    // so phase 1 sifts the root once and phase 2 makes a single extraction.
    ordered_pair.push_back(1);
    ordered_pair.push_back(2);
    heap_apply_ints(ordered_pair.begin(), ordered_pair.end());
    result.check(ordered_pair[0] == 1);
    result.check(ordered_pair[1] == 2);

    reversed_pair.push_back(2);
    reversed_pair.push_back(1);
    heap_apply_ints(reversed_pair.begin(), reversed_pair.end());
    result.check(reversed_pair[0] == 1);
    result.check(reversed_pair[1] == 2);

    populated = shape_descending(8);
    heap_apply_ints(populated.begin(), populated.begin());
    result.check(ranges_equal(populated, shape_descending(8)));

    return result.ok();
}


// tests_heap_sort_apply_ordering_battery
bool tests_heap_sort_apply_ordering_battery()
{
    verdict result;

    verify_battery_reference(&heap_apply_ints, less<int>(), result);

    return result.ok();
}


// tests_heap_sort_apply_heap_invariant
bool tests_heap_sort_apply_heap_invariant()
{
    verdict          result;
    std::vector<int> data;
    std::vector<int> expected;
    std::ptrdiff_t   size;
    std::ptrdiff_t   i;
    std::size_t      size_index;
    std::size_t      shape_index;
    std::size_t      length;

    //   heap_sort_apply's two phases are not observable from outside, so
    // they are replicated here from the same primitives and checked
    // separately.  Phase 1 must leave a genuine max-heap -- an ordering
    // check on the final output would pass even if heapify were subtly
    // wrong, because phase 2 would paper over it.
    for (size_index = 0; size_index < battery_size_count; ++size_index)
    {
        length = battery_size_at(size_index);

        if (length < 2)
        {
            continue;
        }

        for (shape_index = 0;
             shape_index < battery_shape_count;
             ++shape_index)
        {
            data     = make_shape(shape_index, length);
            expected = data;
            size     = static_cast<std::ptrdiff_t>(length);

            std::stable_sort(expected.begin(),
                             expected.end(),
                             less<int>());

            // phase 1 -- heapify, bottom-up from the last parent
            i = ::djinterp::internal::heap_parent(size - 1);

            for (;;)
            {
                heap_sift_ints(data.begin(), size, i);

                if (i == 0)
                {
                    break;
                }

                --i;
            }

            result.check(is_max_heap(data.begin(),
                                     data.end(),
                                     less<int>()));

            // the maximum has to be at the root once phase 1 is done
            result.check(data[0] == expected[length - 1]);

            // phase 2 -- extract, shrinking the heap by one each time
            i = (size - 1);

            while (i > 0)
            {
                std::iter_swap(data.begin(), data.begin() + i);
                heap_sift_ints(data.begin(), i, 0);

                //   the invariant survives every extraction: the shrunken
                // prefix is still a heap, and the suffix is already the tail
                // of the sorted output
                result.check(is_max_heap(data.begin(),
                                         data.begin() + i,
                                         less<int>()));

                --i;
            }

            result.check(data == expected);
        }
    }

    return result.ok();
}


NS_END  // testing
NS_END  // djinterp
