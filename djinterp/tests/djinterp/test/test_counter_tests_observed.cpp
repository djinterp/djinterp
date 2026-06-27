// djinterp
#include "test_counter_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_test_counter_observe
  Verifies a single observe() registration.
  Tests the following:
  - the observed count increments
  - observed(0) returns the address of the registered external counter
*/
bool
tests_test_counter_observe()
{
    tc observer;
    tc external(5, 0, 100);

    D_TC_CHECK(observer.observed_count() == 0);

    observer.observe(external);

    D_TC_CHECK(observer.observed_count() == 1);
    D_TC_CHECK(observer.observed(0)      == &external);

    return true;
}


/*
tests_test_counter_observe_multiple
  Verifies several observed counters are tracked in order.
  Tests the following:
  - the observed count reflects both registrations
  - each index returns the matching external counter's address
*/
bool
tests_test_counter_observe_multiple()
{
    tc observer;
    tc a;
    tc b;

    observer.observe(a);
    observer.observe(b);

    D_TC_CHECK(observer.observed_count() == 2);
    D_TC_CHECK(observer.observed(0)      == &a);
    D_TC_CHECK(observer.observed(1)      == &b);

    return true;
}


/*
tests_test_counter_observed_out_of_range
  Verifies the bounds check in observed().
  Tests the following:
  - observed(0) on an empty list returns nullptr
  - after one registration, the in-range index returns the pointer and the
    first past-the-end index returns nullptr
*/
bool
tests_test_counter_observed_out_of_range()
{
    tc observer;
    tc external;

    D_TC_CHECK(observer.observed(0) == nullptr);

    observer.observe(external);

    D_TC_CHECK(observer.observed(0) == &external);
    D_TC_CHECK(observer.observed(1) == nullptr);

    return true;
}


/*
tests_test_counter_observed_live_view
  Verifies the observed pointer is a live view of the external counter.
  Tests the following:
  - mutating the external counter is visible through observed(0)
*/
bool
tests_test_counter_observed_live_view()
{
    tc observer;
    tc external(0, 0, 100);

    observer.observe(external);

    external.increment(42);

    D_TC_CHECK(observer.observed(0)->value() == 42);

    return true;
}


/*
tests_test_counter_observed_count
  Verifies observed_count() tracks registrations.
  Tests the following:
  - the count starts at zero and increments with each observe()
*/
bool
tests_test_counter_observed_count()
{
    tc observer;
    tc a;
    tc b;

    D_TC_CHECK(observer.observed_count() == 0);

    observer.observe(a);

    D_TC_CHECK(observer.observed_count() == 1);

    observer.observe(b);

    D_TC_CHECK(observer.observed_count() == 2);

    return true;
}


/*
tests_test_counter_observe_non_owning
  Verifies that observed counters are non-owning: reset_all() does not touch
  them.
  Tests the following:
  - an observed (external) counter keeps its value across the observer's
    reset_all()
  - the registration count is unaffected
*/
bool
tests_test_counter_observe_non_owning()
{
    tc observer;
    tc external(0, 0, 100);

    observer.observe(external);

    external.increment(60);

    observer.reset_all();

    D_TC_CHECK(external.value()          == 60);
    D_TC_CHECK(observer.observed_count() == 1);

    return true;
}


NS_END  // testing
NS_END  // djinterp
