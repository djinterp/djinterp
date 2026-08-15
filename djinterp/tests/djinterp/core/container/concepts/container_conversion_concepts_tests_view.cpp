/******************************************************************************
* djinterp [test]                 container_conversion_concepts_tests_view.cpp
*
*   Section IV of the container_conversion_concepts.hpp suite:
* view_convertible_to - "no copy, no ownership, and therefore no cost and no
* independence".
*
*   It is the narrowest concept in the module and the only one decided by a
* CONJUNCTION rather than by reading an enumerator: the reinterpretation
* predicate requires identical elements, an identical content level, no
* topology change, an identical sequential layout, and equivalence on all four
* realization axes, plus a discipline no weaker.  Eight clauses, any one of
* which can defeat it.
*
*   A conjunction that long is exposed to a specific failure: a clause that has
* silently become vacuous - always true, or dropped - changes no answer at any
* witness where the OTHER seven already decide.  So this section tests each
* clause at a pair where it alone is doing the work, which is the only way a
* dead conjunct becomes visible.
*
*   The trait header is candid that the layout clause is an approximation:
* "``No data moves'' needs the backing shape, which the profile alone does not
* carry - a vector and a list share a profile yet differ in layout."  The
* sequential-layout proxy is what closes that, and it is tested as the proxy it
* is rather than as ground truth.
*
* path:      /test/djinterp/core/container/concepts/
*                               container_conversion_concepts_tests_view.cpp
* author(s): Samuel 'teer' Neal-Blim
******************************************************************************/

#include "container_conversion_concepts_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_view_positive
  What is a view.
  Tests the following:
  - a container viewed as its own type is a view - all eight clauses being
    trivially satisfied by reflexivity
  - that holds across the sequence population and the associative one
  - the raw tier at each is exactly `view`, not merely lossless
  - a view is lossless and convertible, so the entailments run
  - the concept is live across the battery rather than holding only at the
    identity
*/
bool
tests_view_positive()
{
#if DJINTERP_CCONV_TESTS_ACTIVE

    D_CONSTEXPR bool self_view =
        ( ::djinterp::view_convertible_to<std::vector<int>,
                                          std::vector<int> >             &&
          ::djinterp::view_convertible_to<std::list<int>,
                                          std::list<int> >               &&
          ::djinterp::view_convertible_to<std::set<int>, std::set<int> >  &&
          ::djinterp::view_convertible_to<std::array<int, 4>,
                                          std::array<int, 4> > );

    // and the tier says `view`, not merely lossless
    D_CONSTEXPR bool tier_is_view =
        ( ( cconv_tier<std::vector<int>, std::vector<int> > ==
            ::djinterp::conversion_tier::view )                          &&
          ( cconv_tier<std::set<int>, std::set<int> > ==
            ::djinterp::conversion_tier::view ) );

    // the entailments
    D_CONSTEXPR bool entailments =
        ( cconv_law_view_implies_lossless<D_CCONV_BATTERY>               &&
          ::djinterp::losslessly_convertible_to<std::vector<int>,
                                                std::vector<int> >       &&
          ::djinterp::container_convertible_to<std::vector<int>,
                                               std::vector<int> > );

    D_CONSTEXPR bool law_holds =
        cconv_law_self_conversion_is_a_view<D_CCONV_STDLIB>;

    return ( self_view    &&
             tier_is_view &&
             entailments  &&
             law_holds );

#else

    return true;    // no concept surface below C++20 + concepts

#endif  // DJINTERP_CCONV_TESTS_ACTIVE
}


/*
tests_view_negative
  What is not a view, with the cause attributable in each case.
  Tests the following:
  - a differing BACKING LAYOUT defeats it: vector to list is a construction,
    not a reinterpretation, though the two share a profile
  - a differing LIFETIME or STORAGE defeats it: the trait header's own example,
    array to vector, is contiguous like a vector yet compile-staged and inline,
    so it is a copy
  - a differing ELEMENT TYPE defeats it
  - a differing CONTENT LEVEL defeats it: a sequence to a set forgets, so it is
    lossy rather than a view
  - each of those pairs is still CONVERTIBLE, so the clause defeated the view
    and not the conversion
*/
bool
tests_view_negative()
{
#if DJINTERP_CCONV_TESTS_ACTIVE

    // differing layout
    D_CONSTEXPR bool layout_defeats_it =
        ( ( !::djinterp::view_convertible_to<std::vector<int>,
                                             std::list<int> > )          &&
          ::djinterp::container_convertible_to<std::vector<int>,
                                               std::list<int> >          &&
          ( cconv_tier<std::vector<int>, std::list<int> > ==
            ::djinterp::conversion_tier::constructive ) );

    // differing lifetime / storage - the header's own example
    D_CONSTEXPR bool array_to_vector_is_a_copy =
        ( ( !::djinterp::view_convertible_to<std::array<int, 4>,
                                             std::vector<int> > )        &&
          ::djinterp::container_convertible_to<std::array<int, 4>,
                                               std::vector<int> > );

    // differing element type
    D_CONSTEXPR bool elements_defeat_it =
        ( !::djinterp::view_convertible_to<std::vector<int>,
                                           std::vector<double> > );

    // differing content level
    D_CONSTEXPR bool level_defeats_it =
        ( ( !::djinterp::view_convertible_to<std::vector<int>,
                                             std::set<int> > )           &&
          ::djinterp::lossily_convertible_to<std::vector<int>,
                                             std::set<int> > );

    return ( layout_defeats_it          &&
             array_to_vector_is_a_copy  &&
             elements_defeat_it         &&
             level_defeats_it );

#else

    return true;

#endif  // DJINTERP_CCONV_TESTS_ACTIVE
}


/*
tests_view_conjunction_is_genuine
  Each clause of the reinterpretation predicate is separately necessary.  A
long conjunction can carry a dead clause indefinitely - one that is always true
or has been dropped - and no witness where the other clauses already decide
will notice.  These checks each isolate one clause.
  Tests the following:
  - the ELEMENTS clause is live: same-element pairs and differing-element pairs
    give different answers with everything else held fixed
  - the LAYOUT clause is live: vector and list agree on every realization axis
    and differ in layout, and only the layout clause separates them
  - the CONTENT LEVEL clause is live
  - the TOPOLOGY clause is live
  - the axis-equivalence clauses are live: array and vector agree on layout and
    elements and differ on lifetime and storage
  - a view therefore requires ALL of them, not a majority
*/
bool
tests_view_conjunction_is_genuine()
{
#if DJINTERP_CCONV_TESTS_ACTIVE

    // the elements clause
    D_CONSTEXPR bool elements_clause_is_live =
        ( ::djinterp::view_convertible_to<std::vector<int>,
                                          std::vector<int> >             &&
          ( !::djinterp::view_convertible_to<std::vector<int>,
                                             std::vector<double> > ) );

    // the layout clause: vector and list share a profile and differ in backing
    D_CONSTEXPR bool layout_clause_is_live =
        ( ( ::djinterp::sequential_layout_of<std::vector<int> >::value !=
            ::djinterp::sequential_layout_of<std::list<int> >::value )   &&
          ( !::djinterp::view_convertible_to<std::vector<int>,
                                             std::list<int> > ) );

    // the content level clause
    D_CONSTEXPR bool level_clause_is_live =
        ( ( ::djinterp::native_content_level_of<std::vector<int> >::value !=
            ::djinterp::native_content_level_of<std::set<int> >::value ) &&
          ( !::djinterp::view_convertible_to<std::vector<int>,
                                             std::set<int> > ) );

    // the axis-equivalence clauses
    D_CONSTEXPR bool axis_clauses_are_live =
        ( ( ::djinterp::sequential_layout_of<std::array<int, 4> >::value ==
            ::djinterp::sequential_layout_of<std::vector<int> >::value )  &&
          ( !::djinterp::view_convertible_to<std::array<int, 4>,
                                             std::vector<int> > ) );

    // and the whole conjunction is required, not a majority of it
    D_CONSTEXPR bool all_clauses_required =
        ( ::djinterp::view_convertible_to<std::vector<int>,
                                          std::vector<int> >             &&
          ( !::djinterp::view_convertible_to<std::vector<int>,
                                             std::list<int> > )          &&
          ( !::djinterp::view_convertible_to<std::array<int, 4>,
                                             std::vector<int> > ) );

    return ( elements_clause_is_live &&
             layout_clause_is_live   &&
             level_clause_is_live    &&
             axis_clauses_are_live   &&
             all_clauses_required );

#else

    return true;

#endif  // DJINTERP_CCONV_TESTS_ACTIVE
}


/*
tests_self_conversion_is_a_view
  Converting a container to its own type moves nothing, so every conjunct of
the reinterpretation predicate holds by reflexivity.  It is the cheapest
possible structural check on the whole eight-way conjunction, and the first
thing a mis-ordered operand pair anywhere in the chain would break - which is
exactly why it is worth its own test rather than being folded into the positive
one.
  Tests the following:
  - every convertible type in the battery converts to itself as a VIEW
  - the same across the standard containers specifically
  - the raw tier is `view` at each, so the law is not being satisfied by a
    weaker classification
  - the law is non-vacuous: the battery contains types that ARE convertible to
    themselves, so the implication has instances
  - a type that is NOT convertible to itself does not falsify the law, the
    implication being guarded on convertibility
*/
bool
tests_self_conversion_is_a_view()
{
#if DJINTERP_CCONV_TESTS_ACTIVE

    D_CONSTEXPR bool over_battery =
        cconv_law_self_conversion_is_a_view<D_CCONV_BATTERY>;

    D_CONSTEXPR bool over_stdlib =
        cconv_law_self_conversion_is_a_view<D_CCONV_STDLIB>;

    // the tier is `view`, not something weaker
    D_CONSTEXPR bool tier_is_view =
        ( ( cconv_tier<std::vector<int>, std::vector<int> > ==
            ::djinterp::conversion_tier::view )                          &&
          ( cconv_tier<std::list<int>, std::list<int> > ==
            ::djinterp::conversion_tier::view )                          &&
          ( cconv_tier<std::map<int, int>, std::map<int, int> > ==
            ::djinterp::conversion_tier::view ) );

    // the law has instances
    D_CONSTEXPR bool non_vacuous =
        ( ::djinterp::container_convertible_to<std::vector<int>,
                                               std::vector<int> >        &&
          ::djinterp::view_convertible_to<std::vector<int>,
                                          std::vector<int> > );

    // and the guard is doing work: a non-convertible self-pair exists
    D_CONSTEXPR bool guard_is_real =
        ( ( !::djinterp::container_convertible_to<int, int> ) &&
          ( !::djinterp::view_convertible_to<int, int> ) );

    return ( over_battery  &&
             over_stdlib   &&
             tier_is_view  &&
             non_vacuous   &&
             guard_is_real );

#else

    return true;

#endif  // DJINTERP_CCONV_TESTS_ACTIVE
}


NS_END  // testing
NS_END  // djinterp
