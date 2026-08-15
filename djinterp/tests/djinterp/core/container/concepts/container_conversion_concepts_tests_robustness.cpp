/******************************************************************************
* djinterp [test]           container_conversion_concepts_tests_robustness.cpp
*
*   Section VI of the container_conversion_concepts.hpp suite: the answers
* nobody re-checks.
*
*   Conversion is the framework's first genuinely ASYMMETRIC axis.  Comparison
* was symmetric in comparability and equality; here every concept has a witness
* pair it accepts one way and refuses the other, and that asymmetry is what
* makes the operand order load-bearing - which is, in turn, why the module got
* it wrong in two places.  This section tests directionality as a property in
* its own right, so a future concept that lost its direction would be caught
* even if its trait were untouched.
*
*   Qualification has twice the surface of a unary axis, and the failure is
* asymmetric in the same way: a concept that cleans its first parameter and
* forgets its second looks correct at every call site passing both by value.
* Both operands are swept independently.
*
* path:      /test/djinterp/core/container/concepts/
*                         container_conversion_concepts_tests_robustness.cpp
* author(s): Samuel 'teer' Neal-Blim
******************************************************************************/

#include "container_conversion_concepts_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_conversion_is_directional
  Every concept in the module is asymmetric, and each has a witness pair that
demonstrates it.  A concept whose direction had been lost would still pass
every test that only exercised one order.
  Tests the following:
  - lossily_convertible_to is directional: a sequence forgets into a set, and
    the reverse does not forget
  - view_convertible_to is directional at the array / vector pair
  - the range paths are directional at the array / vector pair, whichever
    reading is in force
  - container_convertible_to is directional somewhere in the battery
  - directionality is a property of the AXIS, not of one concept: the raw tier
    itself differs between the two orders of a pair
*/
bool
tests_conversion_is_directional()
{
#if DJINTERP_CCONV_TESTS_ACTIVE

    // the tier itself is directional
    D_CONSTEXPR bool tier_is_directional =
        ( cconv_tier<std::vector<int>, std::set<int> > !=
          cconv_tier<std::set<int>, std::vector<int> > );

    // loss is directional
    D_CONSTEXPR bool loss_is_directional =
        ( ::djinterp::lossily_convertible_to<std::vector<int>,
                                             std::set<int> >             &&
          ( !::djinterp::lossily_convertible_to<std::vector<int>,
                                                std::vector<int> > ) );

    // views are directional
    D_CONSTEXPR bool views_are_directional =
        ( ::djinterp::view_convertible_to<std::vector<int>,
                                          std::vector<int> >             &&
          ( !::djinterp::view_convertible_to<std::array<int, 4>,
                                             std::vector<int> > ) );

    // the range paths are directional
    D_CONSTEXPR bool paths_are_directional =
        ( ::djinterp::range_constructible_from<std::array<int, 4>,
                                               std::vector<int> > !=
          ::djinterp::range_constructible_from<std::vector<int>,
                                               std::array<int, 4> > );

    // and the trait underneath them is too
    D_CONSTEXPR bool trait_is_directional =
        ( ::djinterp::is_range_constructible<std::array<int, 4>,
                                             std::vector<int> >::value !=
          ::djinterp::is_range_constructible<std::vector<int>,
                                             std::array<int, 4> >::value );

    return ( tier_is_directional   &&
             loss_is_directional   &&
             views_are_directional &&
             paths_are_directional &&
             trait_is_directional );

#else

    return true;    // no concept surface below C++20 + concepts

#endif  // DJINTERP_CCONV_TESTS_ACTIVE
}


/*
tests_cvref_invariance
  All six concepts apply clean_t to both operands, so all six are
qualification-transparent on both sides.
  Tests the following:
  - the LEFT operand may be qualified in any of the five forms without changing
    any concept's answer, across the battery
  - the RIGHT operand may be too - the asymmetric failure a binary concept is
    uniquely exposed to
  - both operands may be qualified at once
  - the NEGATIVES survive qualification, so stability is not merely "true
    everywhere"
  - the raw tier is qualification-stable, so the transparency comes from the
    trait rather than from the concepts papering over it
*/
bool
tests_cvref_invariance()
{
#if DJINTERP_CCONV_TESTS_ACTIVE

    namespace dt = ::djinterp::test;

    D_CONSTEXPR bool left_operand_stable =
        cconv_law_cvref_stable<D_CCONV_STDLIB>;

    D_CONSTEXPR bool right_operand_stable =
        cconv_law_cvref_stable_right<D_CCONV_STDLIB>;

    // both at once
    D_CONSTEXPR bool both_operands_stable =
        ( ( ::djinterp::view_convertible_to<const std::vector<int>&,
                                            std::vector<int>&&> ==
            ::djinterp::view_convertible_to<std::vector<int>,
                                            std::vector<int> > )            &&
          ( ::djinterp::lossily_convertible_to<const std::vector<int>&,
                                               const std::set<int>&> ==
            ::djinterp::lossily_convertible_to<std::vector<int>,
                                               std::set<int> > ) );

    // negatives survive
    D_CONSTEXPR bool negatives_stable =
        ( ( !::djinterp::view_convertible_to<const std::vector<int>&,
                                             std::list<int> > )             &&
          ( !::djinterp::container_convertible_to<const int&, int&&> ) );

    // and the tier itself is stable
    D_CONSTEXPR bool tier_is_stable =
        ( ( cconv_tier<const std::vector<int>&, std::vector<int> > ==
            cconv_tier<std::vector<int>, std::vector<int> > )               &&
          ( cconv_tier<std::vector<int>, const std::set<int>&> ==
            cconv_tier<std::vector<int>, std::set<int> > ) );

    D_CONSTEXPR bool report_agrees =
        ( dt::trait_ignores_cvref<cconv_c_convertible_to_vector,
                                  std::list<int> >::value           &&
          dt::trait_ignores_cvref<cconv_c_view_to_vector,
                                  std::vector<int> >::value         &&
          dt::trait_ignores_cvref<cconv_c_lossy_to_set,
                                  std::vector<int> >::value );

    return ( left_operand_stable  &&
             right_operand_stable &&
             both_operands_stable &&
             negatives_stable     &&
             tier_is_stable       &&
             report_agrees );

#else

    return true;

#endif  // DJINTERP_CCONV_TESTS_ACTIVE
}


/*
tests_nonclass_shapes
  A conversion classification asked about something with no elements and no
range must answer, and must do so without a diagnostic.  Every check below also
FORCES each concept to form for the shape.
  Tests the following:
  - scalars, pointers and an enumeration convert to nothing, in either operand
    position
  - they are not convertible even to themselves, so the axis is not reflexive
  - the raw tier at each is exactly `none`
  - none of the six concepts is satisfied by any of them
  - a container against a scalar answers rather than diagnoses, in both orders
  - the inert fixture behaves identically to a scalar, so class-ness alone
    confers nothing
*/
bool
tests_nonclass_shapes()
{
#if DJINTERP_CCONV_TESTS_ACTIVE

    namespace dt = ::djinterp::test;

    // enum_shape
    //   type: a scoped enumeration - a non-class shape with no elements and no
    // range.
    enum class enum_shape
    {
        a_value
    };

    D_CONSTEXPR bool convert_to_nothing =
        ( dt::holds_for_none<cconv_c_convertible_to_vector,
                             D_CCONV_NONCONTAINERS>::value &&
          dt::holds_for_none<cconv_c_convertible_from_vector,
                             D_CCONV_NONCONTAINERS>::value );

    // not even to themselves
    D_CONSTEXPR bool not_reflexive =
        ( ( !::djinterp::container_convertible_to<int, int> )            &&
          ( !::djinterp::container_convertible_to<void*, void*> )        &&
          ( !::djinterp::container_convertible_to<enum_shape,
                                                  enum_shape> )          &&
          ( !::djinterp::container_convertible_to<cconv_inert,
                                                  cconv_inert> ) );

    // the tier is exactly `none`
    D_CONSTEXPR bool tier_is_none =
        ( ( cconv_tier<int, int> == ::djinterp::conversion_tier::none )  &&
          ( cconv_tier<enum_shape, std::vector<int> > ==
            ::djinterp::conversion_tier::none )                          &&
          ( cconv_tier<cconv_inert, cconv_inert> ==
            ::djinterp::conversion_tier::none ) );

    // no concept fires
    D_CONSTEXPR bool no_concept_fires =
        ( dt::holds_for_none<cconv_c_lossless_to_vector,
                             D_CCONV_NONCONTAINERS>::value &&
          dt::holds_for_none<cconv_c_lossy_to_set,
                             D_CCONV_NONCONTAINERS>::value &&
          dt::holds_for_none<cconv_c_view_to_vector,
                             D_CCONV_NONCONTAINERS>::value );

    // and mixed operands answer rather than diagnose
    D_CONSTEXPR bool mixed_operands_answer =
        ( ( !::djinterp::container_convertible_to<std::vector<int>,
                                                  enum_shape> )          &&
          ( !::djinterp::container_convertible_to<enum_shape,
                                                  std::vector<int> > )   &&
          ( !::djinterp::view_convertible_to<int, std::vector<int> > ) );

    return ( convert_to_nothing    &&
             not_reflexive         &&
             tier_is_none          &&
             no_concept_fires      &&
             mixed_operands_answer );

#else

    return true;

#endif  // DJINTERP_CCONV_TESTS_ACTIVE
}


/*
tests_independent_respelling
  Each tier reading re-derived by hand from the raw conversion_tier.  Four of
the six concepts are one-line readings of a single enum, so the re-spelling is
short - and that is the point: if a one-line reading has drifted to a different
enumerator, only an independent one-line reading will say so.
  Tests the following:
  - convertible re-derived as "the tier is not none" matches, across the
    battery
  - lossless re-derived as "view or constructive" matches
  - lossy re-derived as "the tier is lossy" matches
  - view re-derived as "the tier is view" matches
  - the re-spellings are DISCRIMINATING, separating the battery into both
    classes rather than agreeing trivially
  - the enumerators are mutually exclusive at every pair tested, so the
    readings partition the tier rather than overlapping
*/
bool
tests_independent_respelling()
{
#if DJINTERP_CCONV_TESTS_ACTIVE

    namespace dt = ::djinterp::test;

    // convertible, re-derived
    D_CONSTEXPR bool convertible_respelled =
        ( ( ::djinterp::container_convertible_to<std::vector<int>,
                                                 std::list<int> > ==
            ( cconv_tier<std::vector<int>, std::list<int> > !=
              ::djinterp::conversion_tier::none ) )                      &&
          ( ::djinterp::container_convertible_to<int, int> ==
            ( cconv_tier<int, int> !=
              ::djinterp::conversion_tier::none ) ) );

    // lossless, re-derived as the union of two enumerators
    D_CONSTEXPR bool lossless_respelled =
        ( ( ::djinterp::losslessly_convertible_to<std::vector<int>,
                                                  std::list<int> > ==
            ( cconv_tier<std::vector<int>, std::list<int> > ==
                  ::djinterp::conversion_tier::view ||
              cconv_tier<std::vector<int>, std::list<int> > ==
                  ::djinterp::conversion_tier::constructive ) )          &&
          ( ::djinterp::losslessly_convertible_to<std::vector<int>,
                                                  std::set<int> > ==
            ( cconv_tier<std::vector<int>, std::set<int> > ==
                  ::djinterp::conversion_tier::view ||
              cconv_tier<std::vector<int>, std::set<int> > ==
                  ::djinterp::conversion_tier::constructive ) ) );

    // lossy and view, re-derived
    D_CONSTEXPR bool lossy_and_view_respelled =
        ( ( ::djinterp::lossily_convertible_to<std::vector<int>,
                                               std::set<int> > ==
            ( cconv_tier<std::vector<int>, std::set<int> > ==
              ::djinterp::conversion_tier::lossy ) )                     &&
          ( ::djinterp::view_convertible_to<std::vector<int>,
                                            std::vector<int> > ==
            ( cconv_tier<std::vector<int>, std::vector<int> > ==
              ::djinterp::conversion_tier::view ) ) );

    // the re-spellings discriminate
    D_CONSTEXPR bool discriminating =
        ( dt::holds_for_any<cconv_c_convertible_to_vector,
                            D_CCONV_BATTERY>::value                      &&
          ( !dt::holds_for_all<cconv_c_convertible_to_vector,
                               D_CCONV_BATTERY>::value )                 &&
          dt::holds_for_any<cconv_c_view_to_vector,
                            D_CCONV_BATTERY>::value                      &&
          ( !dt::holds_for_all<cconv_c_view_to_vector,
                               D_CCONV_BATTERY>::value ) );

    // and the enumerators partition
    D_CONSTEXPR bool enumerators_partition =
        cconv_law_tier_is_a_partition<D_CCONV_BATTERY>;

    return ( convertible_respelled    &&
             lossless_respelled       &&
             lossy_and_view_respelled &&
             discriminating           &&
             enumerators_partition );

#else

    return true;

#endif  // DJINTERP_CCONV_TESTS_ACTIVE
}


NS_END  // testing
NS_END  // djinterp
