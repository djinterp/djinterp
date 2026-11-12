/*******************************************************************************
* djinterp [cpp]                                      sort_tests_integration.cpp
*
*
* path:      /tests/djinterp/cpp/util/sort/sort_tests_integration.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.11.12
*******************************************************************************/
// djinterp
// djinterp [test]
#include "sort_tests.hpp"


NS_DJINTERP
NS_TESTING


// every_form_agrees
//   helper: sorts _source through all eighteen spellings the facade offers --
// the six run-time selectors, the six tags and the six named entry points --
// and reports whether every result equals _reference.
static
bool every_form_agrees(
    const std::vector<int>& _source,
    const std::vector<int>& _reference
)
{
    static const DSortAlgorithm every[6] =
    {
        DSortAlgorithm::bubble,
        DSortAlgorithm::selection,
        DSortAlgorithm::insertion,
        DSortAlgorithm::merge,
        DSortAlgorithm::quick,
        DSortAlgorithm::heap
    };

    bool             ok;
    std::vector<int> c;
    std::size_t      j;

    ok = true;

    for (j = 0; j < 6; ++j)
    {
        c = _source;
        sort(every[j], c.begin(), c.end());
        ok = (ok && (c == _reference));
    }

    //   The compile-time family, over the same input.
    c = _source;
    sort(DBubbleSortTag(), c.begin(), c.end());
    ok = (ok && (c == _reference));

    c = _source;
    sort(DSelectionSortTag(), c.begin(), c.end());
    ok = (ok && (c == _reference));

    c = _source;
    sort(DInsertionSortTag(), c.begin(), c.end());
    ok = (ok && (c == _reference));

    c = _source;
    sort(DMergeSortTag(), c.begin(), c.end());
    ok = (ok && (c == _reference));

    c = _source;
    sort(DQuickSortTag(), c.begin(), c.end());
    ok = (ok && (c == _reference));

    c = _source;
    sort(DHeapSortTag(), c.begin(), c.end());
    ok = (ok && (c == _reference));

    //   And the six named entry points.
    c = _source;
    bubble_sort(c.begin(), c.end());
    ok = (ok && (c == _reference));

    c = _source;
    selection_sort(c.begin(), c.end());
    ok = (ok && (c == _reference));

    c = _source;
    insertion_sort(c.begin(), c.end());
    ok = (ok && (c == _reference));

    c = _source;
    merge_sort(c.begin(), c.end());
    ok = (ok && (c == _reference));

    c = _source;
    quick_sort(c.begin(), c.end());
    ok = (ok && (c == _reference));

    c = _source;
    heap_sort(c.begin(), c.end());
    ok = (ok && (c == _reference));

    return ok;
}


// tests_sort_facade_every_algorithm_agrees
bool tests_sort_facade_every_algorithm_agrees()
{
    bool             ok;
    std::vector<int> source;
    std::vector<int> reference;
    std::size_t      i;
    std::size_t      length;
    unsigned int     state;

    ok    = true;
    state = 987654321u;

    //   The subsystem's central promise: the choice of algorithm is a
    // performance decision and never a semantic one.  Six algorithms, sizes
    // spanning quicksort's insertion threshold and several of merge sort's
    // pass counts, all reached through the facade -- and every form must
    // agree, element for element, with std::sort.
    //
    //   This test carries its own generator rather than the shared corpus,
    // because the facade suite deliberately includes only sort.hpp: it is
    // the one place where a stray dependency on the test support header
    // would go unnoticed.
    for (length = 0; length < 40; ++length)
    {
        source.clear();
        source.reserve(length);

        for (i = 0; i < length; ++i)
        {
            state = ((state * 1103515245u) + 12345u);
            source.push_back(static_cast<int>((state >> 16) % 17));
        }

        reference = source;
        std::sort(reference.begin(), reference.end());

        ok = (ok && every_form_agrees(source, reference));
    }

    return ok;
}


// tests_sort_facade_dispatch_and_monoid_agree
bool tests_sort_facade_dispatch_and_monoid_agree()
{
    bool             ok;
    std::vector<int> source;
    std::vector<int> reference;
    std::size_t      i;
    std::size_t      length;
    unsigned int     state;

    ok    = true;
    state = 24680u;

    //   The facade's two halves must produce the same answer.  Folding
    // singleton runs through the monoid is a bottom-up merge sort; running
    // the dispatch layer is a top-down one; and make_sorted_run goes through
    // ::djinterp::merge_sort directly.  All three, plus std::stable_sort,
    // have to agree.
    for (length = 0; length < 40; ++length)
    {
        source.clear();
        source.reserve(length);

        for (i = 0; i < length; ++i)
        {
            state = ((state * 1103515245u) + 12345u);
            source.push_back(static_cast<int>((state >> 16) % 11));
        }

        reference = source;
        std::stable_sort(reference.begin(), reference.end());

        ok = (ok && (mconcat(to_singleton_runs(source)).data() ==
                     reference));
        ok = (ok && (make_sorted_run(source).data() == reference));
        ok = (ok && (make_sorted_run(source, std::less<int>()).data() ==
                     reference));

        //   Two runs built from the halves of the input, merged through the
        // algebra, must equal the sort of the whole.
        ok = (ok && (mappend(
                         make_sorted_run(std::vector<int>(
                             source.begin(),
                             source.begin() +
                                 static_cast<std::ptrdiff_t>(length / 2))),
                         make_sorted_run(std::vector<int>(
                             source.begin() +
                                 static_cast<std::ptrdiff_t>(length / 2),
                             source.end()))).data() == reference));
    }

    return ok;
}


// tests_sort_facade_comparator_algebra_end_to_end
bool tests_sort_facade_comparator_algebra_end_to_end()
{
    bool             ok;
    std::vector<int> data;
    std::vector<int> ascending;
    std::vector<int> descending;

    ok = true;

    //   sort_common.hpp pulls in the functional comparator algebra, so a
    // composed comparator must reach every layer the facade exports --
    // the algorithms, the dispatch surface and the monoid -- with no glue.
    data.push_back(5);
    data.push_back(1);
    data.push_back(4);
    data.push_back(1);
    data.push_back(9);

    ascending = data;
    std::stable_sort(ascending.begin(), ascending.end());

    descending = ascending;
    std::reverse(descending.begin(), descending.end());

    ok = (ok && is_comparator<std::less<int>, int>::value);

    //   Through an algorithm.
    data = ascending;
    heap_sort(data.begin(),
              data.end(),
              ::djinterp::natural<int>() | ::djinterp::reversed());
    ok = (ok && (data == descending));

    //   Through the compile-time dispatch surface.
    data = descending;
    sort(DMergeSortTag(),
         data.begin(),
         data.end(),
         ::djinterp::natural<int>());
    ok = (ok && (data == ascending));

    //   Through the run-time dispatch surface.
    data = ascending;
    sort(DSortAlgorithm::selection,
         data.begin(),
         data.end(),
         ::djinterp::natural<int>() | ::djinterp::reversed());
    ok = (ok && (data == descending));

    //   Through a DSorter's ordered overload, where the composed comparator
    // is itself wrapped by an DOrderComparator.
    data = descending;
    DSorter<DBubbleSortTag>()(
        data.begin(),
        data.end(),
        ::djinterp::natural<int>() | ::djinterp::reversed(),
        sort_order::descending);
    ok = (ok && (data == ascending));

    //   And through the monoid, whose runs are parameterised on the
    // comparator type.
    ok = (ok && (make_sorted_run(
                     data,
                     ::djinterp::natural<int>() |
                         ::djinterp::reversed()).data() == descending));

    return ok;
}


NS_END  // testing
NS_END  // djinterp
