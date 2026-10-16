/*******************************************************************************
* djinterp [core]                      concurrency_strategy_tags_tests_arity.cpp
*
*
* path:      /tests/djinterp/core/sync/concurrency_strategy_tags_tests_arity.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp [test]             concurrency_strategy_tags_tests_arity.cpp
//   Section II (b): concurrency_arity - how many agents may act at once.
//
//   Two values, one order: swmr < mwmr.  A two-valued enum is the one most
// likely to be "simplified" into a bool by a later hand, so the tests below
// spend most of their effort proving it is NOT one: it does not convert to
// bool, it is not integral, and it has a third representable value (any int)
// that is named by neither enumerator - which a bool could not have, and which
// is exactly why a switch over it still needs a default arm.

// std
#include <cstring>
// djinterp
#include "concurrency_strategy_tags_tests.hpp"


NS_DJINTERP
NS_TESTING

namespace
{
    // arity
    //   type: the arity, spelled short.  The tests that are ABOUT
    // the type still name it in full (concurrency_arity); the shorthand
    // is for the ENUMERATORS, whose comparisons would otherwise carry more
    // qualification than content.
    using arity = concurrency_arity;

    // admits_concurrent_writers
    //   trait: a policy keyed on the ARITY as a non-type template parameter.
    // True only for the multi-writer arity; the single-writer arity is the
    // reader/writer split the monograph inherits from Mutability.
    template<arity _Arity>
    struct admits_concurrent_writers
    {
        static D_CONSTEXPR bool value = (_Arity == arity::mwmr);
    };
}

/*
tests_arity_enumerator_values
  each arity carries the value the header gives it.
  Tests the following:
  - swmr is 0 and mwmr is 1
*/
bool
tests_arity_enumerator_values()
{
    D_CONSTEXPR bool values =
        ( (underlying(arity::swmr) == 0) &&
          (underlying(arity::mwmr) == 1) );

    static_assert(values, "the arities must be swmr = 0 and mwmr = 1");

    return values;
}

/*
tests_arity_is_scoped
  it is an `enum class`.
  Tests the following:
  - no implicit conversion to int, nor from int
  - its enumerators are reached only as arity::x
*/
bool
tests_arity_is_scoped()
{
    using underlying_type =
        typename std::underlying_type<concurrency_arity>::type;

    D_CONSTEXPR bool scoped =
        ( is_scoped_enumeration<concurrency_arity>::value                &&
          std::is_enum<concurrency_arity>::value                         &&
          !std::is_convertible<concurrency_arity, int>::value            &&
          !std::is_convertible<concurrency_arity, underlying_type>::value &&
          !std::is_convertible<int, concurrency_arity>::value );

    static_assert(scoped, "concurrency_arity must be a scoped enum");

    return scoped;
}

/*
tests_arity_underlying_type
  the arity's representation.
  Tests the following:
  - the underlying type is int, as for every enum in the vocabulary (none of
    them fixes a base, and all of their values fit)
  - the four coordinates therefore share one representation, which is what lets
    a signature aggregate pack cleanly
*/
bool
tests_arity_underlying_type()
{
    using underlying_type =
        typename std::underlying_type<concurrency_arity>::type;

    D_CONSTEXPR bool is_int = std::is_same<underlying_type, int>::value;

    static_assert(is_int, "concurrency_arity must be backed by int");

    return is_int;
}

/*
tests_arity_size_and_alignment
  the arity costs exactly what its underlying type costs.
  Tests the following:
  - sizeof and alignof match those of int - a two-valued enum is not packed
    into a byte, and nothing should assume it is
*/
bool
tests_arity_size_and_alignment()
{
    D_CONSTEXPR bool same_as_int =
        ( (sizeof(concurrency_arity)  == sizeof(int)) &&
          (alignof(concurrency_arity) == alignof(int)) );

    static_assert(same_as_int, "an arity must cost one int");

    return same_as_int;
}

/*
tests_arity_is_a_total_order
  the header's stated order: swmr < mwmr.
  Tests the following:
  - swmr < mwmr, and not the reverse
  - each is <= itself, and neither is < itself
  - swmr is the floor and mwmr the ceiling
*/
bool
tests_arity_is_a_total_order()
{
    D_CONSTEXPR bool ordered =
        ( (arity::swmr < arity::mwmr)    &&
          !(arity::mwmr < arity::swmr)   &&
          (arity::mwmr > arity::swmr) );

    D_CONSTEXPR bool reflexive =
        ( (arity::swmr <= arity::swmr)  &&
          (arity::mwmr <= arity::mwmr)  &&
          !(arity::swmr < arity::swmr)  &&
          !(arity::mwmr < arity::mwmr) );

    static_assert(ordered, "swmr < mwmr");

    return (ordered && reflexive);
}

/*
tests_arity_is_not_a_boolean
  two values do not make a bool.
  Tests the following:
  - no implicit conversion to bool, so `if (arity)` cannot compile
  - the type is not bool, and is not integral
  - a value outside {0, 1} is still representable (see the fall-through test),
    which a bool would not be
*/
bool
tests_arity_is_not_a_boolean()
{
    D_CONSTEXPR bool not_a_bool =
        ( !std::is_convertible<concurrency_arity, bool>::value  &&
          !std::is_same<concurrency_arity, bool>::value         &&
          !std::is_integral<concurrency_arity>::value           &&
          !std::is_convertible<bool, concurrency_arity>::value );

    static_assert(not_a_bool, "an arity is a coordinate, not a flag");

    return not_a_bool;
}

/*
tests_arity_round_trips_through_the_underlying_type
  an arity survives the trip out to its integer and back.
  Tests the following:
  - static_cast out and back yields the same arity, for both
*/
bool
tests_arity_round_trips_through_the_underlying_type()
{
    D_CONSTEXPR bool round_trips =
        ( (static_cast<arity>(underlying(arity::swmr)) == arity::swmr) &&
          (static_cast<arity>(underlying(arity::mwmr)) == arity::mwmr) );

    static_assert(round_trips, "an arity must survive a cast round trip");

    return round_trips;
}

/*
tests_arity_in_constant_expressions
  the arity is a compile-time value.
  Tests the following:
  - a constexpr variable of the type
  - the arity as a non-type template parameter, keying a policy: only mwmr
    admits concurrent writers
*/
bool
tests_arity_in_constant_expressions()
{
    D_CONSTEXPR arity arity = arity::mwmr;

    static_assert(arity > arity::swmr,
                  "an arity must be comparable at compile time");

    D_CONSTEXPR bool keyed_a_policy =
        ( !admits_concurrent_writers<arity::swmr>::value &&
          admits_concurrent_writers<arity::mwmr>::value );

    static_assert(keyed_a_policy,
                  "an arity must be usable as a non-type template parameter");

    return keyed_a_policy;
}

/*
tests_arity_switch_covers_both_arities
  both arities are reachable, and each reaches its own arm.
  Tests the following:
  - a switch over the two named arities hits a distinct arm for each
*/
bool
tests_arity_switch_covers_both_arities()
{
    const char* const swmr = arity_name(arity::swmr);
    const char* const mwmr = arity_name(arity::mwmr);

    const bool both_named =
        ( (swmr != nullptr) &&
          (mwmr != nullptr) );

    // check if either arity fell through before comparing the arms
    if (!both_named)
    {
        return false;
    }

    const bool correct_arms =
        ( (std::strcmp(swmr, "swmr") == 0) &&
          (std::strcmp(mwmr, "mwmr") == 0) );

    return correct_arms;
}

/*
tests_arity_has_exactly_two_arities
  the arity set is closed: two names, contiguous from zero.
  Tests the following:
  - the values 0 and 1 are named
  - the value 2 is not - there is no third arity (no "multi-writer /
    single-reader", which the monograph's reader/writer split has no room for)
*/
bool
tests_arity_has_exactly_two_arities()
{
    const bool two_named =
        ( (arity_name(static_cast<arity>(0)) != nullptr) &&
          (arity_name(static_cast<arity>(1)) != nullptr) );

    const bool no_third =
        (arity_name(static_cast<arity>(2)) == nullptr);

    return (two_named && no_third);
}

/*
tests_arity_out_of_range_value_falls_through
  a value outside the two names is still a value of the type.
  Tests the following:
  - casting 7 to the arity is well-defined (the value range is the underlying
    type's), and round-trips unchanged
  - it falls through the switch to the default arm
*/
bool
tests_arity_out_of_range_value_falls_through()
{
    const arity unnamed = static_cast<arity>(7);

    const bool value_survives = (underlying(unnamed) == 7);
    const bool falls_through  = (arity_name(unnamed) == nullptr);
    const bool still_ordered  = (unnamed > arity::mwmr);

    return ( value_survives &&
             falls_through  &&
             still_ordered );
}


NS_END  // testing
NS_END  // djinterp
