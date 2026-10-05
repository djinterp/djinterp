/*******************************************************************************
* djinterp [config]                                             cfg_test_timer.h
*
*   Build-time configuration for the timer kernel.
*
*   targets:  test/test_timer_common.h -> D_INTERNAL_TEST_TIMER_*
*   requires: cfg_common.h
*
*   THERE IS NO KNOB FOR THE CLOCK, and that is the point: the clock is an
* argument, not a configuration. A build cannot choose a tick source, because
* choosing one is the caller's job at the moment they bind it -- which is what
* lets a parity body inject a counter and a production build inject a real
* clock from the same code.
*
*
* path:      /inc/djinterp/config/test/cfg_test_timer.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.30
*                                                            revised: 2026.09.20
*******************************************************************************/

#ifndef DJINTERP_CONFIG_TEST_CFG_TEST_TIMER_H
#define DJINTERP_CONFIG_TEST_CFG_TEST_TIMER_H 1

// djinterp
#include "../cfg_common.h"

// D_CFG_TEST_TIMER_MAX_DEPTH
//   brief: how deep a timer subtree may be walked. Default 64. Bounded for the
// same reason as the counter's: the child array is caller-provided, nothing
// stops a cycle, and a bounded walk turns a stack overflow into a truncated
// total.
#ifndef D_CFG_TEST_TIMER_MAX_DEPTH
#   define D_CFG_TEST_TIMER_MAX_DEPTH 64
#endif

// D_CFG_TEST_TIMER_ENFORCE_LIMIT
//   brief: detect the limit and fire D_TIMER_EVENT_LIMIT (1, the default), or
// let a timer accumulate past it (0).
//   The limit is still recorded on the struct with this off, so a build with it
// off and one with it on diverge visibly at the first overrun rather than
// silently agreeing.
#ifndef D_CFG_TEST_TIMER_ENFORCE_LIMIT
#   define D_CFG_TEST_TIMER_ENFORCE_LIMIT 1
#endif

// D_CFG_TEST_TIMER_EVENTS
//   brief: notify the observer on change (1, the default).
#ifndef D_CFG_TEST_TIMER_EVENTS
#   define D_CFG_TEST_TIMER_EVENTS 1
#endif

// D_CFG_TEST_TIMER_STRICT_STATE
//   brief: report D_TIMER_ALREADY when starting a running timer or stopping a
// stopped one (1, the default), or treat both as no-ops (0).
//   ON because a double-start is almost always a bookkeeping bug, and silently
// absorbing it is how a suite reports half the elapsed time it should.
#ifndef D_CFG_TEST_TIMER_STRICT_STATE
#   define D_CFG_TEST_TIMER_STRICT_STATE 1
#endif

#if !D_CFG_IS_BOOL(D_CFG_TEST_TIMER_ENFORCE_LIMIT)
#   error "D_CFG_TEST_TIMER_ENFORCE_LIMIT must be 0 or 1"
#endif
#if !D_CFG_IS_BOOL(D_CFG_TEST_TIMER_EVENTS)
#   error "D_CFG_TEST_TIMER_EVENTS must be 0 or 1"
#endif
#if !D_CFG_IS_BOOL(D_CFG_TEST_TIMER_STRICT_STATE)
#   error "D_CFG_TEST_TIMER_STRICT_STATE must be 0 or 1"
#endif
#if ( D_CFG_NORM(D_CFG_TEST_TIMER_MAX_DEPTH) < 1 )
#   error "D_CFG_TEST_TIMER_MAX_DEPTH must be at least 1"
#endif

#define D_INTERNAL_TEST_TIMER_MAX_DEPTH D_CFG_TEST_TIMER_MAX_DEPTH

#if ( D_CFG_IS_ON(D_CFG_TESTING) ||                                            \
      D_CFG_IS_ON(D_CFG_TEST_TIMER_ENFORCE_LIMIT) )
#   define D_INTERNAL_TEST_TIMER_LIMIT 1
#else
#   define D_INTERNAL_TEST_TIMER_LIMIT 0
#endif

#if ( D_CFG_IS_ON(D_CFG_TESTING) || D_CFG_IS_ON(D_CFG_TEST_TIMER_EVENTS) )
#   define D_INTERNAL_TEST_TIMER_EVENTS 1
#else
#   define D_INTERNAL_TEST_TIMER_EVENTS 0
#endif

#if ( D_CFG_IS_ON(D_CFG_TESTING) ||                                            \
      D_CFG_IS_ON(D_CFG_TEST_TIMER_STRICT_STATE) )
#   define D_INTERNAL_TEST_TIMER_STRICT 1
#else
#   define D_INTERNAL_TEST_TIMER_STRICT 0
#endif

#endif  // DJINTERP_CONFIG_TEST_CFG_TEST_TIMER_H
