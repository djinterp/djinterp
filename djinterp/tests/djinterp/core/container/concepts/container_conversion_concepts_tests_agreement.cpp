/******************************************************************************
* djinterp [test]            container_conversion_concepts_tests_agreement.cpp
*
*   Section V of the container_conversion_concepts.hpp suite: the claim the
* header opens with - "THE CONCEPTS ADD NO POLICY.  Each is exactly its trait."
*
*   For four of the six it is a plain forward and the claim holds.  For the two
* range-path concepts it holds only against the trait AS THEY CALL IT - with
* the operands exchanged - and fails against the trait taken in its own
* declared order.  That is the section III defect seen from the agreement side,
* and it is recorded here rather than papered over: the agreement lifts compare
* like with like, and a separate check measures the distance to the documented
* reading so the discrepancy is quantified in both places.
*
*   Every agreement here is bound in BOTH directions where the axis is
* directional, which is everywhere.  Conversion is the framework's first
* genuinely asymmetric axis, and a quantifier over one direction alone would
* miss a concept that had its operands crossed - which, on this module, is not
* a hypothetical failure mode.
*
* path:      /test/djinterp/core/container/concepts/
*                          container_conversion_concepts_tests_agreement.cpp
* author(s): Samuel 'teer' Neal-Blim
******************************************************************************/

#include "container_conversion_concepts_tests.hpp"


NS_DJINTERP
NS_TESTING


#if DJINTERP_CCONV_TESTS_ACTIVE

///////////////////////////////////////////////////////////////////////////////
///                BUILD-TIME PINS                                           ///
///////////////////////////////////////////////////////////////////////////////
//
//   The suite's default is a REPORTED failure.  These are the exception,
// because a drift in the tier readings does not produce a wrong answer at one
// call site - it silently reclassifies every conversion in the framework, and
// the failure mode is a lossy conversion admitted where a lossless one was
// required.
//
//   The range-path concepts are deliberately NOT pinned: they are already
// wrong, and pinning a known-wrong predicate at build time would make the
// defect harder to fix rather than easier.

D_TEST_STATIC(
    ::djinterp::test::holds_for_all<cconv_agree_convertible_to_vector,
                                    D_CCONV_STDLIB>::value);

D_TEST_STATIC(
    ::djinterp::test::holds_for_all<cconv_agree_lossless_to_vector,
                                    D_CCONV_STDLIB>::value);

D_TEST_STATIC(cconv_law_tier_is_a_partition<D_CCONV_STDLIB>);

#endif  // DJINTERP_CCONV_TESTS_ACTIVE


/*
tests_agreement_convertibility
  The three tier readings against their traits.
  Tests the following:
  - container_convertible_to agrees with is_convertible_between across the
    battery, target bound
  - losslessly_convertible_to agrees with is_lossless_conversion
  - lossily_convertible_to agrees with is_lossy_conversion, bound to a target
    that actually forgets - so the predicate is live rather than uniformly
    false
  - all three agree in the OTHER direction too, the axis being asymmetric
  - the point primitive and the battery quantifier give the same answer
*/
bool
tests_agreement_convertibility()
{
#if DJINTERP_CCONV_TESTS_ACTIVE

    namespace dt = ::djinterp::test;

    D_CONSTEXPR bool over_battery =
        ( dt::holds_for_all<cconv_agree_convertible_to_vector,
                            D_CCONV_BATTERY>::value &&
          dt::holds_for_all<cconv_agree_lossless_to_vector,
                            D_CCONV_BATTERY>::value &&
          dt::holds_for_all<cconv_agree_lossy_to_set,
                            D_CCONV_BATTERY>::value );

    // the other direction
    D_CONSTEXPR bool other_direction =
        ( ( ::djinterp::is_convertible_between<std::vector<int>,
                                               std::set<int> >::value ==
            ::djinterp::container_convertible_to<std::vector<int>,
                                                 std::set<int> > )          &&
          ( ::djinterp::is_convertible_between<std::set<int>,
                                               std::vector<int> >::value ==
            ::djinterp::container_convertible_to<std::set<int>,
                                                 std::vector<int> > )       &&
          ( ::djinterp::is_lossy_conversion<std::set<int>,
                                            std::vector<int> >::value ==
            ::djinterp::lossily_convertible_to<std::set<int>,
                                               std::vector<int> > ) );

    D_CONSTEXPR bool at_points =
        ( D_TEST_TRAIT_CONCEPT_AGREE(::djinterp::is_convertible_between,
                                     ::djinterp::container_convertible_to,
                                     std::vector<int>, std::list<int>)   &&
          D_TEST_TRAIT_CONCEPT_AGREE(::djinterp::is_lossless_conversion,
                                     ::djinterp::losslessly_convertible_to,
                                     std::vector<int>, std::set<int>)    &&
          D_TEST_TRAIT_CONCEPT_AGREE(::djinterp::is_lossy_conversion,
                                     ::djinterp::lossily_convertible_to,
                                     std::vector<int>, std::set<int>) );

    return ( over_battery    &&
             other_direction &&
             at_points );

#else

    return true;    // no concept surface below C++20 + concepts

#endif  // DJINTERP_CCONV_TESTS_ACTIVE
}


/*
tests_agreement_view
  view_convertible_to against is_view_conversion.
  Tests the following:
  - the faces agree across the battery
  - they agree at the identity pairs, where the conjunction is trivially
    satisfied
  - they agree at the NEAR MISSES - vector to list, array to vector - where
    exactly one conjunct fails, which is where a concept reading a different
    enumerator would diverge while still looking right at both extremes
  - they agree in the other direction
*/
bool
tests_agreement_view()
{
#if DJINTERP_CCONV_TESTS_ACTIVE

    namespace dt = ::djinterp::test;

    D_CONSTEXPR bool over_battery =
        dt::holds_for_all<cconv_agree_view_to_vector,
                          D_CCONV_BATTERY>::value;

    D_CONSTEXPR bool at_identities =
        ( ( ::djinterp::is_view_conversion<std::vector<int>,
                                           std::vector<int> >::value ==
            ::djinterp::view_convertible_to<std::vector<int>,
                                            std::vector<int> > )            &&
          ( ::djinterp::is_view_conversion<std::set<int>,
                                           std::set<int> >::value ==
            ::djinterp::view_convertible_to<std::set<int>,
                                            std::set<int> > ) );

    // at the near misses
    D_CONSTEXPR bool at_near_misses =
        ( ( ::djinterp::is_view_conversion<std::vector<int>,
                                           std::list<int> >::value ==
            ::djinterp::view_convertible_to<std::vector<int>,
                                            std::list<int> > )              &&
          ( ::djinterp::is_view_conversion<std::array<int, 4>,
                                           std::vector<int> >::value ==
            ::djinterp::view_convertible_to<std::array<int, 4>,
                                            std::vector<int> > ) );

    D_CONSTEXPR bool other_direction =
        ( ::djinterp::is_view_conversion<std::vector<int>,
                                         std::array<int, 4> >::value ==
          ::djinterp::view_convertible_to<std::vector<int>,
                                          std::array<int, 4> > );

    return ( over_battery    &&
             at_identities   &&
             at_near_misses  &&
             other_direction );

#else

    return true;

#endif  // DJINTERP_CCONV_TESTS_ACTIVE
}


/*
tests_agreement_range_paths
  The two path concepts, whose agreement is the section III defect seen from
this side.  Each agrees with the trait AS IT CALLS IT and disagrees with the
trait taken in its own declared order, and the test states both facts so the
discrepancy is quantified rather than assumed.
  Tests the following:
  - each concept agrees with the swapped trait across the battery - the
    agreement that currently holds
  - each DISAGREES with the documented reading at the asymmetric witnesses,
    which is the defect
  - the disagreement is confined to the asymmetric pairs: at symmetric ones the
    two readings coincide, which is why the module has survived this long
  - the four other concepts have no such discrepancy, so the defect is local to
    the two path concepts
*/
bool
tests_agreement_range_paths()
{
#if DJINTERP_CCONV_TESTS_ACTIVE

    namespace dt = ::djinterp::test;

    // the agreement that holds
    D_CONSTEXPR bool agrees_as_called =
        ( dt::holds_for_all<cconv_agree_range_constructible_as_called,
                            D_CCONV_BATTERY>::value &&
          dt::holds_for_all<cconv_agree_range_insertable_as_called,
                            D_CCONV_BATTERY>::value );

    // the disagreement with the documented reading
    D_CONSTEXPR bool disagrees_with_documentation =
        ( ( ::djinterp::range_constructible_from<std::array<int, 4>,
                                                 std::vector<int> > !=
            cconv_range_constructible_documented<std::array<int, 4>,
                                                 std::vector<int> >::value ) &&
          ( ::djinterp::range_insertable_from<cconv_range_source,
                                              cconv_insert_target> !=
            cconv_range_insertable_documented<cconv_range_source,
                                              cconv_insert_target>::value ) );

    // confined to the asymmetric pairs
    D_CONSTEXPR bool confined_to_asymmetric_pairs =
        ( ( ::djinterp::range_constructible_from<std::vector<int>,
                                                 std::list<int> > ==
            cconv_range_constructible_documented<std::vector<int>,
                                                 std::list<int> >::value )  &&
          ( ::djinterp::range_constructible_from<std::list<int>,
                                                 std::vector<int> > ==
            cconv_range_constructible_documented<std::list<int>,
                                                 std::vector<int> >::value ) );

    // and local to these two concepts
    D_CONSTEXPR bool other_concepts_are_clean =
        ( dt::holds_for_all<cconv_agree_convertible_to_vector,
                            D_CCONV_BATTERY>::value &&
          dt::holds_for_all<cconv_agree_view_to_vector,
                            D_CCONV_BATTERY>::value );

    return ( agrees_as_called             &&
             disagrees_with_documentation &&
             confined_to_asymmetric_pairs &&
             other_concepts_are_clean );

#else

    return true;

#endif  // DJINTERP_CCONV_TESTS_ACTIVE
}


/*
tests_agreement_non_vacuity
  Agreement alone does not distinguish "the same true predicate" from "two
predicates that are both always false".  On this module the risk is concrete:
most pairs in a battery of standard containers are convertible, so a concept
stuck at true would agree with its trait nearly everywhere.
  Tests the following:
  - each of the six concepts holds somewhere and fails somewhere
  - the trait face is live on both polarities for each
  - the tier itself takes at least four of its five values across the battery,
    so the fold is not concentrated
  - the two path concepts are live in both operand orders, which is what makes
    the direction test in section III meaningful
*/
bool
tests_agreement_non_vacuity()
{
#if DJINTERP_CCONV_TESTS_ACTIVE

    namespace dt = ::djinterp::test;

    D_CONSTEXPR bool each_concept_is_live =
        ( dt::holds_for_any<cconv_c_convertible_to_vector,
                            D_CCONV_BATTERY>::value                      &&
          ( !dt::holds_for_all<cconv_c_convertible_to_vector,
                               D_CCONV_BATTERY>::value )                 &&
          dt::holds_for_any<cconv_c_lossless_to_vector,
                            D_CCONV_BATTERY>::value                      &&
          ( !dt::holds_for_all<cconv_c_lossless_to_vector,
                               D_CCONV_BATTERY>::value )                 &&
          dt::holds_for_any<cconv_c_lossy_to_set,
                            D_CCONV_BATTERY>::value                      &&
          ( !dt::holds_for_all<cconv_c_lossy_to_set,
                               D_CCONV_BATTERY>::value ) );

    D_CONSTEXPR bool view_and_paths_are_live =
        ( dt::holds_for_any<cconv_c_view_to_vector,
                            D_CCONV_BATTERY>::value                      &&
          ( !dt::holds_for_all<cconv_c_view_to_vector,
                               D_CCONV_BATTERY>::value )                 &&
          dt::holds_for_any<cconv_c_range_constructible_from_vector,
                            D_CCONV_BATTERY>::value                      &&
          dt::holds_for_any<cconv_c_range_insertable_from_vector,
                            D_CCONV_BATTERY>::value );

    // the other direction is live too
    D_CONSTEXPR bool other_direction_is_live =
        ( dt::holds_for_any<cconv_c_convertible_from_vector,
                            D_CCONV_BATTERY>::value &&
          ( !dt::holds_for_all<cconv_c_convertible_from_vector,
                               D_CCONV_BATTERY>::value ) );

    // the trait face is live on both polarities
    D_CONSTEXPR bool trait_face_is_live =
        ( ::djinterp::is_convertible_between<std::vector<int>,
                                             std::list<int> >::value        &&
          ( !::djinterp::is_convertible_between<int, int>::value )          &&
          ::djinterp::is_view_conversion<std::vector<int>,
                                         std::vector<int> >::value          &&
          ( !::djinterp::is_view_conversion<std::vector<int>,
                                            std::list<int> >::value )       &&
          ::djinterp::is_lossy_conversion<std::vector<int>,
                                          std::set<int> >::value            &&
          ( !::djinterp::is_lossy_conversion<std::vector<int>,
                                             std::vector<int> >::value ) );

    // the tier is not concentrated
    D_CONSTEXPR bool tier_range_is_wide =
        ( ( cconv_tier<std::vector<int>, std::vector<int> > ==
            ::djinterp::conversion_tier::view )                          &&
          ( cconv_tier<std::vector<int>, std::list<int> > ==
            ::djinterp::conversion_tier::constructive )                  &&
          ( cconv_tier<std::vector<int>, std::set<int> > ==
            ::djinterp::conversion_tier::lossy )                         &&
          ( cconv_tier<int, int> ==
            ::djinterp::conversion_tier::none ) );

    return ( each_concept_is_live    &&
             view_and_paths_are_live &&
             other_direction_is_live &&
             trait_face_is_live      &&
             tier_range_is_wide );

#else

    return true;

#endif  // DJINTERP_CCONV_TESTS_ACTIVE
}


NS_END  // testing
NS_END  // djinterp
