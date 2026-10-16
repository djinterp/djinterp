/*******************************************************************************
* djinterp [core]                                 sort_dispatch_tests_bridge.cpp
*
*
* path:      /tests/djinterp/core/util/sort/sort_dispatch_tests_bridge.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
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
        sort_algorithm_traits<sort_algorithm::bubble>::tag,
        bubble_sort_tag>::value));
    result.check((std::is_same<
        sort_algorithm_traits<sort_algorithm::selection>::tag,
        selection_sort_tag>::value));
    result.check((std::is_same<
        sort_algorithm_traits<sort_algorithm::insertion>::tag,
        insertion_sort_tag>::value));
    result.check((std::is_same<
        sort_algorithm_traits<sort_algorithm::merge>::tag,
        merge_sort_tag>::value));
    result.check((std::is_same<
        sort_algorithm_traits<sort_algorithm::quick>::tag,
        quick_sort_tag>::value));
    result.check((std::is_same<
        sort_algorithm_traits<sort_algorithm::heap>::tag,
        heap_sort_tag>::value));

    result.check(sort_algorithm_traits<sort_algorithm::bubble>::is_stable ==
                 bubble_sort_tag::is_stable);
    result.check(
        sort_algorithm_traits<sort_algorithm::selection>::is_in_place ==
        selection_sort_tag::is_in_place);
    result.check(
        sort_algorithm_traits<sort_algorithm::insertion>::is_adaptive ==
        insertion_sort_tag::is_adaptive);
    result.check(sort_algorithm_traits<sort_algorithm::merge>::is_in_place ==
                 merge_sort_tag::is_in_place);
    result.check(sort_algorithm_traits<sort_algorithm::quick>::is_stable ==
                 quick_sort_tag::is_stable);
    result.check(sort_algorithm_traits<sort_algorithm::heap>::is_adaptive ==
                 heap_sort_tag::is_adaptive);

    return result.ok();
}


// tests_sort_dispatch_algorithm_tag_alias
bool tests_sort_dispatch_algorithm_tag_alias()
{
    verdict result;

    //   algorithm_tag_t is the enumerator-to-tag direction of the join.
    // Composed with tag::value it has to be the identity, which is the
    // property that keeps the compile-time and run-time surfaces in step.
    result.check((std::is_same<algorithm_tag_t<sort_algorithm::bubble>,
                               bubble_sort_tag>::value));
    result.check((std::is_same<algorithm_tag_t<sort_algorithm::selection>,
                               selection_sort_tag>::value));
    result.check((std::is_same<algorithm_tag_t<sort_algorithm::insertion>,
                               insertion_sort_tag>::value));
    result.check((std::is_same<algorithm_tag_t<sort_algorithm::merge>,
                               merge_sort_tag>::value));
    result.check((std::is_same<algorithm_tag_t<sort_algorithm::quick>,
                               quick_sort_tag>::value));
    result.check((std::is_same<algorithm_tag_t<sort_algorithm::heap>,
                               heap_sort_tag>::value));

    // round trip: tag -> enumerator -> tag
    result.check(algorithm_tag_t<bubble_sort_tag::value>::value ==
                 sort_algorithm::bubble);
    result.check(algorithm_tag_t<merge_sort_tag::value>::value ==
                 sort_algorithm::merge);
    result.check(algorithm_tag_t<heap_sort_tag::value>::value ==
                 sort_algorithm::heap);

    return result.ok();
}


// tests_sort_dispatch_properties_of
bool tests_sort_dispatch_properties_of()
{
    verdict     result;
    sort_traits properties;

    //   properties_of is the run-time face of the same table.  Every
    // enumerator must produce exactly its tag's values.
    properties = properties_of(sort_algorithm::bubble);
    result.check(properties.is_stable == bubble_sort_tag::is_stable);
    result.check(properties.is_in_place == bubble_sort_tag::is_in_place);
    result.check(properties.is_adaptive == bubble_sort_tag::is_adaptive);

    properties = properties_of(sort_algorithm::selection);
    result.check(properties.is_stable == selection_sort_tag::is_stable);
    result.check(properties.is_in_place == selection_sort_tag::is_in_place);
    result.check(properties.is_adaptive == selection_sort_tag::is_adaptive);

    properties = properties_of(sort_algorithm::insertion);
    result.check(properties.is_stable == insertion_sort_tag::is_stable);
    result.check(properties.is_in_place == insertion_sort_tag::is_in_place);
    result.check(properties.is_adaptive == insertion_sort_tag::is_adaptive);

    properties = properties_of(sort_algorithm::merge);
    result.check(properties.is_stable == merge_sort_tag::is_stable);
    result.check(properties.is_in_place == merge_sort_tag::is_in_place);
    result.check(properties.is_adaptive == merge_sort_tag::is_adaptive);

    properties = properties_of(sort_algorithm::quick);
    result.check(properties.is_stable == quick_sort_tag::is_stable);
    result.check(properties.is_in_place == quick_sort_tag::is_in_place);
    result.check(properties.is_adaptive == quick_sort_tag::is_adaptive);

    properties = properties_of(sort_algorithm::heap);
    result.check(properties.is_stable == heap_sort_tag::is_stable);
    result.check(properties.is_in_place == heap_sort_tag::is_in_place);
    result.check(properties.is_adaptive == heap_sort_tag::is_adaptive);

    return result.ok();
}


// tests_sort_dispatch_name_of
bool tests_sort_dispatch_name_of()
{
    verdict result;

    result.check(names_equal(name_of(sort_algorithm::bubble),
                             bubble_sort_tag::name()));
    result.check(names_equal(name_of(sort_algorithm::selection),
                             selection_sort_tag::name()));
    result.check(names_equal(name_of(sort_algorithm::insertion),
                             insertion_sort_tag::name()));
    result.check(names_equal(name_of(sort_algorithm::merge),
                             merge_sort_tag::name()));
    result.check(names_equal(name_of(sort_algorithm::quick),
                             quick_sort_tag::name()));
    result.check(names_equal(name_of(sort_algorithm::heap),
                             heap_sort_tag::name()));

    result.check(names_equal(name_of(sort_algorithm::quick), "quick_sort"));

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
                              bubble_sort_tag::name()));
    result.check(!names_equal(name_of(unrecognised_algorithm()),
                              quick_sort_tag::name()));

    return result.ok();
}


NS_END  // testing
NS_END  // djinterp
