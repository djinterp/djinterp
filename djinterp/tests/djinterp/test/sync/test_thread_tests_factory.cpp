/*******************************************************************************
* djinterp [tests]                                 test_thread_tests_factory.cpp
*
*   Section TU: make_test_thread / make_named_test_thread factory helpers.
*
*
* path:      /tests/djinterp/test/sync/test_thread_tests_factory.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                                created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/

// std
#include <atomic>
#include <string>
// djinterp
#include "test_thread_tests.hpp"


NS_DJINTERP
NS_TESTING


bool tests_factory_make_test_thread()
{
    ht_worker_ran sink;

    dt::test_thread t =
        dt::make_test_thread(4, ht_make_stamp(sink));

    D_HT_CHECK(t.id() == 4u);
    D_HT_CHECK(t.name().empty());
    D_HT_CHECK(t.is_idle());

    return true;
}

bool tests_factory_make_named_test_thread()
{
    ht_worker_ran sink;

    dt::test_thread t =
        dt::make_named_test_thread(8, std::string("named"), ht_make_stamp(sink));

    D_HT_CHECK(t.id() == 8u);
    D_HT_CHECK(t.name() == "named");
    D_HT_CHECK(t.is_idle());

    return true;
}

bool tests_factory_make_and_run()
{
    ht_worker_ran sink;

    dt::test_thread t =
        dt::make_test_thread(1, ht_make_stamp(sink));

    t.start();
    t.join();

    D_HT_CHECK(t.completed());
    D_HT_CHECK(sink.ran.load(std::memory_order_acquire));
    D_HT_CHECK(sink.seen_id.load(std::memory_order_acquire) == 1u);

    return true;
}

bool tests_factory_worker_forwarding()
{
    // Verifies the callable is forwarded intact: a plain lambda capturing a
    // local counter is observed to have run after start/join.
    std::atomic<int> hits(0);

    dt::test_thread t =
        dt::make_test_thread(
            0,
            [&hits](std::size_t)
            {
                hits.fetch_add(1, std::memory_order_acq_rel);
            });

    t.start();
    t.join();

    D_HT_CHECK(t.completed());
    D_HT_CHECK(hits.load(std::memory_order_acquire) == 1);

    return true;
}


NS_END  // testing
NS_END  // djinterp
