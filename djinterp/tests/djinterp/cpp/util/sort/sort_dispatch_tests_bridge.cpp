/*******************************************************************************
* djinterp [cpp]                                  sort_dispatch_tests_bridge.cpp
*
*
* path:      /tests/djinterp/cpp/util/sort/sort_dispatch_tests_bridge.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.11.12
*******************************************************************************/
// djinterp
// djinterp [test]
#include "sort_dispatch_tests.hpp"


NS_DJINTERP
NS_TESTING


// tests_sort_dispatch_algorithm_traits
bool tests_sort_dispatch_algorithm_traits()
{
    verdict result;

    //   sort_algorithm_traits lifts an enumerator to its tag and republishes
    // the tag's properties.  Every specialization must agree with the tag it
    // names -- the traits block is a view onto the tag, not a second copy of
    // the answer.
    result.check((std::is_same<
        sort_algorithm_traits<DSortAlgorithm::bubble>::tag,
        DBubbleSortTag>::value));
    result.check((std::is_same<
        sort_algorithm_traits<DSortAlgorithm::selection>::tag,
        DSelectionSortTag>::value));
    result.check((std::is_same<
        sort_algorithm_traits<DSortAlgorithm::insertion>::tag,
        DInsertionSortTag>::value));
    result.check((std::is_same<
        sort_algorithm_traits<DSortAlgorithm::merge>::tag,
        DMergeSortTag>::value));
    result.check((std::is_same<
        sort_algorithm_traits<DSortAlgorithm::quick>::tag,
        DQuickSortTag>::value));
    result.check((std::is_same<
        sort_algorithm_traits<DSortAlgorithm::heap>::tag,
        DHeapSortTag>::value));

    result.check(sort_algorithm_traits<DSortAlgorithm::bubble>::is_stable ==
                 DBubbleSortTag::is_stable);
    result.check(
        sort_algorithm_traits<DSortAlgorithm::selection>::is_in_place ==
        DSelectionSortTag::is_in_place);
    result.check(
        sort_algorithm_traits<DSortAlgorithm::insertion>::is_adaptive ==
        DInsertionSortTag::is_adaptive);
    result.check(sort_algorithm_traits<DSortAlgorithm::merge>::is_in_place ==
                 DMergeSortTag::is_in_place);
    result.check(sort_algorithm_traits<DSortAlgorithm::quick>::is_stable ==
                 DQuickSortTag::is_stable);
    result.check(sort_algorithm_traits<DSortAlgorithm::heap>::is_adaptive ==
                 DHeapSortTag::is_adaptive);

    return result.ok();
}


// tests_sort_dispatch_algorithm_tag_alias
bool tests_sort_dispatch_algorithm_tag_alias()
{
    verdict result;

    //   algorithm_tag_t is the enumerator-to-tag direction of the join.
    // Composed with tag::value it has to be the identity, which is the
    // property that keeps the compile-time and run-time surfaces in step.
    result.check((std::is_same<algorithm_tag_t<DSortAlgorithm::bubble>,
                               DBubbleSortTag>::value));
    result.check((std::is_same<algorithm_tag_t<DSortAlgorithm::selection>,
                               DSelectionSortTag>::value));
    result.check((std::is_same<algorithm_tag_t<DSortAlgorithm::insertion>,
                               DInsertionSortTag>::value));
    result.check((std::is_same<algorithm_tag_t<DSortAlgorithm::merge>,
                               DMergeSortTag>::value));
    result.check((std::is_same<algorithm_tag_t<DSortAlgorithm::quick>,
                               DQuickSortTag>::value));
    result.check((std::is_same<algorithm_tag_t<DSortAlgorithm::heap>,
                               DHeapSortTag>::value));

    // round trip: tag -> enumerator -> tag
    result.check(algorithm_tag_t<DBubbleSortTag::value>::value ==
                 DSortAlgorithm::bubble);
    result.check(algorithm_tag_t<DMergeSortTag::value>::value ==
                 DSortAlgorithm::merge);
    result.check(algorithm_tag_t<DHeapSortTag::value>::value ==
                 DSortAlgorithm::heap);

    return result.ok();
}


// tests_sort_dispatch_properties_of
bool tests_sort_dispatch_properties_of()
{
    verdict     result;
    sort_traits properties;

    //   properties_of is the run-time face of the same table.  Every
    // enumerator must produce exactly its tag's values.
    properties = properties_of(DSortAlgorithm::bubble);
    result.check(properties.is_stable == DBubbleSortTag::is_stable);
    result.check(properties.is_in_place == DBubbleSortTag::is_in_place);
    result.check(properties.is_adaptive == DBubbleSortTag::is_adaptive);

    properties = properties_of(DSortAlgorithm::selection);
    result.check(properties.is_stable == DSelectionSortTag::is_stable);
    result.check(properties.is_in_place == DSelectionSortTag::is_in_place);
    result.check(properties.is_adaptive == DSelectionSortTag::is_adaptive);

    properties = properties_of(DSortAlgorithm::insertion);
    result.check(properties.is_stable == DInsertionSortTag::is_stable);
    result.check(properties.is_in_place == DInsertionSortTag::is_in_place);
    result.check(properties.is_adaptive == DInsertionSortTag::is_adaptive);

    properties = properties_of(DSortAlgorithm::merge);
    result.check(properties.is_stable == DMergeSortTag::is_stable);
    result.check(properties.is_in_place == DMergeSortTag::is_in_place);
    result.check(properties.is_adaptive == DMergeSortTag::is_adaptive);

    properties = properties_of(DSortAlgorithm::quick);
    result.check(properties.is_stable == DQuickSortTag::is_stable);
    result.check(properties.is_in_place == DQuickSortTag::is_in_place);
    result.check(properties.is_adaptive == DQuickSortTag::is_adaptive);

    properties = properties_of(DSortAlgorithm::heap);
    result.check(properties.is_stable == DHeapSortTag::is_stable);
    result.check(properties.is_in_place == DHeapSortTag::is_in_place);
    result.check(properties.is_adaptive == DHeapSortTag::is_adaptive);

    return result.ok();
}


// tests_sort_dispatch_name_of
bool tests_sort_dispatch_name_of()
{
    verdict result;

    result.check(names_equal(name_of(DSortAlgorithm::bubble),
                             DBubbleSortTag::name()));
    result.check(names_equal(name_of(DSortAlgorithm::selection),
                             DSelectionSortTag::name()));
    result.check(names_equal(name_of(DSortAlgorithm::insertion),
                             DInsertionSortTag::name()));
    result.check(names_equal(name_of(DSortAlgorithm::merge),
                             DMergeSortTag::name()));
    result.check(names_equal(name_of(DSortAlgorithm::quick),
                             DQuickSortTag::name()));
    result.check(names_equal(name_of(DSortAlgorithm::heap),
                             DHeapSortTag::name()));

    result.check(names_equal(name_of(DSortAlgorithm::quick), "quick_sort"));

    return result.ok();
}


// tests_sort_dispatch_unrecognised_enumerator
bool tests_sort_dispatch_unrecognised_enumerator()
{
    verdict     result;
    sort_traits properties;

    //   None of the three switches in this header carries a default label,
    // so an enumerator outside the six falls straight through and each
    // function returns whatever it initialised its result to.  That
    // behaviour is deliberate and documented for name_of; it is checked here
    // for all three, because a caller who casts a bad integer gets it
    // whether the author intended it or not.
    //
    //   properties_of: all three flags false.
    properties = properties_of(unrecognised_algorithm());

    result.check(!properties.is_stable);
    result.check(!properties.is_in_place);
    result.check(!properties.is_adaptive);

    //   name_of: the string "unknown".
    result.check(names_equal(name_of(unrecognised_algorithm()), "unknown"));
    result.check(name_of(unrecognised_algorithm()) != nullptr);

    //   The name is not one of the six, so a caller comparing against them
    // cannot be fooled.
    result.check(!names_equal(name_of(unrecognised_algorithm()),
                              DBubbleSortTag::name()));
    result.check(!names_equal(name_of(unrecognised_algorithm()),
                              DQuickSortTag::name()));

    return result.ok();
}


NS_END  // testing
NS_END  // djinterp
