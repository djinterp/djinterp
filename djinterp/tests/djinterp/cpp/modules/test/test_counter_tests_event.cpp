/*******************************************************************************
* djinterp [cpp][test]                              test_counter_tests_event.cpp
*
* The test_counter suite, block "event": test_counter_event.hpp's handlers,
* bound on a dispatcher, moving a test counter or a tally key per occurrence.
*
*
* path:      /tests/djinterp/cpp/modules/test/test_counter_tests_event.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.10
*                                                            revised: 2026.11.10
*******************************************************************************/
// djinterp
#include "test_counter_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_test_counter_event_each_occurrence
  Verifies make_test_counter_handler moves a test counter by its step on each
  occurrence of the event it is bound to, and no other.
  Tests the following:
  - three occurrences count three steps
  - an event the handler is not bound to moves nothing
*/
bool tests_test_counter_event_each_occurrence()
{
    DEventDispatcher dispatcher;
    tc               ticks(tc::options_type{2, false});

    (void)dispatcher.bind<DTcTick>(test::make_test_counter_handler(ticks));
    (void)dispatcher.fire<DTcTick>();
    (void)dispatcher.fire<DTcTick>();
    (void)dispatcher.fire<DTcTick>();
    (void)dispatcher.fire<DTcMiss>();

    D_TC_CHECK(ticks.value() == 6);

    return true;
}


/*
tests_test_counter_event_operations
  Verifies counter_event.hpp's handler on test counters, which are counters.
  Tests the following:
  - decrement by a fixed amount, on each occurrence
  - add, by the occurrence's payload
  - reset, back to the start
*/
bool tests_test_counter_event_operations()
{
    DEventDispatcher dispatcher;
    tc               down(100);
    tc               bytes;
    tc               cleared(5);

    (void)dispatcher.bind<DTcTick>(
        make_counter_handler(down,
                             DCounterEventOp::decrement,
                             10));
    (void)dispatcher.bind<DTcBytes>(
        make_counter_handler(bytes,
                             DCounterEventOp::add));
    (void)dispatcher.bind<DTcMiss>(
        test::make_test_counter_handler(cleared,
                                        DCounterEventOp::reset));
    (void)cleared.increment(4);

    (void)dispatcher.fire<DTcTick>();
    (void)dispatcher.fire<DTcBytes>(7);
    (void)dispatcher.fire<DTcBytes>(5);
    (void)dispatcher.fire<DTcMiss>();

    D_TC_CHECK(down.value() == 90);
    D_TC_CHECK(bytes.value() == 12);
    D_TC_CHECK(cleared.value() == 5);

    return true;
}


/*
tests_test_counter_event_unbind
  Verifies the dispatcher's handler id unbinds a test counter's handler.
  Tests the following:
  - occurrences after unbinding move nothing
*/
bool tests_test_counter_event_unbind()
{
    DEventDispatcher dispatcher;
    tc               ticks;
    const auto       id = dispatcher.bind<DTcTick>(
                              test::make_test_counter_handler(ticks));

    (void)dispatcher.fire<DTcTick>();
    D_TC_CHECK(dispatcher.unbind(id));
    (void)dispatcher.fire<DTcTick>();

    D_TC_CHECK(ticks.value() == ticks.step());

    return true;
}


/*
tests_test_tally_event
  Verifies make_tally_handler records a key per occurrence, tracking it as it
  must, and that a tally's counter takes counter_event.hpp's handler.
  Tests the following:
  - each occurrence records one step of the handler's key
  - the key need not be tracked before the first occurrence
  - payload amounts add under DCounterEventOp::add, on a tracked counter
*/
bool tests_test_tally_event()
{
    DEventDispatcher dispatcher;
    tl               tally;

    (void)dispatcher.bind<DTcTick>(test::make_tally_handler<DTcTick>(tally));
    (void)dispatcher.bind<DTcBytes>(
        make_counter_handler(tally.track<DTcBytes>(),
                             DCounterEventOp::add));
    D_TC_CHECK(tally.size() == 1);

    (void)dispatcher.fire<DTcTick>();
    (void)dispatcher.fire<DTcTick>();
    (void)dispatcher.fire<DTcBytes>(40);

    D_TC_CHECK(tally.count<DTcTick>() == 2);
    D_TC_CHECK(tally.count<DTcBytes>() == 40);
    D_TC_CHECK(tally.size() == 2);

    return true;
}


NS_END  // testing
NS_END  // djinterp
