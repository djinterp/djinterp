/*******************************************************************************
* djinterp [c][utility]                                            timer_mutex.h
*
* The generic timer, safe to share between threads: timer.h's timer behind a
* mutex. Every function is timer.h's, function for function, taken under the
* lock, so the two forms cannot drift.
*   A mutex rather than atomics, because a timer's state is several fields
* that change together -- a stop reads the clock and moves both `elapsed` and
* `running` -- and a lock is the honest way to make that one step.
*
*   Set up, then share; stop sharing, then destroy. init creates the mutex and
* can fail; destroy releases it. Neither is thread-safe.
*
* MERGING TWO SHARED TIMERS
* -------------------------
*   Merge reads its source under the source's lock and adds to its destination
* under the destination's, one lock at a time, so two threads merging two timers
* into each other cannot deadlock.
*
* HOW TO USE
* ----------
*   Set it up, use it from any thread, and destroy it:
*
*     struct dj_timer_mutex busy;
*
*     if (d_timer_mutex_init(&busy))
*     {
*         d_timer_mutex_start(&busy);
*         d_timer_mutex_stop(&busy);
*         d_timer_mutex_destroy(&busy);
*     }
*
*   d_timer_mutex_ref makes it any timer.
*
* OPTIONS
* -------
*   The limit (d_timer_mutex_init_limited) and the clock
* (d_timer_mutex_set_clock, under the lock).
*
* CONFIGURATION
* -------------
*   None: no D_CFG_* knob reaches this header.
*
*
* path:      /inc/djinterp/c/util/timer/timer_mutex.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.09
*                                                            revised: 2026.11.10
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  TYPES
    -----
    1.  The guarded timer
         1.  dj_timer_mutex
2.  OPERATIONS
    ----------
    1.  Lifetime
    2.  Starting and stopping
    3.  The five operations
    4.  Queries
    5.  Any timer
*/

#ifndef DJINTERP_C_UTIL_TIMER_MUTEX_H
#define DJINTERP_C_UTIL_TIMER_MUTEX_H 1

// djinterp
#include "../../djinterp.h"  // framework root
#include "../../sync/dmutex.h"   // dj_mutex_t
#include "./timer.h"             // dj_timer, fn_timer_clock
// re_std
#include "../../../../re_std/c/cstdint/dstdint.h"  // int64_t


D_EXTERN_C_BEGIN


//==============================================================================
// 1.  TYPES
//==============================================================================


// 1.1    The guarded timer
//------------------------------------------------------------------------------
// 1.1.1
// dj_timer_mutex
//   struct: a timer and the mutex that guards it. Touch `timer` only through
// the functions below once the timer is shared.
struct dj_timer_mutex
{
    struct dj_timer timer;  // the guarded timer
    dj_mutex_t      lock;   // held for every operation
    bool            ready;  // whether `lock` was created
};


//==============================================================================
// 2.  OPERATIONS
//==============================================================================
// timer.h's contracts, function for function. A timer whose init failed, or
// that was destroyed, is not ready: operations on it do nothing and report
// failure, and queries answer as for NULL.


// 2.1    Lifetime
//------------------------------------------------------------------------------
// None is thread-safe: a timer is set up before it is shared, and destroyed
// after it no longer is.
/**
 * @brief Sets up a guarded stopped timer at zero with no limit.
 *
 * @param[out] _timer  the timer to set up.
 * @return `true` when ready; `false` when `_timer` is `NULL` or the mutex
 *         could not be created.
 * @post On success the timer must be released with d_timer_mutex_destroy.
 */
bool
d_timer_mutex_init(
    struct dj_timer_mutex* _timer
);

/**
 * @brief Sets up a guarded stopped timer at zero that expires at `_limit`.
 *
 * @param[out] _timer  the timer to set up.
 * @param[in]  _limit  the limit in nanoseconds; not negative.
 * @return `true` when ready; `false` when `_timer` is `NULL`, `_limit` is
 *         negative, or the mutex could not be created.
 * @post On success the timer must be released with d_timer_mutex_destroy.
 */
bool
d_timer_mutex_init_limited(
    struct dj_timer_mutex* _timer,
    int64_t                _limit
);

/**
 * @brief Releases a guarded timer's mutex.
 *
 * @param[in,out] _timer  the timer; `NULL`, or one not ready, is ignored.
 * @pre No thread is using the timer.
 * @post The timer is not ready.
 */
void
d_timer_mutex_destroy(
    struct dj_timer_mutex* _timer
);

/**
 * @brief Changes the clock a guarded timer reads, under its lock.
 *
 * @warning As d_timer_set_clock: change it only while the timer is stopped.
 *
 * @param[in,out] _timer    the timer; `NULL`, or one not ready, is ignored.
 * @param[in]     _clock    the clock; `NULL` restores the monotonic one.
 * @param[in]     _context  the clock's argument.
 */
void
d_timer_mutex_set_clock(
    struct dj_timer_mutex* _timer,
    fn_timer_clock         _clock,
    void*                  _context
);

// 2.2    Starting and stopping
//------------------------------------------------------------------------------
/**
 * @brief Opens an interval, under the lock.
 *
 * @param[in,out] _timer  the timer.
 * @return as d_timer_start; `false` too for a timer not ready.
 */
bool
d_timer_mutex_start(
    struct dj_timer_mutex* _timer
);

/**
 * @brief Closes the open interval, under the lock.
 *
 * @param[in,out] _timer  the timer.
 * @return as d_timer_stop; `false` too for a timer not ready.
 */
bool
d_timer_mutex_stop(
    struct dj_timer_mutex* _timer
);

// 2.3    The five operations
//------------------------------------------------------------------------------
/**
 * @brief Adds `_amount` nanoseconds, under the lock.
 *
 * @param[in,out] _timer   the timer.
 * @param[in]     _amount  the nanoseconds; negative removes time.
 * @return as d_timer_increment; `false` too for a timer not ready.
 */
bool
d_timer_mutex_increment(
    struct dj_timer_mutex* _timer,
    int64_t                _amount
);

/**
 * @brief Removes `_amount` nanoseconds, under the lock.
 *
 * @param[in,out] _timer   the timer.
 * @param[in]     _amount  the nanoseconds; negative adds time.
 * @return as d_timer_decrement; `false` too for a timer not ready.
 */
bool
d_timer_mutex_decrement(
    struct dj_timer_mutex* _timer,
    int64_t                _amount
);

/**
 * @brief Orders two guarded timers by elapsed time.
 *
 * @note Each timer is read under its own lock, one at a time.
 *
 * @param[in] _a  the first timer; may be `NULL`.
 * @param[in] _b  the second timer; may be `NULL`.
 * @return as d_timer_compare; a timer not ready orders as `NULL`.
 */
int
d_timer_mutex_compare(
    const struct dj_timer_mutex* _a,
    const struct dj_timer_mutex* _b
);

/**
 * @brief Adds `_source`'s elapsed time to `_destination`.
 *
 * @note One lock at a time: `_source` is read and released before
 *       `_destination` is locked.
 *
 * @param[in,out] _destination  the timer that gains the time.
 * @param[in]     _source       the timer whose elapsed time is added.
 * @return as d_timer_merge; `false` too when either is not ready.
 */
bool
d_timer_mutex_merge(
    struct dj_timer_mutex*       _destination,
    const struct dj_timer_mutex* _source
);

/**
 * @brief Adds a plain timer's elapsed time to `_destination`.
 *
 * @note This is how a thread's own timer is folded into a shared total.
 *
 * @param[in,out] _destination  the guarded timer that gains the time.
 * @param[in]     _source       the plain timer; the caller's, and not locked.
 * @return as d_timer_merge; `false` too when `_destination` is not ready.
 */
bool
d_timer_mutex_merge_timer(
    struct dj_timer_mutex* _destination,
    const struct dj_timer* _source
);

/**
 * @brief Stops a guarded timer and returns it to zero, under the lock.
 *
 * @param[in,out] _timer  the timer; `NULL`, or one not ready, is ignored.
 */
void
d_timer_mutex_reset(
    struct dj_timer_mutex* _timer
);

// 2.4    Queries
//------------------------------------------------------------------------------
// Each reads the timer once, under its lock.
/**
 * @brief Returns a guarded timer's elapsed time.
 *
 * @param[in] _timer  the timer; may be `NULL`.
 * @return as d_timer_elapsed; 0 for a timer not ready.
 */
int64_t
d_timer_mutex_elapsed(
    const struct dj_timer_mutex* _timer
);

/**
 * @brief Returns the time left before a guarded timer's limit.
 *
 * @param[in] _timer  the timer; may be `NULL`.
 * @return as d_timer_remaining; 0 for a timer not ready.
 */
int64_t
d_timer_mutex_remaining(
    const struct dj_timer_mutex* _timer
);

/**
 * @brief Whether a guarded timer has an open interval.
 *
 * @param[in] _timer  the timer; may be `NULL`.
 * @return as d_timer_running; `false` for a timer not ready.
 */
bool
d_timer_mutex_running(
    const struct dj_timer_mutex* _timer
);

/**
 * @brief Whether a guarded timer has reached its limit.
 *
 * @param[in] _timer  the timer; may be `NULL`.
 * @return as d_timer_expired; `false` for a timer not ready.
 */
bool
d_timer_mutex_expired(
    const struct dj_timer_mutex* _timer
);

/**
 * @brief Copies a guarded timer, under its lock, into a plain one.
 *
 * @param[in]  _timer  the timer; may be `NULL`.
 * @param[out] _out    the copy; may be `NULL`.
 * @return `true` when copied; `false` when either argument is `NULL` or the
 *         timer is not ready.
 */
bool
d_timer_mutex_snapshot(
    const struct dj_timer_mutex* _timer,
    struct dj_timer*             _out
);

// 2.5    Any timer
//------------------------------------------------------------------------------
/**
 * @brief Refers to a guarded timer, for timer.h's d_timer_ref_* functions.
 *
 * @param[in] _timer  the timer; `NULL` gives a reference to no timer.
 * @return the reference.
 */
struct dj_timer_ref
d_timer_mutex_ref(
    struct dj_timer_mutex* _timer
);


D_EXTERN_C_END


#endif  // DJINTERP_C_UTIL_TIMER_MUTEX_H
