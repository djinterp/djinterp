/*******************************************************************************
* djinterp [core]                  concurrency_strategy_tags_tests_signature.cpp
*
*
* path:      /tests/djinterp/core/sync/concurrency_strategy_tags_tests_signature.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp [test]         concurrency_strategy_tags_tests_signature.cpp
//   Section II (e): the four coordinates AS A VOCABULARY.
//
//   The previous four sections each took one enum apart.  This one is about
// what the header says they are TOGETHER: "the monograph fixes a container's
// place on the concurrency axis as a PAIR (progress grade, arity), together
// with the ITERATION overlap-semantics it offers and the RECLAMATION
// discipline it owes."  Four coordinates, one signature.
//
//   The header carries only the vocabulary; the `concurrency_signature`
// aggregate and the strategy -> signature MAPPING live in
// concurrency_strategy_traits.hpp, one layer up.  The layering claim is that
// the vocabulary is complete enough to be used without that layer - so this
// section builds a signature aggregate and a full seven-tag mapping out of
// nothing but the header under test, and then checks that no trait-header
// symbol was needed to do it.  The mapping here is LOCAL and illustrative; it
// is not the canonical one, and it is not asserted to agree with it.

// djinterp
#include "concurrency_strategy_tags_tests.hpp"


NS_DJINTERP
NS_TESTING

namespace
{
    // named_signature
    //   function: every coordinate of the signature resolves to a named value
    // of its enum (no field holds a cast-in stranger).
    D_INLINE bool
    named_signature(
        const mock_signature& _s
    )
    {
        return ( (progress_name(_s.progress)       != nullptr) &&
                 (arity_name(_s.arity)             != nullptr) &&
                 (iteration_name(_s.iteration)     != nullptr) &&
                 (reclamation_name(_s.reclamation) != nullptr) );
    }
}

/*
tests_the_four_enums_are_distinct_types
  four coordinates, four types.
  Tests the following:
  - all six unordered pairs of the four enums are different types
  - so a signature cannot silently hold its arity where its progress belongs
*/
bool
tests_the_four_enums_are_distinct_types()
{
    D_CONSTEXPR bool distinct =
        ( !std::is_same<concurrency_progress,
                        concurrency_arity>::value              &&
          !std::is_same<concurrency_progress,
                        iteration_consistency>::value          &&
          !std::is_same<concurrency_progress,
                        reclamation_obligation>::value         &&
          !std::is_same<concurrency_arity,
                        iteration_consistency>::value          &&
          !std::is_same<concurrency_arity,
                        reclamation_obligation>::value         &&
          !std::is_same<iteration_consistency,
                        reclamation_obligation>::value );

    static_assert(distinct, "the four coordinates must be four types");

    return distinct;
}

/*
tests_the_enums_do_not_interconvert
  no coordinate converts to another.
  Tests the following:
  - std::is_convertible is false for all twelve ordered pairs
  - scoped enums do not convert to their underlying type, and so cannot reach
    each other THROUGH it - which is the conversion an unscoped enum would have
    allowed, silently, in both directions
*/
bool
tests_the_enums_do_not_interconvert()
{
    D_CONSTEXPR bool progress_holds =
        ( !std::is_convertible<concurrency_progress,
                               concurrency_arity>::value      &&
          !std::is_convertible<concurrency_progress,
                               iteration_consistency>::value  &&
          !std::is_convertible<concurrency_progress,
                               reclamation_obligation>::value );

    D_CONSTEXPR bool arity_holds =
        ( !std::is_convertible<concurrency_arity,
                               concurrency_progress>::value   &&
          !std::is_convertible<concurrency_arity,
                               iteration_consistency>::value  &&
          !std::is_convertible<concurrency_arity,
                               reclamation_obligation>::value );

    D_CONSTEXPR bool iteration_holds =
        ( !std::is_convertible<iteration_consistency,
                               concurrency_progress>::value   &&
          !std::is_convertible<iteration_consistency,
                               concurrency_arity>::value      &&
          !std::is_convertible<iteration_consistency,
                               reclamation_obligation>::value );

    D_CONSTEXPR bool reclamation_holds =
        ( !std::is_convertible<reclamation_obligation,
                               concurrency_progress>::value   &&
          !std::is_convertible<reclamation_obligation,
                               concurrency_arity>::value      &&
          !std::is_convertible<reclamation_obligation,
                               iteration_consistency>::value );

    static_assert(progress_holds && arity_holds,
                  "the coordinates must not interconvert");

    return ( progress_holds  &&
             arity_holds     &&
             iteration_holds &&
             reclamation_holds );
}

/*
tests_coincident_values_are_not_coincident_types
  four zeros, four different meanings.
  Tests the following:
  - sequential, swmr, inconsistent and none ALL carry the underlying value 0,
    and lock_free, reference_count and snapshot all carry 2
  - and yet none of those enumerators has the type of any other
  - the values coincide; the coordinates do not.  Comparing two of them
    directly would not compile, which is the whole point of the scoping - so
    what is asserted here is the pair of facts that makes that protection
    matter
*/
bool
tests_coincident_values_are_not_coincident_types()
{
    D_CONSTEXPR bool zeros_coincide =
        ( (underlying(concurrency_progress::sequential)      == 0) &&
          (underlying(concurrency_arity::swmr)               == 0) &&
          (underlying(iteration_consistency::inconsistent)   == 0) &&
          (underlying(reclamation_obligation::none)          == 0) );

    D_CONSTEXPR bool twos_coincide =
        ( (underlying(concurrency_progress::lock_free)       == 2) &&
          (underlying(iteration_consistency::snapshot)       == 2) &&
          (underlying(reclamation_obligation::reference_count) == 2) );

    D_CONSTEXPR bool but_the_types_differ =
        ( !std::is_same<decltype(concurrency_progress::sequential),
                        decltype(concurrency_arity::swmr)>::value       &&
          !std::is_same<decltype(concurrency_progress::lock_free),
                        decltype(iteration_consistency::snapshot)>::value &&
          !std::is_same<decltype(iteration_consistency::inconsistent),
                        decltype(reclamation_obligation::none)>::value );

    static_assert(zeros_coincide && but_the_types_differ,
                  "coincident values must not make coincident coordinates");

    return ( zeros_coincide      &&
             twos_coincide       &&
             but_the_types_differ );
}

/*
tests_the_enums_are_not_arithmetic
  a coordinate is a name, not a number.
  Tests the following:
  - none of the four is integral or arithmetic
  - none converts to bool
  - all four ARE enums
  - so a coordinate cannot be added, multiplied, or tested for truth by
    accident; it must be cast out to its underlying type on purpose
*/
bool
tests_the_enums_are_not_arithmetic()
{
    D_CONSTEXPR bool are_enums =
        ( std::is_enum<concurrency_progress>::value      &&
          std::is_enum<concurrency_arity>::value         &&
          std::is_enum<iteration_consistency>::value     &&
          std::is_enum<reclamation_obligation>::value );

    D_CONSTEXPR bool not_arithmetic =
        ( !std::is_arithmetic<concurrency_progress>::value      &&
          !std::is_arithmetic<concurrency_arity>::value         &&
          !std::is_arithmetic<iteration_consistency>::value     &&
          !std::is_arithmetic<reclamation_obligation>::value    &&
          !std::is_integral<concurrency_progress>::value        &&
          !std::is_integral<concurrency_arity>::value           &&
          !std::is_integral<iteration_consistency>::value       &&
          !std::is_integral<reclamation_obligation>::value );

    D_CONSTEXPR bool no_truth_value =
        ( !std::is_convertible<concurrency_progress, bool>::value   &&
          !std::is_convertible<concurrency_arity, bool>::value      &&
          !std::is_convertible<iteration_consistency, bool>::value  &&
          !std::is_convertible<reclamation_obligation, bool>::value );

    static_assert(are_enums && not_arithmetic && no_truth_value,
                  "a coordinate must be a name, not a number");

    return ( are_enums      &&
             not_arithmetic &&
             no_truth_value );
}

/*
tests_the_vocabulary_composes_into_a_signature
  the four coordinates bundle into one value.
  Tests the following:
  - an aggregate of the four enums is constructible from four enumerators, and
    reads back field by field
  - it is trivially copyable and standard-layout, and costs four ints
  - which is the shape concurrency_strategy_traits.hpp's concurrency_signature
    has - built here from the header under test ALONE
*/
bool
tests_the_vocabulary_composes_into_a_signature()
{
    const mock_signature rcu_like =
        { concurrency_progress::lock_free,
          concurrency_arity::mwmr,
          iteration_consistency::snapshot,
          reclamation_obligation::grace_period };

    const bool reads_back =
        ( (rcu_like.progress    == concurrency_progress::lock_free)     &&
          (rcu_like.arity       == concurrency_arity::mwmr)             &&
          (rcu_like.iteration   == iteration_consistency::snapshot)     &&
          (rcu_like.reclamation == reclamation_obligation::grace_period) );

    D_CONSTEXPR bool well_shaped =
        ( std::is_trivially_copyable<mock_signature>::value &&
          std::is_standard_layout<mock_signature>::value    &&
          (sizeof(mock_signature) == (4 * sizeof(int))) );

    static_assert(well_shaped,
                  "four coordinates must bundle into four ints");

    return ( reads_back &&
             well_shaped &&
             named_signature(rcu_like) );
}

/*
tests_a_strategy_to_signature_mapping_is_expressible
  every strategy's coordinates can be NAMED in this vocabulary.
  Tests the following:
  - a mapping keyed on each of the seven tags yields a signature whose every
    field is a named value of its enum
  - the strategy that synchronizes nothing lands on the floor of all four
    coordinates (sequential / swmr / inconsistent / none)
  - the RCU strategy's discipline is the grace period, and the hazard-pointer
    strategy's is the hazard slot - the two the enum names for them
  The mapping used here is LOCAL and illustrative.  The canonical one lives in
  concurrency_strategy_traits.hpp; this test asserts only that nothing in the
  vocabulary PREVENTS a strategy's coordinates from being written down, which
  is the header's job.
*/
bool
tests_a_strategy_to_signature_mapping_is_expressible()
{
    const bool every_tag_maps =
        ( named_signature(mock_signature_of<none_strategy_tag>::value())   &&
          named_signature(mock_signature_of<locked_strategy_tag>::value()) &&
          named_signature(mock_signature_of<cow_strategy_tag>::value())    &&
          named_signature(mock_signature_of<rcu_strategy_tag>::value())    &&
          named_signature(mock_signature_of<atomic_strategy_tag>::value()) &&
          named_signature(mock_signature_of<hazard_strategy_tag>::value()) &&
          named_signature(mock_signature_of<hybrid_strategy_tag>::value()) );

    D_CONSTEXPR mock_signature unsynchronized =
        mock_signature_of<none_strategy_tag>::value();

    D_CONSTEXPR bool none_sits_on_the_floor =
        ( (unsynchronized.progress ==
           concurrency_progress::sequential)                   &&
          (unsynchronized.arity == concurrency_arity::swmr)    &&
          (unsynchronized.iteration ==
           iteration_consistency::inconsistent)                &&
          (unsynchronized.reclamation ==
           reclamation_obligation::none) );

    D_CONSTEXPR bool the_disciplines_are_named_for_their_strategies =
        ( (mock_signature_of<rcu_strategy_tag>::value().reclamation ==
           reclamation_obligation::grace_period) &&
          (mock_signature_of<hazard_strategy_tag>::value().reclamation ==
           reclamation_obligation::hazard) );

    static_assert(none_sits_on_the_floor,
                  "the unsynchronized strategy must sit on the floor of every "
                  "coordinate");

    return ( every_tag_maps         &&
             none_sits_on_the_floor &&
             the_disciplines_are_named_for_their_strategies );
}

/*
tests_the_signature_is_a_compile_time_value
  a signature is a constant expression, not a runtime lookup.
  Tests the following:
  - a signature comes out of the mapping inside a constexpr initializer
  - its fields are readable in a static_assert
  - so a container's signature can gate a static_assert or select a
    specialization, which is what the trait layer above will want of it
*/
bool
tests_the_signature_is_a_compile_time_value()
{
    D_CONSTEXPR mock_signature hazard_like =
        mock_signature_of<hazard_strategy_tag>::value();

    static_assert(hazard_like.progress >= concurrency_progress::lock_free,
                  "the hazard strategy must be at least lock-free");

    static_assert(hazard_like.reclamation == reclamation_obligation::hazard,
                  "the hazard strategy must owe the hazard discipline");

    D_CONSTEXPR bool readable_at_compile_time =
        ( (hazard_like.arity == concurrency_arity::mwmr) &&
          (hazard_like.iteration >
           iteration_consistency::inconsistent) );

    return readable_at_compile_time;
}

/*
tests_every_enumerator_is_reachable
  the section's coverage sweep: all fifteen enumerators, in one place.
  Tests the following:
  - the 4 progress grades, 2 arities, 3 overlap semantics and 6 reclamation
    obligations each carry their documented value AND resolve to a name
  - so no enumerator of the vocabulary is left untouched by the suite, whatever
    the per-enum sections happen to check
*/
bool
tests_every_enumerator_is_reachable()
{
    D_CONSTEXPR bool values =
        ( (underlying(concurrency_progress::sequential)          == 0) &&
          (underlying(concurrency_progress::blocking)            == 1) &&
          (underlying(concurrency_progress::lock_free)           == 2) &&
          (underlying(concurrency_progress::wait_free)           == 3) &&

          (underlying(concurrency_arity::swmr)                   == 0) &&
          (underlying(concurrency_arity::mwmr)                   == 1) &&

          (underlying(iteration_consistency::inconsistent)       == 0) &&
          (underlying(iteration_consistency::weakly_consistent)  == 1) &&
          (underlying(iteration_consistency::snapshot)           == 2) &&

          (underlying(reclamation_obligation::none)              == 0) &&
          (underlying(reclamation_obligation::individual)        == 1) &&
          (underlying(reclamation_obligation::reference_count)   == 2) &&
          (underlying(reclamation_obligation::grace_period)      == 3) &&
          (underlying(reclamation_obligation::hazard)            == 4) &&
          (underlying(reclamation_obligation::collector)         == 5) );

    const bool names =
        ( (progress_name(concurrency_progress::sequential)       != nullptr) &&
          (progress_name(concurrency_progress::blocking)         != nullptr) &&
          (progress_name(concurrency_progress::lock_free)        != nullptr) &&
          (progress_name(concurrency_progress::wait_free)        != nullptr) &&

          (arity_name(concurrency_arity::swmr)                   != nullptr) &&
          (arity_name(concurrency_arity::mwmr)                   != nullptr) &&

          (iteration_name(iteration_consistency::inconsistent)   != nullptr) &&
          (iteration_name(
               iteration_consistency::weakly_consistent)         != nullptr) &&
          (iteration_name(iteration_consistency::snapshot)       != nullptr) &&

          (reclamation_name(reclamation_obligation::none)        != nullptr) &&
          (reclamation_name(
               reclamation_obligation::individual)               != nullptr) &&
          (reclamation_name(
               reclamation_obligation::reference_count)          != nullptr) &&
          (reclamation_name(
               reclamation_obligation::grace_period)             != nullptr) &&
          (reclamation_name(reclamation_obligation::hazard)      != nullptr) &&
          (reclamation_name(
               reclamation_obligation::collector)                != nullptr) );

    static_assert(values, "every enumerator must carry its documented value");

    return (values && names);
}

/*
tests_the_vocabulary_needs_no_trait_header
  the layering claim, observed from inside this TU.
  Tests the following:
  - this TU included the header under test and NOTHING else from djinterp, and
    it has just built a four-coordinate signature and a seven-tag mapping out
    of it
  - no strategy-trait or threadsafe-container-trait guard is defined here, so
    nothing arrived through the back door
  - which is the whole reason the vocabulary lives beside the tags instead of
    with the traits that consume it: the mechanism layer can name a progress
    grade without a cycle
*/
bool
tests_the_vocabulary_needs_no_trait_header()
{
#if ( defined(DJINTERP_CONCURRENCY_STRATEGY_TRAITS_)           || \
      defined(DJINTERP_SYNC_CONCURRENCY_STRATEGY_TRAITS_)      || \
      defined(DJINTERP_CONTAINER_TRAITS_CONCURRENCY_STRATEGY_TRAITS_HPP) || \
      defined(DJINTERP_CONTAINER_TRAITS_THREADSAFE_CONTAINER_TRAITS_HPP) ||    \
      defined(DJINTERP_CONTAINER_THREADSAFE_CONTAINER_TRAITS_) )
    const bool no_trait_header = false;
#else
    const bool no_trait_header = true;
#endif

    // and the vocabulary was enough on its own
    const bool the_vocabulary_sufficed =
        named_signature(mock_signature_of<hybrid_strategy_tag>::value());

    return (no_trait_header && the_vocabulary_sufficed);
}


NS_END  // testing
NS_END  // djinterp
