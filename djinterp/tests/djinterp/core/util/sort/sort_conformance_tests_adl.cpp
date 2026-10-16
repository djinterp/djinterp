/*******************************************************************************
* djinterp [core]                                 sort_conformance_tests_adl.cpp
*
*
* path:      /tests/djinterp/core/util/sort/sort_conformance_tests_adl.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
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

    //   DEFECT: djinterp::sort collides with std::sort under ADL.
    //
    //   sort(first, last, comp) written unqualified inside any namespace that
    // can see djinterp::sort -- which includes every `using namespace
    // djinterp;` translation unit -- is AMBIGUOUS when the iterator is a
    // standard-library one.  Ordinary lookup finds djinterp::sort; ADL on
    // __normal_iterator<int*, std::vector<int> > (whose associated namespaces
    // are std and __gnu_cxx) adds std::sort; both are exact matches.
    //
    //   The caller did nothing wrong, so the fix belongs in the library --
    // but note what does NOT work: SFINAE-constraining the entry point (e.g.
    // require is_comparator<_Comparator, value_type>) cannot disambiguate.
    // When the argument really is a comparator both candidates stay viable;
    // the constraint can only DROP djinterp::sort, which silently hands the
    // call to std::sort -- a worse outcome, since it picks the wrong
    // algorithm without a diagnostic.  The real fixes are to RENAME the free
    // entry points (e.g. sort_range) so they never share overload resolution
    // with std::sort, or to document that they must be called qualified as
    // djinterp::sort.  Until then these two checks stay red.
    result.check(
        unqualified_sort3_probe<vector_iterator, less<int> >::value);
    result.check(
        unqualified_sort2_probe<vector_iterator>::value);

    //   A raw pointer has no associated user namespace beyond the global one,
    // so std::sort is NOT pulled in and the same call is fine.  This is the
    // control: it shows the ambiguity is caused by ADL on the iterator type
    // rather than by anything intrinsic to the overload set.
    result.check(
        unqualified_sort3_probe<pointer_iterator, less<int> >::value);
    result.check(
        unqualified_sort2_probe<pointer_iterator>::value);

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

    //   The positive control for the probe above: fully qualified, the entry
    // point is unambiguous and is exactly sort<default_sort_tag>.  Identical
    // output alone would not show that -- every correct sort of ints agrees --
    // so the comparison COUNT is checked too.
    qualified             = shape_random(128);
    tagged                = qualified;
    qualified_comparisons = 0;
    tagged_comparisons    = 0;

    ::djinterp::sort(qualified.begin(),
                     qualified.end(),
                     make_counting(less<int>(), &qualified_comparisons));

    ::djinterp::sort<default_sort_tag>(
        tagged.begin(),
        tagged.end(),
        make_counting(less<int>(), &tagged_comparisons));

    result.check(is_sorted_range(qualified.begin(),
                                 qualified.end(),
                                 less<int>()));
    result.check(ranges_equal(qualified, tagged));
    result.check(qualified_comparisons == tagged_comparisons);
    result.check(qualified_comparisons > 0);

    //   And the run-time form, which takes the algorithm first and so was
    // never exposed to the collision.
    qualified = shape_random(128);

    ::djinterp::sort(sort_algorithm::heap,
                     qualified.begin(),
                     qualified.end(),
                     less<int>());

    result.check(is_sorted_range(qualified.begin(),
                                 qualified.end(),
                                 less<int>()));

    return result.ok();
}


NS_END  // testing
NS_END  // djinterp
