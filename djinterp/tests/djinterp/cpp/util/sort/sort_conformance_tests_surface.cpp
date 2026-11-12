/*******************************************************************************
* djinterp [cpp]                              sort_conformance_tests_surface.cpp
*
*
* path:      /tests/djinterp/cpp/util/sort/sort_conformance_tests_surface.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.11.12
*******************************************************************************/
// djinterp
// djinterp [test]
#include "sort_conformance_tests.hpp"


NS_DJINTERP
NS_TESTING


// tests_sort_conformance_ordered_surface_is_uniform
bool tests_sort_conformance_ordered_surface_is_uniform()
{
    verdict result;

    typedef std::vector<int>::iterator iterator_type;
    typedef std::less<int>             compare_type;

    //   A caller swapping one algorithm for another depends on the six
    // *_sort_ordered entry points having the SAME shape: (It, It, Cmp,
    // sort_order::value).  They did not -- bubble, selection and insertion
    // once took a bool, which no sort_order converts to -- and the per-module
    // *_entry sections each report their own algorithm's half of this.  The
    // claim HERE is the cross-cutting one: the surface is uniform.
    result.check(conf_bubble_ordered_probe<iterator_type,
                                           compare_type>::value);
    result.check(conf_selection_ordered_probe<iterator_type,
                                              compare_type>::value);
    result.check(conf_insertion_ordered_probe<iterator_type,
                                              compare_type>::value);
    result.check(conf_merge_ordered_probe<iterator_type,
                                          compare_type>::value);
    result.check(conf_quick_ordered_probe<iterator_type,
                                          compare_type>::value);
    result.check(conf_heap_ordered_probe<iterator_type,
                                         compare_type>::value);

    return result.ok();
}


// ordered_entry
//   type: a pointer to one of the six *_sort_ordered entry points,
// instantiated for vector<int> under std::less<int>.
typedef void (*ordered_entry)(std::vector<int>::iterator,
                              std::vector<int>::iterator,
                              std::less<int>,
                              sort_order::value);

// plain_entry
//   type: a pointer to one of the six three-argument entry points,
// instantiated for vector<int> under std::greater<int>.
typedef void (*plain_entry)(std::vector<int>::iterator,
                            std::vector<int>::iterator,
                            std::greater<int>);


// tests_sort_conformance_ordered_means_the_same
bool tests_sort_conformance_ordered_means_the_same()
{
    verdict          result;
    std::vector<int> ordered;
    std::vector<int> reversed;
    std::vector<int> unspecified;
    std::vector<int> ascending;
    std::size_t      algorithm;
    std::size_t      size_index;
    std::size_t      shape_index;

    const ordered_entry ordered_entries[6] =
    {
        &bubble_sort_ordered<std::vector<int>::iterator, std::less<int> >,
        &selection_sort_ordered<std::vector<int>::iterator, std::less<int> >,
        &insertion_sort_ordered<std::vector<int>::iterator, std::less<int> >,
        &merge_sort_ordered<std::vector<int>::iterator, std::less<int> >,
        &quick_sort_ordered<std::vector<int>::iterator, std::less<int> >,
        &heap_sort_ordered<std::vector<int>::iterator, std::less<int> >
    };

    const plain_entry plain_entries[6] =
    {
        &bubble_sort<std::vector<int>::iterator, std::greater<int> >,
        &selection_sort<std::vector<int>::iterator, std::greater<int> >,
        &insertion_sort<std::vector<int>::iterator, std::greater<int> >,
        &merge_sort<std::vector<int>::iterator, std::greater<int> >,
        &quick_sort<std::vector<int>::iterator, std::greater<int> >,
        &heap_sort<std::vector<int>::iterator, std::greater<int> >
    };

    //   Every banner says the same thing about _ordered: the direction is
    // folded into the comparator, the C++ counterpart of the C module's
    // _order parameter.  So for every algorithm, descending under a
    // comparator must give what the three-argument form gives under the
    // reversed comparator, and none must give what ascending gives -- on
    // every input of the battery.
    for (algorithm = 0; algorithm < 6; ++algorithm)
    {
        for (size_index = 0; size_index < battery_size_count; ++size_index)
        {
            for (shape_index = 0;
                 shape_index < battery_shape_count;
                 ++shape_index)
            {
                ordered = make_shape(shape_index,
                                     battery_size_at(size_index));
                reversed    = ordered;
                unspecified = ordered;
                ascending   = ordered;

                ordered_entries[algorithm](ordered.begin(),
                                           ordered.end(),
                                           std::less<int>(),
                                           sort_order::descending);
                plain_entries[algorithm](reversed.begin(),
                                         reversed.end(),
                                         std::greater<int>());

                result.check(ranges_equal(ordered, reversed));

                ordered_entries[algorithm](unspecified.begin(),
                                           unspecified.end(),
                                           std::less<int>(),
                                           sort_order::none);
                ordered_entries[algorithm](ascending.begin(),
                                           ascending.end(),
                                           std::less<int>(),
                                           sort_order::ascending);

                result.check(ranges_equal(unspecified, ascending));
            }
        }
    }

    return result.ok();
}


NS_END  // testing
NS_END  // djinterp
