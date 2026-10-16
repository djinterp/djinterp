/*******************************************************************************
* djinterp [tests]                                      test_sync_tests_gate.cpp
*
*   Section TU: test_gate - manual reset gate.  State and pass accounting are
* asserted single-threaded; the blocking release is asserted with a real
* waiter.
*
*
* path:      /tests/djinterp/test/sync/test_sync_tests_gate.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                                created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/

// std
#include <atomic>
#include <chrono>
#include <thread>
// djinterp
#include "test_sync_tests.hpp"


NS_DJINTERP
NS_TESTING


bool tests_gate_starts_closed()
{
    dt::test_gate gate;

    D_HY_CHECK(!gate.is_open());
    D_HY_CHECK(gate.pass_count() == 0u);

    return true;
}

bool tests_gate_open_close()
{
    dt::test_gate gate;

    gate.open();
    D_HY_CHECK(gate.is_open());

    gate.close();
    D_HY_CHECK(!gate.is_open());

    gate.open();
    D_HY_CHECK(gate.is_open());

    return true;
}

bool tests_gate_wait_when_open()
{
    dt::test_gate gate;
    gate.open();

    gate.wait();   // open: returns without blocking

    D_HY_CHECK(gate.pass_count() == 1u);

    return true;
}

bool tests_gate_wait_for_timeout()
{
    dt::test_gate gate;   // closed

    const bool opened = gate.wait_for(hy_short());

    D_HY_CHECK(!opened);
    D_HY_CHECK(!gate.is_open());

    return true;
}

bool tests_gate_wait_for_success()
{
    dt::test_gate gate;
    gate.open();

    const bool opened = gate.wait_for(hy_short());

    D_HY_CHECK(opened);
    D_HY_CHECK(gate.pass_count() == 1u);

    return true;
}

bool tests_gate_pass_count_accumulates()
{
    dt::test_gate gate;
    gate.open();

    gate.wait();
    gate.wait();
    D_HY_CHECK(gate.pass_count() == 2u);

    D_HY_CHECK(gate.wait_for(hy_short()));
    D_HY_CHECK(gate.pass_count() == 3u);   // a timed pass counts too

    return true;
}

bool tests_gate_wait_for_no_increment_on_timeout()
{
    dt::test_gate gate;
    gate.open();
    gate.wait();
    D_HY_CHECK(gate.pass_count() == 1u);

    gate.close();
    D_HY_CHECK(!gate.wait_for(hy_short()));

    // a timed-out wait is not a passage
    D_HY_CHECK(gate.pass_count() == 1u);

    return true;
}

bool tests_gate_reclose()
{
    dt::test_gate gate;

    gate.open();
    D_HY_CHECK(gate.wait_for(hy_short()));

    gate.close();
    D_HY_CHECK(!gate.wait_for(hy_short()));   // blocks again

    gate.open();
    D_HY_CHECK(gate.wait_for(hy_short()));    // and re-opens

    return true;
}

bool tests_gate_blocks_until_open_mt()
{
    dt::test_gate gate;   // closed
    std::atomic<bool> passed(false);

    std::thread waiter(
        [&gate, &passed]()
        {
            gate.wait();
            passed.store(true, std::memory_order_release);
        });

    std::this_thread::sleep_for(hy_short());
    D_HY_CHECK(!passed.load(std::memory_order_acquire));

    gate.open();
    waiter.join();

    D_HY_CHECK(passed.load(std::memory_order_acquire));
    D_HY_CHECK(gate.pass_count() == 1u);

    return true;
}


NS_END  // testing
NS_END  // djinterp
