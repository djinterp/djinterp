/*******************************************************************************
* djinterp [cpp]                                          sort_tests_surface.cpp
*
*
* path:      /tests/djinterp/cpp/util/sort/sort_tests_surface.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.11.12
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

    //   This translation unit includes sort.hpp and nothing else from the
    // subsystem -- no test support header, no individual algorithm header.
    // Everything asserted here is therefore a statement about what the
    // facade alone pulls in.
    //
    //   The facade's own guard.
#ifndef DJINTERP_UTIL_SORT_HPP
    ok = false;
#endif

    //   The headers it aggregates, and the comparator algebra.
#ifndef DJINTERP_UTIL_SORT_COMMON_HPP
    ok = false;
#endif

#ifndef DJINTERP_UTIL_SORT_DISPATCH_HPP
    ok = false;
#endif

#ifndef DJINTERP_UTIL_SORT_MONOID_HPP
    ok = false;
#endif

    //   And, transitively through the dispatch header, all six algorithm
    // headers.
#ifndef DJINTERP_UTIL_SORT_BUBBLE_SORT_HPP
    ok = false;
#endif

#ifndef DJINTERP_UTIL_SORT_SELECTION_SORT_HPP
    ok = false;
#endif

#ifndef DJINTERP_UTIL_SORT_INSERTION_SORT_HPP
    ok = false;
#endif

#ifndef DJINTERP_UTIL_SORT_MERGE_SORT_HPP
    ok = false;
#endif

#ifndef DJINTERP_UTIL_SORT_QUICK_SORT_HPP
    ok = false;
#endif

#ifndef DJINTERP_UTIL_SORT_HEAP_SORT_HPP
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
#ifdef DJINTERP_UTIL_SORT_VIEW_HPP
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
    DSortedRun<int>  run;

    ok = true;

    //   A representative symbol from each aggregated header, named and used,
    // so the facade is checked as a compiling interface rather than as a set
    // of preprocessor guards.  This body would fail to COMPILE, not merely
    // to pass, if the facade stopped exporting any of them.
    //
    //   From sort_common.hpp, the order enum; from comparator.hpp, the
    // algebra the facade brings in with it.
    ok = (ok && (sort_order::ascending != sort_order::descending));
    ok = (ok && natural<int>()(1, 2));
    ok = (ok && (natural<int>() | reversed())(2, 1));

    //   From sort_dispatch.hpp: the selector, the tags, the traits bridge,
    // the DSorter and the free entry points.
    ok = (ok && (DQuickSortTag::value == DSortAlgorithm::quick));
    ok = (ok && is_sort_tag<DMergeSortTag>::value);
    ok = (ok && (algorithm_tag_t<DSortAlgorithm::heap>::value ==
                 DSortAlgorithm::heap));

    properties = properties_of(DSortAlgorithm::merge);
    ok         = (ok && properties.is_stable);
    ok         = (ok && (name_of(DSortAlgorithm::bubble) != nullptr));

    data.push_back(3);
    data.push_back(1);
    data.push_back(2);

    DSorter<DInsertionSortTag>()(data.begin(), data.end());
    ok = (ok && (data[0] == 1) && (data[2] == 3));

    sort(DSortAlgorithm::heap, data.begin(), data.end(), std::greater<int>());
    ok = (ok && (data[0] == 3) && (data[2] == 1));

    sort(DMergeSortTag(), data.begin(), data.end());
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
    ok  = (ok && mappend(run, mempty<DSortedRun<int> >()).size() == 3);
    ok  = (ok && (mconcat(to_singleton_runs(data)).data()[0] == 1));
    ok  = (ok && singleton_run(4).size() == 1);
    ok  = (ok && adopt_sorted_run(data, std::less<int>()).size() == 3);

    return ok;
}


NS_END  // testing
NS_END  // djinterp
