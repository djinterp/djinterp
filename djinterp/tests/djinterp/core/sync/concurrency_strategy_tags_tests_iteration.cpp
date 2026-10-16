/*******************************************************************************
* djinterp [core]                  concurrency_strategy_tags_tests_iteration.cpp
*
*
* path:      /tests/djinterp/core/sync/concurrency_strategy_tags_tests_iteration.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp [test]         concurrency_strategy_tags_tests_iteration.cpp
//   Section II (c): iteration_consistency - the overlap-semantics of a
// traversal that runs while another agent mutates.
//
//   Three points, and the header is careful about what it claims for them: it
// names `inconsistent` "the non-concurrent degenerate", and the enumerator
// values 0, 1, 2 run from that degenerate up through weakly_consistent to
// snapshot.  Unlike progress and arity, the header does NOT call this a total
// order - so the tests below check the ORDER THE VALUES ENCODE (a fact of the
// enum) and the identity of the floor (a claim of the header), and stop short
// of asserting a monograph claim the header never made.

// std
#include <cstring>
// djinterp
#include "concurrency_strategy_tags_tests.hpp"


NS_DJINTERP
NS_TESTING

namespace
{
    // consistency
    //   type: the overlap semantics, spelled short.  The tests that are ABOUT
    // the type still name it in full (iteration_consistency); the shorthand
    // is for the ENUMERATORS, whose comparisons would otherwise carry more
    // qualification than content.
    using consistency = iteration_consistency;

    // survives_overlap
    //   trait: a policy keyed on the ITERATION semantics as a non-type
    // template parameter.  A traversal is safe to overlap with a mutation
    // above the degenerate; at the degenerate it is not.
    template<consistency _Semantics>
    struct survives_overlap
    {
        static D_CONSTEXPR bool value =
            (_Semantics > consistency::inconsistent);
    };
}

/*
tests_iteration_enumerator_values
  each overlap-semantics carries the value the header gives it.
  Tests the following:
  - inconsistent is 0, weakly_consistent is 1, snapshot is 2
*/
bool
tests_iteration_enumerator_values()
{
    D_CONSTEXPR bool values =
        ( (underlying(consistency::inconsistent)      == 0) &&
          (underlying(consistency::weakly_consistent) == 1) &&
          (underlying(consistency::snapshot)          == 2) );

    static_assert(values,
                  "the overlap semantics must be 0, 1, 2");

    return values;
}

/*
tests_iteration_is_scoped
  it is an `enum class`.
  Tests the following:
  - no implicit conversion to int, to its underlying type, or to bool
  - no implicit conversion from int
*/
bool
tests_iteration_is_scoped()
{
    using underlying_type =
        typename std::underlying_type<iteration_consistency>::type;

    D_CONSTEXPR bool scoped =
        ( is_scoped_enumeration<iteration_consistency>::value                &&
          std::is_enum<iteration_consistency>::value                         &&
          !std::is_convertible<iteration_consistency, int>::value            &&
          !std::is_convertible<iteration_consistency, underlying_type>::value &&
          !std::is_convertible<iteration_consistency, bool>::value           &&
          !std::is_convertible<int, iteration_consistency>::value );

    static_assert(scoped, "iteration_consistency must be a scoped enum");

    return scoped;
}

/*
tests_iteration_underlying_type
  the coordinate's representation.
  Tests the following:
  - the underlying type is int, as for the rest of the vocabulary
*/
bool
tests_iteration_underlying_type()
{
    using underlying_type =
        typename std::underlying_type<iteration_consistency>::type;

    D_CONSTEXPR bool is_int = std::is_same<underlying_type, int>::value;

    D_CONSTEXPR bool same_as_int =
        ( (sizeof(iteration_consistency)  == sizeof(int)) &&
          (alignof(iteration_consistency) == alignof(int)) );

    static_assert(is_int, "iteration_consistency must be backed by int");

    return (is_int && same_as_int);
}

/*
tests_iteration_orders_the_overlap_semantics
  the values run from the degenerate upward, and the comparisons follow.
  Tests the following:
  - inconsistent < weakly_consistent < snapshot, and transitively
    inconsistent < snapshot
  - none of the reverse comparisons holds
  - each point is <= itself
  Note: the header calls progress and arity total orders in so many words; for
  this coordinate it does not, so what is checked here is the order the ENUM
  encodes - which is what any consumer comparing two of these will see.
*/
bool
tests_iteration_orders_the_overlap_semantics()
{
    D_CONSTEXPR bool ascending =
        ( (consistency::inconsistent      < consistency::weakly_consistent) &&
          (consistency::weakly_consistent < consistency::snapshot)          &&
          (consistency::inconsistent      < consistency::snapshot) );

    D_CONSTEXPR bool not_descending =
        ( !(consistency::weakly_consistent < consistency::inconsistent)      &&
          !(consistency::snapshot          < consistency::weakly_consistent) &&
          !(consistency::snapshot          < consistency::inconsistent) );

    D_CONSTEXPR bool reflexive =
        ( (consistency::inconsistent <= consistency::inconsistent) &&
          (consistency::snapshot     >= consistency::snapshot) );

    static_assert(ascending,
                  "inconsistent < weakly_consistent < snapshot");

    return ( ascending      &&
             not_descending &&
             reflexive );
}

/*
tests_iteration_the_degenerate_is_the_floor
  `inconsistent` is the non-concurrent degenerate, and sits at the bottom.
  Tests the following:
  - it is the minimum of the three
  - it is zero - so a value-initialized coordinate lands on the UNSAFE
    semantics, never on a safety it has not earned
  - the two consistent semantics are both strictly above it, and only they
    survive an overlapping mutation
*/
bool
tests_iteration_the_degenerate_is_the_floor()
{
    D_CONSTEXPR bool is_the_minimum =
        ( (consistency::inconsistent <= consistency::weakly_consistent) &&
          (consistency::inconsistent <= consistency::snapshot) );

    D_CONSTEXPR bool is_zero =
        (underlying(consistency::inconsistent) == 0);

    D_CONSTEXPR bool only_the_others_survive_overlap =
        ( !survives_overlap<consistency::inconsistent>::value     &&
          survives_overlap<consistency::weakly_consistent>::value &&
          survives_overlap<consistency::snapshot>::value );

    static_assert(is_zero && is_the_minimum,
                  "the degenerate must be the zero and the floor");

    return ( is_the_minimum &&
             is_zero        &&
             only_the_others_survive_overlap );
}

/*
tests_iteration_round_trips_through_the_underlying_type
  the coordinate survives the trip out to its integer and back.
  Tests the following:
  - static_cast out and back yields the same semantics, for all three
*/
bool
tests_iteration_round_trips_through_the_underlying_type()
{
    D_CONSTEXPR bool round_trips =
        ( (static_cast<consistency>(underlying(consistency::inconsistent)) ==
           consistency::inconsistent) &&
          (static_cast<consistency>(
               underlying(consistency::weakly_consistent)) ==
           consistency::weakly_consistent) &&
          (static_cast<consistency>(underlying(consistency::snapshot)) ==
           consistency::snapshot) );

    static_assert(round_trips,
                  "the overlap semantics must survive a cast round trip");

    return round_trips;
}

/*
tests_iteration_in_constant_expressions
  the coordinate is a compile-time value.
  Tests the following:
  - a constexpr variable of the type
  - the semantics as a non-type template parameter, keying a policy
*/
bool
tests_iteration_in_constant_expressions()
{
    D_CONSTEXPR consistency semantics =
        consistency::snapshot;

    static_assert(semantics > consistency::weakly_consistent,
                  "the semantics must be comparable at compile time");

    D_CONSTEXPR bool keyed_a_policy =
        survives_overlap<semantics>::value;

    static_assert(keyed_a_policy,
                  "the semantics must be usable as a non-type template "
                  "parameter");

    return keyed_a_policy;
}

/*
tests_iteration_switch_covers_every_semantics
  all three are reachable, and each reaches its own arm.
  Tests the following:
  - a switch over the three named semantics hits a distinct arm for each
*/
bool
tests_iteration_switch_covers_every_semantics()
{
    const char* const inconsistent =
        iteration_name(consistency::inconsistent);
    const char* const weakly =
        iteration_name(consistency::weakly_consistent);
    const char* const snapshot =
        iteration_name(consistency::snapshot);

    const bool all_named =
        ( (inconsistent != nullptr) &&
          (weakly       != nullptr) &&
          (snapshot     != nullptr) );

    // check if any semantics fell through before comparing the arms
    if (!all_named)
    {
        return false;
    }

    const bool correct_arms =
        ( (std::strcmp(inconsistent, "inconsistent")      == 0) &&
          (std::strcmp(weakly,       "weakly_consistent") == 0) &&
          (std::strcmp(snapshot,     "snapshot")          == 0) );

    return correct_arms;
}

/*
tests_iteration_has_exactly_three_semantics
  the set is closed: three names, contiguous from zero.
  Tests the following:
  - the values 0..2 are all named
  - the value 3 is not - snapshot is the strongest the header offers
*/
bool
tests_iteration_has_exactly_three_semantics()
{
    const bool three_named =
        ( (iteration_name(static_cast<consistency>(0)) != nullptr) &&
          (iteration_name(static_cast<consistency>(1)) != nullptr) &&
          (iteration_name(static_cast<consistency>(2)) != nullptr) );

    const bool no_fourth =
        (iteration_name(static_cast<consistency>(3)) == nullptr);

    D_CONSTEXPR bool snapshot_is_the_ceiling =
        (underlying(consistency::snapshot) == 2);

    return ( three_named            &&
             no_fourth              &&
             snapshot_is_the_ceiling );
}

/*
tests_iteration_out_of_range_value_falls_through
  a value outside the three names is still a value of the type.
  Tests the following:
  - casting 5 to the coordinate is well-defined and round-trips unchanged
  - it falls through the switch to the default arm
*/
bool
tests_iteration_out_of_range_value_falls_through()
{
    const consistency unnamed =
        static_cast<consistency>(5);

    const bool value_survives = (underlying(unnamed) == 5);
    const bool falls_through  = (iteration_name(unnamed) == nullptr);
    const bool still_ordered  = (unnamed > consistency::snapshot);

    return ( value_survives &&
             falls_through  &&
             still_ordered );
}


NS_END  // testing
NS_END  // djinterp
