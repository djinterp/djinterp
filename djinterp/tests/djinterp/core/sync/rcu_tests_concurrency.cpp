/*******************************************************************************
* djinterp [core]                                      rcu_tests_concurrency.cpp
*
*
* path:      /tests/djinterp/core/sync/rcu_tests_concurrency.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp [test]                   rcu_tests_concurrency.cpp
//   Section: the reclamation protocol under real threads.  The single-thread
// reclaimer section already pins the safety RULE deterministically (a pinned
// reader holds a node back; releasing it lets the node go); this section puts
// genuine contention on top - concurrent slot claiming, and the end-to-end RCU
// invariant that a reader never dereferences a node the writer has freed.
//
// The safety test builds the RCU pattern from the primitives directly, with a
// PER-THREAD reader slot each, rather than through rcu_protected - whose single
// shared reader slot makes its read API single-reader by construction.  It is
// the harness the sanitizers validate: under AddressSanitizer a premature free
// surfaces as a use-after-free on the reader's dereference, and the rc_node
// magic stamp is a second, cheaper check.

// std
#include <vector>
// djinterp
#include "rcu_tests.hpp"


NS_DJINTERP
NS_TESTING

/*
tests_registry_concurrent_registration_is_distinct
  concurrent claimers each get their own slot.
  Tests the following:
  - with capacity exactly k_agents and k_agents threads each claiming once,
    every claim returns a non-null slot and no two threads receive the same
    pointer
  The lock-free CAS on the active flag must serialize claims so slots are never
  double-handed-out.
*/
bool
tests_registry_concurrent_registration_is_distinct()
{
    epoch_registry registry(k_agents);

    std::atomic<std::uint64_t>* slots[k_agents];
    for (int i = 0; i < k_agents; ++i)
    {
        slots[i] = NULL;
    }

    std::atomic<bool>        go(false);
    std::vector<std::thread> workers;

    for (int i = 0; i < k_agents; ++i)
    {
        workers.push_back(std::thread(
            [&registry, &slots, &go, i]()
            {
                spin_until(go, k_deadline_ms);
                slots[i] = registry.register_thread();
            }));
    }

    go.store(true);
    for (std::size_t i = 0; i < workers.size(); ++i)
    {
        workers[i].join();
    }

    // all claims succeeded
    for (int i = 0; i < k_agents; ++i)
    {
        if (!slots[i])
        {
            return false;
        }
    }

    // all distinct
    for (int i = 0; i < k_agents; ++i)
    {
        for (int j = i + 1; j < k_agents; ++j)
        {
            if (slots[i] == slots[j])
            {
                return false;
            }
        }
    }

    return true;
}

/*
tests_registry_concurrent_registration_respects_capacity
  under oversubscription, exactly capacity claims succeed.
  Tests the following:
  - with capacity 2 and 2*k_agents threads all racing to claim, precisely 2
    register_thread() calls return non-null and the rest get nullptr
  The pool never over-commits, no matter how many threads contend.
*/
bool
tests_registry_concurrent_registration_respects_capacity()
{
    const int capacity = 2;
    const int agents   = k_agents * 2;

    epoch_registry registry(capacity);

    std::atomic<int>         successes(0);
    std::atomic<bool>        go(false);
    std::vector<std::thread> workers;

    for (int i = 0; i < agents; ++i)
    {
        workers.push_back(std::thread(
            [&registry, &successes, &go]()
            {
                spin_until(go, k_deadline_ms);
                if (registry.register_thread() != NULL)
                {
                    successes.fetch_add(1);
                }
            }));
    }

    go.store(true);
    for (std::size_t i = 0; i < workers.size(); ++i)
    {
        workers[i].join();
    }

    return (successes.load() == capacity);
}

/*
tests_concurrent_readers_never_observe_reclaimed_data
  the end-to-end RCU invariant holds under contention.
  Tests the following:
  - several reader threads, each with its own slot, repeatedly enter a critical
    section, load the shared node pointer, and verify the node's magic stamp,
    while a writer publishes fresh nodes and retires the old ones through the
    shared reclaimer
  - no reader ever sees a corrupt or freed node (the magic check never fails,
    and under AddressSanitizer no use-after-free is reported)
  Because a reader records its epoch before loading the pointer and the writer
  advances the epoch only after swapping, any node a reader can still see is at
  or above the frontier and cannot be reclaimed until that reader exits.  The
  final force_reclaim runs only after every reader has joined.
*/
bool
tests_concurrent_readers_never_observe_reclaimed_data()
{
    deferred_reclaimer<rc_node> reclaimer(k_agents + 4);
    std::atomic<rc_node*>       data(new rc_node(0));

    std::atomic<bool> go(false);
    std::atomic<bool> writer_done(false);
    std::atomic<bool> bad(false);

    std::vector<std::thread> workers;

    // readers
    for (int r = 0; r < k_agents; ++r)
    {
        workers.push_back(std::thread(
            [&reclaimer, &data, &go, &writer_done, &bad]()
            {
                std::atomic<std::uint64_t>* slot =
                    reclaimer.register_reader();
                if (!slot)
                {
                    bad.store(true);
                    return;
                }

                spin_until(go, k_deadline_ms);

                while (!writer_done.load())
                {
                    epoch_guard guard(reclaimer.epoch(), *slot);

                    rc_node* node =
                        data.load(std::memory_order_acquire);

                    if (!node || node->magic != k_node_magic)
                    {
                        bad.store(true);
                    }
                }

                reclaimer.unregister_reader(slot);
            }));
    }

    // writer
    workers.push_back(std::thread(
        [&reclaimer, &data, &go, &writer_done]()
        {
            spin_until(go, k_deadline_ms);

            for (int i = 1; i <= k_iterations; ++i)
            {
                rc_node* fresh = new rc_node(i);
                rc_node* old   =
                    data.exchange(fresh, std::memory_order_acq_rel);

                reclaimer.retire(old);          // auto-scans at threshold

                if ((i & 0x3F) == 0)
                {
                    reclaimer.scan();
                }
            }

            writer_done.store(true);
        }));

    go.store(true);
    for (std::size_t i = 0; i < workers.size(); ++i)
    {
        workers[i].join();
    }

    // every reader has exited and the writer is done, so nothing can be read
    // now - safe to drop the remaining retired nodes and the live one.
    reclaimer.force_reclaim();
    delete data.load(std::memory_order_relaxed);

    return !bad.load();
}

/*
tests_concurrent_retire_and_scan_make_progress
  a writer retiring and scanning amid churning readers makes full progress.
  Tests the following:
  - with reader threads continuously entering and leaving critical sections, a
    writer that retires k_iterations nodes and periodically scans completes
    every retirement (the epoch advances by exactly k_iterations) and deadlocks
    on nothing
  - after all threads join, a force_reclaim drains the pending list to empty
  This is the liveness companion to the safety test: the lock-free scan must not
  stall against a moving minimum-active-epoch.
*/
bool
tests_concurrent_retire_and_scan_make_progress()
{
    deferred_reclaimer<int> reclaimer(k_agents + 4);

    std::atomic<bool> go(false);
    std::atomic<bool> writer_done(false);
    std::atomic<bool> reader_failed(false);

    std::vector<std::thread> workers;

    // readers churn critical sections to keep the frontier moving
    for (int r = 0; r < k_agents; ++r)
    {
        workers.push_back(std::thread(
            [&reclaimer, &go, &writer_done, &reader_failed]()
            {
                std::atomic<std::uint64_t>* slot =
                    reclaimer.register_reader();
                if (!slot)
                {
                    reader_failed.store(true);
                    return;
                }

                spin_until(go, k_deadline_ms);

                while (!writer_done.load())
                {
                    epoch_guard guard(reclaimer.epoch(), *slot);
                    (void)guard;
                }

                reclaimer.unregister_reader(slot);
            }));
    }

    // writer retires and scans
    workers.push_back(std::thread(
        [&reclaimer, &go, &writer_done]()
        {
            spin_until(go, k_deadline_ms);

            for (int i = 1; i <= k_iterations; ++i)
            {
                reclaimer.retire(new int(i));

                if ((i & 0x0F) == 0)
                {
                    reclaimer.scan();
                }
            }

            writer_done.store(true);
        }));

    go.store(true);
    for (std::size_t i = 0; i < workers.size(); ++i)
    {
        workers[i].join();
    }

    reclaimer.force_reclaim();

    const bool epoch_advanced =
        (reclaimer.current_epoch() ==
         static_cast<std::uint64_t>(k_iterations));
    const bool drained = (reclaimer.pending() == 0);

    return (epoch_advanced && drained && !reader_failed.load());
}


NS_END  // testing
NS_END  // djinterp
