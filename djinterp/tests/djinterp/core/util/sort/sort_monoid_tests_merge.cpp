/*******************************************************************************
* djinterp [core]                                    sort_monoid_tests_merge.cpp
*
*
* path:      /tests/djinterp/core/util/sort/sort_monoid_tests_merge.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
// djinterp [test]
#include "sort_monoid_tests.hpp"


NS_DJINTERP
NS_TESTING


// tests_sort_monoid_merge_interleaves
bool tests_sort_monoid_merge_interleaves()
{
    verdict          result;
    std::vector<int> merged;

    static const int odds[]     = { 1, 3, 5 };
    static const int evens[]    = { 2, 4, 6 };
    static const int expected[] = { 1, 2, 3, 4, 5, 6 };

    //   The classic interleave: the comparison in the loop head must be
    // consulted on every step, alternating the source.
    merged = ::djinterp::internal::merge_sorted_helper(ints_of(odds, 3),
                                                       ints_of(evens, 3),
                                                       less<int>());

    result.check(ranges_equal(merged, ints_of(expected, 6)));

    //   The mirror image: starting from the other side must interleave the
    // same values, and because 1 strictly precedes 2 the first element now
    // comes from the RIGHT operand -- the one branch of the loop that a
    // left-only merge would never take.
    merged = ::djinterp::internal::merge_sorted_helper(ints_of(evens, 3),
                                                       ints_of(odds, 3),
                                                       less<int>());

    result.check(ranges_equal(merged, ints_of(expected, 6)));

    //   Disjoint runs, right entirely after left: the loop drains the left
    // without ever taking from the right.
    static const int low[]        = { 1, 2 };
    static const int high[]       = { 8, 9 };
    static const int low_high[]   = { 1, 2, 8, 9 };

    merged = ::djinterp::internal::merge_sorted_helper(ints_of(low, 2),
                                                       ints_of(high, 2),
                                                       less<int>());

    result.check(ranges_equal(merged, ints_of(low_high, 4)));

    //   Disjoint the other way round: the loop takes from the right four
    // times before the left is touched.
    merged = ::djinterp::internal::merge_sorted_helper(ints_of(high, 2),
                                                       ints_of(low, 2),
                                                       less<int>());

    result.check(ranges_equal(merged, ints_of(low_high, 4)));

    //   The result length is always the sum of the operand lengths -- no
    // element is dropped and none is duplicated.
    result.check(merged.size() == 4);

    return result.ok();
}


// tests_sort_monoid_merge_empty_operands
bool tests_sort_monoid_merge_empty_operands()
{
    verdict          result;
    std::vector<int> merged;
    std::vector<int> nothing;

    static const int values[] = { 1, 4, 4, 9 };

    //   An empty operand on either side skips the comparison loop entirely
    // and falls straight through to one of the two drain loops.  Both
    // directions are checked because they are different code paths, not
    // symmetric ones.
    merged = ::djinterp::internal::merge_sorted_helper(nothing,
                                                       ints_of(values, 4),
                                                       less<int>());

    result.check(ranges_equal(merged, ints_of(values, 4)));

    merged = ::djinterp::internal::merge_sorted_helper(ints_of(values, 4),
                                                       nothing,
                                                       less<int>());

    result.check(ranges_equal(merged, ints_of(values, 4)));

    //   Two empty operands: every loop is skipped and the result is empty.
    merged = ::djinterp::internal::merge_sorted_helper(nothing,
                                                       nothing,
                                                       less<int>());

    result.check(merged.empty());

    return result.ok();
}


// tests_sort_monoid_merge_exhaustion_tails
bool tests_sort_monoid_merge_exhaustion_tails()
{
    verdict          result;
    std::vector<int> merged;

    static const int single_low[]  = { 0 };
    static const int long_high[]   = { 1, 2, 3, 4, 5 };
    static const int expected_ab[] = { 0, 1, 2, 3, 4, 5 };

    //   The left runs out first, so the SECOND drain loop copies the whole
    // remaining right operand.  With a one-element left run the comparison
    // loop executes exactly once.
    merged = ::djinterp::internal::merge_sorted_helper(
        ints_of(single_low, 1),
        ints_of(long_high, 5),
        less<int>());

    result.check(ranges_equal(merged, ints_of(expected_ab, 6)));

    //   The right runs out first, so the FIRST drain loop copies the
    // remaining left operand.
    merged = ::djinterp::internal::merge_sorted_helper(
        ints_of(long_high, 5),
        ints_of(single_low, 1),
        less<int>());

    result.check(ranges_equal(merged, ints_of(expected_ab, 6)));

    //   Equal-length runs that exhaust together: the comparison loop ends on
    // its own terms and neither drain loop has anything to do.
    static const int a_side[]   = { 1, 3 };
    static const int b_side[]   = { 2, 4 };
    static const int expected[] = { 1, 2, 3, 4 };

    merged = ::djinterp::internal::merge_sorted_helper(ints_of(a_side, 2),
                                                       ints_of(b_side, 2),
                                                       less<int>());

    result.check(ranges_equal(merged, ints_of(expected, 4)));

    //   A long tail on either side, to be sure the drain loops copy every
    // remaining element rather than just the first.
    static const int one[]        = { 9 };
    static const int many[]       = { 1, 2, 3, 4, 5, 6, 7, 8 };
    static const int many_then[]  = { 1, 2, 3, 4, 5, 6, 7, 8, 9 };

    merged = ::djinterp::internal::merge_sorted_helper(ints_of(one, 1),
                                                       ints_of(many, 8),
                                                       less<int>());

    result.check(ranges_equal(merged, ints_of(many_then, 9)));

    merged = ::djinterp::internal::merge_sorted_helper(ints_of(many, 8),
                                                       ints_of(one, 1),
                                                       less<int>());

    result.check(ranges_equal(merged, ints_of(many_then, 9)));

    return result.ok();
}


// tests_sort_monoid_merge_is_stable
bool tests_sort_monoid_merge_is_stable()
{
    verdict                  result;
    std::vector<keyed_value> left;
    std::vector<keyed_value> right;
    std::vector<keyed_value> merged;

    static const int all_ones[]  = { 1, 1 };
    static const int early_tags[] = { 0, 1 };
    static const int later_tags[] = { 2, 3 };

    //   The merge takes from the right operand ONLY when its element
    // strictly precedes the left one.  Under a key-only comparator every
    // element here is equivalent, so the entire left run must come out
    // first -- the a-before-b rule the header documents as making combine a
    // stable merge.
    left   = keyed_of(all_ones, early_tags, 2);
    right  = keyed_of(all_ones, later_tags, 2);
    merged = ::djinterp::internal::merge_sorted_helper(left,
                                                       right,
                                                       keyed_value_less());

    result.check(merged.size() == 4);
    result.check(merged[0] == make_keyed(1, 0));
    result.check(merged[1] == make_keyed(1, 1));
    result.check(merged[2] == make_keyed(1, 2));
    result.check(merged[3] == make_keyed(1, 3));

    //   Swapping the operands proves the rule really is positional rather
    // than a lucky tag ordering: now the higher tags come first, because
    // they are on the left.
    merged = ::djinterp::internal::merge_sorted_helper(right,
                                                       left,
                                                       keyed_value_less());

    result.check(merged[0] == make_keyed(1, 2));
    result.check(merged[1] == make_keyed(1, 3));
    result.check(merged[2] == make_keyed(1, 0));
    result.check(merged[3] == make_keyed(1, 1));

    //   Interleaved keys with ties inside them: equivalents keep
    // a-before-b, distinct keys still order correctly.
    static const int left_keys[]  = { 1, 2, 2 };
    static const int left_tags[]  = { 0, 1, 2 };
    static const int right_keys[] = { 2, 2, 3 };
    static const int right_tags[] = { 3, 4, 5 };

    left   = keyed_of(left_keys, left_tags, 3);
    right  = keyed_of(right_keys, right_tags, 3);
    merged = ::djinterp::internal::merge_sorted_helper(left,
                                                       right,
                                                       keyed_value_less());

    result.check(merged.size() == 6);
    result.check(merged[0] == make_keyed(1, 0));
    result.check(merged[1] == make_keyed(2, 1));
    result.check(merged[2] == make_keyed(2, 2));
    result.check(merged[3] == make_keyed(2, 3));
    result.check(merged[4] == make_keyed(2, 4));
    result.check(merged[5] == make_keyed(3, 5));
    result.check(is_stable_result(merged));

    return result.ok();
}


NS_END  // testing
NS_END  // djinterp
