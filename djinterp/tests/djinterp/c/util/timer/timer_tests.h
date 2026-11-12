/*******************************************************************************
* djinterp [c][utility]                                            timer_tests.h
*
* The timer family's suite in DTest's C face (c/modules/test/test_suite.h):
* timer.h's timer, timer_mutex.h's guarded form, timer_event.h's bindings,
* timer_atomic.h's lock-free form, and timer.h's interface over any timer,
* one block each.
*   Every test but one drives its timers from a hand-wound clock, so every
* time it checks is exact; the one that reads the monotonic clock checks only
* that time does not run backwards.
*
*
* path:      /tests/djinterp/c/util/timer/timer_tests.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.09
*                                                            revised: 2026.11.11
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  FIXTURE
    -------
    1.  The hand-wound clock
         1.  dj_tests_timer_clock
         2.  d_tests_timer_clock_read
2.  TESTS
    -----
    1.  The timer
    2.  The guarded timer
    3.  Events
    4.  The atomic timer
    5.  Any timer
3.  SPEC
    ----
    1.  The provider
*/

#ifndef DJINTERP_TESTS_C_UTIL_TIMER_TESTS_H
#define DJINTERP_TESTS_C_UTIL_TIMER_TESTS_H 1

// djinterp
#include "../../../../../inc/djinterp/c/modules/test/test_suite.h"  // D_TEST_*
// tested
#include "../../../../../inc/djinterp/c/util/timer/timer.h"  // plain
#include "../../../../../inc/djinterp/c/util/timer/timer_mutex.h"  // locked
#include "../../../../../inc/djinterp/c/util/timer/timer_atomic.h"  // CAS
#include "../../../../../inc/djinterp/c/util/timer/timer_event.h"  // events
#include "../../../../../inc/djinterp/c/event/event_dispatcher_common.h"  // fire


D_EXTERN_C_BEGIN


//==============================================================================
// 1.  FIXTURE
//==============================================================================


// 1.1    The hand-wound clock
//------------------------------------------------------------------------------
// 1.1.1
// dj_tests_timer_clock
//   struct: a clock that reads whatever the test last set.
struct dj_tests_timer_clock
{
    int64_t now;  // the reading, in nanoseconds
};

// 1.1.2
// d_tests_timer_clock_read
//   function: the fn_timer_clock over a struct dj_tests_timer_clock.
D_STATIC_INLINE
int64_t
d_tests_timer_clock_read(
    void* _clock
)
{
    return ((const struct dj_tests_timer_clock*)_clock)->now;
}


//==============================================================================
// 2.  TESTS
//==============================================================================


// 2.1    The timer
//------------------------------------------------------------------------------
bool
d_tests_timer_zero(void);

bool
d_tests_timer_start_stop(void);

bool
d_tests_timer_step(void);

bool
d_tests_timer_compare(void);

bool
d_tests_timer_merge(void);

bool
d_tests_timer_reset(void);

bool
d_tests_timer_limit(void);

bool
d_tests_timer_clock(void);

bool
d_tests_timer_null(void);

// 2.2    The guarded timer
//------------------------------------------------------------------------------
bool
d_tests_timer_mutex_lifetime(void);

bool
d_tests_timer_mutex_operations(void);

bool
d_tests_timer_mutex_merge(void);

bool
d_tests_timer_mutex_threads(void);

bool
d_tests_timer_mutex_not_ready(void);

// 2.3    Events
//------------------------------------------------------------------------------
bool
d_tests_timer_event_init(void);

bool
d_tests_timer_event_span(void);

bool
d_tests_timer_event_operations(void);

bool
d_tests_timer_event_mutex(void);

bool
d_tests_timer_event_null(void);


// 2.4    The atomic timer
//------------------------------------------------------------------------------
bool
d_tests_timer_atomic_init(void);

bool
d_tests_timer_atomic_start_stop(void);

bool
d_tests_timer_atomic_step(void);

bool
d_tests_timer_atomic_merge(void);

bool
d_tests_timer_atomic_limit(void);

bool
d_tests_timer_atomic_threads(void);

bool
d_tests_timer_atomic_null(void);


// 2.5    Any timer
//------------------------------------------------------------------------------
bool
d_tests_timer_ref_make(void);

bool
d_tests_timer_ref_operations(void);

bool
d_tests_timer_ref_compare(void);

bool
d_tests_timer_ref_merge(void);

bool
d_tests_timer_ref_none(void);


//==============================================================================
// 3.  SPEC
//==============================================================================


// 3.1    The provider
//------------------------------------------------------------------------------
#ifdef DTEST_SPEC_MODE

// d_tests_timer_timer_rows
//   constant: the timer block's rows.
static const struct dj_test_case_spec d_tests_timer_timer_rows[] =
{
    { "d_tests_timer_zero",
      "a zero-filled timer is D_TIMER_INIT's: stopped, at zero, unlimited",
      d_tests_timer_zero },
    { "d_tests_timer_start_stop",
      "intervals accumulate, and a second start or stop is refused",
      d_tests_timer_start_stop },
    { "d_tests_timer_step",
      "increment and decrement add and remove time, held in [0, INT64_MAX]",
      d_tests_timer_step },
    { "d_tests_timer_compare",
      "three-way by elapsed time, a running timer's included, NULL first",
      d_tests_timer_compare },
    { "d_tests_timer_merge",
      "a merge adds the source's elapsed time, its open interval included",
      d_tests_timer_merge },
    { "d_tests_timer_reset",
      "reset stops and zeroes, and keeps the limit and the clock",
      d_tests_timer_reset },
    { "d_tests_timer_limit",
      "a limited timer expires at its limit and will not restart",
      d_tests_timer_limit },
    { "d_tests_timer_clock",
      "a clock wound back counts no time; the default clock runs forward",
      d_tests_timer_clock },
    { "d_tests_timer_null",
      "every function takes NULL: queries answer 0, operations fail",
      d_tests_timer_null }
};

// d_tests_timer_mutex_rows
//   constant: the guarded timer block's rows.
static const struct dj_test_case_spec d_tests_timer_mutex_rows[] =
{
    { "d_tests_timer_mutex_lifetime",
      "init makes a ready timer, with or without a limit; destroy unreadies",
      d_tests_timer_mutex_lifetime },
    { "d_tests_timer_mutex_operations",
      "every operation is timer.h's, under the lock",
      d_tests_timer_mutex_operations },
    { "d_tests_timer_mutex_merge",
      "merge from a guarded or a plain timer adds its elapsed time",
      d_tests_timer_mutex_merge },
    { "d_tests_timer_mutex_threads",
      "eight threads adding time at once lose none of it",
      d_tests_timer_mutex_threads },
    { "d_tests_timer_mutex_not_ready",
      "a timer not ready, or NULL, refuses operations and reads as zero",
      d_tests_timer_mutex_not_ready }
};

// d_tests_timer_event_rows
//   constant: the events block's rows.
static const struct dj_test_case_spec d_tests_timer_event_rows[] =
{
    { "d_tests_timer_event_init",
      "init aims one binding type at a timer of any kind",
      d_tests_timer_event_init },
    { "d_tests_timer_event_span",
      "START on one key and STOP on another time the span between them",
      d_tests_timer_event_span },
    { "d_tests_timer_event_operations",
      "increment, decrement, add-the-payload and reset, by event",
      d_tests_timer_event_operations },
    { "d_tests_timer_event_mutex",
      "a binding drives a guarded timer the same way",
      d_tests_timer_event_mutex },
    { "d_tests_timer_event_null",
      "no target, or a NULL binding, passes; the step is the hand-off",
      d_tests_timer_event_null }
};

// d_tests_timer_atomic_rows
//   constant: the atomic timer block's rows.
static const struct dj_test_case_spec d_tests_timer_atomic_rows[] =
{
    { "d_tests_timer_atomic_init",
      "init and init_limited; a snapshot is the same timer, plain",
      d_tests_timer_atomic_init },
    { "d_tests_timer_atomic_start_stop",
      "intervals accumulate, and a second start or stop is refused",
      d_tests_timer_atomic_start_stop },
    { "d_tests_timer_atomic_step",
      "decrement takes from an open interval; time tops out at the maximum",
      d_tests_timer_atomic_step },
    { "d_tests_timer_atomic_merge",
      "compare, merge from an atomic or plain timer or itself, and reset",
      d_tests_timer_atomic_merge },
    { "d_tests_timer_atomic_limit",
      "a limited timer expires at its limit and will not restart",
      d_tests_timer_atomic_limit },
    { "d_tests_timer_atomic_threads",
      "eight threads adding time at once lose none of it",
      d_tests_timer_atomic_threads },
    { "d_tests_timer_atomic_null",
      "every function takes NULL: queries answer 0, operations fail",
      d_tests_timer_atomic_null }
};

// d_tests_timer_ref_rows
//   constant: the any-timer block's rows.
static const struct dj_test_case_spec d_tests_timer_ref_rows[] =
{
    { "d_tests_timer_ref_make",
      "each kind's maker refers through its own full table; NULL is none",
      d_tests_timer_ref_make },
    { "d_tests_timer_ref_operations",
      "start, stop, steps, limit, snapshot and reset act the same on every kind",
      d_tests_timer_ref_operations },
    { "d_tests_timer_ref_compare",
      "three-way across kinds, no timer first",
      d_tests_timer_ref_compare },
    { "d_tests_timer_ref_merge",
      "every kind merges every kind, itself included",
      d_tests_timer_ref_merge },
    { "d_tests_timer_ref_none",
      "no timer, or one not set up: operations fail, queries answer 0",
      d_tests_timer_ref_none }
};

// d_tests_timer_blocks
//   constant: the module's blocks.
static const struct dj_test_block_spec d_tests_timer_blocks[] =
{
    { "timer",
      "timer.h's timer and its five operations",
      d_tests_timer_timer_rows,
      sizeof(d_tests_timer_timer_rows) /
          sizeof(d_tests_timer_timer_rows[0]) },
    { "mutex",
      "timer_mutex.h's guarded timer",
      d_tests_timer_mutex_rows,
      sizeof(d_tests_timer_mutex_rows) /
          sizeof(d_tests_timer_mutex_rows[0]) },
    { "event",
      "timer_event.h's handlers, bound on the event core",
      d_tests_timer_event_rows,
      sizeof(d_tests_timer_event_rows) /
          sizeof(d_tests_timer_event_rows[0]) },
    { "atomic",
      "timer_atomic.h's lock-free timer",
      d_tests_timer_atomic_rows,
      sizeof(d_tests_timer_atomic_rows) /
          sizeof(d_tests_timer_atomic_rows[0]) },
    { "ref",
      "timer.h's interface over any timer",
      d_tests_timer_ref_rows,
      sizeof(d_tests_timer_ref_rows) /
          sizeof(d_tests_timer_ref_rows[0]) }
};

// d_tests_timer_module
//   constant: the module spec.
static const struct dj_test_module_spec d_tests_timer_module =
{
    "timer",
    "c/util/timer: the generic timer, guarded, by event, atomic and by ref",
    d_tests_timer_blocks,
    sizeof(d_tests_timer_blocks) / sizeof(d_tests_timer_blocks[0])
};

// d_tests_timer_spec
//   function: the suite's provider.
D_STATIC_INLINE
const struct dj_test_module_spec*
d_tests_timer_spec(void)
{
    return &d_tests_timer_module;
}

#endif  // DTEST_SPEC_MODE


D_EXTERN_C_END


#endif  // DJINTERP_TESTS_C_UTIL_TIMER_TESTS_H
