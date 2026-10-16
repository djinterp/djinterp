/*******************************************************************************
* djinterp [test]                         test_report_runner_tests_recording.cpp
*
*
* path:      /tests/djinterp/test/output/test_report_runner_tests_recording.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "test_report_runner_tests.hpp"

NS_DJINTERP
NS_TESTING


/*
tests_report_runner_module_appends
  opening modules.
  Tests the following:
  - module() appends to the report in call order
  - the new module carries the name and description it was given
  - subsequent recording lands in the most recently opened module
*/
bool
tests_report_runner_module_appends()
{
    dt::report_builder rb;

    rb.set_console(false);

    rb.module("first", "the first module");
    rb.module("second", "the second module");

    D_RR_CHECK(rb.report().modules.size() == 2u);
    D_RR_CHECK(rb.report().modules[0].name == "first");
    D_RR_CHECK(rb.report().modules[0].description == "the first module");
    D_RR_CHECK(rb.report().modules[1].name == "second");
    D_RR_CHECK(rb.report().modules[1].description == "the second module");

    // a module opened without a description carries an empty one
    rb.module("third");

    D_RR_CHECK(rb.report().modules.size() == 3u);
    D_RR_CHECK(rb.report().modules[2].description.empty());

    // recording now lands in the third module
    rb.unit("u", true);

    D_RR_CHECK(rb.report().modules[2].units.size() == 1u);
    D_RR_CHECK(rb.report().modules[0].units.empty());

    return true;
}

/*
tests_report_runner_module_returns_reference
  the reference module() hands back.
  Tests the following:
  - the returned reference designates the stored module, not a copy
  - an edit through it is visible in the report
  - the builder keeps writing to that same module
*/
bool
tests_report_runner_module_returns_reference()
{
    dt::report_builder rb;

    rb.set_console(false);

    dt::report_module& m = rb.module("m", "original");

    D_RR_CHECK(&m == &rb.report().modules[0]);

    // an edit through the reference reaches the model
    m.description = "edited through the reference";

    D_RR_CHECK(rb.report().modules[0].description ==
               "edited through the reference");

    // and the builder's own recording lands in the same module
    rb.unit("u", true);

    D_RR_CHECK(m.units.size() == 1u);
    D_RR_CHECK(m.units[0].name == "u");

    return true;
}

/*
tests_report_runner_run_records_result
  running a predicate.
  Tests the following:
  - run() records a unit named after the test, carrying exactly one check
  - a passing predicate yields a passing unit and a failing one a failing
    unit
  - the tallies pick both up
*/
bool
tests_report_runner_run_records_result()
{
    dt::report_builder rb;

    rb.set_console(false);
    rb.module("m");

    rb.run("a_passing_test", &rr_pass);
    rb.run("a_failing_test", &rr_fail);

    const dt::report_module& m = rb.report().modules[0];

    D_RR_CHECK(m.units.size() == 2u);
    D_RR_CHECK(m.units[0].name == "a_passing_test");
    D_RR_CHECK(m.units[0].checks.size() == 1u);
    D_RR_CHECK(m.units[0].passed());
    D_RR_CHECK(m.units[1].name == "a_failing_test");
    D_RR_CHECK(!m.units[1].passed());

    D_RR_CHECK(m.total_units() == 2u);
    D_RR_CHECK(m.passed_units() == 1u);
    D_RR_CHECK(m.total_checks() == 2u);
    D_RR_CHECK(m.passed_checks() == 1u);

    return true;
}

/*
tests_report_runner_run_returns_predicate_value
  run()'s return.
  Tests the following:
  - run() hands back exactly what the predicate returned
  - the return is independent of what was recorded before it
*/
bool
tests_report_runner_run_returns_predicate_value()
{
    dt::report_builder rb;

    rb.set_console(false);
    rb.module("m");

    D_RR_CHECK(rb.run("ok", &rr_pass) == true);
    D_RR_CHECK(rb.run("bad", &rr_fail) == false);
    D_RR_CHECK(rb.run("ok_again", &rr_pass) == true);

    D_RR_CHECK(rb.report().modules[0].units.size() == 3u);

    return true;
}

/*
tests_report_runner_run_null_predicate
  the null-function-pointer guard.
  Tests the following:
  - a null predicate is not called and the unit is recorded as a failure
  - run() returns false in that case
  - the run is otherwise unharmed
*/
bool
tests_report_runner_run_null_predicate()
{
    dt::report_builder rb;

    rb.set_console(false);
    rb.module("m");

    D_RR_CHECK(rb.run("null_test", nullptr) == false);

    const dt::report_module& m = rb.report().modules[0];

    D_RR_CHECK(m.units.size() == 1u);
    D_RR_CHECK(m.units[0].name == "null_test");
    D_RR_CHECK(!m.units[0].passed());

    // recording continues normally afterwards
    rb.run("after", &rr_pass);

    D_RR_CHECK(m.units.size() == 2u);
    D_RR_CHECK(m.units[1].passed());

    return true;
}

/*
tests_report_runner_ensure_module_autocreates
  recording with nothing open.
  Tests the following:
  - the first recording call with no module open creates the default
    `tests` module
  - the guard fires only once, so later calls reuse it
  - open_unit and unit() take the same path
*/
bool
tests_report_runner_ensure_module_autocreates()
{
    dt::report_builder rb;

    rb.set_console(false);

    D_RR_CHECK(rb.report().modules.empty());

    rb.run("first", &rr_pass);

    D_RR_CHECK(rb.report().modules.size() == 1u);
    D_RR_CHECK(rb.report().modules[0].name == "tests");
    D_RR_CHECK(rb.report().modules[0].units.size() == 1u);

    // the second call reuses the module rather than making another
    rb.run("second", &rr_pass);

    D_RR_CHECK(rb.report().modules.size() == 1u);
    D_RR_CHECK(rb.report().modules[0].units.size() == 2u);

    // unit() and open_unit() take the same route on a fresh builder
    dt::report_builder rb2;

    rb2.set_console(false);
    rb2.unit("u", true);

    D_RR_CHECK(rb2.report().modules.size() == 1u);
    D_RR_CHECK(rb2.report().modules[0].name == "tests");

    dt::report_builder rb3;

    rb3.set_console(false);
    rb3.open_unit("u");

    D_RR_CHECK(rb3.report().modules.size() == 1u);
    D_RR_CHECK(rb3.report().modules[0].name == "tests");

    return true;
}

/*
tests_report_runner_unit_records_known_result
  recording an already-decided result.
  Tests the following:
  - unit() records a one-check unit without calling any predicate
  - both outcomes are recorded faithfully
  - it is the boolean counterpart of run()
*/
bool
tests_report_runner_unit_records_known_result()
{
    dt::report_builder rb;

    rb.set_console(false);
    rb.module("m");

    rb.unit("known_good", true);
    rb.unit("known_bad", false);

    const dt::report_module& m = rb.report().modules[0];

    D_RR_CHECK(m.units.size() == 2u);
    D_RR_CHECK(m.units[0].name == "known_good");
    D_RR_CHECK(m.units[0].passed());
    D_RR_CHECK(m.units[0].checks.size() == 1u);
    D_RR_CHECK(!m.units[1].passed());

    // the same shape run() would have produced
    rb.run("through_run", &rr_pass);

    D_RR_CHECK(m.units[2].checks.size() == m.units[0].checks.size());

    return true;
}

/*
tests_report_runner_open_unit_named
  beginning a multi-assertion unit.
  Tests the following:
  - open_unit appends an EMPTY unit carrying the given name
  - the unit's verdict starts as `empty`, so it is not yet a pass
  - checks recorded afterwards land in that unit
*/
bool
tests_report_runner_open_unit_named()
{
    dt::report_builder rb;

    rb.set_console(false);
    rb.module("m");
    rb.open_unit("rich_unit");

    const dt::report_module& m = rb.report().modules[0];

    D_RR_CHECK(m.units.size() == 1u);
    D_RR_CHECK(m.units[0].name == "rich_unit");
    D_RR_CHECK(m.units[0].checks.empty());
    D_RR_CHECK(m.units[0].description.empty());
    D_RR_CHECK(m.units[0].verdict == dt::report_verdict::empty);
    D_RR_CHECK(!m.units[0].passed());

    // the following checks accumulate into it
    rb.check("expr_a", "1", "1", true);
    rb.check("expr_b", "2", "2", true);

    D_RR_CHECK(m.units.size() == 1u);
    D_RR_CHECK(m.units[0].checks.size() == 2u);
    D_RR_CHECK(m.units[0].passed());

    return true;
}

/*
tests_report_runner_open_unit_described
  the descriptor overload.
  Tests the following:
  - the two-argument form stores the descriptor alongside the name
  - the described and undescribed forms are otherwise identical
  - an empty descriptor is stored as empty
*/
bool
tests_report_runner_open_unit_described()
{
    dt::report_builder rb;

    rb.set_console(false);
    rb.module("m");

    rb.open_unit("described", "what this block covers");
    rb.close_unit();
    rb.open_unit("undescribed");
    rb.close_unit();
    rb.open_unit("blank_descriptor", std::string());

    const dt::report_module& m = rb.report().modules[0];

    D_RR_CHECK(m.units.size() == 3u);
    D_RR_CHECK(m.units[0].name == "described");
    D_RR_CHECK(m.units[0].description == "what this block covers");
    D_RR_CHECK(m.units[1].description.empty());
    D_RR_CHECK(m.units[2].name == "blank_descriptor");
    D_RR_CHECK(m.units[2].description.empty());

    // both forms begin equally empty
    D_RR_CHECK(m.units[0].checks.empty());
    D_RR_CHECK(m.units[1].checks.empty());

    return true;
}

/*
tests_report_runner_check_detail_overload
  the four-argument assertion.
  Tests the following:
  - the expression, expected and actual values are all stored
  - the check's status follows the boolean
  - such a check makes the module read as assertion-level
*/
bool
tests_report_runner_check_detail_overload()
{
    dt::report_builder rb;

    rb.set_console(false);
    rb.module("m");
    rb.open_unit("u");

    rb.check("tokens.size()", "4", "4", true);
    rb.check("tokens.back()", "z", "q", false);

    const dt::report_unit& u = rb.report().modules[0].units[0];

    D_RR_CHECK(u.checks.size() == 2u);
    D_RR_CHECK(u.checks[0].expression == "tokens.size()");
    D_RR_CHECK(u.checks[0].expected == "4");
    D_RR_CHECK(u.checks[0].actual == "4");
    D_RR_CHECK(u.checks[0].status == dt::test_status::passed);

    D_RR_CHECK(u.checks[1].expression == "tokens.back()");
    D_RR_CHECK(u.checks[1].expected == "z");
    D_RR_CHECK(u.checks[1].actual == "q");
    D_RR_CHECK(u.checks[1].status == dt::test_status::failed);

    // a valued check is what makes a module render its assertion columns
    D_RR_CHECK(!u.passed());

    return true;
}

/*
tests_report_runner_check_named_overload
  the three-argument assertion.
  Tests the following:
  - the identifier is stored as the expression and the prose as the
    description
  - no expected or actual value is recorded
  - the status still follows the boolean
*/
bool
tests_report_runner_check_named_overload()
{
    dt::report_builder rb;

    rb.set_console(false);
    rb.module("m");
    rb.open_unit("u");

    rb.check("tests_the_thing", "what the thing does", true);
    rb.check("tests_other", "what the other does", false);

    const dt::report_unit& u = rb.report().modules[0].units[0];

    D_RR_CHECK(u.checks.size() == 2u);
    D_RR_CHECK(u.checks[0].expression == "tests_the_thing");
    D_RR_CHECK(u.checks[0].description == "what the thing does");
    D_RR_CHECK(u.checks[0].expected.empty());
    D_RR_CHECK(u.checks[0].actual.empty());
    D_RR_CHECK(u.checks[0].status == dt::test_status::passed);

    D_RR_CHECK(u.checks[1].description == "what the other does");
    D_RR_CHECK(u.checks[1].status == dt::test_status::failed);
    D_RR_CHECK(u.checks[1].expected.empty());

    return true;
}

/*
tests_report_runner_check_without_unit_is_noop
  a check with a module but no unit.
  Tests the following:
  - the check is silently dropped rather than faulting on an empty unit
    vector
  - no unit is created to receive it
  - both overloads behave the same way
*/
bool
tests_report_runner_check_without_unit_is_noop()
{
    rr_capture cap("dtest_rr_check_no_unit.txt");

    D_RR_CHECK(cap.valid());

    dt::report_builder rb;

    rb.set_file(cap.file());
    rb.module("m");

    rb.check("expr", "1", "1", true);
    rb.check("named", "prose", true);

    D_RR_CHECK(rb.report().modules.size() == 1u);
    D_RR_CHECK(rb.report().modules[0].units.empty());
    D_RR_CHECK(rb.report().total_checks() == 0u);

    // the dropped checks were not printed either
    D_RR_CHECK(!rr_contains(cap.text(), "expr"));
    D_RR_CHECK(!rr_contains(cap.text(), "prose"));

    return true;
}

/*
tests_report_runner_check_without_module_is_noop
  a check with nothing open at all.
  Tests the following:
  - the check is dropped without creating the default module
  - the report stays completely empty
  - close_unit in the same state is equally inert
*/
bool
tests_report_runner_check_without_module_is_noop()
{
    dt::report_builder rb;

    rb.set_console(false);

    rb.check("expr", "1", "1", true);
    rb.check("named", "prose", false);
    rb.close_unit();

    D_RR_CHECK(rb.report().modules.empty());
    D_RR_CHECK(rb.report().total_modules() == 0u);
    D_RR_CHECK(rb.report().total_units() == 0u);
    D_RR_CHECK(rb.report().total_checks() == 0u);

    // and the builder still works normally afterwards
    rb.module("m");
    rb.open_unit("u");
    rb.check("expr", "1", "1", true);

    D_RR_CHECK(rb.report().total_checks() == 1u);

    return true;
}

/*
tests_report_runner_close_unit_without_unit_is_noop
  closing nothing.
  Tests the following:
  - close_unit with a module open but no unit emits nothing
  - close_unit with no module at all is equally inert
  - repeated closes of the same unit are harmless and simply repeat the
    verdict line
*/
bool
tests_report_runner_close_unit_without_unit_is_noop()
{
    rr_capture cap("dtest_rr_close_no_unit.txt");

    D_RR_CHECK(cap.valid());

    dt::report_builder rb;

    rb.set_file(cap.file());
    rb.set_console(false);

    rb.close_unit();
    D_RR_CHECK(rb.report().modules.empty());

    rb.module("m");
    rb.close_unit();
    D_RR_CHECK(rb.report().modules[0].units.empty());

    // with a unit open, closing twice simply repeats the verdict line
    rb.set_console(true);
    rb.open_unit("u");
    rb.check("expr", "1", "1", true);
    rb.close_unit();
    rb.close_unit();

    D_RR_CHECK(rr_count(cap.text(), "u unit test passed") == 2u);
    D_RR_CHECK(rb.report().modules[0].units.size() == 1u);
    D_RR_CHECK(rb.report().modules[0].units[0].checks.size() == 1u);

    return true;
}

/*
tests_report_runner_multiple_modules_isolated
  keeping modules apart.
  Tests the following:
  - units recorded after a second module() land in that module only
  - each module keeps its own tallies
  - the report rolls both up
*/
bool
tests_report_runner_multiple_modules_isolated()
{
    dt::report_builder rb;

    rb.set_console(false);

    rb.module("alpha");
    rr_seed_units(rb, 2, 0);

    rb.module("beta");
    rr_seed_units(rb, 1, 1);

    const dt::test_report& r = rb.report();

    D_RR_CHECK(r.modules.size() == 2u);
    D_RR_CHECK(r.modules[0].total_units() == 2u);
    D_RR_CHECK(r.modules[0].passed_units() == 2u);
    D_RR_CHECK(r.modules[0].passed());

    D_RR_CHECK(r.modules[1].total_units() == 2u);
    D_RR_CHECK(r.modules[1].passed_units() == 1u);
    D_RR_CHECK(!r.modules[1].passed());

    // the report rolls both modules up
    D_RR_CHECK(r.total_modules() == 2u);
    D_RR_CHECK(r.passed_modules() == 1u);
    D_RR_CHECK(r.total_units() == 4u);
    D_RR_CHECK(r.passed_units() == 3u);
    D_RR_CHECK(r.failed_units() == 1u);
    D_RR_CHECK(!r.passed());

    // and reopening the first name makes a THIRD module, not a merge
    rb.module("alpha");
    rb.unit("late", true);

    D_RR_CHECK(r.modules.size() == 3u);
    D_RR_CHECK(r.modules[0].total_units() == 2u);
    D_RR_CHECK(r.modules[2].total_units() == 1u);

    return true;
}

NS_END  // testing
NS_END  // djinterp
