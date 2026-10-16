/*******************************************************************************
* djinterp [core]                            lock_policy_c_tests_concurrency.cpp
*
*
* path:      /tests/djinterp/core/sync/lock_policy_c_tests_concurrency.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp [test]              lock_policy_c_tests_concurrency.cpp
//   Section: CONCURRENCY - the claims that only many threads can test.
//
// The earlier sections showed each primitive locks and unlocks on one thread.
// This one shows what the C policies are FOR: that c_exclusive serializes
// writers to an exact counter total (no lost updates), that c_shared lets two
// readers genuinely overlap yet blocks a reader while a writer holds the lock,
// that c_recursive stays mutually exclusive ACROSS threads even while a single
// thread re-enters it, and that the spinlock serializes writers too.
// Everything runs through the policy guards - c_lock_guard and c_shared_guard
// over the policies' mutex_types - so it exercises the real RAII types, not
// just the raw mutexes.
//
//   The tests are deterministic, not probabilistic:
//     - threads meet at a rendezvous (an atomic "go" flag) so they truly run at
//       once;
//     - a thread that must be seen HOLDING a lock announces it with a flag, and
//       the main thread waits;
//     - a reader that must be seen BLOCKED announces that it has started, then
//       attempts the lock; the main thread, still holding the writer, confirms
//       the reader has not acquired - which the rwlock guarantees, not luck;
//     - every thread is joined; spin-waits sleep, so on one core a spinning
//       thread cannot starve the thread it waits on.
//   A failure here is a wrong count or a wrong ownership answer, never a hang.
//
//   The spinlock test is gated to C++11; the rest run from the C++11 floor (the
// suite needs std::thread throughout anyway).

// std
#include <vector>
// djinterp
#include "lock_policy_c_tests.hpp"


NS_DJINTERP
NS_TESTING

/*
tests_c_exclusive_serializes_writers
  a c_exclusive lock gives mutual exclusion: no lost updates.
  Tests the following:
  - k_agents threads each increment a shared plain long k_iterations times, each
    increment guarded by a c_lock_guard<c_exclusive::mutex_type>
  - the final total is EXACTLY k_agents * k_iterations
  A single missed exclusion would race two increments on the non-atomic counter
  and lose one, so an exact total across tens of thousands of contended
  increments is strong evidence the guard serializes.  Threads rendezvous so the
  contention is real.
*/
bool
tests_c_exclusive_serializes_writers()
{
    c_exclusive::mutex_type  mutex;
    long                     counter = 0;
    std::atomic<bool>        go(false);
    std::atomic<int>         ready(0);

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
                    c_lock_guard<c_exclusive::mutex_type> guard(mutex);
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
tests_c_shared_admits_concurrent_readers
  under c_shared, two reader guards overlap.
  Tests the following:
  - two threads each hold a c_shared_guard<c_shared::mutex_type> over one rwlock
    and announce that they are inside
  - both are observed inside AT THE SAME TIME (the count of readers holding
    reaches 2)
  - so the shared guard admits reader concurrency, not one reader at a time
  Each reader holds until both have arrived, so the overlap is genuine rather
  than sequential.
*/
bool
tests_c_shared_admits_concurrent_readers()
{
    c_shared::mutex_type rwlock;

    std::atomic<int>  inside(0);
    std::atomic<bool> go(false);
    std::atomic<bool> release(false);
    std::atomic<int>  ready(0);

    std::vector<std::thread> readers;
    int                      i = 0;

    for (i = 0; i < 2; ++i)
    {
        readers.push_back(std::thread(
            [&rwlock, &inside, &go, &release, &ready]()
            {
                ready.fetch_add(1);

                while (!go.load())
                {
                    std::this_thread::yield();
                }

                c_shared_guard<c_shared::mutex_type> guard(rwlock);

                inside.fetch_add(1);

                // hold the read lock until both readers are in
                while (!release.load())
                {
                    std::this_thread::yield();
                }
            }));
    }

    const bool all_ready = spin_until_count(ready, 2, k_deadline_ms);

    go.store(true);

    const bool both_inside = spin_until_count(inside, 2, k_deadline_ms);

    release.store(true);

    for (i = 0; i < static_cast<int>(readers.size()); ++i)
    {
        readers[static_cast<std::size_t>(i)].join();
    }

    return (all_ready && both_inside);
}

/*
tests_c_shared_writer_excludes_readers
  while a writer holds c_shared exclusively, a reader cannot acquire.
  Tests the following:
  - the main thread holds a c_lock_guard<c_shared::mutex_type> (the WRITER path
    - an exclusive lock on the rwlock)
  - a reader thread announces it has started, then tries to take a
    c_shared_guard; while the writer is held, the reader does NOT get in (its
    "acquired" flag stays false)
  - once the writer releases, the reader acquires and finishes
  The reader staying out while the writer holds is guaranteed by the rwlock (a
  write lock excludes readers), not by timing; the brief wait only gives the
  reader time to reach the blocking call.
*/
bool
tests_c_shared_writer_excludes_readers()
{
    c_shared::mutex_type rwlock;

    std::atomic<bool> reader_started(false);
    std::atomic<bool> reader_acquired(false);

    // The reader thread lives in the outer scope so it can be joined AFTER the
    // writer has been released - joining it while the writer still held the
    // lock would deadlock (main waiting on a reader that waits on main).
    std::thread reader;

    bool started            = false;
    bool reader_was_blocked = false;

    {
        // Acquire the writer (exclusive) lock BEFORE starting the reader, so
        // the reader is guaranteed to find the lock held and block - the
        // ordering is forced, not raced.
        c_lock_guard<c_shared::mutex_type> writer(rwlock);

        reader = std::thread(
            [&rwlock, &reader_started, &reader_acquired]()
            {
                reader_started.store(true);

                // blocks here until the writer releases
                c_shared_guard<c_shared::mutex_type> guard(rwlock);

                reader_acquired.store(true);
            });

        // wait until the reader thread is up and about to block
        started = spin_until(reader_started, k_deadline_ms);

        // give it time to reach lock_shared, then confirm it is still out
        sleep_ms(30);

        reader_was_blocked = (started && (!reader_acquired.load()));
    }   // writer released here

    reader.join();   // reader now acquires, sets its flag, and returns

    // after release, the reader must have gotten in
    const bool reader_got_in = reader_acquired.load();

    return (reader_was_blocked && reader_got_in);
}

/*
tests_c_recursive_is_exclusive_across_threads
  a recursive lock still excludes OTHER threads while one re-enters it.
  Tests the following:
  - k_agents threads each increment a shared counter k_iterations times; on
    every increment the thread holds TWO NESTED c_lock_guards over the same
    c_recursive mutex (a reentrant acquisition, which only a recursive mutex
    permits)
  - the final total is EXACTLY k_agents * k_iterations
  So recursion (one thread re-locking) and mutual exclusion (other threads kept
  out) hold at the same time: the reentrancy does not weaken the exclusion.  A
  plain c_exclusive mutex could not run this test - the nested lock would
  deadlock each thread.
*/
bool
tests_c_recursive_is_exclusive_across_threads()
{
    c_recursive::mutex_type  mutex;
    long                     counter = 0;
    std::atomic<bool>        go(false);
    std::atomic<int>         ready(0);

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
                    // nested reentrant acquisition of the SAME mutex
                    c_lock_guard<c_recursive::mutex_type> outer(mutex);
                    c_lock_guard<c_recursive::mutex_type> inner(mutex);
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
tests_c_spinlock_serializes_writers
  the spinlock gives mutual exclusion too.
  Tests the following:
  - k_agents threads each increment a shared plain long k_iterations times, each
    increment guarded by a c_lock_guard<c_spinlock::mutex_type>
  - the final total is EXACTLY k_agents * k_iterations
  The busy-wait spinlock serializes exactly as the blocking mutexes do; the
  exact total confirms the atomic_flag test-and-set really excludes.  Gated to
  C++11, where the spinlock exists.
*/
bool
tests_c_spinlock_serializes_writers()
{
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    c_spinlock::mutex_type   spin;
    long                     counter = 0;
    std::atomic<bool>        go(false);
    std::atomic<int>         ready(0);

    std::vector<std::thread> writers;
    int                      i = 0;

    for (i = 0; i < k_agents; ++i)
    {
        writers.push_back(std::thread(
            [&spin, &counter, &go, &ready]()
            {
                ready.fetch_add(1);

                while (!go.load())
                {
                    std::this_thread::yield();
                }

                int n = 0;

                for (n = 0; n < k_iterations; ++n)
                {
                    c_lock_guard<c_spinlock::mutex_type> guard(spin);
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
#else
    return true;
#endif
}

/*
tests_c_exclusive_try_lock_excludes_a_contending_thread
  a try-lock fails against a lock another thread holds.
  Tests the following:
  - a holder thread takes a c_exclusive mutex (via a c_lock_guard) and keeps it,
    announcing that it holds it
  - the main thread, once the holder is confirmed in, calls try_lock directly on
    the same mutex and gets false
  - so the mutex reports a genuine cross-thread conflict, not a false success
  The holder keeps the lock until the main thread has finished its attempt.
*/
bool
tests_c_exclusive_try_lock_excludes_a_contending_thread()
{
    c_exclusive::mutex_type mutex;
    std::atomic<bool>       holder_has_it(false);
    std::atomic<bool>       release(false);

    std::thread holder(
        [&mutex, &holder_has_it, &release]()
        {
            c_lock_guard<c_exclusive::mutex_type> guard(mutex);

            holder_has_it.store(true);

            while (!release.load())
            {
                std::this_thread::yield();
            }
        });

    const bool holder_in = spin_until(holder_has_it, k_deadline_ms);

    // the holder has it; our try must fail
    const bool contender_excluded = (!mutex.try_lock());

    if (!contender_excluded)
    {
        mutex.unlock();
    }

    release.store(true);
    holder.join();

    return (holder_in && contender_excluded);
}


NS_END  // testing
NS_END  // djinterp
