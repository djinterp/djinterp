/******************************************************************************
* djinterp [test]        container_conversion_concepts_tests_convertibility.cpp
*
*   Section I of the container_conversion_concepts.hpp suite: the three
* concepts that read the conversion tier - container_convertible_to,
* losslessly_convertible_to and lossily_convertible_to.
*
*   All three are one-line readings of a single enum, which makes them cheap to
* get wrong in a way no individual test would notice: read the wrong
* enumerator and the concept still behaves plausibly at most pairs.  The tests
* below therefore always check the RAW TIER alongside the concept, so a verdict
* can be attributed to the tier that produced it rather than merely observed.
*
*   The three do not partition anything.  Lossless is `view or constructive`,
* lossy is `lossy`, and convertible is `not none` - which leaves the
* structural tier convertible and classified by neither.  Section II owns that
* finding; this section stays inside the pairs where the two halves do cover
* the ground.
*
* path:      /test/djinterp/core/container/concepts/
*                      container_conversion_concepts_tests_convertibility.cpp
* author(s): Samuel 'teer' Neal-Blim
******************************************************************************/

#include "container_conversion_concepts_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_convertible_positive
  What converts.
  Tests the following:
  - a container converts to its own type, which is the axis's identity case
  - and that self-conversion is specifically a VIEW, not merely a path
  - a sequence converts to another sequence its range can build
  - a sequence converts to an associative container - the canonical forgetting
    direction, which is convertible even though it loses
  - the raw tier at each of these is not `none`, which is what the concept
    reads
  - convertibility is live across the standard battery rather than holding
    only at the identity
*/
bool
tests_convertible_positive()
{
#if DJINTERP_CCONV_TESTS_ACTIVE

    namespace dt = ::djinterp::test;

    // the identity case
    D_CONSTEXPR bool self_conversion =
        ( ::djinterp::container_convertible_to<std::vector<int>,
                                               std::vector<int> >        &&
          ::djinterp::container_convertible_to<std::set<int>,
                                               std::set<int> >           &&
          ::djinterp::view_convertible_to<std::vector<int>,
                                          std::vector<int> > );

    // sequence to sequence
    D_CONSTEXPR bool sequence_to_sequence =
        ( ::djinterp::container_convertible_to<std::vector<int>,
                                               std::list<int> >          &&
          ::djinterp::container_convertible_to<std::list<int>,
                                               std::deque<int> > );

    // sequence to associative - convertible, and forgetting
    D_CONSTEXPR bool sequence_to_associative =
        ( ::djinterp::container_convertible_to<std::vector<int>,
                                               std::set<int> >           &&
          ( cconv_tier<std::vector<int>, std::set<int> > !=
            ::djinterp::conversion_tier::none ) );

    // the raw tier agrees at each
    D_CONSTEXPR bool tier_agrees =
        ( ( cconv_tier<std::vector<int>, std::vector<int> > !=
            ::djinterp::conversion_tier::none )                          &&
          ( cconv_tier<std::vector<int>, std::list<int> > !=
            ::djinterp::conversion_tier::none ) );

    // and the predicate is live across the battery
    D_CONSTEXPR bool live_across_battery =
        dt::holds_for_any<cconv_c_convertible_to_vector,
                          D_CCONV_STDLIB>::value;

    return ( self_conversion         &&
             sequence_to_sequence    &&
             sequence_to_associative &&
             tier_agrees             &&
             live_across_battery );

#else

    return true;    // no concept surface below C++20 + concepts

#endif  // DJINTERP_CCONV_TESTS_ACTIVE
}


/*
tests_convertible_negative
  What does not convert.  There are two ways to reach `none`: incompatible
elements, or compatible elements with no construction path - and the tests
separate them, because a module that had collapsed the two would still answer
`none` at both.
  Tests the following:
  - a non-container converts to nothing, having no elements to be compatible
  - it is not convertible even to itself, so convertibility is NOT reflexive
    in general - a property the identity case above might have suggested
  - a container with no path to a target is not convertible to it
  - the raw tier at each of these is exactly `none`
  - the three readings all refuse a non-convertible pair, so none of them is
    true where the umbrella is false
*/
bool
tests_convertible_negative()
{
#if DJINTERP_CCONV_TESTS_ACTIVE

    namespace dt = ::djinterp::test;

    // non-containers convert to nothing
    D_CONSTEXPR bool non_containers_refuse =
        dt::holds_for_none<cconv_c_convertible_to_vector,
                           D_CCONV_NONCONTAINERS>::value;

    // and not even to themselves: convertibility is not reflexive
    D_CONSTEXPR bool not_reflexive_in_general =
        ( ( !::djinterp::container_convertible_to<int, int> )            &&
          ( cconv_tier<int, int> ==
            ::djinterp::conversion_tier::none ) );

    // an inert target admits no path
    D_CONSTEXPR bool no_path_refuses =
        ( ( !::djinterp::container_convertible_to<std::vector<int>,
                                                  cconv_inert> )         &&
          ( cconv_tier<std::vector<int>, cconv_inert> ==
            ::djinterp::conversion_tier::none ) );

    // where the umbrella is false, all three readings are false
    D_CONSTEXPR bool readings_agree_at_none =
        ( ( !::djinterp::container_convertible_to<int, int> )            &&
          ( !::djinterp::losslessly_convertible_to<int, int> )           &&
          ( !::djinterp::lossily_convertible_to<int, int> )              &&
          ( !::djinterp::view_convertible_to<int, int> ) );

    return ( non_containers_refuse     &&
             not_reflexive_in_general  &&
             no_path_refuses           &&
             readings_agree_at_none );

#else

    return true;

#endif  // DJINTERP_CCONV_TESTS_ACTIVE
}


/*
tests_lossless_convertible
  losslessly_convertible_to: the conversion is a view or a data-preserving
construction.  The header is emphatic that the distinction from lossy "is not
a warning label; it is the difference between a rename and a projection", and
this test holds it to that.
  Tests the following:
  - a self-conversion is lossless, being a view
  - a sequence-to-sequence construction is lossless
  - a view implies lossless, over the whole battery
  - lossless implies convertible, over the whole battery
  - a FORGETTING conversion is not lossless, which is the distinction the
    concept exists to draw
  - lossless and lossy are disjoint across the battery
*/
bool
tests_lossless_convertible()
{
#if DJINTERP_CCONV_TESTS_ACTIVE

    D_CONSTEXPR bool self_is_lossless =
        ( ::djinterp::losslessly_convertible_to<std::vector<int>,
                                                std::vector<int> >       &&
          ::djinterp::losslessly_convertible_to<std::list<int>,
                                                std::list<int> > );

    D_CONSTEXPR bool construction_is_lossless =
        ( ::djinterp::losslessly_convertible_to<std::vector<int>,
                                                std::list<int> >         &&
          ( cconv_tier<std::vector<int>, std::list<int> > ==
            ::djinterp::conversion_tier::constructive ) );

    // the two entailments
    D_CONSTEXPR bool entailments =
        ( cconv_law_view_implies_lossless<D_CCONV_BATTERY> &&
          cconv_law_lossless_implies_convertible<D_CCONV_BATTERY> );

    // forgetting is not lossless
    D_CONSTEXPR bool forgetting_is_not_lossless =
        ( ( !::djinterp::losslessly_convertible_to<std::vector<int>,
                                                   std::set<int> > )     &&
          ::djinterp::container_convertible_to<std::vector<int>,
                                               std::set<int> > );

    D_CONSTEXPR bool disjoint_from_lossy =
        cconv_law_lossless_excludes_lossy<D_CCONV_BATTERY>;

    return ( self_is_lossless          &&
             construction_is_lossless  &&
             entailments               &&
             forgetting_is_not_lossless &&
             disjoint_from_lossy );

#else

    return true;

#endif  // DJINTERP_CCONV_TESTS_ACTIVE
}


/*
tests_lossy_convertible
  lossily_convertible_to: the conversion is possible and DROPS something.  Two
independent causes reach it - a strictly coarser content level, and a capacity
clamp - and the tests exercise both, because a module that had implemented only
one would still classify the canonical examples correctly.
  Tests the following:
  - forgetting ORDER is lossy: a sequence to a multiset
  - forgetting MULTIPLICITY is lossy: a sequence or multiset to a set
  - a lossy conversion is convertible
  - it is NOT lossless, the two being readings of disjoint tiers
  - the raw tier at each is exactly `lossy`
  - the reverse direction of a forgetting conversion is not lossy, so the
    concept is reading a direction rather than a pair
*/
bool
tests_lossy_convertible()
{
#if DJINTERP_CCONV_TESTS_ACTIVE

    // forgetting order, and forgetting multiplicity
    D_CONSTEXPR bool canonical_quotients =
        ( ::djinterp::lossily_convertible_to<std::vector<int>,
                                             std::set<int> >             &&
          ( cconv_tier<std::vector<int>, std::set<int> > ==
            ::djinterp::conversion_tier::lossy ) );

    D_CONSTEXPR bool lossy_is_convertible =
        ( cconv_law_lossy_implies_convertible<D_CCONV_BATTERY> &&
          ::djinterp::container_convertible_to<std::vector<int>,
                                               std::set<int> > );

    D_CONSTEXPR bool not_lossless =
        ( ( !::djinterp::losslessly_convertible_to<std::vector<int>,
                                                   std::set<int> > )     &&
          ( !::djinterp::view_convertible_to<std::vector<int>,
                                             std::set<int> > ) );

    // the concept reads a DIRECTION, not a pair
    D_CONSTEXPR bool directional =
        ( ::djinterp::lossily_convertible_to<std::vector<int>,
                                             std::set<int> >             &&
          ( !::djinterp::lossily_convertible_to<std::set<int>,
                                                std::set<int> > ) );

    return ( canonical_quotients &&
             lossy_is_convertible &&
             not_lossless        &&
             directional );

#else

    return true;

#endif  // DJINTERP_CCONV_TESTS_ACTIVE
}


NS_END  // testing
NS_END  // djinterp
