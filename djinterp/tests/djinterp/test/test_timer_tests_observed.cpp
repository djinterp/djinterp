// djinterp
#include "test_timer_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_test_timer_observe
  Verifies a single observe() registration.
  Tests the following:
  - the observed count increments
  - observed(0) returns the address of the registered external timer
*/
bool
tests_test_timer_observe()
{
    reset_clock();

    tt observer;
    tt external;

    D_TT_CHECK(observer.observed_count() == 0);

    observer.observe(external);

    D_TT_CHECK(observer.observed_count() == 1);
    D_TT_CHECK(observer.observed(0)      == &external);

    return true;
}


/*
tests_test_timer_observe_multiple
  Verifies several observed timers are tracked in order.
  Tests the following:
  - the observed count reflects both registrations
  - each index returns the matching external timer's address
*/
bool
tests_test_timer_observe_multiple()
{
    reset_clock();

    tt observer;
    tt a;
    tt b;

    observer.observe(a);
    observer.observe(b);

    D_TT_CHECK(observer.observed_count() == 2);
    D_TT_CHECK(observer.observed(0)      == &a);
    D_TT_CHECK(observer.observed(1)      == &b);

    return true;
}


/*
tests_test_timer_observed_out_of_range
  Verifies the bounds check in observed().
  Tests the following:
  - observed(0) on an empty list returns nullptr
  - after one registration, the in-range index returns the pointer and the
    first past-the-end index returns nullptr
*/
bool
tests_test_timer_observed_out_of_range()
{
    reset_clock();

    tt observer;
    tt external;

    D_TT_CHECK(observer.observed(0) == nullptr);

    observer.observe(external);

    D_TT_CHECK(observer.observed(0) == &external);
    D_TT_CHECK(observer.observed(1) == nullptr);

    return true;
}


/*
tests_test_timer_observed_live_view
  Verifies the observed pointer is a live view of the external timer.
  Tests the following:
  - timing the external timer is visible through observed(0)
*/
bool
tests_test_timer_observed_live_view()
{
    reset_clock();

    tt observer;
    tt external;

    observer.observe(external);

    external.start();
    test_clock::advance(msec(45));
    external.stop();

    D_TT_CHECK(observer.observed(0)->elapsed() == msec(45));

    return true;
}


/*
tests_test_timer_observed_count
  Verifies observed_count() tracks registrations.
  Tests the following:
  - the count starts at zero and increments with each observe()
*/
bool
tests_test_timer_observed_count()
{
    reset_clock();

    tt observer;
    tt a;
    tt b;

    D_TT_CHECK(observer.observed_count() == 0);

    observer.observe(a);

    D_TT_CHECK(observer.observed_count() == 1);

    observer.observe(b);

    D_TT_CHECK(observer.observed_count() == 2);

    return true;
}


/*
tests_test_timer_observe_non_owning
  Verifies that observed timers are non-owning: reset_all() does not touch
  them.
  Tests the following:
  - an observed (external) timer keeps its accumulated time across the
    observer's reset_all()
  - the registration count is unaffected
*/
bool
tests_test_timer_observe_non_owning()
{
    reset_clock();

    tt observer;
    tt external;

    observer.observe(external);

    external.start();
    test_clock::advance(msec(60));
    external.stop();

    observer.reset_all();

    D_TT_CHECK(external.elapsed()        == msec(60));
    D_TT_CHECK(observer.observed_count() == 1);

    return true;
}


NS_END  // testing
NS_END  // djinterp
