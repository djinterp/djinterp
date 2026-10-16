/*******************************************************************************
* djinterp [core]                                         sort_tests_surface.cpp
*
*
* path:      /tests/djinterp/core/util/sort/sort_tests_surface.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
// djinterp [test]
#include "sort_tests.hpp"


NS_DJINTERP
NS_TESTING


// tests_sort_facade_include_guards
bool tests_sort_facade_include_guards()
{
    bool ok;

    ok = true;

    //   This translation unit includes <djinterp/core/util/sort/sort.hpp>
    // and nothing else from the subsystem -- no test support header, no
    // individual algorithm header.  Everything asserted here is therefore a
    // statement about what the facade alone pulls in.
    //
    //   The facade's own guard.
#ifndef DJINTERP_UTILITY_SORT_FACADE_
    ok = false;
#endif

    //   The three headers it aggregates.
#ifndef DJINTERP_UTILITY_SORT_
    ok = false;
#endif

#ifndef DJINTERP_UTILITY_SORT_DISPATCH_
    ok = false;
#endif

#ifndef DJINTERP_UTILITY_SORT_MONOID_
    ok = false;
#endif

    //   And, transitively through the dispatch header, all six algorithm
    // headers.
#ifndef DJINTERP_UTILITY_SORT_BUBBLE_
    ok = false;
#endif

#ifndef DJINTERP_UTILITY_SORT_SELECTION_
    ok = false;
#endif

#ifndef DJINTERP_UTILITY_SORT_INSERTION_
    ok = false;
#endif

#ifndef DJINTERP_UTILITY_SORT_MERGE_
    ok = false;
#endif

#ifndef DJINTERP_UTILITY_SORT_QUICK_
    ok = false;
#endif

#ifndef DJINTERP_UTILITY_SORT_HEAP_
    ok = false;
#endif

    return ok;
}


// tests_sort_facade_view_is_not_included
bool tests_sort_facade_view_is_not_included()
{
    bool ok;

    ok = true;

    //   sort_view.hpp is DELIBERATELY absent from the facade: the bridge to
    // the view subsystem is opt-in, so that including <sort.hpp> does not
    // drag the whole functional view layer into every translation unit that
    // wants a sort.
    //
    //   The guard being undefined is the only way to observe that from
    // inside the language, and it is worth observing, because adding the
    // include would be a silent, one-line compile-time regression for every
    // caller of the facade.
#ifdef DJINTERP_UTILITY_SORT_VIEW_
    ok = false;
#endif

    return ok;
}


// tests_sort_facade_core_symbols_visible
bool tests_sort_facade_core_symbols_visible()
{
    bool             ok;
    std::vector<int> data;
    sort_traits      properties;
    sorted_run<int>  run;

    ok = true;

    //   A representative symbol from each aggregated header, named and used,
    // so the facade is checked as a compiling interface rather than as a set
    // of preprocessor guards.  This body would fail to COMPILE, not merely
    // to pass, if the facade stopped exporting any of them.
    //
    //   From sort_common.hpp: the order enum, the comparator primitives and
    // the default-comparator alias.
    ok = (ok && (sort_order::ascending != sort_order::descending));
    ok = (ok && less<int>()(1, 2));
    ok = (ok && greater<int>()(2, 1));
    ok = (ok && default_comparator_t<int>()(1, 2));

    //   From sort_dispatch.hpp: the selector, the tags, the traits bridge,
    // the sorter and the free entry points.
    ok = (ok && (quick_sort_tag::value == sort_algorithm::quick));
    ok = (ok && is_sort_tag<merge_sort_tag>::value);
    ok = (ok && (algorithm_tag_t<sort_algorithm::heap>::value ==
                 sort_algorithm::heap));

    properties = properties_of(sort_algorithm::merge);
    ok         = (ok && properties.is_stable);
    ok         = (ok && (name_of(sort_algorithm::bubble) != nullptr));

    data.push_back(3);
    data.push_back(1);
    data.push_back(2);

    sorter<insertion_sort_tag>()(data.begin(), data.end());
    ok = (ok && (data[0] == 1) && (data[2] == 3));

    sort(sort_algorithm::heap, data.begin(), data.end(), greater<int>());
    ok = (ok && (data[0] == 3) && (data[2] == 1));

    sort<merge_sort_tag>(data.begin(), data.end());
    ok = (ok && (data[0] == 1));

    //   The six algorithm entry points, reachable through the facade
    // directly rather than only through a tag.
    bubble_sort(data.begin(), data.end());
    selection_sort(data.begin(), data.end());
    insertion_sort(data.begin(), data.end());
    merge_sort(data.begin(), data.end());
    quick_sort(data.begin(), data.end());
    heap_sort(data.begin(), data.end());
    ok = (ok && (data[0] == 1) && (data[2] == 3));

    //   From sort_monoid.hpp: the run type, the algebra and the factories.
    run = make_sorted_run(data);
    ok  = (ok && (run.size() == 3));
    ok  = (ok && mappend(run, mempty<sorted_run<int> >()).size() == 3);
    ok  = (ok && (mconcat(to_singleton_runs(data)).data()[0] == 1));
    ok  = (ok && singleton_run(4).size() == 1);
    ok  = (ok && adopt_sorted_run(data, less<int>()).size() == 3);

    return ok;
}


NS_END  // testing
NS_END  // djinterp
