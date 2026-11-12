/*******************************************************************************
* djinterp [c][test]                                          test_timer_event.h
*
* The event tie-in for DTest's timer: a start and a stop handler on one test
* timer, so the span between two events is timed. It couples the event core's
* handler (c/event's struct dj_event_step) to test_timer.h through
* c/util/timer/timer_event.h and does nothing else: the caller binds the
* handlers wherever handlers are bound. Neither test_timer.h nor DTest's
* runners include this file.
*
* THE CALLER OWNS THE BINDINGS
* ----------------------------
*   A step keeps a pointer to its binding, and the binding to the timer, so both
* must outlive every place the step is bound, or it is unbound first.
*
* HOW TO USE
* ----------
*   Aim two timer_event.h bindings at a test timer, then bind each to its own
* event:
*
*     struct dj_timer_event open;
*     struct dj_timer_event close;
*
*     d_test_timer_event_init(&open,
*                             &close,
*                             &span);
*     d_event_dispatcher_bind(&dispatcher,
*                             KEY_OPEN,
*                             d_timer_event_step(&open),
*                             NULL,
*                             &open_id);
*
* OPTIONS
* -------
*   None: nothing in this header is chosen at run time.
*
* CONFIGURATION
* -------------
*   None: no D_CFG_* knob reaches this header.
*
*
* path:      /inc/djinterp/c/modules/test/test_timer_event.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.10
*                                                            revised: 2026.11.10
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  OPERATIONS
    ----------
    1.  A span
*/

#ifndef DJINTERP_C_TEST_TIMER_EVENT_H
#define DJINTERP_C_TEST_TIMER_EVENT_H 1

// djinterp
#include "../../djinterp.h"                  // framework root
#include "../../util/timer/timer_event.h"    // dj_timer_event
#include "./test_timer.h"                    // dj_test_timer


D_EXTERN_C_BEGIN


//==============================================================================
// 1.  OPERATIONS
//==============================================================================


// 1.1    A span
//------------------------------------------------------------------------------
/**
 * @brief Aims two timer_event.h bindings at a test timer: `_start` starts
 *        it and `_stop` stops it.
 *
 * @note Hand each off with d_timer_event_step and bind it to its own event.
 *       A start while running, or a stop while stopped, does nothing, so an
 *       event that arrives twice does not corrupt the time.
 *
 * @param[out] _start  the binding that starts the timer; `NULL` is ignored.
 * @param[out] _stop   the binding that stops it; `NULL` is ignored.
 * @param[in]  _timer  the test timer; may be `NULL`, and then firing does
 *                     nothing.
 */
void
d_test_timer_event_init(
    struct dj_timer_event* _start,
    struct dj_timer_event* _stop,
    struct dj_test_timer*  _timer
);


D_EXTERN_C_END


#endif  // DJINTERP_C_TEST_TIMER_EVENT_H
