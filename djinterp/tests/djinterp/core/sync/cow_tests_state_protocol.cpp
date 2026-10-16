/*******************************************************************************
* djinterp [core]                                   cow_tests_state_protocol.cpp
*
*
* path:      /tests/djinterp/core/sync/cow_tests_state_protocol.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp [test]                     cow_tests_state_protocol.cpp
//   Section IV (b): the cow_state PROTOCOL - the four operations, and the one
// guarantee that ties them together.
//
//   THE GUARANTEE:  take a snapshot, then write.  The snapshot shares the
// block,
// so the write finds a refcount of 2, so it CLONES - and the snapshot is left
// holding the old value, forever, while the state moves on.  That is the whole
// of copy-on-write, and it is one test:
// tests_state_write_after_a_snapshot_clones.
// Everything else in this file either sets it up or checks that it did not cost
// more than it should have.
//
//   THE HAZARD:  read() and write() hand back a REFERENCE, and the lock guard
// that protected it is destroyed on the way out of the function.  The header
// warns about half of this ("callers must not retain references"); the other
// half it does not mention, and it is worse - a concurrent replace() frees the
// block the reference points into.  modify() is the operation that does not
// have
// this problem, because the callable runs INSIDE the lock.  See
// tests_state_read_and_write_return_unguarded_references.

// djinterp
#include "cow_tests.hpp"


NS_DJINTERP
NS_TESTING

namespace
{
    typedef cow_state<tracked, exclusive_lock_policy> tracked_state;

    // lock_prober
    //   function: try to take the state's mutex from ANOTHER thread while the
    // main thread is inside a modify().  A write lock that is actually held
    // makes this fail - which is the only way to observe, from outside, that
    // the
    // callable really did run under it.
    void
    lock_prober(
        cow_state<int, exclusive_lock_policy>* _state,
        const std::atomic<bool>*               _inside,
        std::atomic<bool>*                     _took_the_lock,
        std::atomic<bool>*                     _done
    )
    {
        while (!_inside->load())
        {
            std::this_thread::yield();
        }

        const bool taken = _state->mutex().try_lock();

        _took_the_lock->store(taken);

        if (taken)
        {
            _state->mutex().unlock();
        }

        _done->store(true);

        return;
    }
}

/*
tests_state_write_mutates_and_bumps_the_version
  write() is the mutation, and the version is its receipt.
  Tests the following:
  - the mutation is visible through read()
  - the version rises by EXACTLY one per call to write()
  - it rises whether or not the caller actually changes anything: the bump is in
    write() itself, not in whatever the caller does with the reference.  A
    caller that takes a write() reference and reads from it has still announced
    a change to every reader watching the version
*/
bool
tests_state_write_mutates_and_bumps_the_version()
{
    cow_state<int> state(1);

    state.write() = 2;

    const bool mutated =
        ( (state.read() == 2) &&
          (state.version() == 1) );

    // a write() that changes nothing still bumps
    const int seen = state.write();

    return ( mutated                  &&
             (seen == 2)              &&
             (state.version() == 2) );
}

/*
tests_state_write_without_a_snapshot_does_not_clone
  an unshared state writes in place.
  Tests the following:
  - a hundred writes on a state nobody has snapshotted copy the payload ZERO
    times
  - the payload stays at one address throughout
  So the cost of copy-on-write, absent readers, is the lock and the version -
  and
  not a copy.
*/
bool
tests_state_write_without_a_snapshot_does_not_clone()
{
    probe_reset();

    tracked_state state(tracked(0, 0));

    const long copies_before        = probe_copies().load();
    const tracked* const address    = &state.read();
    int i = 0;

    for (i = 1; i <= 100; ++i)
    {
        state.write().first = i;
    }

    return ( (probe_copies().load() == copies_before) &&
             (&state.read() == address)               &&
             (state.read().first == 100)              &&
             (state.version() == 100) );
}

/*
tests_state_snapshot_shares_the_block
  snapshot() is an atomic increment, and nothing more.
  Tests the following:
  - taking a snapshot copies NO payload
  - the snapshot names the same object the state does, at the same address
  - the block's use_count is now 2 - which is exactly what will make the next
    write clone
  - and the snapshot carries the state's current version
*/
bool
tests_state_snapshot_shares_the_block()
{
    probe_reset();

    tracked_state state(tracked(3, 4));

    state.write().first = 5;                    // version 1

    const long copies_before = probe_copies().load();

    immutable_snapshot<tracked> snapshot = state.snapshot();

    return ( (probe_copies().load() == copies_before) &&
             (snapshot.use_count() == 2)              &&
             (&snapshot.get() == &state.read())       &&
             (snapshot.get().first == 5)              &&
             (snapshot.version() == 1) );
}

/*
tests_state_write_after_a_snapshot_clones
  THE GUARANTEE.  A snapshot makes the next write pay for itself, and only the
  next one.
  Tests the following:
  - a snapshot is taken; the block's count is 2
  - the next write() clones: exactly ONE copy of the payload
  - the state's payload is now at a DIFFERENT address from the snapshot's
  - both are unique again: the count that was 2 is two counts of 1
  - the state sees the new value; the snapshot sees the old one
  - and the write AFTER that clones nothing, because the state is unique again
*/
bool
tests_state_write_after_a_snapshot_clones()
{
    probe_reset();

    tracked_state state(tracked(1, 1));

    immutable_snapshot<tracked> snapshot = state.snapshot();

    const tracked* const shared_address = &state.read();
    const long copies_before            = probe_copies().load();

    state.write().first = 2;                    // shared -> must clone

    const bool cloned_once =
        ( (probe_copies().load() == (copies_before + 1)) &&
          (&state.read() != shared_address)              &&
          (&snapshot.get() == shared_address)            &&
          (state.read().first == 2)                      &&
          (snapshot.get().first == 1)                    &&
          (snapshot.use_count() == 1) );

    // ...and the state is unique again, so the next write is free
    state.write().first = 3;

    return ( cloned_once                              &&
             (probe_copies().load() == (copies_before + 1)) &&
             (state.read().first == 3)                &&
             (snapshot.get().first == 1) );
}

/*
tests_state_snapshot_is_isolated_from_later_writes
  a snapshot is frozen: nothing the state does afterwards can reach it.
  Tests the following:
  - a snapshot is taken, and the state is then written to a hundred times
  - the snapshot's value never changes, and its address never moves
  - only the FIRST of those writes cloned; the other ninety-nine found the state
    unique and wrote in place
  This is the whole economic argument for the type: one snapshot costs one copy,
  no matter how many mutations follow it.
*/
bool
tests_state_snapshot_is_isolated_from_later_writes()
{
    probe_reset();

    tracked_state state(tracked(7, 7));

    immutable_snapshot<tracked> snapshot = state.snapshot();

    const tracked* const frozen  = &snapshot.get();
    const long copies_before     = probe_copies().load();
    int i = 0;

    for (i = 1; i <= 100; ++i)
    {
        state.write().first = i;
    }

    return ( (probe_copies().load() == (copies_before + 1)) &&
             (snapshot.get().first == 7)                    &&
             (snapshot.get().second == 7)                   &&
             (&snapshot.get() == frozen)                    &&
             (state.read().first == 100) );
}

/*
tests_state_snapshot_pins_its_version
  the version travels with the snapshot, and stops there.
  Tests the following:
  - the snapshot is stamped with the state's version at the moment it was taken
  - later writes raise the STATE's version and leave the snapshot's where it was
  - so a reader holding a snapshot can tell exactly how stale it is, by
    subtraction
*/
bool
tests_state_snapshot_pins_its_version()
{
    cow_state<int> state(0);

    state.write() = 1;
    state.write() = 2;                          // version 2

    immutable_snapshot<int> snapshot = state.snapshot();

    const bool stamped_on_arrival =
        ( (snapshot.version() == 2) &&
          (state.version() == 2) );

    state.write() = 3;
    state.write() = 4;                          // version 4

    return ( stamped_on_arrival     &&
             (snapshot.version() == 2) &&
             (state.version() == 4)    &&
             (snapshot.get() == 2)     &&
             (state.read() == 4) );
}

/*
tests_state_modify_runs_under_the_write_lock
  modify() is the operation that does its work INSIDE the lock.
  Tests the following:
  - while the callable is running, another thread cannot take the state's mutex:
    its try_lock fails
  - so the mutation, the clone and the version bump are all one atomic step from
    any other agent's point of view
  This is what read() and write() cannot offer, because they hand back a
  reference and then unlock.  modify() is the safe mutator, and this is why.
*/
bool
tests_state_modify_runs_under_the_write_lock()
{
    cow_state<int, exclusive_lock_policy> state(0);

    std::atomic<bool> inside(false);
    std::atomic<bool> took_the_lock(true);      // pessimistic: must be cleared
    std::atomic<bool> done(false);

    std::thread prober(&lock_prober, &state, &inside, &took_the_lock, &done);

    state.modify([&inside, &done](int& _value)
                 {
                     inside.store(true);

                     // hold the lock until the prober has had its go
                     while (!done.load())
                     {
                         std::this_thread::yield();
                     }

                     _value = 1;
                 });

    prober.join();

    return ( (!took_the_lock.load()) &&
             (state.read() == 1)     &&
             (state.version() == 1) );
}

/*
tests_state_modify_returns_the_callables_result
  modify() forwards whatever the callable returns.
  Tests the following:
  - a callable returning a value hands it back, through the deduced return type
  - the mutation it performed is visible afterwards
  - the callable receives a MUTABLE reference to the payload
*/
bool
tests_state_modify_returns_the_callables_result()
{
    cow_state<int> state(10);

    const int previous = state.modify([](int& _value)
                                      {
                                          const int was = _value;

                                          _value = 20;

                                          return was;
                                      });

    return ( (previous == 10)       &&
             (state.read() == 20)   &&
             (state.version() == 1) );
}

/*
tests_state_modify_accepts_a_void_callable
  a callable that returns nothing is fine.
  Tests the following:
  - modify()'s trailing return type deduces void, and the function returns
    nothing at all - without a special case for it
  - the mutation still lands and the version still moves
*/
bool
tests_state_modify_accepts_a_void_callable()
{
    cow_state<tracked> state(tracked(1, 1));

    state.modify([](tracked& _value)
                 {
                     _value.first  = 9;
                     _value.second = 9;
                 });

    return ( (state.read().first == 9)  &&
             (state.read().second == 9) &&
             (state.version() == 1) );
}

/*
tests_state_modify_bumps_the_version
  ...once per call, like write().
  Tests the following:
  - each modify() raises the version by exactly one
  - including a modify() whose callable changes nothing: the bump is
    unconditional, and announces a change that may not have happened
*/
bool
tests_state_modify_bumps_the_version()
{
    cow_state<int> state(0);

    state.modify([](int& _value) { _value += 1; });
    state.modify([](int& _value) { _value += 1; });

    const bool two_mutations =
        ( (state.read() == 2) &&
          (state.version() == 2) );

    // a modify that mutates nothing STILL bumps
    state.modify([](int& _value) { (void)_value; });

    return ( two_mutations         &&
             (state.read() == 2)   &&
             (state.version() == 3) );
}

/*
tests_state_replace_swaps_the_whole_value
  replace() drops the old block and installs a new one.
  Tests the following:
  - the state's value is the new one, at a NEW address - replace() does not
    assign through the old block, it makes a fresh one
  - the version rises by one
  - both overloads work: replace(const T&) copies, replace(T&&) moves
*/
bool
tests_state_replace_swaps_the_whole_value()
{
    probe_reset();

    tracked_state state(tracked(1, 1));

    const tracked* const original = &state.read();
    const tracked        source(2, 2);

    state.replace(source);                      // copies

    const bool copied_in =
        ( (probe_copies().load() == 1)   &&
          (&state.read() != original)    &&
          (state.read().first == 2)      &&
          (state.version() == 1) );

    const long moves_before = probe_moves().load();

    state.replace(tracked(3, 3));                // moves

    return ( copied_in                                &&
             (probe_moves().load() == (moves_before + 1)) &&
             (state.read().first == 3)                &&
             (state.version() == 2) );
}

/*
tests_state_replace_leaves_old_snapshots_valid
  replacing the value does not disturb anyone holding the old one.
  Tests the following:
  - a snapshot is taken; the state is then replaced outright
  - the snapshot still reads the OLD value, at the OLD address, with its OLD
    version - it holds its own reference, so the block cannot be freed under it
  - the state's new value is somewhere else entirely
  The header says as much ("previous snapshots remain valid, they hold their own
  cow_ptr"), and this is the test that it is true rather than intended.
*/
bool
tests_state_replace_leaves_old_snapshots_valid()
{
    probe_reset();

    tracked_state state(tracked(1, 2));

    immutable_snapshot<tracked> snapshot = state.snapshot();

    const tracked* const old_block = &snapshot.get();

    state.replace(tracked(8, 9));

    return ( (snapshot.get().first == 1)    &&
             (snapshot.get().second == 2)   &&
             (&snapshot.get() == old_block) &&
             (snapshot.version() == 0)      &&
             (snapshot.use_count() == 1)    &&
             (state.read().first == 8)      &&
             (&state.read() != old_block)   &&
             (state.version() == 1) );
}

/*
tests_state_read_and_write_return_unguarded_references
  the hazard at the centre of the class.
  Tests the following:
  - read() returns `const T&` and write() returns `T&` - REFERENCES, both of
    them
  - and the lock guard that protected the access is a local of that function: it
    is destroyed as the reference is returned.  By the time the caller has the
    reference, the lock is gone
  - modify(), by contrast, hands the reference to a callable that runs INSIDE
    the
    lock, and never lets it out
  The header warns about part of this: "the lock is held only during this call;
  callers must not retain references".  What it does not say is the sharper
  consequence, which ThreadSanitizer makes plain: because write() returns the
  reference and THEN releases the lock, the caller's mutation through it -
  `state.write() = x` - runs with NO lock held at all.  It is not merely that a
  RETAINED reference is unsafe; the single expression `state.write() = x` is
  itself a data race against any concurrent accessor, because the assignment
  happens after the guard is gone.  The only fully-synchronized mutator on
  cow_state is modify(), whose callable runs INSIDE the lock - which is why
  every
  concurrent writer in this suite uses modify() and not write().
  The suite pins the SIGNATURES and does not demonstrate the dangle, because
  demonstrating it would be undefined behaviour.  snapshot() is the operation
  that has none of these problems for reads, and modify() for writes.
*/
bool
tests_state_read_and_write_return_unguarded_references()
{
    typedef cow_state<tracked> state_type;

    D_CONSTEXPR bool read_returns_a_reference =
        std::is_same<decltype(std::declval<const state_type&>().read()),
                     const tracked&>::value;

    D_CONSTEXPR bool write_returns_a_reference =
        std::is_same<decltype(std::declval<state_type&>().write()),
                     tracked&>::value;

    // ...whereas modify hands the reference to a callable and keeps it inside
    D_CONSTEXPR bool modify_returns_the_callables_type =
        std::is_same<decltype(std::declval<state_type&>().modify(
                         std::declval<int (&)(tracked&)>())),
                     int>::value;

    static_assert(read_returns_a_reference && write_returns_a_reference,
                  "read() and write() hand out references the lock no longer "
                  "protects");

    // the safe idiom, exercised: take a snapshot, and hold THAT
    state_type                  state(tracked(1, 2));
    immutable_snapshot<tracked> held = state.snapshot();

    state.replace(tracked(3, 4));       // would have invalidated a reference

    return ( read_returns_a_reference          &&
             write_returns_a_reference         &&
             modify_returns_the_callables_type &&
             (held.get().first == 1)           &&
             (state.read().first == 3) );
}


NS_END  // testing
NS_END  // djinterp
