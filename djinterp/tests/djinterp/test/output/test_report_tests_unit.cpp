/*******************************************************************************
* djinterp [test]                                     test_report_tests_unit.cpp
*
*   Section III of the test_report suite: THE UNIT.
*
*   A unit is the first place the model FOLDS: its checks derive its verdict.  Two
* behaviours here are worth more than the rest of the section put together, and
* both are traps rather than bugs:
*
*   1. THE LATCH.  `verdict` is a PUBLIC field, but the "was it set explicitly?"
*      flag behind it is PRIVATE and only set_verdict() raises it.  So assigning
*      u.verdict = failed LOOKS like pinning the verdict and is not: the next
*      add_check() re-derives it and silently throws the assignment away.  Two
*      spellings, one of which quietly does nothing.
*
*   2. THE ALL-SKIPPED PASS.  A unit whose every check was skipped derives to
*      `passed`, with zero passed checks behind it.  (The fold's doing - see the
*      verdict section - but this is where it becomes visible on a real object.)
*
*
* path:      /tests/djinterp/test/output/test_report_tests_unit.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.06
*                                                            revised: 2026.09.21
*******************************************************************************/

// std
#include <cstddef>
#include <cstdint>
#include <string>
#include <type_traits>
#include <vector>
// djinterp
#include "test_report_tests.hpp"


NS_DJINTERP
NS_TESTING


// tests_report_unit_defaults_to_an_empty_pending_free_unit
//   a fresh unit is EMPTY, not pending and not passed - it has observed nothing.
bool
tests_report_unit_defaults_to_an_empty_pending_free_unit()
{
    const dt::report_unit _u;

    D_RT_CHECK(_u.name.empty());
    D_RT_CHECK(_u.checks.empty());
    D_RT_CHECK(_u.verdict == dt::report_verdict::empty);
    D_RT_CHECK(_u.elapsed_ns == 0);

    D_RT_CHECK(!_u.passed());              // empty is NOT a pass
    D_RT_CHECK(_u.total_checks()  == 0u);
    D_RT_CHECK(_u.passed_checks() == 0u);
    D_RT_CHECK(_u.failed_checks() == 0u);

    return true;
}


// tests_report_unit_named_constructor
bool
tests_report_unit_named_constructor()
{
    const dt::report_unit _u("alpha");

    D_RT_CHECK(_u.name == "alpha");
    D_RT_CHECK(_u.checks.empty());
    D_RT_CHECK(_u.verdict == dt::report_verdict::empty);

    // the constructor is explicit - a bare string does not silently become a unit
    D_RT_CHECK((!std::is_convertible<std::string, dt::report_unit>::value));

    return true;
}


// tests_report_unit_add_check_appends_and_derives
//   the ordinary path: adding a check both records it and re-derives the verdict.
bool
tests_report_unit_add_check_appends_and_derives()
{
    dt::report_unit _u("u");

    D_RT_CHECK(_u.verdict == dt::report_verdict::empty);

    _u.add_check("one", dt::test_status::passed);

    D_RT_CHECK(_u.total_checks() == 1u);
    D_RT_CHECK(_u.checks[0].description == "one");
    D_RT_CHECK(_u.checks[0].status == dt::test_status::passed);
    D_RT_CHECK(_u.verdict == dt::report_verdict::passed);   // derived, not empty
    D_RT_CHECK(_u.passed());

    return true;
}


// tests_report_unit_add_check_rederives_on_every_call
//   the verdict tracks the checks as they accumulate - it is not computed once and
// cached.  A unit that was passing goes red the moment a failure lands, and STAYS
// red when a later check passes.
bool
tests_report_unit_add_check_rederives_on_every_call()
{
    dt::report_unit _u("u");

    _u.add_check("a", dt::test_status::passed);
    D_RT_CHECK(_u.verdict == dt::report_verdict::passed);

    _u.add_check("b", dt::test_status::failed);
    D_RT_CHECK(_u.verdict == dt::report_verdict::failed);    // went red

    _u.add_check("c", dt::test_status::passed);
    D_RT_CHECK(_u.verdict == dt::report_verdict::failed);    // ...and stays red

    D_RT_CHECK(_u.total_checks()  == 3u);
    D_RT_CHECK(_u.passed_checks() == 2u);
    D_RT_CHECK(_u.failed_checks() == 1u);

    return true;
}


// tests_report_unit_add_result_maps_bool_to_status
//   the boolean convenience: true -> passed, false -> failed.  It cannot produce
// any of the other three statuses, which is the point - it is for leaf runners
// whose tests are simply true or false.
bool
tests_report_unit_add_result_maps_bool_to_status()
{
    dt::report_unit _u("u");

    _u.add_result("ok",  true);
    _u.add_result("bad", false);

    D_RT_CHECK(_u.total_checks() == 2u);
    D_RT_CHECK(_u.checks[0].status == dt::test_status::passed);
    D_RT_CHECK(_u.checks[1].status == dt::test_status::failed);
    D_RT_CHECK(_u.checks[0].description == "ok");
    D_RT_CHECK(_u.checks[1].description == "bad");

    // it derives the verdict exactly as add_check does
    D_RT_CHECK(_u.verdict == dt::report_verdict::failed);

    // a unit of nothing but true results is a clean pass
    dt::report_unit _clean("c");

    _clean.add_result("a", true);
    _clean.add_result("b", true);

    D_RT_CHECK(_clean.verdict == dt::report_verdict::passed);
    D_RT_CHECK(_clean.passed());

    return true;
}


// tests_report_unit_set_verdict_pins_it_against_later_checks
//   set_verdict() raises the private latch, so the unit's conclusion stops
// tracking its checks.  This is the supported way to say "this unit failed for a
// reason that is not in its assertion list" - a crash, a timeout, a bail-out.
bool
tests_report_unit_set_verdict_pins_it_against_later_checks()
{
    dt::report_unit _u("u");

    _u.set_verdict(dt::report_verdict::failed);

    D_RT_CHECK(_u.verdict == dt::report_verdict::failed);

    // ...and a subsequent PASSING check does not un-fail it
    _u.add_check("a", dt::test_status::passed);

    D_RT_CHECK(_u.verdict == dt::report_verdict::failed);   // still pinned
    D_RT_CHECK(!_u.passed());

    // the check WAS still recorded - pinning the verdict does not stop the tally
    D_RT_CHECK(_u.total_checks()  == 1u);
    D_RT_CHECK(_u.passed_checks() == 1u);

    // so a pinned unit can report a failing verdict over a wholly passing tally,
    // which is exactly what a crashed-but-asserted unit looks like
    D_RT_CHECK(_u.failed_checks() == 0u);

    // the latch works for any verdict, including pinning a PASS over failures
    dt::report_unit _forced("f");

    _forced.set_verdict(dt::report_verdict::passed);
    _forced.add_check("boom", dt::test_status::failed);

    D_RT_CHECK(_forced.verdict == dt::report_verdict::passed);
    D_RT_CHECK(_forced.passed());
    D_RT_CHECK(_forced.failed_checks() == 1u);      // green verdict, red tally

    return true;
}


// tests_report_unit_assigning_the_verdict_field_does_not_pin_it
//   A TRAP, pinned as fact.
//
//   `verdict` is public and assignable, so this compiles and looks right:
//
//       u.verdict = report_verdict::failed;
//
//   It is not right.  The latch that stops add_check() from re-deriving is
// PRIVATE (m_verdict_explicit) and only set_verdict() raises it - so the next
// add_check() recomputes the verdict from the checks and SILENTLY DISCARDS the
// assignment.  Two spellings that read identically at the call site; one of them
// quietly does nothing.
//
//   The field-assignment form is only safe if no check is ever added afterwards,
// which is invisible from the assignment itself.  set_verdict() is the one that
// means what it says.
bool
tests_report_unit_assigning_the_verdict_field_does_not_pin_it()
{
    // -- the trap: assign the field, then add a check --------------------------
    dt::report_unit _assigned("a");

    _assigned.verdict = dt::report_verdict::failed;

    D_RT_CHECK(_assigned.verdict == dt::report_verdict::failed);   // ...so far so good

    _assigned.add_check("x", dt::test_status::passed);

    //   the assignment is GONE - re-derived from the checks
    D_RT_CHECK(_assigned.verdict == dt::report_verdict::passed);
    D_RT_CHECK(_assigned.passed());                                // silently green

    // -- the supported form: set_verdict(), then add a check -------------------
    dt::report_unit _pinned("p");

    _pinned.set_verdict(dt::report_verdict::failed);
    _pinned.add_check("x", dt::test_status::passed);

    D_RT_CHECK(_pinned.verdict == dt::report_verdict::failed);     // held
    D_RT_CHECK(!_pinned.passed());

    //   the two units carry IDENTICAL checks and report OPPOSITE verdicts, and the
    // only difference is which spelling the caller reached for
    D_RT_CHECK(_assigned.total_checks() == _pinned.total_checks());
    D_RT_CHECK(_assigned.passed() != _pinned.passed());

    //   the assignment does survive if nothing is added after it, which is what
    // makes the trap quiet: it works right up until someone appends a check
    dt::report_unit _lucky("l");

    _lucky.add_check("x", dt::test_status::passed);
    _lucky.verdict = dt::report_verdict::failed;     // assigned LAST

    D_RT_CHECK(_lucky.verdict == dt::report_verdict::failed);

    return true;
}


// tests_report_unit_check_tallies
//   total / passed / failed over a mixed unit.  Note the three do NOT partition:
// skipped and pending checks are in `total` but in neither of the other two.
bool
tests_report_unit_check_tallies()
{
    const dt::report_unit _u = unit_of("mixed",
    {
        dt::test_status::passed,
        dt::test_status::passed,
        dt::test_status::failed,
        dt::test_status::skipped,
        dt::test_status::pending,
        dt::test_status::error
    });

    D_RT_CHECK(_u.total_checks()  == 6u);
    D_RT_CHECK(_u.passed_checks() == 2u);
    D_RT_CHECK(_u.failed_checks() == 2u);       // the failure AND the error

    // passed + failed != total: the skip and the pending are in neither bucket
    D_RT_CHECK(_u.passed_checks() + _u.failed_checks() == 4u);
    D_RT_CHECK(_u.passed_checks() + _u.failed_checks() != _u.total_checks());

    return true;
}


// tests_report_unit_failed_checks_counts_errors_too
//   the two non-pass TERMINAL states are folded together: an errored check is a
// failed check as far as the tally is concerned.
bool
tests_report_unit_failed_checks_counts_errors_too()
{
    const dt::report_unit _e = unit_of("e", { dt::test_status::error });

    D_RT_CHECK(_e.failed_checks() == 1u);
    D_RT_CHECK(_e.passed_checks() == 0u);
    D_RT_CHECK(_e.verdict == dt::report_verdict::failed);

    const dt::report_unit _both = unit_of("b",
        { dt::test_status::failed, dt::test_status::error });

    D_RT_CHECK(_both.failed_checks() == 2u);

    return true;
}


// tests_report_unit_failed_checks_ignores_skipped_and_pending
//   the counterpart: a skipped or pending check is NOT a failure.  So a unit can
// have zero failed checks and still not be a pass (it would be pending), and - the
// stranger direction - it can have zero failed checks, zero passed checks, and BE
// a pass (all skipped; see below).
bool
tests_report_unit_failed_checks_ignores_skipped_and_pending()
{
    const dt::report_unit _s = unit_of("s",
        { dt::test_status::skipped, dt::test_status::pending });

    D_RT_CHECK(_s.total_checks()  == 2u);
    D_RT_CHECK(_s.failed_checks() == 0u);      // neither counts as a failure
    D_RT_CHECK(_s.passed_checks() == 0u);      // ...nor as a pass

    // the pending check does block the verdict, though
    D_RT_CHECK(_s.verdict == dt::report_verdict::pending);
    D_RT_CHECK(!_s.passed());

    return true;
}


// tests_report_unit_an_all_skipped_unit_reads_as_passed
//   A FINDING, on a real object this time.
//
//   Every check skipped -> the fold finds no failure and no pending -> `passed`.
// The unit reports a CLEAN PASS with zero passed checks behind it.  A renderer
// asking "did this unit pass?" is told yes; a renderer asking "how many assertions
// passed?" is told none.  Both are reading this same object.
//
//   See tests_report_counts_helper_skipped_alone_reads_as_passed for the fold that
// causes it and the design question it raises.
bool
tests_report_unit_an_all_skipped_unit_reads_as_passed()
{
    const dt::report_unit _u = unit_of("skipped_everything",
    {
        dt::test_status::skipped,
        dt::test_status::skipped,
        dt::test_status::skipped
    });

    D_RT_CHECK(_u.verdict == dt::report_verdict::passed);     // GREEN...
    D_RT_CHECK(_u.passed());

    D_RT_CHECK(_u.total_checks()  == 3u);
    D_RT_CHECK(_u.passed_checks() == 0u);                     // ...over nothing
    D_RT_CHECK(_u.failed_checks() == 0u);

    //   one skipped check among passing ones is invisible, which is the same rule
    // and much less alarming
    const dt::report_unit _mostly = unit_of("m",
        { dt::test_status::passed, dt::test_status::skipped });

    D_RT_CHECK(_mostly.verdict == dt::report_verdict::passed);
    D_RT_CHECK(_mostly.passed_checks() == 1u);
    D_RT_CHECK(_mostly.total_checks()  == 2u);

    return true;
}


// tests_report_unit_derives_every_status
//   the derivation swept over each status ALONE, so every arm of derive_verdict's
// switch is exercised.
bool
tests_report_unit_derives_every_status()
{
    D_RT_CHECK(unit_of("a", { dt::test_status::passed  }).verdict ==
               dt::report_verdict::passed);
    D_RT_CHECK(unit_of("b", { dt::test_status::failed  }).verdict ==
               dt::report_verdict::failed);
    D_RT_CHECK(unit_of("c", { dt::test_status::skipped }).verdict ==
               dt::report_verdict::passed);      // <- the surprise, again
    D_RT_CHECK(unit_of("d", { dt::test_status::pending }).verdict ==
               dt::report_verdict::pending);
    D_RT_CHECK(unit_of("e", { dt::test_status::error   }).verdict ==
               dt::report_verdict::failed);

    // ...and the precedence, on a real unit
    D_RT_CHECK(unit_of("f", { dt::test_status::passed,
                              dt::test_status::pending,
                              dt::test_status::failed }).verdict ==
               dt::report_verdict::failed);      // failure outranks pending

    return true;
}


// tests_report_unit_an_unknown_status_splits_the_verdict_from_the_tally
//   A FINDING, pinned as fact.
//
//   derive_verdict() and failed_checks() classify an out-of-range status
// DIFFERENTLY, because they are written differently:
//
//       derive_verdict()   switches, with `default:` folding into the ERROR count
//       failed_checks()    tests  == failed || == error,  explicitly
//
//   So an unknown status is an error to the VERDICT (the unit goes red) and is
// nothing at all to the TALLY (it counts as neither passed nor failed).  The unit
// then reports FAIL with zero failed checks - the verdict and the tally disagree
// about the same object.
//
//   On the five real statuses the two agree, so this is only reachable with a
// corrupt or out-of-range value.  It is pinned anyway because it shows the two code
// paths do not share a classification, and a sixth status added to the enum would
// land in exactly this gap: red verdict, invisible in every count.
bool
tests_report_unit_an_unknown_status_splits_the_verdict_from_the_tally()
{
    dt::report_unit _u("u");

    _u.add_check("corrupt", static_cast<dt::test_status>(99));

    // the VERDICT treats it as an error -> the unit is red
    D_RT_CHECK(_u.verdict == dt::report_verdict::failed);
    D_RT_CHECK(!_u.passed());

    // the TALLY treats it as neither -> the unit has no failures
    D_RT_CHECK(_u.total_checks()  == 1u);
    D_RT_CHECK(_u.failed_checks() == 0u);      // <- disagrees with the verdict
    D_RT_CHECK(_u.passed_checks() == 0u);

    //   a renderer printing "FAIL (0/1 assertions passed, 0 failed)" is reading a
    // consistent object and producing a nonsensical line
    D_RT_CHECK(!dt::report_verdict_is_pass(_u.verdict));
    D_RT_CHECK(_u.failed_checks() == 0u);

    //   for contrast, a REAL error is classified the same way by both
    dt::report_unit _real("r");

    _real.add_check("boom", dt::test_status::error);

    D_RT_CHECK(_real.verdict == dt::report_verdict::failed);
    D_RT_CHECK(_real.failed_checks() == 1u);   // <- agrees

    return true;
}


// tests_report_unit_carries_its_elapsed_time_untouched
//   elapsed_ns is pure payload: the model never reads it, derives from it, or
// resets it.  A renderer that wants a duration reads it back exactly as set.
bool
tests_report_unit_carries_its_elapsed_time_untouched()
{
    dt::report_unit _u("u");

    D_RT_CHECK(_u.elapsed_ns == 0);

    _u.elapsed_ns = 1234567890;

    _u.add_check("a", dt::test_status::passed);
    _u.set_verdict(dt::report_verdict::failed);

    D_RT_CHECK(_u.elapsed_ns == 1234567890);   // untouched by either

    // it is a signed 64-bit field, so a large duration is representable
    _u.elapsed_ns = static_cast<std::int64_t>(9000000000000000000LL);

    D_RT_CHECK(_u.elapsed_ns == 9000000000000000000LL);

    return true;
}


NS_END   // testing
NS_END   // djinterp
