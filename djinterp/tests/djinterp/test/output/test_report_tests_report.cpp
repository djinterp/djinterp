/*******************************************************************************
* djinterp [test]                                   test_report_tests_report.cpp
*
*   Section V of the test_report suite: THE RUN.
*
*   The top of the fold.  test_report rolls the modules up into the comprehensive
* figures the summary page prints, and produces the process exit code.
*
*   THE THING THIS SECTION IS REALLY ABOUT is what `empty` means, because the two
* levels of the model answer that differently and nobody wrote it down:
*
*       report_module::failed_units()    excludes empty units   (verdict != empty)
*       test_report::failed_modules()    counts  !passed()       -- no such guard
*
*   `empty` is not a pass, so an empty MODULE lands in failed_modules() while an
* empty UNIT lands in neither of its module's counts.  The unit counts therefore do
* not partition their total and the module counts always do - the same word, two
* rules, one level apart.
*
*   Its sharpest form is a report of nothing but empty modules, which simultaneously
* reports verdict EMPTY, N failed modules, and exit code 1.  Pinned below.
*
*
* path:      /tests/djinterp/test/output/test_report_tests_report.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.06
*                                                            revised: 2026.09.21
*******************************************************************************/

// std
#include <cstddef>
#include <string>
#include <vector>
// djinterp
#include "test_report_tests.hpp"


NS_DJINTERP
NS_TESTING


// tests_report_report_defaults_to_empty_metadata
bool
tests_report_report_defaults_to_empty_metadata()
{
    const dt::test_report _r;

    D_RT_CHECK(_r.title.empty());
    D_RT_CHECK(_r.subtitle.empty());
    D_RT_CHECK(_r.author.empty());
    D_RT_CHECK(_r.description.empty());
    D_RT_CHECK(_r.notes.empty());
    D_RT_CHECK(_r.modules.empty());

    D_RT_CHECK(_r.total_modules() == 0u);
    D_RT_CHECK(_r.total_units()   == 0u);
    D_RT_CHECK(_r.total_checks()  == 0u);

    D_RT_CHECK(_r.verdict() == dt::report_verdict::empty);
    D_RT_CHECK(!_r.passed());

    //   a report that ran nothing exits NON-ZERO.  Deliberate (nothing was proven)
    // and worth knowing: an empty run is a red build, not a vacuous green one.
    D_RT_CHECK(_r.exit_code() == 1);

    return true;
}


// tests_report_report_carries_its_metadata_verbatim
//   the five metadata strings are pure payload - the model never inspects, trims,
// defaults, or sanitizes them.  (The PDF cover supplies its own "Test Report"
// fallback for an empty title; that is the RENDERER's policy, not the model's.)
bool
tests_report_report_carries_its_metadata_verbatim()
{
    dt::test_report _r;

    _r.title       = "  The Run  ";        // leading/trailing space preserved
    _r.subtitle    = "nightly";
    _r.author      = "teer";
    _r.description = "a description";
    _r.notes       = "line one\nline two";

    D_RT_CHECK(_r.title       == "  The Run  ");
    D_RT_CHECK(_r.subtitle    == "nightly");
    D_RT_CHECK(_r.author      == "teer");
    D_RT_CHECK(_r.description == "a description");
    D_RT_CHECK(_r.notes       == "line one\nline two");

    // adding modules does not disturb them
    _r.module("m");

    D_RT_CHECK(_r.title == "  The Run  ");
    D_RT_CHECK(_r.notes == "line one\nline two");

    return true;
}


// tests_report_report_add_module_and_the_module_convenience
//   module(name, desc) is add_module(report_module(name, desc)); both return the
// stored module for population.
bool
tests_report_report_add_module_and_the_module_convenience()
{
    dt::test_report _r;

    dt::report_module& _a = _r.add_module(dt::report_module("a", "the a module"));

    D_RT_CHECK(_r.total_modules() == 1u);
    D_RT_CHECK(&_a == &_r.modules[0]);
    D_RT_CHECK(_a.name == "a");
    D_RT_CHECK(_a.description == "the a module");

    // the convenience form, with an explicit description
    dt::report_module& _b = _r.module("b", "the b module");

    D_RT_CHECK(_r.total_modules() == 2u);
    D_RT_CHECK(_b.name == "b");
    D_RT_CHECK(_b.description == "the b module");

    // ...and with the description defaulted
    dt::report_module& _c = _r.module("c");

    D_RT_CHECK(_c.name == "c");
    D_RT_CHECK(_c.description.empty());

    // the returned reference really populates the report
    _c.add_result_unit("t", true);

    D_RT_CHECK(_r.modules[2].total_units() == 1u);
    D_RT_CHECK(_r.total_units() == 1u);

    return true;
}


// tests_report_report_add_module_invalidates_the_previous_reference
//   the same vector-reallocation hazard as report_module::add_unit, one level up:
// add_module() hands back a reference into `modules`, and the next add_module()
// may move the buffer out from under it.
//
//   It is worse here, because the natural way to build a report is exactly the
// pattern that breaks:
//
//       auto& a = report.module("a");
//       auto& b = report.module("b");     // <- `a` may now dangle
//       a.add_result_unit("t", true);     // <- USE AFTER FREE
//
//   As in the module section, the stale reference is NOT dereferenced here; the
// reallocation itself is what is shown.
bool
tests_report_report_add_module_invalidates_the_previous_reference()
{
    dt::test_report _r;

    const dt::report_module* _first = &_r.module("a");
    const dt::report_module* _buf_a = _r.modules.data();

    D_RT_CHECK(_first == _buf_a);

    std::size_t _adds = 0;

    while ( (_r.modules.data() == _buf_a) &&
            (_adds < 64u) )
    {
        _r.module("filler");
        ++_adds;
    }

    // the buffer moved; _first is now stale and must not be dereferenced
    D_RT_CHECK(_r.modules.data() != _buf_a);
    D_RT_CHECK(_first != _r.modules.data());

    // the report is intact - only the caller's saved reference was lost
    D_RT_CHECK(_r.total_modules() == _adds + 1u);
    D_RT_CHECK(_r.modules[0].name == "a");

    // the safe idiom: finish with one module before starting the next, or re-index
    _r.modules[0].add_result_unit("late", true);

    D_RT_CHECK(_r.modules[0].total_units() == 1u);

    return true;
}


// tests_report_report_module_tally
bool
tests_report_report_module_tally()
{
    const dt::test_report _r = report_of(
    {
        module_of("a", { unit_of("u", { dt::test_status::passed }) }),
        module_of("b", { unit_of("u", { dt::test_status::passed }) }),
        module_of("c", { unit_of("u", { dt::test_status::failed }) })
    });

    D_RT_CHECK(_r.total_modules()  == 3u);
    D_RT_CHECK(_r.passed_modules() == 2u);
    D_RT_CHECK(_r.failed_modules() == 1u);

    return true;
}


// tests_report_report_failed_modules_counts_an_empty_module_as_failed
//   A FINDING, pinned as fact.
//
//   failed_modules() is `if (!modules[i].passed()) ++n;` - with NO guard for the
// empty case, unlike its counterpart failed_units() one level down.  An empty
// module's verdict is `empty`, and empty is not a pass, so an empty module is
// counted as a FAILED module.
//
//   A module that was declared but never populated - a stub, a filtered-out group,
// a module whose units all got skipped at registration - is therefore reported as a
// failure.  The same emptiness at the UNIT level is reported as nothing at all.
bool
tests_report_report_failed_modules_counts_an_empty_module_as_failed()
{
    dt::test_report _r;

    _r.module("stub");            // declared, never populated -> verdict empty

    D_RT_CHECK(_r.total_modules() == 1u);
    D_RT_CHECK(_r.modules[0].verdict() == dt::report_verdict::empty);
    D_RT_CHECK(!_r.modules[0].passed());

    D_RT_CHECK(_r.passed_modules() == 0u);
    D_RT_CHECK(_r.failed_modules() == 1u);      // <- the empty module, counted as FAILED

    //   ...while the equivalent emptiness one level down is counted as NEITHER
    const dt::report_module _m = module_of("m", { dt::report_unit("empty_unit") });

    D_RT_CHECK(_m.total_units()  == 1u);
    D_RT_CHECK(_m.passed_units() == 0u);
    D_RT_CHECK(_m.failed_units() == 0u);        // <- the empty unit, counted as NOTHING

    //   the same word, two rules, one level apart

    return true;
}


// tests_report_report_module_counts_partition_but_unit_counts_do_not
//   the structural consequence of the finding above, stated as an invariant a
// renderer would want and only half gets:
//
//       passed_modules() + failed_modules()  ==  total_modules()      ALWAYS
//       passed_units()   + failed_units()    ==  total_units()        NOT always
//
//   because failed_modules() is a strict `!passed()` complement while failed_units()
// carves `empty` out of both buckets.  A summary page that prints both lines in the
// same format is printing two figures that do not mean the same thing.
bool
tests_report_report_module_counts_partition_but_unit_counts_do_not()
{
    //   a report with an empty module, an empty unit, and one real passing unit -
    // every degenerate shape at once
    dt::test_report _r;

    _r.add_module(dt::report_module("stub", ""));                  // empty MODULE
    _r.add_module(module_of("real",
    {
        unit_of("k", { dt::test_status::passed }),                 // a real unit
        dt::report_unit("empty_unit")                              // empty UNIT
    }));

    // -- modules: a strict partition ------------------------------------------
    D_RT_CHECK(_r.total_modules()  == 2u);
    D_RT_CHECK(_r.passed_modules() == 1u);
    D_RT_CHECK(_r.failed_modules() == 1u);       // the stub
    D_RT_CHECK(_r.passed_modules() + _r.failed_modules() == _r.total_modules());

    // -- units: NOT a partition ------------------------------------------------
    D_RT_CHECK(_r.total_units()  == 2u);         // the real one + the empty one
    D_RT_CHECK(_r.passed_units() == 1u);
    D_RT_CHECK(_r.failed_units() == 0u);         // the empty unit is in neither
    D_RT_CHECK(_r.passed_units() + _r.failed_units() != _r.total_units());

    //   so the summary reads "1 passed, 1 failed of 2 modules" (adds up) and
    // "1 passed, 0 failed of 2 unit tests" (does not) - from the same object

    return true;
}


// tests_report_report_unit_tally_sums_across_modules
bool
tests_report_report_unit_tally_sums_across_modules()
{
    const dt::test_report _r = report_of(
    {
        module_of("a",
        {
            unit_of("1", { dt::test_status::passed }),
            unit_of("2", { dt::test_status::failed })
        }),
        module_of("b",
        {
            unit_of("3", { dt::test_status::passed }),
            unit_with_verdict("4", dt::report_verdict::pending),
            dt::report_unit("5")                                  // empty
        })
    });

    D_RT_CHECK(_r.total_units()  == 5u);
    D_RT_CHECK(_r.passed_units() == 2u);
    D_RT_CHECK(_r.failed_units() == 2u);      // the failure + the PENDING one

    // an empty report sums to nothing
    const dt::test_report _e;

    D_RT_CHECK(_e.total_units()  == 0u);
    D_RT_CHECK(_e.passed_units() == 0u);
    D_RT_CHECK(_e.failed_units() == 0u);

    return true;
}


// tests_report_report_check_tally_sums_across_modules
//   the comprehensive assertion figures - the ones the summary's "X/Y assertions"
// line is built from.
bool
tests_report_report_check_tally_sums_across_modules()
{
    const dt::test_report _r = report_of(
    {
        module_of("a",
        {
            unit_of("1", { dt::test_status::passed,
                           dt::test_status::passed,
                           dt::test_status::failed })
        }),
        module_of("b",
        {
            unit_of("2", { dt::test_status::error,
                           dt::test_status::skipped,
                           dt::test_status::pending }),
            dt::report_unit("empty")
        })
    });

    D_RT_CHECK(_r.total_checks()  == 6u);
    D_RT_CHECK(_r.passed_checks() == 2u);
    D_RT_CHECK(_r.failed_checks() == 2u);      // the failure + the error

    // the skip and the pending are in the total and in neither bucket
    D_RT_CHECK(_r.passed_checks() + _r.failed_checks() == 4u);

    return true;
}


// tests_report_report_verdict_folds_the_module_verdicts
bool
tests_report_report_verdict_folds_the_module_verdicts()
{
    // all green
    D_RT_CHECK(report_of(
    {
        module_of("a", { unit_of("u", { dt::test_status::passed }) }),
        module_of("b", { unit_of("u", { dt::test_status::passed }) })
    }).verdict() == dt::report_verdict::passed);

    // one red module sinks the run
    D_RT_CHECK(report_of(
    {
        module_of("a", { unit_of("u", { dt::test_status::passed }) }),
        module_of("b", { unit_of("u", { dt::test_status::failed }) })
    }).verdict() == dt::report_verdict::failed);

    // a pending module blocks the pass without failing it
    D_RT_CHECK(report_of(
    {
        module_of("a", { unit_of("u", { dt::test_status::passed }) }),
        module_of("b", { unit_of("u", { dt::test_status::pending }) })
    }).verdict() == dt::report_verdict::pending);

    // failure outranks pending, at the top too
    D_RT_CHECK(report_of(
    {
        module_of("a", { unit_of("u", { dt::test_status::pending }) }),
        module_of("b", { unit_of("u", { dt::test_status::failed }) })
    }).verdict() == dt::report_verdict::failed);

    return true;
}


// tests_report_report_verdict_ignores_empty_modules
//   the fold drops `empty` modules through its `default:` arm, exactly as the
// module fold drops empty units.  So an empty module cannot spoil an otherwise
// clean run's VERDICT...
bool
tests_report_report_verdict_ignores_empty_modules()
{
    dt::test_report _r;

    _r.module("stub");                                         // empty
    _r.add_module(module_of("real", { unit_of("u", { dt::test_status::passed }) }));

    D_RT_CHECK(_r.verdict() == dt::report_verdict::passed);    // still green
    D_RT_CHECK(_r.passed());
    D_RT_CHECK(_r.exit_code() == 0);

    //   ...even though that same empty module IS counted in failed_modules().  The
    // verdict says the run passed; the module tally says one module failed.
    D_RT_CHECK(_r.failed_modules() == 1u);

    return true;
}


// tests_report_report_of_only_empty_modules_contradicts_itself
//   THE FINDING, in its sharpest form.
//
//   Take a run of nothing but declared-and-never-populated modules.  Three of the
// report's own accessors then describe it three incompatible ways:
//
//       verdict()         ==  EMPTY      (the fold ignored the empty modules)
//       failed_modules()  ==  N > 0      (!passed() counted every one of them)
//       exit_code()       ==  1          (because empty is not a pass)
//
//   So the run "observed nothing", "had N failed modules", and "failed" - all at
// once, from one object.  A summary page reading all three prints a report that
// says EMPTY at the top and "0 passed, 3 failed" underneath.
//
//   The exit code is defensible on its own (nothing was proven, so do not go
// green).  The contradiction is between verdict() and failed_modules(), which have
// simply made different decisions about what emptiness means - and only one of them
// matches the unit level.
bool
tests_report_report_of_only_empty_modules_contradicts_itself()
{
    dt::test_report _r;

    _r.module("stub_a");
    _r.module("stub_b");
    _r.module("stub_c");

    // 1. the run observed nothing
    D_RT_CHECK(_r.verdict() == dt::report_verdict::empty);
    D_RT_CHECK(std::string(dt::report_verdict_word(_r.verdict())) == "EMPTY");

    // 2. ...and yet every module is counted as failed
    D_RT_CHECK(_r.total_modules()  == 3u);
    D_RT_CHECK(_r.passed_modules() == 0u);
    D_RT_CHECK(_r.failed_modules() == 3u);

    // 3. ...and the run exits as a failure
    D_RT_CHECK(!_r.passed());
    D_RT_CHECK(_r.exit_code() == 1);

    //   there are no units and no checks behind any of it
    D_RT_CHECK(_r.total_units()  == 0u);
    D_RT_CHECK(_r.total_checks() == 0u);
    D_RT_CHECK(_r.failed_checks() == 0u);

    //   the same emptiness, one level down, is counted as nothing at all
    const dt::report_module _m = module_of("m",
    {
        dt::report_unit("e1"),
        dt::report_unit("e2"),
        dt::report_unit("e3")
    });

    D_RT_CHECK(_m.verdict() == dt::report_verdict::empty);   // agrees with the run...
    D_RT_CHECK(_m.failed_units() == 0u);                     // ...but counts nothing failed

    return true;
}


// tests_report_report_exit_code_is_zero_only_on_a_clean_pass
//   the process result.  Every non-pass verdict - failed, pending, AND empty -
// exits non-zero, which is the right conservative default for CI.
bool
tests_report_report_exit_code_is_zero_only_on_a_clean_pass()
{
    // a clean run
    const dt::test_report _pass = report_of(
        { module_of("a", { unit_of("u", { dt::test_status::passed }) }) });

    D_RT_CHECK(_pass.verdict() == dt::report_verdict::passed);
    D_RT_CHECK(_pass.passed());
    D_RT_CHECK(_pass.exit_code() == 0);

    // a failing run
    const dt::test_report _fail = report_of(
        { module_of("a", { unit_of("u", { dt::test_status::failed }) }) });

    D_RT_CHECK(_fail.exit_code() == 1);

    // a pending run: not failed, but not green either
    const dt::test_report _pend = report_of(
        { module_of("a", { unit_of("u", { dt::test_status::pending }) }) });

    D_RT_CHECK(_pend.verdict() == dt::report_verdict::pending);
    D_RT_CHECK(_pend.exit_code() == 1);

    // an empty run
    const dt::test_report _empty;

    D_RT_CHECK(_empty.exit_code() == 1);

    //   ...and the all-skipped run, which DOES exit zero - the fold's doing
    const dt::test_report _skipped = report_of(
        { module_of("a", { unit_of("u", { dt::test_status::skipped,
                                          dt::test_status::skipped }) }) });

    D_RT_CHECK(_skipped.verdict() == dt::report_verdict::passed);
    D_RT_CHECK(_skipped.exit_code() == 0);        // GREEN, having run nothing
    D_RT_CHECK(_skipped.passed_checks() == 0u);
    D_RT_CHECK(_skipped.total_checks()  == 2u);

    return true;
}


// tests_report_report_tallies_agree_end_to_end
//   the whole model, on one realistic report: every figure the summary page prints,
// checked against the shape it was built from.  This is the integration test the
// renderers implicitly rely on - if these numbers are wrong, all four output
// formats are wrong together and none of them will disagree.
bool
tests_report_report_tallies_agree_end_to_end()
{
    const dt::test_report _r = report_of(
    {
        //  module a: 2 units, 4 checks (2 passed, 1 failed, 1 skipped) -> FAILED
        module_of("a",
        {
            unit_of("a1", { dt::test_status::passed,
                            dt::test_status::passed }),           // passed
            unit_of("a2", { dt::test_status::failed,
                            dt::test_status::skipped })           // failed
        }),
        //  module b: 2 units, 2 checks (2 passed) -> PASSED
        module_of("b",
        {
            unit_of("b1", { dt::test_status::passed }),           // passed
            unit_of("b2", { dt::test_status::passed })            // passed
        })
    });

    // modules
    D_RT_CHECK(_r.total_modules()  == 2u);
    D_RT_CHECK(_r.passed_modules() == 1u);       // b
    D_RT_CHECK(_r.failed_modules() == 1u);       // a

    // units
    D_RT_CHECK(_r.total_units()  == 4u);
    D_RT_CHECK(_r.passed_units() == 3u);         // a1, b1, b2
    D_RT_CHECK(_r.failed_units() == 1u);         // a2

    // assertions
    D_RT_CHECK(_r.total_checks()  == 6u);
    D_RT_CHECK(_r.passed_checks() == 4u);
    D_RT_CHECK(_r.failed_checks() == 1u);        // the skip is in neither bucket

    // the verdict, and the figures a summary would print from it
    D_RT_CHECK(_r.verdict() == dt::report_verdict::failed);
    D_RT_CHECK(_r.exit_code() == 1);

    D_RT_CHECK(dt::report_ratio(_r.passed_checks(), _r.total_checks()) == "4/6");
    D_RT_CHECK(dt::report_pass_rate(_r.passed_checks(), _r.total_checks()) == "66.67%");
    D_RT_CHECK(dt::report_ratio(_r.passed_units(), _r.total_units()) == "3/4");
    D_RT_CHECK(dt::report_ratio(_r.passed_modules(), _r.total_modules()) == "1/2");

    // the per-module figures the module table would print
    D_RT_CHECK(_r.modules[0].total_checks()  == 4u);
    D_RT_CHECK(_r.modules[0].passed_checks() == 2u);
    D_RT_CHECK(_r.modules[0].verdict() == dt::report_verdict::failed);
    D_RT_CHECK(_r.modules[1].total_checks()  == 2u);
    D_RT_CHECK(_r.modules[1].passed_checks() == 2u);
    D_RT_CHECK(_r.modules[1].verdict() == dt::report_verdict::passed);

    return true;
}


NS_END   // testing
NS_END   // djinterp
