/*******************************************************************************
* djinterp [tests]                              test_deadlock_tests_watchdog.cpp
*
*   Section TU: deadlock_watchdog - configuration, the arm/disarm cycle, and
* the fire path.
*
*   Timing discipline: tests that must observe a FIRE arm a short deadline and
* then wait an order of magnitude longer; tests that must observe NO fire arm
* a deadline no test will reach and disarm at once.  Neither direction is a
* race.
*
*   set_fatal_on_fire(true) is never combined with an actual fire: that path
* calls std::abort() and would kill the test process.
*
*
* path:      /tests/djinterp/test/sync/test_deadlock_tests_watchdog.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                                created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/

// std
#include <atomic>
#include <chrono>
#include <string>
#include <thread>
// djinterp
#include "test_deadlock_tests.hpp"


NS_DJINTERP
NS_TESTING


bool tests_dwatchdog_default_construct()
{
    dt::deadlock_watchdog wd;

    D_HD_CHECK(!wd.armed());
    D_HD_CHECK(!wd.fired());
    D_HD_CHECK(!wd.stop_requested());
    D_HD_CHECK(wd.report().ok());
    D_HD_CHECK(wd.report().event_count() == 0u);
    // the default deadline is recorded up front
    D_HD_CHECK(wd.report().configured_timeout >
               std::chrono::nanoseconds(0));

    return true;
}

bool tests_dwatchdog_set_timeout_records()
{
    dt::deadlock_watchdog wd;

    wd.set_timeout(std::chrono::milliseconds(250));

    D_HD_CHECK(wd.report().configured_timeout ==
               std::chrono::nanoseconds(
                   std::chrono::milliseconds(250)));

    return true;
}

bool tests_dwatchdog_set_fatal_flag_accepted()
{
    // only the setter is exercised - a fatal fire would abort the process
    dt::deadlock_watchdog wd;

    wd.set_fatal_on_fire(true);
    wd.set_fatal_on_fire(false);

    D_HD_CHECK(!wd.armed());
    D_HD_CHECK(!wd.fired());

    return true;
}

bool tests_dwatchdog_arm_disarm_no_fire()
{
    dt::deadlock_watchdog wd;
    wd.set_timeout(hd_long_timeout());   // far beyond any test's life

    wd.arm();
    D_HD_CHECK(wd.armed());

    wd.disarm();

    D_HD_CHECK(!wd.armed());
    D_HD_CHECK(!wd.fired());
    D_HD_CHECK(!wd.stop_requested());
    D_HD_CHECK(wd.report().ok());
    D_HD_CHECK(wd.report().timeout_count == 0u);

    return true;
}

bool tests_dwatchdog_arm_twice()
{
    dt::deadlock_watchdog wd;
    wd.set_timeout(hd_long_timeout());

    wd.arm();
    D_HD_CHECK(wd.armed());

    wd.arm();               // already armed: must not spawn a second watcher
    D_HD_CHECK(wd.armed());

    wd.disarm();            // a single disarm still fully cleans up
    D_HD_CHECK(!wd.armed());
    D_HD_CHECK(!wd.fired());

    return true;
}

bool tests_dwatchdog_disarm_without_arm()
{
    dt::deadlock_watchdog wd;

    wd.disarm();            // never armed: safe no-op

    D_HD_CHECK(!wd.armed());
    D_HD_CHECK(!wd.fired());

    return true;
}

bool tests_dwatchdog_disarm_twice()
{
    dt::deadlock_watchdog wd;
    wd.set_timeout(hd_long_timeout());

    wd.arm();
    wd.disarm();
    wd.disarm();            // already disarmed: safe no-op

    D_HD_CHECK(!wd.armed());
    D_HD_CHECK(!wd.fired());

    return true;
}

bool tests_dwatchdog_fires_on_timeout()
{
    dt::deadlock_watchdog wd;
    wd.set_timeout(hd_fire_timeout());

    wd.arm();
    hd_await_fire();        // an order of magnitude past the deadline
    wd.disarm();

    D_HD_CHECK(wd.fired());
    D_HD_CHECK(!wd.report().ok());

    return true;
}

bool tests_dwatchdog_fire_records_event()
{
    dt::deadlock_watchdog wd;
    wd.set_timeout(hd_fire_timeout());

    wd.arm();
    hd_await_fire();
    wd.disarm();

    const dt::deadlock_report& rep = wd.report();

    // the watchdog is one-shot: exactly one timeout event
    D_HD_CHECK(rep.fired);
    D_HD_CHECK(rep.timeout_count == 1u);
    D_HD_CHECK(rep.cycle_count == 0u);
    D_HD_CHECK(rep.event_count() == 1u);
    D_HD_CHECK(rep.events[0].kind == dt::deadlock_event_kind::timeout);

    return true;
}

bool tests_dwatchdog_fire_sets_stop_requested()
{
    dt::deadlock_watchdog wd;
    wd.set_timeout(hd_fire_timeout());

    D_HD_CHECK(!wd.stop_requested());

    wd.arm();
    hd_await_fire();
    wd.disarm();

    // the flag the system under test polls to abandon its work
    D_HD_CHECK(wd.stop_requested());

    return true;
}

bool tests_dwatchdog_fire_invokes_callback()
{
    std::atomic<int> hits(0);

    dt::deadlock_watchdog wd;
    wd.set_timeout(hd_fire_timeout());
    wd.set_on_fire(
        dt::deadlock_watchdog::on_fire_callback(
            [&hits]()
            {
                hits.fetch_add(1, std::memory_order_acq_rel);
            }));

    wd.arm();
    hd_await_fire();
    wd.disarm();

    D_HD_CHECK(wd.fired());
    D_HD_CHECK(hits.load(std::memory_order_acquire) == 1);

    return true;
}

bool tests_dwatchdog_description_in_event()
{
    dt::deadlock_watchdog wd;
    wd.set_timeout(hd_fire_timeout());
    wd.set_description(std::string("queue drain stalled"));

    wd.arm();
    hd_await_fire();
    wd.disarm();

    const dt::deadlock_report& rep = wd.report();
    D_HD_CHECK(rep.event_count() == 1u);
    D_HD_CHECK(rep.events[0].description == "queue drain stalled");

    return true;
}

bool tests_dwatchdog_default_description()
{
    dt::deadlock_watchdog wd;
    wd.set_timeout(hd_fire_timeout());
    // no description set

    wd.arm();
    hd_await_fire();
    wd.disarm();

    const dt::deadlock_report& rep = wd.report();
    D_HD_CHECK(rep.event_count() == 1u);
    // a default description is substituted rather than left blank
    D_HD_CHECK(!rep.events[0].description.empty());

    return true;
}

bool tests_dwatchdog_reset_clears_report()
{
    dt::deadlock_watchdog wd;
    wd.set_timeout(hd_fire_timeout());

    wd.arm();
    hd_await_fire();
    wd.disarm();
    D_HD_CHECK(wd.fired());
    D_HD_CHECK(wd.stop_requested());

    wd.reset();

    D_HD_CHECK(!wd.fired());
    D_HD_CHECK(!wd.stop_requested());
    D_HD_CHECK(wd.report().ok());
    D_HD_CHECK(wd.report().event_count() == 0u);
    // the configured deadline survives the reset
    D_HD_CHECK(wd.report().configured_timeout ==
               std::chrono::nanoseconds(hd_fire_timeout()));

    return true;
}

bool tests_dwatchdog_reset_ignored_while_armed()
{
    dt::deadlock_watchdog wd;
    wd.set_timeout(hd_fire_timeout());

    wd.arm();
    hd_await_fire();
    // still armed (never disarmed), so the report must be left alone
    D_HD_CHECK(wd.armed());
    D_HD_CHECK(wd.fired());

    wd.reset();

    D_HD_CHECK(wd.fired());              // refused
    D_HD_CHECK(wd.report().timeout_count == 1u);

    wd.disarm();

    return true;
}

bool tests_dwatchdog_rearm_after_fire()
{
    dt::deadlock_watchdog wd;
    wd.set_timeout(hd_fire_timeout());

    wd.arm();
    hd_await_fire();
    wd.disarm();
    D_HD_CHECK(wd.fired());

    wd.reset();
    wd.set_timeout(hd_long_timeout());

    // the second cycle is disarmed promptly, so it must stay clean
    wd.arm();
    D_HD_CHECK(wd.armed());
    wd.disarm();

    D_HD_CHECK(!wd.fired());
    D_HD_CHECK(wd.report().ok());

    return true;
}

bool tests_dwatchdog_destructor_disarms()
{
    // an armed watchdog going out of scope must join its watcher rather than
    // terminating the process
    std::atomic<int> hits(0);

    {
        dt::deadlock_watchdog wd;
        wd.set_timeout(hd_long_timeout());
        wd.set_on_fire(
            dt::deadlock_watchdog::on_fire_callback(
                [&hits]()
                {
                    hits.fetch_add(1, std::memory_order_acq_rel);
                }));

        wd.arm();
        D_HD_CHECK(wd.armed());
    }   // dtor disarms and joins

    // the deadline was never reached, so the callback never ran
    D_HD_CHECK(hits.load(std::memory_order_acquire) == 0);

    return true;
}


NS_END  // testing
NS_END  // djinterp
