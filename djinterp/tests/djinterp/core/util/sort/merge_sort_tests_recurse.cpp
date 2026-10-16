/*******************************************************************************
* djinterp [core]                                   merge_sort_tests_recurse.cpp
*
*
* path:      /tests/djinterp/core/util/sort/merge_sort_tests_recurse.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
// djinterp [test]
#include "merge_sort_tests.hpp"


NS_DJINTERP
NS_TESTING


// tests_merge_sort_recurse_base_cases
bool tests_merge_sort_recurse_base_cases()
{
    verdict          result;
    std::vector<int> data;
    std::vector<int> buffer;

    buffer.assign(8, -1);

    //   size == 0 and size == 1 both take the `size <= 1` base case: the
    // recursion returns before touching either the range or the buffer.
    data.clear();

    merge_recurse_ints(data.begin(), data.begin(), buffer.data());

    result.check(data.empty());
    result.check(buffer[0] == -1);

    data.clear();
    data.push_back(42);

    merge_recurse_ints(data.begin(), data.end(), buffer.data());

    result.check(data.size() == 1);
    result.check(data[0] == 42);
    result.check(buffer[0] == -1);

    //   size == 2 is the smallest range that recurses: two base cases and
    // one merge.
    data.clear();
    data.push_back(2);
    data.push_back(1);

    merge_recurse_ints(data.begin(), data.end(), buffer.data());

    result.check(data[0] == 1);
    result.check(data[1] == 2);

    return result.ok();
}


// tests_merge_sort_recurse_ordering_battery
bool tests_merge_sort_recurse_ordering_battery()
{
    verdict          result;
    std::vector<int> work;
    std::vector<int> expected;
    std::vector<int> buffer;
    std::size_t      size_index;
    std::size_t      shape_index;
    std::size_t      length;

    //   The recursive driver is exercised with a CALLER-owned buffer, which
    // is how merge_sort_apply uses it: one allocation shared by every level
    // of the recursion.  Driving it directly separates a fault in the
    // recursion from a fault in the allocation wrapper above it.
    for (size_index = 0; size_index < battery_size_count; ++size_index)
    {
        length = battery_size_at(size_index);

        for (shape_index = 0;
             shape_index < battery_shape_count;
             ++shape_index)
        {
            work     = make_shape(shape_index, length);
            expected = work;

            std::stable_sort(expected.begin(),
                             expected.end(),
                             less<int>());

            buffer.assign(length + 1, 0);

            merge_recurse_ints(work.begin(), work.end(), buffer.data());

            result.check(work == expected);
        }
    }

    return result.ok();
}


// tests_merge_sort_recurse_split_parities
bool tests_merge_sort_recurse_split_parities()
{
    verdict                  result;
    std::vector<int>         work;
    std::vector<int>         expected;
    std::vector<int>         buffer;
    std::vector<keyed_value> keyed;
    std::vector<keyed_value> keyed_buffer;
    std::size_t              length;
    std::size_t              i;

    //   The split is size / 2, so an odd range gives the SMALLER half to the
    // left and an even range splits evenly.  Sweeping every length from 0 to
    // 96 walks both parities at every level of the recursion tree, which no
    // single hand-picked length can do.
    for (length = 0; length <= 96; ++length)
    {
        work     = shape_random(length);
        expected = work;

        std::stable_sort(expected.begin(), expected.end(), less<int>());

        buffer.assign(length + 1, 0);

        merge_recurse_ints(work.begin(), work.end(), buffer.data());

        result.check(work == expected);

        //   The same sweep under a key-only comparator: an off-by-one in the
        // split would show up as a stability break long before it showed up
        // as an ordering break.
        keyed.clear();
        keyed.reserve(length);

        for (i = 0; i < length; ++i)
        {
            keyed.push_back(make_keyed(static_cast<int>((i * 7919) % 5),
                                       static_cast<int>(i)));
        }

        keyed_buffer.assign(length + 1, make_keyed(0, 0));

        ::djinterp::internal::merge_sort_recurse<
            std::vector<keyed_value>::iterator,
            keyed_value,
            keyed_value_less,
            std::ptrdiff_t>(keyed.begin(),
                            keyed.end(),
                            keyed_buffer.data(),
                            keyed_value_less());

        result.check(is_sorted_range(keyed.begin(),
                                     keyed.end(),
                                     keyed_value_less()));
        result.check(is_stable_result(keyed));
    }

    return result.ok();
}


NS_END  // testing
NS_END  // djinterp
