/*******************************************************************************
* djinterp [cpp][utility]                                  timer_tests_event.cpp
*
* The timer suite's event tests: timer_handler, bound on a real
* DEventDispatcher, on the hand-advanced test clock.
*
*
* path:      /tests/djinterp/cpp/util/timer/timer_tests_event.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.09
*                                                            revised: 2026.11.10
*******************************************************************************/
// djinterp
#include "timer_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_timer_event_span
  Timing between two events.
  Tests the following:
  - start on one tag and stop on another time the span between occurrences
  - spans accumulate
  - a second start while running, or a stop while stopped, changes nothing
*/
bool tests_timer_event_span()
{
    reset_clock();

    DEventDispatcher dispatcher;
    DMsTimer         t;

    (void)dispatcher.bind<DEvTmOpen>(
        make_timer_handler(t,
                           DTimerEventOp::start));
    (void)dispatcher.bind<DEvTmClose>(
        make_timer_handler(t,
                           DTimerEventOp::stop));

    dispatcher.fire<DEvTmOpen>();
    test_clock::advance(DMsec(60));
    dispatcher.fire<DEvTmOpen>();
    test_clock::advance(DMsec(15));
    dispatcher.fire<DEvTmClose>();

    D_TM_CHECK(t.elapsed() == DMsec(75));

    test_clock::advance(DMsec(300));
    dispatcher.fire<DEvTmClose>();
    dispatcher.fire<DEvTmOpen>();
    test_clock::advance(DMsec(25));
    dispatcher.fire<DEvTmClose>();

    D_TM_CHECK(t.elapsed() == DMsec(100));

    return true;
}

/*
tests_timer_event_operations
  Moving time by event.
  Tests the following:
  - increment and decrement add and remove the binding's amount
  - add adds the duration the occurrence carries; an event without one adds
    nothing
  - reset stops the timer and zeroes it
*/
bool tests_timer_event_operations()
{
    reset_clock();

    DEventDispatcher dispatcher;
    DMsTimer         t;

    (void)dispatcher.bind<DEvTmOpen>(
        make_timer_handler(t,
                           DTimerEventOp::increment,
                           DMsec(50)));
    (void)dispatcher.bind<DEvTmClose>(
        make_timer_handler(t,
                           DTimerEventOp::decrement,
                           DMsec(20)));
    (void)dispatcher.bind<DEvTmTook>(
        make_timer_handler(t,
                           DTimerEventOp::add));

    dispatcher.fire<DEvTmOpen>();
    dispatcher.fire<DEvTmClose>();

    D_TM_CHECK(t.elapsed() == DMsec(30));

    dispatcher.fire<DEvTmTook>(DMsec(1234));

    D_TM_CHECK(t.elapsed() == DMsec(1264));

    DEventDispatcher clearing;

    (void)clearing.bind<DEvTmOpen>(
        make_timer_handler(t,
                           DTimerEventOp::add));
    (void)clearing.bind<DEvTmClose>(
        make_timer_handler(t,
                           DTimerEventOp::reset));

    t.start();
    clearing.fire<DEvTmOpen>();

    D_TM_CHECK(t.elapsed() == DMsec(1264));

    clearing.fire<DEvTmClose>();

    D_TM_CHECK(!t.running());
    D_TM_CHECK(t.elapsed() == DMsec::zero());

    return true;
}

/*
tests_timer_event_mutex_timer
  A mutex_timer target.
  Tests the following:
  - start, stop and add drive a mutex_timer as they drive a timer
*/
bool tests_timer_event_mutex_timer()
{
    reset_clock();

    DEventDispatcher               dispatcher;
    mutex_timer<test_clock, DMsec> t;

    (void)dispatcher.bind<DEvTmOpen>(
        make_timer_handler(t,
                           DTimerEventOp::start));
    (void)dispatcher.bind<DEvTmClose>(
        make_timer_handler(t,
                           DTimerEventOp::stop));
    (void)dispatcher.bind<DEvTmTook>(
        make_timer_handler(t,
                           DTimerEventOp::add));

    dispatcher.fire<DEvTmOpen>();
    test_clock::advance(DMsec(40));
    dispatcher.fire<DEvTmClose>();
    dispatcher.fire<DEvTmTook>(DMsec(8));

    D_TM_CHECK(t.elapsed() == DMsec(48));
    D_TM_CHECK(!t.running());

    return true;
}


NS_END  // testing
NS_END  // djinterp
