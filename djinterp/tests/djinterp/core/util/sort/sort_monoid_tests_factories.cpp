/*******************************************************************************
* djinterp [core]                                sort_monoid_tests_factories.cpp
*
*
* path:      /tests/djinterp/core/util/sort/sort_monoid_tests_factories.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
// djinterp [test]
#include "sort_monoid_tests.hpp"


NS_DJINTERP
NS_TESTING


// tests_sort_monoid_make_sorted_run
bool tests_sort_monoid_make_sorted_run()
{
    verdict          result;
    std::vector<int> source;
    std::vector<int> expected;
    std::size_t      shape_index;
    std::size_t      size_index;
    std::size_t      length;

    sorted_run<int, greater<int> > descending;

    //   make_sorted_run is the SORTING factory: it takes arbitrary data,
    // orders it with ::djinterp::merge_sort, and wraps the result.  Checked
    // over the whole corpus against std::stable_sort.
    for (size_index = 0; size_index < battery_size_count; ++size_index)
    {
        length = battery_size_at(size_index);

        if (length > 128)
        {
            continue;
        }

        for (shape_index = 0;
             shape_index < battery_shape_count;
             ++shape_index)
        {
            source   = make_shape(shape_index, length);
            expected = source;

            std::stable_sort(expected.begin(),
                             expected.end(),
                             less<int>());

            result.check(ranges_equal(
                make_sorted_run(source, less<int>()).data(), expected));
            result.check(
                make_sorted_run(source, less<int>()).size() == length);
        }
    }

    //   An explicit comparator is honoured, and the run remembers it.
    source     = shape_random(32);
    expected   = source;
    descending = make_sorted_run(source, greater<int>());

    std::stable_sort(expected.begin(), expected.end(), greater<int>());

    result.check(ranges_equal(descending.data(), expected));
    result.check(descending.compare()(2, 1));

    //   The source vector is taken by value, so the caller's copy is
    // untouched.
    result.check(!ranges_equal(source, expected));

    return result.ok();
}


// tests_sort_monoid_make_sorted_run_default
bool tests_sort_monoid_make_sorted_run_default()
{
    verdict          result;
    std::vector<int> source;
    std::vector<int> expected;
    int_run          defaulted;

    source   = shape_random(64);
    expected = source;

    std::stable_sort(expected.begin(), expected.end(), less<int>());

    //   The one-argument overload supplies default_comparator_t<T>, which is
    // less<T> -- ascending.  Its return type is the run parameterised on
    // that comparator, so it interoperates with int_run without a cast.
    defaulted = make_sorted_run(source);

    result.check(ranges_equal(defaulted.data(), expected));
    result.check((std::is_same<decltype(make_sorted_run(source)),
                               int_run>::value));
    result.check(defaulted.compare()(1, 2));

    //   Degenerate inputs.
    result.check(make_sorted_run(std::vector<int>()).empty());
    result.check(make_sorted_run(shape_ascending(1)).size() == 1);

    return result.ok();
}


// tests_sort_monoid_adopt_sorted_run
bool tests_sort_monoid_adopt_sorted_run()
{
    verdict result;

    static const int ordered[]   = { 1, 2, 2, 5 };
    static const int unordered[] = { 5, 1, 2 };

    int_run adopted;
    int_run misused;
    int_run sorted_the_other_way;

    //   adopt_sorted_run is the ZERO-COST path: it wraps without sorting.
    // Distinguishing it from make_sorted_run requires an input where the two
    // would disagree -- so the test feeds it data that violates its
    // precondition and asserts the data comes back verbatim.  A factory that
    // quietly sorted would fail here, and that is exactly the regression
    // worth catching, because the two factories are one keyword apart at the
    // call site.
    adopted = adopt_sorted_run(ints_of(ordered, 4), less<int>());
    misused = adopt_sorted_run(ints_of(unordered, 3), less<int>());

    result.check(ranges_equal(adopted.data(), ints_of(ordered, 4)));
    result.check(adopted.size() == 4);

    result.check(ranges_equal(misused.data(), ints_of(unordered, 3)));
    result.check(misused.data()[0] == 5);

    //   For comparison, the sorting factory on the same input.
    sorted_the_other_way = make_sorted_run(ints_of(unordered, 3));

    result.check(sorted_the_other_way.data()[0] == 1);
    result.check(!ranges_equal(sorted_the_other_way.data(),
                               misused.data()));

    //   Adopting nothing yields the identity.
    result.check(adopt_sorted_run(std::vector<int>(), less<int>()).empty());

    //   An adopted run participates in the algebra like any other.
    result.check(mappend(adopted, adopted).size() == 8);

    return result.ok();
}


// tests_sort_monoid_singleton_run
bool tests_sort_monoid_singleton_run()
{
    verdict result;

    sorted_run<int, greater<int> > descending_one;
    int_run                        defaulted_one;
    keyed_run                      keyed_one;

    //   A one-element run is trivially sorted, so the factory needs no
    // comparison at all -- it is the atom the bottom-up merge sort is built
    // from.
    defaulted_one = singleton_run(7);

    result.check(defaulted_one.size() == 1);
    result.check(!defaulted_one.empty());
    result.check(defaulted_one.data()[0] == 7);
    result.check((std::is_same<decltype(singleton_run(7)),
                               int_run>::value));

    //   With an explicit comparator, which the run carries into any later
    // combine.
    descending_one = singleton_run(7, greater<int>());

    result.check(descending_one.size() == 1);
    result.check(descending_one.data()[0] == 7);
    result.check(descending_one.compare()(2, 1));

    //   Two singletons combine into an ordered pair, which is the base case
    // of the fold.
    result.check(mappend(singleton_run(2), singleton_run(1)).data()[0] == 1);
    result.check(mappend(singleton_run(1), singleton_run(2)).data()[0] == 1);

    //   And under a descending comparator the same pair comes out the other
    // way round -- the comparator travels with the LEFT operand.
    result.check(mappend(singleton_run(1, greater<int>()),
                         singleton_run(2, greater<int>())).data()[0] == 2);

    //   Singletons of a user type work the same way, and the merge's
    // a-before-b rule makes two equivalent singletons keep their order.
    keyed_one = singleton_run(make_keyed(1, 0), keyed_value_less());

    result.check(keyed_one.size() == 1);
    result.check(keyed_one.data()[0] == make_keyed(1, 0));
    result.check(mappend(keyed_one,
                         singleton_run(make_keyed(1, 1),
                                       keyed_value_less())).data()[0].tag ==
                 0);

    return result.ok();
}


// tests_sort_monoid_to_singleton_runs
bool tests_sort_monoid_to_singleton_runs()
{
    verdict                  result;
    std::vector<int>         source;
    std::vector<int_run>     runs;
    std::vector<keyed_value> keyed_source;
    std::size_t              i;

    static const int values[] = { 5, 1, 4 };

    //   to_singleton_runs lifts a sequence into a Foldable of atoms.  Every
    // run holds exactly one element, in input order -- the lift itself sorts
    // nothing.
    source = ints_of(values, 3);
    runs   = to_singleton_runs(source, less<int>());

    result.check(runs.size() == 3);

    for (i = 0; i < runs.size(); ++i)
    {
        result.check(runs[i].size() == 1);
        result.check(runs[i].data()[0] == source[i]);
    }

    //   The default-comparator overload agrees.
    runs = to_singleton_runs(source);

    result.check(runs.size() == 3);
    result.check(runs[0].data()[0] == 5);
    result.check((std::is_same<decltype(to_singleton_runs(source)),
                               std::vector<int_run> >::value));

    //   An empty source lifts to an empty sequence of runs, which mconcat
    // folds back to the identity.
    result.check(to_singleton_runs(std::vector<int>()).empty());
    result.check(mconcat(to_singleton_runs(std::vector<int>())).empty());

    //   The round trip on a larger, user-typed input: lift, fold, and the
    // result is the stable sort of the original.
    keyed_source = keyed_battery()[(8 * keyed_modulus_count) + 2].data;

    result.check(keyed_source.size() == 33);

    result.check(is_stable_result(
        mconcat(to_singleton_runs(keyed_source,
                                  keyed_value_less())).data()));

    return result.ok();
}


NS_END  // testing
NS_END  // djinterp
