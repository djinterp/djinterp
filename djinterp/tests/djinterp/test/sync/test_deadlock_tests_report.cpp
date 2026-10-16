/*******************************************************************************
* djinterp [tests]                                test_deadlock_tests_report.cpp
*
*   Section TU: deadlock_event_kind, deadlock_event, and deadlock_report -
* the plain aggregates the detection machinery fills in.  Single-threaded
* throughout.
*
*
* path:      /tests/djinterp/test/sync/test_deadlock_tests_report.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                                created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/

// std
#include <chrono>
#include <string>
// djinterp
#include "test_deadlock_tests.hpp"


NS_DJINTERP
NS_TESTING


bool tests_dreport_event_kind_values()
{
    D_HD_CHECK(static_cast<int>(dt::deadlock_event_kind::timeout)       == 0);
    D_HD_CHECK(static_cast<int>(dt::deadlock_event_kind::cycle)         == 1);
    D_HD_CHECK(static_cast<int>(dt::deadlock_event_kind::abandoned)     == 2);
    D_HD_CHECK(static_cast<int>(dt::deadlock_event_kind::user_reported) == 3);

    return true;
}

bool tests_dreport_event_defaults()
{
    dt::deadlock_event ev;

    D_HD_CHECK(ev.kind == dt::deadlock_event_kind::user_reported);
    D_HD_CHECK(ev.description.empty());
    D_HD_CHECK(ev.thread_id == 0u);
    D_HD_CHECK(ev.lock_id_a == 0u);
    D_HD_CHECK(ev.lock_id_b == 0u);
    // the stamp is taken at construction, not left at the epoch
    D_HD_CHECK(ev.when > std::chrono::steady_clock::time_point());

    return true;
}

bool tests_dreport_event_fields()
{
    dt::deadlock_event ev;

    ev.kind        = dt::deadlock_event_kind::cycle;
    ev.description = "AB-BA";
    ev.thread_id   = 3;
    ev.lock_id_a   = 10;
    ev.lock_id_b   = 20;

    D_HD_CHECK(ev.kind == dt::deadlock_event_kind::cycle);
    D_HD_CHECK(ev.description == "AB-BA");
    D_HD_CHECK(ev.thread_id == 3u);
    D_HD_CHECK(ev.lock_id_a == 10u);
    D_HD_CHECK(ev.lock_id_b == 20u);

    return true;
}

bool tests_dreport_defaults()
{
    dt::deadlock_report rep;

    D_HD_CHECK(!rep.fired);
    D_HD_CHECK(rep.timeout_count == 0u);
    D_HD_CHECK(rep.cycle_count == 0u);
    D_HD_CHECK(rep.events.empty());
    D_HD_CHECK(rep.configured_timeout == std::chrono::nanoseconds(0));
    D_HD_CHECK(rep.ok());
    D_HD_CHECK(rep.event_count() == 0u);

    return true;
}

bool tests_dreport_ok_predicate()
{
    dt::deadlock_report rep;
    D_HD_CHECK(rep.ok());

    rep.fired = true;
    D_HD_CHECK(!rep.ok());

    return true;
}

bool tests_dreport_ok_requires_all_clear()
{
    // each of the three conditions independently fails ok()
    dt::deadlock_report a;
    a.fired = true;
    D_HD_CHECK(!a.ok());

    dt::deadlock_report b;
    b.timeout_count = 1;
    D_HD_CHECK(!b.ok());

    dt::deadlock_report c;
    c.cycle_count = 1;
    D_HD_CHECK(!c.ok());

    return true;
}

bool tests_dreport_event_count()
{
    dt::deadlock_report rep;
    D_HD_CHECK(rep.event_count() == 0u);

    rep.events.push_back(dt::deadlock_event());
    D_HD_CHECK(rep.event_count() == 1u);

    rep.events.push_back(dt::deadlock_event());
    rep.events.push_back(dt::deadlock_event());
    D_HD_CHECK(rep.event_count() == 3u);
    D_HD_CHECK(rep.event_count() == rep.events.size());

    return true;
}

bool tests_dreport_to_test_object_pass()
{
    dt::deadlock_report rep;

    dt::basic_test t = rep.to_test_object(hd_type_id());

    D_HD_CHECK(t.result());
    D_HD_CHECK(t.type_id() == hd_type_id());
    D_HD_CHECK(t.metadata().get("name") == std::string("deadlock_test"));

    return true;
}

bool tests_dreport_to_test_object_fail()
{
    dt::deadlock_report rep;
    rep.fired         = true;
    rep.timeout_count = 1;

    dt::basic_test t = rep.to_test_object(hd_type_id());

    D_HD_CHECK(!t.result());

    return true;
}

bool tests_dreport_to_test_object_custom_name()
{
    dt::deadlock_report rep;

    dt::basic_test t = rep.to_test_object(hd_type_id(), "queue_lock_order");

    D_HD_CHECK(t.metadata().get("name") == std::string("queue_lock_order"));
    D_HD_CHECK(t.type_id() == hd_type_id());

    // the kind argument defaults to zero when omitted
    dt::basic_test d = rep.to_test_object();
    D_HD_CHECK(d.type_id() == static_cast<dt::test_type_id>(0));

    return true;
}


NS_END  // testing
NS_END  // djinterp
