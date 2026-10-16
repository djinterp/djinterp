/*******************************************************************************
* djinterp [core]                concurrency_strategy_tags_tests_reclamation.cpp
*
*
* path:      /tests/djinterp/core/sync/concurrency_strategy_tags_tests_reclamation.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp [test]       concurrency_strategy_tags_tests_reclamation.cpp
//   Section II (d): reclamation_obligation - what a container owes when a cell
// is freed while other agents are running.
//
//   Six disciplines, 0..5.  Two of the header's sentences are testable claims
// rather than prose: `none` means there is nothing to reclaim (and it is the
// zero, so a value-initialized obligation is the empty one), and `individual`
// - free on unlink - is "sound ONLY under exclusion or a single agent",
// unsound for shared nodes freed lock-free.  Both are pinned below.
//
//   The last test in the section is about the NAMES rather than the values:
// `none`, `individual`, `hazard` and `collector` are generic enough words that,
// in an unscoped enum, they would have been injected straight into namespace
// djinterp - the namespace every consumer of this header opens.  The scoping
// is what keeps them out, and that is worth a test of its own.

// std
#include <cstring>
// djinterp
#include "concurrency_strategy_tags_tests.hpp"


NS_DJINTERP
NS_TESTING

namespace
{
    // obligation
    //   type: the reclamation obligation, spelled short.  The tests that are
    // ABOUT the type still name it in full (reclamation_obligation); the
    // shorthand is for the ENUMERATORS, whose comparisons would otherwise
    // carry more qualification than content.
    using obligation = reclamation_obligation;

    // sound_for_lock_free_freeing
    //   trait: a policy keyed on the OBLIGATION as a non-type template
    // parameter, and taken straight from the header's own words: `individual`
    // is sound only under exclusion or a single agent, and unsound for shared
    // nodes freed lock-free.  Every other discipline - including `none`, which
    // has nothing to free - is sound there.
    template<obligation _Obligation>
    struct sound_for_lock_free_freeing
    {
        static D_CONSTEXPR bool value =
            (_Obligation != obligation::individual);
    };

    // hazard
    //   type: a local entity deliberately named for one of the obligations.
    // It compiles, and it is unrelated to both reclamation_obligation::hazard
    // and hazard_strategy_tag - because the enum is SCOPED, so the word
    // `hazard` was never injected into namespace djinterp for it to collide
    // with.  (The same goes for `none`, `individual` and `collector`.)
    struct hazard
    {
        int reclaimed;
    };
}

/*
tests_reclamation_enumerator_values
  each obligation carries the value the header gives it.
  Tests the following:
  - none is 0, individual is 1, reference_count is 2, grace_period is 3,
    hazard is 4, collector is 5
*/
bool
tests_reclamation_enumerator_values()
{
    D_CONSTEXPR bool values =
        ( (underlying(obligation::none)            == 0) &&
          (underlying(obligation::individual)      == 1) &&
          (underlying(obligation::reference_count) == 2) &&
          (underlying(obligation::grace_period)    == 3) &&
          (underlying(obligation::hazard)          == 4) &&
          (underlying(obligation::collector)       == 5) );

    static_assert(values, "the obligations must be 0 through 5");

    return values;
}

/*
tests_reclamation_is_scoped
  it is an `enum class`.
  Tests the following:
  - no implicit conversion to int, to its underlying type, or to bool
  - no implicit conversion from int
*/
bool
tests_reclamation_is_scoped()
{
    using underlying_type =
        typename std::underlying_type<reclamation_obligation>::type;

    D_CONSTEXPR bool scoped =
        ( is_scoped_enumeration<reclamation_obligation>::value                &&
          std::is_enum<reclamation_obligation>::value                         &&
          !std::is_convertible<reclamation_obligation, int>::value            &&
          !std::is_convertible<reclamation_obligation,
                               underlying_type>::value                        &&
          !std::is_convertible<reclamation_obligation, bool>::value           &&
          !std::is_convertible<int, reclamation_obligation>::value );

    static_assert(scoped, "reclamation_obligation must be a scoped enum");

    return scoped;
}

/*
tests_reclamation_underlying_type
  the obligation's representation.
  Tests the following:
  - the underlying type is int, as for the rest of the vocabulary
  - sizeof and alignof match int's - six values do not shrink the type
*/
bool
tests_reclamation_underlying_type()
{
    using underlying_type =
        typename std::underlying_type<reclamation_obligation>::type;

    D_CONSTEXPR bool is_int = std::is_same<underlying_type, int>::value;

    D_CONSTEXPR bool same_as_int =
        ( (sizeof(reclamation_obligation)  == sizeof(int)) &&
          (alignof(reclamation_obligation) == alignof(int)) );

    static_assert(is_int, "reclamation_obligation must be backed by int");

    return (is_int && same_as_int);
}

/*
tests_reclamation_orders_the_disciplines
  the values ascend through the disciplines in the order the header lists them.
  Tests the following:
  - none < individual < reference_count < grace_period < hazard < collector
  - transitivity across the chain, and no reverse comparison
  Note: the header states a TOTAL ORDER for progress and for arity in so many
  words.  It does not for the disciplines - they are a list, not a ladder - so
  what is checked here is the order the ENUM encodes, which is what any
  consumer comparing two obligations will actually see.
*/
bool
tests_reclamation_orders_the_disciplines()
{
    D_CONSTEXPR bool ascending =
        ( (obligation::none            < obligation::individual)      &&
          (obligation::individual      < obligation::reference_count) &&
          (obligation::reference_count < obligation::grace_period)    &&
          (obligation::grace_period    < obligation::hazard)          &&
          (obligation::hazard          < obligation::collector) );

    D_CONSTEXPR bool transitive =
        ( (obligation::none       < obligation::collector) &&
          (obligation::individual < obligation::hazard) );

    D_CONSTEXPR bool not_descending =
        ( !(obligation::collector < obligation::none)         &&
          !(obligation::hazard    < obligation::grace_period) );

    static_assert(ascending,
                  "the disciplines must ascend in the order the header lists");

    return ( ascending  &&
             transitive &&
             not_descending );
}

/*
tests_reclamation_none_is_the_floor
  `none` - nothing to reclaim - is the zero, and the default.
  Tests the following:
  - none is 0, and is <= every other obligation
  - a VALUE-INITIALIZED obligation is `none`, so a signature that forgot to set
    the field claims no discipline rather than claiming a discipline it does
    not implement
  - `individual` is the one discipline the header calls unsound for shared
    nodes freed lock-free; every other obligation (none included) is sound
    there
*/
bool
tests_reclamation_none_is_the_floor()
{
    D_CONSTEXPR bool is_zero =
        (underlying(obligation::none) == 0);

    D_CONSTEXPR bool is_the_minimum =
        ( (obligation::none <= obligation::individual) &&
          (obligation::none <= obligation::collector) );

    // a value-initialized obligation
    const obligation defaulted = obligation();

    const bool defaults_to_none = (defaulted == obligation::none);

    D_CONSTEXPR bool individual_is_the_unsound_one =
        ( sound_for_lock_free_freeing<obligation::none>::value             &&
          !sound_for_lock_free_freeing<obligation::individual>::value      &&
          sound_for_lock_free_freeing<obligation::reference_count>::value  &&
          sound_for_lock_free_freeing<obligation::grace_period>::value     &&
          sound_for_lock_free_freeing<obligation::hazard>::value           &&
          sound_for_lock_free_freeing<obligation::collector>::value );

    static_assert(is_zero, "none must be the zero of the obligations");

    return ( is_zero                       &&
             is_the_minimum                &&
             defaults_to_none              &&
             individual_is_the_unsound_one );
}

/*
tests_reclamation_round_trips_through_the_underlying_type
  an obligation survives the trip out to its integer and back.
  Tests the following:
  - static_cast out and back yields the same obligation, for all six
*/
bool
tests_reclamation_round_trips_through_the_underlying_type()
{
    D_CONSTEXPR bool round_trips =
        ( (static_cast<obligation>(underlying(obligation::none)) ==
           obligation::none) &&
          (static_cast<obligation>(underlying(obligation::individual)) ==
           obligation::individual) &&
          (static_cast<obligation>(underlying(obligation::reference_count)) ==
           obligation::reference_count) &&
          (static_cast<obligation>(underlying(obligation::grace_period)) ==
           obligation::grace_period) &&
          (static_cast<obligation>(underlying(obligation::hazard)) ==
           obligation::hazard) &&
          (static_cast<obligation>(underlying(obligation::collector)) ==
           obligation::collector) );

    static_assert(round_trips,
                  "an obligation must survive a cast round trip");

    return round_trips;
}

/*
tests_reclamation_in_constant_expressions
  the obligation is a compile-time value.
  Tests the following:
  - a constexpr variable of the type
  - the obligation as a non-type template parameter, keying the soundness
    policy the header's own comment states
*/
bool
tests_reclamation_in_constant_expressions()
{
    D_CONSTEXPR obligation obligation =
        obligation::hazard;

    static_assert(obligation > obligation::individual,
                  "an obligation must be comparable at compile time");

    D_CONSTEXPR bool keyed_a_policy =
        sound_for_lock_free_freeing<obligation>::value;

    static_assert(keyed_a_policy,
                  "an obligation must be usable as a non-type template "
                  "parameter");

    return keyed_a_policy;
}

/*
tests_reclamation_switch_covers_every_obligation
  all six are reachable, and each reaches its own arm.
  Tests the following:
  - a switch over the six named obligations hits a distinct arm for each
*/
bool
tests_reclamation_switch_covers_every_obligation()
{
    const char* const none =
        reclamation_name(obligation::none);
    const char* const individual =
        reclamation_name(obligation::individual);
    const char* const refcount =
        reclamation_name(obligation::reference_count);
    const char* const grace =
        reclamation_name(obligation::grace_period);
    const char* const hazard_arm =
        reclamation_name(obligation::hazard);
    const char* const collector =
        reclamation_name(obligation::collector);

    const bool all_named =
        ( (none       != nullptr) &&
          (individual != nullptr) &&
          (refcount   != nullptr) &&
          (grace      != nullptr) &&
          (hazard_arm != nullptr) &&
          (collector  != nullptr) );

    // check if any obligation fell through before comparing the arms
    if (!all_named)
    {
        return false;
    }

    const bool correct_arms =
        ( (std::strcmp(none,       "none")       == 0) &&
          (std::strcmp(individual, "individual") == 0) &&
          (std::strcmp(refcount,   "refcount")   == 0) &&
          (std::strcmp(grace,      "grace")      == 0) &&
          (std::strcmp(hazard_arm, "hazard")     == 0) &&
          (std::strcmp(collector,  "collector")  == 0) );

    return correct_arms;
}

/*
tests_reclamation_has_exactly_six_obligations
  the set is closed: six names, contiguous from zero.
  Tests the following:
  - the values 0..5 are all named
  - the value 6 is not - there is no seventh discipline
*/
bool
tests_reclamation_has_exactly_six_obligations()
{
    const bool six_named =
        ( (reclamation_name(static_cast<obligation>(0)) != nullptr) &&
          (reclamation_name(static_cast<obligation>(1)) != nullptr) &&
          (reclamation_name(static_cast<obligation>(2)) != nullptr) &&
          (reclamation_name(static_cast<obligation>(3)) != nullptr) &&
          (reclamation_name(static_cast<obligation>(4)) != nullptr) &&
          (reclamation_name(static_cast<obligation>(5)) != nullptr) );

    const bool no_seventh =
        (reclamation_name(static_cast<obligation>(6)) == nullptr);

    D_CONSTEXPR bool collector_is_the_ceiling =
        (underlying(obligation::collector) == 5);

    return ( six_named                &&
             no_seventh               &&
             collector_is_the_ceiling );
}

/*
tests_reclamation_out_of_range_value_falls_through
  a value outside the six names is still a value of the type.
  Tests the following:
  - casting 11 to the obligation is well-defined and round-trips unchanged
  - it falls through the switch to the default arm
*/
bool
tests_reclamation_out_of_range_value_falls_through()
{
    const obligation unnamed =
        static_cast<obligation>(11);

    const bool value_survives = (underlying(unnamed) == 11);
    const bool falls_through  = (reclamation_name(unnamed) == nullptr);
    const bool still_ordered  = (unnamed > obligation::collector);

    return ( value_survives &&
             falls_through  &&
             still_ordered );
}

/*
tests_reclamation_enumerators_do_not_collide_with_the_tags
  the scoping keeps six very generic words out of namespace djinterp.
  Tests the following:
  - a local type may be named `hazard` with no ambiguity, because the
    obligation is reachable only as reclamation_obligation::hazard
  - that local `hazard` is neither the obligation nor hazard_strategy_tag
  - the tag names themselves are untouched: hazard_strategy_tag and
    none_strategy_tag still name their one-byte tag types
  An unscoped enum here would have injected `none`, `individual`,
  `reference_count`, `grace_period`, `hazard` and `collector` into the
  namespace of every consumer of this header.
*/
bool
tests_reclamation_enumerators_do_not_collide_with_the_tags()
{
    D_CONSTEXPR bool the_local_name_is_free =
        ( std::is_class<hazard>::value                       &&
          !std::is_same<hazard, hazard_strategy_tag>::value  &&
          !std::is_enum<hazard>::value );

    D_CONSTEXPR bool the_obligation_is_qualified =
        ( std::is_enum<
              decltype(obligation::hazard)>::value &&
          std::is_same<decltype(obligation::hazard),
                       reclamation_obligation>::value );

    D_CONSTEXPR bool the_tags_are_untouched =
        ( (sizeof(hazard_strategy_tag) == 1) &&
          (sizeof(none_strategy_tag)   == 1) &&
          is_strategy_tag<hazard_strategy_tag>::value &&
          is_strategy_tag<none_strategy_tag>::value );

    const bool the_obligation_still_reads =
        (underlying(obligation::hazard) == 4);

    static_assert(the_local_name_is_free,
                  "a scoped enumerator must not claim the name for the "
                  "namespace");

    return ( the_local_name_is_free       &&
             the_obligation_is_qualified  &&
             the_tags_are_untouched       &&
             the_obligation_still_reads );
}


NS_END  // testing
NS_END  // djinterp
