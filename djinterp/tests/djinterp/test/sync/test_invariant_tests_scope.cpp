/*******************************************************************************
* djinterp [tests]                                test_invariant_tests_scope.cpp
*
*   Section TU: invariant_scope - RAII monitoring around a block.  The ctor
* starts the monitor and the dtor stops it, so the report taken after the
* scope closes covers the whole guarded region.
*
*
* path:      /tests/djinterp/test/sync/test_invariant_tests_scope.cpp
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


bool tests_iscope_reports_on_exit()
{
    hi_flag flag;
    dt::invariant_report captured;

    {
        dt::invariant_scope scope(hi_predicate(flag));

        hi_settle();

        // the report is readable while the scope is open
        captured = scope.report();
    }

    D_HI_CHECK(captured.checks_performed > 0u);
    D_HI_CHECK(captured.violation_count == 0u);
    D_HI_CHECK(captured.success());

    return true;
}

bool tests_iscope_interval_construct()
{
    hi_flag flag;

    dt::invariant_scope scope(
        hi_predicate(flag),
        std::chrono::milliseconds(1));

    // the cadence-taking ctor starts the monitor too
    D_HI_CHECK(scope.monitor().running());

    hi_settle();

    D_HI_CHECK(scope.report().checks_performed > 0u);

    return true;
}

bool tests_iscope_monitor_access()
{
    hi_flag flag;
    dt::invariant_scope scope(hi_predicate(flag));

    // monitor() hands back the live, running monitor
    dt::invariant_monitor& mon = scope.monitor();

    D_HI_CHECK(mon.running());
    D_HI_CHECK(mon.success());

    return true;
}

bool tests_iscope_check_now_through_monitor()
{
    hi_flag flag;
    flag.ok.store(false, std::memory_order_release);

    dt::invariant_scope scope(hi_predicate(flag));

    // a synchronous check placed through the scope is recorded like any other
    D_HI_CHECK(!scope.monitor().check_now());
    D_HI_CHECK(scope.report().violation_count > 0u);

    return true;
}

bool tests_iscope_detects_violation()
{
    hi_flag flag;
    dt::invariant_report captured;

    {
        dt::invariant_scope scope(
            hi_predicate(flag),
            std::chrono::milliseconds(1));

        // break the invariant and leave it broken, so the polling monitor
        // cannot miss it before the scope closes
        flag.ok.store(false, std::memory_order_release);
        hi_settle();

        captured = scope.report();
    }

    D_HI_CHECK(captured.violation_count > 0u);
    D_HI_CHECK(!captured.violations.empty());
    D_HI_CHECK(!captured.success());

    return true;
}


NS_END  // testing
NS_END  // djinterp
