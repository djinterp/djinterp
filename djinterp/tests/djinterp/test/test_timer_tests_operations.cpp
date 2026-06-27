// djinterp
#include "test_timer_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_test_timer_start_fires_on_start
  Verifies start() transitions to running and fires on_start once.
  Tests the following:
  - the timer is running after start()
  - exactly one on_start event is dispatched on the transition
*/
bool
tests_test_timer_start_fires_on_start()
{
    reset_clock();

    event_dispatcher eh;
    event_log        log;
    bind_log(eh, log);

    tt t(&eh);

    t.start();

    D_TT_CHECK(t.running() == true);
    D_TT_CHECK(log.starts  == 1);
    D_TT_CHECK(log.stops   == 0);

    return true;
}


/*
tests_test_timer_start_twice_one_event
  Verifies that starting an already-running timer is a no-op for events.
  Tests the following:
  - a second start() does not dispatch a second on_start (the timer was
    already running, so there is no transition)
*/
bool
tests_test_timer_start_twice_one_event()
{
    reset_clock();

    event_dispatcher eh;
    event_log        log;
    bind_log(eh, log);

    tt t(&eh);

    t.start();
    t.start();

    D_TT_CHECK(t.running() == true);
    D_TT_CHECK(log.starts  == 1);

    return true;
}


/*
tests_test_timer_start_when_expired_noop
  Verifies that starting an expired timer does not transition or fire.
  Tests the following:
  - an expired timer stays not-running when start() is called
  - no on_start is dispatched (the start did not take effect)
*/
bool
tests_test_timer_start_when_expired_noop()
{
    reset_clock();

    event_dispatcher eh;
    event_log        log;
    bind_log(eh, log);

    tt t(msec(100), &eh);

    t.start();
    test_clock::advance(msec(100));
    t.stop();

    D_TT_CHECK(t.expired() == true);

    const int starts_before = log.starts;

    t.start();

    D_TT_CHECK(t.running() == false);
    D_TT_CHECK(log.starts  == starts_before);

    return true;
}


/*
tests_test_timer_stop_fires_on_stop
  Verifies stop() accumulates elapsed time and fires on_stop with the count.
  Tests the following:
  - the timer is not running after stop()
  - one on_stop event is dispatched carrying the accumulated count
  - no on_expire fires when there is no maximum limit
*/
bool
tests_test_timer_stop_fires_on_stop()
{
    reset_clock();

    event_dispatcher eh;
    event_log        log;
    bind_log(eh, log);

    tt t(&eh);

    t.start();
    test_clock::advance(msec(50));
    t.stop();

    D_TT_CHECK(t.running()         == false);
    D_TT_CHECK(log.stops           == 1);
    D_TT_CHECK(log.last_stop_count == 50);
    D_TT_CHECK(log.expires         == 0);

    return true;
}


/*
tests_test_timer_stop_fires_on_expire
  Verifies that stopping at or beyond the limit additionally fires on_expire.
  Tests the following:
  - on_stop fires with the accumulated count
  - on_expire fires because accumulated time met the maximum
*/
bool
tests_test_timer_stop_fires_on_expire()
{
    reset_clock();

    event_dispatcher eh;
    event_log        log;
    bind_log(eh, log);

    tt t(msec(100), &eh);

    t.start();
    test_clock::advance(msec(100));
    t.stop();

    D_TT_CHECK(log.stops           == 1);
    D_TT_CHECK(log.last_stop_count == 100);
    D_TT_CHECK(log.expires         == 1);

    return true;
}


/*
tests_test_timer_stop_not_running_noop
  Verifies that stopping a non-running timer is a no-op.
  Tests the following:
  - no on_stop is dispatched when stop() is called on a stopped timer
*/
bool
tests_test_timer_stop_not_running_noop()
{
    reset_clock();

    event_dispatcher eh;
    event_log        log;
    bind_log(eh, log);

    tt t(&eh);

    t.stop();

    D_TT_CHECK(log.stops == 0);

    return true;
}


/*
tests_test_timer_reset_fires_on_reset
  Verifies reset() clears elapsed time and fires on_reset with the prior count.
  Tests the following:
  - one on_reset event is dispatched carrying the count before clearing
  - elapsed time is zero after reset()
*/
bool
tests_test_timer_reset_fires_on_reset()
{
    reset_clock();

    event_dispatcher eh;
    event_log        log;
    bind_log(eh, log);

    tt t(&eh);

    t.start();
    test_clock::advance(msec(30));
    t.stop();

    t.reset();

    D_TT_CHECK(log.resets           == 1);
    D_TT_CHECK(log.last_reset_count == 30);
    D_TT_CHECK(t.elapsed()          == msec(0));

    return true;
}


/*
tests_test_timer_reset_all_leaf
  Verifies reset_all() on a childless timer (the empty-recursion case).
  Tests the following:
  - one on_reset fires for the timer itself with the prior count
  - elapsed time is cleared and the timer still owns no children
*/
bool
tests_test_timer_reset_all_leaf()
{
    reset_clock();

    event_dispatcher eh;
    event_log        log;
    bind_log(eh, log);

    tt t(&eh);

    t.start();
    test_clock::advance(msec(40));
    t.stop();

    t.reset_all();

    D_TT_CHECK(log.resets           == 1);
    D_TT_CHECK(log.last_reset_count == 40);
    D_TT_CHECK(t.elapsed()          == msec(0));
    D_TT_CHECK(t.child_count()      == 0);

    return true;
}


/*
tests_test_timer_reset_all_recurses
  Verifies reset_all() resets this timer and every owned descendant, with each
  firing on_reset through the inherited dispatcher.
  Tests the following:
  - one on_reset fires per timer in the tree (root + two children + a
    grandchild == four)
  - the root's elapsed time is cleared
*/
bool
tests_test_timer_reset_all_recurses()
{
    reset_clock();

    event_dispatcher eh;
    event_log        log;
    bind_log(eh, log);

    tt t(&eh);

    t.add_child();
    t.add_child();
    t.child(0).add_child();

    t.start();
    test_clock::advance(msec(10));
    t.stop();

    t.reset_all();

    D_TT_CHECK(log.resets  == 4);
    D_TT_CHECK(t.elapsed() == msec(0));

    return true;
}


/*
tests_test_timer_multiple_cycles_accumulate
  Verifies that successive start/stop cycles accumulate elapsed time.
  Tests the following:
  - elapsed time is the sum across cycles
  - on_stop fires once per cycle, the last carrying the cumulative count
*/
bool
tests_test_timer_multiple_cycles_accumulate()
{
    reset_clock();

    event_dispatcher eh;
    event_log        log;
    bind_log(eh, log);

    tt t(&eh);

    t.start();
    test_clock::advance(msec(20));
    t.stop();

    D_TT_CHECK(log.last_stop_count == 20);

    t.start();
    test_clock::advance(msec(30));
    t.stop();

    D_TT_CHECK(t.elapsed()         == msec(50));
    D_TT_CHECK(log.stops           == 2);
    D_TT_CHECK(log.last_stop_count == 50);

    return true;
}


NS_END  // testing
NS_END  // djinterp
