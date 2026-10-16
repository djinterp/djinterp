/*******************************************************************************
* djinterp [tests]                            test_deadlock_tests_lock_order.cpp
*
*   Section TU: lock_order_tracker - the acquisition-order graph, immediate
* AB-BA detection, and full cycle analysis.
*
*   The tracker records history rather than observing live locks, so almost
* everything here is driven by replaying hand-built acquisition sequences.
* That makes every assertion exact: no real contention is involved.
*
*
* path:      /tests/djinterp/test/sync/test_deadlock_tests_lock_order.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                                created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/

// std
#include <thread>
#include <vector>
// djinterp
#include "test_deadlock_tests.hpp"


NS_DJINTERP
NS_TESTING


bool tests_dorder_empty()
{
    dt::lock_order_tracker tracker;

    D_HD_CHECK(tracker.edge_count() == 0u);
    D_HD_CHECK(tracker.report().ok());
    D_HD_CHECK(tracker.report().cycle_count == 0u);

    return true;
}

bool tests_dorder_single_acquire_no_edge()
{
    dt::lock_order_tracker tracker;

    tracker.on_acquire(0, 1);   // nothing held beforehand

    D_HD_CHECK(tracker.edge_count() == 0u);
    D_HD_CHECK(tracker.report().ok());

    return true;
}

bool tests_dorder_nested_acquire_adds_edge()
{
    dt::lock_order_tracker tracker;

    tracker.on_acquire(0, 1);   // hold A
    tracker.on_acquire(0, 2);   // take B while holding A -> edge A->B

    D_HD_CHECK(tracker.edge_count() == 1u);
    D_HD_CHECK(tracker.report().ok());   // one direction only: no cycle

    return true;
}

bool tests_dorder_edges_deduplicated()
{
    dt::lock_order_tracker tracker;

    for (int i = 0; i < 5; ++i)
    {
        tracker.on_acquire(0, 1);
        tracker.on_acquire(0, 2);
        tracker.on_release(0, 2);
        tracker.on_release(0, 1);
    }

    // the same ordering seen five times is still one edge
    D_HD_CHECK(tracker.edge_count() == 1u);
    D_HD_CHECK(tracker.report().ok());

    return true;
}

bool tests_dorder_release_pops_held()
{
    dt::lock_order_tracker tracker;

    tracker.on_acquire(0, 1);
    tracker.on_release(0, 1);   // A no longer held

    // B is now taken with nothing held, so no edge is created
    tracker.on_acquire(0, 2);

    D_HD_CHECK(tracker.edge_count() == 0u);

    return true;
}

bool tests_dorder_release_unknown_thread()
{
    dt::lock_order_tracker tracker;

    tracker.on_release(99, 1);   // thread never seen: safe no-op

    D_HD_CHECK(tracker.edge_count() == 0u);
    D_HD_CHECK(tracker.report().ok());

    return true;
}

bool tests_dorder_sequential_no_edge()
{
    dt::lock_order_tracker tracker;

    // strictly non-overlapping acquisition creates no ordering constraint
    tracker.on_acquire(0, 1);
    tracker.on_release(0, 1);
    tracker.on_acquire(0, 2);
    tracker.on_release(0, 2);
    tracker.on_acquire(0, 3);
    tracker.on_release(0, 3);

    D_HD_CHECK(tracker.edge_count() == 0u);
    D_HD_CHECK(tracker.report().ok());

    return true;
}

bool tests_dorder_consistent_order_no_cycle()
{
    dt::lock_order_tracker tracker;

    // both threads take A then B - the classic CORRECT discipline
    tracker.on_acquire(0, 1);
    tracker.on_acquire(0, 2);
    tracker.on_release(0, 2);
    tracker.on_release(0, 1);

    tracker.on_acquire(1, 1);
    tracker.on_acquire(1, 2);
    tracker.on_release(1, 2);
    tracker.on_release(1, 1);

    D_HD_CHECK(tracker.edge_count() == 1u);      // A->B only
    D_HD_CHECK(tracker.report().cycle_count == 0u);
    D_HD_CHECK(tracker.report().ok());

    return true;
}

bool tests_dorder_ab_ba_detected()
{
    dt::lock_order_tracker tracker;

    // thread 0 takes A then B
    tracker.on_acquire(0, 1);
    tracker.on_acquire(0, 2);
    tracker.on_release(0, 2);
    tracker.on_release(0, 1);

    // thread 1 takes B then A - the reverse edge closes the cycle
    tracker.on_acquire(1, 2);
    tracker.on_acquire(1, 1);

    const dt::deadlock_report& rep = tracker.report();

    D_HD_CHECK(rep.cycle_count == 1u);
    D_HD_CHECK(rep.fired);
    D_HD_CHECK(!rep.ok());
    D_HD_CHECK(rep.event_count() == 1u);
    D_HD_CHECK(rep.events[0].kind == dt::deadlock_event_kind::cycle);

    return true;
}

bool tests_dorder_ab_ba_event_fields()
{
    dt::lock_order_tracker tracker;

    tracker.on_acquire(0, 10);
    tracker.on_acquire(0, 20);
    tracker.on_release(0, 20);
    tracker.on_release(0, 10);

    tracker.on_acquire(7, 20);
    tracker.on_acquire(7, 10);   // offending acquisition

    const dt::deadlock_report& rep = tracker.report();
    D_HD_CHECK(rep.event_count() == 1u);

    const dt::deadlock_event& ev = rep.events[0];
    D_HD_CHECK(ev.kind == dt::deadlock_event_kind::cycle);
    D_HD_CHECK(ev.thread_id == 7u);          // the thread that closed it
    D_HD_CHECK(ev.lock_id_a == 20u);         // already held
    D_HD_CHECK(ev.lock_id_b == 10u);         // being taken
    D_HD_CHECK(!ev.description.empty());

    return true;
}

bool tests_dorder_analyze_clean_graph()
{
    dt::lock_order_tracker tracker;

    // a strict chain A->B->C: acyclic
    tracker.on_acquire(0, 1);
    tracker.on_acquire(0, 2);
    tracker.on_acquire(0, 3);
    tracker.on_release(0, 3);
    tracker.on_release(0, 2);
    tracker.on_release(0, 1);

    const dt::deadlock_report rep = tracker.analyze();

    D_HD_CHECK(rep.cycle_count == 0u);
    D_HD_CHECK(rep.ok());

    return true;
}

bool tests_dorder_analyze_finds_long_cycle()
{
    // A->B, B->C, C->A: no single acquisition sees a reverse edge, so the
    // three-lock cycle is only visible to the graph walk
    dt::lock_order_tracker tracker;

    tracker.on_acquire(0, 1);   // hold A
    tracker.on_acquire(0, 2);   // A->B
    tracker.on_release(0, 2);
    tracker.on_release(0, 1);

    tracker.on_acquire(1, 2);   // hold B
    tracker.on_acquire(1, 3);   // B->C
    tracker.on_release(1, 3);
    tracker.on_release(1, 2);

    tracker.on_acquire(2, 3);   // hold C
    tracker.on_acquire(2, 1);   // C->A
    tracker.on_release(2, 1);
    tracker.on_release(2, 3);

    // nothing was caught during acquisition
    D_HD_CHECK(tracker.report().cycle_count == 0u);
    D_HD_CHECK(tracker.edge_count() == 3u);

    const dt::deadlock_report rep = tracker.analyze();

    D_HD_CHECK(rep.cycle_count > 0u);
    D_HD_CHECK(rep.fired);
    D_HD_CHECK(!rep.ok());

    return true;
}

bool tests_dorder_analyze_preserves_prior()
{
    dt::lock_order_tracker tracker;

    // provoke an immediate AB-BA detection first
    tracker.on_acquire(0, 1);
    tracker.on_acquire(0, 2);
    tracker.on_release(0, 2);
    tracker.on_release(0, 1);
    tracker.on_acquire(1, 2);
    tracker.on_acquire(1, 1);

    const std::size_t caught_live = tracker.report().cycle_count;
    D_HD_CHECK(caught_live == 1u);

    const dt::deadlock_report rep = tracker.analyze();

    // analysis adds to the record rather than replacing it
    D_HD_CHECK(rep.cycle_count >= caught_live);
    D_HD_CHECK(rep.fired);

    return true;
}

bool tests_dorder_reset()
{
    dt::lock_order_tracker tracker;

    tracker.on_acquire(0, 1);
    tracker.on_acquire(0, 2);
    tracker.on_acquire(1, 2);
    tracker.on_acquire(1, 1);   // cycle
    D_HD_CHECK(tracker.edge_count() > 0u);
    D_HD_CHECK(!tracker.report().ok());

    tracker.reset();

    D_HD_CHECK(tracker.edge_count() == 0u);
    D_HD_CHECK(tracker.report().ok());
    D_HD_CHECK(tracker.report().cycle_count == 0u);

    // the thread stacks were cleared too: a fresh nested acquisition on the
    // same thread starts from an empty held set
    tracker.on_acquire(0, 5);
    D_HD_CHECK(tracker.edge_count() == 0u);

    return true;
}

bool tests_dorder_concurrent_recording_mt()
{
    // every thread records the SAME consistent ordering (A before B), so no
    // cycle is possible however the calls interleave; the tracker's internal
    // locking is what is under test.
    const std::size_t threads = 4;
    const std::size_t per     = 200;

    dt::lock_order_tracker tracker;

    std::vector<std::thread> ts;
    for (std::size_t t = 0; t < threads; ++t)
    {
        ts.emplace_back(
            [&tracker, t]()
            {
                for (std::size_t i = 0; i < per; ++i)
                {
                    tracker.on_acquire(t, 1);
                    tracker.on_acquire(t, 2);
                    tracker.on_release(t, 2);
                    tracker.on_release(t, 1);
                }
            });
    }

    for (std::size_t i = 0; i < ts.size(); ++i)
    {
        ts[i].join();
    }

    D_HD_CHECK(tracker.edge_count() == 1u);          // A->B, deduplicated
    D_HD_CHECK(tracker.report().cycle_count == 0u);
    D_HD_CHECK(tracker.report().ok());

    return true;
}


NS_END  // testing
NS_END  // djinterp
