/*******************************************************************************
* djinterp [test]                            test_report_runner_tests_finish.cpp
*
*
* path:      /tests/djinterp/test/output/test_report_runner_tests_finish.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "test_report_runner_tests.hpp"

NS_DJINTERP
NS_TESTING


/*
tests_report_runner_finish_exit_code_pass
  a clean run's exit code.
  Tests the following:
  - a run whose every unit passed returns zero
  - the code agrees with the report's own verdict
  - it holds across several modules
*/
bool
tests_report_runner_finish_exit_code_pass()
{
    dt::report_builder rb;

    rb.set_console(false);
    rb.module("alpha");
    rr_seed_units(rb, 3, 0);
    rb.module("beta");
    rr_seed_units(rb, 2, 0);

    const int rc = rb.finish();

    D_RR_CHECK(rc == 0);
    D_RR_CHECK(rb.report().passed());
    D_RR_CHECK(rb.report().verdict() == dt::report_verdict::passed);
    D_RR_CHECK(rc == rb.report().exit_code());

    return true;
}

/*
tests_report_runner_finish_exit_code_fail
  a failing run's exit code.
  Tests the following:
  - one failing unit anywhere in the run returns non-zero
  - the code agrees with the report's own verdict
  - a failure in a later module is not masked by an earlier clean one
*/
bool
tests_report_runner_finish_exit_code_fail()
{
    dt::report_builder rb;

    rb.set_console(false);
    rb.module("clean");
    rr_seed_units(rb, 5, 0);
    rb.module("dirty");
    rr_seed_units(rb, 4, 1);

    const int rc = rb.finish();

    D_RR_CHECK(rc != 0);
    D_RR_CHECK(!rb.report().passed());
    D_RR_CHECK(rb.report().verdict() == dt::report_verdict::failed);
    D_RR_CHECK(rc == rb.report().exit_code());

    // the first module is still individually clean
    D_RR_CHECK(rb.report().modules[0].passed());
    D_RR_CHECK(!rb.report().modules[1].passed());

    return true;
}

/*
tests_report_runner_finish_empty_report
  finishing a run that recorded nothing.
  Tests the following:
  - finish() on an empty report is safe and prints its summaries
  - an empty run is NOT a pass, so the exit code is non-zero
  - the zero-total rates use the two-decimal spelling
*/
bool
tests_report_runner_finish_empty_report()
{
    rr_capture cap("dtest_rr_finish_empty.txt");

    D_RR_CHECK(cap.valid());

    dt::report_builder rb;

    rb.set_file(cap.file());

    const int rc = rb.finish();

    D_RR_CHECK(rc != 0);
    D_RR_CHECK(rb.report().verdict() == dt::report_verdict::empty);

    const std::string out = cap.text();

    // no module boxes were printed, but the roll-up still was
    D_RR_CHECK(!rr_contains(out, "MODULE RESULTS: "));
    D_RR_CHECK(rr_contains(out, "COMPREHENSIVE TEST RESULTS"));
    D_RR_CHECK(rr_contains(out, "  Modules Tested: 0"));
    D_RR_CHECK(rr_contains(out, "  Total Assertions: 0"));
    D_RR_CHECK(rr_contains(out, "  Module Success Rate: 0.00%"));

    // and it concludes as a failure
    D_RR_CHECK(rr_contains(out, "SOME TESTS FAILED"));
    D_RR_CHECK(rr_contains(out, "\npassed: 0   failed: 0\n"));

    return true;
}

/*
tests_report_runner_finish_final_tally
  the closing pass / fail line.
  Tests the following:
  - the tally counts UNITS, not assertions
  - it sums across every module of the run
  - it is the last thing printed
*/
bool
tests_report_runner_finish_final_tally()
{
    rr_capture cap("dtest_rr_tally.txt");

    D_RR_CHECK(cap.valid());

    dt::report_builder rb;

    rb.set_file(cap.file());
    rb.set_console(false);

    rb.module("alpha");
    rr_seed_units(rb, 3, 1);
    rb.module("beta");
    rr_seed_units(rb, 2, 2);

    // a rich unit contributes ONE unit but several assertions
    rb.open_unit("rich");
    rb.check("a", "1", "1", true);
    rb.check("b", "2", "2", true);
    rb.check("c", "3", "3", true);
    rb.close_unit();

    rb.set_console(true);
    rb.finish();

    const std::string out = cap.text();

    // 5 passing + 3 failing units, though there are 11 assertions
    D_RR_CHECK(rb.report().total_units() == 9u);
    D_RR_CHECK(rb.report().total_checks() == 11u);
    D_RR_CHECK(rr_contains(out, "\npassed: 6   failed: 3\n"));

    // the tally closes the output
    D_RR_CHECK(out.find("passed: 6   failed: 3") >
               out.find("OVERALL ASSESSMENT:"));

    return true;
}

/*
tests_report_runner_finish_is_repeatable
  calling finish more than once.
  Tests the following:
  - a second finish repeats the summaries rather than faulting
  - the model is not consumed or mutated by finishing
  - the exit code is the same both times
*/
bool
tests_report_runner_finish_is_repeatable()
{
    rr_capture cap("dtest_rr_refinish.txt");

    D_RR_CHECK(cap.valid());

    dt::report_builder rb;

    rb.set_file(cap.file());
    rb.set_console(false);
    rb.module("m");
    rr_seed_units(rb, 2, 1);
    rb.set_console(true);

    const int first = rb.finish();

    const std::size_t units_after_first = rb.report().total_units();

    const int second = rb.finish();

    D_RR_CHECK(first == second);
    D_RR_CHECK(first != 0);
    D_RR_CHECK(rb.report().total_units() == units_after_first);
    D_RR_CHECK(rb.report().total_units() == 3u);

    // the summaries really were printed twice
    const std::string out = cap.text();

    D_RR_CHECK(rr_count(out, "COMPREHENSIVE TEST RESULTS") == 2u);
    D_RR_CHECK(rr_count(out, "MODULE RESULTS: m") == 2u);
    D_RR_CHECK(rr_count(out, "OVERALL ASSESSMENT:") == 2u);

    return true;
}

/*
tests_report_runner_finish_multi_module_boxes
  one box per module.
  Tests the following:
  - finish prints exactly one results box per module
  - the boxes appear in the order the modules were opened
  - each box carries its own module's tallies
*/
bool
tests_report_runner_finish_multi_module_boxes()
{
    rr_capture cap("dtest_rr_boxes.txt");

    D_RR_CHECK(cap.valid());

    dt::report_builder rb;

    rb.set_file(cap.file());
    rb.set_console(false);

    rb.module("first");
    rr_seed_units(rb, 1, 0);
    rb.module("second");
    rr_seed_units(rb, 2, 0);
    rb.module("third");
    rr_seed_units(rb, 0, 1);

    rb.set_console(true);
    rb.finish();

    const std::string out = cap.text();

    D_RR_CHECK(rr_count(out, "MODULE RESULTS: ") == 3u);
    D_RR_CHECK(rr_contains(out, "MODULE RESULTS: first"));
    D_RR_CHECK(rr_contains(out, "MODULE RESULTS: second"));
    D_RR_CHECK(rr_contains(out, "MODULE RESULTS: third"));

    // in declaration order
    D_RR_CHECK(out.find("MODULE RESULTS: first") <
               out.find("MODULE RESULTS: second"));
    D_RR_CHECK(out.find("MODULE RESULTS: second") <
               out.find("MODULE RESULTS: third"));

    // each with its own tallies
    D_RR_CHECK(rr_contains(out, "Status: [PASS] first MODULE PASSED"));
    D_RR_CHECK(rr_contains(out, "Status: [PASS] second MODULE PASSED"));
    D_RR_CHECK(rr_contains(out, "Status: [FAIL] third MODULE FAILED"));

    return true;
}

/*
tests_report_runner_finish_empty_unit_is_forgiven
  a unit opened but never checked.
  Tests the following:
  - an empty unit carries the `empty` verdict and is not itself a pass
  - yet the module verdict ignores empty units, so a run of otherwise
    passing units still finishes clean and returns zero
  - the empty unit is still counted in the unit total
*/
bool
tests_report_runner_finish_empty_unit_is_forgiven()
{
    dt::report_builder rb;

    rb.set_console(false);
    rb.module("m");
    rr_seed_units(rb, 3, 0);

    // a unit opened and closed without a single assertion
    rb.open_unit("never_checked", "opened but never exercised");
    rb.close_unit();

    const dt::report_unit& u = rb.report().modules[0].units.back();

    D_RR_CHECK(u.name == "never_checked");
    D_RR_CHECK(u.checks.empty());
    D_RR_CHECK(u.verdict == dt::report_verdict::empty);

    // the unit is not itself a clean pass
    D_RR_CHECK(!u.passed());

    // but the module verdict ignores empty units, so the run passes
    D_RR_CHECK(rb.report().modules[0].passed());
    D_RR_CHECK(rb.report().passed());

    // the empty unit is still counted, yet does not join the passed tally
    D_RR_CHECK(rb.report().total_units() == 4u);
    D_RR_CHECK(rb.report().passed_units() == 3u);

    // and so finish() reports success
    D_RR_CHECK(rb.finish() == 0);

    return true;
}

NS_END  // testing
NS_END  // djinterp
