/*******************************************************************************
* djinterp [c][test]                                          test_timer_tests.h
*
* DTest's suite for its C timer, test_timer.h, and its event tie-in,
* test_timer_event.h, in the C spec form: the test timer and the span
* bindings, one block each.
*   Every test drives its timers from a hand-wound clock, so every time it
* checks is exact.
*
*
* path:      /tests/djinterp/c/modules/test/test_timer_tests.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.11
*                                                            revised: 2026.11.11
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  FIXTURE
    -------
    1.  The hand-wound clock
         1.  dj_tests_test_timer_clock
         2.  d_tests_test_timer_clock_read
2.  TESTS
    -----
    1.  The test timer
    2.  Events
3.  SPEC
    ----
    1.  The provider
*/

#ifndef DJINTERP_TESTS_C_TEST_TIMER_TESTS_H
#define DJINTERP_TESTS_C_TEST_TIMER_TESTS_H 1

// djinterp
#include "../../../../../inc/djinterp/c/modules/test/test_suite.h"  // D_TEST_*
// tested
#include "../../../../../inc/djinterp/c/modules/test/test_timer.h"  // timer
#include "../../../../../inc/djinterp/c/modules/test/test_timer_event.h"
#include "../../../../../inc/djinterp/c/event/event_dispatcher_common.h"  // fire


D_EXTERN_C_BEGIN


//==============================================================================
// 1.  FIXTURE
//==============================================================================


// 1.1    The hand-wound clock
//------------------------------------------------------------------------------
// 1.1.1
// dj_tests_test_timer_clock
//   struct: a clock that reads whatever the test last set.
struct dj_tests_test_timer_clock
{
    int64_t now;  // the reading, in nanoseconds
};

// 1.1.2
// d_tests_test_timer_clock_read
//   function: the fn_timer_clock over a struct dj_tests_test_timer_clock.
D_STATIC_INLINE
int64_t
d_tests_test_timer_clock_read(
    void* _clock
)
{
    return ((const struct dj_tests_test_timer_clock*)_clock)->now;
}


//==============================================================================
// 2.  TESTS
//==============================================================================


// 2.1    The test timer
//------------------------------------------------------------------------------
bool
d_tests_test_timer_init(void);

bool
d_tests_test_timer_start_stop(void);

bool
d_tests_test_timer_operations(void);

bool
d_tests_test_timer_elapsed_ms(void);

bool
d_tests_test_timer_null(void);


// 2.2    Events
//------------------------------------------------------------------------------
bool
d_tests_test_timer_event_init(void);

bool
d_tests_test_timer_event_span(void);

bool
d_tests_test_timer_event_twice(void);

bool
d_tests_test_timer_event_null(void);


//==============================================================================
// 3.  SPEC
//==============================================================================


// 3.1    The provider
//------------------------------------------------------------------------------
#ifdef DTEST_SPEC_MODE

// d_tests_test_timer_timer_rows
//   constant: the test timer block's rows.
static const struct dj_test_case_spec d_tests_test_timer_timer_rows[] =
{
    { "d_tests_test_timer_init",
      "init and D_TEST_TIMER_INIT: stopped, at zero, on the monotonic clock",
      d_tests_test_timer_init },
    { "d_tests_test_timer_start_stop",
      "intervals accumulate on the timer's clock; a second start is refused",
      d_tests_test_timer_start_stop },
    { "d_tests_test_timer_operations",
      "the five operations are timer.h's, on the timer it wraps",
      d_tests_test_timer_operations },
    { "d_tests_test_timer_elapsed_ms",
      "elapsed_ms is the nanoseconds over a million",
      d_tests_test_timer_elapsed_ms },
    { "d_tests_test_timer_null",
      "every function takes NULL: queries answer 0, operations fail",
      d_tests_test_timer_null }
};

// d_tests_test_timer_event_rows
//   constant: the events block's rows.
static const struct dj_test_case_spec d_tests_test_timer_event_rows[] =
{
    { "d_tests_test_timer_event_init",
      "init aims a start and a stop binding at the test timer",
      d_tests_test_timer_event_init },
    { "d_tests_test_timer_event_span",
      "the start event opens an interval and the stop event closes it",
      d_tests_test_timer_event_span },
    { "d_tests_test_timer_event_twice",
      "a start that arrives twice, or a stray stop, corrupts nothing",
      d_tests_test_timer_event_twice },
    { "d_tests_test_timer_event_null",
      "no timer, or NULL bindings: init does nothing, firing passes",
      d_tests_test_timer_event_null }
};

// d_tests_test_timer_blocks
//   constant: the module's blocks.
static const struct dj_test_block_spec d_tests_test_timer_blocks[] =
{
    { "timer",
      "the test timer: timer.h's timer, as DTest's",
      d_tests_test_timer_timer_rows,
      sizeof(d_tests_test_timer_timer_rows) /
          sizeof(d_tests_test_timer_timer_rows[0]) },
    { "event",
      "test_timer_event.h's span bindings, on the event core",
      d_tests_test_timer_event_rows,
      sizeof(d_tests_test_timer_event_rows) /
          sizeof(d_tests_test_timer_event_rows[0]) }
};

// d_tests_test_timer_module
//   constant: the module spec.
static const struct dj_test_module_spec d_tests_test_timer_module =
{
    "test_timer",
    "c/modules/test: DTest's test timer and its events",
    d_tests_test_timer_blocks,
    sizeof(d_tests_test_timer_blocks) / sizeof(d_tests_test_timer_blocks[0])
};

// d_tests_test_timer_spec
//   function: the suite's provider.
D_STATIC_INLINE
const struct dj_test_module_spec*
d_tests_test_timer_spec(void)
{
    return &d_tests_test_timer_module;
}

#endif  // DTEST_SPEC_MODE


D_EXTERN_C_END


#endif  // DJINTERP_TESTS_C_TEST_TIMER_TESTS_H
