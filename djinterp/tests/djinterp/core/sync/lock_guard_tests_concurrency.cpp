/*******************************************************************************
* djinterp [core]                               lock_guard_tests_concurrency.cpp
*
*
* path:      /tests/djinterp/core/sync/lock_guard_tests_concurrency.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp [test]                 lock_guard_tests_concurrency.cpp
//   Section: CONCURRENCY - the claims that only many threads can test.
//
//   The single-threaded sections established that each guard takes and releases
// the right lock.  This one establishes what those locks are FOR: that a write
// guard actually serializes writers (a shared counter lands on its exact
// expected total, with no lost updates), that a try guard and a timed guard
// really exclude a contending thread (the try fails, the timed one times out
// after roughly its deadline), that a read guard under a shared policy lets two
// readers genuinely overlap, and that an upgrade_lock holds its write lock
// exclusively against another thread.
//
//   The tests are deterministic, not probabilistic:
//     - threads meet at a rendezvous (an atomic "go" flag every worker spins
// on)
//       so they truly run at once rather than lining up by luck;
//     - a thread that must be seen HOLDING a lock before the main thread probes
//       it announces that with its own flag, and the main thread waits;
//     - every thread is joined;
//     - spin-waits SLEEP between polls, so on a single-core box a spinning
//       thread cannot starve the one it is waiting for.
//   A failure here is a wrong count or a wrong ownership answer - never a hang.
//
//   The shared-policy and upgrade tests are gated to C++17; the mutual-
// exclusion,
// try and timed tests run from the C++11 floor.

// std
#include <mutex>
#include <vector>
// djinterp
#include "lock_guard_tests.hpp"
#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    // std
    #include <shared_mutex>
#endif


NS_DJINTERP
NS_TESTING

/*
tests_write_lock_serializes_writers
  a write guard gives mutual exclusion: no lost updates.
  Tests the following:
  - k_agents threads each increment a shared plain long k_iterations times, each
    increment under a scoped_write_lock<exclusive_lock_policy>
  - the final total is EXACTLY k_agents * k_iterations
  A single missed exclusion would let two increments race on the non-atomic
  counter and lose one, so an exact total across tens of thousands of contended
  increments is a strong statement that the guard serializes.  The threads
  rendezvous on a go flag so the contention is real.
*/
bool
tests_write_lock_serializes_writers()
{
    std::mutex        mutex;
    long              counter = 0;
    std::atomic<bool> go(false);
    std::atomic<int>  ready(0);

    std::vector<std::thread> writers;
    int                      i = 0;

    for (i = 0; i < k_agents; ++i)
    {
        writers.push_back(std::thread(
            [&mutex, &counter, &go, &ready]()
            {
                ready.fetch_add(1);

                while (!go.load())
                {
                    std::this_thread::yield();
                }

                int n = 0;

                for (n = 0; n < k_iterations; ++n)
                {
                    scoped_write_lock<exclusive_lock_policy> guard(mutex);
                    ++counter;
                }
            }));
    }

    const bool all_ready = spin_until_count(ready, k_agents, k_deadline_ms);

    go.store(true);

    for (i = 0; i < static_cast<int>(writers.size()); ++i)
    {
        writers[static_cast<std::size_t>(i)].join();
    }

    const long expected =
        static_cast<long>(k_agents) * static_cast<long>(k_iterations);

    return (all_ready && (counter == expected));
}

/*
tests_read_lock_admits_concurrent_readers
  under a shared policy, two read guards overlap.
  Tests the following:
  - two threads each take a scoped_read_lock<shared_lock_policy> over one
    shared_mutex and announce that they are inside
  - both are observed inside AT THE SAME TIME (the count of readers currently
    holding reaches 2)
  - so the read guard really admits reader concurrency, not just one reader at a
    time
  Each reader holds its lock until both have arrived, so the overlap is genuine
  rather than sequential.  Below C++17 there is no shared policy and this is a
  stated no-op.
*/
bool
tests_read_lock_admits_concurrent_readers()
{
#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    std::shared_mutex mutex;

    std::atomic<int>  inside(0);
    std::atomic<bool> go(false);
    std::atomic<bool> release(false);
    std::atomic<int>  ready(0);

    std::vector<std::thread> readers;
    int                      i = 0;

    for (i = 0; i < 2; ++i)
    {
        readers.push_back(std::thread(
            [&mutex, &inside, &go, &release, &ready]()
            {
                ready.fetch_add(1);

                while (!go.load())
                {
                    std::this_thread::yield();
                }

                scoped_read_lock<shared_lock_policy> guard(mutex);

                inside.fetch_add(1);

                // hold the read lock until told to let go, so the two readers
                // are inside at the same time
                while (!release.load())
                {
                    std::this_thread::yield();
                }
            }));
    }

    const bool all_ready = spin_until_count(ready, 2, k_deadline_ms);

    go.store(true);

    // both readers must be inside simultaneously
    const bool both_inside = spin_until_count(inside, 2, k_deadline_ms);

    release.store(true);

    for (i = 0; i < static_cast<int>(readers.size()); ++i)
    {
        readers[static_cast<std::size_t>(i)].join();
    }

    return (all_ready && both_inside);
#else
    return true;
#endif
}

/*
tests_try_lock_excludes_a_contending_thread
  a try guard fails against a lock another thread holds.
  Tests the following:
  - a holder thread takes a scoped_try_lock<exclusive_lock_policy> and keeps it,
    announcing that it holds it
  - the main thread, once the holder is confirmed in, constructs its own
    scoped_try_lock over the same mutex and gets owns_lock() == false
  - so the try guard reports a genuine cross-thread conflict, not a false
    success
  The holder keeps the lock until the main thread has finished its attempt, so
  the conflict is real.
*/
bool
tests_try_lock_excludes_a_contending_thread()
{
    std::mutex        mutex;
    std::atomic<bool> holder_has_it(false);
    std::atomic<bool> release(false);
    std::atomic<bool> holder_owned(false);

    std::thread holder(
        [&mutex, &holder_has_it, &release, &holder_owned]()
        {
            scoped_try_lock<exclusive_lock_policy> guard(mutex);

            holder_owned.store(guard.owns_lock());
            holder_has_it.store(true);

            while (!release.load())
            {
                std::this_thread::yield();
            }
        });

    const bool holder_in = spin_until(holder_has_it, k_deadline_ms);

    // the holder has the lock; our try must fail
    bool contender_excluded = false;
    {
        scoped_try_lock<exclusive_lock_policy> guard(mutex);
        contender_excluded = (!guard.owns_lock());
    }

    release.store(true);
    holder.join();

    return ( holder_in            &&
             holder_owned.load()  &&
             contender_excluded );
}

/*
tests_timed_lock_times_out_under_contention
  a timed guard gives up after its deadline when the lock is held elsewhere.
  Tests the following:
  - a holder thread takes a scoped_timed_lock<timed_lock_policy> and keeps it
  - the main thread then tries a scoped_timed_lock with a SHORT timeout and gets
    owns_lock() == false - it timed out rather than blocking forever
  - the attempt took at least roughly the timeout (a loose lower bound, to avoid
    flaking on a busy box), confirming it actually waited
  So the timed guard's deadline is real: it neither succeeds against a held lock
  nor hangs.  The holder keeps the lock across the whole attempt.
*/
bool
tests_timed_lock_times_out_under_contention()
{
    std::timed_mutex  mutex;
    std::atomic<bool> holder_has_it(false);
    std::atomic<bool> release(false);

    // The holder is scaffolding: it exists only to HOLD the lock so that the
    // main thread's timed attempt has something to time out against.  It takes
    // the lock with a PLAIN std::lock_guard rather than a scoped_timed_lock on
    // purpose - the guard under test is the one on the main thread.  Using a
    // timed acquire here would add nothing (its success path is covered by
    // tests_timed_acquires_a_free_mutex) and would trip a known ThreadSanitizer
    // false positive: TSan's pthread_mutex_timedlock interceptor does not
    // record
    // a SUCCESSFUL try_lock_for, so the holder's later, entirely valid unlock
    // is
    // misreported as "unlock of an unlocked mutex".  A plain lock is modelled
    // correctly, so the test stays TSan-clean while testing exactly the same
    // thing.
    std::thread holder(
        [&mutex, &holder_has_it, &release]()
        {
            std::lock_guard<std::timed_mutex> guard(mutex);

            holder_has_it.store(true);

            while (!release.load())
            {
                std::this_thread::yield();
            }
        });

    const bool holder_in = spin_until(holder_has_it, k_deadline_ms);

    // try to take it with a short deadline; must time out
    const test_clock::time_point start = test_clock::now();

    bool timed_out = false;
    {
        scoped_timed_lock<timed_lock_policy> guard(
            mutex, std::chrono::milliseconds(40));

        timed_out = (!guard.owns_lock());
    }

    const long long waited = elapsed_ms(start);

    release.store(true);
    holder.join();

    // it must have failed, and not returned instantly (a loose 20ms floor keeps
    // the bound honest without flaking under load)
    return ( holder_in         &&
             timed_out         &&
             (waited >= 20) );
}

/*
tests_upgrade_holds_the_write_lock_exclusively
  an upgrade_lock excludes another thread's readers and writers.
  Tests the following:
  - the main thread takes a read lock and upgrades it to an exclusive write lock
  - while the upgrade is held, a SEPARATE thread's try_lock_shared (a reader)
    and
    try_lock (a writer) on the same mutex both FAIL
  - so the promoted lock is exclusive across threads, not merely against the
    same
    thread
  This is the cross-thread proof of the exclusivity that
  tests_upgrade_acquires_the_write_lock checks locally.  Below C++17 there is no
  upgrade_lock and this is a stated no-op.
*/
bool
tests_upgrade_holds_the_write_lock_exclusively()
{
#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    std::shared_mutex mutex;

    std::shared_lock<std::shared_mutex> read(mutex);

    upgrade_lock<shared_lock_policy> upgraded(std::move(read), mutex);

    // another thread must find the mutex fully exclusive
    std::atomic<int> reader_result(-1);
    std::atomic<int> writer_result(-1);

    std::thread probe(
        [&mutex, &reader_result, &writer_result]()
        {
            const bool got_shared = mutex.try_lock_shared();
            reader_result.store(got_shared ? 1 : 0);

            if (got_shared)
            {
                mutex.unlock_shared();
            }

            const bool got_exclusive = mutex.try_lock();
            writer_result.store(got_exclusive ? 1 : 0);

            if (got_exclusive)
            {
                mutex.unlock();
            }
        });

    probe.join();

    const bool owns             = upgraded.owns_lock();
    const bool reader_excluded  = (reader_result.load() == 0);
    const bool writer_excluded  = (writer_result.load() == 0);

    return (owns && reader_excluded && writer_excluded);
#else
    return true;
#endif
}


NS_END  // testing
NS_END  // djinterp
