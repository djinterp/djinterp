/*******************************************************************************
* djinterp [cpp]                                  sort_conformance_tests_adl.cpp
*
*
* path:      /tests/djinterp/cpp/util/sort/sort_conformance_tests_adl.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.11.12
*******************************************************************************/
// djinterp
// djinterp [test]
#include "sort_conformance_tests.hpp"


NS_DJINTERP
NS_TESTING


// tests_sort_conformance_unqualified_sort_unambiguous
bool tests_sort_conformance_unqualified_sort_unambiguous()
{
    verdict result;

    typedef std::vector<int>::iterator vector_iterator;
    typedef int*                       pointer_iterator;

    //   sort(first, last[, comp]) written unqualified inside any namespace
    // that can see djinterp::sort -- which includes every `using namespace
    // djinterp;` translation unit -- must resolve when the iterator is a
    // standard-library one.  Ordinary lookup finds djinterp::sort; argument-
    // dependent lookup on __normal_iterator<int*, std::vector<int> > would add
    // std::sort, and the two would match equally well.
    //
    //   It does not, because djinterp::sort is an object: lookup that finds
    // an object performs no argument-dependent lookup.  Note what would NOT
    // have worked: SFINAE-constraining a function template.  While the
    // argument really is a comparator both candidates stay viable, and a
    // constraint can only DROP djinterp::sort, handing the call to std::sort
    // without a diagnostic.
    result.check(
        DUnqualifiedSort3Probe<vector_iterator, std::less<int> >::value);
    result.check(
        DUnqualifiedSort2Probe<vector_iterator>::value);

    //   A raw pointer has no associated namespace beyond the global one, so
    // std::sort would not be pulled in even for a function template.  This
    // is the control: the call resolves either way.
    result.check(
        DUnqualifiedSort3Probe<pointer_iterator, std::less<int> >::value);
    result.check(
        DUnqualifiedSort2Probe<pointer_iterator>::value);

    return result.ok();
}


// tests_sort_conformance_qualified_sort_dispatches
bool tests_sort_conformance_qualified_sort_dispatches()
{
    verdict          result;
    std::vector<int> qualified;
    std::vector<int> tagged;
    std::size_t      qualified_comparisons;
    std::size_t      tagged_comparisons;

    //   The positive control for the probe above: fully qualified, the call
    // is exactly sort(DDefaultSortTag(), ...).  Identical output alone would
    // not show that -- every correct sort of ints agrees -- so the comparison
    // COUNT is checked too.
    qualified             = shape_random(128);
    tagged                = qualified;
    qualified_comparisons = 0;
    tagged_comparisons    = 0;

    ::djinterp::sort(qualified.begin(),
                     qualified.end(),
                     make_counting(std::less<int>(), &qualified_comparisons));

    ::djinterp::sort(DDefaultSortTag(),
                     tagged.begin(),
                     tagged.end(),
                     make_counting(std::less<int>(), &tagged_comparisons));

    result.check(is_sorted_range(qualified.begin(),
                                 qualified.end(),
                                 std::less<int>()));
    result.check(ranges_equal(qualified, tagged));
    result.check(qualified_comparisons == tagged_comparisons);
    result.check(qualified_comparisons > 0);

    //   And the run-time form, which takes the algorithm first, as the tag
    // form does.
    qualified = shape_random(128);

    ::djinterp::sort(DSortAlgorithm::heap,
                     qualified.begin(),
                     qualified.end(),
                     std::less<int>());

    result.check(is_sorted_range(qualified.begin(),
                                 qualified.end(),
                                 std::less<int>()));

    return result.ok();
}


NS_END  // testing
NS_END  // djinterp
