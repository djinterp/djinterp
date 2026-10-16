/*******************************************************************************
* djinterp [tests]                               test_invariant_tests_report.cpp
*
*   Section TU: invariant_violation and invariant_report - the plain
* aggregates the monitoring machinery fills in.  Single-threaded throughout.
*
*
* path:      /tests/djinterp/test/sync/test_invariant_tests_report.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                                created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/

// std
#include <string>
// djinterp
#include "test_invariant_tests.hpp"


NS_DJINTERP
NS_TESTING


bool tests_ireport_violation_defaults()
{
    dt::invariant_violation v;

    D_HI_CHECK(v.detail.empty());
    D_HI_CHECK(v.observed_at == dt::invariant_violation::time_point());

    return true;
}

bool tests_ireport_violation_value_construct()
{
    const dt::invariant_violation::time_point now =
        dt::test_thread::clock_type::now();

    dt::invariant_violation v(now, std::string("size went negative"));

    D_HI_CHECK(v.observed_at == now);
    D_HI_CHECK(v.detail == "size went negative");

    return true;
}

bool tests_ireport_defaults()
{
    dt::invariant_report rep;

    D_HI_CHECK(rep.checks_performed == 0u);
    D_HI_CHECK(rep.violation_count == 0u);
    D_HI_CHECK(rep.violations.empty());
    D_HI_CHECK(rep.success());          // nothing checked, nothing broken

    return true;
}

bool tests_ireport_success_predicate()
{
    dt::invariant_report rep;
    rep.checks_performed = 1000;
    D_HI_CHECK(rep.success());

    rep.violation_count = 1;            // a single violation is fatal
    D_HI_CHECK(!rep.success());

    return true;
}

bool tests_ireport_to_test_object_pass()
{
    dt::invariant_report rep;
    rep.checks_performed = 50;

    dt::basic_test t = rep.to_test_object(hi_type_id());

    D_HI_CHECK(t.result());
    D_HI_CHECK(t.type_id() == hi_type_id());
    D_HI_CHECK(t.metadata().get("name") == std::string("invariant"));
    D_HI_CHECK(t.metadata().contains("message"));

    return true;
}

bool tests_ireport_to_test_object_fail()
{
    dt::invariant_report pass;

    dt::invariant_report fail;
    fail.checks_performed = 10;
    fail.violation_count  = 2;

    dt::basic_test a = pass.to_test_object(hi_type_id());
    dt::basic_test b = fail.to_test_object(hi_type_id());

    D_HI_CHECK(!b.result());
    // the two verdicts carry distinct messages
    D_HI_CHECK(a.metadata().get("message") != b.metadata().get("message"));

    return true;
}

bool tests_ireport_to_test_object_custom_name()
{
    dt::invariant_report rep;

    dt::basic_test t = rep.to_test_object(hi_type_id(), "size_never_negative");

    D_HI_CHECK(t.metadata().get("name") ==
               std::string("size_never_negative"));

    return true;
}


NS_END  // testing
NS_END  // djinterp
