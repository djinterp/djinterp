/*******************************************************************************
* djinterp [config]                                           cfg_test_counter.h
*
*   Build-time configuration for the counter kernel: how deep a tree may go,
* and whether the bounds and events are enforced at all.
*
*   targets:  test/test_counter_common.h -> D_INTERNAL_TEST_COUNTER_*
*             test/test_counter.h        -> D_INTERNAL_TEST_COUNTER_*
*   requires: cfg_common.h (helpers, D_CFG_TESTING)
*
*   THE STORAGE IS THE CALLER'S, so there is no capacity knob here -- a caller
* who wants more children passes a bigger array. What IS configured is the
* recursion bound, which the caller cannot express, and the two enforcement
* gates, which a release build may want to drop.
*
*
* path:      /inc/djinterp/config/test/cfg_test_counter.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.30
*                                                            revised: 2026.09.20
*******************************************************************************/

#ifndef DJINTERP_CONFIG_TEST_CFG_TEST_COUNTER_H
#define DJINTERP_CONFIG_TEST_CFG_TEST_COUNTER_H 1

// djinterp
#include "../cfg_common.h"

// D_CFG_TEST_COUNTER_MAX_DEPTH
//   brief: how deep a counter subtree may be walked before the walk stops.
// Default 64.
//   A bound is needed because the child array is caller-provided and nothing
// stops a caller pointing a counter's children at an array containing itself.
// The kernel cannot detect that cheaply, so it bounds the walk instead: a
// cycle costs a truncated total rather than a stack overflow, which is a much
// better failure. Raise it for a genuinely deep suite; there is no reason to
// lower it.
#ifndef D_CFG_TEST_COUNTER_MAX_DEPTH
#   define D_CFG_TEST_COUNTER_MAX_DEPTH 64
#endif

// D_CFG_TEST_COUNTER_ENFORCE_BOUNDS
//   brief: clamp to min/max and report CLAMPED / AT_LIMIT (1, the default), or
// let a counter run past its bounds (0).
//   OFF makes increment a plain add. The bounds are still recorded on the
// struct, so a parity record still shows what they WERE -- which means a build
// with this off and a build with it on diverge visibly at the first clamp
// rather than silently agreeing.
#ifndef D_CFG_TEST_COUNTER_ENFORCE_BOUNDS
#   define D_CFG_TEST_COUNTER_ENFORCE_BOUNDS 1
#endif

// D_CFG_TEST_COUNTER_EVENTS
//   brief: notify the observer on change (1, the default), or compile the
// callback out entirely (0).
//   OFF removes one null check and one indirect call per operation. It also
// removes an observable behaviour, so a suite that asserts event ordering
// cannot run against such a build -- which is why cfg_testing.h forces it back
// on.
#ifndef D_CFG_TEST_COUNTER_EVENTS
#   define D_CFG_TEST_COUNTER_EVENTS 1
#endif

#if !D_CFG_IS_BOOL(D_CFG_TEST_COUNTER_ENFORCE_BOUNDS)
#   error "D_CFG_TEST_COUNTER_ENFORCE_BOUNDS must be 0 or 1"
#endif
#if !D_CFG_IS_BOOL(D_CFG_TEST_COUNTER_EVENTS)
#   error "D_CFG_TEST_COUNTER_EVENTS must be 0 or 1"
#endif
#if ( D_CFG_NORM(D_CFG_TEST_COUNTER_MAX_DEPTH) < 1 )
#   error "D_CFG_TEST_COUNTER_MAX_DEPTH must be at least 1"
#endif

// D_INTERNAL_TEST_COUNTER_MAX_DEPTH / _BOUNDS / _EVENTS
//   brief: the resolved values the kernel reads. The two gates are forced ON
// in a test build: a parity suite against a counter that neither clamps nor
// notifies is not measuring the counter this framework ships.
#define D_INTERNAL_TEST_COUNTER_MAX_DEPTH   D_CFG_TEST_COUNTER_MAX_DEPTH

#if ( D_CFG_IS_ON(D_CFG_TESTING) ||                                            \
      D_CFG_IS_ON(D_CFG_TEST_COUNTER_ENFORCE_BOUNDS) )
#   define D_INTERNAL_TEST_COUNTER_BOUNDS 1
#else
#   define D_INTERNAL_TEST_COUNTER_BOUNDS 0
#endif

#if ( D_CFG_IS_ON(D_CFG_TESTING) || D_CFG_IS_ON(D_CFG_TEST_COUNTER_EVENTS) )
#   define D_INTERNAL_TEST_COUNTER_EVENTS 1
#else
#   define D_INTERNAL_TEST_COUNTER_EVENTS 0
#endif

#endif  // DJINTERP_CONFIG_TEST_CFG_TEST_COUNTER_H
