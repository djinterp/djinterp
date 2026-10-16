/*******************************************************************************
* djinterp [core]                                     merge_sort_tests_merge.cpp
*
*
* path:      /tests/djinterp/core/util/sort/merge_sort_tests_merge.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
// djinterp [test]
#include "merge_sort_tests.hpp"


NS_DJINTERP
NS_TESTING


// tests_merge_sort_merge_halves_basic
bool tests_merge_sort_merge_halves_basic()
{
    verdict          result;
    std::vector<int> data;
    std::vector<int> buffer;
    std::vector<int> expected;
    std::size_t      i;

    //   [1, 3, 5 | 2, 4, 6] -- the fully interleaved case, where the merge
    // alternates halves on every step and neither exhaustion loop runs until
    // the very end.
    data.push_back(1);
    data.push_back(3);
    data.push_back(5);
    data.push_back(2);
    data.push_back(4);
    data.push_back(6);

    buffer.resize(data.size() + 1);

    merge_halves_ints(data.begin(),
                      data.begin() + 3,
                      data.end(),
                      buffer.data());

    for (i = 1; i <= 6; ++i)
    {
        expected.push_back(static_cast<int>(i));
    }

    result.check(ranges_equal(data, expected));

    //   A single element either side: the smallest range for which the
    // interleave loop runs at all.
    data.clear();
    data.push_back(2);
    data.push_back(1);

    buffer.assign(3, 0);

    merge_halves_ints(data.begin(),
                      data.begin() + 1,
                      data.end(),
                      buffer.data());

    result.check(data[0] == 1);
    result.check(data[1] == 2);

    return result.ok();
}


// tests_merge_sort_merge_halves_empty_halves
bool tests_merge_sort_merge_halves_empty_halves()
{
    verdict          result;
    std::vector<int> data;
    std::vector<int> buffer;
    std::vector<int> original;

    buffer.resize(8);

    //   An empty LEFT half (mid == first): the interleave loop is skipped
    // outright and the whole range arrives through the right-hand tail.
    data.clear();
    data.push_back(3);
    data.push_back(7);
    data.push_back(9);
    original = data;

    merge_halves_ints(data.begin(),
                      data.begin(),
                      data.end(),
                      buffer.data());

    result.check(ranges_equal(data, original));

    //   An empty RIGHT half (mid == last): same story through the left-hand
    // tail.
    data.clear();
    data.push_back(3);
    data.push_back(7);
    data.push_back(9);
    original = data;

    merge_halves_ints(data.begin(),
                      data.end(),
                      data.end(),
                      buffer.data());

    result.check(ranges_equal(data, original));

    //   A wholly empty range: every loop bound is zero and nothing is
    // written, including into the buffer.
    data.clear();
    buffer.assign(4, -1);

    merge_halves_ints(data.begin(),
                      data.begin(),
                      data.begin(),
                      buffer.data());

    result.check(data.empty());
    result.check(buffer[0] == -1);

    return result.ok();
}


// tests_merge_sort_merge_halves_exhaustion_tails
bool tests_merge_sort_merge_halves_exhaustion_tails()
{
    verdict          result;
    std::vector<int> data;
    std::vector<int> buffer;
    std::vector<int> expected;
    std::size_t      i;

    buffer.resize(16);

    //   Left exhausts first: [1, 2 | 3, 4, 5].  The interleave loop ends on
    // li == left_size, so the RIGHT-hand tail loop drains 3, 4, 5 and the
    // left-hand tail loop is skipped.
    data.clear();
    data.push_back(1);
    data.push_back(2);
    data.push_back(3);
    data.push_back(4);
    data.push_back(5);

    merge_halves_ints(data.begin(),
                      data.begin() + 2,
                      data.end(),
                      buffer.data());

    expected.clear();

    for (i = 1; i <= 5; ++i)
    {
        expected.push_back(static_cast<int>(i));
    }

    result.check(ranges_equal(data, expected));

    //   Right exhausts first: [3, 4, 5 | 1, 2].  Now the interleave loop
    // ends on ri == total and the LEFT-hand tail loop is the one that runs.
    data.clear();
    data.push_back(3);
    data.push_back(4);
    data.push_back(5);
    data.push_back(1);
    data.push_back(2);

    merge_halves_ints(data.begin(),
                      data.begin() + 3,
                      data.end(),
                      buffer.data());

    result.check(ranges_equal(data, expected));

    //   Fully disjoint in the other direction: every right element precedes
    // every left one, so the merge is a rotation.
    data.clear();
    data.push_back(8);
    data.push_back(9);
    data.push_back(1);
    data.push_back(2);
    data.push_back(3);

    merge_halves_ints(data.begin(),
                      data.begin() + 2,
                      data.end(),
                      buffer.data());

    expected.clear();
    expected.push_back(1);
    expected.push_back(2);
    expected.push_back(3);
    expected.push_back(8);
    expected.push_back(9);

    result.check(ranges_equal(data, expected));

    return result.ok();
}


// tests_merge_sort_merge_halves_stability
bool tests_merge_sort_merge_halves_stability()
{
    verdict                  result;
    std::vector<keyed_value> data;
    std::vector<keyed_value> buffer;

    //   The merge takes from the right half only when the right element
    // STRICTLY precedes the left one; on an equivalence it takes from the
    // left.  That single asymmetry is the whole of merge sort's stability,
    // so it is asserted on a range where every key is equal and only the
    // tags distinguish the halves.
    data.push_back(make_keyed(5, 0));
    data.push_back(make_keyed(5, 1));
    data.push_back(make_keyed(5, 2));
    data.push_back(make_keyed(5, 3));

    buffer.resize(data.size() + 1);

    merge_halves_keyed(data.begin(),
                       data.begin() + 2,
                       data.end(),
                       buffer.data());

    result.check(data[0] == make_keyed(5, 0));
    result.check(data[1] == make_keyed(5, 1));
    result.check(data[2] == make_keyed(5, 2));
    result.check(data[3] == make_keyed(5, 3));

    //   Interleaved equivalents: [ (1,0) (2,1) | (1,2) (2,3) ] must come out
    // with each key group in tag order.
    data.clear();
    data.push_back(make_keyed(1, 0));
    data.push_back(make_keyed(2, 1));
    data.push_back(make_keyed(1, 2));
    data.push_back(make_keyed(2, 3));

    buffer.assign(data.size() + 1, make_keyed(0, 0));

    merge_halves_keyed(data.begin(),
                       data.begin() + 2,
                       data.end(),
                       buffer.data());

    result.check(data[0] == make_keyed(1, 0));
    result.check(data[1] == make_keyed(1, 2));
    result.check(data[2] == make_keyed(2, 1));
    result.check(data[3] == make_keyed(2, 3));
    result.check(is_stable_result(data));

    return result.ok();
}


NS_END  // testing
NS_END  // djinterp
