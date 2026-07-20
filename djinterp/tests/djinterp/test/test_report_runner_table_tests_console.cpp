/******************************************************************************
* djinterp [tests]                    test_report_runner_table_tests_console.cpp
*
*   The live console and the finish() summaries, captured by pointing the
* builder's FILE* sink at a tmpfile().  Asserts the module banner (with and
* without a description), the per-test result lines, the module results box, the
* comprehensive roll-up, the pass and fail assessments, the open_unit/check/
* close_unit lines, the disabled-console silence, and the finish() exit code.
* The exact ratio / rate / tally strings are pinned against known runs.
*
* path:      /tests/djinterp/test/output/test_report_runner_table_tests_console.cpp
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.19
******************************************************************************/

#include "test_report_runner_table_tests.hpp"


NS_DJINTERP
NS_TESTING


bool
tests_report_runner_table_console_module_banner()
{
    // with a description: banner + description line + rule of '='
    {
        console_capture    _cap;
        dt::report_builder _rb;
        _rb.set_file(_cap.f);

        _rb.module("mymod", "mydesc");

        const std::string _t = _cap.str();
        D_RRT_CHECK(contains(_t, "TESTING MODULE: mymod"));
        D_RRT_CHECK(contains(_t, "description: mydesc"));
        D_RRT_CHECK(contains(_t, std::string(80, '=')));   // rule('=')
    }

    // without a description: no description line
    {
        console_capture    _cap;
        dt::report_builder _rb;
        _rb.set_file(_cap.f);

        _rb.module("bare", "");

        const std::string _t = _cap.str();
        D_RRT_CHECK(contains(_t, "TESTING MODULE: bare"));
        D_RRT_CHECK(!contains(_t, "description:"));
    }

    return true;
}


bool
tests_report_runner_table_console_unit_lines()
{
    console_capture    _cap;
    dt::report_builder _rb;
    _rb.set_file(_cap.f);

    _rb.module("m", "");
    _rb.run("passing_test", &rr_pred_pass);
    _rb.run("failing_test", &rr_pred_fail);

    const std::string _t = _cap.str();
    D_RRT_CHECK(contains(_t, "  [PASS] passing_test"));
    D_RRT_CHECK(contains(_t, "  [FAIL] failing_test"));

    return true;
}


bool
tests_report_runner_table_console_finish_summaries()
{
    console_capture    _cap;
    dt::report_builder _rb;
    _rb.set_file(_cap.f);

    _rb.module("mod", "");
    _rb.run("t1", &rr_pred_pass);

    const int         _code = _rb.finish();
    const std::string _t    = _cap.str();

    D_RRT_CHECK(_code == 0);

    // per-module results box (exact ratio + rate)
    D_RRT_CHECK(contains(_t, "MODULE RESULTS: mod"));
    D_RRT_CHECK(contains(_t, "Assertions: 1/1 passed (100.00%)"));
    D_RRT_CHECK(contains(_t, "Unit Tests: 1/1 passed (100.00%)"));
    D_RRT_CHECK(contains(_t, "Status: [PASS] mod MODULE PASSED"));

    // cross-module comprehensive roll-up (size_str counts)
    D_RRT_CHECK(contains(_t, "COMPREHENSIVE TEST RESULTS"));
    D_RRT_CHECK(contains(_t, "Modules Tested: 1"));
    D_RRT_CHECK(contains(_t, "Total Assertions: 1"));
    D_RRT_CHECK(contains(_t, "Total Unit Tests: 1"));

    // overall assessment (pass arm) + final tally
    D_RRT_CHECK(contains(_t, "OVERALL ASSESSMENT:"));
    D_RRT_CHECK(contains(_t, "[PASS] ALL TESTS PASSED"));
    D_RRT_CHECK(contains(_t, "passed: 1   failed: 0"));

    return true;
}


bool
tests_report_runner_table_console_assessment_fail()
{
    console_capture    _cap;
    dt::report_builder _rb;
    _rb.set_file(_cap.f);

    _rb.module("mod", "");
    _rb.run("t1", &rr_pred_pass);
    _rb.run("t2", &rr_pred_fail);

    const int         _code = _rb.finish();
    const std::string _t    = _cap.str();

    D_RRT_CHECK(_code == 1);
    D_RRT_CHECK(contains(_t, "Status: [FAIL] mod MODULE FAILED"));
    D_RRT_CHECK(contains(_t, "[FAIL] SOME TESTS FAILED - ATTENTION REQUIRED"));
    D_RRT_CHECK(contains(_t, "Review failed tests before proceeding"));
    D_RRT_CHECK(contains(_t, "passed: 1   failed: 1"));

    return true;
}


bool
tests_report_runner_table_console_unit_block_lines()
{
    console_capture    _cap;
    dt::report_builder _rb;
    _rb.set_file(_cap.f);

    _rb.module("m", "");
    _rb.open_unit("mycase");
    _rb.check("x == y", "4", "4", true);         // detail overload
    _rb.close_unit();

    const std::string _t = _cap.str();
    D_RRT_CHECK(contains(_t, "  --- Testing mycase ---"));
    D_RRT_CHECK(contains(_t, "    [PASS] x == y  (expected 4, got 4)"));
    D_RRT_CHECK(contains(_t, "    [PASS] mycase unit test passed"));

    return true;
}


bool
tests_report_runner_table_console_disabled()
{
    // (a) console explicitly disabled: the sink stays empty
    {
        console_capture    _cap;
        dt::report_builder _rb;
        _rb.set_file(_cap.f);
        _rb.set_console(false);

        _rb.module("m", "");
        _rb.run("t", &rr_pred_pass);
        _rb.finish();

        D_RRT_CHECK(_cap.str().empty());
    }

    // (b) console enabled but the sink is null: emit's file guard makes the
    //     whole run inert (and, above all, does not crash)
    {
        dt::report_builder _rb;
        _rb.set_file(nullptr);            // console still on (default)

        _rb.module("m", "");
        _rb.open_unit("u");
        _rb.check("e", "x", "a", true);
        _rb.close_unit();
        const int _code = _rb.finish();

        D_RRT_CHECK(_code == 0);           // reached the end with no output sink
    }

    return true;
}


bool
tests_report_runner_table_finish_exit_code()
{
    // a clean run returns 0
    {
        dt::report_builder _rb;
        _rb.set_console(false);
        _rb.module("m", "");
        _rb.run("t", &rr_pred_pass);

        D_RRT_CHECK(_rb.finish() == 0);
    }

    // a run with a failure returns 1
    {
        dt::report_builder _rb;
        _rb.set_console(false);
        _rb.module("m", "");
        _rb.run("t", &rr_pred_fail);

        D_RRT_CHECK(_rb.finish() == 1);
    }

    return true;
}


NS_END  // testing
NS_END  // djinterp
