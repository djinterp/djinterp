/******************************************************************************
* djinterp [tests]                    test_report_runner_table_tests_record.cpp
*
*   Module / unit / assertion recording, verified through the exposed
* test_report: module opening, run's one-check units and its return, the null
* predicate, the default module, unit(), the open_unit + check overloads, the
* no-op guards, and the report-wide roll-up.  The console is disabled so these
* tests read the model, not the printed text (that is the console section).
*
* path:      /tests/djinterp/test/output/test_report_runner_table_tests_record.cpp
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.19
******************************************************************************/

#include "test_report_runner_table_tests.hpp"


NS_DJINTERP
NS_TESTING


bool
tests_report_runner_table_module_opens()
{
    dt::report_builder _rb;
    _rb.set_console(false);

    dt::report_module& _m = _rb.module("obj", "unified object");

    D_RRT_CHECK(_rb.report().modules.size() == std::size_t(1));
    D_RRT_CHECK(_rb.report().modules[0].name == "obj");
    D_RRT_CHECK(_rb.report().modules[0].description == "unified object");
    D_RRT_CHECK(&_m == &_rb.report().modules[0]);   // returns the stored module

    // a second module becomes current
    _rb.module("second", "");
    D_RRT_CHECK(_rb.report().modules.size() == std::size_t(2));
    D_RRT_CHECK(_rb.report().modules[1].name == "second");

    return true;
}


bool
tests_report_runner_table_run_records_and_returns()
{
    dt::report_builder _rb;
    _rb.set_console(false);
    _rb.module("m", "");

    const bool _p = _rb.run("passing", &rr_pred_pass);
    const bool _f = _rb.run("failing", &rr_pred_fail);

    D_RRT_CHECK(_p == true);
    D_RRT_CHECK(_f == false);

    const dt::report_module& _m = _rb.report().modules[0];
    D_RRT_CHECK(_m.units.size() == std::size_t(2));
    D_RRT_CHECK(_m.units[0].name == "passing");
    D_RRT_CHECK(_m.units[0].total_checks() == std::size_t(1));   // one-check unit
    D_RRT_CHECK(_m.units[0].passed() == true);
    D_RRT_CHECK(_m.units[1].name == "failing");
    D_RRT_CHECK(_m.units[1].passed() == false);

    return true;
}


bool
tests_report_runner_table_run_null_predicate()
{
    dt::report_builder _rb;
    _rb.set_console(false);
    _rb.module("m", "");

    const bool _r = _rb.run("null", nullptr);

    D_RRT_CHECK(_r == false);                                    // null -> fail
    D_RRT_CHECK(_rb.report().modules[0].units[0].passed() == false);

    return true;
}


bool
tests_report_runner_table_run_creates_default_module()
{
    dt::report_builder _rb;
    _rb.set_console(false);

    // no module() first: run must open the default "tests" module
    _rb.run("t", &rr_pred_pass);

    D_RRT_CHECK(_rb.report().modules.size() == std::size_t(1));
    D_RRT_CHECK(_rb.report().modules[0].name == "tests");
    D_RRT_CHECK(_rb.report().modules[0].units.size() == std::size_t(1));
    D_RRT_CHECK(_rb.report().modules[0].units[0].name == "t");

    return true;
}


bool
tests_report_runner_table_unit_records_known()
{
    dt::report_builder _rb;
    _rb.set_console(false);
    _rb.module("m", "");

    _rb.unit("known_pass", true);
    _rb.unit("known_fail", false);

    const dt::report_module& _m = _rb.report().modules[0];
    D_RRT_CHECK(_m.units.size() == std::size_t(2));
    D_RRT_CHECK(_m.units[0].name == "known_pass");
    D_RRT_CHECK(_m.units[0].passed() == true);
    D_RRT_CHECK(_m.units[1].passed() == false);

    return true;
}


bool
tests_report_runner_table_open_unit_and_check_detail()
{
    dt::report_builder _rb;
    _rb.set_console(false);
    _rb.module("m", "");

    _rb.open_unit("case");
    _rb.check("x == y", "4", "4", true);
    _rb.check("a == b", "1", "2", false);

    const dt::report_module& _m = _rb.report().modules[0];
    D_RRT_CHECK(_m.units.size() == std::size_t(1));

    const dt::report_unit& _u = _m.units[0];
    D_RRT_CHECK(_u.name == "case");
    D_RRT_CHECK(_u.checks.size() == std::size_t(2));

    D_RRT_CHECK(_u.checks[0].expression == "x == y");
    D_RRT_CHECK(_u.checks[0].expected   == "4");
    D_RRT_CHECK(_u.checks[0].actual     == "4");
    D_RRT_CHECK(_u.checks[0].status     == dt::test_status::passed);

    D_RRT_CHECK(_u.checks[1].expected == "1");
    D_RRT_CHECK(_u.checks[1].actual   == "2");
    D_RRT_CHECK(_u.checks[1].status   == dt::test_status::failed);

    return true;
}


bool
tests_report_runner_table_check_noop_without_unit()
{
    // (a) a module is open but no unit: check / close_unit are no-ops
    {
        dt::report_builder _rb;
        _rb.set_console(false);
        _rb.module("m", "");

        _rb.check("e", "x", "a", true);     // detail overload  -> no-op
        _rb.close_unit();                    // no-op

        D_RRT_CHECK(_rb.report().modules[0].units.empty());
    }

    // (b) no module at all: the same calls are inert and create nothing
    {
        dt::report_builder _rb;
        _rb.set_console(false);

        _rb.check("e", "x", "a", true);     // !have_module -> no-op
        _rb.close_unit();                    // no-op

        D_RRT_CHECK(_rb.report().modules.empty());
    }

    return true;
}


bool
tests_report_runner_table_multi_module_tallies()
{
    dt::report_builder _rb;
    _rb.set_console(false);

    _rb.module("m1", "");
    _rb.run("a", &rr_pred_pass);
    _rb.run("b", &rr_pred_pass);

    _rb.module("m2", "");
    _rb.run("c", &rr_pred_pass);
    _rb.run("d", &rr_pred_fail);

    const dt::test_report& _r = _rb.report();
    D_RRT_CHECK(_r.total_modules()  == std::size_t(2));
    D_RRT_CHECK(_r.total_units()    == std::size_t(4));
    D_RRT_CHECK(_r.passed_units()   == std::size_t(3));
    D_RRT_CHECK(_r.failed_units()   == std::size_t(1));
    D_RRT_CHECK(_r.total_checks()   == std::size_t(4));   // one check per run
    D_RRT_CHECK(_r.passed_checks()  == std::size_t(3));
    D_RRT_CHECK(_r.failed_checks()  == std::size_t(1));
    D_RRT_CHECK(_r.passed()         == false);            // m2 failed

    return true;
}


NS_END  // testing
NS_END  // djinterp
