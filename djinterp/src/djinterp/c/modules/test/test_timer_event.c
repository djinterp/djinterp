/*******************************************************************************
* djinterp [c][test]                                          test_timer_event.c
*
* DTest's timer tie-in: two timer_event.h bindings on one test timer.
*
*
* path:      /src/djinterp/c/modules/test/test_timer_event.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.10
*                                                            revised: 2026.11.10
*******************************************************************************/
#include "../../../../../inc/djinterp/c/modules/test/test_timer_event.h"  // corresponding header


/*
d_test_timer_event_init
  A START and a STOP on the plain timer the test timer wraps; d_timer_event_init
ignores a NULL binding, so either may be left out.
*/
void
d_test_timer_event_init(
    struct dj_timer_event* _start,
    struct dj_timer_event* _stop,
    struct dj_test_timer*  _timer
)
{
    const struct dj_timer_ref target =
        d_timer_ref(d_test_timer_as_timer(_timer));

    d_timer_event_init(_start,
                       target,
                       D_TIMER_EVENT_START,
                       0);
    d_timer_event_init(_stop,
                       target,
                       D_TIMER_EVENT_STOP,
                       0);

    return;
}
