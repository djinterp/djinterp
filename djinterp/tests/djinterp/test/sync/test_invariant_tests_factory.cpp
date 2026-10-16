/*******************************************************************************
* djinterp [tests]                              test_invariant_tests_factory.cpp
*
*   Section TU: make_invariant_monitor.
*
*   Unlike the factories in test_concurrent / test_stress / test_race, this
* one returns a PRVALUE (`return invariant_monitor(...)`) rather than a named
* local, so guaranteed copy elision applies and no move constructor is needed.
* The block is therefore NOT gated.
*
*
* path:      /tests/djinterp/test/sync/test_invariant_tests_factory.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                                created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/

// std
#include <atomic>
#include <chrono>
// djinterp
#include "test_invariant_tests.hpp"


NS_DJINTERP
NS_TESTING


bool tests_ifactory_make_invariant_monitor()
{
    hi_flag flag;

    dt::invariant_monitor mon =
        dt::make_invariant_monitor(hi_predicate(flag));

    // built but not started, and nothing checked yet
    D_HI_CHECK(!mon.running());
    D_HI_CHECK(mon.report().checks_performed == 0u);
    D_HI_CHECK(mon.success());

    return true;
}

bool tests_ifactory_make_and_check()
{
    hi_flag flag;

    dt::invariant_monitor mon =
        dt::make_invariant_monitor(hi_predicate(flag));

    // the predicate really was forwarded into the monitor
    D_HI_CHECK(mon.check_now());
    D_HI_CHECK(mon.report().checks_performed == 1u);

    flag.ok.store(false, std::memory_order_release);
    D_HI_CHECK(!mon.check_now());
    D_HI_CHECK(mon.violation_count() == 1u);

    return true;
}

bool tests_ifactory_make_and_run_background()
{
    hi_flag flag;

    dt::invariant_monitor mon =
        dt::make_invariant_monitor(hi_predicate(flag));
    mon.set_poll_interval(std::chrono::milliseconds(1));

    mon.start();
    D_HI_CHECK(mon.running());

    hi_settle();

    mon.stop();
    D_HI_CHECK(!mon.running());

    D_HI_CHECK(mon.report().checks_performed > 0u);
    D_HI_CHECK(mon.success());

    return true;
}


NS_END  // testing
NS_END  // djinterp
