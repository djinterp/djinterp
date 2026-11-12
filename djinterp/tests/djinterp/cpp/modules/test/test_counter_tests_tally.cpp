/*******************************************************************************
* djinterp [cpp][test]                              test_counter_tests_tally.cpp
*
* The test_counter suite, block "tally": test_tally, test counters kept by
* event type.
*
*
* path:      /tests/djinterp/cpp/modules/test/test_counter_tests_tally.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.10
*                                                            revised: 2026.11.10
*******************************************************************************/
// djinterp
#include "test_counter_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_test_tally_track
  Verifies track() makes one counter per key, at zero, at a stable address.
  Tests the following:
  - the first track() of a key makes a counter at zero
  - a second track() of it returns the same counter
  - tracking more keys does not move the first one
*/
bool tests_test_tally_track()
{
    tl        tally;
    tc&       ticks = tally.track<DTcTick>();
    tc* const first = &ticks;

    D_TC_CHECK(ticks.value() == 0);
    D_TC_CHECK(&tally.track<DTcTick>() == first);
    (void)tally.track<DTcBytes>();
    (void)tally.track<DTcMiss>();
    D_TC_CHECK(&tally.track<DTcTick>() == first);
    D_TC_CHECK(tally.size() == 3);

    return true;
}


/*
tests_test_tally_increment_and_count
  Verifies record() and increment() track as they must, and count() and
  find() read without tracking.
  Tests the following:
  - record() of a new key tracks it and counts one step
  - increment() adds its amount
  - count() of a key not tracked is zero, and does not track it
  - find() returns nullptr for a key not tracked
*/
bool tests_test_tally_increment_and_count()
{
    tl tally;

    D_TC_CHECK(tally.record<DTcTick>());
    D_TC_CHECK(tally.increment<DTcTick>(2));
    D_TC_CHECK(tally.count<DTcTick>() == 3);
    D_TC_CHECK(tally.count<DTcMiss>() == 0);
    D_TC_CHECK(tally.find<DTcMiss>() == nullptr);
    D_TC_CHECK(tally.size() == 1);

    return true;
}


/*
tests_test_tally_merge
  Verifies merge() adds another tally's movement key by key, tracking new
  keys.
  Tests the following:
  - a shared key's counts add
  - a key only the source tracks is tracked and counted
  - a self-merge doubles each count once
*/
bool tests_test_tally_merge()
{
    tl into;
    tl from;

    (void)into.increment<DTcTick>(2);
    (void)from.increment<DTcTick>(3);
    (void)from.increment<DTcBytes>(7);

    D_TC_CHECK(into.merge(from));
    D_TC_CHECK(into.count<DTcTick>() == 5);
    D_TC_CHECK(into.count<DTcBytes>() == 7);
    D_TC_CHECK(into.merge(into));
    D_TC_CHECK(into.count<DTcTick>() == 10);
    D_TC_CHECK(into.size() == 2);

    return true;
}


/*
tests_test_tally_reset_and_clear
  Verifies reset() returns counts to zero and keeps keys; clear() forgets
  them; names report.
  Tests the following:
  - reset() zeroes every count and keeps size()
  - name() reports each key's name in tracking order, "" out of range
  - clear() empties the tally
*/
bool tests_test_tally_reset_and_clear()
{
    tl tally;

    (void)tally.increment<DTcTick>(4);
    (void)tally.increment<DTcBytes>(9);
    tally.reset();

    D_TC_CHECK(tally.count<DTcTick>() == 0);
    D_TC_CHECK(tally.count<DTcBytes>() == 0);
    D_TC_CHECK(tally.size() == 2);
    D_TC_CHECK(std::strcmp(tally.name(0), "DTcTick") == 0);
    D_TC_CHECK(std::strcmp(tally.name(1), "DTcBytes") == 0);
    D_TC_CHECK(std::strcmp(tally.name(2), "") == 0);

    tally.clear();

    D_TC_CHECK(tally.size() == 0);

    return true;
}


NS_END  // testing
NS_END  // djinterp
