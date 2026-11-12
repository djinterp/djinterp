/*******************************************************************************
* djinterp [cpp][utility]                                counter_tests_event.cpp
*
* The counter suite's event tests: counter_handler, bound on a real
* DEventDispatcher, for each operation and each of the three counters.
*
*
* path:      /tests/djinterp/cpp/util/counter/counter_tests_event.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.09
*                                                            revised: 2026.11.10
*******************************************************************************/
// djinterp
#include "counter_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_counter_event_counts
  Counting an event.
  Tests the following:
  - each occurrence of the bound tag moves the counter by the amount
  - occurrences of another tag move nothing
*/
bool tests_counter_event_counts()
{
    DEventDispatcher dispatcher;
    DCtr             ticks(0);

    (void)dispatcher.bind<DEvCtrTick>(
        make_counter_handler(ticks,
                             DCounterEventOp::increment,
                             2));

    dispatcher.fire<DEvCtrTick>();
    dispatcher.fire<DEvCtrTick>();
    dispatcher.fire<DEvCtrAmount>(std::int64_t{40});

    D_CTR_CHECK(ticks.value() == 4);

    return true;
}

/*
tests_counter_event_operations
  The other operations.
  Tests the following:
  - decrement moves down by the amount
  - reset returns the counter to its start
  - the default operation is increment by one
*/
bool tests_counter_event_operations()
{
    DEventDispatcher dispatcher;
    DCtr             tally(10);
    DCtr             plain(0);

    (void)dispatcher.bind<DEvCtrTick>(
        make_counter_handler(tally,
                             DCounterEventOp::decrement,
                             3));
    (void)dispatcher.bind<DEvCtrTick>(make_counter_handler(plain));
    (void)dispatcher.bind<DEvCtrLabel>(
        make_counter_handler(tally,
                             DCounterEventOp::reset));

    dispatcher.fire<DEvCtrTick>();

    D_CTR_CHECK(tally.value() == 7);
    D_CTR_CHECK(plain.value() == 1);

    dispatcher.fire<DEvCtrLabel>("clear");

    D_CTR_CHECK(tally.value() == 10);

    return true;
}

/*
tests_counter_event_add_payload
  Amounts that travel with the event.
  Tests the following:
  - add moves by the payload's value, either sign
  - an event with no payload, or one that does not convert, adds nothing
*/
bool tests_counter_event_add_payload()
{
    DEventDispatcher dispatcher;
    DCtr             bytes(0);

    (void)dispatcher.bind<DEvCtrAmount>(
        make_counter_handler(bytes,
                             DCounterEventOp::add));
    (void)dispatcher.bind<DEvCtrTick>(
        make_counter_handler(bytes,
                             DCounterEventOp::add));
    (void)dispatcher.bind<DEvCtrLabel>(
        make_counter_handler(bytes,
                             DCounterEventOp::add));

    dispatcher.fire<DEvCtrAmount>(std::int64_t{40});
    dispatcher.fire<DEvCtrAmount>(std::int64_t{-15});
    dispatcher.fire<DEvCtrTick>();
    dispatcher.fire<DEvCtrLabel>("ignored");

    D_CTR_CHECK(bytes.value() == 25);

    return true;
}

/*
tests_counter_event_unbind
  Unbinding.
  Tests the following:
  - binding the handler returns the dispatcher's handler id
  - after unbinding, occurrences move nothing
*/
bool tests_counter_event_unbind()
{
    DEventDispatcher dispatcher;
    DCtr             ticks(0);

    const DHandlerId id = dispatcher.bind<DEvCtrTick>(
        make_counter_handler(ticks));

    dispatcher.fire<DEvCtrTick>();

    D_CTR_CHECK(dispatcher.contains(id));
    D_CTR_CHECK(dispatcher.unbind(id));

    dispatcher.fire<DEvCtrTick>();

    D_CTR_CHECK(ticks.value() == 1);

    return true;
}

/*
tests_counter_event_other_counters
  The thread-safe counters.
  Tests the following:
  - atomic_counter and mutex_counter bind as counter does
  - one event moves all three at once
*/
bool tests_counter_event_other_counters()
{
    DEventDispatcher             dispatcher;
    DCtr                         plain(0);
    atomic_counter<std::int64_t> atomic(0);
    mutex_counter<std::int64_t>  guarded(0);

    (void)dispatcher.bind<DEvCtrAmount>(
        make_counter_handler(plain,
                             DCounterEventOp::add));
    (void)dispatcher.bind<DEvCtrAmount>(
        make_counter_handler(atomic,
                             DCounterEventOp::add));
    (void)dispatcher.bind<DEvCtrAmount>(
        make_counter_handler(guarded,
                             DCounterEventOp::add));

    dispatcher.fire<DEvCtrAmount>(std::int64_t{6});

    D_CTR_CHECK(plain.value() == 6);
    D_CTR_CHECK(atomic.value() == 6);
    D_CTR_CHECK(guarded.value() == 6);

    return true;
}


NS_END  // testing
NS_END  // djinterp
