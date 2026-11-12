/*******************************************************************************
* djinterp [c][test]                                                test_timer.h
*
* DTest's timer for the C face: a wrapper over the generic timer
* (c/util/timer/timer.h), and nothing more.
*   A struct dj_test_timer begins with a struct dj_timer, so a pointer to one
* is a pointer to the other: through d_test_timer_as_timer it takes every
* timer.h function and the merge_timer of the shared forms (timer_atomic.h,
* timer_mutex.h). The functions below are timer.h's under DTest's names, plus
* one DTest reports with: the elapsed time in milliseconds.
*   Nothing here knows about events: a suite starts and stops a test timer by
* hand, or binds the handlers of test_timer_event.h, the tie-in.
*
* ZERO IS A TIMER
* ---------------
*   A zero-filled struct dj_test_timer, or one set from D_TEST_TIMER_INIT, is a
* stopped timer at zero on the monotonic clock.
*
* HOW TO USE
* ----------
*   A zero-filled test timer is stopped at zero on the monotonic clock:
*
*     struct dj_test_timer span = D_TEST_TIMER_INIT;
*
*     d_test_timer_start(&span);
*     // the work
*     d_test_timer_stop(&span);
*     d_test_timer_elapsed_ms(&span);       // for the report
*
*   test_timer_event.h times a span between two events instead.
*
* OPTIONS
* -------
*   The clock (d_test_timer_set_clock), which a test can drive by hand.
*
* CONFIGURATION
* -------------
*   None: no D_CFG_* knob reaches this header.
*
*
* path:      /inc/djinterp/c/modules/test/test_timer.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.09
*                                                            revised: 2026.11.10
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  TYPES
    -----
    1.  The test timer
         1.  dj_test_timer
         2.  D_TEST_TIMER_INIT
2.  OPERATIONS
    ----------
    1.  Initialization
    2.  Starting and stopping
    3.  The five operations
    4.  Queries
*/

#ifndef DJINTERP_C_TEST_TIMER_H
#define DJINTERP_C_TEST_TIMER_H 1

// djinterp
#include "../../djinterp.h"  // framework root
#include "../../util/timer/timer.h"  // dj_timer
// re_std
#include "../../../../re_std/c/cstdint/dstdint.h"  // int64_t


D_EXTERN_C_BEGIN


//==============================================================================
// 1.  TYPES
//==============================================================================


// 1.1    The test timer
//------------------------------------------------------------------------------
// 1.1.1
// dj_test_timer
//   struct: the generic timer, as DTest's.
struct dj_test_timer
{
    struct dj_timer timer;  // the wrapped timer
};

// 1.1.2
// D_TEST_TIMER_INIT
//   constant: an initializer for a stopped timer at zero on the monotonic
// clock.
#define D_TEST_TIMER_INIT  { D_TIMER_INIT }


//==============================================================================
// 2.  OPERATIONS
//==============================================================================
// timer.h's, function for function, with timer.h's contracts.


// 2.1    Initialization
//------------------------------------------------------------------------------
/**
 * @brief Sets a stopped test timer at zero, on the monotonic clock.
 *
 * @param[out] _timer  the timer to set; `NULL` is ignored.
 * @post `_timer` equals D_TEST_TIMER_INIT.
 */
void
d_test_timer_init(
    struct dj_test_timer* _timer
);

/**
 * @brief Changes the clock a test timer reads.
 *
 * @warning As d_timer_set_clock: change it only while the timer is stopped.
 *
 * @param[in,out] _timer    the timer; `NULL` is ignored.
 * @param[in]     _clock    the clock; `NULL` restores the monotonic one.
 * @param[in]     _context  the clock's argument.
 */
void
d_test_timer_set_clock(
    struct dj_test_timer* _timer,
    fn_timer_clock        _clock,
    void*                 _context
);

// 2.2    Starting and stopping
//------------------------------------------------------------------------------
/**
 * @brief Opens an interval at the clock's reading.
 *
 * @param[in,out] _timer  the timer.
 * @return as d_timer_start.
 */
bool
d_test_timer_start(
    struct dj_test_timer* _timer
);

/**
 * @brief Closes the open interval into the elapsed time.
 *
 * @param[in,out] _timer  the timer.
 * @return as d_timer_stop.
 */
bool
d_test_timer_stop(
    struct dj_test_timer* _timer
);

// 2.3    The five operations
//------------------------------------------------------------------------------
/**
 * @brief Adds `_amount` nanoseconds to the elapsed time.
 *
 * @param[in,out] _timer   the timer.
 * @param[in]     _amount  the nanoseconds; negative removes time.
 * @return as d_timer_increment.
 */
bool
d_test_timer_increment(
    struct dj_test_timer* _timer,
    int64_t               _amount
);

/**
 * @brief Removes `_amount` nanoseconds from the elapsed time.
 *
 * @param[in,out] _timer   the timer.
 * @param[in]     _amount  the nanoseconds; negative adds time.
 * @return as d_timer_decrement.
 */
bool
d_test_timer_decrement(
    struct dj_test_timer* _timer,
    int64_t               _amount
);

/**
 * @brief Orders two test timers by elapsed time.
 *
 * @param[in] _a  the first timer; may be `NULL`.
 * @param[in] _b  the second timer; may be `NULL`.
 * @return as d_timer_compare.
 */
int
d_test_timer_compare(
    const struct dj_test_timer* _a,
    const struct dj_test_timer* _b
);

/**
 * @brief Adds `_source`'s elapsed time to `_destination`.
 *
 * @param[in,out] _destination  the timer that gains the time.
 * @param[in]     _source       the timer whose elapsed time is added.
 * @return as d_timer_merge.
 */
bool
d_test_timer_merge(
    struct dj_test_timer*       _destination,
    const struct dj_test_timer* _source
);

/**
 * @brief Stops a test timer and returns it to zero.
 *
 * @param[in,out] _timer  the timer; `NULL` is ignored.
 */
void
d_test_timer_reset(
    struct dj_test_timer* _timer
);

// 2.4    Queries
//------------------------------------------------------------------------------
/**
 * @brief Returns a test timer's elapsed time in nanoseconds.
 *
 * @param[in] _timer  the timer; may be `NULL`.
 * @return as d_timer_elapsed.
 */
int64_t
d_test_timer_elapsed(
    const struct dj_test_timer* _timer
);

/**
 * @brief Returns a test timer's elapsed time in milliseconds, for reports.
 *
 * @param[in] _timer  the timer; may be `NULL`.
 * @return the nanoseconds divided by 1e6, or 0 for `NULL`.
 */
double
d_test_timer_elapsed_ms(
    const struct dj_test_timer* _timer
);

/**
 * @brief Whether a test timer has an open interval.
 *
 * @param[in] _timer  the timer; may be `NULL`.
 * @return as d_timer_running.
 */
bool
d_test_timer_running(
    const struct dj_test_timer* _timer
);

/**
 * @brief Returns a test timer as the generic timer it wraps.
 *
 * @param[in] _timer  the timer; may be `NULL`.
 * @return `&_timer->timer`, for timer.h's functions, or `NULL` for `NULL`.
 */
struct dj_timer*
d_test_timer_as_timer(
    struct dj_test_timer* _timer
);


D_EXTERN_C_END


#endif  // DJINTERP_C_TEST_TIMER_H
