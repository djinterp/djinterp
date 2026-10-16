/*******************************************************************************
* djinterp [tests]                              test_concurrent_tests_report.cpp
*
*   Section TU: thread_report, concurrent_run_report, the concurrent_run_outcome
* enum, and the to_test_object lowering.  These are plain aggregates, so the
* whole section is single-threaded.
*
*
* path:      /tests/djinterp/test/sync/test_concurrent_tests_report.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                                created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/

// std
#include <string>
// djinterp
#include "test_concurrent_tests.hpp"


NS_DJINTERP
NS_TESTING


bool tests_creport_outcome_enum_values()
{
    D_HC_CHECK(static_cast<int>(dt::concurrent_run_outcome::success)      == 0);
    D_HC_CHECK(static_cast<int>(dt::concurrent_run_outcome::thread_threw) == 1);
    D_HC_CHECK(static_cast<int>(dt::concurrent_run_outcome::timed_out)    == 2);
    D_HC_CHECK(static_cast<int>(dt::concurrent_run_outcome::not_started)  == 3);
    D_HC_CHECK(static_cast<int>(dt::concurrent_run_outcome::unknown)      == 4);

    return true;
}

bool tests_creport_thread_report_defaults()
{
    dt::thread_report tr;

    D_HC_CHECK(tr.id == 0u);
    D_HC_CHECK(tr.name.empty());
    D_HC_CHECK(tr.final_state == dt::test_thread_state::idle);
    D_HC_CHECK(!tr.failed);
    D_HC_CHECK(tr.exception_message.empty());
    D_HC_CHECK(tr.elapsed == dt::thread_report::duration_type::zero());

    return true;
}

bool tests_creport_thread_report_fields()
{
    dt::thread_report tr;

    tr.id                = 5;
    tr.name              = "worker";
    tr.final_state       = dt::test_thread_state::failed;
    tr.failed            = true;
    tr.exception_message = "boom";

    D_HC_CHECK(tr.id == 5u);
    D_HC_CHECK(tr.name == "worker");
    D_HC_CHECK(tr.final_state == dt::test_thread_state::failed);
    D_HC_CHECK(tr.failed);
    D_HC_CHECK(tr.exception_message == "boom");

    return true;
}

bool tests_creport_run_report_defaults()
{
    dt::concurrent_run_report rep;

    D_HC_CHECK(rep.outcome == dt::concurrent_run_outcome::not_started);
    D_HC_CHECK(rep.thread_count == 0u);
    D_HC_CHECK(rep.completed_count == 0u);
    D_HC_CHECK(rep.failed_count == 0u);
    D_HC_CHECK(rep.surviving_count == 0u);
    D_HC_CHECK(rep.threads.empty());
    D_HC_CHECK(rep.first_exception == nullptr);
    D_HC_CHECK(!rep.success());

    return true;
}

bool tests_creport_success_predicate()
{
    dt::concurrent_run_report rep;

    rep.outcome = dt::concurrent_run_outcome::success;
    D_HC_CHECK(rep.success());

    rep.outcome = dt::concurrent_run_outcome::thread_threw;
    D_HC_CHECK(!rep.success());

    rep.outcome = dt::concurrent_run_outcome::timed_out;
    D_HC_CHECK(!rep.success());

    rep.outcome = dt::concurrent_run_outcome::not_started;
    D_HC_CHECK(!rep.success());

    rep.outcome = dt::concurrent_run_outcome::unknown;
    D_HC_CHECK(!rep.success());

    return true;
}

bool tests_creport_to_test_object_success()
{
    dt::concurrent_run_report rep;
    rep.outcome = dt::concurrent_run_outcome::success;

    dt::basic_test t = rep.to_test_object(hc_type_id());

    D_HC_CHECK(t.result());
    D_HC_CHECK(t.metadata().contains("name"));
    D_HC_CHECK(t.metadata().contains("message"));
    D_HC_CHECK(t.metadata().get("name") == std::string("concurrent_run"));

    return true;
}

bool tests_creport_to_test_object_thread_threw()
{
    dt::concurrent_run_report rep;
    rep.outcome = dt::concurrent_run_outcome::thread_threw;

    dt::basic_test t = rep.to_test_object(hc_type_id());

    D_HC_CHECK(!t.result());
    // the message distinguishes a throw from the other failure modes
    D_HC_CHECK(t.metadata().get("message") !=
               t.metadata().get("nonexistent-key"));

    return true;
}

bool tests_creport_to_test_object_timed_out()
{
    dt::concurrent_run_report threw;
    threw.outcome = dt::concurrent_run_outcome::thread_threw;

    dt::concurrent_run_report timed;
    timed.outcome = dt::concurrent_run_outcome::timed_out;

    dt::basic_test a = threw.to_test_object(hc_type_id());
    dt::basic_test b = timed.to_test_object(hc_type_id());

    D_HC_CHECK(!b.result());
    // each failure mode carries its own message
    D_HC_CHECK(a.metadata().get("message") != b.metadata().get("message"));

    return true;
}

bool tests_creport_to_test_object_not_started()
{
    dt::concurrent_run_report rep;   // defaults to not_started

    dt::basic_test t = rep.to_test_object(hc_type_id());

    D_HC_CHECK(!t.result());
    D_HC_CHECK(t.metadata().contains("message"));

    return true;
}

bool tests_creport_to_test_object_custom_name()
{
    dt::concurrent_run_report rep;
    rep.outcome = dt::concurrent_run_outcome::success;

    dt::basic_test t = rep.to_test_object(hc_type_id(), "my_run");

    D_HC_CHECK(t.metadata().get("name") == std::string("my_run"));
    D_HC_CHECK(t.type_id() == hc_type_id());

    return true;
}


NS_END  // testing
NS_END  // djinterp
