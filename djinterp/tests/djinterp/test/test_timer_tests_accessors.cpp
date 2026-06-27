// djinterp
#include "test_timer_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_test_timer_elapsed
  Verifies elapsed() reflects accumulated time, including live time while
  running.
  Tests the following:
  - elapsed starts at zero
  - while running, elapsed includes time since the last start
  - after stop, elapsed holds the accumulated total
*/
bool
tests_test_timer_elapsed()
{
    reset_clock();

    tt t;

    D_TT_CHECK(t.elapsed() == msec(0));

    t.start();
    test_clock::advance(msec(30));

    D_TT_CHECK(t.elapsed() == msec(30));

    t.stop();

    D_TT_CHECK(t.elapsed() == msec(30));

    return true;
}


/*
tests_test_timer_max
  Verifies max() returns the configured limit.
  Tests the following:
  - max() returns the duration supplied at construction
*/
bool
tests_test_timer_max()
{
    reset_clock();

    tt t(msec(250));

    D_TT_CHECK(t.max() == msec(250));

    return true;
}


/*
tests_test_timer_has_max
  Verifies has_max() distinguishes limited from unlimited timers.
  Tests the following:
  - has_max() is true when a limit was supplied, false otherwise
*/
bool
tests_test_timer_has_max()
{
    reset_clock();

    tt with_max(msec(100));
    tt without;

    D_TT_CHECK(with_max.has_max() == true);
    D_TT_CHECK(without.has_max()  == false);

    return true;
}


/*
tests_test_timer_running
  Verifies running() tracks the running state.
  Tests the following:
  - running() is false initially, true after start, false after stop
*/
bool
tests_test_timer_running()
{
    reset_clock();

    tt t;

    D_TT_CHECK(t.running() == false);

    t.start();

    D_TT_CHECK(t.running() == true);

    t.stop();

    D_TT_CHECK(t.running() == false);

    return true;
}


/*
tests_test_timer_expired
  Verifies expired() reflects whether accumulated time met the limit.
  Tests the following:
  - a limited timer is not expired until accumulated time meets the maximum
  - a timer with no maximum never expires
*/
bool
tests_test_timer_expired()
{
    reset_clock();

    tt limited(msec(100));

    D_TT_CHECK(limited.expired() == false);

    limited.start();
    test_clock::advance(msec(100));
    limited.stop();

    D_TT_CHECK(limited.expired() == true);

    tt unlimited;

    unlimited.start();
    test_clock::advance(msec(1000));
    unlimited.stop();

    D_TT_CHECK(unlimited.expired() == false);

    return true;
}


/*
tests_test_timer_remaining
  Verifies remaining() returns time left before expiry, clamped at zero.
  Tests the following:
  - remaining equals the full limit before any time elapses
  - remaining decreases as time accumulates
  - remaining clamps to zero past the limit, and is zero with no maximum
*/
bool
tests_test_timer_remaining()
{
    reset_clock();

    tt t(msec(100));

    D_TT_CHECK(t.remaining() == msec(100));

    t.start();
    test_clock::advance(msec(30));
    t.stop();

    D_TT_CHECK(t.remaining() == msec(70));

    t.start();
    test_clock::advance(msec(200));
    t.stop();

    D_TT_CHECK(t.remaining() == msec(0));

    tt unlimited;

    D_TT_CHECK(unlimited.remaining() == msec(0));

    return true;
}


NS_END  // testing
NS_END  // djinterp
