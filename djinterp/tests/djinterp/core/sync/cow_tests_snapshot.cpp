/*******************************************************************************
* djinterp [core]                                         cow_tests_snapshot.cpp
*
*
* path:      /tests/djinterp/core/sync/cow_tests_snapshot.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp [test]                          cow_tests_snapshot.cpp
//   Section III: immutable_snapshot - a frozen view that costs an atomic
// increment.
//
//   The class has FOUR constructors, and two of them do completely different
// things while looking almost identical at the call site:
//
//       immutable_snapshot<T> a(value);      // COPIES the value into a new
// block
//       immutable_snapshot<T> b(cow_pointer); // SHARES the block. Zero copies.
//
//   The first is a deep copy wearing a snapshot's clothes; the second is the
// cheap thing the class exists for.  Only the counters can tell them apart, and
// the tests below do.
//
//   Everything else in the class follows from holding a cow_ptr: copies share,
// moves steal, the data outlives its source, and there is no way to mutate what
// it points at.  There is also no way to make an EMPTY one - the class has no
// default constructor - unless you hand it an empty cow_ptr, which it will
// accept without complaint.

// djinterp
#include "cow_tests.hpp"


NS_DJINTERP
NS_TESTING

/*
tests_snapshot_from_a_value_copies_it
  the value constructor is a DEEP COPY, not a share.
  Tests the following:
  - immutable_snapshot(value) copies the payload into a brand-new block
  - the snapshot's use_count is 1: it shares with nobody, not even the object it
    was made from
  - so this constructor costs an allocation and a copy, and is the expensive one
*/
bool
tests_snapshot_from_a_value_copies_it()
{
    probe_reset();

    const tracked source(1, 2);

    immutable_snapshot<tracked> snapshot(source);

    return ( (probe_copies().load() == 1)   &&
             (snapshot.use_count() == 1)    &&
             (snapshot.get().first == 1)    &&
             (&snapshot.get() != &source) );
}

/*
tests_snapshot_from_a_cow_ptr_is_zero_copy
  the cow_ptr constructor SHARES.  This is the one the class is for.
  Tests the following:
  - immutable_snapshot(cow_ptr) copies NO payload: the counters do not move
  - the source pointer and the snapshot both report use_count 2
  - both name the same object, at the same address
  - "cheap to create (one atomic increment)", as the banner says - and here is
    the increment, and here is the absence of the copy
*/
bool
tests_snapshot_from_a_cow_ptr_is_zero_copy()
{
    probe_reset();

    cow_ptr<tracked> source = cow_ptr<tracked>::make(3, 4);

    const long copies_before = probe_copies().load();

    immutable_snapshot<tracked> snapshot(source);

    return ( (probe_copies().load() == copies_before) &&
             (probe_moves().load() == 0)              &&
             (source.use_count() == 2)                &&
             (snapshot.use_count() == 2)              &&
             (&snapshot.get() == &source.read())      &&
             (snapshot.get().first == 3) );
}

/*
tests_snapshot_version_defaults_to_zero
  a snapshot with no version stamp carries zero.
  Tests the following:
  - both one-argument constructors leave version() at 0
  - so an unstamped snapshot is indistinguishable from one taken at the
    beginning of time, and a consumer comparing versions must know which
    constructor made it
*/
bool
tests_snapshot_version_defaults_to_zero()
{
    cow_ptr<int> pointer = cow_ptr<int>::make(1);

    immutable_snapshot<int> from_value(7);
    immutable_snapshot<int> from_pointer(pointer);

    return ( (from_value.version() == 0) &&
             (from_pointer.version() == 0) );
}

/*
tests_snapshot_carries_the_version_it_was_given
  the two-argument constructors stamp it.
  Tests the following:
  - the version is stored and handed back unchanged, from both the value form
    and the cow_ptr form
  - it is a std::uint64_t, so it will not wrap in any program that finishes
  - version() is const and noexcept
*/
bool
tests_snapshot_carries_the_version_it_was_given()
{
    cow_ptr<int> pointer = cow_ptr<int>::make(1);

    const std::uint64_t stamp = 42;

    immutable_snapshot<int> from_value(7, stamp);
    immutable_snapshot<int> from_pointer(pointer, stamp + 1);

    D_CONSTEXPR bool version_is_64_bit =
        std::is_same<decltype(std::declval<
                         const immutable_snapshot<int>&>().version()),
                     std::uint64_t>::value;

    D_CONSTEXPR bool version_is_noexcept =
        noexcept(std::declval<const immutable_snapshot<int>&>().version());

    static_assert(version_is_64_bit,
                  "the version stamp must be a uint64");

    return ( version_is_64_bit             &&
             version_is_noexcept           &&
             (from_value.version() == 42)  &&
             (from_pointer.version() == 43) );
}

/*
tests_snapshot_get_arrow_and_star
  three spellings, one object.
  Tests the following:
  - get(), operator-> and operator* all reach the same payload at the same
    address
  - all three are const and noexcept
*/
bool
tests_snapshot_get_arrow_and_star()
{
    immutable_snapshot<tracked> snapshot(tracked(5, 6));

    D_CONSTEXPR bool all_noexcept =
        ( noexcept(std::declval<const immutable_snapshot<tracked>&>().get()) &&
          noexcept(std::declval<
                       const immutable_snapshot<tracked>&>().operator->()) &&
          noexcept(*std::declval<const immutable_snapshot<tracked>&>()) );

    static_assert(all_noexcept,
                  "the snapshot accessors must be noexcept");

    return ( all_noexcept                          &&
             (snapshot.get().first == 5)           &&
             (snapshot->second == 6)               &&
             ((*snapshot).first == 5)              &&
             (&snapshot.get() == snapshot.operator->()) &&
             (&snapshot.get() == &(*snapshot)) );
}

/*
tests_snapshot_exposes_only_const_access
  a snapshot cannot be written through.  That is what "immutable" means here.
  Tests the following:
  - get(), operator-> and operator* all return CONST access, from a const
    snapshot AND from a non-const one - there is no mutable overload anywhere
  - so no holder of a snapshot can reach the shared block and change it under
    the others, which is what makes sharing safe
  Note what is NOT claimed: the snapshot OBJECT is assignable (its copy and move
  assignment are defaulted), so a snapshot variable can be made to point at a
  different frozen value.  It is the DATA that is immutable, not the handle.
*/
bool
tests_snapshot_exposes_only_const_access()
{
    typedef immutable_snapshot<tracked> snapshot_type;

    D_CONSTEXPR bool const_even_when_mutable =
        ( std::is_same<decltype(std::declval<snapshot_type&>().get()),
                       const tracked&>::value &&
          std::is_same<decltype(std::declval<snapshot_type&>().operator->()),
                       const tracked*>::value &&
          std::is_same<decltype(*std::declval<snapshot_type&>()),
                       const tracked&>::value );

    // ...and the handle itself is assignable, which is a different thing
    D_CONSTEXPR bool the_handle_is_assignable =
        ( std::is_copy_assignable<snapshot_type>::value &&
          std::is_move_assignable<snapshot_type>::value );

    static_assert(const_even_when_mutable,
                  "a snapshot must offer no mutable path to its data");

    return (const_even_when_mutable && the_handle_is_assignable);
}

/*
tests_snapshot_copy_shares_the_block
  copying a snapshot is an atomic increment.
  Tests the following:
  - the copy copies no payload
  - both snapshots report the raised count, and name the same object
  - "cheap to copy (shared)", as the banner says
*/
bool
tests_snapshot_copy_shares_the_block()
{
    probe_reset();

    cow_ptr<tracked> source = cow_ptr<tracked>::make(1, 2);

    immutable_snapshot<tracked> first(source);

    const long copies_before = probe_copies().load();

    immutable_snapshot<tracked> second(first);

    return ( (probe_copies().load() == copies_before) &&
             (first.use_count() == 3)                 &&
             (second.use_count() == 3)                &&
             (source.use_count() == 3)                &&
             (&first.get() == &second.get()) );
}

/*
tests_snapshot_move_steals_the_block
  moving a snapshot transfers the reference without raising it.
  Tests the following:
  - the move copies no payload and adds no reference: the count is what it was
  - the moved-from snapshot is left EMPTY - falsy, use_count 0.  Its defaulted
    move constructor moves the cow_ptr, and cow_ptr's move empties its source
  - so a moved-from snapshot is one of the few ways to obtain a snapshot whose
    get() is undefined behaviour.  It is falsy, and it must be checked before it
    is read
*/
bool
tests_snapshot_move_steals_the_block()
{
    probe_reset();

    cow_ptr<tracked> source = cow_ptr<tracked>::make(1, 2);

    immutable_snapshot<tracked> first(source, 9);

    const long copies_before = probe_copies().load();
    const std::size_t count_before = first.use_count();

    immutable_snapshot<tracked> second(std::move(first));

    return ( (probe_copies().load() == copies_before) &&
             (second.use_count() == count_before)     &&
             (second.version() == 9)                  &&
             (second.get().first == 1)                &&
             (!static_cast<bool>(first))              &&
             (first.use_count() == 0) );
}

/*
tests_snapshot_is_not_default_constructible
  there is no such thing as an empty snapshot - not directly.
  Tests the following:
  - immutable_snapshot has NO default constructor: the four explicit ones
    suppress it, and none was defaulted back
  - so a snapshot cannot be declared and filled in later; it must be constructed
    from something
  - which also means it cannot be an uninitialized member, nor an element of a
    default-constructed array or a resized vector
*/
bool
tests_snapshot_is_not_default_constructible()
{
    D_CONSTEXPR bool no_default =
        !std::is_default_constructible<immutable_snapshot<int> >::value;

    // ...but it IS copy-constructible and movable, so a container of them works
    // as long as every element is given a value
    D_CONSTEXPR bool copyable_and_movable =
        ( std::is_copy_constructible<immutable_snapshot<int> >::value &&
          std::is_move_constructible<immutable_snapshot<int> >::value );

    static_assert(no_default,
                  "a snapshot must be constructed from something");

    return (no_default && copyable_and_movable);
}

/*
tests_snapshot_outlives_its_source
  the snapshot owns its data, and does not merely borrow it.
  Tests the following:
  - a snapshot is taken from a cow_ptr, and the cow_ptr is then destroyed
  - the payload is NOT destroyed: the snapshot held a reference, and the count
    fell from 2 to 1 rather than to 0
  - the snapshot still reads the value
  This is what makes a snapshot safe to hand across a thread boundary, or to
  hold long after the container it came from has moved on.
*/
bool
tests_snapshot_outlives_its_source()
{
    probe_reset();

    immutable_snapshot<tracked>* snapshot = nullptr;

    {
        cow_ptr<tracked> source = cow_ptr<tracked>::make(8, 9);

        snapshot = new immutable_snapshot<tracked>(source, 3);
    }

    // the source is gone; the payload must not be
    const bool survived =
        ( (probe_destructions().load() == 0) &&
          (probe_live().load() == 1)         &&
          (snapshot->use_count() == 1)       &&
          (snapshot->get().first == 8)       &&
          (snapshot->version() == 3) );

    delete snapshot;

    return ( survived                           &&
             (probe_destructions().load() == 1) &&
             (probe_live().load() == 0) );
}

/*
tests_snapshot_from_an_empty_cow_ptr_is_falsy
  the class will happily wrap nothing.
  Tests the following:
  - a snapshot constructed from an EMPTY cow_ptr is accepted, and is falsy
  - its use_count is 0
  - and its get() would be undefined behaviour, exactly as cow_ptr::read() is on
    an empty pointer - which is why operator bool exists on the snapshot at all
  The suite does not dereference it.  A caller must not either.
*/
bool
tests_snapshot_from_an_empty_cow_ptr_is_falsy()
{
    cow_ptr<tracked> empty;

    immutable_snapshot<tracked> snapshot(empty);

    return ( (!static_cast<bool>(snapshot)) &&
             (snapshot.use_count() == 0)    &&
             (snapshot.version() == 0) );
}

/*
tests_snapshot_use_count
  the count a snapshot reports is the block's, not its own.
  Tests the following:
  - one snapshot of a cow_ptr: 2 (the pointer and the snapshot)
  - a second snapshot of the same pointer: 3
  - after the pointer dies: 2, the two snapshots between them
  - after one snapshot dies: 1
  - use_count is const and noexcept
*/
bool
tests_snapshot_use_count()
{
    D_CONSTEXPR bool is_noexcept =
        noexcept(std::declval<const immutable_snapshot<int>&>().use_count());

    static_assert(is_noexcept, "use_count must be noexcept");

    cow_ptr<int>* pointer = new cow_ptr<int>(cow_ptr<int>::make(1));

    immutable_snapshot<int>  first(*pointer);
    immutable_snapshot<int>* second = new immutable_snapshot<int>(*pointer);

    const bool three_owners =
        ( (first.use_count() == 3) &&
          (second->use_count() == 3) );

    delete pointer;

    const bool two_owners = (first.use_count() == 2);

    delete second;

    return ( three_owners            &&
             two_owners              &&
             (first.use_count() == 1) &&
             is_noexcept );
}


NS_END  // testing
NS_END  // djinterp
