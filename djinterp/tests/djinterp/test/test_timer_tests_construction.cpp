// djinterp
#include "test_timer_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_test_timer_default_ctor
  Verifies the default constructor's initial state.
  Tests the following:
  - no maximum limit is set; max() is zero
  - the timer is not running and has zero elapsed time
  - it owns no children, observes nothing, and has no event handler
*/
bool
tests_test_timer_default_ctor()
{
    reset_clock();

    tt t;

    D_TT_CHECK(t.has_max()        == false);
    D_TT_CHECK(t.max()            == msec(0));
    D_TT_CHECK(t.running()        == false);
    D_TT_CHECK(t.elapsed()        == msec(0));
    D_TT_CHECK(t.child_count()    == 0);
    D_TT_CHECK(t.observed_count() == 0);
    D_TT_CHECK(t.handler()        == nullptr);

    return true;
}


/*
tests_test_timer_max_ctor
  Verifies the max-duration constructor, whose handler argument defaults to
  null.
  Tests the following:
  - a maximum limit is set and reported by max()
  - the handler defaults to nullptr when only a max is supplied
*/
bool
tests_test_timer_max_ctor()
{
    reset_clock();

    tt t(msec(100));

    D_TT_CHECK(t.has_max() == true);
    D_TT_CHECK(t.max()     == msec(100));
    D_TT_CHECK(t.running() == false);
    D_TT_CHECK(t.handler() == nullptr);

    return true;
}


/*
tests_test_timer_max_handler_ctor
  Verifies the max-plus-handler constructor.
  Tests the following:
  - both the maximum limit and the supplied dispatcher are stored
*/
bool
tests_test_timer_max_handler_ctor()
{
    reset_clock();

    event_dispatcher eh;
    tt t(msec(100), &eh);

    D_TT_CHECK(t.has_max() == true);
    D_TT_CHECK(t.max()     == msec(100));
    D_TT_CHECK(t.handler() == &eh);

    return true;
}


/*
tests_test_timer_handler_ctor
  Verifies the handler-only constructor (no maximum limit).
  Tests the following:
  - the supplied dispatcher is stored
  - no maximum limit is set
*/
bool
tests_test_timer_handler_ctor()
{
    reset_clock();

    event_dispatcher eh;
    tt t(&eh);

    D_TT_CHECK(t.has_max() == false);
    D_TT_CHECK(t.handler() == &eh);
    D_TT_CHECK(t.running() == false);

    return true;
}


NS_END  // testing
NS_END  // djinterp
