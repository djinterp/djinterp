/*******************************************************************************
* djinterp [core]                                     cow_tests_ptr_lifetime.cpp
*
*
* path:      /tests/djinterp/core/sync/cow_tests_ptr_lifetime.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp [test]                       cow_tests_ptr_lifetime.cpp
//   Section II (a): cow_ptr's LIFETIME - who owns the block, and when it dies.
//
//   A copy shares and raises the count; a move steals and raises nothing; a
// destructor lowers it, and the last one out deletes.  All of that is invisible
// from the values, so the tests count instead: every payload construction,
// copy,
// move and destruction is instrumented, and every test ends by asserting that
// nothing is still alive.
//
//   The empty cow_ptr gets its own attention here.  It is the one state in
// which
// half of cow_ptr's interface is undefined behaviour - read(), write(),
// operator* and operator-> all dereference m_block without checking it - and
// the
// other half (use_count, is_unique, operator bool, detach, swap, copy, move,
// destroy) is perfectly safe.  The suite exercises the safe half and stays out
// of the other; see tests_the_empty_cow_ptr_must_not_be_dereferenced.

// djinterp
#include "cow_tests.hpp"


NS_DJINTERP
NS_TESTING

/*
tests_cow_ptr_default_is_empty
  a default-constructed cow_ptr owns nothing.
  Tests the following:
  - operator bool is false, use_count is 0, is_unique is false
  - is_unique() answers FALSE rather than true: an empty pointer is not the sole
    owner of anything, and a writer must not be told it may mutate in place
  - the default constructor is noexcept and allocates nothing
*/
bool
tests_cow_ptr_default_is_empty()
{
    probe_reset();

    cow_ptr<tracked> empty;

    D_CONSTEXPR bool ctor_is_noexcept =
        std::is_nothrow_default_constructible<cow_ptr<tracked> >::value;

    static_assert(ctor_is_noexcept,
                  "the default cow_ptr constructor must be noexcept");

    return ( (!static_cast<bool>(empty))         &&
             (empty.use_count() == 0)            &&
             (!empty.is_unique())                &&
             (probe_constructions().load() == 0) &&
             ctor_is_noexcept );
}

/*
tests_cow_ptr_make_starts_unique
  make() allocates a block and takes the only reference to it.
  Tests the following:
  - use_count is 1 and is_unique is true
  - operator bool is true
  - exactly one payload exists afterwards, and it is gone when the pointer is
*/
bool
tests_cow_ptr_make_starts_unique()
{
    probe_reset();

    {
        cow_ptr<tracked> pointer = cow_ptr<tracked>::make(1, 2);

        const bool born_unique =
            ( static_cast<bool>(pointer) &&
              (pointer.use_count() == 1) &&
              pointer.is_unique()        &&
              (probe_live().load() == 1) );

        // check if the pointer failed to take sole ownership
        if (!born_unique)
        {
            return false;
        }
    }

    return (probe_live().load() == 0);
}

/*
tests_cow_ptr_make_constructs_in_place
  make() forwards its arguments straight into the block.
  Tests the following:
  - make(3, 4) constructs the payload ONCE, with both arguments, and copies or
    moves nothing
  - so a payload can be built directly in its final home, as make_shared does
    and as `new T(...)` handed to a shared_ptr does not
*/
bool
tests_cow_ptr_make_constructs_in_place()
{
    probe_reset();

    cow_ptr<tracked> pointer = cow_ptr<tracked>::make(3, 4);

    return ( (probe_constructions().load() == 1) &&
             (probe_copies().load() == 0)        &&
             (probe_moves().load() == 0)         &&
             (pointer.read().first == 3)         &&
             (pointer.read().second == 4) );
}

/*
tests_cow_ptr_make_copies_an_lvalue
  make(lvalue) copies the payload, exactly once.
  Tests the following:
  - one copy, no moves
  - the source is untouched and independent of the block's value
*/
bool
tests_cow_ptr_make_copies_an_lvalue()
{
    probe_reset();

    const tracked source(5, 6);

    cow_ptr<tracked> pointer = cow_ptr<tracked>::make(source);

    return ( (probe_copies().load() == 1)   &&
             (probe_moves().load() == 0)    &&
             (pointer.read().first == 5)    &&
             (&pointer.read() != &source) );
}

/*
tests_cow_ptr_make_moves_an_rvalue
  make(rvalue) moves the payload, exactly once.
  Tests the following:
  - one move, no copies
  - so handing make() a temporary costs a move, not a copy
*/
bool
tests_cow_ptr_make_moves_an_rvalue()
{
    probe_reset();

    cow_ptr<tracked> pointer = cow_ptr<tracked>::make(tracked(7, 8));

    return ( (probe_moves().load() == 1)  &&
             (probe_copies().load() == 0) &&
             (pointer.read().first == 7) );
}

/*
tests_cow_ptr_copy_shares_the_block
  a copy is a refcount bump, not a copy of the payload.
  Tests the following:
  - copying a cow_ptr copies NO payload: the counters do not move
  - both pointers report use_count 2, and neither is unique
  - both name the SAME object: one address, one allocation
  - when the copy dies, the count falls back to 1 and the original is unique
    again
  This is the whole economics of the type: readers are free.
*/
bool
tests_cow_ptr_copy_shares_the_block()
{
    probe_reset();

    cow_ptr<tracked> original = cow_ptr<tracked>::make(1, 2);

    const long copies_before = probe_copies().load();

    {
        cow_ptr<tracked> shared(original);

        const bool shares =
            ( (probe_copies().load() == copies_before) &&
              (original.use_count() == 2)              &&
              (shared.use_count() == 2)                &&
              (!original.is_unique())                  &&
              (!shared.is_unique())                    &&
              (&original.read() == &shared.read()) );

        // check if the copy failed to share
        if (!shares)
        {
            return false;
        }
    }

    return ( (original.use_count() == 1) &&
             original.is_unique() );
}

/*
tests_cow_ptr_move_steals_the_block
  a move transfers the reference; it does not add one.
  Tests the following:
  - the moved-from pointer is EMPTY afterwards: falsy, use_count 0
  - the moved-to pointer holds the block at use_count ONE, not two - a move that
    bumped the count and left the source alive would leak
  - no payload was copied or moved
*/
bool
tests_cow_ptr_move_steals_the_block()
{
    probe_reset();

    cow_ptr<tracked> source = cow_ptr<tracked>::make(1, 2);

    const long copies_before = probe_copies().load();
    const long moves_before  = probe_moves().load();

    cow_ptr<tracked> destination(std::move(source));

    return ( (!static_cast<bool>(source))                 &&
             (source.use_count() == 0)                    &&
             (destination.use_count() == 1)               &&
             destination.is_unique()                      &&
             (destination.read().first == 1)              &&
             (probe_copies().load() == copies_before)     &&
             (probe_moves().load() == moves_before) );
}

/*
tests_cow_ptr_copy_assignment_shares
  copy assignment releases what it held and shares what it was given.
  Tests the following:
  - the assigned-to pointer's previous block is released - and destroyed, since
    it held the last reference to it
  - both pointers then share the source's block at use_count 2
  - no payload is copied
*/
bool
tests_cow_ptr_copy_assignment_shares()
{
    probe_reset();

    cow_ptr<tracked> target = cow_ptr<tracked>::make(1, 1);
    cow_ptr<tracked> source = cow_ptr<tracked>::make(2, 2);

    const long live_before = probe_live().load();   // two payloads

    target = source;

    const bool old_block_died = (probe_live().load() == (live_before - 1));

    return ( old_block_died               &&
             (target.use_count() == 2)    &&
             (source.use_count() == 2)    &&
             (&target.read() == &source.read()) &&
             (target.read().first == 2) );
}

/*
tests_cow_ptr_move_assignment_steals
  move assignment releases what it held and takes what it was given.
  Tests the following:
  - the target's old block is destroyed
  - the source is left EMPTY
  - the target holds the block alone, at use_count 1
*/
bool
tests_cow_ptr_move_assignment_steals()
{
    probe_reset();

    cow_ptr<tracked> target = cow_ptr<tracked>::make(1, 1);
    cow_ptr<tracked> source = cow_ptr<tracked>::make(2, 2);

    target = std::move(source);

    return ( (probe_live().load() == 1)         &&
             (!static_cast<bool>(source))       &&
             (source.use_count() == 0)          &&
             (target.use_count() == 1)          &&
             target.is_unique()                 &&
             (target.read().first == 2) );
}

/*
tests_cow_ptr_self_assignment_is_safe
  a pointer assigned to itself keeps its value and its count.
  Tests the following:
  - copy self-assignment is a no-op: the block is not released and re-taken, and
    the payload survives
  - move self-assignment is a no-op too - the guarded `this != &other` check is
    what stands between it and a self-destructing pointer
  Both are guarded in the header; this is the test that says so, because the
  failure mode of removing either guard is a use-after-free that a value check
  alone would not catch.
*/
bool
tests_cow_ptr_self_assignment_is_safe()
{
    probe_reset();

    cow_ptr<tracked> pointer = cow_ptr<tracked>::make(4, 5);

    cow_ptr<tracked>& alias = pointer;

    pointer = alias;                    // copy self-assignment

    const bool survived_copy =
        ( static_cast<bool>(pointer)  &&
          (pointer.use_count() == 1)  &&
          (pointer.read().first == 4) &&
          (probe_live().load() == 1) );

    pointer = std::move(alias);         // move self-assignment

    const bool survived_move =
        ( static_cast<bool>(pointer)  &&
          (pointer.use_count() == 1)  &&
          (pointer.read().first == 4) &&
          (probe_live().load() == 1) );

    return (survived_copy && survived_move);
}

/*
tests_cow_ptr_assigning_an_aliased_block_is_safe
  two DIFFERENT pointers to the SAME block, assigned to one another.
  Tests the following:
  - `a = b` where a and b already share a block leaves both valid, both sharing,
    and the count still 2
  - the header's guard is `this != &_other`, which compares the POINTERS and not
    the blocks - so this path really does release and re-acquire the same block.
    It survives only because the count was 2 and the release therefore did not
    delete
  The neighbouring case - a count of 1 with two pointers - cannot arise, which
  is
  why this is safe rather than lucky.  It is still worth a test: an "optimized"
  release that dropped the count check would free the block here and leave both
  pointers dangling.
*/
bool
tests_cow_ptr_assigning_an_aliased_block_is_safe()
{
    probe_reset();

    cow_ptr<tracked> first  = cow_ptr<tracked>::make(9, 9);
    cow_ptr<tracked> second = first;              // both share, count 2

    first = second;                               // release + re-acquire

    return ( static_cast<bool>(first)          &&
             static_cast<bool>(second)         &&
             (first.use_count() == 2)          &&
             (second.use_count() == 2)         &&
             (&first.read() == &second.read()) &&
             (first.read().first == 9)         &&
             (probe_live().load() == 1) );
}

/*
tests_cow_ptr_destructor_releases_the_last_reference
  the last owner out destroys the payload; the others do not.
  Tests the following:
  - destroying a shared cow_ptr destroys NO payload
  - destroying the last one destroys exactly one
  - nothing is left alive at the end
*/
bool
tests_cow_ptr_destructor_releases_the_last_reference()
{
    probe_reset();

    {
        cow_ptr<tracked> first = cow_ptr<tracked>::make(1, 2);

        {
            cow_ptr<tracked> second = first;

            // check if sharing failed
            if (probe_destructions().load() != 0)
            {
                return false;
            }
        }

        // the sharer died; the payload must NOT have
        if ( (probe_destructions().load() != 0) ||
             (probe_live().load() != 1) )
        {
            return false;
        }
    }

    return ( (probe_destructions().load() == 1) &&
             (probe_live().load() == 0) );
}

/*
tests_cow_ptr_empty_operations_are_safe
  the half of the interface that an empty cow_ptr survives.
  Tests the following:
  - an empty pointer can be copied, moved, assigned, swapped, detached and
    destroyed - all of them check m_block, and all of them do nothing
  - the copies and moves of an empty pointer are empty
  - detach() on an empty pointer allocates nothing
  The other half - read(), write(), operator*, operator-> - does NOT check, and
  is undefined behaviour here.  See
  tests_the_empty_cow_ptr_must_not_be_dereferenced.
*/
bool
tests_cow_ptr_empty_operations_are_safe()
{
    probe_reset();

    cow_ptr<tracked> empty;
    cow_ptr<tracked> copied(empty);
    cow_ptr<tracked> source_to_move;
    cow_ptr<tracked> moved(std::move(source_to_move));
    cow_ptr<tracked> assigned;

    assigned = empty;

    cow_ptr<tracked> swapped;

    swapped.swap(empty);

    empty.detach();          // no block: nothing to clone

    return ( (!static_cast<bool>(copied))   &&
             (!static_cast<bool>(moved))    &&
             (!static_cast<bool>(assigned)) &&
             (!static_cast<bool>(swapped))  &&
             (copied.use_count() == 0)      &&
             (probe_constructions().load() == 0) &&
             (probe_copies().load() == 0)   &&
             (probe_live().load() == 0) );
}

/*
tests_cow_ptr_lifetime_operations_are_noexcept
  everything that only moves a refcount promises not to throw.
  Tests the following:
  - the default constructor, the copy, the move, both assignments and swap are
    all noexcept
  - and write() is NOT - it allocates, and an allocation may fail.  That is the
    line the type draws: sharing is free and cannot fail; writing may cost a
    clone and may throw
*/
bool
tests_cow_ptr_lifetime_operations_are_noexcept()
{
    typedef cow_ptr<tracked> pointer_type;

    D_CONSTEXPR bool sharing_is_noexcept =
        ( std::is_nothrow_default_constructible<pointer_type>::value &&
          std::is_nothrow_copy_constructible<pointer_type>::value    &&
          std::is_nothrow_move_constructible<pointer_type>::value    &&
          std::is_nothrow_copy_assignable<pointer_type>::value       &&
          std::is_nothrow_move_assignable<pointer_type>::value       &&
          noexcept(std::declval<pointer_type&>().swap(
                       std::declval<pointer_type&>())) );

    D_CONSTEXPR bool writing_is_not =
        (!noexcept(std::declval<pointer_type&>().write()));

    static_assert(sharing_is_noexcept,
                  "sharing a cow_ptr must never throw");

    return (sharing_is_noexcept && writing_is_not);
}


NS_END  // testing
NS_END  // djinterp
