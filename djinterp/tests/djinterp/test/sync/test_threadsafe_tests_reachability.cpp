/*******************************************************************************
* djinterp [tests]                        test_threadsafe_tests_reachability.cpp
*
*   Section TU: every submodule's public entities resolve and construct after
* a single include of the umbrella.
*
*   Nothing here includes a submodule header directly - that is the whole
* point.  If an include is dropped from test_threadsafe.hpp, this TU stops
* compiling, which is exactly the regression signal wanted.
*
*
* path:      /tests/djinterp/test/sync/test_threadsafe_tests_reachability.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                                created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/

// std
#include <atomic>
#include <chrono>
#include <string>
// djinterp
#include "test_threadsafe_tests.hpp"


NS_DJINTERP
NS_TESTING


bool tests_ureach_thread_module()
{
    dt::test_thread       t;
    dt::test_thread_group g;

    D_HU_CHECK(t.is_idle());
    D_HU_CHECK(t.id() == 0u);
    D_HU_CHECK(g.empty());

    // the nested type aliases came along too
    D_HU_CHECK(t.elapsed() == dt::test_thread::duration_type::zero());

    return true;
}

bool tests_ureach_sync_module()
{
    dt::test_latch          latch(1);
    dt::test_barrier        barrier(1);
    dt::test_gate           gate;
    dt::test_rendezvous<int> rv;
    dt::simultaneous_start  start(1);

    D_HU_CHECK(latch.remaining() == 1u);
    D_HU_CHECK(barrier.expected() == 1u);
    D_HU_CHECK(!gate.is_open());
    D_HU_CHECK(start.expected() == 1u);

    int out = 0;
    D_HU_CHECK(!rv.try_receive(&out));

    return true;
}

bool tests_ureach_concurrent_module()
{
    dt::concurrent_runner     runner;
    dt::pipeline_runner       pipe(1);
    dt::concurrent_run_report report;
    dt::thread_report         tr;

    D_HU_CHECK(runner.thread_count() == 0u);
    D_HU_CHECK(pipe.phase_count() == 0u);
    D_HU_CHECK(!report.success());
    D_HU_CHECK(tr.id == 0u);

    return true;
}

bool tests_ureach_stress_module()
{
    dt::stress_runner runner;
    dt::timed_stress  timed;
    dt::chaos_runner  chaos;
    dt::stress_op     op;
    dt::stress_report report;

    D_HU_CHECK(runner.thread_count() == 1u);
    D_HU_CHECK(chaos.op_count() == 0u);
    D_HU_CHECK(op.weight == 1u);
    D_HU_CHECK(report.total_ops == 0u);

    // timed_stress exposes no getters; reaching a setter proves the type
    timed.set_thread_count(1);

    return true;
}

bool tests_ureach_race_module()
{
    dt::race_probe                probe;
    dt::atomicity_observer<int>   observer;
    dt::linearization_log         log;
    dt::consistency_check         checker;
    dt::race_probe_report         probe_report;
    dt::atomicity_observer_report obs_report;
    dt::linearization_event       event;

    D_HU_CHECK(log.size() == 0u);
    D_HU_CHECK(probe_report.success());
    D_HU_CHECK(obs_report.success());
    D_HU_CHECK(event.thread_id == 0u);
    D_HU_CHECK(checker.check(log).consistent);

    // reaching a setter on each configurable type proves it resolved
    probe.set_runs(1);
    observer.set_observer_count(1);

    return true;
}

bool tests_ureach_invariant_module()
{
    dt::invariant_monitor      monitor;
    dt::monotonic_guard<int>   mono;
    dt::bounded_guard<int>     bounded(0, 10);
    dt::invariant_report       report;
    dt::invariant_violation    violation;

    D_HU_CHECK(!monitor.running());
    D_HU_CHECK(mono.observations() == 0u);
    D_HU_CHECK(bounded.low() == 0);
    D_HU_CHECK(report.success());
    D_HU_CHECK(violation.detail.empty());

    return true;
}

bool tests_ureach_deadlock_module()
{
    dt::deadlock_watchdog     wd;
    dt::lock_order_tracker    tracker;
    dt::deadlock_report       report;
    dt::deadlock_event        event;

    D_HU_CHECK(!wd.armed());
    D_HU_CHECK(tracker.edge_count() == 0u);
    D_HU_CHECK(report.ok());
    D_HU_CHECK(event.thread_id == 0u);

    return true;
}

bool tests_ureach_enums()
{
    // every public enumeration across the subframework resolves
    D_HU_CHECK(dt::test_thread_state::idle !=
               dt::test_thread_state::running);

    D_HU_CHECK(dt::concurrent_run_outcome::success !=
               dt::concurrent_run_outcome::timed_out);

    D_HU_CHECK(dt::deadlock_event_kind::timeout !=
               dt::deadlock_event_kind::cycle);

    return true;
}

bool tests_ureach_factories()
{
    // the thread factories ...
    std::atomic<int> hits(0);

    dt::test_thread t =
        dt::make_test_thread(
            0,
            [&hits](std::size_t)
            {
                hits.fetch_add(1, std::memory_order_acq_rel);
            });
    D_HU_CHECK(t.id() == 0u);

    dt::test_thread named =
        dt::make_named_test_thread(
            1, std::string("n"), [](std::size_t) {});
    D_HU_CHECK(named.name() == "n");

    // ... and the execution patterns, which must also be callable
    const dt::concurrent_run_report rep =
        dt::run_simultaneous(
            2,
            [&hits](std::size_t)
            {
                hits.fetch_add(1, std::memory_order_acq_rel);
            });

    D_HU_CHECK(rep.outcome == dt::concurrent_run_outcome::success);
    D_HU_CHECK(hits.load(std::memory_order_acquire) == 2);

    return true;
}


NS_END  // testing
NS_END  // djinterp
