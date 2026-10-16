/*******************************************************************************
* djinterp [test]                       test_report_runner_tests_convenience.cpp
*
*
* path:      /tests/djinterp/test/output/test_report_runner_tests_convenience.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// std
#include <ostream>
// djinterp
#include "test_report_runner_tests.hpp"

NS_DJINTERP
NS_TESTING


// rr_streamable
//   type: a small user-defined value the report subsystem knows nothing
// about, carrying its own stream insertion operator so the generic
// to_report_string arm has a non-builtin type to render.
struct rr_streamable
{
    int tag;

    explicit rr_streamable(int _tag) : tag(_tag) {}
};

// operator<<
//   renders an rr_streamable as "tag#<n>", a spelling no builtin path would
// produce, so a passing assertion proves the generic arm was taken.
inline std::ostream&
operator<<(
    std::ostream&        _os,
    const rr_streamable& _v
)
{
    _os << "tag#" << _v.tag;

    return _os;
}


/*
tests_report_runner_run_named_tests_exit_code
  the compact table-driven entry point.
  Tests the following:
  - a table whose every predicate passes returns zero
  - one failing predicate anywhere in the table returns non-zero
  - a null predicate in the table counts as a failure
*/
bool
tests_report_runner_run_named_tests_exit_code()
{
    const std::string       names[3] = { "a", "b", "c" };
    dt::test_predicate_fn   fns[3]   = { &rr_pass, &rr_pass, &rr_pass };

    D_RR_CHECK(dt::run_named_tests("clean", "", names, fns, 3,
                                   dt::default_test_options()) == 0);

    // one failure anywhere flips the code
    fns[1] = &rr_fail;
    D_RR_CHECK(dt::run_named_tests("dirty", "", names, fns, 3,
                                   dt::default_test_options()) != 0);

    // and so does a null entry
    fns[1] = &rr_pass;
    fns[2] = nullptr;
    D_RR_CHECK(dt::run_named_tests("nulled", "", names, fns, 3,
                                   dt::default_test_options()) != 0);

    return true;
}

/*
tests_report_runner_run_named_tests_records_all
  what the table becomes.
  Tests the following:
  - a prefix of the table is run when a shorter count is given
  - the count, not the array length, decides how many tests run
  - the module name and description reach the run
*/
bool
tests_report_runner_run_named_tests_records_all()
{
    const std::string     names[4] = { "t0", "t1", "t2", "t3" };
    dt::test_predicate_fn fns[4]   =
        { &rr_pass, &rr_pass, &rr_fail, &rr_fail };

    // only the first two entries are run, so the run is clean
    D_RR_CHECK(dt::run_named_tests("prefix", "the first two only",
                                   names, fns, 2,
                                   dt::default_test_options()) == 0);

    // taking three reaches the first failure
    D_RR_CHECK(dt::run_named_tests("three", "", names, fns, 3,
                                   dt::default_test_options()) != 0);

    return true;
}

/*
tests_report_runner_run_named_tests_zero_count
  the degenerate table.
  Tests the following:
  - a zero-length table runs nothing and does not dereference the arrays
  - the module is still opened, so the run is empty rather than absent
  - an empty run is not a pass
*/
bool
tests_report_runner_run_named_tests_zero_count()
{
    const std::string     names[1] = { "unused" };
    dt::test_predicate_fn fns[1]   = { &rr_fail };

    // the arrays are never read, so even a failing entry is irrelevant
    const int rc = dt::run_named_tests("empty_table", "nothing to run",
                                       names, fns, 0,
                                       dt::default_test_options());

    D_RR_CHECK(rc != 0);

    // null arrays are equally safe at a zero count
    const int rc2 = dt::run_named_tests("null_table", "",
                                        nullptr, nullptr, 0,
                                        dt::default_test_options());

    D_RR_CHECK(rc2 != 0);

    return true;
}

/*
tests_report_runner_check_eq_records_detail
  the assertion-capturing macro.
  Tests the following:
  - the macro stringizes its expression argument into the ASSERTION column
  - both values are rendered into the EXPECTED and ACTUAL columns
  - a matching comparison is recorded as a pass
*/
bool
tests_report_runner_check_eq_records_detail()
{
    dt::report_builder rb;

    rb.set_console(false);
    rb.module("m");
    rb.open_unit("u");

    const int value = 21;

    D_CHECK_EQ(rb, value * 2, 42);

    const dt::report_unit& u = rb.report().modules[0].units[0];

    D_RR_CHECK(u.checks.size() == 1u);
    D_RR_CHECK(u.checks[0].expression == "value * 2");
    D_RR_CHECK(u.checks[0].expected == "42");
    D_RR_CHECK(u.checks[0].actual == "42");
    D_RR_CHECK(u.checks[0].status == dt::test_status::passed);

    // it works over strings too
    const std::string s = "abc";

    D_CHECK_EQ(rb, s, std::string("abc"));

    D_RR_CHECK(u.checks.size() == 2u);
    D_RR_CHECK(u.checks[1].expression == "s");
    D_RR_CHECK(u.checks[1].expected == "abc");
    D_RR_CHECK(u.checks[1].actual == "abc");
    D_RR_CHECK(u.checks[1].status == dt::test_status::passed);

    return true;
}

/*
tests_report_runner_check_eq_reports_mismatch
  a failed comparison.
  Tests the following:
  - a mismatch is recorded as a failure
  - both the expected and the actual value are preserved, so the table
    can show what went wrong
  - the enclosing unit fails with it
*/
bool
tests_report_runner_check_eq_reports_mismatch()
{
    dt::report_builder rb;

    rb.set_console(false);
    rb.module("m");
    rb.open_unit("u");

    const int got = 7;

    D_CHECK_EQ(rb, got, 8);

    const dt::report_unit& u = rb.report().modules[0].units[0];

    D_RR_CHECK(u.checks.size() == 1u);
    D_RR_CHECK(u.checks[0].expression == "got");
    D_RR_CHECK(u.checks[0].expected == "8");
    D_RR_CHECK(u.checks[0].actual == "7");
    D_RR_CHECK(u.checks[0].status == dt::test_status::failed);

    // the unit and the whole run fail with it
    D_RR_CHECK(!u.passed());
    D_RR_CHECK(!rb.report().passed());

    return true;
}

/*
tests_report_runner_check_eq_evaluates_once
  the macro's side-effect discipline.
  Tests the following:
  - the expression argument is evaluated exactly once, even though it is
    used three times in the expansion
  - the expected argument is likewise evaluated exactly once
*/
bool
tests_report_runner_check_eq_evaluates_once()
{
    dt::report_builder rb;

    rb.set_console(false);
    rb.module("m");
    rb.open_unit("u");

    rr_calls() = 0;

    D_CHECK_EQ(rb, rr_counted(5), 5);

    // the expression appears three times in the expansion but runs once
    D_RR_CHECK(rr_calls() == 1);

    rr_calls() = 0;

    D_CHECK_EQ(rb, 9, rr_counted(9));

    D_RR_CHECK(rr_calls() == 1);

    // both operands together are still two evaluations, not six
    rr_calls() = 0;

    D_CHECK_EQ(rb, rr_counted(3), rr_counted(3));

    D_RR_CHECK(rr_calls() == 2);

    // and every one of them was recorded correctly
    const dt::report_unit& u = rb.report().modules[0].units[0];

    D_RR_CHECK(u.checks.size() == 3u);
    D_RR_CHECK(u.passed());

    return true;
}

/*
tests_report_runner_to_report_string_strings
  rendering text values.
  Tests the following:
  - a std::string passes through unchanged, including an empty one
  - a C-string literal is copied
  - a NULL C-string renders empty rather than faulting
*/
bool
tests_report_runner_to_report_string_strings()
{
    namespace rd = ::djinterp::test::report_detail;

    D_RR_CHECK(rd::to_report_string(std::string("hello")) == "hello");
    D_RR_CHECK(rd::to_report_string(std::string()).empty());
    D_RR_CHECK(rd::to_report_string(std::string("  spaced  ")) ==
               "  spaced  ");

    D_RR_CHECK(rd::to_report_string("literal") == "literal");
    D_RR_CHECK(rd::to_report_string("") == "");

    // the null guard - a null pointer renders as an empty cell
    const char* nothing = nullptr;

    D_RR_CHECK(rd::to_report_string(nothing).empty());

    return true;
}

/*
tests_report_runner_to_report_string_bool
  rendering booleans.
  Tests the following:
  - true and false render as those words, not as 1 and 0
  - the bool overload is selected ahead of the generic template
*/
bool
tests_report_runner_to_report_string_bool()
{
    namespace rd = ::djinterp::test::report_detail;

    D_RR_CHECK(rd::to_report_string(true) == "true");
    D_RR_CHECK(rd::to_report_string(false) == "false");

    // an expression of bool type takes the same overload
    const int a = 1;
    const int b = 2;

    D_RR_CHECK(rd::to_report_string(a < b) == "true");
    D_RR_CHECK(rd::to_report_string(a == b) == "false");

    // the words, not the numerals a stream would print for an int
    D_RR_CHECK(rd::to_report_string(true) != "1");
    D_RR_CHECK(rd::to_report_string(false) != "0");

    return true;
}

/*
tests_report_runner_to_report_string_generic
  rendering everything else.
  Tests the following:
  - an integral value is rendered through its stream insertion operator
  - so is a floating-point value and a character
  - a user-defined type with an operator<< is rendered by it
*/
bool
tests_report_runner_to_report_string_generic()
{
    namespace rd = ::djinterp::test::report_detail;

    D_RR_CHECK(rd::to_report_string(42) == "42");
    D_RR_CHECK(rd::to_report_string(-7) == "-7");
    D_RR_CHECK(rd::to_report_string(0) == "0");
    D_RR_CHECK(rd::to_report_string(static_cast<std::size_t>(1234)) ==
               "1234");

    // a character is rendered as itself, not as its code
    D_RR_CHECK(rd::to_report_string('x') == "x");

    // a floating-point value takes the stream's default formatting
    D_RR_CHECK(rd::to_report_string(3.5) == "3.5");
    D_RR_CHECK(rd::to_report_string(2.0) == "2");

    // and a type the report knows nothing about is rendered by its own
    // insertion operator
    D_RR_CHECK(rd::to_report_string(rr_streamable(5)) == "tag#5");
    D_RR_CHECK(rd::to_report_string(rr_streamable(-1)) == "tag#-1");

    return true;
}

NS_END  // testing
NS_END  // djinterp
