/*******************************************************************************
* djinterp [cpp]                                       merge_sort_tests_pass.cpp
*
*
* path:      /tests/djinterp/cpp/util/sort/merge_sort_tests_pass.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.12
*                                                            revised: 2026.11.12
*******************************************************************************/
// std
#include <algorithm>
// djinterp
// djinterp [test]
#include "merge_sort_tests.hpp"


NS_DJINTERP
NS_TESTING


// tests_merge_sort_pass_short_and_lone_runs
bool tests_merge_sort_pass_short_and_lone_runs()
{
    verdict          result;
    std::vector<int> src;
    std::vector<int> dst;
    std::vector<int> block;
    std::size_t      begin;
    std::size_t      end;

    //   Seven elements in ordered runs of 2, width 2: the runs pair as
    // [0,2)+[2,4) and [4,6)+[6,7) -- the second pair's right run is SHORT.
    // Each block of 2 * width comes out ordered, and only from its own
    // elements: the pairs are disjoint, which is the property a concurrent
    // driver exploits.
    src.push_back(4);
    src.push_back(9);
    src.push_back(1);
    src.push_back(7);
    src.push_back(2);
    src.push_back(8);
    src.push_back(0);
    dst.assign(src.size(), -1);

    merge_pass_ints(src, dst, 2);

    result.check((dst[0] == 1) && (dst[1] == 4) &&
                 (dst[2] == 7) && (dst[3] == 9));
    result.check((dst[4] == 0) && (dst[5] == 2) && (dst[6] == 8));

    //   Width 4 over the same seven: one pair, [0,4)+[4,7).  Then width 8:
    // a LONE run with no partner, which is copied through untouched.
    src.clear();
    src.push_back(1);
    src.push_back(4);
    src.push_back(7);
    src.push_back(9);
    src.push_back(0);
    src.push_back(2);
    src.push_back(8);
    dst.assign(src.size(), -1);

    merge_pass_ints(src, dst, 4);

    result.check(is_sorted_range(dst.begin(), dst.end(), std::less<int>()));
    result.check(is_permutation_of(dst, src));

    src = dst;
    dst.assign(src.size(), -1);

    merge_pass_ints(src, dst, 8);

    result.check(ranges_equal(dst, src));

    //   Over a longer input whose width-3 runs are ordered -- as the passes
    // before this one would have left them -- every block of 6 comes out as
    // the sorted contents of the same block of the source.
    src = shape_random(50);

    for (begin = 0; begin < src.size(); begin += 3)
    {
        end = std::min(begin + 3, src.size());

        std::sort(src.begin() + static_cast<std::ptrdiff_t>(begin),
                  src.begin() + static_cast<std::ptrdiff_t>(end));
    }

    dst.assign(src.size(), -1);

    merge_pass_ints(src, dst, 3);

    for (begin = 0; begin < src.size(); begin += 6)
    {
        end   = std::min(begin + 6, src.size());
        block = std::vector<int>(src.begin() + static_cast<std::ptrdiff_t>(
                                     begin),
                                 src.begin() + static_cast<std::ptrdiff_t>(
                                     end));

        std::stable_sort(block.begin(), block.end());

        result.check(std::equal(block.begin(),
                                block.end(),
                                dst.begin() + static_cast<std::ptrdiff_t>(
                                    begin)));
    }

    return result.ok();
}


// tests_merge_sort_pass_degenerate_widths
bool tests_merge_sort_pass_degenerate_widths()
{
    verdict          result;
    std::vector<int> src;
    std::vector<int> dst;

    src = shape_random(9);

    //   A zero or negative width would never advance, so the pass refuses it
    // and writes nothing.
    dst.assign(src.size(), -1);
    merge_pass_ints(src, dst, 0);
    merge_pass_ints(src, dst, -3);
    result.check(dst == std::vector<int>(src.size(), -1));

    //   An empty range has no runs.
    src.clear();
    dst.assign(4, -1);
    merge_pass_ints(src, dst, 1);
    result.check(dst == std::vector<int>(4, -1));

    //   A width at least the length is a lone run: a copy.
    src = shape_random(9);
    dst.assign(src.size(), -1);
    merge_pass_ints(src, dst, 9);
    result.check(ranges_equal(dst, src));

    return result.ok();
}


// tests_merge_sort_apply_ordering_battery
bool tests_merge_sort_apply_ordering_battery()
{
    verdict result;

    //   The driver with a scratch it is handed: every length and shape,
    // against a std::stable_sort reference.  Driving it directly separates a
    // fault in the passes from one in the entry points that own the buffer.
    verify_battery_reference(&merge_apply_ints, std::less<int>(), result);

    return result.ok();
}


// tests_merge_sort_apply_pass_parities
bool tests_merge_sort_apply_pass_parities()
{
    verdict                  result;
    std::vector<int>         work;
    std::vector<int>         expected;
    std::vector<DKeyedValue> keyed;
    std::vector<DKeyedValue> scratch;
    std::size_t              length;
    std::size_t              i;

    //   Each pass writes the other buffer, so after an ODD number of passes
    // the result sits in the scratch and must be brought home; after an even
    // number it is already in place.  Lengths 2, 3-4, 5-8, 9-16, ... take
    // 1, 2, 3, 4 ... passes, so sweeping 0 to 96 crosses that boundary at
    // every power of two, in both parities.
    for (length = 0; length <= 96; ++length)
    {
        work     = shape_random(length);
        expected = work;
        std::stable_sort(expected.begin(), expected.end());

        merge_apply_ints(work.begin(), work.end());

        result.check(work == expected);

        //   The same sweep under a key-only comparator: a lost write-back or
        // a mis-clamped run would show as a stability break before it showed
        // as an ordering one.
        keyed.clear();
        keyed.reserve(length);

        for (i = 0; i < length; ++i)
        {
            keyed.push_back(make_keyed(static_cast<int>((i * 7919) % 5),
                                       static_cast<int>(i)));
        }

        scratch = keyed;

        ::djinterp::internal::merge_sort_apply(keyed.begin(),
                                               keyed.end(),
                                               scratch.begin(),
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
