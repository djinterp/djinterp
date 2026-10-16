/*******************************************************************************
* djinterp [tests]                                test_deadlock_tests_scoped.cpp
*
*   Section TU: scoped_deadlock_watchdog - RAII arming of a borrowed watchdog
* around a block.  The ctor sets the timeout and arms; the dtor disarms.
*
*
* path:      /tests/djinterp/test/sync/test_deadlock_tests_scoped.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                                created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/

// std
#include <chrono>
#include <thread>
// djinterp
#include "test_deadlock_tests.hpp"


NS_DJINTERP
NS_TESTING


bool tests_dscoped_arms_and_disarms()
{
    dt::deadlock_watchdog wd;

    D_HD_CHECK(!wd.armed());

    {
        dt::scoped_deadlock_watchdog scope(wd, hd_long_timeout());

        D_HD_CHECK(wd.armed());
    }   // dtor disarms

    D_HD_CHECK(!wd.armed());

    return true;
}

bool tests_dscoped_no_fire_when_prompt()
{
    dt::deadlock_watchdog wd;

    {
        dt::scoped_deadlock_watchdog scope(wd, hd_long_timeout());
        // the guarded region finishes immediately
    }

    D_HD_CHECK(!wd.fired());
    D_HD_CHECK(!wd.stop_requested());
    D_HD_CHECK(wd.report().ok());
    D_HD_CHECK(wd.report().timeout_count == 0u);

    return true;
}

bool tests_dscoped_sets_timeout()
{
    dt::deadlock_watchdog wd;

    {
        dt::scoped_deadlock_watchdog scope(
            wd, std::chrono::milliseconds(500));

        // the scope pushed its deadline into the watchdog
        D_HD_CHECK(wd.report().configured_timeout ==
                   std::chrono::nanoseconds(
                       std::chrono::milliseconds(500)));
    }

    D_HD_CHECK(!wd.fired());

    return true;
}

bool tests_dscoped_fire_inside_scope()
{
    dt::deadlock_watchdog wd;

    {
        dt::scoped_deadlock_watchdog scope(wd, hd_fire_timeout());

        // the guarded region deliberately overruns its deadline
        hd_await_fire();
    }   // dtor disarms, joining the already-fired watcher

    D_HD_CHECK(wd.fired());
    D_HD_CHECK(wd.stop_requested());
    D_HD_CHECK(!wd.armed());
    D_HD_CHECK(wd.report().timeout_count == 1u);

    return true;
}


NS_END  // testing
NS_END  // djinterp
