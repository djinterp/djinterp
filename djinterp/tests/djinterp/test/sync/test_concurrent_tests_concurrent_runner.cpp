/*******************************************************************************
* djinterp [tests]                   test_concurrent_tests_concurrent_runner.cpp
*
*   Section TU: concurrent_runner - configuration, worker resolution, and the
* outcomes produced by run().
*
*
* path:      /tests/djinterp/test/sync/test_concurrent_tests_concurrent_runner.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                                created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/

// std
#include <atomic>
#include <chrono>
#include <mutex>
#include <set>
#include <string>
#include <vector>
// djinterp
#include "test_concurrent_tests.hpp"


NS_DJINTERP
NS_TESTING


bool tests_crunner_default_construct()
{
    dt::concurrent_runner runner;

    D_HC_CHECK(runner.thread_count() == 0u);
    D_HC_CHECK(runner.join_timeout() ==
               dt::concurrent_runner::duration_type::zero());

    return true;
}

bool tests_crunner_explicit_construct()
{
    dt::concurrent_runner runner(4);

    D_HC_CHECK(runner.thread_count() == 4u);

    return true;
}

bool tests_crunner_set_thread_count()
{
    dt::concurrent_runner runner(2);

    runner.set_thread_count(6);
    D_HC_CHECK(runner.thread_count() == 6u);

    runner.set_thread_count(0);
    D_HC_CHECK(runner.thread_count() == 0u);

    return true;
}

bool tests_crunner_thread_count_from_heterogeneous()
{
    hc_counter sink;
    dt::concurrent_runner runner(10);   // configured count ...

    runner.add_worker(hc_bump(sink));
    runner.add_worker(hc_bump(sink));

    // ... is overridden once per-thread workers are present
    D_HC_CHECK(runner.thread_count() == 2u);

    return true;
}

bool tests_crunner_clear_workers()
{
    hc_counter sink;
    dt::concurrent_runner runner(3);

    runner.add_worker(hc_bump(sink));
    D_HC_CHECK(runner.thread_count() == 1u);

    runner.clear_workers();

    // the heterogeneous list is empty again, so the configured count returns
    D_HC_CHECK(runner.thread_count() == 3u);

    // and no worker is bound, so a run cannot start
    const dt::concurrent_run_report rep = runner.run();
    D_HC_CHECK(rep.outcome == dt::concurrent_run_outcome::not_started);

    return true;
}

bool tests_crunner_join_timeout_accessors()
{
    dt::concurrent_runner runner(1);

    D_HC_CHECK(runner.join_timeout() ==
               dt::concurrent_runner::duration_type::zero());

    runner.set_join_timeout(std::chrono::milliseconds(50));
    D_HC_CHECK(runner.join_timeout() >
               dt::concurrent_runner::duration_type::zero());

    return true;
}

bool tests_crunner_run_zero_threads_not_started()
{
    hc_counter sink;
    dt::concurrent_runner runner(0);
    runner.set_worker(hc_bump(sink));

    const dt::concurrent_run_report rep = runner.run();

    D_HC_CHECK(rep.outcome == dt::concurrent_run_outcome::not_started);
    D_HC_CHECK(rep.thread_count == 0u);
    D_HC_CHECK(sink.calls.load(std::memory_order_acquire) == 0);

    return true;
}

bool tests_crunner_run_without_worker_not_started()
{
    dt::concurrent_runner runner(3);   // threads configured, no worker bound

    const dt::concurrent_run_report rep = runner.run();

    D_HC_CHECK(rep.outcome == dt::concurrent_run_outcome::not_started);

    return true;
}

bool tests_crunner_run_homogeneous_success()
{
    hc_counter sink;
    dt::concurrent_runner runner(4);
    runner.set_worker(hc_bump(sink));

    const dt::concurrent_run_report rep = runner.run();

    D_HC_CHECK(rep.outcome == dt::concurrent_run_outcome::success);
    D_HC_CHECK(rep.success());
    D_HC_CHECK(rep.thread_count == 4u);
    D_HC_CHECK(rep.completed_count == 4u);
    D_HC_CHECK(rep.failed_count == 0u);
    D_HC_CHECK(rep.first_exception == nullptr);

    return true;
}

bool tests_crunner_run_invokes_each_thread_once()
{
    hc_counter sink;
    dt::concurrent_runner runner(5);
    runner.set_worker(hc_bump(sink));

    runner.run();

    D_HC_CHECK(sink.calls.load(std::memory_order_acquire) == 5);

    return true;
}

bool tests_crunner_run_thread_ids_unique()
{
    std::mutex           mx;
    std::set<std::size_t> ids;

    dt::concurrent_runner runner(4);
    runner.set_worker(
        dt::test_thread::worker_fn(
            [&mx, &ids](std::size_t _id)
            {
                std::lock_guard<std::mutex> lk(mx);
                ids.insert(_id);
            }));

    runner.run();

    D_HC_CHECK(ids.size() == 4u);
    D_HC_CHECK(ids.count(0) == 1u);
    D_HC_CHECK(ids.count(1) == 1u);
    D_HC_CHECK(ids.count(2) == 1u);
    D_HC_CHECK(ids.count(3) == 1u);

    return true;
}

bool tests_crunner_run_heterogeneous()
{
    hc_counter a;
    hc_counter b;
    hc_counter c;

    dt::concurrent_runner runner;
    runner.add_worker(hc_bump(a));
    runner.add_worker(hc_bump(b));
    runner.add_worker(hc_bump(c));

    const dt::concurrent_run_report rep = runner.run();

    D_HC_CHECK(rep.outcome == dt::concurrent_run_outcome::success);
    D_HC_CHECK(rep.thread_count == 3u);
    D_HC_CHECK(a.calls.load(std::memory_order_acquire) == 1);
    D_HC_CHECK(b.calls.load(std::memory_order_acquire) == 1);
    D_HC_CHECK(c.calls.load(std::memory_order_acquire) == 1);

    return true;
}

bool tests_crunner_run_per_thread_reports()
{
    hc_counter sink;
    dt::concurrent_runner runner(3);
    runner.set_worker(hc_bump(sink));

    const dt::concurrent_run_report rep = runner.run();

    D_HC_CHECK(rep.threads.size() == 3u);

    for (std::size_t i = 0; i < rep.threads.size(); ++i)
    {
        const dt::thread_report& tr = rep.threads[i];

        D_HC_CHECK(tr.id == i);
        D_HC_CHECK(tr.final_state == dt::test_thread_state::completed);
        D_HC_CHECK(!tr.failed);
        D_HC_CHECK(tr.exception_message.empty());
    }

    return true;
}

bool tests_crunner_run_thread_threw()
{
    dt::concurrent_runner runner(2);
    runner.set_worker(hc_thrower("worker blew up"));

    const dt::concurrent_run_report rep = runner.run();

    D_HC_CHECK(rep.outcome == dt::concurrent_run_outcome::thread_threw);
    D_HC_CHECK(!rep.success());
    D_HC_CHECK(rep.failed_count == 2u);
    D_HC_CHECK(rep.completed_count == 0u);
    D_HC_CHECK(rep.first_exception != nullptr);

    for (std::size_t i = 0; i < rep.threads.size(); ++i)
    {
        D_HC_CHECK(rep.threads[i].failed);
        D_HC_CHECK(rep.threads[i].exception_message == "worker blew up");
    }

    return true;
}

bool tests_crunner_run_first_exception()
{
    dt::concurrent_runner runner(1);
    runner.set_worker(hc_thrower("captured"));

    const dt::concurrent_run_report rep = runner.run();

    D_HC_CHECK(rep.first_exception != nullptr);

    std::string what;
    try
    {
        std::rethrow_exception(rep.first_exception);
    }
    catch (const std::runtime_error& e)
    {
        what = e.what();
    }
    catch (...)
    {
    }

    D_HC_CHECK(what == "captured");

    return true;
}

bool tests_crunner_run_partial_failure_counts()
{
    hc_counter ok;

    dt::concurrent_runner runner;
    runner.add_worker(hc_bump(ok));
    runner.add_worker(hc_thrower("bad"));
    runner.add_worker(hc_bump(ok));

    const dt::concurrent_run_report rep = runner.run();

    D_HC_CHECK(rep.outcome == dt::concurrent_run_outcome::thread_threw);
    D_HC_CHECK(rep.thread_count == 3u);
    D_HC_CHECK(rep.completed_count == 2u);
    D_HC_CHECK(rep.failed_count == 1u);
    D_HC_CHECK(ok.calls.load(std::memory_order_acquire) == 2);

    return true;
}

bool tests_crunner_run_timeout_outcome()
{
    // the worker outlasts the join window by an order of magnitude, so the
    // timed_out verdict is deterministic; the group destructor then joins.
    dt::concurrent_runner runner(2);
    runner.set_worker(hc_sleeper(hc_long_ms()));
    runner.set_join_timeout(hc_short());

    const dt::concurrent_run_report rep = runner.run();

    D_HC_CHECK(rep.outcome == dt::concurrent_run_outcome::timed_out);
    D_HC_CHECK(!rep.success());
    D_HC_CHECK(rep.thread_count == 2u);
    D_HC_CHECK(rep.threads.size() == 2u);

    return true;
}

bool tests_crunner_simultaneous_start()
{
    hc_counter sink;
    dt::concurrent_runner runner(4);
    runner.set_worker(hc_bump(sink));
    runner.set_simultaneous_start(true);

    const dt::concurrent_run_report rep = runner.run();

    D_HC_CHECK(rep.outcome == dt::concurrent_run_outcome::success);
    D_HC_CHECK(rep.completed_count == 4u);
    D_HC_CHECK(sink.calls.load(std::memory_order_acquire) == 4);

    // and the handshake can be turned back off
    runner.set_simultaneous_start(false);
    const dt::concurrent_run_report again = runner.run();
    D_HC_CHECK(again.outcome == dt::concurrent_run_outcome::success);
    D_HC_CHECK(sink.calls.load(std::memory_order_acquire) == 8);

    return true;
}

bool tests_crunner_wall_clock_nonnegative()
{
    hc_counter sink;
    dt::concurrent_runner runner(2);
    runner.set_worker(hc_bump(sink));

    const dt::concurrent_run_report rep = runner.run();

    D_HC_CHECK(rep.wall_clock >=
               dt::concurrent_run_report::duration_type::zero());

    return true;
}


NS_END  // testing
NS_END  // djinterp
