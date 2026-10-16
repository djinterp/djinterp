/*******************************************************************************
* djinterp [core]                                       cow_tests_exceptions.cpp
*
*
* path:      /tests/djinterp/core/sync/cow_tests_exceptions.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp [test]                        cow_tests_exceptions.cpp
//   Section: EXCEPTION SAFETY - what survives when the payload's copy
// constructor throws.
//
//   Every clone in cow.hpp is a copy construction, and a copy construction can
// throw.  There are five places it can happen - make(), ensure_unique() (behind
// write()), detach(), and cow_state's replace() and write() - and the header
// never says what it guarantees at any of them.  It turns out to guarantee
// quite
// a lot: `new control_block(...)` either produces a whole block or produces
// nothing, and every one of those five assigns the new pointer only AFTER the
// allocation has succeeded.  So four of the five are strongly exception-safe -
// the operation either happens or it did not happen - and nothing leaks.
//
//   The fifth is not, and it is the finding of this section.
// cow_state::write()
// is:
//
//       write_lock_type guard(m_mutex);
//       m_version.bump();          // <-- FIRST
//       return m_data.write();     // <-- and THIS is what throws
//
//   The version is bumped BEFORE the clone is attempted.  When the clone
// throws,
// the exception propagates and the value is untouched - but the version has
// advanced, and every reader watching it now believes a mutation happened.  A
// cache keyed on the version invalidates for nothing; a reader comparing
// against
// a snapshot's stamp concludes it is stale when it is not.  modify() has the
// same
// shape and the same problem.  Moving the bump below the mutation would fix
// both.

// djinterp
#include "cow_tests.hpp"


NS_DJINTERP
NS_TESTING

namespace
{
    // armed_scope
    //   type: arms the throwing payload's copy constructor for the length of a
    // scope, and disarms it however the scope is left - including by the very
    // exception it was armed to provoke.
    struct armed_scope
    {
        armed_scope()
        {
            probe_armed() = true;
        }

        ~armed_scope()
        {
            probe_armed() = false;
        }
    };
}

/*
tests_make_propagates_a_throwing_constructor
  a payload that cannot be built does not produce a cow_ptr.
  Tests the following:
  - make() lets the payload's exception through, unchanged and untranslated
  - no cow_ptr is produced, so there is nothing half-built to clean up
*/
bool
tests_make_propagates_a_throwing_constructor()
{
    probe_reset();

    const throwing_copy source(1);

    bool caught = false;

    try
    {
        const armed_scope armed;

        cow_ptr<throwing_copy> pointer = cow_ptr<throwing_copy>::make(source);

        (void)pointer;
    }
    catch (const throwing_copy::copy_failed&)
    {
        caught = true;
    }

    return caught;
}

/*
tests_a_failed_make_leaks_nothing
  the allocation a failed make() started is given back.
  Tests the following:
  - after a make() whose payload constructor threw, the live payload count is
    exactly what it was before: the half-built block was freed
  - `new control_block(...)` is what guarantees this: when the constructor
    throws, the memory the new-expression allocated is released before the
    exception leaves
  - and a make() that SUCCEEDS afterwards still works, so nothing was left in a
    bad state
*/
bool
tests_a_failed_make_leaks_nothing()
{
    probe_reset();

    const throwing_copy source(1);

    const long live_before = probe_live().load();   // just `source`

    try
    {
        const armed_scope armed;

        cow_ptr<throwing_copy> pointer = cow_ptr<throwing_copy>::make(source);

        (void)pointer;
    }
    catch (const throwing_copy::copy_failed&)
    {
        // expected
    }

    const bool nothing_leaked = (probe_live().load() == live_before);

    // ...and the type still works
    cow_ptr<throwing_copy> good = cow_ptr<throwing_copy>::make(source);

    return ( nothing_leaked            &&
             (good.read().value == 1)  &&
             (good.use_count() == 1) );
}

/*
tests_write_on_a_shared_pointer_is_strongly_safe
  a clone that throws leaves BOTH holders exactly as they were.
  Tests the following:
  - two pointers share a block; the writer's clone throws
  - the exception propagates
  - the writer still names the ORIGINAL block, with the original value, and is
    still sharing it: use_count is still 2 at both ends
  - the reader is untouched
  - nothing leaked
  This is the strong guarantee, and it comes from the order of operations in
  ensure_unique(): the new block is built FIRST, and m_block is only reassigned
  once that has succeeded.  Had the release come first, a failed clone would
  have
  left the writer holding nothing.
*/
bool
tests_write_on_a_shared_pointer_is_strongly_safe()
{
    probe_reset();

    cow_ptr<throwing_copy> reader = cow_ptr<throwing_copy>::make(
        throwing_copy(5));
    cow_ptr<throwing_copy> writer = reader;

    const throwing_copy* const shared_block = &reader.read();
    const long live_before                  = probe_live().load();

    bool caught = false;

    try
    {
        const armed_scope armed;

        writer.write().value = 6;
    }
    catch (const throwing_copy::copy_failed&)
    {
        caught = true;
    }

    return ( caught                          &&
             (&writer.read() == shared_block) &&
             (&reader.read() == shared_block) &&
             (writer.read().value == 5)      &&
             (reader.read().value == 5)      &&
             (writer.use_count() == 2)       &&
             (reader.use_count() == 2)       &&
             (probe_live().load() == live_before) );
}

/*
tests_detach_is_strongly_safe
  a detach that cannot copy leaves the pointer where it was.
  Tests the following:
  - the exception propagates
  - the pointer still names its ORIGINAL block, with its value intact - it was
    not released before the clone was built
  - nothing leaked
  Note how close this comes to disaster: detach() releases its old block as soon
  as the clone succeeds, and on a UNIQUE pointer that release DELETES it.  A
  version of detach() that released first would, on a failed copy, have deleted
  the only block and left the pointer dangling.
*/
bool
tests_detach_is_strongly_safe()
{
    probe_reset();

    cow_ptr<throwing_copy> pointer = cow_ptr<throwing_copy>::make(
        throwing_copy(3));

    const throwing_copy* const original = &pointer.read();
    const long live_before              = probe_live().load();

    bool caught = false;

    try
    {
        const armed_scope armed;

        pointer.detach();
    }
    catch (const throwing_copy::copy_failed&)
    {
        caught = true;
    }

    return ( caught                             &&
             static_cast<bool>(pointer)         &&
             (&pointer.read() == original)      &&
             (pointer.read().value == 3)        &&
             (pointer.use_count() == 1)         &&
             (probe_live().load() == live_before) );
}

/*
tests_state_replace_is_strongly_safe
  a replace() that cannot copy changes nothing - not even the version.
  Tests the following:
  - the exception propagates
  - the state's value is unchanged
  - AND the version is unchanged, because replace() bumps it AFTER the new block
    has been built:
        m_data = cow_ptr<_Type>::make(_new_value);   // may throw
        m_version.bump();                            // only if it did not
  - compare tests_state_write_bumps_the_version_even_when_the_clone_throws,
    where
    the two are the other way round.  Two operations in the same class, five
    lines apart, with opposite exception guarantees on the version
*/
bool
tests_state_replace_is_strongly_safe()
{
    probe_reset();

    cow_state<throwing_copy> state{ throwing_copy(1) };

    const throwing_copy  source(2);
    const std::uint64_t  version_before = state.version();

    bool caught = false;

    try
    {
        const armed_scope armed;

        state.replace(source);
    }
    catch (const throwing_copy::copy_failed&)
    {
        caught = true;
    }

    return ( caught                             &&
             (state.read().value == 1)          &&
             (state.version() == version_before) );
}

/*
tests_state_write_bumps_the_version_even_when_the_clone_throws
  THE FINDING.  A failed write still announces a change.
  Tests the following:
  - a snapshot is taken, so the state's block is shared and the next write must
    clone
  - the clone throws.  The exception propagates and the VALUE is unchanged - so
    far, so strong
  - but the version HAS advanced, by exactly one, for a mutation that never
    happened
  - the snapshot, meanwhile, still holds the old value and the old stamp - so a
    reader comparing the two now sees a version gap with no change behind it
  The cause is the order in cow_state::write():
      m_version.bump();          // first
      return m_data.write();     // and this is what threw
  A reader that trusts the version to mean "something changed" is now wrong.  A
  cache keyed on it invalidates for nothing.  Swapping those two lines - bumping
  after the clone has succeeded - would give write() the same strong guarantee
  replace() already has.
*/
bool
tests_state_write_bumps_the_version_even_when_the_clone_throws()
{
    probe_reset();

    cow_state<throwing_copy> state{ throwing_copy(1) };

    // share the block, so that write() is forced to clone
    immutable_snapshot<throwing_copy> snapshot = state.snapshot();

    const std::uint64_t version_before = state.version();

    bool caught = false;

    try
    {
        const armed_scope armed;

        state.write().value = 2;
    }
    catch (const throwing_copy::copy_failed&)
    {
        caught = true;
    }

    const bool the_value_survived  = (state.read().value == 1);
    const bool the_version_moved   = (state.version() == (version_before + 1));
    const bool the_snapshot_is_old = (snapshot.get().value == 1);

    // the state and the snapshot still share the block: the clone never
    // happened
    const bool still_sharing = (snapshot.use_count() == 2);

    return ( caught                &&
             the_value_survived    &&
             the_version_moved     &&   // <-- the bug, pinned
             the_snapshot_is_old   &&
             still_sharing );
}

/*
tests_state_modify_bumps_the_version_even_when_the_callable_throws
  ...and modify() has the same shape, and the same problem - twice over.
  Tests the following:
  - the version advances even when the CLONE throws (as in write())
  - and even when the CALLABLE itself throws, having mutated nothing: the bump
    happens before the callable is ever invoked
  - the value is unchanged in both cases
  So a modify() whose body validates its input and throws on bad data leaves the
  version raised anyway.  The mutation was rejected; the announcement was not.
*/
bool
tests_state_modify_bumps_the_version_even_when_the_callable_throws()
{
    struct rejected
    {};

    cow_state<int> state(1);

    const std::uint64_t version_before = state.version();

    bool caught = false;

    try
    {
        state.modify([](int& _value) -> int
                     {
                         // the callable validates, and refuses
                         if (_value == 1)
                         {
                             throw rejected();
                         }

                         _value = 2;

                         return _value;
                     });
    }
    catch (const rejected&)
    {
        caught = true;
    }

    return ( caught                                     &&
             (state.read() == 1)                        &&
             (state.version() == (version_before + 1)) );
}

/*
tests_state_modify_releases_the_lock_when_the_callable_throws
  the lock, at least, is not lost.
  Tests the following:
  - a callable that throws unwinds through modify(), and the write lock guard's
    destructor releases the mutex on the way out
  - the state is usable afterwards: another write succeeds, and another thread
    can take the mutex
  - so a throwing callable costs a spurious version bump, and not a deadlock
  This is what RAII buys, and it is worth a test precisely because the version
  bump right beside it is NOT protected the same way.
*/
bool
tests_state_modify_releases_the_lock_when_the_callable_throws()
{
    struct rejected
    {};

    cow_state<int, exclusive_lock_policy> state(1);

    try
    {
        state.modify([](int& _value) -> int
                     {
                         (void)_value;

                         throw rejected();
                     });
    }
    catch (const rejected&)
    {
        // expected
    }

    // the mutex must be free
    const bool lock_was_released = state.mutex().try_lock();

    if (lock_was_released)
    {
        state.mutex().unlock();
    }

    // ...and the state still works
    state.write() = 5;

    return ( lock_was_released    &&
             (state.read() == 5) );
}

/*
tests_a_snapshot_survives_a_failed_write
  the reader is never punished for the writer's failure.
  Tests the following:
  - a snapshot is held while a write on the state throws
  - the snapshot's value, address and version are all unchanged
  - and it is still readable afterwards, and the state is still usable: a
    subsequent successful write clones, as it should have the first time
  The strong guarantee on the DATA is what makes this work: the failed clone
  left
  the shared block exactly where it was, and the snapshot was holding it.
*/
bool
tests_a_snapshot_survives_a_failed_write()
{
    probe_reset();

    cow_state<throwing_copy> state{ throwing_copy(4) };

    immutable_snapshot<throwing_copy> snapshot = state.snapshot();

    const throwing_copy* const frozen = &snapshot.get();

    try
    {
        const armed_scope armed;

        state.write().value = 5;
    }
    catch (const throwing_copy::copy_failed&)
    {
        // expected
    }

    const bool snapshot_intact =
        ( (snapshot.get().value == 4) &&
          (&snapshot.get() == frozen) &&
          (snapshot.version() == 0) );

    // and the state still works: this write clones, as the failed one should
    // have
    state.write().value = 6;

    return ( snapshot_intact             &&
             (state.read().value == 6)   &&
             (snapshot.get().value == 4) &&
             (&snapshot.get() == frozen) &&
             (snapshot.use_count() == 1) );
}


NS_END  // testing
NS_END  // djinterp
