/*******************************************************************************
* djinterp [core]                                     counter_tests_observed.cpp
*
*
* path:      /tests/djinterp/core/util/counter/counter_tests_observed.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "counter_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_counter_observe
  Verifies a single observe() registration.
  Tests the following:
  - the observed count increments
  - observed(0) returns the address of the registered external counter
*/
bool
tests_counter_observe()
{
    ctr observer;
    ctr external(5, 0, 10);

    D_CTR_CHECK(observer.observed_count() == 0);

    observer.observe(external);

    D_CTR_CHECK(observer.observed_count() == 1);
    D_CTR_CHECK(observer.observed(0)      == &external);

    return true;
}


/*
tests_counter_observe_multiple
  Verifies several observed counters are tracked in order.
  Tests the following:
  - the observed count reflects both registrations
  - each index returns the matching external counter's address
*/
bool
tests_counter_observe_multiple()
{
    ctr observer;
    ctr a(1, 0, 10);
    ctr b(2, 0, 10);

    observer.observe(a);
    observer.observe(b);

    D_CTR_CHECK(observer.observed_count() == 2);
    D_CTR_CHECK(observer.observed(0)      == &a);
    D_CTR_CHECK(observer.observed(1)      == &b);

    return true;
}


/*
tests_counter_observed_out_of_range
  Verifies the bounds check in observed().
  Tests the following:
  - observed(0) on an empty list returns nullptr
  - after one registration, the in-range index returns the pointer and the
    first past-the-end index returns nullptr
*/
bool
tests_counter_observed_out_of_range()
{
    ctr observer;
    ctr external(5, 0, 10);

    // empty list: any index is out of range.
    D_CTR_CHECK(observer.observed(0) == nullptr);

    observer.observe(external);

    // in range returns the pointer; past the end returns nullptr.
    D_CTR_CHECK(observer.observed(0) == &external);
    D_CTR_CHECK(observer.observed(1) == nullptr);

    return true;
}


/*
tests_counter_observed_live_view
  Verifies the observed pointer is a live view of the external counter.
  Tests the following:
  - an increment of the external counter is visible through observed(0)
  - a subsequent decrement is likewise reflected through the view
*/
bool
tests_counter_observed_live_view()
{
    ctr observer;
    ctr external(0, 0, 100);

    observer.observe(external);

    // changes to the external counter are visible through the view.
    external.increment(30);

    D_CTR_CHECK(observer.observed(0)->value() == 30);

    external.decrement(10);

    D_CTR_CHECK(observer.observed(0)->value() == 20);

    return true;
}


/*
tests_counter_observed_count
  Verifies observed_count() tracks registrations.
  Tests the following:
  - the count starts at zero and increments with each observe()
*/
bool
tests_counter_observed_count()
{
    ctr observer;
    ctr a;
    ctr b;

    D_CTR_CHECK(observer.observed_count() == 0);

    observer.observe(a);

    D_CTR_CHECK(observer.observed_count() == 1);

    observer.observe(b);

    D_CTR_CHECK(observer.observed_count() == 2);

    return true;
}


/*
tests_counter_observe_non_owning
  Verifies the observed pointer is non-owning: it drives the real external.
  Tests the following:
  - incrementing through observed(0) accumulates on the external counter
    itself
  - the registration count is unaffected by driving the view
*/
bool
tests_counter_observe_non_owning()
{
    ctr observer;
    ctr external(0, 0, 100);

    observer.observe(external);

    // the observed pointer is non-owning: driving it drives the external.
    observer.observed(0)->increment(15);
    observer.observed(0)->increment(5);

    D_CTR_CHECK(external.value()          == 20);
    D_CTR_CHECK(observer.observed_count() == 1);

    return true;
}


NS_END  // testing
NS_END  // djinterp
