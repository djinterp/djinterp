/*******************************************************************************
* djinterp [core]                                sync_common_tests_ownership.cpp
*
*
* path:      /tests/djinterp/core/sync/sync_common_tests_ownership.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp [test]             sync_common_tests_ownership.cpp
//   Section I: the ownership bases and their macro forms.  noncopyable
// deletes copy but KEEPS move; nonmovable deletes both and pins the type at
// its address.  The macros must say exactly the same thing from inside a class
// body - and the interesting failure mode is silent: a bare deleted copy
// constructor suppresses the implicit move, so a naive D_NONCOPYABLE would
// quietly pin types that meant to stay movable.  The section holds base and
// macro against each other in both directions, and also checks the properties
// that make these usable as bases at all: they are empty (no size cost), they
// are not standalone objects (protected special members), and a derived type
// keeps its own construction and destruction.

// djinterp
#include "sync_common_tests.hpp"


NS_DJINTERP
NS_TESTING

/*
tests_noncopyable_blocks_copying
  inheriting noncopyable removes the copy operations.
  Tests the following:
  - a type derived from noncopyable is neither copy-constructible nor
    copy-assignable
  This is the whole purpose of the base: one line replacing two deleted
  members, so a unique resource cannot be duplicated by accident.
*/
bool
tests_noncopyable_blocks_copying()
{
    const bool no_copy =
        ( !std::is_copy_constructible<derived_noncopyable>::value &&
          !std::is_copy_assignable<derived_noncopyable>::value );

    static_assert(!std::is_copy_constructible<derived_noncopyable>::value,
                  "noncopyable must delete copy construction");

    return no_copy;
}

/*
tests_noncopyable_preserves_moving
  ... while leaving the move operations intact.
  Tests the following:
  - a type derived from noncopyable IS move-constructible and move-assignable
  This is the distinction from nonmovable, and it is easy to lose: had the
  base merely deleted the copy members without defaulting the move ones, the
  deleted copy constructor would have suppressed the implicit move and made
  every derived type immovable.  A unique-resource type that can still be
  relocated (returned from a factory, stored in a vector) depends on this.
*/
bool
tests_noncopyable_preserves_moving()
{
    const bool moves =
        ( std::is_move_constructible<derived_noncopyable>::value &&
          std::is_move_assignable<derived_noncopyable>::value );

    static_assert(std::is_move_constructible<derived_noncopyable>::value,
                  "noncopyable must preserve move construction");

    return moves;
}

/*
tests_nonmovable_blocks_copying
  inheriting nonmovable removes the copy operations too.
  Tests the following:
  - a type derived from nonmovable is neither copy-constructible nor
    copy-assignable
*/
bool
tests_nonmovable_blocks_copying()
{
    const bool no_copy =
        ( !std::is_copy_constructible<derived_nonmovable>::value &&
          !std::is_copy_assignable<derived_nonmovable>::value );

    static_assert(!std::is_copy_constructible<derived_nonmovable>::value,
                  "nonmovable must delete copy construction");

    return no_copy;
}

/*
tests_nonmovable_blocks_moving
  ... and the move operations as well, pinning the object.
  Tests the following:
  - a type derived from nonmovable is neither move-constructible nor
    move-assignable
  This is the contract for anything others hold a pointer or reference INTO -
  a mutex, an epoch slot, a registry - where relocating the object would leave
  those aliases dangling.  It is the property the whole sync module leans on,
  and until now every such type spelled it out by hand.
*/
bool
tests_nonmovable_blocks_moving()
{
    const bool no_move =
        ( !std::is_move_constructible<derived_nonmovable>::value &&
          !std::is_move_assignable<derived_nonmovable>::value );

    static_assert(!std::is_move_constructible<derived_nonmovable>::value,
                  "nonmovable must delete move construction");

    return no_move;
}

/*
tests_macro_noncopyable_matches_the_base
  D_NONCOPYABLE says exactly what the noncopyable base says.
  Tests the following:
  - a type using the macro is not copyable, and IS movable
  - each of those four trait answers equals the base's answer, term by term
  The two forms exist so a class that already has a base need not inherit;
  they are only useful if they are interchangeable, so the test compares them
  directly rather than restating the expected answers.
*/
bool
tests_macro_noncopyable_matches_the_base()
{
    const bool macro_is_right =
        ( !std::is_copy_constructible<macro_noncopyable>::value &&
          !std::is_copy_assignable<macro_noncopyable>::value    &&
          std::is_move_constructible<macro_noncopyable>::value  &&
          std::is_move_assignable<macro_noncopyable>::value );

    const bool agrees_with_base =
        ( (std::is_copy_constructible<macro_noncopyable>::value ==
           std::is_copy_constructible<derived_noncopyable>::value) &&
          (std::is_copy_assignable<macro_noncopyable>::value ==
           std::is_copy_assignable<derived_noncopyable>::value)    &&
          (std::is_move_constructible<macro_noncopyable>::value ==
           std::is_move_constructible<derived_noncopyable>::value) &&
          (std::is_move_assignable<macro_noncopyable>::value ==
           std::is_move_assignable<derived_noncopyable>::value) );

    static_assert(std::is_move_constructible<macro_noncopyable>::value,
                  "D_NONCOPYABLE must preserve move, as the base does");

    return (macro_is_right && agrees_with_base);
}

/*
tests_macro_nonmovable_matches_the_base
  D_NONMOVABLE says exactly what the nonmovable base says.
  Tests the following:
  - a type using the macro is neither copyable nor movable
  - each of those four trait answers equals the base's answer, term by term
*/
bool
tests_macro_nonmovable_matches_the_base()
{
    const bool macro_is_right =
        ( !std::is_copy_constructible<macro_nonmovable>::value &&
          !std::is_copy_assignable<macro_nonmovable>::value    &&
          !std::is_move_constructible<macro_nonmovable>::value &&
          !std::is_move_assignable<macro_nonmovable>::value );

    const bool agrees_with_base =
        ( (std::is_copy_constructible<macro_nonmovable>::value ==
           std::is_copy_constructible<derived_nonmovable>::value) &&
          (std::is_copy_assignable<macro_nonmovable>::value ==
           std::is_copy_assignable<derived_nonmovable>::value)    &&
          (std::is_move_constructible<macro_nonmovable>::value ==
           std::is_move_constructible<derived_nonmovable>::value) &&
          (std::is_move_assignable<macro_nonmovable>::value ==
           std::is_move_assignable<derived_nonmovable>::value) );

    static_assert(!std::is_move_constructible<macro_nonmovable>::value,
                  "D_NONMOVABLE must delete move, as the base does");

    return (macro_is_right && agrees_with_base);
}

/*
tests_bases_are_empty_and_cost_nothing
  inheriting an ownership base does not grow the derived type.
  Tests the following:
  - both bases are empty classes
  - a type derived from either is the same size as the identical type with no
    base at all
  The empty base optimization must apply, or a policy that is meant to be pure
  documentation would silently inflate every synchronization object that uses
  it - unacceptable in a module whose types sit in hot data structures.
*/
bool
tests_bases_are_empty_and_cost_nothing()
{
    const bool bases_empty =
        ( std::is_empty<noncopyable>::value &&
          std::is_empty<nonmovable>::value );

    const bool no_growth =
        ( (sizeof(derived_noncopyable) == sizeof(plain_probe)) &&
          (sizeof(derived_nonmovable) == sizeof(plain_probe)) );

    return (bases_empty && no_growth);
}

/*
tests_bases_are_not_standalone_objects
  the bases are for inheriting, not instantiating.
  Tests the following:
  - neither noncopyable nor nonmovable is default-constructible as a
    standalone object, because its special members are protected
  - a DERIVED type is default-constructible, which is what those protected
    members exist to allow
  Declaring a bare `noncopyable x;` is meaningless; the compiler should say so.
*/
bool
tests_bases_are_not_standalone_objects()
{
    const bool not_standalone =
        ( !std::is_default_constructible<noncopyable>::value &&
          !std::is_default_constructible<nonmovable>::value );

    const bool derived_is_constructible =
        ( std::is_default_constructible<derived_noncopyable>::value &&
          std::is_default_constructible<derived_nonmovable>::value );

    return (not_standalone && derived_is_constructible);
}

/*
tests_derived_types_stay_constructible
  the bases do not interfere with the derived type's own construction.
  Tests the following:
  - a derived object constructs, holds its own state, and can be mutated
  - inheriting a base does not DEGRADE the derived type's noexcept answer: it
    matches the identical type with no base at all
  Note the comparison is against a baseline rather than an outright nothrow
  claim, because the probe types here declare their own constructors without
  noexcept - so the honest question is whether the BASES cost anything, and
  they do not (their own constructors are noexcept and add no exception
  surface).  A policy base that quietly made its dependents throwing would
  ripple straight into the noexcept promises the sync types make.
*/
bool
tests_derived_types_stay_constructible()
{
    derived_noncopyable a;
    derived_nonmovable  b;

    a.value = 11;
    b.value = 22;

    const bool state_ok = ((a.value == 11) && (b.value == 22));

    const bool no_degradation =
        ( (std::is_nothrow_default_constructible<
               derived_noncopyable>::value ==
           std::is_nothrow_default_constructible<plain_probe>::value) &&
          (std::is_nothrow_default_constructible<
               derived_nonmovable>::value ==
           std::is_nothrow_default_constructible<plain_probe>::value) );

    return (state_ok && no_degradation);
}

/*
tests_derived_types_keep_their_destructor
  the bases' protected destructors still allow normal destruction.
  Tests the following:
  - a derived type is destructible, and nothrow-destructible
  A protected non-virtual destructor in the base is the right choice - it
  blocks deletion through a base pointer (which would be a slice) while
  leaving ordinary destruction of the derived object alone.
*/
bool
tests_derived_types_keep_their_destructor()
{
    const bool destructible =
        ( std::is_destructible<derived_noncopyable>::value &&
          std::is_destructible<derived_nonmovable>::value  &&
          std::is_nothrow_destructible<derived_noncopyable>::value &&
          std::is_nothrow_destructible<derived_nonmovable>::value );

    // and it really runs: a derived object leaving scope is well-formed
    {
        derived_noncopyable scoped;
        scoped.value = 1;
    }

    return destructible;
}


NS_END  // testing
NS_END  // djinterp
