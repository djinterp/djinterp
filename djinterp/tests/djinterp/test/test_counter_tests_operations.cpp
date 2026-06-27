// djinterp
#include "test_counter_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_test_counter_increment_within_bounds
  Verifies a plain increment fires on_increment(old, new) and not on_limit.
  Tests the following:
  - the value rises and the call returns true
  - on_increment carries the old and new values
  - the default amount is one
*/
bool
tests_test_counter_increment_within_bounds()
{
    event_dispatcher eh;
    event_log        log;
    bind_log(eh, log);

    tc c(0, 0, 100, &eh);

    const bool result = c.increment(5);

    D_TC_CHECK(result           == true);
    D_TC_CHECK(c.value()        == 5);
    D_TC_CHECK(log.increments   == 1);
    D_TC_CHECK(log.last_inc_old == 0);
    D_TC_CHECK(log.last_inc_new == 5);
    D_TC_CHECK(log.limits       == 0);

    c.increment();

    D_TC_CHECK(c.value()        == 6);
    D_TC_CHECK(log.last_inc_new == 6);

    return true;
}


/*
tests_test_counter_increment_exact_max_no_limit
  Verifies that landing exactly on max is a success, not a clamp.
  Tests the following:
  - reaching max exactly returns true and fires on_increment
  - no on_limit fires (the bound was met, not overshot)
*/
bool
tests_test_counter_increment_exact_max_no_limit()
{
    event_dispatcher eh;
    event_log        log;
    bind_log(eh, log);

    tc c(95, 0, 100, &eh);

    const bool result = c.increment(5);

    D_TC_CHECK(result         == true);
    D_TC_CHECK(c.value()      == 100);
    D_TC_CHECK(c.at_max()     == true);
    D_TC_CHECK(log.increments == 1);
    D_TC_CHECK(log.limits     == 0);

    return true;
}


/*
tests_test_counter_increment_overshoot_clamps
  Verifies an overshooting increment clamps to max and fires both events.
  Tests the following:
  - the value clamps to max and the call returns false
  - on_increment fires with the clamped new value
  - on_limit fires with the clamped value
*/
bool
tests_test_counter_increment_overshoot_clamps()
{
    event_dispatcher eh;
    event_log        log;
    bind_log(eh, log);

    tc c(50, 0, 100, &eh);

    const bool result = c.increment(80);

    D_TC_CHECK(result           == false);
    D_TC_CHECK(c.value()        == 100);
    D_TC_CHECK(log.increments   == 1);
    D_TC_CHECK(log.last_inc_old == 50);
    D_TC_CHECK(log.last_inc_new == 100);
    D_TC_CHECK(log.limits       == 1);
    D_TC_CHECK(log.last_limit   == 100);

    return true;
}


/*
tests_test_counter_increment_at_max_noop
  Verifies incrementing an already-maxed counter fires on_limit but not
  on_increment.
  Tests the following:
  - the value does not change, so no on_increment fires
  - the clamped operation still returns false and fires on_limit
*/
bool
tests_test_counter_increment_at_max_noop()
{
    event_dispatcher eh;
    event_log        log;
    bind_log(eh, log);

    tc c(100, 0, 100, &eh);

    const bool result = c.increment(5);

    D_TC_CHECK(result         == false);
    D_TC_CHECK(c.value()      == 100);
    D_TC_CHECK(log.increments == 0);
    D_TC_CHECK(log.limits     == 1);
    D_TC_CHECK(log.last_limit == 100);

    return true;
}


/*
tests_test_counter_increment_zero_noop
  Verifies a zero increment is neither a change nor a clamp.
  Tests the following:
  - the value is unchanged and the call returns true
  - neither on_increment nor on_limit fires
*/
bool
tests_test_counter_increment_zero_noop()
{
    event_dispatcher eh;
    event_log        log;
    bind_log(eh, log);

    tc c(50, 0, 100, &eh);

    const bool result = c.increment(0);

    D_TC_CHECK(result         == true);
    D_TC_CHECK(c.value()      == 50);
    D_TC_CHECK(log.increments == 0);
    D_TC_CHECK(log.limits     == 0);

    return true;
}


/*
tests_test_counter_decrement_within_bounds
  Verifies a plain decrement fires on_decrement(old, new) and not on_limit.
  Tests the following:
  - the value falls and the call returns true
  - on_decrement carries the old and new values
  - the default amount is one
*/
bool
tests_test_counter_decrement_within_bounds()
{
    event_dispatcher eh;
    event_log        log;
    bind_log(eh, log);

    tc c(50, 0, 100, &eh);

    const bool result = c.decrement(20);

    D_TC_CHECK(result           == true);
    D_TC_CHECK(c.value()        == 30);
    D_TC_CHECK(log.decrements   == 1);
    D_TC_CHECK(log.last_dec_old == 50);
    D_TC_CHECK(log.last_dec_new == 30);
    D_TC_CHECK(log.limits       == 0);

    c.decrement();

    D_TC_CHECK(c.value()        == 29);

    return true;
}


/*
tests_test_counter_decrement_undershoot_clamps
  Verifies an undershooting decrement clamps to min and fires both events.
  Tests the following:
  - the value clamps to min and the call returns false
  - on_decrement fires with the clamped new value
  - on_limit fires with the clamped value
*/
bool
tests_test_counter_decrement_undershoot_clamps()
{
    event_dispatcher eh;
    event_log        log;
    bind_log(eh, log);

    tc c(50, 0, 100, &eh);

    const bool result = c.decrement(80);

    D_TC_CHECK(result           == false);
    D_TC_CHECK(c.value()        == 0);
    D_TC_CHECK(log.decrements   == 1);
    D_TC_CHECK(log.last_dec_old == 50);
    D_TC_CHECK(log.last_dec_new == 0);
    D_TC_CHECK(log.limits       == 1);
    D_TC_CHECK(log.last_limit   == 0);

    return true;
}


/*
tests_test_counter_decrement_at_min_noop
  Verifies decrementing an already-minned counter fires on_limit but not
  on_decrement.
  Tests the following:
  - the value does not change, so no on_decrement fires
  - the clamped operation still returns false and fires on_limit
*/
bool
tests_test_counter_decrement_at_min_noop()
{
    event_dispatcher eh;
    event_log        log;
    bind_log(eh, log);

    tc c(0, 0, 100, &eh);

    const bool result = c.decrement(5);

    D_TC_CHECK(result         == false);
    D_TC_CHECK(c.value()      == 0);
    D_TC_CHECK(log.decrements == 0);
    D_TC_CHECK(log.limits     == 1);
    D_TC_CHECK(log.last_limit == 0);

    return true;
}


/*
tests_test_counter_decrement_zero_noop
  Verifies a zero decrement is neither a change nor a clamp.
  Tests the following:
  - the value is unchanged and the call returns true
  - neither on_decrement nor on_limit fires
*/
bool
tests_test_counter_decrement_zero_noop()
{
    event_dispatcher eh;
    event_log        log;
    bind_log(eh, log);

    tc c(50, 0, 100, &eh);

    const bool result = c.decrement(0);

    D_TC_CHECK(result         == true);
    D_TC_CHECK(c.value()      == 50);
    D_TC_CHECK(log.decrements == 0);
    D_TC_CHECK(log.limits     == 0);

    return true;
}


/*
tests_test_counter_reset_fires_on_reset
  Verifies reset() restores the initial value and fires on_reset with the
  prior value.
  Tests the following:
  - the value returns to its initial
  - one on_reset event carries the value before clearing
*/
bool
tests_test_counter_reset_fires_on_reset()
{
    event_dispatcher eh;
    event_log        log;
    bind_log(eh, log);

    tc c(10, 0, 100, &eh);

    c.increment(20);

    c.reset();

    D_TC_CHECK(c.value()      == 10);
    D_TC_CHECK(log.resets     == 1);
    D_TC_CHECK(log.last_reset == 30);

    return true;
}


/*
tests_test_counter_reset_all_leaf
  Verifies reset_all() on a childless counter (the empty-recursion case).
  Tests the following:
  - one on_reset fires for the counter itself with the prior value
  - the value returns to its initial and it still owns no children
*/
bool
tests_test_counter_reset_all_leaf()
{
    event_dispatcher eh;
    event_log        log;
    bind_log(eh, log);

    tc c(10, 0, 100, &eh);

    c.increment(20);

    c.reset_all();

    D_TC_CHECK(c.value()       == 10);
    D_TC_CHECK(log.resets      == 1);
    D_TC_CHECK(log.last_reset  == 30);
    D_TC_CHECK(c.child_count() == 0);

    return true;
}


/*
tests_test_counter_reset_all_recurses
  Verifies reset_all() resets this counter and every owned descendant, with
  each firing on_reset through the inherited dispatcher.
  Tests the following:
  - one on_reset fires per counter in the tree (root + two children + a
    grandchild == four)
  - the root's value returns to its initial
*/
bool
tests_test_counter_reset_all_recurses()
{
    event_dispatcher eh;
    event_log        log;
    bind_log(eh, log);

    tc c(0, 0, 100, &eh);

    c.add_child();
    c.add_child();
    c.child(0).add_child();

    c.increment(7);

    c.reset_all();

    D_TC_CHECK(log.resets == 4);
    D_TC_CHECK(c.value()  == 0);

    return true;
}


NS_END  // testing
NS_END  // djinterp
