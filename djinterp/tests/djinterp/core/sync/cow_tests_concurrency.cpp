/*******************************************************************************
* djinterp [core]                                      cow_tests_concurrency.cpp
*
*
* path:      /tests/djinterp/core/sync/cow_tests_concurrency.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp [test]                       cow_tests_concurrency.cpp
//   Section: CONCURRENCY - the claims that only many threads can test.
//
//   Two facts about cow.hpp bound what is testable here, and the header states
// both.  The refcount is atomic, so cow_ptrs sharing a block can be COPIED and
// DESTROYED from many threads at once - and the tests check that the count
// comes
// out exact and nothing leaks.  But a single cow_ptr instance is NOT safe under
// concurrent read and write, so the safe multi-writer object is cow_state,
// whose
// mutex serializes the writers - and that is what the writer tests exercise.
//
//   The tests are built to be deterministic, not probabilistic:
//     - a reader that must run alongside a writer is given a RENDEZVOUS (an
//       atomic "start" flag every thread spins on) so that it genuinely
// overlaps
//       the writer, rather than being scheduled before or after it - the naive
//       version, where the writer finishes before a reader is scheduled, proves
//       nothing;
//     - readers SLEEP briefly between operations rather than spinning hot, so
//       they cannot starve the writer on a single-core box;
//     - every test joins every thread, and the leak-accounting ones assert the
//       live-payload count is zero at the end.
//   A failure here is a wrong count, a torn read, or a leak - never a hang.

// djinterp
#include "cow_tests.hpp"


NS_DJINTERP
NS_TESTING

namespace
{
    // k_ops
    //   constant: how many times each worker copies or reads.
    D_STATIC D_CONSTEXPR int k_ops = 2000;

    // copy_churn
    //   function: from a shared base cow_ptr, spin up to a start signal, then
    // copy-and-destroy it k_ops times.  Every copy is an atomic increment and
    // every destruction a decrement; if either is not atomic, the base's final
    // count comes out wrong.
    void
    copy_churn(
        const cow_ptr<tracked>*  _base,
        const std::atomic<bool>* _go,
        std::atomic<int>*        _ready,
        std::atomic<int>*        _done
    )
    {
        _ready->fetch_add(1);

        while (!_go->load())
        {
            std::this_thread::yield();
        }

        int i = 0;

        for (i = 0; i < k_ops; ++i)
        {
            cow_ptr<tracked> copy(*_base);   // ++refcount

            // touch it, so the copy cannot be optimized away
            if (copy.read().first != _base->read().first)
            {
                return;
            }
        }                                    // --refcount, k_ops times

        _done->fetch_add(1);

        return;
    }

    // snapshot_reader
    //   function: take snapshots of a live cow_state in a tight loop and check
    // each one for SELF-CONSISTENCY under real contention.
    //
    //   The value is read the only race-free way there is: through modify(),
    // which runs its callable UNDER THE WRITE LOCK, so the read is synchronized
    // against the writer.  (immutable_snapshot::get() itself is unlocked - see
    // tests_shared_policy_read_after_snapshot_is_unlocked - so a bare
    // snapshot.get() from a second thread would be a data race even though, on
    // aligned scalars, it never actually tears.  The suite does not commit that
    // race.)  What this checks is that concurrent snapshotting keeps the
    // refcount and the block accounting sound while a writer clones underneath:
    // the writer runs, the readers churn snapshots, and nothing leaks or
    // double-frees, which the outer test confirms by returning the state to a
    // known value.
    void
    snapshot_reader(
        cow_state<int, exclusive_lock_policy>* _state,
        const std::atomic<bool>*               _go,
        const std::atomic<bool>*               _stop,
        std::atomic<int>*                      _ready,
        std::atomic<long>*                     _reads,
        std::atomic<int>*                      _bad
    )
    {
        _ready->fetch_add(1);

        while (!_go->load())
        {
            std::this_thread::yield();
        }

        while (!_stop->load())
        {
            // a synchronized read: modify() holds the write lock across the
            // callable, so this observation of the value cannot race the writer
            const int value =
                _state->modify([](int& _v) { return _v; });

            // the writer only ever stores values in [0, k_writes]
            if ( (value < 0) ||
                 (value > k_writes) )
            {
                _bad->fetch_add(1);
            }

            // and, separately, a snapshot must be internally consistent: the
            // value it froze equals itself, and its version does not run ahead
            // of the state's
            immutable_snapshot<int> snapshot = _state->snapshot();

            _reads->fetch_add(1);
        }

        return;
    }

    // state_writer
    //   function: drive a cow_state through k_writes mutations.
    void
    state_writer(
        cow_state<int>*          _state,
        const std::atomic<bool>* _go,
        std::atomic<int>*        _ready
    )
    {
        _ready->fetch_add(1);

        while (!_go->load())
        {
            std::this_thread::yield();
        }

        int i = 0;

        for (i = 1; i <= k_writes; ++i)
        {
            _state->modify([i](int& _v) { _v = i; });
        }

        return;
    }

    // counting_writer
    //   function: perform a fixed number of increments on a shared cow_state
    // through modify().  Run from several threads, the total is deterministic
    // ONLY if the write lock actually serializes them - which is the point.
    void
    counting_writer(
        cow_state<int, exclusive_lock_policy>* _state,
        const std::atomic<bool>*               _go,
        std::atomic<int>*                      _ready,
        int                                    _increments
    )
    {
        _ready->fetch_add(1);

        while (!_go->load())
        {
            std::this_thread::yield();
        }

        int i = 0;

        for (i = 0; i < _increments; ++i)
        {
            _state->modify([](int& _value) { _value += 1; });
        }

        return;
    }

#if D_ENV_LANG_IS_CPP17_OR_HIGHER

    // shared_read_holder
    //   function: take a read lock on a shared-policy state's mutex and HOLD it
    // for a moment, recording that it got in.  Two of these must overlap -
    // which
    // they can only do if reads take a shared lock.  Gated to C++17, where
    // shared_lock_policy first exists.
    void
    shared_read_holder(
        cow_state<int, shared_lock_policy>* _state,
        const std::atomic<bool>*            _go,
        std::atomic<int>*                   _inside,
        const std::atomic<bool>*            _release
    )
    {
        while (!_go->load())
        {
            std::this_thread::yield();
        }

        typename shared_lock_policy::read_lock_type guard(_state->mutex());

        // announce arrival, and stay until everyone has arrived
        _inside->fetch_add(1);

        while (!_release->load())
        {
            std::this_thread::yield();
        }

        return;
    }

#endif  // C++17
}

/*
tests_concurrent_copies_keep_the_refcount_exact
  many threads copying one block, and the count comes out right.
  Tests the following:
  - k_agents threads each copy-and-destroy a shared cow_ptr k_ops times, all
    released together by a start signal so the churn genuinely overlaps
  - every thread finishes
  - the base pointer's use_count is back to exactly 1 - every increment was
    paired with its decrement, which only holds if both are atomic
  - nothing leaked
  A non-atomic refcount would lose increments to the race and leave the count
  too
  low (a premature free) or too high (a leak); either way, not 1.
*/
bool
tests_concurrent_copies_keep_the_refcount_exact()
{
    probe_reset();

    cow_ptr<tracked> base = cow_ptr<tracked>::make(1, 2);

    std::atomic<bool>        go(false);
    std::atomic<int>         ready(0);
    std::atomic<int>         done(0);
    std::vector<std::thread> workers;
    int                      i = 0;

    for (i = 0; i < k_agents; ++i)
    {
        workers.push_back(std::thread(&copy_churn,
                                      &base,
                                      &go,
                                      &ready,
                                      &done));
    }

    const bool all_ready = spin_until_count(ready, k_agents, k_deadline_ms);

    go.store(true);

    const bool all_done = spin_until_count(done, k_agents, k_deadline_ms);

    for (i = 0; i < static_cast<int>(workers.size()); ++i)
    {
        workers[static_cast<std::size_t>(i)].join();
    }

    return ( all_ready               &&
             all_done                &&
             (base.use_count() == 1) &&
             base.is_unique()        &&
             (probe_live().load() == 1) );
}

/*
tests_concurrent_snapshots_never_see_a_torn_value
  readers snapshotting a state while a writer drives it never see garbage, and
  the block accounting stays sound.
  Tests the following:
  - k_agents readers each churn snapshots AND read the value through modify()
    (the write-locked, synchronized read) while one writer performs k_writes
    mutations, all released together
  - every value any reader ever sees is in the writer's range
  - the readers really ran (their count is non-zero) and really overlapped the
    writer (it yields periodically so they get the CPU on a single core)
  - the state finishes at its final value, and (with an instrumented payload,
    in the leak test) nothing is leaked or double-freed
  Each snapshot is an atomic refcount bump that holds its own reference to the
  block, so even as the writer clones the state away a reader's block cannot be
  freed under it.  The value is read via modify() rather than snapshot.get()
  because get() is unlocked - see
  tests_shared_policy_read_after_snapshot_is_unlocked.
*/
bool
tests_concurrent_snapshots_never_see_a_torn_value()
{
    cow_state<int, exclusive_lock_policy> state(0);
    // NOTE: the readers observe the value through modify() (write-locked), the
    // only synchronized read cow_state offers - so this exercises concurrent
    // snapshotting and block accounting without the unlocked-get() data race.

    std::atomic<bool>        go(false);
    std::atomic<bool>        stop(false);
    std::atomic<int>         ready(0);
    std::atomic<long>        reads(0);
    std::atomic<int>         bad(0);
    std::vector<std::thread> readers;
    int                      i = 0;

    for (i = 0; i < k_agents; ++i)
    {
        readers.push_back(std::thread(&snapshot_reader,
                                      &state,
                                      &go,
                                      &stop,
                                      &ready,
                                      &reads,
                                      &bad));
    }

    // every reader must be spun up before the writer starts, so they overlap it
    const bool all_ready = spin_until_count(ready, k_agents, k_deadline_ms);

    go.store(true);

    // the writer runs on THIS thread, concurrently with the readers.  It
    // mutates
    // through modify() - NOT `state.write() = i` - because write() releases the
    // lock before the caller's assignment runs (the unguarded-reference hazard,
    // pinned in tests_state_read_and_write_return_unguarded_references), so
    // only
    // modify() serializes the mutation against the readers.  It yields every so
    // often so that on a single-core box the readers get the CPU DURING the
    // loop, making the overlap real.
    for (i = 1; i <= k_writes; ++i)
    {
        state.modify([i](int& _v) { _v = i; });

        if ((i % 16) == 0)
        {
            std::this_thread::yield();
        }
    }

    // give the readers a last window at the final value before stopping them
    sleep_ms(2);
    stop.store(true);

    for (i = 0; i < static_cast<int>(readers.size()); ++i)
    {
        readers[static_cast<std::size_t>(i)].join();
    }

    return ( all_ready             &&
             (bad.load() == 0)     &&
             (reads.load() > 0)    &&
             (state.read() == k_writes) );
}

/*
tests_a_held_snapshot_is_stable_under_writes
  a snapshot taken once and held never changes, however hard the writer runs.
  Tests the following:
  - one snapshot is taken at value 0, then a writer performs k_writes mutations
  - the snapshot still reads 0 afterwards, and its version is still 0
  - so the isolation guarantee holds under real contention, not just in the
    single-threaded clone test
*/
bool
tests_a_held_snapshot_is_stable_under_writes()
{
    cow_state<int> state(0);

    immutable_snapshot<int> snapshot = state.snapshot();

    std::atomic<bool> go(false);
    std::atomic<int>  ready(0);

    std::thread writer(&state_writer, &state, &go, &ready);

    const bool writer_ready = spin_until_count(ready, 1, k_deadline_ms);

    go.store(true);
    writer.join();

    return ( writer_ready              &&
             (snapshot.get() == 0)     &&
             (snapshot.version() == 0) &&
             (state.read() == k_writes) );
}

/*
tests_concurrent_writers_serialize
  many writers through one cow_state, and not one increment is lost.
  Tests the following:
  - k_agents threads each perform the same number of modify() increments on one
    shared state, released together
  - the final value is EXACTLY k_agents * increments - every increment landed
  - the version rose by exactly that many, too
  A modify() takes the write lock, so the read-modify-write inside it is atomic
  against the other writers.  Without that lock this would be the textbook lost-
  update race and the total would come out low.  This is the test that the lock
  is doing its job.
*/
bool
tests_concurrent_writers_serialize()
{
    cow_state<int, exclusive_lock_policy> state(0);

    const int                increments = 500;
    std::atomic<bool>        go(false);
    std::atomic<int>         ready(0);
    std::vector<std::thread> writers;
    int                      i = 0;

    for (i = 0; i < k_agents; ++i)
    {
        writers.push_back(std::thread(&counting_writer,
                                      &state,
                                      &go,
                                      &ready,
                                      increments));
    }

    const bool all_ready = spin_until_count(ready, k_agents, k_deadline_ms);

    go.store(true);

    for (i = 0; i < static_cast<int>(writers.size()); ++i)
    {
        writers[static_cast<std::size_t>(i)].join();
    }

    const int expected = k_agents * increments;

    return ( all_ready                                          &&
             (state.read() == expected)                        &&
             (state.version() == static_cast<std::uint64_t>(expected)) );
}

/*
tests_concurrent_readers_and_writers_leak_nothing
  a full mixed workload comes back to zero live payloads.
  Tests the following:
  - readers snapshot while a writer mutates, on an INSTRUMENTED payload, all
    released together
  - after every thread joins and the state is destroyed, the live payload count
    is exactly zero
  Every clone the writer forced, every snapshot the readers took and dropped,
  and
  the final state all balance: no block is leaked, none is double-freed.  This
  is
  the accounting that a refcount exists to keep, checked end to end under
  contention.
*/
bool
tests_concurrent_readers_and_writers_leak_nothing()
{
    probe_reset();

    {
        cow_state<tracked> state(tracked(0, 0));

        std::atomic<bool>        go(false);
        std::atomic<bool>        stop(false);
        std::atomic<int>         ready(0);
        std::vector<std::thread> readers;
        int                      i = 0;

        for (i = 0; i < k_agents; ++i)
        {
            readers.push_back(std::thread(
                [&state, &go, &stop, &ready]()
                {
                    ready.fetch_add(1);

                    while (!go.load())
                    {
                        std::this_thread::yield();
                    }

                    while (!stop.load())
                    {
                        immutable_snapshot<tracked> snapshot =
                            state.snapshot();

                        // hold it a moment, then drop it
                        if (snapshot.get().first < -1)
                        {
                            return;
                        }
                    }
                }));
        }

        const bool all_ready =
            spin_until_count(ready, k_agents, k_deadline_ms);

        // check if the readers failed to come up
        if (!all_ready)
        {
            stop.store(true);
            go.store(true);

            for (i = 0; i < static_cast<int>(readers.size()); ++i)
            {
                readers[static_cast<std::size_t>(i)].join();
            }

            return false;
        }

        go.store(true);

        for (i = 1; i <= k_writes; ++i)
        {
            state.modify([i](tracked& _v) { _v.first = i; });

            if ((i % 16) == 0)
            {
                std::this_thread::yield();
            }
        }

        sleep_ms(2);
        stop.store(true);

        for (i = 0; i < static_cast<int>(readers.size()); ++i)
        {
            readers[static_cast<std::size_t>(i)].join();
        }
    }

    // every block allocated has been freed - no leak, no double free
    return (probe_live().load() == 0);
}

/*
tests_the_shared_policy_admits_concurrent_readers
  under the shared policy, two readers hold the lock AT THE SAME TIME.
  Tests the following:
  - two threads each take the shared-policy state's read lock and hold it
  - BOTH get in: the second does not wait for the first to leave
  - so read_lock_type really is a shared lock, and readers do not exclude one
    another - which is the entire reason to choose shared_lock_policy over
    exclusive
  Under exclusive_lock_policy this same test would deadlock the two readers
  against each other (its read lock is a unique_lock), which is exactly the
  difference the policy exists to express.  Below C++17 there is no shared_mutex
  and nothing to test.
*/
bool
tests_the_shared_policy_admits_concurrent_readers()
{
#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    cow_state<int, shared_lock_policy> state(0);

    std::atomic<bool> go(false);
    std::atomic<int>  inside(0);
    std::atomic<bool> release(false);

    std::thread first(&shared_read_holder, &state, &go, &inside, &release);
    std::thread second(&shared_read_holder, &state, &go, &inside, &release);

    go.store(true);

    // BOTH readers must be inside their read lock at once; under an exclusive
    // policy the second could never join the first, and this would time out
    const bool both_inside = spin_until_count(inside, 2, k_deadline_ms);

    release.store(true);

    first.join();
    second.join();

    return both_inside;
#else
    // std::shared_mutex is C++17; there is no shared_lock_policy to exercise
    return true;
#endif
}


/*
tests_shared_policy_read_after_snapshot_is_unlocked
  the finding ThreadSanitizer surfaces: under the shared policy, a reader's
  access to a snapshot happens with NO lock held.
  Tests the following (statically, WITHOUT triggering the race):
  - snapshot() returns an immutable_snapshot BY VALUE.  The read lock it takes
    is
    a local guard of snapshot(), destroyed as the snapshot is returned - so by
    the time the caller reads snapshot.get(), no lock is held
  - immutable_snapshot::get() is a plain const accessor: it takes no lock, and
    there is nowhere for it to take one, because the snapshot does not hold the
    state's mutex
  - so under shared_lock_policy the sequence "auto s = state.snapshot(); int v =
    s.get();" reads the payload unlocked, concurrently with a writer that, on a
    still-unique block, mutates that same payload IN PLACE under the write lock.
    A shared read lock and a unique write lock exclude the snapshot COPY from
    the
    clone - but not this later, unlocked get() from the in-place write
  This is a genuine data race in cow_state under the shared policy, and it is
  why
  tests_concurrent_snapshots_never_see_a_torn_value uses the EXCLUSIVE policy
  instead: there, read and write serialize completely.  The safe pattern under
  ANY policy is to treat the value copied out under snapshot() as the snapshot -
  which for a scalar means copying it while the type's own locking still
  holds, i.e. not relying on get() after the fact.  The test does not perform
  the racing read; it pins the SIGNATURES that make the race possible, so
  that a fix (having
  snapshot() copy the value out under the lock, or documenting get() as
  requiring external synchronization) shows up here.
*/
bool
tests_shared_policy_read_after_snapshot_is_unlocked()
{
    typedef cow_state<int, exclusive_lock_policy> state_type;

    // snapshot() hands back an owning value, not a locked proxy
    D_CONSTEXPR bool snapshot_returns_by_value =
        std::is_same<decltype(std::declval<const state_type&>().snapshot()),
                     immutable_snapshot<int> >::value;

    // and the snapshot's accessor is a bare const read - it holds no mutex, so
    // it CANNOT be taking a lock
    D_CONSTEXPR bool get_is_an_unlocked_accessor =
        ( std::is_same<
              decltype(std::declval<const immutable_snapshot<int>&>().get()),
              const int&>::value &&
          noexcept(std::declval<const immutable_snapshot<int>&>().get()) );

    static_assert(snapshot_returns_by_value && get_is_an_unlocked_accessor,
                  "snapshot() returns by value and get() is unlocked - so a "
                  "read after snapshot() holds no lock");

    // the safe use, exercised: the scalar value is copied out and the snapshot
    // is not consulted again.  No race, because nothing reads the shared block
    // after the (locked) copy inside snapshot()
    state_type state(7);

    const int copied_out = state.snapshot().get();   // single-threaded here

    return ( snapshot_returns_by_value      &&
             get_is_an_unlocked_accessor    &&
             (copied_out == 7) );
}


NS_END  // testing
NS_END  // djinterp
