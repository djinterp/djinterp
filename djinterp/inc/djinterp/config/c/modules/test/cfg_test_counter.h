/*******************************************************************************
* djinterp [c][test]                                          cfg_test_counter.h
*
*   Build-time configuration for DTest's counters (c/modules/test/
* test_counter.h, and the C++ face's test_counter.hpp): the defaults every
* test counter and tally starts from.
*
*   targets:  c/modules/test/test_counter.h  -> D_INTERNAL_TEST_TALLY_*,
*                                               D_INTERNAL_TEST_COUNTER_*
*             cpp/modules/test/test_counter.hpp (the counter defaults)
*   requires: cfg_common.h (helpers)
*
* THREE LAYERS, EACH OPTIONAL
* ---------------------------
*   A knob here is the build's default. The run time can override it per tally
* or per counter through the options structs (struct dj_test_tally_options,
* struct dj_test_counter_options), whose init functions read these knobs. And a
* program that runs DTest as an abstract machine can opt in, with
* D_CFG_TEST_COUNTER_MACHINE, to a schema of the same settings for
* c/option/option_env.h, its defaults pinned to these knobs, so text, a script,
* the environment or the command line can set them.
*   A build that sets nothing gets the generic counter's behaviour: unbounded
* counters that step by one, and a tally of 16 that refuses a 17th event.
*
* HOW TO USE
* ----------
*   Nothing to call: dconfig.h includes this file. Define a knob before that, or
* pass it with -D, to change its default.
*
*     cc -DD_CFG_TEST_TALLY_CAPACITY=32 -DD_CFG_TEST_COUNTER_FLOOR=1 ...
*
* OPTIONS
* -------
*   Each knob is a default the run can override: struct dj_test_counter_options
* and struct dj_test_tally_options in test_counter.h, and, with
* D_CFG_TEST_COUNTER_MACHINE, their option_env schema.
*
* CONFIGURATION
* -------------
*   D_CFG_TEST_TALLY_CAPACITY (16, 1 to 4096), D_CFG_TEST_TALLY_WHEN_FULL (0
* refuse, 1 overflow), D_CFG_TEST_COUNTER_STEP (1, not 0),
* D_CFG_TEST_COUNTER_FLOOR (0) and D_CFG_TEST_COUNTER_MACHINE (0); each is
* checked, and resolved as D_INTERNAL_TEST_*.
*
*
* path:      /inc/djinterp/config/c/modules/test/cfg_test_counter.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.30
*                                                            revised: 2026.11.10
*******************************************************************************/

#ifndef DJINTERP_CONFIG_C_TEST_CFG_TEST_COUNTER_H
#define DJINTERP_CONFIG_C_TEST_CFG_TEST_COUNTER_H 1

// djinterp
#include "../../../cfg_common.h"  // D_CFG_* helpers, user overrides


// D_CFG_TEST_TALLY_CAPACITY
//   brief: how many keys one struct dj_test_tally can hold. Default 16.
//   Each slot is a test counter and its binding, about a hundred bytes, so
// the default costs under two kilobytes a tally. It is storage, fixed at
// build time; a run may track fewer (dj_test_tally_options.limit), never more.
#ifndef D_CFG_TEST_TALLY_CAPACITY
#   define D_CFG_TEST_TALLY_CAPACITY 16
#endif

#if ( (D_CFG_NORM(D_CFG_TEST_TALLY_CAPACITY) < 1) ||                          \
      (D_CFG_NORM(D_CFG_TEST_TALLY_CAPACITY) > 4096) )
#   error "D_CFG_TEST_TALLY_CAPACITY must be between 1 and 4096"
#endif

// D_CFG_TEST_TALLY_WHEN_FULL
//   brief: what a full tally does with a key it cannot track. Default 0.
//   0 refuses it: tracking returns NULL and counting it fails, so the loss is
// visible. 1 counts it in the tally's overflow counter instead, so totals
// stay whole and the overflow says how much was not told apart.
#ifndef D_CFG_TEST_TALLY_WHEN_FULL
#   define D_CFG_TEST_TALLY_WHEN_FULL 0
#endif

#if ( (D_CFG_NORM(D_CFG_TEST_TALLY_WHEN_FULL) < 0) ||                         \
      (D_CFG_NORM(D_CFG_TEST_TALLY_WHEN_FULL) > 1) )
#   error "D_CFG_TEST_TALLY_WHEN_FULL must be 0 (refuse) or 1 (overflow)"
#endif

// D_CFG_TEST_COUNTER_STEP
//   brief: how far one recorded occurrence moves a test counter.
// Default 1. Not 0, which would count nothing; negative counts down.
#ifndef D_CFG_TEST_COUNTER_STEP
#   define D_CFG_TEST_COUNTER_STEP 1
#endif

#if (D_CFG_NORM(D_CFG_TEST_COUNTER_STEP) == 0)
#   error "D_CFG_TEST_COUNTER_STEP must not be 0"
#endif

// D_CFG_TEST_COUNTER_FLOOR
//   brief: hold test counters at or above zero (1), or leave them unbounded
// as the generic counter is (0, the default). A count of occurrences cannot be
// negative, so a build that only counts may want 1; one that also counts
// down to a balance wants 0.
#ifndef D_CFG_TEST_COUNTER_FLOOR
#   define D_CFG_TEST_COUNTER_FLOOR 0
#endif

#if ( (D_CFG_NORM(D_CFG_TEST_COUNTER_FLOOR) < 0) ||                           \
      (D_CFG_NORM(D_CFG_TEST_COUNTER_FLOOR) > 1) )
#   error "D_CFG_TEST_COUNTER_FLOOR must be 0 or 1"
#endif

// D_CFG_TEST_COUNTER_MACHINE
//   brief: compile the counters' abstract-machine schema (1), or leave it out
// (0, the default). Opt-in: it pulls in c/option/option_env.h, which needs a
// 64-bit integer, and a program that does not run DTest as a machine has no
// use for it.
#ifndef D_CFG_TEST_COUNTER_MACHINE
#   define D_CFG_TEST_COUNTER_MACHINE 0
#endif

#if ( (D_CFG_NORM(D_CFG_TEST_COUNTER_MACHINE) < 0) ||                         \
      (D_CFG_NORM(D_CFG_TEST_COUNTER_MACHINE) > 1) )
#   error "D_CFG_TEST_COUNTER_MACHINE must be 0 or 1"
#endif

// D_INTERNAL_TEST_TALLY_CAPACITY
//   brief: the resolved capacity.
#define D_INTERNAL_TEST_TALLY_CAPACITY    D_CFG_NORM(D_CFG_TEST_TALLY_CAPACITY)

// D_INTERNAL_TEST_TALLY_WHEN_FULL
//   brief: the resolved full-tally policy.
#define D_INTERNAL_TEST_TALLY_WHEN_FULL   D_CFG_NORM(D_CFG_TEST_TALLY_WHEN_FULL)

// D_INTERNAL_TEST_COUNTER_STEP
//   brief: the resolved step.
#define D_INTERNAL_TEST_COUNTER_STEP      D_CFG_NORM(D_CFG_TEST_COUNTER_STEP)

// D_INTERNAL_TEST_COUNTER_FLOOR
//   brief: the resolved floor switch.
#define D_INTERNAL_TEST_COUNTER_FLOOR     D_CFG_NORM(D_CFG_TEST_COUNTER_FLOOR)

// D_INTERNAL_TEST_COUNTER_MACHINE
//   brief: the resolved machine switch.
#define D_INTERNAL_TEST_COUNTER_MACHINE   D_CFG_NORM(D_CFG_TEST_COUNTER_MACHINE)


#endif  // DJINTERP_CONFIG_C_TEST_CFG_TEST_COUNTER_H
