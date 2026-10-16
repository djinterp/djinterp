/*******************************************************************************
* djinterp [core]                                  sort_dispatch_tests_entry.cpp
*
*
* path:      /tests/djinterp/core/util/sort/sort_dispatch_tests_entry.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
// djinterp [test]
#include "sort_dispatch_tests.hpp"


NS_DJINTERP
NS_TESTING


// tests_sort_dispatch_entry_compile_time_forms
bool tests_sort_dispatch_entry_compile_time_forms()
{
    verdict          result;
    std::vector<int> data;
    std::vector<int> expected_ascending;
    std::vector<int> expected_descending;

    expected_ascending = shape_random(64);
    std::stable_sort(expected_ascending.begin(),
                     expected_ascending.end(),
                     less<int>());

    expected_descending = shape_random(64);
    std::stable_sort(expected_descending.begin(),
                     expected_descending.end(),
                     greater<int>());

    //   Form A1: sort<tag>(first, last, comp).
    data = shape_random(64);
    sort<merge_sort_tag>(data.begin(), data.end(), greater<int>());
    result.check(data == expected_descending);

    //   Form A2: sort<tag>(first, last) -- default comparator.
    data = shape_random(64);
    sort<heap_sort_tag>(data.begin(), data.end());
    result.check(data == expected_ascending);

    //   Form A3: sort<tag>(first, last, comp, order).
    data = shape_random(64);
    sort<insertion_sort_tag>(data.begin(),
                             data.end(),
                             less<int>(),
                             sort_order::descending);
    result.check(data == expected_descending);

    data = shape_random(64);
    sort<insertion_sort_tag>(data.begin(),
                             data.end(),
                             less<int>(),
                             sort_order::ascending);
    result.check(data == expected_ascending);

    data = shape_random(64);
    sort<insertion_sort_tag>(data.begin(),
                             data.end(),
                             less<int>(),
                             sort_order::none);
    result.check(data == expected_ascending);

    //   All six tags reach an algorithm through the free compile-time form.
    data = shape_random(64);
    sort<bubble_sort_tag>(data.begin(), data.end());
    result.check(data == expected_ascending);

    data = shape_random(64);
    sort<selection_sort_tag>(data.begin(), data.end());
    result.check(data == expected_ascending);

    data = shape_random(64);
    sort<insertion_sort_tag>(data.begin(), data.end());
    result.check(data == expected_ascending);

    data = shape_random(64);
    sort<merge_sort_tag>(data.begin(), data.end());
    result.check(data == expected_ascending);

    data = shape_random(64);
    sort<quick_sort_tag>(data.begin(), data.end());
    result.check(data == expected_ascending);

    return result.ok();
}


// tests_sort_dispatch_entry_runtime_forms
bool tests_sort_dispatch_entry_runtime_forms()
{
    verdict          result;
    std::vector<int> data;
    std::vector<int> expected_ascending;
    std::vector<int> expected_descending;
    std::size_t      comparisons;
    std::size_t      i;

    sort_algorithm every[6];

    every[0] = sort_algorithm::bubble;
    every[1] = sort_algorithm::selection;
    every[2] = sort_algorithm::insertion;
    every[3] = sort_algorithm::merge;
    every[4] = sort_algorithm::quick;
    every[5] = sort_algorithm::heap;

    expected_ascending = shape_random(64);
    std::stable_sort(expected_ascending.begin(),
                     expected_ascending.end(),
                     less<int>());

    expected_descending = shape_random(64);
    std::stable_sort(expected_descending.begin(),
                     expected_descending.end(),
                     greater<int>());

    //   Form B2: sort(algorithm, first, last).  The leading sort_algorithm
    // is what tells the overload set to take the run-time family; because
    // the enumeration is scoped there is no conversion by which this could
    // be mistaken for the compile-time sort(first, last, comp).
    for (i = 0; i < 6; ++i)
    {
        data = shape_random(64);
        sort(every[i], data.begin(), data.end());
        result.check(data == expected_ascending);
    }

    //   Form B1: sort(algorithm, first, last, comp).
    for (i = 0; i < 6; ++i)
    {
        data = shape_random(64);
        sort(every[i], data.begin(), data.end(), greater<int>());
        result.check(data == expected_descending);
    }

    //   Form B3: sort(algorithm, first, last, comp, order).
    for (i = 0; i < 6; ++i)
    {
        data = shape_random(64);
        sort(every[i],
             data.begin(),
             data.end(),
             less<int>(),
             sort_order::descending);
        result.check(data == expected_descending);

        data = shape_random(64);
        sort(every[i],
             data.begin(),
             data.end(),
             less<int>(),
             sort_order::ascending);
        result.check(data == expected_ascending);
    }

    //   An unrecognised enumerator is a no-op here too, through all three
    // run-time forms.
    data        = shape_random(64);
    comparisons = 0;

    sort(unrecognised_algorithm(),
         data.begin(),
         data.end(),
         make_counting(less<int>(), &comparisons));

    result.check(ranges_equal(data, shape_random(64)));
    result.check(comparisons == 0);

    data = shape_random(64);
    sort(unrecognised_algorithm(), data.begin(), data.end());
    result.check(ranges_equal(data, shape_random(64)));

    data = shape_random(64);
    sort(unrecognised_algorithm(),
         data.begin(),
         data.end(),
         less<int>(),
         sort_order::descending);
    result.check(ranges_equal(data, shape_random(64)));

    return result.ok();
}


// tests_sort_dispatch_entry_default_tag
bool tests_sort_dispatch_entry_default_tag()
{
    verdict          result;
    std::vector<int> defaulted;
    std::vector<int> named;
    std::size_t      defaulted_comparisons;
    std::size_t      named_comparisons;
    std::size_t      foreign_comparisons;

    //   sort(first, last, comp) with no explicit tag must be exactly
    // sort<default_sort_tag>(first, last, comp).  Identical output is not
    // enough to show that -- every correct sort of ints produces identical
    // output -- so the comparison COUNT is checked too.  Two different
    // algorithms would essentially never agree on it.
    defaulted             = shape_random(256);
    named                 = defaulted;
    defaulted_comparisons = 0;
    named_comparisons     = 0;
    foreign_comparisons   = 0;

    //   Qualified deliberately.  An unqualified sort(first, last, comp) on a
    // standard-library iterator is AMBIGUOUS: ordinary lookup finds
    // djinterp::sort, and ADL on __normal_iterator<int*, std::vector<int> >
    // adds std::sort, which matches equally well.  The default-tag entry
    // point is still what is under test here -- the qualification only says
    // WHICH sort, it does not supply a tag.
    ::djinterp::sort(defaulted.begin(),
                     defaulted.end(),
                     make_counting(less<int>(), &defaulted_comparisons));

    sort<default_sort_tag>(
        named.begin(),
        named.end(),
        make_counting(less<int>(), &named_comparisons));

    result.check(ranges_equal(defaulted, named));
    result.check(defaulted_comparisons == named_comparisons);
    result.check(defaulted_comparisons > 0);

    //   And the count really does discriminate: a different algorithm on the
    // same input disagrees.
    named = shape_random(256);

    sort<bubble_sort_tag>(
        named.begin(),
        named.end(),
        make_counting(less<int>(), &foreign_comparisons));

    result.check(foreign_comparisons != defaulted_comparisons);

    //   The two-argument defaulted form, and the ordered defaulted form.
    defaulted = shape_random(64);
    named     = shape_random(64);

    ::djinterp::sort(defaulted.begin(), defaulted.end());
    sort<default_sort_tag>(named.begin(), named.end());

    result.check(ranges_equal(defaulted, named));

    defaulted = shape_random(64);
    named     = shape_random(64);

    ::djinterp::sort(defaulted.begin(),
                     defaulted.end(),
                     less<int>(),
                     sort_order::descending);
    sort<default_sort_tag>(named.begin(),
                           named.end(),
                           less<int>(),
                           sort_order::descending);

    result.check(ranges_equal(defaulted, named));
    result.check(is_sorted_range(defaulted.begin(),
                                 defaulted.end(),
                                 greater<int>()));

    return result.ok();
}


// tests_sort_dispatch_entry_battery
bool tests_sort_dispatch_entry_battery()
{
    verdict result;

    //   The compile-time free form, every tag, over the whole corpus.
    verify_battery_reference(&free_sort_ints<bubble_sort_tag>,
                             less<int>(),
                             result);
    verify_battery_reference(&free_sort_ints<selection_sort_tag>,
                             less<int>(),
                             result);
    verify_battery_reference(&free_sort_ints<insertion_sort_tag>,
                             less<int>(),
                             result);
    verify_battery_reference(&free_sort_ints<merge_sort_tag>,
                             less<int>(),
                             result);
    verify_battery_reference(&free_sort_ints<quick_sort_tag>,
                             less<int>(),
                             result);
    verify_battery_reference(&free_sort_ints<heap_sort_tag>,
                             less<int>(),
                             result);

    //   The run-time free form and the dynamic sorter, likewise.  These go
    // through the functor adapters, because the selector is a value rather
    // than a template argument.
    verify_battery_reference(
        free_runtime_sort_adapter(sort_algorithm::bubble),
        less<int>(),
        result);
    verify_battery_reference(
        free_runtime_sort_adapter(sort_algorithm::merge),
        less<int>(),
        result);
    verify_battery_reference(
        free_runtime_sort_adapter(sort_algorithm::heap),
        less<int>(),
        result);

    verify_battery_reference(
        dynamic_sorter_adapter(sort_algorithm::selection),
        less<int>(),
        result);
    verify_battery_reference(
        dynamic_sorter_adapter(sort_algorithm::insertion),
        less<int>(),
        result);
    verify_battery_reference(
        dynamic_sorter_adapter(sort_algorithm::quick),
        less<int>(),
        result);

    return result.ok();
}


// tests_sort_dispatch_entry_pointer_iterators
bool tests_sort_dispatch_entry_pointer_iterators()
{
    verdict result;

    //   The whole dispatch layer is iterator-generic, so it has to work over
    // raw pointers as well as over vector iterators.  This is the check that
    // would catch a stray dependence on a class-type iterator anywhere in
    // the chain from sort() down to the algorithm.
    verify_battery_raw(&free_sort_ints_raw<bubble_sort_tag>,
                       less<int>(),
                       result);
    verify_battery_raw(&free_sort_ints_raw<selection_sort_tag>,
                       less<int>(),
                       result);
    verify_battery_raw(&free_sort_ints_raw<insertion_sort_tag>,
                       less<int>(),
                       result);
    verify_battery_raw(&free_sort_ints_raw<merge_sort_tag>,
                       less<int>(),
                       result);
    verify_battery_raw(&free_sort_ints_raw<quick_sort_tag>,
                       less<int>(),
                       result);
    verify_battery_raw(&free_sort_ints_raw<heap_sort_tag>,
                       less<int>(),
                       result);

    verify_battery_raw(free_runtime_sort_adapter(sort_algorithm::quick),
                       less<int>(),
                       result);
    verify_battery_raw(dynamic_sorter_adapter(sort_algorithm::merge),
                       less<int>(),
                       result);

    return result.ok();
}


NS_END  // testing
NS_END  // djinterp
