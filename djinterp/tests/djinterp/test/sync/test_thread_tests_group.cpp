/*******************************************************************************
* djinterp [tests]                                   test_thread_tests_group.cpp
*
*   Section TU: test_thread_group - membership, bulk start/join, aggregate
* verdict queries, and iteration.
*
*
* path:      /tests/djinterp/test/sync/test_thread_tests_group.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                                created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/

// std
#include <atomic>
#include <chrono>
#include <string>
// djinterp
#include "test_thread_tests.hpp"


NS_DJINTERP
NS_TESTING


bool tests_group_default_empty()
{
    dt::test_thread_group g;

    D_HT_CHECK(g.empty());
    D_HT_CHECK(g.size() == 0u);
    D_HT_CHECK(!g.any_failed());
    D_HT_CHECK(g.first_exception() == nullptr);

    return true;
}

bool tests_group_emplace_id_name_worker()
{
    ht_worker_ran sink;
    dt::test_thread_group g;

    dt::test_thread& t = g.emplace(5, std::string("w5"), ht_make_stamp(sink));

    D_HT_CHECK(g.size() == 1u);
    D_HT_CHECK(!g.empty());
    D_HT_CHECK(t.id() == 5u);
    D_HT_CHECK(t.name() == "w5");
    D_HT_CHECK(&t == &g.at(0));

    return true;
}

bool tests_group_emplace_id_worker()
{
    ht_worker_ran sink;
    dt::test_thread_group g;

    dt::test_thread& t = g.emplace(2, ht_make_stamp(sink));

    D_HT_CHECK(g.size() == 1u);
    D_HT_CHECK(t.id() == 2u);
    D_HT_CHECK(t.name().empty());

    return true;
}

bool tests_group_emplace_auto_id()
{
    ht_worker_ran s0, s1, s2;
    dt::test_thread_group g;

    dt::test_thread& a = g.emplace(ht_make_stamp(s0));
    dt::test_thread& b = g.emplace(ht_make_stamp(s1));
    dt::test_thread& c = g.emplace(ht_make_stamp(s2));

    D_HT_CHECK(a.id() == 0u);
    D_HT_CHECK(b.id() == 1u);
    D_HT_CHECK(c.id() == 2u);
    D_HT_CHECK(g.size() == 3u);

    return true;
}

bool tests_group_at_access()
{
    ht_worker_ran s0, s1;
    dt::test_thread_group g;
    g.emplace(0, std::string("zero"), ht_make_stamp(s0));
    g.emplace(1, std::string("one"),  ht_make_stamp(s1));

    D_HT_CHECK(g.at(0).name() == "zero");
    D_HT_CHECK(g.at(1).name() == "one");

    const dt::test_thread_group& cg = g;
    D_HT_CHECK(cg.at(0).id() == 0u);
    D_HT_CHECK(cg.at(1).id() == 1u);

    return true;
}

bool tests_group_start_join_all()
{
    ht_worker_ran s0, s1, s2;
    dt::test_thread_group g;
    g.emplace(0, ht_make_stamp(s0));
    g.emplace(1, ht_make_stamp(s1));
    g.emplace(2, ht_make_stamp(s2));

    g.start_all();
    g.join_all();

    D_HT_CHECK(s0.ran.load(std::memory_order_acquire));
    D_HT_CHECK(s1.ran.load(std::memory_order_acquire));
    D_HT_CHECK(s2.ran.load(std::memory_order_acquire));
    D_HT_CHECK(g.all_completed());

    return true;
}

bool tests_group_any_failed_false_on_success()
{
    ht_worker_ran s0, s1;
    dt::test_thread_group g;
    g.emplace(0, ht_make_stamp(s0));
    g.emplace(1, ht_make_stamp(s1));

    g.start_all();
    g.join_all();

    D_HT_CHECK(!g.any_failed());
    D_HT_CHECK(g.failure_count() == 0u);

    return true;
}

bool tests_group_any_failed_true_on_throw()
{
    ht_worker_ran s0, s2;
    dt::test_thread_group g;
    g.emplace(0, ht_make_stamp(s0));
    g.emplace(1, ht_make_throwing("x"));
    g.emplace(2, ht_make_stamp(s2));

    g.start_all();
    g.join_all();

    D_HT_CHECK(g.any_failed());
    D_HT_CHECK(!g.all_completed());

    return true;
}

bool tests_group_all_completed_requires_nonempty()
{
    dt::test_thread_group empty;
    D_HT_CHECK(!empty.all_completed());   // empty is NOT all-completed

    ht_worker_ran s0, s1;
    dt::test_thread_group ok;
    ok.emplace(0, ht_make_stamp(s0));
    ok.emplace(1, ht_make_stamp(s1));
    ok.start_all();
    ok.join_all();
    D_HT_CHECK(ok.all_completed());

    dt::test_thread_group bad;
    bad.emplace(0, ht_make_throwing("f"));
    bad.start_all();
    bad.join_all();
    D_HT_CHECK(!bad.all_completed());

    return true;
}

bool tests_group_failure_count()
{
    ht_worker_ran s0;
    dt::test_thread_group g;
    g.emplace(0, ht_make_stamp(s0));
    g.emplace(1, ht_make_throwing("a"));
    g.emplace(2, ht_make_throwing("b"));

    g.start_all();
    g.join_all();

    D_HT_CHECK(g.failure_count() == 2u);

    return true;
}

bool tests_group_first_exception_in_id_order()
{
    ht_worker_ran s0;
    dt::test_thread_group g;
    g.emplace(0, ht_make_stamp(s0));       // ok
    g.emplace(1, ht_make_throwing("first"));
    g.emplace(2, ht_make_throwing("second"));

    g.start_all();
    g.join_all();

    std::exception_ptr ep = g.first_exception();
    D_HT_CHECK(ep != nullptr);

    std::string what;
    try
    {
        std::rethrow_exception(ep);
    }
    catch (const std::runtime_error& e)
    {
        what = e.what();
    }
    catch (...)
    {
    }

    D_HT_CHECK(what == "first");   // earliest thrower in member order

    return true;
}

bool tests_group_rethrow_first_failure()
{
    ht_worker_ran s0;
    dt::test_thread_group g;
    g.emplace(0, ht_make_stamp(s0));
    g.emplace(1, ht_make_throwing("propagated"));

    g.start_all();
    g.join_all();

    std::string what;
    try
    {
        g.rethrow_first_failure();
    }
    catch (const std::runtime_error& e)
    {
        what = e.what();
    }
    catch (...)
    {
    }

    D_HT_CHECK(what == "propagated");

    return true;
}

bool tests_group_rethrow_noop_when_clean()
{
    ht_worker_ran s0, s1;
    dt::test_thread_group g;
    g.emplace(0, ht_make_stamp(s0));
    g.emplace(1, ht_make_stamp(s1));

    g.start_all();
    g.join_all();

    bool threw = false;
    try
    {
        g.rethrow_first_failure();
    }
    catch (...)
    {
        threw = true;
    }

    D_HT_CHECK(!threw);

    return true;
}

bool tests_group_clear()
{
    ht_worker_ran s0, s1;
    dt::test_thread_group g;
    g.emplace(0, ht_make_stamp(s0));
    g.emplace(1, ht_make_stamp(s1));

    g.start_all();
    g.join_all();

    g.clear();

    D_HT_CHECK(g.empty());
    D_HT_CHECK(g.size() == 0u);

    return true;
}

bool tests_group_range_iteration()
{
    ht_worker_ran s0, s1, s2;
    dt::test_thread_group g;
    g.emplace(0, ht_make_stamp(s0));
    g.emplace(1, ht_make_stamp(s1));
    g.emplace(2, ht_make_stamp(s2));

    g.start_all();
    g.join_all();

    std::size_t seen      = 0;
    std::size_t completed = 0;
    for (const dt::test_thread& t : g)
    {
        ++seen;
        if (t.completed())
        {
            ++completed;
        }
    }

    D_HT_CHECK(seen == 3u);
    D_HT_CHECK(completed == 3u);

    return true;
}

bool tests_group_try_join_all_for()
{
    // fast group joins inside a generous window
    ht_worker_ran s0, s1;
    dt::test_thread_group fast;
    fast.emplace(0, ht_make_stamp(s0));
    fast.emplace(1, ht_make_stamp(s1));
    fast.start_all();
    D_HT_CHECK(fast.try_join_all_for(std::chrono::seconds(2)));

    // a slow member overruns a tiny window -> false; join_all then cleans up
    ht_worker_ran s2;
    dt::test_thread_group slow;
    slow.emplace(0, ht_make_stamp(s2));
    slow.emplace(1, ht_make_sleeper(300));
    slow.start_all();
    D_HT_CHECK(!slow.try_join_all_for(std::chrono::milliseconds(10)));
    slow.join_all();
    D_HT_CHECK(slow.all_completed());

    return true;
}


NS_END  // testing
NS_END  // djinterp
