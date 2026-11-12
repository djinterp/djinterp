/*******************************************************************************
* djinterp [cpp]                                      merge_sort_tests_merge.cpp
*
*
* path:      /tests/djinterp/cpp/util/sort/merge_sort_tests_merge.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.11.12
*******************************************************************************/
// djinterp
// djinterp [test]
#include "merge_sort_tests.hpp"


NS_DJINTERP
NS_TESTING


// ints_from
//   helper: a vector<int> of the first _count values of _values.
static
std::vector<int> ints_from(
    const int*  _values,
    std::size_t _count
)
{
    return std::vector<int>(_values, _values + _count);
}


// tests_merge_sort_merge_runs_basic
bool tests_merge_sort_merge_runs_basic()
{
    static const int interleaved[] = { 1, 3, 5, 2, 4, 6 };
    static const int ascending[]   = { 1, 2, 3, 4, 5, 6 };
    static const int pair[]        = { 2, 1 };

    verdict          result;
    std::vector<int> src;
    std::vector<int> dst;

    //   [1, 3, 5 | 2, 4, 6] -- the fully interleaved case, where the merge
    // alternates runs on every step until the very end.
    src = ints_from(interleaved, 6);
    dst.assign(6, 0);

    merge_runs_ints(src, dst, 0, 3, 6);

    result.check(ranges_equal(dst, ints_from(ascending, 6)));

    //   The source is only read: a merge writes the other buffer, which is
    // what lets the passes alternate between two.
    result.check(ranges_equal(src, ints_from(interleaved, 6)));

    //   One element each side: the smallest span that interleaves at all.
    src = ints_from(pair, 2);
    dst.assign(2, 0);

    merge_runs_ints(src, dst, 0, 1, 2);

    result.check((dst[0] == 1) && (dst[1] == 2));

    return result.ok();
}


// tests_merge_sort_merge_runs_empty_runs
bool tests_merge_sort_merge_runs_empty_runs()
{
    static const int values[] = { 3, 7, 9 };

    verdict          result;
    std::vector<int> src;
    std::vector<int> dst;

    src = ints_from(values, 3);

    //   An empty LEFT run (mid == begin): nothing to interleave, so the span
    // is a straight copy -- with no comparison, since the comparator is not
    // even consulted when one run is empty.
    dst.assign(3, -1);
    merge_runs_ints(src, dst, 0, 0, 3);
    result.check(ranges_equal(dst, src));

    //   An empty RIGHT run (mid == end): the same copy from the other side.
    dst.assign(3, -1);
    merge_runs_ints(src, dst, 0, 3, 3);
    result.check(ranges_equal(dst, src));

    //   An empty span writes nothing at all.
    dst.assign(3, -1);
    merge_runs_ints(src, dst, 2, 2, 2);
    result.check((dst[0] == -1) && (dst[1] == -1) && (dst[2] == -1));

    //   And a span is exactly that: a merge of [1, 2) and [2, 3) leaves the
    // destination's position 0 alone.
    dst.assign(3, -1);
    merge_runs_ints(src, dst, 1, 2, 3);
    result.check((dst[0] == -1) && (dst[1] == 7) && (dst[2] == 9));

    return result.ok();
}


// tests_merge_sort_merge_runs_exhaustion_tails
bool tests_merge_sort_merge_runs_exhaustion_tails()
{
    static const int left_first[]  = { 1, 4, 2, 3, 5 };
    static const int right_first[] = { 3, 4, 5, 1, 2 };
    static const int sorted[]      = { 1, 2, 3, 4, 5 };
    static const int rotation[]    = { 8, 9, 1, 2, 3 };
    static const int rotated[]     = { 1, 2, 3, 8, 9 };

    verdict          result;
    std::vector<int> src;
    std::vector<int> dst;

    //   [1, 4 | 2, 3, 5] exhausts the LEFT run first -- 1, 2, 3, 4 -- so the
    // last element comes from the right; [3, 4, 5 | 1, 2] exhausts the RIGHT
    // run first, so the rest of the span comes from the left.
    src = ints_from(left_first, 5);
    dst.assign(5, 0);
    merge_runs_ints(src, dst, 0, 2, 5);
    result.check(ranges_equal(dst, ints_from(sorted, 5)));

    src = ints_from(right_first, 5);
    dst.assign(5, 0);
    merge_runs_ints(src, dst, 0, 3, 5);
    result.check(ranges_equal(dst, ints_from(sorted, 5)));

    //   Every right element precedes every left one, so the merge is a
    // rotation: the right run drains first, then the whole left run.
    src = ints_from(rotation, 5);
    dst.assign(5, 0);
    merge_runs_ints(src, dst, 0, 2, 5);
    result.check(ranges_equal(dst, ints_from(rotated, 5)));

    return result.ok();
}


// tests_merge_sort_merge_runs_stability
bool tests_merge_sort_merge_runs_stability()
{
    verdict                  result;
    std::vector<DKeyedValue> src;
    std::vector<DKeyedValue> dst;

    //   The merge takes from the right run only when the right element
    // STRICTLY precedes the left one; on an equivalence it takes from the
    // left.  That single asymmetry is the whole of merge sort's stability,
    // so it is asserted where every key is equal and only the tags
    // distinguish the runs.
    src.push_back(make_keyed(5, 0));
    src.push_back(make_keyed(5, 1));
    src.push_back(make_keyed(5, 2));
    src.push_back(make_keyed(5, 3));
    dst.assign(4, make_keyed(0, -1));

    merge_runs_keyed(src, dst, 0, 2, 4);

    result.check(dst[0] == make_keyed(5, 0));
    result.check(dst[1] == make_keyed(5, 1));
    result.check(dst[2] == make_keyed(5, 2));
    result.check(dst[3] == make_keyed(5, 3));

    //   Interleaved equivalents: [ (1,0) (2,1) | (1,2) (2,3) ] must come out
    // with each key group in tag order.
    src.clear();
    src.push_back(make_keyed(1, 0));
    src.push_back(make_keyed(2, 1));
    src.push_back(make_keyed(1, 2));
    src.push_back(make_keyed(2, 3));
    dst.assign(4, make_keyed(0, -1));

    merge_runs_keyed(src, dst, 0, 2, 4);

    result.check(dst[0] == make_keyed(1, 0));
    result.check(dst[1] == make_keyed(1, 2));
    result.check(dst[2] == make_keyed(2, 1));
    result.check(dst[3] == make_keyed(2, 3));
    result.check(is_stable_result(dst));

    return result.ok();
}


// tests_merge_sort_merge_runs_ordered_pair_copied
bool tests_merge_sort_merge_runs_ordered_pair_copied()
{
    static const int ordered[] = { 1, 2, 3, 3, 4, 5 };
    static const int crossed[] = { 1, 2, 4, 3, 5, 6 };

    verdict          result;
    std::vector<int> src;
    std::vector<int> dst;
    std::size_t      comparisons;

    //   When the right run's first element does not precede the left run's
    // last, every left element belongs first: one comparison settles it and
    // the span is copied.  This is the source of merge_sort_traits'
    // is_adaptive, and the equal pair at the seam (3 | 3) is still ordered.
    src         = ints_from(ordered, 6);
    comparisons = 0;
    dst.assign(6, 0);

    ::djinterp::internal::merge_runs(
        src.begin(),
        dst.begin(),
        static_cast<std::ptrdiff_t>(0),
        static_cast<std::ptrdiff_t>(3),
        static_cast<std::ptrdiff_t>(6),
        make_counting(std::less<int>(), &comparisons));

    result.check(comparisons == 1);
    result.check(ranges_equal(dst, src));

    //   One element out of place at the seam is enough to make it merge.
    src         = ints_from(crossed, 6);
    comparisons = 0;
    dst.assign(6, 0);

    ::djinterp::internal::merge_runs(
        src.begin(),
        dst.begin(),
        static_cast<std::ptrdiff_t>(0),
        static_cast<std::ptrdiff_t>(3),
        static_cast<std::ptrdiff_t>(6),
        make_counting(std::less<int>(), &comparisons));

    result.check(comparisons > 1);
    result.check(is_sorted_range(dst.begin(), dst.end(), std::less<int>()));

    return result.ok();
}


NS_END  // testing
NS_END  // djinterp
