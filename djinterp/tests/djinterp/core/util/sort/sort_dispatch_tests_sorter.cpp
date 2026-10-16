/*******************************************************************************
* djinterp [core]                                 sort_dispatch_tests_sorter.cpp
*
*
* path:      /tests/djinterp/core/util/sort/sort_dispatch_tests_sorter.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
// djinterp [test]
#include "sort_dispatch_tests.hpp"


NS_DJINTERP
NS_TESTING


// tests_sort_dispatch_sorter_static_surface
bool tests_sort_dispatch_sorter_static_surface()
{
    verdict result;

    //   sorter<tag> republishes its tag's compile-time surface, so a caller
    // can query the algorithm without naming the tag twice.  Every member
    // must agree with the tag it was built from.
    result.check(sorter<bubble_sort_tag>::algorithm ==
                 sort_algorithm::bubble);
    result.check(sorter<merge_sort_tag>::algorithm ==
                 sort_algorithm::merge);
    result.check(sorter<heap_sort_tag>::algorithm == sort_algorithm::heap);

    result.check(sorter<insertion_sort_tag>::is_stable);
    result.check(sorter<insertion_sort_tag>::is_in_place);
    result.check(sorter<insertion_sort_tag>::is_adaptive);

    result.check(sorter<merge_sort_tag>::is_stable);
    result.check(!sorter<merge_sort_tag>::is_in_place);
    result.check(!sorter<merge_sort_tag>::is_adaptive);

    result.check(!sorter<quick_sort_tag>::is_stable);
    result.check(sorter<quick_sort_tag>::is_in_place);

    result.check(names_equal(sorter<selection_sort_tag>::name(),
                             "selection_sort"));
    result.check(names_equal(sorter<heap_sort_tag>::name(), "heap_sort"));

    //   sorter<default_sort_tag> is the compile-time form of the default.
    // Note that sorter<> is NOT this type -- the template parameter defaults
    // to the dynamic_algorithm sentinel, so sorter<> is the run-time
    // specialization and has no static `algorithm` member at all.
    result.check(sorter<default_sort_tag>::algorithm ==
                 sort_algorithm::quick);
    result.check(sorter<quick_sort_tag>::algorithm ==
                 default_sort_tag::value);

    //   The compile-time form is stateless: that is what makes it
    // zero-overhead next to calling the algorithm directly.
    result.check(std::is_empty<sorter<quick_sort_tag> >::value);

    return result.ok();
}


// tests_sort_dispatch_sorter_static_calls
bool tests_sort_dispatch_sorter_static_calls()
{
    verdict          result;
    std::vector<int> data;
    std::vector<int> expected;

    //   The two-argument call operator, over the whole corpus, for every
    // tag.
    verify_battery_reference(&static_sorter_ints<bubble_sort_tag>,
                             less<int>(),
                             result);
    verify_battery_reference(&static_sorter_ints<selection_sort_tag>,
                             less<int>(),
                             result);
    verify_battery_reference(&static_sorter_ints<insertion_sort_tag>,
                             less<int>(),
                             result);
    verify_battery_reference(&static_sorter_ints<merge_sort_tag>,
                             less<int>(),
                             result);
    verify_battery_reference(&static_sorter_ints<quick_sort_tag>,
                             less<int>(),
                             result);
    verify_battery_reference(&static_sorter_ints<heap_sort_tag>,
                             less<int>(),
                             result);

    //   The three-argument call operator forwards an explicit comparator.
    data     = shape_random(64);
    expected = data;

    std::stable_sort(expected.begin(), expected.end(), greater<int>());

    sorter<merge_sort_tag>()(data.begin(), data.end(), greater<int>());

    result.check(data == expected);

    return result.ok();
}


// tests_sort_dispatch_sorter_dynamic_construction
bool tests_sort_dispatch_sorter_dynamic_construction()
{
    verdict result;

    sorter<dynamic_algorithm> defaulted;
    sorter<dynamic_algorithm> explicit_bubble(sort_algorithm::bubble);
    sorter<>                  shorthand;

    //   The default constructor binds to default_sort_tag -- quicksort --
    // matching the compile-time default.
    result.check(defaulted.algorithm() == default_sort_tag::value);
    result.check(defaulted.algorithm() == sort_algorithm::quick);
    result.check(names_equal(defaulted.name(), "quick_sort"));

    //   sorter<> and sorter<dynamic_algorithm> are the same type: the
    // template parameter defaults to the sentinel.
    result.check((std::is_same<sorter<>,
                               sorter<dynamic_algorithm> >::value));
    result.check(shorthand.algorithm() == defaulted.algorithm());

    //   The converting constructor is explicit, so a bare sort_algorithm
    // never silently becomes a sorter.
    result.check(explicit_bubble.algorithm() == sort_algorithm::bubble);
    result.check(names_equal(explicit_bubble.name(), "bubble_sort"));
    result.check((!std::is_convertible<
                      sort_algorithm,
                      sorter<dynamic_algorithm> >::value));

    return result.ok();
}


// tests_sort_dispatch_sorter_dynamic_rebinding
bool tests_sort_dispatch_sorter_dynamic_rebinding()
{
    verdict          result;
    std::vector<int> data;
    std::vector<int> expected;
    std::size_t      i;

    sorter<dynamic_algorithm> dispatcher;

    sort_algorithm every[6];

    every[0] = sort_algorithm::bubble;
    every[1] = sort_algorithm::selection;
    every[2] = sort_algorithm::insertion;
    every[3] = sort_algorithm::merge;
    every[4] = sort_algorithm::quick;
    every[5] = sort_algorithm::heap;

    expected = shape_random(64);
    std::stable_sort(expected.begin(), expected.end(), less<int>());

    //   One instance, rebound six times: the selector is state, not a
    // compile-time property, and every observable follows it.
    for (i = 0; i < 6; ++i)
    {
        dispatcher.set_algorithm(every[i]);

        result.check(dispatcher.algorithm() == every[i]);
        result.check(names_equal(dispatcher.name(), name_of(every[i])));

        data = shape_random(64);

        dispatcher(data.begin(), data.end());

        result.check(data == expected);
    }

    //   Rebinding to an unrecognised enumerator makes the sorter a no-op
    // rather than a fault -- it inherits dispatch_runtime's fall-through.
    dispatcher.set_algorithm(unrecognised_algorithm());

    data     = shape_random(64);
    expected = data;

    dispatcher(data.begin(), data.end());

    result.check(ranges_equal(data, expected));
    result.check(names_equal(dispatcher.name(), "unknown"));

    return result.ok();
}


// tests_sort_dispatch_sorter_dynamic_properties
bool tests_sort_dispatch_sorter_dynamic_properties()
{
    verdict     result;
    std::size_t i;

    sorter<dynamic_algorithm> dispatcher;

    sort_algorithm every[6];

    every[0] = sort_algorithm::bubble;
    every[1] = sort_algorithm::selection;
    every[2] = sort_algorithm::insertion;
    every[3] = sort_algorithm::merge;
    every[4] = sort_algorithm::quick;
    every[5] = sort_algorithm::heap;

    //   The dynamic sorter's property accessors are functions where the
    // static sorter's are constants, but they must answer identically for
    // the same algorithm.  Checked against properties_of, which is where
    // they source their answers.
    for (i = 0; i < 6; ++i)
    {
        dispatcher.set_algorithm(every[i]);

        result.check(dispatcher.is_stable() ==
                     properties_of(every[i]).is_stable);
        result.check(dispatcher.is_in_place() ==
                     properties_of(every[i]).is_in_place);
        result.check(dispatcher.is_adaptive() ==
                     properties_of(every[i]).is_adaptive);
    }

    //   Spot-checked against the compile-time surface, so a divergence
    // between the two faces of the subsystem shows up here.
    dispatcher.set_algorithm(sort_algorithm::merge);
    result.check(dispatcher.is_stable() == sorter<merge_sort_tag>::is_stable);
    result.check(dispatcher.is_in_place() ==
                 sorter<merge_sort_tag>::is_in_place);

    dispatcher.set_algorithm(sort_algorithm::heap);
    result.check(dispatcher.is_stable() == sorter<heap_sort_tag>::is_stable);
    result.check(dispatcher.is_in_place() ==
                 sorter<heap_sort_tag>::is_in_place);

    //   An unrecognised enumerator reports all-false, inherited from
    // properties_of.
    dispatcher.set_algorithm(unrecognised_algorithm());

    result.check(!dispatcher.is_stable());
    result.check(!dispatcher.is_in_place());
    result.check(!dispatcher.is_adaptive());

    return result.ok();
}


// tests_sort_dispatch_sorter_ordered_overloads
bool tests_sort_dispatch_sorter_ordered_overloads()
{
    verdict          result;
    std::vector<int> data;
    std::vector<int> expected_ascending;
    std::vector<int> expected_descending;

    sorter<dynamic_algorithm> dispatcher(sort_algorithm::merge);

    expected_ascending = shape_random(64);
    std::stable_sort(expected_ascending.begin(),
                     expected_ascending.end(),
                     less<int>());

    expected_descending = shape_random(64);
    std::stable_sort(expected_descending.begin(),
                     expected_descending.end(),
                     greater<int>());

    //   Both sorter forms carry a four-argument call operator that wraps the
    // comparator in an order_comparator.  Unlike the bubble, selection and
    // insertion *_sort_ordered free functions, these take a real sort_order
    // and so are callable as documented.
    data = shape_random(64);
    sorter<merge_sort_tag>()(data.begin(),
                             data.end(),
                             less<int>(),
                             sort_order::ascending);
    result.check(data == expected_ascending);

    data = shape_random(64);
    sorter<merge_sort_tag>()(data.begin(),
                             data.end(),
                             less<int>(),
                             sort_order::descending);
    result.check(data == expected_descending);

    data = shape_random(64);
    sorter<merge_sort_tag>()(data.begin(),
                             data.end(),
                             less<int>(),
                             sort_order::none);
    result.check(data == expected_ascending);

    data = shape_random(64);
    dispatcher(data.begin(), data.end(), less<int>(), sort_order::descending);
    result.check(data == expected_descending);

    data = shape_random(64);
    dispatcher(data.begin(), data.end(), less<int>(), sort_order::ascending);
    result.check(data == expected_ascending);

    //   Handing the ordered overload an already-descending comparator flips
    // it back, which is the composition a caller would expect.
    data = shape_random(64);
    dispatcher(data.begin(),
               data.end(),
               greater<int>(),
               sort_order::descending);
    result.check(data == expected_ascending);

    return result.ok();
}


NS_END  // testing
NS_END  // djinterp
