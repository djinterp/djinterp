/*******************************************************************************
* djinterp [cpp][test]                         test_timer_tests_construction.cpp
*
*
* path:      /tests/djinterp/cpp/modules/test/test_timer_tests_construction.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.11.10
*******************************************************************************/
// djinterp
#include "test_timer_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_test_timer_default_ctor
  Verifies the default constructor's initial state.
  Tests the following:
  - no maximum limit is set; max() is zero
  - the timer is not running and has zero elapsed time
  - it owns no children and observes nothing
*/
bool tests_test_timer_default_ctor()
{
    reset_clock();

    tt t;

    D_TT_CHECK(t.has_max()        == false);
    D_TT_CHECK(t.max()            == DMsec(0));
    D_TT_CHECK(t.running()        == false);
    D_TT_CHECK(t.elapsed()        == DMsec(0));
    D_TT_CHECK(t.child_count()    == 0);
    D_TT_CHECK(t.observed_count() == 0);

    return true;
}


/*
tests_test_timer_max_ctor
  Verifies the max-duration constructor.
  Tests the following:
  - a maximum limit is set and reported by max()
*/
bool tests_test_timer_max_ctor()
{
    reset_clock();

    tt t(DMsec(100));

    D_TT_CHECK(t.has_max() == true);
    D_TT_CHECK(t.max()     == DMsec(100));
    D_TT_CHECK(t.running() == false);

    return true;
}


NS_END  // testing
NS_END  // djinterp
