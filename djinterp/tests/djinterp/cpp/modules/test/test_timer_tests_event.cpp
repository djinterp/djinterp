/*******************************************************************************
* djinterp [cpp][test]                                test_timer_tests_event.cpp
*
* The test_timer suite, block "event": test_timer_event.hpp's start and stop
* handlers, bound on a dispatcher, timing the span between two events.
*
*
* path:      /tests/djinterp/cpp/modules/test/test_timer_tests_event.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.10
*                                                            revised: 2026.11.10
*******************************************************************************/
// djinterp
#include "test_timer_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_test_timer_event_span
  Verifies make_test_timer_handlers times the span between the two events
  its handlers are bound to.
  Tests the following:
  - DTtOpen starts the timer and DTtClose stops it
  - a second DTtOpen while running, or DTtClose while stopped, changes
    nothing
  - spans accumulate over several open/close pairs
*/
bool tests_test_timer_event_span()
{
    reset_clock();

    DEventDispatcher dispatcher;
    tt               t;

    const auto handlers = test::make_test_timer_handlers(t);

    (void)dispatcher.bind<DTtOpen>(handlers.first);
    (void)dispatcher.bind<DTtClose>(handlers.second);

    (void)dispatcher.fire<DTtOpen>();
    test_clock::advance(DMsec(40));
    (void)dispatcher.fire<DTtOpen>();
    test_clock::advance(DMsec(10));
    (void)dispatcher.fire<DTtClose>();
    (void)dispatcher.fire<DTtClose>();
    D_TT_CHECK(!t.running());
    D_TT_CHECK(t.elapsed() == DMsec(50));

    test_clock::advance(DMsec(500));
    (void)dispatcher.fire<DTtOpen>();
    test_clock::advance(DMsec(5));
    (void)dispatcher.fire<DTtClose>();
    D_TT_CHECK(t.elapsed() == DMsec(55));

    return true;
}


/*
tests_test_timer_event_unbind
  Verifies the dispatcher's handler ids unbind both handlers.
  Tests the following:
  - after unbinding, neither event moves the timer
*/
bool tests_test_timer_event_unbind()
{
    reset_clock();

    DEventDispatcher dispatcher;
    tt               t;
    const auto       handlers = test::make_test_timer_handlers(t);
    const auto       open     = dispatcher.bind<DTtOpen>(handlers.first);
    const auto       close    = dispatcher.bind<DTtClose>(handlers.second);

    D_TT_CHECK(dispatcher.unbind(open));
    D_TT_CHECK(dispatcher.unbind(close));
    (void)dispatcher.fire<DTtOpen>();
    test_clock::advance(DMsec(30));
    (void)dispatcher.fire<DTtClose>();

    D_TT_CHECK(!t.running());
    D_TT_CHECK(t.elapsed() == DMsec(0));

    return true;
}


NS_END  // testing
NS_END  // djinterp
