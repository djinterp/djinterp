/*******************************************************************************
* djinterp [test]                                   test_runner_tests_facade.cpp
*
*   Section II (facade surface) - construction and the non-rendering public API.
* Default construction adopts the framework option defaults; an explicit option
* set is taken by value; options() exposes a mutable/const option set and
* set_options replaces it; set_console redirects the console sink; handler()
* exposes the underlying test_handler for extra listeners.  run() then tallies a
* tree into the counters, and result / verdict / exit_code report it - with the
* deliberate rule that exit_code (via all_passed) is 0 only for a non-empty,
* everything-passed run, while verdict is the three-way empty/failed/pending/
* passed classification.
*
*
* path:      /tests/djinterp/test/test_runner_tests_facade.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.06
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "test_runner_tests.hpp"

NS_DJINTERP
NS_TESTING


// tests_facade_default_options
//   a default runner carries the framework option defaults.
bool
tests_facade_default_options()
{
    bool ok = true;
    dt::test_runner r;

    ok = D_TR_CHECK(r.options().show  == dt::test_show::all)     && ok;
    ok = D_TR_CHECK(r.options().sinks == dt::test_sink::console) && ok;
    ok = D_TR_CHECK(r.options().line_width == static_cast<std::size_t>(80)) && ok;
    ok = D_TR_CHECK(r.options().number_tests == true)            && ok;
    ok = D_TR_CHECK(r.options().format_module == "{name}")       && ok;

    return ok;
}

// tests_facade_explicit_options
//   the explicit-option constructor adopts the supplied set.
bool
tests_facade_explicit_options()
{
    bool ok = true;

    dt::test_option_set opts;
    opts.show       = dt::test_show::silent;
    opts.line_width = static_cast<std::size_t>(42);

    dt::test_runner r(opts);

    ok = D_TR_CHECK(r.options().show == dt::test_show::silent)               && ok;
    ok = D_TR_CHECK(r.options().line_width == static_cast<std::size_t>(42))  && ok;

    return ok;
}

// tests_facade_options_mutable
//   options() exposes a mutable option set; the const overload observes it.
bool
tests_facade_options_mutable()
{
    bool ok = true;
    dt::test_runner r;

    r.options().show       = dt::test_show::summary_only;
    r.options().line_width = static_cast<std::size_t>(100);

    ok = D_TR_CHECK(r.options().show == dt::test_show::summary_only)          && ok;

    const dt::test_runner& cr = r;
    ok = D_TR_CHECK(cr.options().line_width == static_cast<std::size_t>(100)) && ok;

    return ok;
}

// tests_facade_set_options
//   set_options replaces the option set wholesale.
bool
tests_facade_set_options()
{
    bool ok = true;
    dt::test_runner r;

    dt::test_option_set opts;
    opts.show = dt::test_show::failures_only;
    r.set_options(opts);

    ok = D_TR_CHECK(r.options().show == dt::test_show::failures_only) && ok;

    return ok;
}

// tests_facade_set_console
//   set_console redirects the console sink: a run's output lands in the given
// stream (not std::cout).
bool
tests_facade_set_console()
{
    bool ok = true;
    std::ostringstream oss;

    dt::test_runner r;
    r.set_console(oss);

    std::vector<dt::basic_test> tree;
    tree.push_back(make_leaf(dt::test_status::passed, "alpha"));
    r.run(tree);

    ok = D_TR_CHECK(contains(oss.str(), "alpha")) && ok;

    return ok;
}

// tests_facade_handler_access
//   handler() returns the same object mutably and const, and a listener bound
// through it fires during the walk alongside the reporter.
bool
tests_facade_handler_access()
{
    bool ok = true;
    std::ostringstream oss;

    dt::test_runner r;
    r.set_console(oss);

    const dt::test_runner& cr = r;
    ok = D_TR_CHECK(&r.handler() == &cr.handler()) && ok;   // one underlying handler

    int hits = 0;
    r.handler().on<dt::on_test_end>(
        [&hits](const dt::basic_test*) { ++hits; });

    std::vector<dt::basic_test> tree;
    tree.push_back(make_leaf(dt::test_status::passed, "a"));
    tree.push_back(make_leaf(dt::test_status::failed, "b"));
    r.run(tree);

    ok = D_TR_CHECK(hits == 2) && ok;   // extra listener fired once per leaf

    return ok;
}

// tests_facade_run_counters
//   run() tallies each leaf status into the running counters.
bool
tests_facade_run_counters()
{
    bool ok = true;
    std::ostringstream oss;

    dt::test_runner r;
    r.set_console(oss);

    std::vector<dt::basic_test> tree;
    tree.push_back(make_leaf(dt::test_status::passed,  "a"));
    tree.push_back(make_leaf(dt::test_status::passed,  "b"));
    tree.push_back(make_leaf(dt::test_status::failed,  "c"));
    tree.push_back(make_leaf(dt::test_status::skipped, "d"));
    tree.push_back(make_leaf(dt::test_status::error,   "e"));
    tree.push_back(make_leaf(dt::test_status::pending, "f"));
    r.run(tree);

    ok = D_TR_CHECK(r.passed()  == static_cast<std::size_t>(2)) && ok;
    ok = D_TR_CHECK(r.failed()  == static_cast<std::size_t>(1)) && ok;
    ok = D_TR_CHECK(r.skipped() == static_cast<std::size_t>(1)) && ok;
    ok = D_TR_CHECK(r.errors()  == static_cast<std::size_t>(1)) && ok;
    ok = D_TR_CHECK(r.pending() == static_cast<std::size_t>(1)) && ok;
    ok = D_TR_CHECK(r.total()   == static_cast<std::size_t>(6)) && ok;

    return ok;
}

// tests_facade_verdict
//   verdict is the three-way classification of the walked tree.
bool
tests_facade_verdict()
{
    bool ok = true;
    std::ostringstream oss;

    {   // all passed -> passed
        dt::test_runner r; r.set_console(oss);
        std::vector<dt::basic_test> t;
        t.push_back(make_leaf(dt::test_status::passed, "a"));
        t.push_back(make_leaf(dt::test_status::passed, "b"));
        r.run(t);
        ok = D_TR_CHECK(r.verdict() == dt::session_verdict::passed) && ok;
    }
    {   // a real failure dominates -> failed
        dt::test_runner r; r.set_console(oss);
        std::vector<dt::basic_test> t;
        t.push_back(make_leaf(dt::test_status::passed, "a"));
        t.push_back(make_leaf(dt::test_status::failed, "b"));
        r.run(t);
        ok = D_TR_CHECK(r.verdict() == dt::session_verdict::failed) && ok;
    }
    {   // work remains, nothing failed -> pending
        dt::test_runner r; r.set_console(oss);
        std::vector<dt::basic_test> t;
        t.push_back(make_leaf(dt::test_status::passed,  "a"));
        t.push_back(make_leaf(dt::test_status::pending, "b"));
        r.run(t);
        ok = D_TR_CHECK(r.verdict() == dt::session_verdict::pending) && ok;
    }
    {   // nothing observed -> empty
        dt::test_runner r; r.set_console(oss);
        std::vector<dt::basic_test> t;
        r.run(t);
        ok = D_TR_CHECK(r.verdict() == dt::session_verdict::empty) && ok;
    }

    return ok;
}

// tests_facade_exit_code
//   exit_code is 0 only when at least one leaf was observed and every leaf
// passed; a failure, a pending, or an empty run all yield 1.
bool
tests_facade_exit_code()
{
    bool ok = true;
    std::ostringstream oss;

    {   // all passed -> 0
        dt::test_runner r; r.set_console(oss);
        std::vector<dt::basic_test> t;
        t.push_back(make_leaf(dt::test_status::passed, "a"));
        t.push_back(make_leaf(dt::test_status::passed, "b"));
        r.run(t);
        ok = D_TR_CHECK(r.exit_code() == 0) && ok;
    }
    {   // a failure -> 1
        dt::test_runner r; r.set_console(oss);
        std::vector<dt::basic_test> t;
        t.push_back(make_leaf(dt::test_status::failed, "a"));
        r.run(t);
        ok = D_TR_CHECK(r.exit_code() == 1) && ok;
    }
    {   // pending (unfinished) -> 1, never green
        dt::test_runner r; r.set_console(oss);
        std::vector<dt::basic_test> t;
        t.push_back(make_leaf(dt::test_status::pending, "a"));
        r.run(t);
        ok = D_TR_CHECK(r.exit_code() == 1) && ok;
    }
    {   // empty -> 1
        dt::test_runner r; r.set_console(oss);
        std::vector<dt::basic_test> t;
        r.run(t);
        ok = D_TR_CHECK(r.exit_code() == 1) && ok;
    }

    return ok;
}

// tests_facade_result_snapshot
//   result() returns the session snapshot, agreeing with the counter accessors.
bool
tests_facade_result_snapshot()
{
    bool ok = true;
    std::ostringstream oss;

    dt::test_runner r;
    r.set_console(oss);

    std::vector<dt::basic_test> t;
    t.push_back(make_leaf(dt::test_status::passed, "a"));
    t.push_back(make_leaf(dt::test_status::failed, "b"));
    r.run(t);

    dt::session_result res = r.result();
    ok = D_TR_CHECK(res.total  == static_cast<std::size_t>(2)) && ok;
    ok = D_TR_CHECK(res.passed == static_cast<std::size_t>(1)) && ok;
    ok = D_TR_CHECK(res.failed == static_cast<std::size_t>(1)) && ok;
    ok = D_TR_CHECK(res.passed == r.passed() && res.total == r.total()) && ok;

    return ok;
}


NS_END  // testing
NS_END  // djinterp
