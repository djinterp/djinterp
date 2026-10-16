/*******************************************************************************
* djinterp [core]                                  sort_view_tests_terminals.cpp
*
*
* path:      /tests/djinterp/core/util/sort/sort_view_tests_terminals.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
// djinterp [test]
#include "sort_view_tests.hpp"


NS_DJINTERP
NS_TESTING


// tests_sort_view_default_terminal_applies
bool tests_sort_view_default_terminal_applies()
{
    verdict          result;
    std::vector<int> source;
    std::vector<int> expected;
    std::size_t      i;

    ::djinterp::internal::sort_default_terminal terminal(
        sort_algorithm::quick);

    source   = view_sample();
    expected = source;

    std::stable_sort(expected.begin(), expected.end(), less<int>());

    //   The terminal drains the upstream view into a vector and orders it
    // with the element type's DEFAULT comparator -- ascending.  Driving
    // apply() directly, rather than through operator|, isolates the terminal
    // from the pipe machinery.
    result.check(ranges_equal(
        terminal.apply(source | ::djinterp::views::transform(identity_int())),
        expected));

    //   The upstream is not disturbed: the terminal owns the vector it
    // builds.
    result.check(ranges_equal(source, view_sample()));

    //   The element type of the result follows the view's value_type, not
    // the source container's.  A transform that changes the values must be
    // sorted AFTER the transform, not before -- negating reverses the
    // ordering, so this is the check that would catch a terminal that sorted
    // its input rather than its output.
    expected = view_sample();

    for (i = 0; i < expected.size(); ++i)
    {
        expected[i] = -expected[i];
    }

    std::stable_sort(expected.begin(), expected.end(), less<int>());

    result.check(ranges_equal(
        terminal.apply(source | ::djinterp::views::transform(negate_int())),
        expected));
    result.check(expected[0] == -9);
    result.check(expected[6] == -1);

    return result.ok();
}


// tests_sort_view_with_terminal_applies
bool tests_sort_view_with_terminal_applies()
{
    verdict          result;
    std::vector<int> source;
    std::vector<int> expected;

    ::djinterp::internal::sort_with_terminal<greater<int> > descending(
        greater<int>(), sort_algorithm::quick);

    ::djinterp::internal::sort_with_terminal<less<int> > ascending(
        less<int>(), sort_algorithm::merge);

    source   = view_sample();
    expected = source;

    std::stable_sort(expected.begin(), expected.end(), greater<int>());

    //   sort_with_terminal carries an explicit comparator, stored by value.
    result.check(ranges_equal(
        descending.apply(source | ::djinterp::views::transform(identity_int())),
        expected));

    std::stable_sort(expected.begin(), expected.end(), less<int>());

    result.check(ranges_equal(
        ascending.apply(source | ::djinterp::views::transform(identity_int())),
        expected));

    //   The terminal is const-callable and reusable: applying it twice must
    // give the same answer, which it would not if apply() consumed or
    // mutated the stored comparator.
    result.check(ranges_equal(
        ascending.apply(source | ::djinterp::views::transform(identity_int())),
        expected));

    return result.ok();
}


// tests_sort_view_terminal_algorithm_selection
bool tests_sort_view_terminal_algorithm_selection()
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

    //   The stored algorithm is forwarded to sorter<dynamic_algorithm>, so
    // every enumerator must work and all six must agree on the answer.
    for (i = 0; i < 6; ++i)
    {
        result.check(ranges_equal(
            ::djinterp::internal::sort_default_terminal(every[i]).apply(
                source | ::djinterp::views::transform(identity_int())),
            expected));

        result.check(ranges_equal(
            ::djinterp::internal::sort_with_terminal<less<int> >(
                less<int>(), every[i]).apply(
                    source | ::djinterp::views::transform(identity_int())),
            expected));
    }

    //   An unrecognised enumerator inherits dispatch_runtime's fall-through:
    // the pipeline still drains, but the result comes back in view order.
    // A caller who casts a bad integer gets their data unsorted rather than
    // a diagnostic, so the behaviour is pinned here.
    result.check(ranges_equal(
        ::djinterp::internal::sort_default_terminal(
            unrecognised_view_algorithm()).apply(
                source | ::djinterp::views::transform(identity_int())),
        view_sample()));

    return result.ok();
}


// tests_sort_view_terminal_empty_pipeline
bool tests_sort_view_terminal_empty_pipeline()
{
    verdict          result;
    std::vector<int> empty;
    std::vector<int> single;

    ::djinterp::internal::sort_default_terminal terminal(
        sort_algorithm::quick);

    ::djinterp::internal::sort_with_terminal<greater<int> > descending(
        greater<int>(), sort_algorithm::heap);

    //   A pipeline that yields nothing produces an empty vector, and the
    // sorter is handed an empty range -- the case that would fault if
    // quicksort's short-range guard were missing.
    result.check(
        terminal.apply(empty | ::djinterp::views::transform(identity_int())).empty());
    result.check(
        descending.apply(empty | ::djinterp::views::transform(identity_int())).empty());

    //   One element.
    single.push_back(3);

    result.check(
        terminal.apply(single | ::djinterp::views::transform(identity_int())).size() ==
        1);
    result.check(
        terminal.apply(single | ::djinterp::views::transform(identity_int()))[0] == 3);

    //   A filter that rejects everything is the same empty case reached
    // through a longer pipeline.
    result.check(terminal.apply(
        view_sample_storage() | ::djinterp::views::filter(is_negative())).empty());

    return result.ok();
}


// tests_sort_view_terminal_is_recognised
bool tests_sort_view_terminal_is_recognised()
{
    verdict result;

    //   view.hpp recognises a terminal structurally: a type with a template
    // apply(const View&) whose result is a non-view value.  Both sort
    // terminals must satisfy that probe, because it is what the (view |
    // terminal) and (container | terminal) operator| overloads dispatch on.
    // A trailing-return-type body is used precisely so the probe -- which
    // instantiates only the signature -- never touches the body.
    result.check(
        is_terminal<::djinterp::internal::sort_default_terminal>::value);
    result.check(
        is_terminal<
            ::djinterp::internal::sort_with_terminal<less<int> > >::value);
    result.check(
        is_terminal<
            ::djinterp::internal::sort_with_terminal<
                greater<int> > >::value);

    //   And the factories return exactly those terminal types.
    result.check((std::is_same<
        decltype(sorted()),
        ::djinterp::internal::sort_default_terminal>::value));
    result.check((std::is_same<
        decltype(sorted(sort_algorithm::heap)),
        ::djinterp::internal::sort_default_terminal>::value));
    result.check((std::is_same<
        decltype(sorted_by(less<int>())),
        ::djinterp::internal::sort_with_terminal<less<int> > >::value));

    //   A type that is not a terminal is rejected, so the trait is
    // discriminating rather than vacuously true.
    result.check(!is_terminal<int>::value);
    result.check(!is_terminal<less<int> >::value);

    return result.ok();
}


NS_END  // testing
NS_END  // djinterp
