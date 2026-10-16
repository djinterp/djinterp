/*******************************************************************************
* djinterp [test]                                  test_handler_tests_result.cpp
*
*   Section III - session results.  session_result is a plain counter snapshot
* with three derived predicates, so these checks set its public fields and read
* the predicates back.  all_passed holds only when at least one leaf was seen
* AND every one passed (pending is deliberately NOT a pass).  any_failed holds
* when a failure or an error was seen.  verdict is the three-way roll-up with a
* fixed precedence: empty (nothing seen) is distinguished from passed, a real
* failure or error outranks any amount of pending, and pending is reported only
* when nothing actually broke but work remains.
*
*
* path:      /tests/djinterp/test/test_handler_tests_result.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.06
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "test_handler_tests.hpp"

NS_DJINTERP
NS_TESTING


// tests_result_default_zero
//   a default-constructed result has every counter zero and reports empty /
// not-passed / not-failed.
bool
tests_result_default_zero()
{
    bool ok = true;

    dt::session_result r;
    ok = D_TH_CHECK(r.passed == 0 && r.failed == 0 && r.skipped == 0 &&
                    r.errors == 0 && r.pending == 0 && r.total == 0) && ok;
    ok = D_TH_CHECK(r.verdict() == dt::session_verdict::empty) && ok;
    ok = D_TH_CHECK(!r.all_passed()) && ok;
    ok = D_TH_CHECK(!r.any_failed()) && ok;

    return ok;
}

// tests_result_all_passed
//   all_passed iff total > 0 and passed == total; a shortfall or an empty
// result is false.
bool
tests_result_all_passed()
{
    bool ok = true;

    dt::session_result r;
    r.passed = 3; r.total = 3;
    ok = D_TH_CHECK(r.all_passed()) && ok;         // total > 0 and passed == total

    r.total = 4;                                   // passed < total
    ok = D_TH_CHECK(!r.all_passed()) && ok;

    dt::session_result e;                          // total == 0
    ok = D_TH_CHECK(!e.all_passed()) && ok;

    return ok;
}

// tests_result_any_failed
//   any_failed iff a failure or an error was observed; passes alone are not a
// failure.
bool
tests_result_any_failed()
{
    bool ok = true;

    dt::session_result r1; r1.failed = 1;
    ok = D_TH_CHECK(r1.any_failed()) && ok;

    dt::session_result r2; r2.errors = 1;
    ok = D_TH_CHECK(r2.any_failed()) && ok;

    dt::session_result r3; r3.passed = 5;          // no failures/errors
    ok = D_TH_CHECK(!r3.any_failed()) && ok;

    return ok;
}

// tests_result_verdict_empty
//   a zero total yields the empty verdict.
bool
tests_result_verdict_empty()
{
    bool ok = true;

    dt::session_result r;
    ok = D_TH_CHECK(r.verdict() == dt::session_verdict::empty) && ok;

    return ok;
}

// tests_result_verdict_passed
//   all-passed with at least one leaf yields the passed verdict.
bool
tests_result_verdict_passed()
{
    bool ok = true;

    dt::session_result r; r.passed = 2; r.total = 2;
    ok = D_TH_CHECK(r.verdict() == dt::session_verdict::passed) && ok;

    return ok;
}

// tests_result_verdict_pending
//   pending with no failure/error yields the pending verdict.
bool
tests_result_verdict_pending()
{
    bool ok = true;

    dt::session_result r; r.passed = 1; r.pending = 1; r.total = 2;
    ok = D_TH_CHECK(r.verdict() == dt::session_verdict::pending) && ok;

    return ok;
}

// tests_result_verdict_failed_dominates
//   a failure or an error yields failed, outranking any pending.
bool
tests_result_verdict_failed_dominates()
{
    bool ok = true;

    dt::session_result r1; r1.failed = 1; r1.total = 1;
    ok = D_TH_CHECK(r1.verdict() == dt::session_verdict::failed) && ok;

    dt::session_result r2; r2.errors = 1; r2.total = 1;
    ok = D_TH_CHECK(r2.verdict() == dt::session_verdict::failed) && ok;

    dt::session_result r3; r3.failed = 1; r3.pending = 3; r3.total = 4;   // failure > pending
    ok = D_TH_CHECK(r3.verdict() == dt::session_verdict::failed) && ok;

    dt::session_result r4; r4.errors = 1; r4.pending = 3; r4.total = 4;   // error   > pending
    ok = D_TH_CHECK(r4.verdict() == dt::session_verdict::failed) && ok;

    return ok;
}


NS_END  // testing
NS_END  // djinterp
