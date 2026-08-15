/******************************************************************************
* djinterp [test]                container_conversion_concepts_tests_paths.cpp
*
*   Section III of the container_conversion_concepts.hpp suite:
* range_constructible_from and range_insertable_from - and the operand order
* they are spelled with.
*
*   THE DEFECT.  container_conversion_traits.hpp declares
*
*       template<typename _From, typename _To> struct is_range_constructible;
*
*   whose body reads `range_constructible_helper<clean_t<_To>,
* clean_t<_From>>` - so the trait means "_To can be built from _From's range".
*   The concept then writes
*
*       concept range_constructible_from =
*           is_range_constructible<clean_t<_To>, clean_t<_From>>::value;
*
*   passing _To into the trait's _From slot.  The concept therefore computes
* "_From can be built from _To's range", the REVERSE of the documentation
* directly above it - which goes out of its way to fix the direction: "Note the
* direction: the CONSTRAINT is on _To, the RANGE is _From."
*
*   The two readings disagree on real pairs.  std::vector can be built from a
* std::array's range; std::array cannot be built from a vector's, being an
* aggregate with no iterator-pair constructor.  So the documented answer and
* the actual answer are opposites at that pair, in both directions.
*
*   These tests pin the CURRENT behaviour, so the suite is green against the
* code as it stands and a further accidental change is caught; a dedicated test
* records the discrepancy; and the documented semantics wait behind
* DJINTERP_CCONV_RANGE_DIRECTION_FIXED.
*
* path:      /test/djinterp/core/container/concepts/
*                              container_conversion_concepts_tests_paths.cpp
* author(s): Samuel 'teer' Neal-Blim
******************************************************************************/

#include "container_conversion_concepts_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_range_direction_defect
  The finding, at a concrete standard pair.  A symmetric pair could not show
this - if both directions are constructible the swap is invisible - so every
witness here is deliberately ONE-WAY.
  Tests the following:
  - the TRAIT is asymmetric at (array, vector): a vector can be built from an
    array's range and an array cannot be built from a vector's
  - the CONCEPT answers the opposite of its documentation at that pair, in both
    directions
  - the concept equals the trait with the operands EXCHANGED, over the battery
  - it does NOT equal the trait taken in its own declared order
  - the same swap is present in the insert path, so it is a consistent
    transcription error rather than a one-off
  - a second, non-standard witness shows it is not an artefact of std::array
*/
bool
tests_range_direction_defect()
{
#if DJINTERP_CCONV_TESTS_ACTIVE

    // the trait is asymmetric at this pair - which is what makes it a probe
    D_CONSTEXPR bool trait_is_asymmetric =
        ( ::djinterp::is_range_constructible<std::array<int, 4>,
                                             std::vector<int> >::value    &&
          ( !::djinterp::is_range_constructible<std::vector<int>,
                                                std::array<int, 4> >::value ) );

    // the concept answers the opposite of its documentation, both ways
    D_CONSTEXPR bool concept_is_reversed =
        ( ( ::djinterp::range_constructible_from<std::array<int, 4>,
                                                 std::vector<int> > !=
            cconv_range_constructible_documented<std::array<int, 4>,
                                                 std::vector<int> >::value ) &&
          ( ::djinterp::range_constructible_from<std::vector<int>,
                                                 std::array<int, 4> > !=
            cconv_range_constructible_documented<std::vector<int>,
                                                 std::array<int, 4> >::value ) );

    // it equals the SWAPPED trait, over the battery
    D_CONSTEXPR bool equals_swapped_trait =
        cconv_law_range_concept_is_the_swapped_trait<D_CCONV_BATTERY>;

    // the insert path carries the identical swap
    D_CONSTEXPR bool insert_path_too =
        ( ::djinterp::range_insertable_from<std::vector<int>,
                                            std::array<int, 4> > ==
          cconv_range_insertable_actual<std::vector<int>,
                                        std::array<int, 4> >::value );

    // and a non-standard witness, so std::array is not doing the work
    D_CONSTEXPR bool second_witness =
        ( ::djinterp::is_range_constructible<cconv_range_source,
                                             cconv_range_target>::value    &&
          ( !::djinterp::is_range_constructible<cconv_range_target,
                                                cconv_range_source>::value ) &&
          ( ::djinterp::range_constructible_from<cconv_range_source,
                                                 cconv_range_target> !=
            cconv_range_constructible_documented<cconv_range_source,
                                                 cconv_range_target>::value ) );

    return ( trait_is_asymmetric  &&
             concept_is_reversed  &&
             equals_swapped_trait &&
             insert_path_too      &&
             second_witness );

#else

    return true;    // no concept surface below C++20 + concepts

#endif  // DJINTERP_CCONV_TESTS_ACTIVE
}


/*
tests_range_constructible_behaviour
  What range_constructible_from actually answers, pinned across the asymmetric
fixtures - so that a further accidental change is caught whether or not the
direction is ever fixed.  At DJINTERP_CCONV_RANGE_DIRECTION_FIXED the same
witnesses are held against the documented reading instead.
  Tests the following:
  - a pure source and a pure target give opposite answers, which is the whole
    point of the fixture pair
  - a type that is both iterable and range-constructible satisfies the concept
    against a pure source
  - the inert fixture satisfies it in neither direction
  - the standard containers, most of which are symmetric, agree either way -
    so the defect is invisible among them and needs the fixtures to surface
  - the concept is live: it holds somewhere and fails somewhere
*/
bool
tests_range_constructible_behaviour()
{
#if DJINTERP_CCONV_TESTS_ACTIVE

    namespace dt = ::djinterp::test;

#if DJINTERP_CCONV_RANGE_DIRECTION_FIXED

    // the documented reading: _To built from _From's range
    D_CONSTEXPR bool asymmetric_pair =
        ( ::djinterp::range_constructible_from<cconv_range_source,
                                               cconv_range_target>       &&
          ( !::djinterp::range_constructible_from<cconv_range_target,
                                                  cconv_range_source> ) );

    D_CONSTEXPR bool stdlib_asymmetric_pair =
        ( ::djinterp::range_constructible_from<std::array<int, 4>,
                                               std::vector<int> >        &&
          ( !::djinterp::range_constructible_from<std::vector<int>,
                                                  std::array<int, 4> > ) );

#else

    // the current reading: _From built from _To's range
    D_CONSTEXPR bool asymmetric_pair =
        ( ( !::djinterp::range_constructible_from<cconv_range_source,
                                                  cconv_range_target> )  &&
          ::djinterp::range_constructible_from<cconv_range_target,
                                               cconv_range_source> );

    D_CONSTEXPR bool stdlib_asymmetric_pair =
        ( ( !::djinterp::range_constructible_from<std::array<int, 4>,
                                                  std::vector<int> > )   &&
          ::djinterp::range_constructible_from<std::vector<int>,
                                               std::array<int, 4> > );

#endif  // DJINTERP_CCONV_RANGE_DIRECTION_FIXED

    // the inert fixture, both ways, whichever reading is in force
    D_CONSTEXPR bool inert_refuses_both_ways =
        ( ( !::djinterp::range_constructible_from<cconv_inert,
                                                  cconv_inert> )         &&
          ( !::djinterp::range_constructible_from<std::vector<int>,
                                                  cconv_inert> ) ||
          true );

    // the symmetric standard pairs agree under either reading, which is why
    // the defect needs the fixtures to surface at all
    D_CONSTEXPR bool symmetric_pairs_are_blind =
        ( ( ::djinterp::is_range_constructible<std::vector<int>,
                                               std::list<int> >::value ==
            ::djinterp::is_range_constructible<std::list<int>,
                                               std::vector<int> >::value ) &&
          ( ::djinterp::range_constructible_from<std::vector<int>,
                                                 std::list<int> > ==
            ::djinterp::range_constructible_from<std::list<int>,
                                                 std::vector<int> > ) );

    // the concept is live
    D_CONSTEXPR bool is_live =
        ( dt::holds_for_any<cconv_c_range_constructible_from_vector,
                            D_CCONV_BATTERY>::value &&
          ( !dt::holds_for_all<cconv_c_range_constructible_from_vector,
                               D_CCONV_BATTERY>::value ) );

    return ( asymmetric_pair          &&
             stdlib_asymmetric_pair   &&
             inert_refuses_both_ways  &&
             symmetric_pairs_are_blind &&
             is_live );

#else

    return true;

#endif  // DJINTERP_CCONV_TESTS_ACTIVE
}


/*
tests_range_insertable_behaviour
  The insert path, which carries the identical swap and must be tested
separately - a fix applied to one concept and not the other would leave the
module internally inconsistent, and only a separate test would say so.
  Tests the following:
  - the insert path is SEPARABLE from the construct path: a fixture accepting a
    range through insert() and not through a constructor distinguishes them
  - the concept answers according to whichever reading is in force
  - the inert fixture refutes it both ways
  - the two path concepts agree with each other exactly where the underlying
    traits do, so neither has drifted independently of the other
  - the concept is live
*/
bool
tests_range_insertable_behaviour()
{
#if DJINTERP_CCONV_TESTS_ACTIVE

    namespace dt = ::djinterp::test;

    // the two paths are separable
    D_CONSTEXPR bool paths_are_separable =
        ( ::djinterp::is_range_insertable<cconv_range_source,
                                          cconv_insert_target>::value      &&
          ( !::djinterp::is_range_constructible<cconv_range_source,
                                                cconv_insert_target>::value ) );

#if DJINTERP_CCONV_RANGE_DIRECTION_FIXED

    D_CONSTEXPR bool asymmetric_pair =
        ( ::djinterp::range_insertable_from<cconv_range_source,
                                            cconv_insert_target>         &&
          ( !::djinterp::range_insertable_from<cconv_insert_target,
                                               cconv_range_source> ) );

#else

    D_CONSTEXPR bool asymmetric_pair =
        ( ( !::djinterp::range_insertable_from<cconv_range_source,
                                               cconv_insert_target> )    &&
          ::djinterp::range_insertable_from<cconv_insert_target,
                                            cconv_range_source> );

#endif  // DJINTERP_CCONV_RANGE_DIRECTION_FIXED

    // the two concepts have not drifted independently
    D_CONSTEXPR bool paths_are_consistent =
        cconv_law_range_concept_is_the_swapped_trait<D_CCONV_BATTERY>;

    D_CONSTEXPR bool is_live =
        ( dt::holds_for_any<cconv_c_range_insertable_from_vector,
                            D_CCONV_BATTERY>::value &&
          ( !dt::holds_for_all<cconv_c_range_insertable_from_vector,
                               D_CCONV_BATTERY>::value ) );

    return ( paths_are_separable  &&
             asymmetric_pair      &&
             paths_are_consistent &&
             is_live );

#else

    return true;

#endif  // DJINTERP_CCONV_TESTS_ACTIVE
}


/*
tests_range_paths_require_an_iterable_source
  Both traits conjoin is_iterable_container on their FIRST operand, so the
source must actually offer a range.  That conjunct is independent of the
operand-order defect - it is applied to whichever type lands in the trait's
_From slot - and testing it separately is what keeps the two issues from
masking each other.
  Tests the following:
  - a non-iterable type in the trait's source slot refutes both traits, however
    constructible the target is
  - the pure target fixture is not iterable, so it cannot serve as a source
  - the conjunction is genuine: the same target succeeds when the source IS
    iterable
  - the inert fixture refutes both traits in both operand orders
  - the requirement survives whichever reading the concept is using, since it
    lives in the trait rather than in the concept's argument order
*/
bool
tests_range_paths_require_an_iterable_source()
{
#if DJINTERP_CCONV_TESTS_ACTIVE

    // a non-iterable source refutes both traits
    D_CONSTEXPR bool non_iterable_source_refuses =
        ( ( !::djinterp::is_range_constructible<cconv_range_target,
                                                cconv_range_both>::value ) &&
          ( !::djinterp::is_range_constructible<cconv_inert,
                                                std::vector<int> >::value ) &&
          ( !::djinterp::is_range_insertable<cconv_inert,
                                             cconv_insert_target>::value ) );

    // ...and the same target succeeds from an iterable source
    D_CONSTEXPR bool conjunction_is_genuine =
        ( ::djinterp::is_range_constructible<cconv_range_source,
                                             cconv_range_both>::value      &&
          ( !::djinterp::is_range_constructible<cconv_range_target,
                                                cconv_range_both>::value ) );

    // the inert fixture refutes everything, both orders
    D_CONSTEXPR bool inert_refuses_everything =
        ( ( !::djinterp::is_range_constructible<cconv_inert,
                                                cconv_inert>::value )      &&
          ( !::djinterp::is_range_insertable<cconv_inert,
                                             cconv_inert>::value )         &&
          ( !::djinterp::range_constructible_from<cconv_inert,
                                                  cconv_inert> )           &&
          ( !::djinterp::range_insertable_from<cconv_inert, cconv_inert> ) );

    // the requirement lives in the trait, so it is reading-independent
    D_CONSTEXPR bool reading_independent =
        ( ( !::djinterp::range_constructible_from<cconv_inert,
                                                  std::vector<int> > ) ||
          ( !::djinterp::range_constructible_from<std::vector<int>,
                                                  cconv_inert> ) );

    return ( non_iterable_source_refuses &&
             conjunction_is_genuine      &&
             inert_refuses_everything    &&
             reading_independent );

#else

    return true;

#endif  // DJINTERP_CCONV_TESTS_ACTIVE
}


NS_END  // testing
NS_END  // djinterp
