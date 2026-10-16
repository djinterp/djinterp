/*******************************************************************************
* djinterp [test]                           test_report_runner_tests_console.cpp
*
*
* path:      /tests/djinterp/test/output/test_report_runner_tests_console.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "test_report_runner_tests.hpp"

NS_DJINTERP
NS_TESTING


/*
tests_report_runner_module_banner_text
  the banner that opens a module's console section.
  Tests the following:
  - the banner names the module on its own line
  - it is fenced above and below by an equals rule
  - one banner is emitted per module opened
*/
bool
tests_report_runner_module_banner_text()
{
    rr_capture cap("dtest_rr_banner.txt");

    D_RR_CHECK(cap.valid());

    dt::report_builder rb;

    rb.set_file(cap.file());
    rb.module("the_module");

    const std::string out = cap.text();

    D_RR_CHECK(rr_contains(out, "TESTING MODULE: the_module"));

    // the name line is fenced by two equals rules
    D_RR_CHECK(rr_count(out, std::string(80, '=')) == 2u);

    // a second module adds a second banner
    rb.module("another");

    const std::string out2 = cap.text();

    D_RR_CHECK(rr_contains(out2, "TESTING MODULE: another"));
    D_RR_CHECK(rr_count(out2, "TESTING MODULE: ") == 2u);

    return true;
}

/*
tests_report_runner_module_banner_description
  the optional description line.
  Tests the following:
  - a described module prints its description and a third rule
  - an undescribed module prints neither
*/
bool
tests_report_runner_module_banner_description()
{
    rr_capture cap("dtest_rr_banner_desc.txt");

    D_RR_CHECK(cap.valid());

    dt::report_builder rb;

    rb.set_file(cap.file());
    rb.module("described", "what this module covers");

    const std::string out = cap.text();

    D_RR_CHECK(rr_contains(out, "description: what this module covers"));
    D_RR_CHECK(rr_count(out, std::string(80, '=')) == 3u);

    // an undescribed module prints no description line
    rr_capture cap2("dtest_rr_banner_nodesc.txt");

    D_RR_CHECK(cap2.valid());

    dt::report_builder rb2;

    rb2.set_file(cap2.file());
    rb2.module("plain");

    const std::string out2 = cap2.text();

    D_RR_CHECK(!rr_contains(out2, "description: "));
    D_RR_CHECK(rr_count(out2, std::string(80, '=')) == 2u);

    return true;
}

/*
tests_report_runner_unit_line_symbols
  the live per-unit result lines.
  Tests the following:
  - a passing unit prints the pass label and the test's name
  - a failing unit prints the fail label
  - one line is printed per unit, whether recorded through run() or unit()
*/
bool
tests_report_runner_unit_line_symbols()
{
    rr_capture cap("dtest_rr_unit_lines.txt");

    D_RR_CHECK(cap.valid());

    dt::report_builder rb;

    rb.set_file(cap.file());
    rb.set_console(true);
    rb.module("m");

    rb.run("ran_ok", &rr_pass);
    rb.run("ran_bad", &rr_fail);
    rb.unit("recorded_ok", true);
    rb.unit("recorded_bad", false);

    const std::string out = cap.text();

    D_RR_CHECK(rr_contains(out, "  [PASS] ran_ok\n"));
    D_RR_CHECK(rr_contains(out, "  [FAIL] ran_bad\n"));
    D_RR_CHECK(rr_contains(out, "  [PASS] recorded_ok\n"));
    D_RR_CHECK(rr_contains(out, "  [FAIL] recorded_bad\n"));

    // exactly four unit lines, two of each label
    D_RR_CHECK(rr_count(out, "  [PASS] ") == 2u);
    D_RR_CHECK(rr_count(out, "  [FAIL] ") == 2u);

    return true;
}

/*
tests_report_runner_open_unit_banner
  the multi-assertion unit's own header.
  Tests the following:
  - open_unit prints a testing banner naming the unit
  - both the named and described overloads print the same banner
  - the descriptor itself is not printed on that line
*/
bool
tests_report_runner_open_unit_banner()
{
    rr_capture cap("dtest_rr_open_unit.txt");

    D_RR_CHECK(cap.valid());

    dt::report_builder rb;

    rb.set_file(cap.file());
    rb.module("m");

    rb.open_unit("plain_unit");

    D_RR_CHECK(rr_contains(cap.text(), "  --- Testing plain_unit ---\n"));

    rb.close_unit();
    rb.open_unit("described_unit", "the descriptor");

    const std::string out = cap.text();

    D_RR_CHECK(rr_contains(out, "  --- Testing described_unit ---\n"));

    // the descriptor is stored, not printed on the banner line
    D_RR_CHECK(!rr_contains(out, "Testing described_unit --- the "));
    D_RR_CHECK(rr_count(out, "  --- Testing ") == 2u);

    return true;
}

/*
tests_report_runner_check_detail_line
  the four-argument assertion's console line.
  Tests the following:
  - the line carries the label, the expression and both values
  - a passing and a failing assertion differ only in the label
*/
bool
tests_report_runner_check_detail_line()
{
    rr_capture cap("dtest_rr_check_detail.txt");

    D_RR_CHECK(cap.valid());

    dt::report_builder rb;

    rb.set_file(cap.file());
    rb.module("m");
    rb.open_unit("u");

    rb.check("size()", "4", "4", true);
    rb.check("back()", "z", "q", false);

    const std::string out = cap.text();

    D_RR_CHECK(rr_contains(out, "    [PASS] size()  (expected 4, got 4)\n"));
    D_RR_CHECK(rr_contains(out, "    [FAIL] back()  (expected z, got q)\n"));

    // two assertion lines, one of each label
    D_RR_CHECK(rr_count(out, "    [PASS] ") == 1u);
    D_RR_CHECK(rr_count(out, "    [FAIL] ") == 1u);

    return true;
}

/*
tests_report_runner_check_named_line
  the three-argument assertion's console line.
  Tests the following:
  - the line carries the identifier followed by its descriptor
  - no expected / got clause is printed for a named check
*/
bool
tests_report_runner_check_named_line()
{
    rr_capture cap("dtest_rr_check_named.txt");

    D_RR_CHECK(cap.valid());

    dt::report_builder rb;

    rb.set_file(cap.file());
    rb.module("m");
    rb.open_unit("u");

    rb.check("tests_the_thing", "what the thing does", true);
    rb.check("tests_other", "what the other does", false);

    const std::string out = cap.text();

    D_RR_CHECK(rr_contains(
        out, "    [PASS] tests_the_thing  what the thing does\n"));
    D_RR_CHECK(rr_contains(
        out, "    [FAIL] tests_other  what the other does\n"));

    // a named check never prints the detail overload's clause
    D_RR_CHECK(!rr_contains(out, "(expected "));
    D_RR_CHECK(!rr_contains(out, ", got "));

    return true;
}

/*
tests_report_runner_close_unit_line
  the unit's closing verdict line.
  Tests the following:
  - a unit whose assertions all passed closes as passed
  - a unit carrying any failure closes as failed
  - the line names the unit
*/
bool
tests_report_runner_close_unit_line()
{
    rr_capture cap("dtest_rr_close_unit.txt");

    D_RR_CHECK(cap.valid());

    dt::report_builder rb;

    rb.set_file(cap.file());
    rb.module("m");

    rb.open_unit("good_unit");
    rb.check("a", "1", "1", true);
    rb.check("b", "2", "2", true);
    rb.close_unit();

    rb.open_unit("bad_unit");
    rb.check("c", "1", "1", true);
    rb.check("d", "2", "3", false);
    rb.close_unit();

    const std::string out = cap.text();

    D_RR_CHECK(rr_contains(out, "    [PASS] good_unit unit test passed\n"));
    D_RR_CHECK(rr_contains(out, "    [FAIL] bad_unit unit test failed\n"));

    return true;
}

/*
tests_report_runner_module_results_box
  the per-module summary box printed at finish.
  Tests the following:
  - the box names the module and carries both tallies with their rates
  - the status line spells the module's verdict
  - the box is fenced by dash rules
*/
bool
tests_report_runner_module_results_box()
{
    rr_capture cap("dtest_rr_results_box.txt");

    D_RR_CHECK(cap.valid());

    dt::report_builder rb;

    rb.set_file(cap.file());
    rb.set_console(false);
    rb.module("boxed");
    rr_seed_units(rb, 3, 1);
    rb.set_console(true);

    rb.finish();

    const std::string out = cap.text();

    D_RR_CHECK(rr_contains(out, "MODULE RESULTS: boxed"));
    D_RR_CHECK(rr_contains(out, "Assertions: 3/4 passed (75.00%)"));
    D_RR_CHECK(rr_contains(out, "Unit Tests: 3/4 passed (75.00%)"));
    D_RR_CHECK(rr_contains(out, "Status: [FAIL] boxed MODULE FAILED"));

    // the box is fenced by three dash rules
    D_RR_CHECK(rr_count(out, std::string(80, '-')) == 3u);

    // a clean module reports the other way round
    rr_capture cap2("dtest_rr_results_box2.txt");

    D_RR_CHECK(cap2.valid());

    dt::report_builder rb2;

    rb2.set_file(cap2.file());
    rb2.set_console(false);
    rb2.module("clean");
    rr_seed_units(rb2, 2, 0);
    rb2.set_console(true);
    rb2.finish();

    D_RR_CHECK(rr_contains(cap2.text(), "Status: [PASS] clean MODULE PASSED"));
    D_RR_CHECK(rr_contains(cap2.text(), "Unit Tests: 2/2 passed (100.00%)"));

    return true;
}

/*
tests_report_runner_comprehensive_summary
  the cross-module roll-up.
  Tests the following:
  - all three summaries are printed, in order
  - the module, assertion and unit-test counts are the report's own
  - the success rates are the two-decimal report_pass_rate spelling
*/
bool
tests_report_runner_comprehensive_summary()
{
    rr_capture cap("dtest_rr_comprehensive.txt");

    D_RR_CHECK(cap.valid());

    dt::report_builder rb;

    rb.set_file(cap.file());
    rb.set_console(false);

    rb.module("alpha");
    rr_seed_units(rb, 2, 0);
    rb.module("beta");
    rr_seed_units(rb, 1, 1);

    rb.set_console(true);
    rb.finish();

    const std::string out = cap.text();

    D_RR_CHECK(rr_contains(out, "COMPREHENSIVE TEST RESULTS"));

    D_RR_CHECK(rr_contains(out, "MODULE SUMMARY:"));
    D_RR_CHECK(rr_contains(out, "  Modules Tested: 2"));
    D_RR_CHECK(rr_contains(out, "  Modules Passed: 1"));
    D_RR_CHECK(rr_contains(out, "  Module Success Rate: 50.00%"));

    D_RR_CHECK(rr_contains(out, "ASSERTION SUMMARY:"));
    D_RR_CHECK(rr_contains(out, "  Total Assertions: 4"));
    D_RR_CHECK(rr_contains(out, "  Assertions Passed: 3"));
    D_RR_CHECK(rr_contains(out, "  Assertions Failed: 1"));

    D_RR_CHECK(rr_contains(out, "UNIT TEST SUMMARY:"));
    D_RR_CHECK(rr_contains(out, "  Total Unit Tests: 4"));
    D_RR_CHECK(rr_contains(out, "  Unit Tests Passed: 3"));
    D_RR_CHECK(rr_contains(out, "  Unit Tests Failed: 1"));
    D_RR_CHECK(rr_contains(out, "  Unit Test Success Rate: 75.00%"));

    // the module summary precedes the assertion summary, which precedes
    // the unit-test summary
    D_RR_CHECK(out.find("MODULE SUMMARY:") < out.find("ASSERTION SUMMARY:"));
    D_RR_CHECK(out.find("ASSERTION SUMMARY:") <
               out.find("UNIT TEST SUMMARY:"));

    return true;
}

/*
tests_report_runner_assessment_pass
  the clean-run conclusion.
  Tests the following:
  - a run whose every unit passed prints the single all-passed line
  - the attention-required lines are not printed
*/
bool
tests_report_runner_assessment_pass()
{
    rr_capture cap("dtest_rr_assess_pass.txt");

    D_RR_CHECK(cap.valid());

    dt::report_builder rb;

    rb.set_file(cap.file());
    rb.set_console(false);
    rb.module("m");
    rr_seed_units(rb, 3, 0);
    rb.set_console(true);
    rb.finish();

    const std::string out = cap.text();

    D_RR_CHECK(rr_contains(out, "OVERALL ASSESSMENT:"));
    D_RR_CHECK(rr_contains(out, "  [PASS] ALL TESTS PASSED\n"));
    D_RR_CHECK(!rr_contains(out, "ATTENTION REQUIRED"));
    D_RR_CHECK(!rr_contains(out, "Review failed tests"));

    return true;
}

/*
tests_report_runner_assessment_fail
  the failing-run conclusion.
  Tests the following:
  - a run carrying any failure prints both attention lines
  - the all-passed line is not printed
  - one failing unit among many is enough
*/
bool
tests_report_runner_assessment_fail()
{
    rr_capture cap("dtest_rr_assess_fail.txt");

    D_RR_CHECK(cap.valid());

    dt::report_builder rb;

    rb.set_file(cap.file());
    rb.set_console(false);
    rb.module("m");
    rr_seed_units(rb, 9, 1);
    rb.set_console(true);
    rb.finish();

    const std::string out = cap.text();

    D_RR_CHECK(rr_contains(out, "OVERALL ASSESSMENT:"));
    D_RR_CHECK(rr_contains(
        out, "  [FAIL] SOME TESTS FAILED - ATTENTION REQUIRED\n"));
    D_RR_CHECK(rr_contains(
        out, "  [FAIL] Review failed tests before proceeding\n"));
    D_RR_CHECK(!rr_contains(out, "ALL TESTS PASSED"));

    return true;
}

/*
tests_report_runner_rule_width
  the horizontal separators.
  Tests the following:
  - every rule is exactly the configured column count wide
  - no rule is longer than that, so the width is a hard setting rather
    than a minimum
  - both rule characters are used
*/
bool
tests_report_runner_rule_width()
{
    rr_capture cap("dtest_rr_rules.txt");

    D_RR_CHECK(cap.valid());

    dt::report_builder rb;

    rb.set_file(cap.file());
    rb.module("m", "described");
    rb.unit("u", true);
    rb.finish();

    const std::string out = cap.text();
    const std::size_t n   =
        static_cast<std::size_t>(D_TEST_REPORT_RULE_COLS);

    // rules of exactly the configured width appear, in both characters
    D_RR_CHECK(rr_contains(out, std::string(n, '=')));
    D_RR_CHECK(rr_contains(out, std::string(n, '-')));

    // and never one character longer
    D_RR_CHECK(!rr_contains(out, std::string(n + 1u, '=')));
    D_RR_CHECK(!rr_contains(out, std::string(n + 1u, '-')));

    // each rule occupies a whole line
    D_RR_CHECK(rr_contains(out, std::string(n, '=') + "\n"));

    return true;
}

/*
tests_report_runner_console_disabled_emits_nothing
  the silenced builder.
  Tests the following:
  - with the console off, a whole run - banners, units, assertions,
    summaries and the final tally - writes not one byte
  - the model and the exit code are unaffected
*/
bool
tests_report_runner_console_disabled_emits_nothing()
{
    rr_capture cap("dtest_rr_silent.txt");

    D_RR_CHECK(cap.valid());

    dt::report_builder rb;

    rb.set_file(cap.file());
    rb.set_console(false);

    rb.module("m", "a description");
    rb.run("t0", &rr_pass);
    rb.run("t1", &rr_fail);
    rb.open_unit("rich", "descriptor");
    rb.check("expr", "1", "2", false);
    rb.check("named", "prose", true);
    rb.close_unit();

    const int rc = rb.finish();

    D_RR_CHECK(cap.text().empty());

    // the run itself was fully recorded and correctly judged
    D_RR_CHECK(rb.report().total_units() == 3u);
    D_RR_CHECK(rb.report().total_checks() == 4u);
    D_RR_CHECK(rc != 0);

    return true;
}

NS_END  // testing
NS_END  // djinterp
