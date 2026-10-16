/*******************************************************************************
* djinterp [test]                            test_defaults_tests_integration.cpp
*
*   The integration tier - the entry points that project a module_spec onto the
* two views the framework keeps in step: the six-kind test_tree and the report
* sink.  build_enriched_tree lowers a suite/module/block/test spec into a
* rank-checked tree, running each predicate as it lowers so the leaf verdicts
* (count_passed / count_failed / any_failed) are live on return.  drive_report
* projects the same spec onto a report_builder (module -> one open_unit per
* block -> one check per test -> close_unit).  run_module / run_suite tie it
* together and return a process exit code (0 all-pass, non-zero on any failure).
* describe_tree / tree_summary are the console views.
*
*   Built against the REAL test_tree and the REAL report_builder - no mock and
* no process-global log.  The tree assertions read the tree's live leaf
* counters.  The drive_report assertions read the report_builder's accumulated
* test_report model (rb.report()): module -> unit -> check structure, with each
* check's descriptor and resolved status - exactly what the console and PDF
* faces render, so the projection is observed on the artifact it actually
* produces.  The run_module / run_suite assertions read the real process exit
* code (their observable contract; each owns its builder internally).
*
*   NOTE: run_module / run_suite emit their live console report as they run
* (they own a console-enabled builder with no injection seam), so those two
* cases print a full report to stdout.  That is expected; only their return
* code is asserted.  The drive_report cases silence their builder
* (set_console(false)) since they inspect the model, not the stream.
*
*
* path:      /tests/djinterp/test/test_defaults_tests_integration.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.06
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "test_defaults_tests.hpp"

NS_DJINTERP
NS_TESTING

namespace
{
    // file-scope predicates (need addresses for test_spec::fn)
    bool ig_pass() { return true; }
    bool ig_fail() { return false; }

    // a test_spec with a given name + predicate
    dt::test_spec spec(const char* _name, bool (*_fn)())
    {
        dt::test_spec s; s.name = _name; s.descriptor = "d"; s.fn = _fn; return s;
    }

    // the canonical fixture: module M, block B1 = {ok(pass), bad(fail)},
    // block B2 = {ok2(pass)}  ->  M=1, B=2, U=3, pass=2, fail=1
    dt::module_spec fixture_module()
    {
        dt::module_spec m; m.name = "M"; m.descriptor = "md";
        dt::block_spec b1; b1.name = "B1"; b1.descriptor = "";
        b1.tests.push_back(spec("ok",  &ig_pass));
        b1.tests.push_back(spec("bad", &ig_fail));
        dt::block_spec b2; b2.name = "B2"; b2.descriptor = "";
        b2.tests.push_back(spec("ok2", &ig_pass));
        m.blocks.push_back(b1); m.blocks.push_back(b2);
        return m;
    }

    // a single-block module of N tests, all pass (fail=false -> all fail)
    dt::module_spec uniform_module(const char* _name, std::size_t _n, bool _pass)
    {
        dt::module_spec m; m.name = _name; m.descriptor = "d";
        dt::block_spec b; b.name = "blk"; b.descriptor = "";
        for (std::size_t i = 0; i < _n; ++i)
        {
            b.tests.push_back(spec("t", _pass ? &ig_pass : &ig_fail));
        }
        m.blocks.push_back(b);
        return m;
    }

    // count nodes of a given kind by walking begin()..end()
    std::size_t count_kind(const dt::enriched_tree& _t, dt::test_type_id _kind)
    {
        std::size_t n = 0;
        for (auto it = _t.begin(); it != _t.end(); ++it)
        {
            if ((*it).type_id() == _kind) { ++n; }
        }
        return n;
    }
    std::size_t iterated(const dt::enriched_tree& _t)
    {
        std::size_t n = 0;
        for (auto it = _t.begin(); it != _t.end(); ++it) { ++n; }
        return n;
    }
}


// tests_integration_tree_shape
//   build_enriched_tree lowers the fixture and evaluates it: the leaf verdicts
// are live, and the node count matches the structure.
bool
tests_integration_tree_shape()
{
    bool ok = true;
    dt::enriched_tree t = dt::build_enriched_tree("S", "sd", fixture_module());

    // structural: 1 suite + 1 module + 2 blocks + 3 units + 3 test_fn leaves = 10
    // typed nodes (the walk also carries a root holder, so size() == 11).
    const std::size_t logical =
        count_kind(t, dt::D_TEST_KIND_TEST_SUITE)  +
        count_kind(t, dt::D_TEST_KIND_TEST_MODULE) +
        count_kind(t, dt::D_TEST_KIND_TEST_BLOCK)  +
        count_kind(t, dt::D_TEST_KIND_UNIT_TEST)   +
        count_kind(t, dt::D_TEST_KIND_TEST_FN);
    ok = D_TD_CHECK(logical == 10)             && ok;
    ok = D_TD_CHECK(iterated(t) == t.size())   && ok;   // the walk spans the whole tree
    ok = D_TD_CHECK(t.count_passed() == 2)     && ok;   // ok, ok2
    ok = D_TD_CHECK(t.count_failed() == 1)     && ok;   // bad
    ok = D_TD_CHECK(t.count_passed() + t.count_failed() == 3) && ok;  // all leaves

    return ok;
}

// tests_integration_tree_kinds
//   the lowered tree carries exactly one node of each interior kind per spec
// element, with a test_fn leaf under every unit.
bool
tests_integration_tree_kinds()
{
    bool ok = true;
    dt::enriched_tree t = dt::build_enriched_tree("S", "sd", fixture_module());

    ok = D_TD_CHECK(count_kind(t, dt::D_TEST_KIND_TEST_SUITE)  == 1) && ok;
    ok = D_TD_CHECK(count_kind(t, dt::D_TEST_KIND_TEST_MODULE) == 1) && ok;
    ok = D_TD_CHECK(count_kind(t, dt::D_TEST_KIND_TEST_BLOCK)  == 2) && ok;
    ok = D_TD_CHECK(count_kind(t, dt::D_TEST_KIND_UNIT_TEST)   == 3) && ok;
    ok = D_TD_CHECK(count_kind(t, dt::D_TEST_KIND_TEST_FN)     == 3) && ok;

    return ok;
}

// tests_integration_tree_all_pass
//   an all-pass module yields no failing leaves; all_passed() holds.
bool
tests_integration_tree_all_pass()
{
    bool ok = true;
    dt::enriched_tree t = dt::build_enriched_tree("S", "sd", uniform_module("M", 4, true));

    ok = D_TD_CHECK(t.count_passed() == 4) && ok;
    ok = D_TD_CHECK(t.count_failed() == 0) && ok;
    ok = D_TD_CHECK(!t.any_failed())       && ok;   // no failed/error leaves
    // all_passed() means "no failed, pending, OR error node"; the interior
    // suite/module/block/unit nodes stay pending after lowering, so it is
    // (correctly) false even when every leaf passed.
    ok = D_TD_CHECK(!t.all_passed())       && ok;

    return ok;
}

// tests_integration_tree_all_fail
//   an all-fail module yields no passing leaves; any_failed() holds.
bool
tests_integration_tree_all_fail()
{
    bool ok = true;
    dt::enriched_tree t = dt::build_enriched_tree("S", "sd", uniform_module("M", 3, false));

    ok = D_TD_CHECK(t.count_passed() == 0) && ok;
    ok = D_TD_CHECK(t.count_failed() == 3) && ok;
    ok = D_TD_CHECK(t.any_failed())        && ok;
    ok = D_TD_CHECK(!t.all_passed())       && ok;

    return ok;
}

// tests_integration_tree_multi_module
//   the vector entry point lowers every module beneath one suite; counts
// aggregate across modules.
bool
tests_integration_tree_multi_module()
{
    bool ok = true;
    std::vector<dt::module_spec> mods;
    mods.push_back(uniform_module("A", 2, true));    // 2 pass
    mods.push_back(uniform_module("B", 3, false));   // 3 fail
    dt::enriched_tree t = dt::build_enriched_tree("S", "sd", mods);

    ok = D_TD_CHECK(count_kind(t, dt::D_TEST_KIND_TEST_MODULE) == 2) && ok;
    ok = D_TD_CHECK(t.count_passed() == 2) && ok;
    ok = D_TD_CHECK(t.count_failed() == 3) && ok;

    return ok;
}

// tests_integration_drive_report
//   drive_report projects the fixture onto a real report_builder: one module
// band, one unit per BLOCK (the fixture has two), one check per test, with the
// right pass/fail tally.  Read back from the builder's accumulated test_report.
bool
tests_integration_drive_report()
{
    bool ok = true;
    dt::module_spec m = fixture_module();        // M: B1={ok,bad}, B2={ok2}

    dt::report_builder rb;
    rb.set_console(false);                        // inspect the model, not stdout
    dt::drive_report(rb, m);

    const dt::test_report& rep = rb.report();
    ok = D_TD_CHECK(rep.modules.size() == 1) && ok;        // one module band
    if (rep.modules.size() == 1)
    {
        const dt::report_module& mod = rep.modules[0];
        ok = D_TD_CHECK(mod.name == "M")          && ok;
        ok = D_TD_CHECK(mod.units.size() == 2)    && ok;   // one unit per block (B1, B2)
        ok = D_TD_CHECK(mod.total_checks()  == 3) && ok;   // one check per test
        ok = D_TD_CHECK(mod.passed_checks() == 2) && ok;   // ok, ok2
        ok = D_TD_CHECK(mod.failed_checks() == 1) && ok;   // bad
    }

    return ok;
}

// tests_integration_drive_report_events
//   the projection nests module > block > test as report module > unit > check
// in spec order.  uniform_module has one block ("blk") holding both tests, so
// both checks land under a single unit, in order, each carrying the test's
// descriptor and its resolved status.  (Read from the report model - the
// ordered structure IS the "event" sequence.)
bool
tests_integration_drive_report_events()
{
    bool ok = true;
    dt::module_spec m = uniform_module("M", 2, true);   // 1 block "blk", 2 passing tests

    dt::report_builder rb;
    rb.set_console(false);
    dt::drive_report(rb, m);

    const dt::test_report& rep = rb.report();
    ok = D_TD_CHECK(rep.modules.size() == 1) && ok;
    if (rep.modules.size() == 1)
    {
        const dt::report_module& mod = rep.modules[0];
        ok = D_TD_CHECK(mod.name == "M")        && ok;
        ok = D_TD_CHECK(mod.units.size() == 1)  && ok;    // one block -> one unit
        if (mod.units.size() == 1)
        {
            const dt::report_unit& u = mod.units[0];
            ok = D_TD_CHECK(u.name == "blk")        && ok;
            ok = D_TD_CHECK(u.checks.size() == 2)   && ok;   // both tests under the block
            if (u.checks.size() == 2)
            {
                ok = D_TD_CHECK(u.checks[0].description == "d") && ok;
                ok = D_TD_CHECK(u.checks[1].description == "d") && ok;
                ok = D_TD_CHECK(u.checks[0].status == dt::test_status::passed) && ok;
                ok = D_TD_CHECK(u.checks[1].status == dt::test_status::passed) && ok;
            }
        }
    }

    return ok;
}

// tests_integration_run_module_pass
//   run_module returns 0 when every test passes.  (It owns a console-enabled
// builder and prints its report; the exit code is the observable contract.)
bool
tests_integration_run_module_pass()
{
    bool ok = true;

    int code = dt::run_module(uniform_module("M", 3, true), "title");
    ok = D_TD_CHECK(code == 0) && ok;

    return ok;
}

// tests_integration_run_module_fail
//   run_module returns non-zero when any test fails.
bool
tests_integration_run_module_fail()
{
    bool ok = true;

    int code = dt::run_module(fixture_module(), "title", "subtitle");   // has 1 fail
    ok = D_TD_CHECK(code == 1) && ok;

    return ok;
}

// tests_integration_run_suite
//   run_suite lowers a vector of modules under one suite and returns non-zero
// if any module has a failure, and 0 when all pass.
bool
tests_integration_run_suite()
{
    bool ok = true;

    std::vector<dt::module_spec> mods;
    mods.push_back(uniform_module("A", 2, true));
    mods.push_back(uniform_module("B", 1, false));   // forces failure
    ok = D_TD_CHECK(dt::run_suite("S", "sd", mods, "title") == 1) && ok;

    // an all-pass suite returns 0
    std::vector<dt::module_spec> pass;
    pass.push_back(uniform_module("A", 2, true));
    ok = D_TD_CHECK(dt::run_suite("S", "sd", pass, "title") == 0) && ok;

    return ok;
}

// tests_integration_describe_summary
//   the console views run without crashing and reflect the evaluated tree.
bool
tests_integration_describe_summary()
{
    bool ok = true;
    dt::enriched_tree t = dt::build_enriched_tree("S", "sd", fixture_module());

    // exercised for non-crash; their content mirrors the tree counters
    dt::describe_tree(t);
    dt::tree_summary(t);

    ok = D_TD_CHECK(t.count_passed() == 2 && t.count_failed() == 1) && ok;

    return ok;
}


NS_END  // testing
NS_END  // djinterp
