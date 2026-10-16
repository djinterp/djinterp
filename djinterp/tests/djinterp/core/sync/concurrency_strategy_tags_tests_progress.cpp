/*******************************************************************************
* djinterp [core]                   concurrency_strategy_tags_tests_progress.cpp
*
*
* path:      /tests/djinterp/core/sync/concurrency_strategy_tags_tests_progress.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp [test]          concurrency_strategy_tags_tests_progress.cpp
//   Section II (a): concurrency_progress - the progress grade.
//
//   The header states a TOTAL ORDER, in words, above the enum:
//       sequential < blocking < lock_free < wait_free
// and then encodes it in the enumerator values 0..3.  The words and the values
// can drift apart; nothing in the language ties them together.  So the values
// are pinned one by one, the order is checked as an order (irreflexive,
// antisymmetric, transitive), and the count is pinned too - because a grade
// inserted in the middle would renumber everything above it and silently
// change the meaning of every comparison the container layer makes.

// std
#include <cstring>
// djinterp
#include "concurrency_strategy_tags_tests.hpp"


NS_DJINTERP
NS_TESTING

namespace
{
    // progress
    //   type: the progress grade, spelled short.  The tests that are ABOUT
    // the type still name it in full (concurrency_progress); the shorthand
    // is for the ENUMERATORS, whose comparisons would otherwise carry more
    // qualification than content.
    using progress = concurrency_progress;

    // demands_no_global_stall
    //   trait: a policy keyed on the GRADE as a non-type template parameter -
    // the compile-time use the vocabulary exists to serve.  True for the two
    // grades at or above lock_free.
    template<progress _Grade>
    struct demands_no_global_stall
    {
        static D_CONSTEXPR bool value =
            (_Grade >= progress::lock_free);
    };
}

/*
tests_progress_enumerator_values
  each grade carries the value the header gives it.
  Tests the following:
  - sequential is 0, blocking is 1, lock_free is 2, wait_free is 3
  - the values are read through the underlying type, since a scoped enum will
    not hand one over implicitly
*/
bool
tests_progress_enumerator_values()
{
    D_CONSTEXPR bool values =
        ( (underlying(progress::sequential) == 0) &&
          (underlying(progress::blocking)   == 1) &&
          (underlying(progress::lock_free)  == 2) &&
          (underlying(progress::wait_free)  == 3) );

    static_assert(values, "the progress grades must be 0, 1, 2, 3");

    return values;
}

/*
tests_progress_is_an_enum_not_an_integer
  the type-trait classification of the grade.
  Tests the following:
  - std::is_enum holds
  - std::is_integral and std::is_arithmetic do NOT (an enum is neither)
  - it is not a class, so it carries no members and no invariants of its own
*/
bool
tests_progress_is_an_enum_not_an_integer()
{
    D_CONSTEXPR bool classified =
        ( std::is_enum<concurrency_progress>::value        &&
          !std::is_integral<concurrency_progress>::value   &&
          !std::is_arithmetic<concurrency_progress>::value &&
          !std::is_class<concurrency_progress>::value );

    static_assert(classified, "concurrency_progress must be an enum type");

    return classified;
}

/*
tests_progress_is_scoped
  it is an `enum class`, and the scoping is load-bearing.
  Tests the following:
  - no implicit conversion to int, nor to its own underlying type
  - no implicit conversion FROM int either
  - no implicit conversion to bool - so `if (grade)` will not compile, and a
    grade can never be mistaken for a flag
  - its enumerators are therefore reached only as progress::x, and
    cannot collide with any other name in namespace djinterp
*/
bool
tests_progress_is_scoped()
{
    using underlying_type =
        typename std::underlying_type<concurrency_progress>::type;

    D_CONSTEXPR bool scoped =
        ( is_scoped_enumeration<concurrency_progress>::value                &&
          !std::is_convertible<concurrency_progress, int>::value            &&
          !std::is_convertible<concurrency_progress, underlying_type>::value &&
          !std::is_convertible<int, concurrency_progress>::value            &&
          !std::is_convertible<concurrency_progress, bool>::value );

    static_assert(scoped, "concurrency_progress must be a scoped enum");

    return scoped;
}

/*
tests_progress_underlying_type
  the grade's representation.
  Tests the following:
  - the underlying type is int - the default for a scoped enum with no fixed
    base whose enumerators all fit
  - it is signed, so a cast of a negative value is representable (and lands in
    the switch's default arm, not in undefined behaviour)
*/
bool
tests_progress_underlying_type()
{
    using underlying_type =
        typename std::underlying_type<concurrency_progress>::type;

    D_CONSTEXPR bool is_int = std::is_same<underlying_type, int>::value;

    D_CONSTEXPR bool is_signed = std::is_signed<underlying_type>::value;

    static_assert(is_int, "concurrency_progress must be backed by int");

    return (is_int && is_signed);
}

/*
tests_progress_size_and_alignment
  the grade costs exactly what its underlying type costs.
  Tests the following:
  - sizeof and alignof match those of int
  - so a signature aggregate of four such enums is four ints wide, and cheap to
    pass by value
*/
bool
tests_progress_size_and_alignment()
{
    D_CONSTEXPR bool same_as_int =
        ( (sizeof(concurrency_progress)  == sizeof(int)) &&
          (alignof(concurrency_progress) == alignof(int)) );

    static_assert(same_as_int, "a progress grade must cost one int");

    return same_as_int;
}

/*
tests_progress_is_a_total_order
  the header's stated order, checked AS an order.
  Tests the following:
  - the chain: sequential < blocking < lock_free < wait_free
  - transitivity across the chain (sequential < wait_free)
  - antisymmetry: none of the reverse comparisons holds
  - irreflexivity: no grade is less than itself, and every grade is <= itself
  - the floor and the ceiling: sequential <= g <= wait_free for every grade
*/
bool
tests_progress_is_a_total_order()
{
    D_CONSTEXPR bool chain =
        ( (progress::sequential < progress::blocking)  &&
          (progress::blocking   < progress::lock_free) &&
          (progress::lock_free  < progress::wait_free) );

    D_CONSTEXPR bool transitive =
        ( (progress::sequential < progress::lock_free) &&
          (progress::sequential < progress::wait_free) &&
          (progress::blocking   < progress::wait_free) );

    D_CONSTEXPR bool antisymmetric =
        ( !(progress::blocking  < progress::sequential) &&
          !(progress::lock_free < progress::blocking)   &&
          !(progress::wait_free < progress::lock_free)  &&
          !(progress::wait_free < progress::sequential) );

    D_CONSTEXPR bool irreflexive =
        ( !(progress::sequential < progress::sequential) &&
          !(progress::wait_free  < progress::wait_free)  &&
          (progress::lock_free  <= progress::lock_free) );

    D_CONSTEXPR bool bounded =
        ( (progress::sequential <= progress::sequential) &&
          (progress::sequential <= progress::blocking)   &&
          (progress::sequential <= progress::lock_free)  &&
          (progress::sequential <= progress::wait_free)  &&
          (progress::wait_free  >= progress::sequential) &&
          (progress::wait_free  >= progress::blocking)   &&
          (progress::wait_free  >= progress::lock_free)  &&
          (progress::wait_free  >= progress::wait_free) );

    static_assert(chain, "sequential < blocking < lock_free < wait_free");

    return ( chain         &&
             transitive    &&
             antisymmetric &&
             irreflexive   &&
             bounded );
}

/*
tests_progress_equality
  the grades compare equal to themselves and to nothing else.
  Tests the following:
  - each of the four equals itself
  - all six unordered pairs are unequal, under both == and !=
*/
bool
tests_progress_equality()
{
    D_CONSTEXPR bool reflexive =
        ( (progress::sequential == progress::sequential) &&
          (progress::blocking   == progress::blocking)   &&
          (progress::lock_free  == progress::lock_free)  &&
          (progress::wait_free  == progress::wait_free) );

    D_CONSTEXPR bool distinct =
        ( (progress::sequential != progress::blocking)  &&
          (progress::sequential != progress::lock_free) &&
          (progress::sequential != progress::wait_free) &&
          (progress::blocking   != progress::lock_free) &&
          (progress::blocking   != progress::wait_free) &&
          (progress::lock_free  != progress::wait_free) );

    static_assert(reflexive && distinct, "the four grades must be four values");

    return (reflexive && distinct);
}

/*
tests_progress_round_trips_through_the_underlying_type
  a grade survives the trip out to its integer and back.
  Tests the following:
  - static_cast to the underlying type and back yields the same grade, for all
    four
  - which is what an option table, a serialized signature, or a C boundary
    needs
*/
bool
tests_progress_round_trips_through_the_underlying_type()
{
    D_CONSTEXPR bool round_trips =
        ( (static_cast<progress>(underlying(progress::sequential)) ==
           progress::sequential) &&
          (static_cast<progress>(underlying(progress::blocking)) ==
           progress::blocking) &&
          (static_cast<progress>(underlying(progress::lock_free)) ==
           progress::lock_free) &&
          (static_cast<progress>(underlying(progress::wait_free)) ==
           progress::wait_free) );

    static_assert(round_trips, "a grade must survive a cast round trip");

    return round_trips;
}

/*
tests_progress_in_constant_expressions
  the grade is a compile-time value.
  Tests the following:
  - a constexpr variable of the type
  - a comparison usable in a static_assert
  - the grade as a NON-TYPE template parameter, keying a policy (lock_free and
    wait_free demand no global stall; sequential and blocking do not)
  - the underlying value as an array bound
*/
bool
tests_progress_in_constant_expressions()
{
    D_CONSTEXPR progress grade = progress::lock_free;

    static_assert(grade > progress::blocking,
                  "a grade must be comparable at compile time");

    D_CONSTEXPR bool keyed_a_policy =
        ( !demands_no_global_stall<progress::sequential>::value &&
          !demands_no_global_stall<progress::blocking>::value   &&
          demands_no_global_stall<progress::lock_free>::value   &&
          demands_no_global_stall<progress::wait_free>::value );

    static_assert(keyed_a_policy,
                  "a grade must be usable as a non-type template parameter");

    // the underlying value is an integral constant expression: an array bound
    D_CONSTEXPR std::size_t slot_count =
        static_cast<std::size_t>(underlying(grade)) + 1;

    int slots[slot_count];

    return ( keyed_a_policy &&
             (sizeof(slots) / sizeof(slots[0]) == 3) );
}

/*
tests_progress_switch_covers_every_grade
  every grade is reachable, and each reaches its own arm.
  Tests the following:
  - a switch over the four named grades hits a distinct arm for each
  - no two grades share an arm
*/
bool
tests_progress_switch_covers_every_grade()
{
    const char* const sequential =
        progress_name(progress::sequential);
    const char* const blocking =
        progress_name(progress::blocking);
    const char* const lock_free =
        progress_name(progress::lock_free);
    const char* const wait_free =
        progress_name(progress::wait_free);

    const bool all_named =
        ( (sequential != nullptr) &&
          (blocking   != nullptr) &&
          (lock_free  != nullptr) &&
          (wait_free  != nullptr) );

    // check if every grade landed on its own arm
    if (!all_named)
    {
        return false;
    }

    const bool correct_arms =
        ( (std::strcmp(sequential, "sequential") == 0) &&
          (std::strcmp(blocking,   "blocking")   == 0) &&
          (std::strcmp(lock_free,  "lock_free")  == 0) &&
          (std::strcmp(wait_free,  "wait_free")  == 0) );

    return correct_arms;
}

/*
tests_progress_has_exactly_four_grades
  the grade set is closed: four names, contiguous from zero.
  Tests the following:
  - the values 0..3 are all named
  - the value 4 is not - there is no fifth grade
  - so the total order the header states is an order over exactly four points,
    and `wait_free + 1` is not a grade
*/
bool
tests_progress_has_exactly_four_grades()
{
    const bool four_named =
        ( (progress_name(static_cast<progress>(0)) != nullptr) &&
          (progress_name(static_cast<progress>(1)) != nullptr) &&
          (progress_name(static_cast<progress>(2)) != nullptr) &&
          (progress_name(static_cast<progress>(3)) != nullptr) );

    const bool no_fifth =
        (progress_name(static_cast<progress>(4)) == nullptr);

    D_CONSTEXPR bool contiguous_from_zero =
        ( (underlying(progress::sequential) == 0) &&
          (underlying(progress::wait_free)  == 3) );

    return ( four_named           &&
             no_fifth             &&
             contiguous_from_zero );
}

/*
tests_progress_out_of_range_value_falls_through
  a value outside the four names is still a VALUE of the type.
  Tests the following:
  - the enum's value range is that of its underlying type, so casting 9 (or a
    negative) to it is well-defined, not undefined
  - such a value round-trips through the cast unchanged
  - and it falls through the switch to the default arm, which is why the
    default arm exists at all
*/
bool
tests_progress_out_of_range_value_falls_through()
{
    const progress unnamed  = static_cast<progress>(9);
    const progress negative = static_cast<progress>(-1);

    const bool value_survives =
        ( (underlying(unnamed)  == 9) &&
          (underlying(negative) == -1) );

    const bool falls_through =
        ( (progress_name(unnamed)  == nullptr) &&
          (progress_name(negative) == nullptr) );

    const bool still_ordered =
        ( (unnamed  > progress::wait_free) &&
          (negative < progress::sequential) );

    return ( value_survives &&
             falls_through  &&
             still_ordered );
}


NS_END  // testing
NS_END  // djinterp
