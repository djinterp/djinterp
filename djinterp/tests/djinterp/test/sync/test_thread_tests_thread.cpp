/*******************************************************************************
* djinterp [tests]                                  test_thread_tests_thread.cpp
*
*   Section TU: the test_thread class and test_thread_state enum.  Normal-mode
* include of the suite header brings the header under test, the fixtures, and
* D_HT_CHECK.  Bodies are flat in djinterp::testing.
*
*
* path:      /tests/djinterp/test/sync/test_thread_tests_thread.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                                created: TBA
*                                                            revised: 2026.10.23
*******************************************************************************/

// std
#include <atomic>
#include <chrono>
#include <thread>
// djinterp
#include "test_thread_tests.hpp"


NS_DJINTERP
NS_TESTING


// spin_until
//   local helper: poll a predicate up to a deadline, yielding between checks;
// returns whether it became true.  Keeps the transient-state assertions
// deterministic without relying on sleep timing.
namespace {

template<typename _Pred>
bool spin_until(_Pred _p, unsigned _budget_ms = 2000)
{
    const std::chrono::steady_clock::time_point deadline =
        std::chrono::steady_clock::now() +
        std::chrono::milliseconds(_budget_ms);

    while (std::chrono::steady_clock::now() < deadline)
    {
        if (_p())
        {
            return true;
        }

        std::this_thread::yield();
    }

    return _p();
}

}  // anonymous namespace


// ---------------------------------------------------------------------------

bool tests_thread_state_enum_values()
{
    // Verifies the five lifecycle enumerators are the distinct documented
    // values (idle,running,completed,failed,detached = 0..4).
    D_HT_CHECK(static_cast<int>(dt::test_thread_state::idle)      == 0);
    D_HT_CHECK(static_cast<int>(dt::test_thread_state::running)   == 1);
    D_HT_CHECK(static_cast<int>(dt::test_thread_state::completed) == 2);
    D_HT_CHECK(static_cast<int>(dt::test_thread_state::failed)    == 3);
    D_HT_CHECK(static_cast<int>(dt::test_thread_state::detached)  == 4);

    return true;
}

bool tests_thread_default_construct()
{
    dt::test_thread t;

    D_HT_CHECK(t.id() == 0u);
    D_HT_CHECK(t.name().empty());
    D_HT_CHECK(t.is_idle());
    D_HT_CHECK(t.state() == dt::test_thread_state::idle);
    D_HT_CHECK(!t.joinable());
    D_HT_CHECK(!t.failed());
    D_HT_CHECK(!t.completed());
    D_HT_CHECK(t.exception() == nullptr);

    return true;
}

bool tests_thread_construct_id_name_worker()
{
    ht_worker_ran sink;
    dt::test_thread t(7, std::string("writer"), ht_make_stamp(sink));

    D_HT_CHECK(t.id() == 7u);
    D_HT_CHECK(t.name() == "writer");
    D_HT_CHECK(t.is_idle());

    return true;
}

bool tests_thread_construct_id_worker()
{
    ht_worker_ran sink;
    dt::test_thread t(3, ht_make_stamp(sink));

    D_HT_CHECK(t.id() == 3u);
    D_HT_CHECK(t.name().empty());
    D_HT_CHECK(t.is_idle());

    return true;
}

bool tests_thread_set_name()
{
    dt::test_thread t;
    D_HT_CHECK(t.name().empty());

    t.set_name(std::string("renamed"));
    D_HT_CHECK(t.name() == "renamed");

    return true;
}

bool tests_thread_set_worker_before_start()
{
    ht_worker_ran sink;

    // an id, and no worker until set_worker gives it one
    dt::test_thread t(1, dt::test_thread::worker_fn());
    t.set_worker(ht_make_stamp(sink));

    t.start();
    t.join();

    D_HT_CHECK(t.completed());
    D_HT_CHECK(sink.ran.load(std::memory_order_acquire));

    return true;
}

bool tests_thread_start_runs_worker()
{
    ht_worker_ran sink;
    dt::test_thread t(9, ht_make_stamp(sink));

    t.start();
    t.join();

    D_HT_CHECK(sink.ran.load(std::memory_order_acquire));
    D_HT_CHECK(t.completed());
    D_HT_CHECK(!t.failed());

    return true;
}

bool tests_thread_start_without_worker_noop()
{
    dt::test_thread t;   // no worker bound

    t.start();

    D_HT_CHECK(t.is_idle());
    D_HT_CHECK(!t.joinable());

    return true;
}

bool tests_thread_start_twice_noop()
{
    // Verifies the idle-guard on start(): once the thread has left idle, a
    // second start() must not spawn the worker again.
    ht_worker_ran sink;
    dt::test_thread t(0, ht_make_stamp(sink));

    t.start();
    t.join();
    D_HT_CHECK(t.completed());

    t.start();   // state is completed, not idle -> no effect

    D_HT_CHECK(!t.joinable());
    D_HT_CHECK(sink.calls.load(std::memory_order_acquire) == 1);

    return true;
}

bool tests_thread_state_query_transitions()
{
    // Verifies is_idle -> is_running -> completed with a gate flag so the
    // running observation is deterministic.
    std::atomic<bool> go(false);
    std::atomic<bool> entered(false);

    dt::test_thread t(
        0,
        dt::test_thread::worker_fn(
            [&go, &entered](std::size_t)
            {
                entered.store(true, std::memory_order_release);
                while (!go.load(std::memory_order_acquire))
                {
                    std::this_thread::yield();
                }
            }));

    D_HT_CHECK(t.is_idle());

    t.start();

    D_HT_CHECK(spin_until([&]() {
        return entered.load(std::memory_order_acquire);
    }));
    D_HT_CHECK(t.is_running());
    D_HT_CHECK(!t.completed());
    D_HT_CHECK(!t.failed());

    go.store(true, std::memory_order_release);
    t.join();

    D_HT_CHECK(t.completed());
    D_HT_CHECK(!t.is_running());
    D_HT_CHECK(!t.is_idle());

    return true;
}

bool tests_thread_join_idempotent()
{
    dt::test_thread t;
    t.join();                       // not joinable: safe no-op
    D_HT_CHECK(!t.joinable());

    ht_worker_ran sink;
    dt::test_thread u(0, ht_make_stamp(sink));
    u.start();
    u.join();
    D_HT_CHECK(!u.joinable());
    u.join();                       // already joined: safe no-op
    D_HT_CHECK(u.completed());

    return true;
}

bool tests_thread_worker_receives_id()
{
    ht_worker_ran sink;
    dt::test_thread t(42, ht_make_stamp(sink));

    t.start();
    t.join();

    D_HT_CHECK(sink.seen_id.load(std::memory_order_acquire) == 42u);

    return true;
}

bool tests_thread_exception_capture()
{
    dt::test_thread t(0, ht_make_throwing("boom"));

    t.start();
    t.join();

    D_HT_CHECK(t.failed());
    D_HT_CHECK(!t.completed());
    D_HT_CHECK(t.exception() != nullptr);
    D_HT_CHECK(t.state() == dt::test_thread_state::failed);

    return true;
}

bool tests_thread_exception_what()
{
    dt::test_thread t(0, ht_make_throwing("kaboom"));

    t.start();
    t.join();

    D_HT_CHECK(t.exception_what() == std::string("kaboom"));

    return true;
}

bool tests_thread_exception_what_empty_on_success()
{
    ht_worker_ran sink;
    dt::test_thread t(0, ht_make_stamp(sink));

    t.start();
    t.join();

    D_HT_CHECK(t.exception_what().empty());

    return true;
}

bool tests_thread_rethrow_if_failed()
{
    dt::test_thread t(0, ht_make_throwing("rethrown"));

    t.start();
    t.join();

    bool caught = false;
    try
    {
        t.rethrow_if_failed();
    }
    catch (const std::runtime_error& e)
    {
        caught = (std::string(e.what()) == "rethrown");
    }
    catch (...)
    {
    }

    D_HT_CHECK(caught);

    return true;
}

bool tests_thread_rethrow_noop_on_success()
{
    ht_worker_ran sink;
    dt::test_thread t(0, ht_make_stamp(sink));

    t.start();
    t.join();

    bool threw = false;
    try
    {
        t.rethrow_if_failed();   // clean run: must not throw
    }
    catch (...)
    {
        threw = true;
    }

    D_HT_CHECK(!threw);

    return true;
}

bool tests_thread_timing_monotonic()
{
    dt::test_thread t(0, ht_make_sleeper(5));

    t.start();
    t.join();

    D_HT_CHECK(t.started_at() <= t.stopped_at());
    D_HT_CHECK(t.elapsed() >= dt::test_thread::duration_type::zero());

    return true;
}

bool tests_thread_elapsed_zero_when_idle()
{
    dt::test_thread t;   // never started

    D_HT_CHECK(t.elapsed() == dt::test_thread::duration_type::zero());

    return true;
}

bool tests_thread_detach_state()
{
    // A detached thread reaches the detached state and is no longer joinable.
    // Use a gate so the detached worker is guaranteed done before the flag it
    // touches goes out of scope.
    std::atomic<bool> go(false);
    std::atomic<bool> done(false);

    dt::test_thread t(
        0,
        dt::test_thread::worker_fn(
            [&go, &done](std::size_t)
            {
                while (!go.load(std::memory_order_acquire))
                {
                    std::this_thread::yield();
                }
                done.store(true, std::memory_order_release);
            }));

    t.start();
    D_HT_CHECK(spin_until([&]() { return t.is_running(); }));

    t.detach();

    D_HT_CHECK(t.state() == dt::test_thread_state::detached);
    D_HT_CHECK(!t.joinable());

    go.store(true, std::memory_order_release);
    D_HT_CHECK(spin_until([&]() {
        return done.load(std::memory_order_acquire);
    }));

    return true;
}

bool tests_thread_try_join_for_success()
{
    ht_worker_ran sink;
    dt::test_thread t(0, ht_make_stamp(sink));

    t.start();

    const bool joined =
        t.try_join_for(std::chrono::seconds(2));

    D_HT_CHECK(joined);
    D_HT_CHECK(t.completed());
    D_HT_CHECK(!t.joinable());

    return true;
}

bool tests_thread_try_join_for_timeout()
{
    dt::test_thread t(0, ht_make_sleeper(300));

    t.start();

    const bool joined =
        t.try_join_for(std::chrono::milliseconds(10));

    D_HT_CHECK(!joined);        // worker outlasts the window
    D_HT_CHECK(t.joinable());   // still running, still joinable

    t.join();                   // a plain join then still succeeds
    D_HT_CHECK(t.completed());

    return true;
}

bool tests_thread_move_construct()
{
    ht_worker_ran sink;
    dt::test_thread src(11, std::string("mover"), ht_make_stamp(sink));

    dt::test_thread dst(static_cast<dt::test_thread&&>(src));

    D_HT_CHECK(dst.id() == 11u);
    D_HT_CHECK(dst.name() == "mover");
    D_HT_CHECK(dst.is_idle());
    D_HT_CHECK(src.is_idle());   // source state reset by the move

    // the moved-to thread is fully functional
    dst.start();
    dst.join();
    D_HT_CHECK(dst.completed());
    D_HT_CHECK(sink.ran.load(std::memory_order_acquire));

    return true;
}


NS_END  // testing
NS_END  // djinterp
