/*******************************************************************************
* djinterp [cpp][test]                              test_timer_tests_reports.cpp
*
* The test_timer suite, block "reports": DTest's addition to the generic
* timer, elapsed_ms().
*
*
* path:      /tests/djinterp/cpp/modules/test/test_timer_tests_reports.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.10
*                                                            revised: 2026.11.10
*******************************************************************************/
// djinterp
#include "test_timer_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_test_timer_elapsed_ms
  Verifies elapsed_ms() is elapsed() in milliseconds.
  Tests the following:
  - 1250ms of closed time reads as 1250.0
  - a running timer's open interval is included
*/
bool tests_test_timer_elapsed_ms()
{
    reset_clock();

    tt t;

    t.start();
    test_clock::advance(DMsec(1250));
    t.stop();
    D_TT_CHECK(t.elapsed_ms() == 1250.0);

    t.start();
    test_clock::advance(DMsec(50));
    D_TT_CHECK(t.elapsed_ms() == 1300.0);

    return true;
}


NS_END  // testing
NS_END  // djinterp
