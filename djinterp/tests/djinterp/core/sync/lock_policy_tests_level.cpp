/*******************************************************************************
* djinterp [core]                                    lock_policy_tests_level.cpp
*
*
* path:      /tests/djinterp/core/sync/lock_policy_tests_level.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp [test]                      lock_policy_tests_level.cpp
//   Section I: thread_safety_leavel - the ordered level enum.
//
//   The enum ranks the thread-safety guarantees from none up to shared_timed,
// and its ORDERING is load-bearing: the selector and the trait system compare
// levels, and the banner promises each level is a strict superset of the one
// below.  So the suite pins the six values, their strict ascending order, that
// it is a scoped enum over int (no accidental integer conversions), and the one
// wart worth remembering - the enum's name is MISSPELLED
// (thread_safety_leavel),
// with a correctly spelled alias (thread_safety_level) naming the same type.
//
//   All of this exists from C++11 (the enum is `enum class`); below that the
// header uses a struct-with-nested-enum instead, which is out of the suite's
// C++11-floored scope.

// djinterp
#include "lock_policy_tests.hpp"


NS_DJINTERP
NS_TESTING

/*
tests_level_has_the_six_named_values
  the enum names exactly the six documented levels, at 0 through 5.
  Tests the following:
  - none is 0, atomic_only 1, exclusive 2, timed 3, shared 4, shared_timed 5
  - so the values are the contiguous run 0..5 the header lays out
  The integer values matter because the levels are compared and, in the trait
  layer, sometimes stored; a renumbering would silently reorder the hierarchy.
*/
bool
tests_level_has_the_six_named_values()
{
    const bool values_ok =
        ( (static_cast<int>(thread_safety_leavel::none) == 0)        &&
          (static_cast<int>(thread_safety_leavel::atomic_only) == 1) &&
          (static_cast<int>(thread_safety_leavel::exclusive) == 2)   &&
          (static_cast<int>(thread_safety_leavel::timed) == 3)       &&
          (static_cast<int>(thread_safety_leavel::shared) == 4)      &&
          (static_cast<int>(thread_safety_leavel::shared_timed) == 5) );

    return values_ok;
}

/*
tests_level_ordering_is_a_strict_hierarchy
  the levels ascend strictly: none < atomic_only < ... < shared_timed.
  Tests the following:
  - each level compares strictly less than the next, as integers
  - so ordering comparisons on the enum express the "strict superset" hierarchy
    the header describes
  This is what lets code ask "is this level at least `exclusive`?" by a single
  comparison.
*/
bool
tests_level_ordering_is_a_strict_hierarchy()
{
    typedef thread_safety_leavel lvl;
    const int none         = static_cast<int>(lvl::none);
    const int atomic_only  = static_cast<int>(lvl::atomic_only);
    const int exclusive    = static_cast<int>(lvl::exclusive);
    const int timed        = static_cast<int>(lvl::timed);
    const int shared       = static_cast<int>(lvl::shared);
    const int shared_timed = static_cast<int>(lvl::shared_timed);

    const bool strictly_ascending =
        ( (none < atomic_only)   &&
          (atomic_only < exclusive) &&
          (exclusive < timed)    &&
          (timed < shared)       &&
          (shared < shared_timed) );

    return strictly_ascending;
}

/*
tests_level_is_a_scoped_enum_over_int
  the level is an `enum class`, underlain by int.
  Tests the following:
  - it is an enumeration type
  - it does NOT implicitly convert to int (a scoped enum requires a cast) - so a
    level cannot be silently mixed into arithmetic or passed where an int is
    expected
  - its underlying type is int (the default for an unfixed scoped enum)
  The scoping is a safety property: it keeps the six level constants from
  colliding with anything and from decaying to integers by accident.
*/
bool
tests_level_is_a_scoped_enum_over_int()
{
    D_CONSTEXPR bool is_enum =
        std::is_enum<thread_safety_leavel>::value;

    D_CONSTEXPR bool not_implicitly_int =
        !std::is_convertible<thread_safety_leavel, int>::value;

    D_CONSTEXPR bool underlying_is_int =
        std::is_same<
            std::underlying_type<thread_safety_leavel>::type, int>::value;

    static_assert(is_enum && not_implicitly_int,
                  "thread_safety_leavel must be a scoped enum");

    return (is_enum && not_implicitly_int && underlying_is_int);
}

/*
tests_the_correctly_spelled_alias_names_the_same_enum
  the wart: the enum is misspelled, and an alias rescues the spelling.
  Tests the following:
  - thread_safety_level (spelled correctly) and thread_safety_leavel (the
    actual,
    misspelled enum name) are the SAME type
  - so code may use either spelling interchangeably
  The primary name carries the typo, and the policies' `level` members are typed
  by it; the alias exists so downstream code need not repeat the misspelling.
  Pinned so the duplication is on record and neither spelling can quietly drift
  from the other.
*/
bool
tests_the_correctly_spelled_alias_names_the_same_enum()
{
    D_CONSTEXPR bool same =
        std::is_same<thread_safety_level, thread_safety_leavel>::value;

    static_assert(same,
                  "thread_safety_level must alias thread_safety_leavel");

    return same;
}

/*
tests_level_is_usable_in_a_constant_expression
  the levels are compile-time constants.
  Tests the following:
  - a level can be used to initialize a constexpr and drive a compile-time
    branch (here, a static_assert on a value derived from one)
  - so the enum is fit for the template-parameter role the selector puts it in:
    select_lock_policy<thread_safety_leavel::timed> is a compile-time mapping
  This is the property that makes the whole selector possible.
*/
bool
tests_level_is_usable_in_a_constant_expression()
{
    D_CONSTEXPR thread_safety_leavel picked = thread_safety_leavel::timed;

    D_CONSTEXPR bool is_timed_slot =
        (static_cast<int>(picked) == 3);

    static_assert(is_timed_slot,
                  "a level must be usable in a constant expression");

    // and as a non-type template argument (the selector's actual use)
    D_CONSTEXPR bool resolves =
        (sizeof(select_lock_policy<picked>) > 0);

    return (is_timed_slot && resolves);
}


NS_END  // testing
NS_END  // djinterp
