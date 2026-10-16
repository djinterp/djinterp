/*******************************************************************************
* djinterp [core]                                       timer_tests_observed.cpp
*
*
* path:      /tests/djinterp/core/util/timer/timer_tests_observed.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "timer_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_timer_observe
  Verifies a single observe() registration.
  Tests the following:
  - the observed count increments
  - observed(0) returns the address of the registered external timer
*/
bool
tests_timer_observe()
{
    reset_clock();

    ms_timer observer;
    ms_timer external;

    D_TM_CHECK(observer.observed_count() == 0);

    observer.observe(external);

    D_TM_CHECK(observer.observed_count() == 1);
    D_TM_CHECK(observer.observed(0)      == &external);

    return true;
}


/*
tests_timer_observe_multiple
  Verifies several observed timers are tracked in order.
  Tests the following:
  - the observed count reflects both registrations
  - each index returns the matching external timer's address
*/
bool
tests_timer_observe_multiple()
{
    reset_clock();

    ms_timer observer;
    ms_timer a;
    ms_timer b;

    observer.observe(a);
    observer.observe(b);

    D_TM_CHECK(observer.observed_count() == 2);
    D_TM_CHECK(observer.observed(0)      == &a);
    D_TM_CHECK(observer.observed(1)      == &b);

    return true;
}


/*
tests_timer_observed_out_of_range
  Verifies the bounds check in observed().
  Tests the following:
  - observed(0) on an empty list returns nullptr
  - after one registration, the in-range index returns the pointer and the
    first past-the-end index returns nullptr
*/
bool
tests_timer_observed_out_of_range()
{
    reset_clock();

    ms_timer observer;
    ms_timer external;

    // empty list: any index is out of range.
    D_TM_CHECK(observer.observed(0) == nullptr);

    observer.observe(external);

    // in range returns the pointer; past the end returns nullptr.
    D_TM_CHECK(observer.observed(0) == &external);
    D_TM_CHECK(observer.observed(1) == nullptr);

    return true;
}


/*
tests_timer_observed_live_view
  Verifies the observed pointer is a live view of the external timer.
  Tests the following:
  - driving the external timer is visible through observed(0)
  - both elapsed() and remaining() read through the view reflect its state
*/
bool
tests_timer_observed_live_view()
{
    reset_clock();

    ms_timer observer;
    ms_timer external(msec(100));

    observer.observe(external);

    // changes to the external timer are visible through the view.
    external.start();
    test_clock::advance(msec(30));
    external.stop();

    D_TM_CHECK(observer.observed(0)->elapsed()   == msec(30));
    D_TM_CHECK(observer.observed(0)->remaining() == msec(70));

    return true;
}


/*
tests_timer_observed_count
  Verifies observed_count() tracks registrations.
  Tests the following:
  - the count starts at zero and increments with each observe()
*/
bool
tests_timer_observed_count()
{
    reset_clock();

    ms_timer observer;
    ms_timer a;
    ms_timer b;

    D_TM_CHECK(observer.observed_count() == 0);

    observer.observe(a);

    D_TM_CHECK(observer.observed_count() == 1);

    observer.observe(b);

    D_TM_CHECK(observer.observed_count() == 2);

    return true;
}


/*
tests_timer_observe_non_owning
  Verifies the observed pointer is non-owning: it drives the real external.
  Tests the following:
  - starting and stopping through observed(0) accumulates time on the
    external timer itself
  - the external timer's running state reflects the operations
*/
bool
tests_timer_observe_non_owning()
{
    reset_clock();

    ms_timer observer;
    ms_timer external;

    observer.observe(external);

    // the observed pointer is non-owning: driving it drives the external.
    observer.observed(0)->start();
    test_clock::advance(msec(15));
    observer.observed(0)->stop();

    D_TM_CHECK(external.elapsed() == msec(15));
    D_TM_CHECK(external.running() == false);

    return true;
}


NS_END  // testing
NS_END  // djinterp
