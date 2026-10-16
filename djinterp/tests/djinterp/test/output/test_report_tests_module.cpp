/*******************************************************************************
* djinterp [test]                                   test_report_tests_module.cpp
*
*   Section IV of the test_report suite: THE MODULE.
*
*   A module folds UNITS into a verdict, and keeps two independent tallies over
* them: a unit-test tally (over unit verdicts) and an assertion tally (over every
* check in every unit).  The renderers read both, which is why they both matter.
*
*   THE INTERESTING PART IS `failed_units()`, and it is doing two things nobody
* would guess from its name:
*
*       if ( (units[i].verdict == failed) ||
*            (!units[i].passed() && (units[i].verdict != empty)) )
*
*   The second clause is TRUE for failed and for PENDING.  So (a) pending units are
* counted as failed, and (b) the first clause is entirely subsumed by the second
* and can never fire on its own - it is dead.  Meanwhile `empty` units are excluded
* from BOTH tallies, so passed_units() + failed_units() does not equal
* total_units().  All three properties are pinned below.
*
*
* path:      /tests/djinterp/test/output/test_report_tests_module.cpp
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


// tests_report_module_defaults_and_named_construction
bool
tests_report_module_defaults_and_named_construction()
{
    const dt::report_module _d;

    D_RT_CHECK(_d.name.empty());
    D_RT_CHECK(_d.description.empty());
    D_RT_CHECK(_d.units.empty());
    D_RT_CHECK(_d.total_units()  == 0u);
    D_RT_CHECK(_d.total_checks() == 0u);
    D_RT_CHECK(_d.verdict() == dt::report_verdict::empty);
    D_RT_CHECK(!_d.passed());

    const dt::report_module _n("core", "the core module");

    D_RT_CHECK(_n.name == "core");
    D_RT_CHECK(_n.description == "the core module");
    D_RT_CHECK(_n.units.empty());

    return true;
}


// tests_report_module_add_unit_appends_and_returns_the_stored_unit
//   add_unit returns a reference to the STORED copy, not to the argument - so
// mutating the return value really does change the module.
bool
tests_report_module_add_unit_appends_and_returns_the_stored_unit()
{
    dt::report_module _m("m", "");

    dt::report_unit& _stored = _m.add_unit(dt::report_unit("alpha"));

    D_RT_CHECK(_m.total_units() == 1u);
    D_RT_CHECK(&_stored == &_m.units[0]);        // the STORED copy, not a temporary

    // ...so the caller may keep populating it, as the header intends
    _stored.add_check("a", dt::test_status::passed);

    D_RT_CHECK(_m.units[0].total_checks() == 1u);
    D_RT_CHECK(_m.units[0].verdict == dt::report_verdict::passed);
    D_RT_CHECK(_m.total_checks() == 1u);

    return true;
}


// tests_report_module_add_unit_invalidates_the_previous_reference
//   A HAZARD, pinned as fact.
//
//   add_unit() push_backs into a std::vector and hands back units.back().  A
// vector REALLOCATES when it outgrows its capacity, and every outstanding
// reference into it dangles the moment it does.  So the pattern the header's own
// comment invites -
//
//       "returns a reference to the stored copy so the caller may keep adding
//        checks to it"
//
//   - is only safe until the NEXT add_unit().  This is fine:
//
//       auto& a = m.add_unit(report_unit("a"));
//       a.add_check(...);                        // done with `a`
//       auto& b = m.add_unit(report_unit("b"));  // `a` may now dangle
//
//   and this is undefined behaviour:
//
//       auto& a = m.add_unit(report_unit("a"));
//       auto& b = m.add_unit(report_unit("b"));  // <- may reallocate
//       a.add_check(...);                        // <- USE AFTER FREE
//
//   Nothing in the signature or the comment warns of it, and it will work by luck
// whenever the vector happens to have spare capacity - which is exactly what makes
// it worth a test.
//
//   This test does NOT dereference the stale reference (that would be the very UB
// it is documenting).  It shows the reallocation happening - the buffer moves -
// which is the necessary and sufficient condition for the dangle.
bool
tests_report_module_add_unit_invalidates_the_previous_reference()
{
    dt::report_module _m("m", "");

    const dt::report_unit* _first = &_m.add_unit(dt::report_unit("a"));
    const dt::report_unit* _buf_a = _m.units.data();

    D_RT_CHECK(_first == _buf_a);          // the reference points into the buffer

    //   grow the module until the vector is forced to reallocate.  A fresh vector
    // has capacity 1 after one push_back on every implementation we care about, so
    // the second add_unit is normally enough - but loop, so the test is about the
    // GUARANTEE rather than about libstdc++'s growth policy.
    std::size_t _adds = 0;

    while ( (_m.units.data() == _buf_a) &&
            (_adds < 64u) )
    {
        _m.add_unit(dt::report_unit("filler"));
        ++_adds;
    }

    const dt::report_unit* _buf_b = _m.units.data();

    //   the buffer moved.  _first now points into freed memory - it is NOT
    // dereferenced here, and must not be.
    D_RT_CHECK(_buf_b != _buf_a);
    D_RT_CHECK(_first != _m.units.data());      // the stale reference is stale
    D_RT_CHECK(_adds <= 64u);

    //   the module itself is perfectly fine - the units are all there.  Only the
    // caller's saved reference was lost.
    D_RT_CHECK(_m.total_units() == _adds + 1u);
    D_RT_CHECK(_m.units[0].name == "a");

    //   the safe idiom: re-index rather than hold a reference across an add
    _m.units[0].add_check("late", dt::test_status::passed);

    D_RT_CHECK(_m.units[0].total_checks() == 1u);

    return true;
}


// tests_report_module_add_result_unit_names_the_check_after_the_unit
//   the leaf-runner convenience.  Note it uses _name TWICE - once as the unit's
// name and once as its single check's description - so a rendered line reads
// "[PASS] my_test" under a unit also called "my_test".  That is the intended shape
// for a runner whose tests are themselves the leaves, but it is worth pinning:
// the check description is NOT independently settable through this entry point.
bool
tests_report_module_add_result_unit_names_the_check_after_the_unit()
{
    dt::report_module _m("m", "");

    dt::report_unit& _u = _m.add_result_unit("my_test", true);

    D_RT_CHECK(_m.total_units() == 1u);
    D_RT_CHECK(_u.name == "my_test");
    D_RT_CHECK(_u.total_checks() == 1u);
    D_RT_CHECK(_u.checks[0].description == "my_test");     // the name, reused
    D_RT_CHECK(_u.checks[0].status == dt::test_status::passed);
    D_RT_CHECK(_u.verdict == dt::report_verdict::passed);

    // false -> a failing unit with a failing check
    dt::report_unit& _f = _m.add_result_unit("bad_test", false);

    D_RT_CHECK(_f.checks[0].status == dt::test_status::failed);
    D_RT_CHECK(_f.verdict == dt::report_verdict::failed);

    D_RT_CHECK(_m.total_units()   == 2u);
    D_RT_CHECK(_m.total_checks()  == 2u);
    D_RT_CHECK(_m.passed_units()  == 1u);
    D_RT_CHECK(_m.failed_units()  == 1u);

    return true;
}


// tests_report_module_unit_tally
//   the ordinary case, where every unit is decisively passed or failed and the two
// counts partition the total.
bool
tests_report_module_unit_tally()
{
    const dt::report_module _m = module_of("m",
    {
        unit_of("a", { dt::test_status::passed }),
        unit_of("b", { dt::test_status::passed }),
        unit_of("c", { dt::test_status::failed })
    });

    D_RT_CHECK(_m.total_units()  == 3u);
    D_RT_CHECK(_m.passed_units() == 2u);
    D_RT_CHECK(_m.failed_units() == 1u);

    // with no empty and no pending units, the counts do partition
    D_RT_CHECK(_m.passed_units() + _m.failed_units() == _m.total_units());

    return true;
}


// tests_report_module_failed_units_counts_pending_as_failed
//   A FINDING, pinned as fact.
//
//   failed_units()'s second clause is `!passed() && verdict != empty`, which is
// true for `failed` AND for `pending`.  So an unfinished unit is counted as a
// FAILED one.
//
//   That is defensible as a conservative reading (an unfinished unit certainly did
// not pass, and the header is explicit elsewhere that a banner must never claim
// success on an unfinished run) - but the FIGURE is labelled "failed", and a
// renderer printing "3 failed" over three merely-pending units is telling the
// reader something untrue.  The verdict-level vocabulary distinguishes failed from
// pending; this tally does not.
//
//   A second, purely mechanical consequence: because the second clause already
// covers `verdict == failed`, the FIRST clause is entirely subsumed and can never
// fire on its own.  It is dead code.
bool
tests_report_module_failed_units_counts_pending_as_failed()
{
    const dt::report_module _m = module_of("m",
    {
        unit_with_verdict("p1", dt::report_verdict::pending),
        unit_with_verdict("p2", dt::report_verdict::pending)
    });

    D_RT_CHECK(_m.total_units()  == 2u);
    D_RT_CHECK(_m.passed_units() == 0u);
    D_RT_CHECK(_m.failed_units() == 2u);      // <- PENDING units, counted as failed

    //   ...even though NOTHING failed: the module's own verdict correctly says
    // pending, not failed.  The verdict and the tally disagree about the same units.
    D_RT_CHECK(_m.verdict() == dt::report_verdict::pending);
    D_RT_CHECK(_m.failed_checks() == 0u);     // and no check failed either

    //   the dead first clause: every unit the first clause would catch
    // (verdict == failed) is ALSO caught by the second (!passed && != empty), so
    // removing the first would change nothing.  Demonstrated by showing the second
    // clause alone accounts for every counted unit.
    const dt::report_module _mixed = module_of("x",
    {
        unit_with_verdict("f", dt::report_verdict::failed),    // caught by BOTH
        unit_with_verdict("p", dt::report_verdict::pending),   // caught by the 2nd only
        unit_with_verdict("k", dt::report_verdict::passed),    // caught by neither
        unit_with_verdict("e", dt::report_verdict::empty)      // caught by neither
    });

    D_RT_CHECK(_mixed.failed_units() == 2u);          // the failed one + the pending one

    std::size_t _second_clause_only = 0;
    std::size_t _i                  = 0;

    for (_i = 0; _i < _mixed.units.size(); ++_i)
    {
        if ( (!_mixed.units[_i].passed()) &&
             (_mixed.units[_i].verdict != dt::report_verdict::empty) )
        {
            ++_second_clause_only;
        }
    }

    //   the second clause alone gives the same answer -> the first is redundant
    D_RT_CHECK(_second_clause_only == _mixed.failed_units());

    return true;
}


// tests_report_module_failed_units_excludes_empty_units
//   the other half of failed_units()'s condition: an EMPTY unit is explicitly
// excluded (`verdict != empty`).  So it counts as neither passed nor failed, and
// the two tallies STOP PARTITIONING the total.
//
//   Remember this, because the report level does the opposite - see
// tests_report_report_module_counts_partition_but_unit_counts_do_not.
bool
tests_report_module_failed_units_excludes_empty_units()
{
    const dt::report_module _m = module_of("m",
    {
        unit_of("k", { dt::test_status::passed }),   // passed
        dt::report_unit("e1"),                       // empty - no checks
        dt::report_unit("e2")                        // empty - no checks
    });

    D_RT_CHECK(_m.total_units()  == 3u);
    D_RT_CHECK(_m.passed_units() == 1u);
    D_RT_CHECK(_m.failed_units() == 0u);      // the empties are NOT failures

    //   ...so the counts do not add up to the total.  Two units are simply
    // unaccounted for in both figures.
    D_RT_CHECK(_m.passed_units() + _m.failed_units() == 1u);
    D_RT_CHECK(_m.passed_units() + _m.failed_units() != _m.total_units());

    //   a renderer printing "1 passed, 0 failed of 3 unit tests" is reading this
    // object correctly and telling the reader nothing about the other two

    return true;
}


// tests_report_module_check_tallies_sum_over_units
//   the assertion tally is a straight sum across the units - including across
// EMPTY ones, which contribute zero.
bool
tests_report_module_check_tallies_sum_over_units()
{
    const dt::report_module _m = module_of("m",
    {
        unit_of("a", { dt::test_status::passed,
                       dt::test_status::passed,
                       dt::test_status::failed }),
        unit_of("b", { dt::test_status::error,
                       dt::test_status::skipped }),
        dt::report_unit("empty")
    });

    D_RT_CHECK(_m.total_checks()  == 5u);      // 3 + 2 + 0
    D_RT_CHECK(_m.passed_checks() == 2u);
    D_RT_CHECK(_m.failed_checks() == 2u);      // the failure + the error

    // the skipped check is in the total and in neither bucket
    D_RT_CHECK(_m.passed_checks() + _m.failed_checks() == 4u);

    // an empty module has an empty tally
    const dt::report_module _e("e", "");

    D_RT_CHECK(_e.total_checks()  == 0u);
    D_RT_CHECK(_e.passed_checks() == 0u);
    D_RT_CHECK(_e.failed_checks() == 0u);

    return true;
}


// tests_report_module_verdict_folds_the_unit_verdicts
//   the module's conclusion is derived from its units' VERDICTS - not from their
// checks.  A unit whose verdict was pinned carries that pinned value up.
bool
tests_report_module_verdict_folds_the_unit_verdicts()
{
    D_RT_CHECK(module_of("a", { unit_with_verdict("u", dt::report_verdict::passed) })
                   .verdict() == dt::report_verdict::passed);

    D_RT_CHECK(module_of("b", { unit_with_verdict("u", dt::report_verdict::failed) })
                   .verdict() == dt::report_verdict::failed);

    D_RT_CHECK(module_of("c", { unit_with_verdict("u", dt::report_verdict::pending) })
                   .verdict() == dt::report_verdict::pending);

    //   the fold reads the unit's VERDICT, so a unit pinned green over red checks
    // carries green upward - the module never looks at the checks
    dt::report_unit _lying("l");

    _lying.set_verdict(dt::report_verdict::passed);
    _lying.add_check("boom", dt::test_status::failed);

    const dt::report_module _m = module_of("m", { _lying });

    D_RT_CHECK(_m.verdict() == dt::report_verdict::passed);   // green...
    D_RT_CHECK(_m.passed());
    D_RT_CHECK(_m.failed_checks() == 1u);                     // ...over a failed check

    return true;
}


// tests_report_module_verdict_ignores_empty_units
//   the fold's switch has no case for `empty` - it falls into `default:` and is
// dropped.  So empty units contribute NOTHING to the module verdict: a module of
// only empty units is itself empty, and one empty unit among passing ones is
// invisible.
bool
tests_report_module_verdict_ignores_empty_units()
{
    // only empty units -> the module observed nothing
    const dt::report_module _all_empty = module_of("m",
    {
        dt::report_unit("e1"),
        dt::report_unit("e2")
    });

    D_RT_CHECK(_all_empty.total_units() == 2u);              // the units exist...
    D_RT_CHECK(_all_empty.verdict() == dt::report_verdict::empty);   // ...but count for nothing
    D_RT_CHECK(!_all_empty.passed());

    // an empty unit alongside a passing one does not spoil the pass
    const dt::report_module _mixed = module_of("m",
    {
        unit_of("k", { dt::test_status::passed }),
        dt::report_unit("e")
    });

    D_RT_CHECK(_mixed.verdict() == dt::report_verdict::passed);
    D_RT_CHECK(_mixed.passed());

    return true;
}


// tests_report_module_verdict_precedence
//   the same precedence the fold applies everywhere: any failure sinks it; failing
// outranks pending; pending outranks passing.
bool
tests_report_module_verdict_precedence()
{
    // a failure among passes
    D_RT_CHECK(module_of("a",
    {
        unit_with_verdict("1", dt::report_verdict::passed),
        unit_with_verdict("2", dt::report_verdict::failed)
    }).verdict() == dt::report_verdict::failed);

    // a pending among passes
    D_RT_CHECK(module_of("b",
    {
        unit_with_verdict("1", dt::report_verdict::passed),
        unit_with_verdict("2", dt::report_verdict::pending)
    }).verdict() == dt::report_verdict::pending);

    // a failure outranks a pending
    D_RT_CHECK(module_of("c",
    {
        unit_with_verdict("1", dt::report_verdict::pending),
        unit_with_verdict("2", dt::report_verdict::failed)
    }).verdict() == dt::report_verdict::failed);

    // all three, plus an empty for good measure
    D_RT_CHECK(module_of("d",
    {
        unit_with_verdict("1", dt::report_verdict::passed),
        unit_with_verdict("2", dt::report_verdict::pending),
        unit_with_verdict("3", dt::report_verdict::failed),
        unit_with_verdict("4", dt::report_verdict::empty)
    }).verdict() == dt::report_verdict::failed);

    return true;
}


// tests_report_module_of_no_units_is_empty_and_not_passed
//   the degenerate module.  It is `empty`, and - the part that matters upward -
// empty is NOT a pass, which is what makes an empty module count as a FAILED module
// one level up.  See the report section.
bool
tests_report_module_of_no_units_is_empty_and_not_passed()
{
    const dt::report_module _m("nothing", "no units at all");

    D_RT_CHECK(_m.total_units()  == 0u);
    D_RT_CHECK(_m.total_checks() == 0u);
    D_RT_CHECK(_m.verdict() == dt::report_verdict::empty);

    D_RT_CHECK(!_m.passed());          // <- the consequential bit
    D_RT_CHECK(!dt::report_verdict_is_pass(_m.verdict()));

    return true;
}


NS_END   // testing
NS_END   // djinterp
