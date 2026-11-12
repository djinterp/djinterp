/*******************************************************************************
* djinterp [cpp][test]                              test_stress_tests_report.cpp
*
*   Section TU: DStressReport - defaults, verdict delegation to the underlying
* concurrent run report, throughput, and the to_test_object lowering.  Plain
* aggregate behaviour, so the whole section is single-threaded.
*
*
* path:      /tests/djinterp/cpp/modules/test/sync/test_stress_tests_report.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                                created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/

// std
#include <chrono>
#include <string>
// djinterp
#include "test_stress_tests.hpp"


NS_DJINTERP
NS_TESTING


bool tests_sreport_defaults()
{
    dt::DStressReport rep;

    D_HS_CHECK(rep.total_ops == 0u);
    D_HS_CHECK(rep.ops_per_thread.empty());
    D_HS_CHECK(rep.ops_per_kind.empty());
    D_HS_CHECK(rep.op_names.empty());
    D_HS_CHECK(rep.wall_clock == dt::DStressReport::duration_type::zero());
    // the underlying report defaults to not_started, so the run is not a pass
    D_HS_CHECK(!rep.success());

    return true;
}

bool tests_sreport_success_follows_underlying()
{
    dt::DStressReport rep;

    rep.underlying.outcome = dt::DConcurrentRunOutcome::success;
    D_HS_CHECK(rep.success());

    rep.underlying.outcome = dt::DConcurrentRunOutcome::thread_threw;
    D_HS_CHECK(!rep.success());

    rep.underlying.outcome = dt::DConcurrentRunOutcome::timed_out;
    D_HS_CHECK(!rep.success());

    return true;
}

bool tests_sreport_ops_per_second_zero_wall()
{
    dt::DStressReport rep;
    rep.total_ops  = 1000;
    rep.wall_clock = dt::DStressReport::duration_type::zero();

    // no elapsed time: throughput is defined as zero rather than infinite
    D_HS_CHECK(rep.ops_per_second() == 0.0);

    return true;
}

bool tests_sreport_ops_per_second_computed()
{
    dt::DStressReport rep;
    rep.total_ops = 500;
    rep.wall_clock =
        std::chrono::duration_cast<dt::DStressReport::duration_type>(
            std::chrono::seconds(1));

    const double rate = rep.ops_per_second();

    // one second of wall clock over 500 ops
    D_HS_CHECK(rate > 400.0);
    D_HS_CHECK(rate < 600.0);

    return true;
}

bool tests_sreport_to_test_object_success()
{
    dt::DStressReport rep;
    rep.underlying.outcome = dt::DConcurrentRunOutcome::success;
    rep.total_ops          = 10;

    dt::basic_test t = rep.to_test_object(hs_type_id());

    D_HS_CHECK(t.result());
    D_HS_CHECK(t.metadata().contains("name"));
    D_HS_CHECK(t.metadata().get("name") == std::string("stress_run"));
    D_HS_CHECK(t.type_id() == hs_type_id());

    return true;
}

bool tests_sreport_to_test_object_failure()
{
    dt::DStressReport rep;
    rep.underlying.outcome = dt::DConcurrentRunOutcome::thread_threw;

    dt::basic_test t = rep.to_test_object(hs_type_id());

    D_HS_CHECK(!t.result());

    return true;
}

bool tests_sreport_to_test_object_custom_name()
{
    dt::DStressReport rep;
    rep.underlying.outcome = dt::DConcurrentRunOutcome::success;

    dt::basic_test t = rep.to_test_object(hs_type_id(), "hammer");

    D_HS_CHECK(t.metadata().get("name") == std::string("hammer"));

    return true;
}


NS_END  // testing
NS_END  // djinterp
