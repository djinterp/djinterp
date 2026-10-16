/*******************************************************************************
* djinterp [core]                                  sort_view_tests_factories.cpp
*
*
* path:      /tests/djinterp/core/util/sort/sort_view_tests_factories.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
// djinterp [test]
#include "sort_view_tests.hpp"


NS_DJINTERP
NS_TESTING


// tests_sort_view_sorted_default_algorithm
bool tests_sort_view_sorted_default_algorithm()
{
    verdict          result;
    std::vector<int> source;
    std::vector<int> expected;

    source   = view_sample();
    expected = source;

    std::stable_sort(expected.begin(), expected.end(), less<int>());

    //   sorted() with no argument builds a terminal bound to the subsystem's
    // default algorithm.  The header states that default matches
    // default_sort_tag, which is quicksort -- so the two must agree.
    result.check(ranges_equal(source | sorted(), expected));
    result.check(ranges_equal(source | sorted(),
                              source | sorted(default_sort_tag::value)));
    result.check(ranges_equal(source | sorted(),
                              source | sorted(sort_algorithm::quick)));

    //   Duplicates survive: the terminal drains and sorts, it does not
    // uniquify.
    result.check((source | sorted()).size() == source.size());
    result.check((source | sorted())[0] == 1);
    result.check((source | sorted())[1] == 1);

    return result.ok();
}


// tests_sort_view_sorted_explicit_algorithm
bool tests_sort_view_sorted_explicit_algorithm()
{
    verdict          result;
    std::vector<int> source;
    std::vector<int> expected;
    std::size_t      i;

    sort_algorithm every[6];

    every[0] = sort_algorithm::bubble;
    every[1] = sort_algorithm::selection;
    every[2] = sort_algorithm::insertion;
    every[3] = sort_algorithm::merge;
    every[4] = sort_algorithm::quick;
    every[5] = sort_algorithm::heap;

    source   = view_sample();
    expected = source;

    std::stable_sort(expected.begin(), expected.end(), less<int>());

    //   The one-argument overload lets a caller pin the algorithm.  All six
    // must produce the same ordered result -- the choice is a performance
    // decision, never a semantic one.
    for (i = 0; i < 6; ++i)
    {
        result.check(ranges_equal(source | sorted(every[i]), expected));
    }

    return result.ok();
}


// tests_sort_view_sorted_by_comparator
bool tests_sort_view_sorted_by_comparator()
{
    verdict          result;
    std::vector<int> source;
    std::vector<int> expected;
    greater<int>     named;

    source   = view_sample();
    expected = source;

    std::stable_sort(expected.begin(), expected.end(), greater<int>());

    //   sorted_by() takes the comparator by forwarding reference and decays
    // it into the terminal, so both an lvalue and a temporary work.
    result.check(ranges_equal(source | sorted_by(greater<int>()), expected));
    result.check(ranges_equal(source | sorted_by(named), expected));

    //   A comparator built from the algebra is the documented use case, and
    // it must need no glue at all.
    std::stable_sort(expected.begin(), expected.end(), less<int>());

    result.check(ranges_equal(
        source | sorted_by(::djinterp::natural<int>()), expected));

    std::stable_sort(expected.begin(), expected.end(), greater<int>());

    result.check(ranges_equal(
        source | sorted_by(::djinterp::natural<int>() |
                           ::djinterp::reversed()),
        expected));

    return result.ok();
}


// tests_sort_view_sorted_by_explicit_algorithm
bool tests_sort_view_sorted_by_explicit_algorithm()
{
    verdict          result;
    std::vector<int> source;
    std::vector<int> expected;
    std::size_t      i;

    sort_algorithm every[6];

    every[0] = sort_algorithm::bubble;
    every[1] = sort_algorithm::selection;
    every[2] = sort_algorithm::insertion;
    every[3] = sort_algorithm::merge;
    every[4] = sort_algorithm::quick;
    every[5] = sort_algorithm::heap;

    source   = view_sample();
    expected = source;

    std::stable_sort(expected.begin(), expected.end(), greater<int>());

    //   The two-argument form puts the ALGORITHM first, deliberately: a
    // trailing sort_algorithm would be ambiguous against a comparator, since
    // the comparator parameter is a forwarding reference that would happily
    // bind it.
    for (i = 0; i < 6; ++i)
    {
        result.check(ranges_equal(
            source | sorted_by(every[i], greater<int>()), expected));
    }

    //   The one- and two-argument forms agree when the two-argument form is
    // given the default algorithm.
    result.check(ranges_equal(
        source | sorted_by(greater<int>()),
        source | sorted_by(sort_algorithm::quick, greater<int>())));

    return result.ok();
}


// tests_sort_view_pipe_from_container
bool tests_sort_view_pipe_from_container()
{
    verdict          result;
    std::vector<int> source;
    std::vector<int> expected;
    std::vector<int> empty;
    std::vector<int> single;

    source   = view_sample();
    expected = source;

    std::stable_sort(expected.begin(), expected.end(), less<int>());

    //   The (container | terminal) overload: a plain vector goes straight
    // into the terminal with no explicit view in between.  This is the form
    // the header's usage examples show.
    result.check(ranges_equal(source | sorted(), expected));
    result.check(ranges_equal(source | sorted_by(less<int>()), expected));

    //   The source is left alone -- the terminal returns a new vector.
    result.check(ranges_equal(source, view_sample()));

    //   Degenerate containers.
    result.check((empty | sorted()).empty());
    result.check((empty | sorted_by(greater<int>())).empty());

    single.push_back(3);

    result.check((single | sorted()).size() == 1);
    result.check((single | sorted())[0] == 3);

    return result.ok();
}


// tests_sort_view_pipe_from_adapted_view
bool tests_sort_view_pipe_from_adapted_view()
{
    verdict          result;
    std::vector<int> expected;

    //   The (view | terminal) overload, with real adapters upstream.  The
    // sort must run on the view's OUTPUT, so a transform that reverses the
    // ordering has to be applied before the sort, not after.
    expected.push_back(-9);
    expected.push_back(-6);
    expected.push_back(-5);
    expected.push_back(-4);
    expected.push_back(-2);
    expected.push_back(-1);
    expected.push_back(-1);

    result.check(ranges_equal(
        view_sample_storage() | ::djinterp::views::transform(negate_int()) | sorted(),
        expected));

    //   A filter upstream: only the odd values reach the sort.
    expected.clear();
    expected.push_back(1);
    expected.push_back(1);
    expected.push_back(5);
    expected.push_back(9);

    result.check(ranges_equal(
        view_sample_storage() | ::djinterp::views::filter(is_odd()) | sorted(),
        expected));

    //   Two adapters and an explicit comparator, which is the fully
    // composed form the bridge exists to support.
    expected.clear();
    expected.push_back(-1);
    expected.push_back(-1);
    expected.push_back(-5);
    expected.push_back(-9);

    result.check(ranges_equal(
        view_sample_storage() |
            ::djinterp::views::filter(is_odd()) |
            ::djinterp::views::transform(negate_int()) |
            sorted_by(greater<int>()),
        expected));

    //   And the same pipeline under a non-default algorithm.
    result.check(ranges_equal(
        view_sample_storage() |
            ::djinterp::views::filter(is_odd()) |
            ::djinterp::views::transform(negate_int()) |
            sorted_by(sort_algorithm::merge, greater<int>()),
        expected));

    return result.ok();
}


NS_END  // testing
NS_END  // djinterp
