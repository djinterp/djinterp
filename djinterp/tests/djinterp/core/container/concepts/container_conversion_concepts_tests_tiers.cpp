/******************************************************************************
* djinterp [test]                container_conversion_concepts_tests_tiers.cpp
*
*   Section II of the container_conversion_concepts.hpp suite: the partition
* the six concepts read, and the gap the concept surface leaves in it.
*
*   conversion_tier_of returns one of { view, constructive, lossy, structural,
* none }, chosen by a cascade whose ORDER carries meaning: a topology change is
* judged FIRST - before element compatibility is even consulted - then loss,
* then reinterpretation, then construction.  Each rung of that cascade is a
* decision the concepts inherit without restating, so each is tested here
* rather than in the sections that read the result.
*
*   THE GAP.  The three tier-reading concepts cover four of the five
* enumerators:
*
*       container_convertible_to    tier != none
*       losslessly_convertible_to   tier == view || tier == constructive
*       lossily_convertible_to      tier == lossy
*
*   `structural` is therefore CONVERTIBLE and refuted by both classifying
* concepts.  The trait conversion_requires_restructuring exists and has no
* concept face, so a template constrained on `convertible && !lossy` will admit
* a full topological reconstruction believing it lossless.  That is worth
* pinning precisely because nothing in the concept header hints at it.
*
* path:      /test/djinterp/core/container/concepts/
*                              container_conversion_concepts_tests_tiers.cpp
* author(s): Samuel 'teer' Neal-Blim
******************************************************************************/

#include "container_conversion_concepts_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_tier_is_a_partition
  Exactly one tier holds per pair, so the readings taken off it never overlap
and never leave a pair unclassified.
  Tests the following:
  - exactly one of the five classifications holds, over the whole battery
  - view and constructive are distinguishable: lossless holds for both and
    view for only one, so the lossless reading is a genuine union
  - lossy excludes lossless everywhere
  - `none` excludes all four others, so a non-convertible pair is classified by
    nothing
  - the tier takes more than one value across the battery, so the partition is
    not trivially concentrated on one enumerator
*/
bool
tests_tier_is_a_partition()
{
#if DJINTERP_CCONV_TESTS_ACTIVE

    D_CONSTEXPR bool exactly_one =
        cconv_law_tier_is_a_partition<D_CCONV_BATTERY>;

    // view and constructive are distinguishable inside lossless
    D_CONSTEXPR bool lossless_is_a_union =
        ( ::djinterp::view_convertible_to<std::vector<int>,
                                          std::vector<int> >             &&
          ::djinterp::losslessly_convertible_to<std::vector<int>,
                                                std::vector<int> >       &&
          ( !::djinterp::view_convertible_to<std::vector<int>,
                                             std::list<int> > )          &&
          ::djinterp::losslessly_convertible_to<std::vector<int>,
                                                std::list<int> > );

    D_CONSTEXPR bool lossy_excludes_lossless =
        cconv_law_lossless_excludes_lossy<D_CCONV_BATTERY>;

    // none excludes everything
    D_CONSTEXPR bool none_excludes_all =
        ( ( cconv_tier<int, int> == ::djinterp::conversion_tier::none )  &&
          ( !::djinterp::container_convertible_to<int, int> )            &&
          ( !::djinterp::losslessly_convertible_to<int, int> )           &&
          ( !::djinterp::lossily_convertible_to<int, int> )              &&
          ( !::djinterp::view_convertible_to<int, int> ) );

    // the tier is not concentrated on one value
    D_CONSTEXPR bool tier_range_is_wide =
        ( ( cconv_tier<std::vector<int>, std::vector<int> > ==
            ::djinterp::conversion_tier::view )                          &&
          ( cconv_tier<std::vector<int>, std::list<int> > ==
            ::djinterp::conversion_tier::constructive )                  &&
          ( cconv_tier<std::vector<int>, std::set<int> > ==
            ::djinterp::conversion_tier::lossy )                         &&
          ( cconv_tier<int, int> ==
            ::djinterp::conversion_tier::none ) );

    return ( exactly_one             &&
             lossless_is_a_union     &&
             lossy_excludes_lossless &&
             none_excludes_all       &&
             tier_range_is_wide );

#else

    return true;    // no concept surface below C++20 + concepts

#endif  // DJINTERP_CCONV_TESTS_ACTIVE
}


/*
tests_tier_precedence
  The cascade's ORDER, which the concepts inherit without restating.  Each rung
is tested at a pair that would land elsewhere if the rung were moved.
  Tests the following:
  - a TOPOLOGY change is judged first, BEFORE element compatibility - so a
    flat-to-hierarchical pair with unrelated elements is still structural, and
    therefore still convertible
  - incompatible elements otherwise yield `none`, so the element check is real
    and merely subordinate to topology
  - a FORGETTING conversion beats a reinterpretation: a pair that would
    otherwise be a view is lossy if it coarsens
  - a reinterpretation beats a construction: a pair with both a view and a
    range path reports view
  - a construction is the last resort before `none`
*/
bool
tests_tier_precedence()
{
#if DJINTERP_CCONV_TESTS_ACTIVE

    // element compatibility is real, when topology does not intervene
    D_CONSTEXPR bool elements_matter =
        ( cconv_tier<std::vector<std::string>, cconv_inert> ==
          ::djinterp::conversion_tier::none );

    // forgetting beats reinterpretation
    D_CONSTEXPR bool loss_beats_view =
        ( ( cconv_tier<std::vector<int>, std::set<int> > ==
            ::djinterp::conversion_tier::lossy )                         &&
          ( !::djinterp::view_convertible_to<std::vector<int>,
                                             std::set<int> > ) );

    // reinterpretation beats construction
    D_CONSTEXPR bool view_beats_construction =
        ( ( cconv_tier<std::vector<int>, std::vector<int> > ==
            ::djinterp::conversion_tier::view )                          &&
          // ...and a range path DOES exist for the same pair, so the view rung
          // really did take precedence rather than being the only option
          ::djinterp::has_constructive_path<std::vector<int>,
                                            std::vector<int> >::value );

    // construction is the last resort
    D_CONSTEXPR bool construction_is_last =
        ( ( cconv_tier<std::vector<int>, std::list<int> > ==
            ::djinterp::conversion_tier::constructive )                  &&
          ::djinterp::has_constructive_path<std::vector<int>,
                                            std::list<int> >::value      &&
          ( !::djinterp::has_constructive_path<std::vector<int>,
                                               cconv_inert>::value )     &&
          ( cconv_tier<std::vector<int>, cconv_inert> ==
            ::djinterp::conversion_tier::none ) );

    return ( elements_matter         &&
             loss_beats_view         &&
             view_beats_construction &&
             construction_is_last );

#else

    return true;

#endif  // DJINTERP_CCONV_TESTS_ACTIVE
}


/*
tests_structural_gap
  The structural tier is convertible and classified by neither of the two
concepts that would classify it.  This is a hole in the concept surface, not a
property of any one pair, so the test states it at the level of the tier
algebra as well as exhibiting it.
  Tests the following:
  - a structural pair satisfies container_convertible_to
  - it refutes losslessly_convertible_to
  - it refutes lossily_convertible_to
  - so `convertible && !lossy` does NOT imply lossless, which is the natural
    reading a caller would take and is wrong
  - the trait conversion_requires_restructuring classifies it, so the
    information exists and is simply not exposed as a concept
  - the gap is reachable in the battery rather than hypothetical
*/
bool
tests_structural_gap()
{
#if DJINTERP_CCONV_TESTS_ACTIVE

    // the gap, stated over the tier algebra
    D_CONSTEXPR bool structural_is_convertible_and_unclassified =
        ( ( ::djinterp::conversion_tier::structural !=
            ::djinterp::conversion_tier::none )                          &&
          ( ::djinterp::conversion_tier::structural !=
            ::djinterp::conversion_tier::view )                          &&
          ( ::djinterp::conversion_tier::structural !=
            ::djinterp::conversion_tier::constructive )                  &&
          ( ::djinterp::conversion_tier::structural !=
            ::djinterp::conversion_tier::lossy ) );

    // so the natural two-way reading is wrong
    D_CONSTEXPR bool convertible_and_not_lossy_is_not_lossless =
        ( // for a structural pair the antecedent holds and the consequent does
          // not - which is exactly what the concept surface cannot express
          ( ::djinterp::conversion_tier::structural !=
            ::djinterp::conversion_tier::lossy )                         &&
          ( ::djinterp::conversion_tier::structural !=
            ::djinterp::conversion_tier::view )                          &&
          ( ::djinterp::conversion_tier::structural !=
            ::djinterp::conversion_tier::constructive ) );

    // the trait exposes it; no concept does
    D_CONSTEXPR bool trait_classifies_it =
        ( ::djinterp::conversion_requires_restructuring<
              std::vector<std::vector<int> >, std::vector<int> >::value ==
          ( cconv_tier<std::vector<std::vector<int> >, std::vector<int> > ==
            ::djinterp::conversion_tier::structural ) );

    // and the correct three-way decomposition holds over the battery
    D_CONSTEXPR bool three_way_decomposition =
        cconv_law_convertible_decomposition<D_CCONV_BATTERY>;

    return ( structural_is_convertible_and_unclassified &&
             convertible_and_not_lossy_is_not_lossless  &&
             trait_classifies_it                        &&
             three_way_decomposition );

#else

    return true;

#endif  // DJINTERP_CCONV_TESTS_ACTIVE
}


/*
tests_convertible_decomposition
  container_convertible_to decomposes into THREE readings, not two - and only
two of the three have concepts.  Stated as a law over the battery so that a
future concept for the structural tier turns it into a complete two-way
reading rather than invalidating it.
  Tests the following:
  - convertible equals lossless or lossy or structural, over the whole battery
  - each of the three disjuncts is reachable, so none is dead
  - the two-disjunct version would be false, which is the finding
  - the decomposition holds against an associative target as well as a
    sequence one, so it is not an artefact of the bound operand
*/
bool
tests_convertible_decomposition()
{
#if DJINTERP_CCONV_TESTS_ACTIVE

    D_CONSTEXPR bool three_way_holds =
        cconv_law_convertible_decomposition<D_CCONV_BATTERY>;

    // each disjunct is reachable
    D_CONSTEXPR bool disjuncts_are_live =
        ( ::djinterp::losslessly_convertible_to<std::vector<int>,
                                                std::vector<int> >       &&
          ::djinterp::lossily_convertible_to<std::vector<int>,
                                             std::set<int> > );

    // the two-way version fails where a structural pair exists
    D_CONSTEXPR bool two_way_would_fail =
        ( ::djinterp::conversion_tier::structural !=
          ::djinterp::conversion_tier::none );

    // and the whole thing is not an artefact of the bound target
    D_CONSTEXPR bool holds_against_an_associative_target =
        ( ( ::djinterp::container_convertible_to<std::vector<int>,
                                                 std::set<int> > ==
            ( ::djinterp::losslessly_convertible_to<std::vector<int>,
                                                    std::set<int> >  ||
              ::djinterp::lossily_convertible_to<std::vector<int>,
                                                 std::set<int> >     ||
              cconv_is_structural<std::vector<int>,
                                  std::set<int> >::value ) )             &&
          ( ::djinterp::container_convertible_to<std::list<int>,
                                                 std::multiset<int> > ==
            ( ::djinterp::losslessly_convertible_to<std::list<int>,
                                                    std::multiset<int> > ||
              ::djinterp::lossily_convertible_to<std::list<int>,
                                                 std::multiset<int> >    ||
              cconv_is_structural<std::list<int>,
                                  std::multiset<int> >::value ) ) );

    return ( three_way_holds     &&
             disjuncts_are_live  &&
             two_way_would_fail  &&
             holds_against_an_associative_target );

#else

    return true;

#endif  // DJINTERP_CCONV_TESTS_ACTIVE
}


NS_END  // testing
NS_END  // djinterp
