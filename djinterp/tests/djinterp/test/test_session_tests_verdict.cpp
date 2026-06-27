// djinterp
#include "test_session_tests.hpp"


NS_DJINTERP
NS_TESTING


//   current_verdict() is private; it is reached through the non-idle
// early-return of run(handler), which returns the verdict computed from the
// session's current counters WITHOUT walking the tree.  Each test below sets
// the counters directly, moves the session out of idle (so run(handler) takes
// the early-return path), and inspects the returned verdict.  A fresh
// default-constructed test_handler is supplied to satisfy the signature; its
// walk is never invoked.


/*
tests_session_current_verdict_empty
  Verifies the verdict is empty when nothing has been observed.
  Tests the following:
  - total == 0 -> session_verdict::empty
*/
bool
tests_session_current_verdict_empty()
{
    session_type  s;
    test_handler  h;

    s.run();   // idle -> running (so the next run(handler) early-returns)

    const session_verdict v = s.run(h);

    return (v == session_verdict::empty);
}

/*
tests_session_current_verdict_failed_on_failed
  Verifies a non-zero failed count yields a failed verdict (first operand of
  the failure guard).
  Tests the following:
  - failed > 0 -> session_verdict::failed
*/
bool
tests_session_current_verdict_failed_on_failed()
{
    session_type  s;
    test_handler  h;

    s.failed().increment(2);
    s.run();

    const session_verdict v = s.run(h);

    return (v == session_verdict::failed);
}

/*
tests_session_current_verdict_failed_on_error
  Verifies a non-zero error count yields a failed verdict (second operand of
  the failure guard, with failed == 0).
  Tests the following:
  - errors > 0 (and failed == 0) -> session_verdict::failed
*/
bool
tests_session_current_verdict_failed_on_error()
{
    session_type  s;
    test_handler  h;

    s.errors().increment(1);
    s.run();

    const session_verdict v = s.run(h);

    return (v == session_verdict::failed);
}

/*
tests_session_current_verdict_pending
  Verifies a non-zero pending count (with no failures or errors) yields a
  pending verdict.
  Tests the following:
  - pending > 0, failed == 0, errors == 0 -> session_verdict::pending
*/
bool
tests_session_current_verdict_pending()
{
    session_type  s;
    test_handler  h;

    s.pending().increment(3);
    s.run();

    const session_verdict v = s.run(h);

    return (v == session_verdict::pending);
}

/*
tests_session_current_verdict_passed
  Verifies the passed verdict when work was observed and nothing failed,
  errored, or remains pending.
  Tests the following:
  - passed > 0, failed == 0, errors == 0, pending == 0 -> passed
*/
bool
tests_session_current_verdict_passed()
{
    session_type  s;
    test_handler  h;

    s.passed().increment(4);
    s.run();

    const session_verdict v = s.run(h);

    return (v == session_verdict::passed);
}


NS_END  // testing
NS_END  // djinterp
