/*******************************************************************************
* djinterp [tests]                              test_invariant_tests_monitor.cpp
*
*   Section TU: invariant_monitor - synchronous checking, violation recording,
* and the background polling lifecycle.
*
*   check_now() is synchronous and therefore exact, so the recording contract
* is pinned there.  Background polling runs on a cadence, so the background
* tests assert only "at least one check happened" / "the violation was seen",
* never an exact poll count.
*
*
* path:      /tests/djinterp/test/sync/test_invariant_tests_monitor.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                                created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/

// std
#include <atomic>
#include <chrono>
#include <stdexcept>
#include <string>
#include <thread>
// djinterp
#include "test_invariant_tests.hpp"


NS_DJINTERP
NS_TESTING


bool tests_imonitor_default_construct()
{
    dt::invariant_monitor mon;

    D_HI_CHECK(!mon.running());
    D_HI_CHECK(mon.violation_count() == 0u);
    D_HI_CHECK(mon.success());

    const dt::invariant_report rep = mon.report();
    D_HI_CHECK(rep.checks_performed == 0u);
    D_HI_CHECK(rep.violations.empty());

    return true;
}

bool tests_imonitor_predicate_construct()
{
    hi_flag flag;
    dt::invariant_monitor mon(hi_predicate(flag));

    D_HI_CHECK(!mon.running());       // constructing does not start it

    // but the predicate is already bound and usable
    D_HI_CHECK(mon.check_now());
    D_HI_CHECK(mon.report().checks_performed == 1u);

    return true;
}

bool tests_imonitor_check_now_without_predicate()
{
    dt::invariant_monitor mon;   // no predicate

    // with nothing to check, the monitor reports success and records nothing
    D_HI_CHECK(mon.check_now());
    D_HI_CHECK(mon.report().checks_performed == 0u);
    D_HI_CHECK(mon.violation_count() == 0u);

    return true;
}

bool tests_imonitor_check_now_true()
{
    hi_flag flag;                    // starts holding
    dt::invariant_monitor mon;
    mon.set_predicate(hi_predicate(flag));

    D_HI_CHECK(mon.check_now());

    const dt::invariant_report rep = mon.report();
    D_HI_CHECK(rep.checks_performed == 1u);
    D_HI_CHECK(rep.violation_count == 0u);
    D_HI_CHECK(rep.violations.empty());
    D_HI_CHECK(mon.success());

    return true;
}

bool tests_imonitor_check_now_false()
{
    hi_flag flag;
    flag.ok.store(false, std::memory_order_release);

    dt::invariant_monitor mon;
    mon.set_predicate(hi_predicate(flag));

    D_HI_CHECK(!mon.check_now());

    const dt::invariant_report rep = mon.report();
    D_HI_CHECK(rep.checks_performed == 1u);
    D_HI_CHECK(rep.violation_count == 1u);
    D_HI_CHECK(rep.violations.size() == 1u);
    D_HI_CHECK(!mon.success());

    return true;
}

bool tests_imonitor_check_now_accumulates()
{
    hi_flag flag;
    dt::invariant_monitor mon;
    mon.set_predicate(hi_predicate(flag));

    mon.check_now();                                     // ok
    mon.check_now();                                     // ok

    flag.ok.store(false, std::memory_order_release);
    mon.check_now();                                     // violation
    mon.check_now();                                     // violation

    flag.ok.store(true, std::memory_order_release);
    mon.check_now();                                     // ok again

    const dt::invariant_report rep = mon.report();
    D_HI_CHECK(rep.checks_performed == 5u);
    D_HI_CHECK(rep.violation_count == 2u);
    D_HI_CHECK(rep.violations.size() == 2u);

    return true;
}

bool tests_imonitor_check_now_throwing_predicate()
{
    dt::invariant_monitor mon;
    mon.set_predicate(
        dt::invariant_monitor::predicate_fn(
            []() -> bool
            {
                throw std::runtime_error("predicate threw");
            }));

    // the throw is swallowed and treated as a failed invariant
    D_HI_CHECK(!mon.check_now());

    const dt::invariant_report rep = mon.report();
    D_HI_CHECK(rep.checks_performed == 1u);
    D_HI_CHECK(rep.violation_count == 1u);

    return true;
}

bool tests_imonitor_detail_provider()
{
    hi_flag flag;
    flag.ok.store(false, std::memory_order_release);

    dt::invariant_monitor mon;
    mon.set_predicate(hi_predicate(flag));
    mon.set_detail_provider(
        dt::invariant_monitor::detail_fn(
            []()
            {
                return std::string("count=-1");
            }));

    mon.check_now();

    const dt::invariant_report rep = mon.report();
    D_HI_CHECK(rep.violations.size() == 1u);
    D_HI_CHECK(rep.violations[0].detail == "count=-1");

    return true;
}

bool tests_imonitor_detail_provider_throws()
{
    hi_flag flag;
    flag.ok.store(false, std::memory_order_release);

    dt::invariant_monitor mon;
    mon.set_predicate(hi_predicate(flag));
    mon.set_detail_provider(
        dt::invariant_monitor::detail_fn(
            []() -> std::string
            {
                throw std::runtime_error("detail threw");
            }));

    mon.check_now();

    // the violation is still recorded; only the detail string is lost
    const dt::invariant_report rep = mon.report();
    D_HI_CHECK(rep.violation_count == 1u);
    D_HI_CHECK(rep.violations.size() == 1u);
    D_HI_CHECK(rep.violations[0].detail.empty());

    return true;
}

bool tests_imonitor_max_violations_recorded()
{
    hi_flag flag;
    flag.ok.store(false, std::memory_order_release);

    dt::invariant_monitor mon;
    mon.set_predicate(hi_predicate(flag));
    mon.set_max_violations_recorded(3);

    for (int i = 0; i < 10; ++i)
    {
        mon.check_now();
    }

    const dt::invariant_report rep = mon.report();

    // the cap bounds STORAGE, not the tally
    D_HI_CHECK(rep.checks_performed == 10u);
    D_HI_CHECK(rep.violation_count == 10u);
    D_HI_CHECK(rep.violations.size() == 3u);

    return true;
}

bool tests_imonitor_start_stop_running()
{
    hi_flag flag;
    dt::invariant_monitor mon;
    mon.set_predicate(hi_predicate(flag));

    D_HI_CHECK(!mon.running());

    mon.start();
    D_HI_CHECK(mon.running());

    mon.stop();
    D_HI_CHECK(!mon.running());

    return true;
}

bool tests_imonitor_start_without_predicate()
{
    dt::invariant_monitor mon;   // no predicate

    mon.start();

    D_HI_CHECK(!mon.running());   // nothing to poll: start is a no-op

    return true;
}

bool tests_imonitor_start_twice()
{
    hi_flag flag;
    dt::invariant_monitor mon;
    mon.set_predicate(hi_predicate(flag));

    mon.start();
    D_HI_CHECK(mon.running());

    mon.start();                  // already running: no second thread
    D_HI_CHECK(mon.running());

    mon.stop();
    D_HI_CHECK(!mon.running());

    return true;
}

bool tests_imonitor_stop_idempotent()
{
    hi_flag flag;
    dt::invariant_monitor mon;
    mon.set_predicate(hi_predicate(flag));

    mon.stop();                   // never started: safe no-op
    D_HI_CHECK(!mon.running());

    mon.start();
    mon.stop();
    mon.stop();                   // already stopped: safe no-op
    D_HI_CHECK(!mon.running());

    return true;
}

bool tests_imonitor_start_resets_report()
{
    hi_flag flag;
    flag.ok.store(false, std::memory_order_release);

    dt::invariant_monitor mon;
    mon.set_predicate(hi_predicate(flag));

    mon.check_now();
    mon.check_now();
    D_HI_CHECK(mon.report().violation_count == 2u);

    // the predicate holds again before the monitored run begins
    flag.ok.store(true, std::memory_order_release);

    mon.start();
    mon.stop();

    // start() clears whatever was accumulated beforehand
    D_HI_CHECK(mon.report().violation_count == 0u);
    D_HI_CHECK(mon.success());

    return true;
}

bool tests_imonitor_background_polls()
{
    hi_flag flag;
    dt::invariant_monitor mon;
    mon.set_predicate(hi_predicate(flag));
    mon.set_poll_interval(std::chrono::milliseconds(1));

    mon.start();
    hi_settle();
    mon.stop();

    // the cadence is not exact, so only "it polled at all" is asserted
    const dt::invariant_report rep = mon.report();
    D_HI_CHECK(rep.checks_performed > 0u);
    D_HI_CHECK(rep.violation_count == 0u);
    D_HI_CHECK(mon.success());

    return true;
}

bool tests_imonitor_background_detects_violation()
{
    hi_flag flag;
    dt::invariant_monitor mon;
    mon.set_predicate(hi_predicate(flag));
    mon.set_poll_interval(std::chrono::milliseconds(1));

    mon.start();

    // break the invariant and leave it broken for the rest of the window, so
    // the monitor cannot miss it
    flag.ok.store(false, std::memory_order_release);
    hi_settle();

    mon.stop();

    const dt::invariant_report rep = mon.report();
    D_HI_CHECK(rep.violation_count > 0u);
    D_HI_CHECK(!rep.violations.empty());
    D_HI_CHECK(!mon.success());

    return true;
}

bool tests_imonitor_success_and_violation_count()
{
    hi_flag flag;
    dt::invariant_monitor mon;
    mon.set_predicate(hi_predicate(flag));

    D_HI_CHECK(mon.success());
    D_HI_CHECK(mon.violation_count() == mon.report().violation_count);

    flag.ok.store(false, std::memory_order_release);
    mon.check_now();

    D_HI_CHECK(!mon.success());
    D_HI_CHECK(mon.violation_count() == 1u);
    D_HI_CHECK(mon.violation_count() == mon.report().violation_count);

    return true;
}


NS_END  // testing
NS_END  // djinterp
