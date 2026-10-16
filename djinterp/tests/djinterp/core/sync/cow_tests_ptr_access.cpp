/*******************************************************************************
* djinterp [core]                                       cow_tests_ptr_access.cpp
*
*
* path:      /tests/djinterp/core/sync/cow_tests_ptr_access.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp [test]                        cow_tests_ptr_access.cpp
//   Section II (b): cow_ptr's ACCESS - and the one decision the whole type
// exists to make.
//
//   read(), operator* and operator-> are const and never clone.  write() is not
// const and clones IF, AND ONLY IF, the block is shared.  That "if and only if"
// is the entire contract, and it is unobservable from the values: the payload
// reads the same before and after a clone.  So the tests below watch the two
// things that DO change - the address of the payload, and the copy counter -
// and assert on those instead.
//
//   The empty cow_ptr is the trap at the end of the file.  read(), write(),
// operator* and operator-> all dereference m_block without checking it, so on a
// default-constructed pointer every one of them is undefined behaviour.  The
// suite pins the precondition rather than triggering it.

// djinterp
#include "cow_tests.hpp"


NS_DJINTERP
NS_TESTING

/*
tests_cow_ptr_read_returns_the_value
  read() hands back the payload, by const reference.
  Tests the following:
  - the value is the one that was constructed
  - the return type is a CONST reference: a reader cannot mutate through it, and
    so cannot mutate a block it may be sharing
  - read() is noexcept and const
*/
bool
tests_cow_ptr_read_returns_the_value()
{
    cow_ptr<tracked> pointer = cow_ptr<tracked>::make(11, 22);

    D_CONSTEXPR bool returns_const_reference =
        std::is_same<decltype(std::declval<const cow_ptr<tracked>&>().read()),
                     const tracked&>::value;

    D_CONSTEXPR bool is_noexcept =
        noexcept(std::declval<const cow_ptr<tracked>&>().read());

    static_assert(returns_const_reference,
                  "read() must hand back a const reference");

    return ( returns_const_reference     &&
             is_noexcept                 &&
             (pointer.read().first == 11) &&
             (pointer.read().second == 22) );
}

/*
tests_cow_ptr_arrow_and_star_reach_the_value
  operator-> and operator* are read()'s two other spellings.
  Tests the following:
  - all three name the same object, at the same address
  - both operators are const-qualified and hand back const access
  - so there is no mutable path through any of them, by design
*/
bool
tests_cow_ptr_arrow_and_star_reach_the_value()
{
    cow_ptr<tracked> pointer = cow_ptr<tracked>::make(3, 4);

    D_CONSTEXPR bool arrow_is_const =
        std::is_same<
            decltype(std::declval<const cow_ptr<tracked>&>().operator->()),
            const tracked*>::value;

    D_CONSTEXPR bool star_is_const =
        std::is_same<
            decltype(*std::declval<const cow_ptr<tracked>&>()),
            const tracked&>::value;

    static_assert(arrow_is_const && star_is_const,
                  "operator-> and operator* must be const paths");

    return ( arrow_is_const               &&
             star_is_const                &&
             (pointer->first == 3)        &&
             ((*pointer).second == 4)     &&
             (&pointer.read() == &(*pointer)) &&
             (&pointer.read() == pointer.operator->()) );
}

/*
tests_cow_ptr_const_access_never_clones
  reading a SHARED block does not clone it - however many times you read.
  Tests the following:
  - two pointers share one block; a hundred reads through both, by all three
    spellings, copy no payload at all
  - the count stays at 2 and the address never moves
  This is the promise that makes readers free.  A const path that cloned would
  turn every reader into a writer and every read into an allocation.
*/
bool
tests_cow_ptr_const_access_never_clones()
{
    probe_reset();

    cow_ptr<tracked> first  = cow_ptr<tracked>::make(1, 2);
    cow_ptr<tracked> second = first;

    const tracked* const address = &first.read();
    const long copies_before     = probe_copies().load();
    long       sum               = 0;
    int        i                 = 0;

    for (i = 0; i < 100; ++i)
    {
        sum += first.read().first;
        sum += second->second;
        sum += (*first).first;
    }

    return ( (probe_copies().load() == copies_before) &&
             (first.use_count() == 2)                 &&
             (&first.read() == address)               &&
             (&second.read() == address)              &&
             (sum == 400) );
}

/*
tests_cow_ptr_write_when_unique_does_not_clone
  a sole owner writes in place.
  Tests the following:
  - write() on a unique pointer copies NOTHING: the counters do not move
  - the payload stays at the same address - the block was not reallocated
  - the mutation lands, and the count is still 1
  If this cloned, copy-on-write would be copy-on-every-write, and the type would
  be a very expensive unique_ptr.
*/
bool
tests_cow_ptr_write_when_unique_does_not_clone()
{
    probe_reset();

    cow_ptr<tracked> pointer = cow_ptr<tracked>::make(1, 2);

    const tracked* const before  = &pointer.read();
    const long copies_before     = probe_copies().load();

    pointer.write().first = 42;

    return ( (probe_copies().load() == copies_before) &&
             (&pointer.read() == before)              &&
             (pointer.read().first == 42)             &&
             (pointer.use_count() == 1)               &&
             pointer.is_unique() );
}

/*
tests_cow_ptr_write_when_shared_clones
  a shared owner clones before it writes.  THIS IS THE TYPE.
  Tests the following:
  - two pointers share a block; one of them calls write()
  - the payload is copied EXACTLY ONCE
  - the writer now names a DIFFERENT address, and is unique again
  - the reader still names the old one, and is unique too - the count that was 2
    is now two counts of 1
  - the writer's mutation is visible only to the writer
*/
bool
tests_cow_ptr_write_when_shared_clones()
{
    probe_reset();

    cow_ptr<tracked> reader = cow_ptr<tracked>::make(1, 2);
    cow_ptr<tracked> writer = reader;

    const tracked* const shared_address = &reader.read();
    const long copies_before            = probe_copies().load();

    writer.write().first = 99;

    return ( (probe_copies().load() == (copies_before + 1)) &&
             (&writer.read() != shared_address)             &&
             (&reader.read() == shared_address)             &&
             (writer.read().first == 99)                    &&
             (reader.read().first == 1)                     &&
             (writer.use_count() == 1)                      &&
             (reader.use_count() == 1)                      &&
             writer.is_unique()                             &&
             reader.is_unique() );
}

/*
tests_cow_ptr_write_leaves_the_other_holder_untouched
  the clone is the writer's business, and nobody else's.
  Tests the following:
  - THREE pointers share a block; one writes
  - the writer detaches with its own copy; the other two still share the
    original, at count 2, at the original address, with the original value
  - so a write costs one clone regardless of how many readers there are, and
    costs the readers nothing
*/
bool
tests_cow_ptr_write_leaves_the_other_holder_untouched()
{
    probe_reset();

    cow_ptr<tracked> first  = cow_ptr<tracked>::make(7, 7);
    cow_ptr<tracked> second = first;
    cow_ptr<tracked> third  = first;

    const tracked* const original = &first.read();

    third.write().first = 8;

    return ( (probe_copies().load() == 1)     &&
             (first.use_count() == 2)         &&
             (second.use_count() == 2)        &&
             (third.use_count() == 1)         &&
             (&first.read() == original)      &&
             (&second.read() == original)     &&
             (&third.read() != original)      &&
             (first.read().first == 7)        &&
             (second.read().first == 7)       &&
             (third.read().first == 8) );
}

/*
tests_cow_ptr_write_clones_exactly_once
  the clone happens on the FIRST write, and not again.
  Tests the following:
  - a shared pointer's first write clones
  - its second, third and hundredth writes do not - it is unique now, and knows
    it
  - so the cost of copy-on-write is paid once per sharing, not once per
    mutation.  A container that writes a thousand times after one snapshot pays
    for one copy
*/
bool
tests_cow_ptr_write_clones_exactly_once()
{
    probe_reset();

    cow_ptr<tracked> reader = cow_ptr<tracked>::make(0, 0);
    cow_ptr<tracked> writer = reader;

    writer.write().first = 1;

    const long after_first_write = probe_copies().load();

    const tracked* const detached = &writer.read();
    int i = 0;

    for (i = 2; i <= 100; ++i)
    {
        writer.write().first = i;
    }

    return ( (after_first_write == 1)              &&
             (probe_copies().load() == 1)          &&
             (&writer.read() == detached)          &&
             (writer.read().first == 100)          &&
             (reader.read().first == 0) );
}

/*
tests_cow_ptr_write_copies_the_value_exactly_once
  the clone is one copy construction - not two, and not a move.
  Tests the following:
  - the clone that write() performs is a COPY of the payload: one copy
    construction, no moves, no extra temporaries
  - the old block is not destroyed (a reader still holds it), so the payload
    count goes UP by one and stays there
*/
bool
tests_cow_ptr_write_copies_the_value_exactly_once()
{
    probe_reset();

    cow_ptr<tracked> reader = cow_ptr<tracked>::make(1, 1);
    cow_ptr<tracked> writer = reader;

    const long live_before = probe_live().load();

    writer.write().second = 2;

    return ( (probe_copies().load() == 1)             &&
             (probe_moves().load() == 0)              &&
             (probe_destructions().load() == 0)       &&
             (probe_live().load() == (live_before + 1)) );
}

/*
tests_cow_ptr_is_unique_and_use_count
  the two queries a writer consults.
  Tests the following:
  - an empty pointer: use_count 0, is_unique FALSE
  - a fresh pointer: 1 and TRUE
  - a shared pointer: 2 and FALSE, from both ends
  - after the sharer dies: 1 and TRUE again
  - both queries are const and noexcept, and both are null-safe - unlike the
    accessors
*/
bool
tests_cow_ptr_is_unique_and_use_count()
{
    cow_ptr<int> empty;
    cow_ptr<int> pointer = cow_ptr<int>::make(1);

    const bool empty_is_nobodys =
        ( (empty.use_count() == 0) &&
          (!empty.is_unique()) );

    const bool fresh_is_sole =
        ( (pointer.use_count() == 1) &&
          pointer.is_unique() );

    {
        cow_ptr<int> sharer = pointer;

        // check if sharing failed to register at both ends
        if ( (pointer.use_count() != 2) ||
             (sharer.use_count() != 2)  ||
             pointer.is_unique()        ||
             sharer.is_unique() )
        {
            return false;
        }
    }

    D_CONSTEXPR bool queries_are_noexcept =
        ( noexcept(std::declval<const cow_ptr<int>&>().use_count()) &&
          noexcept(std::declval<const cow_ptr<int>&>().is_unique()) );

    static_assert(queries_are_noexcept,
                  "the cow_ptr queries must be noexcept");

    return ( empty_is_nobodys       &&
             fresh_is_sole          &&
             (pointer.use_count() == 1) &&
             pointer.is_unique()    &&
             queries_are_noexcept );
}

/*
tests_cow_ptr_operator_bool
  the emptiness test, and it is EXPLICIT.
  Tests the following:
  - false for an empty pointer, true for a full one
  - the conversion is explicit: a cow_ptr does not silently become an int in an
    arithmetic expression, but still works in an `if`
  - it is noexcept
*/
bool
tests_cow_ptr_operator_bool()
{
    cow_ptr<int> empty;
    cow_ptr<int> full = cow_ptr<int>::make(1);

    D_CONSTEXPR bool is_explicit =
        ( std::is_constructible<bool, const cow_ptr<int>&>::value &&
          !std::is_convertible<const cow_ptr<int>&, bool>::value );

    D_CONSTEXPR bool is_noexcept =
        noexcept(static_cast<bool>(std::declval<const cow_ptr<int>&>()));

    static_assert(is_explicit,
                  "operator bool must be explicit");

    // and it does its job in a condition, without a cast
    bool taken = false;

    if (full)
    {
        taken = true;
    }

    return ( is_explicit                    &&
             is_noexcept                    &&
             taken                          &&
             (!static_cast<bool>(empty))    &&
             static_cast<bool>(full) );
}

/*
tests_the_empty_cow_ptr_must_not_be_dereferenced
  the precondition nobody wrote down.
  Tests the following:
  - read(), write(), operator* and operator-> all dereference m_block WITHOUT
    checking it.  On an empty cow_ptr every one of them is undefined behaviour:
    not a throw, not a null return, not an assert
  - write() is the cruellest of the four, because it LOOKS guarded:
    ensure_unique() does check m_block - and then write() dereferences it anyway
  - so this test does the only honest thing available to it.  It asserts that
    the queries which ARE null-safe answer correctly for an empty pointer, and
    it does not dereference one
  The guard a caller must write is `if (pointer)`.  The guard cow_ptr could have
  written is an assert, or a throw, or a null return from read().  It writes
  none of them.
*/
bool
tests_the_empty_cow_ptr_must_not_be_dereferenced()
{
    cow_ptr<tracked> empty;

    // the four accessors are declared, and they compile - which is the problem:
    // nothing about their signatures says an empty pointer is out of bounds
    D_CONSTEXPR bool the_accessors_exist =
        ( std::is_same<
              decltype(std::declval<const cow_ptr<tracked>&>().read()),
              const tracked&>::value &&
          std::is_same<
              decltype(std::declval<cow_ptr<tracked>&>().write()),
              tracked&>::value );

    // ...and none of them is noexcept-free of a null check, because there is no
    // check.  The safe queries, meanwhile, all answer:
    const bool the_safe_queries_answer =
        ( (!static_cast<bool>(empty)) &&
          (empty.use_count() == 0)    &&
          (!empty.is_unique()) );

    static_assert(the_accessors_exist,
                  "the accessors exist - and are UB on an empty pointer");

    return (the_accessors_exist && the_safe_queries_answer);
}

/*
tests_cow_ptr_carries_a_move_only_payload
  ...as far as the first write, and no further.
  Tests the following:
  - make(), read(), the copy, the move and the destructor all work for a payload
    that cannot be copied: none of them copies a T
  - the pointer shares, and the payload is reachable through every reader
  What is NOT tested here, because it cannot be: write() and detach().  Both
  instantiate ensure_unique(), whose body copy-constructs the payload - and that
  body is instantiated whether or not the pointer is shared, so a move-only T
  fails to COMPILE at the first write(), even one that would never have cloned.
  The boundary is the finding: cow_ptr stores a move-only type happily and
  refuses to copy-on-write it.
*/
bool
tests_cow_ptr_carries_a_move_only_payload()
{
    D_CONSTEXPR bool really_move_only =
        ( !std::is_copy_constructible<move_only>::value &&
          std::is_move_constructible<move_only>::value );

    static_assert(really_move_only,
                  "the move_only payload must not be copyable");

    cow_ptr<move_only> pointer = cow_ptr<move_only>::make(move_only(5));
    cow_ptr<move_only> sharer  = pointer;

    return ( really_move_only            &&
             (pointer.read().value == 5) &&
             (sharer.read().value == 5)  &&
             (pointer.use_count() == 2)  &&
             (&pointer.read() == &sharer.read()) );
}


NS_END  // testing
NS_END  // djinterp
