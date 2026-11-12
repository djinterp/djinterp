/*******************************************************************************
* djinterp [c][utility]                                           timer_atomic.h
*
* The generic timer, safe to share between threads without a lock: timer.h's
* operations over a timer whose whole state is one atomic word, changed by
* compare-and-swap.
*   A timer's state is two things -- the time so far, and whether an interval
* is open -- and timer.h keeps them in separate fields. Here both are folded
* into one int64_t, so a start, a stop or a step is one CAS:
*
*     state = 2 * v + running
*
* where, stopped, v is the elapsed time, and, running, v is the elapsed time
* less the clock's reading when the interval opened, so that the time now is
* v plus the clock's reading now.
*
* WHAT THE FOLDING COSTS
* ----------------------
*   The word holds v in 63 bits, so elapsed time saturates at 2^62 - 1
* nanoseconds (about 146 years) rather than INT64_MAX, and the clock's readings
* must stay within that range of zero, as the monotonic clock's do. And because
* a running timer no longer keeps its closed intervals apart from its open one,
* decrement takes time from the whole -- open interval included -- where timer.h
* takes it from the closed intervals only, and a clock wound back below an
* interval's start takes time off where timer.h counts none; both stop at zero.
*
* CONFIGURATION IS FIXED BEFORE SHARING
* -------------------------------------
*   The limit and the clock are set by the init functions and set_clock, none of
* which is thread-safe; after that only the state word changes.
*
* HOW TO USE
* ----------
*   Set the limit and the clock first, then share it:
*
*     struct dj_timer_atomic busy;
*
*     d_timer_atomic_init(&busy);
*     d_timer_atomic_start(&busy);         // from any thread
*     d_timer_atomic_stop(&busy);
*
*   d_timer_atomic_snapshot copies it into a plain timer; d_timer_atomic_ref
* makes it any timer.
*
* OPTIONS
* -------
*   The limit (d_timer_atomic_init_limited) and the clock
* (d_timer_atomic_set_clock), both fixed before the timer is shared.
*
* CONFIGURATION
* -------------
*   None: no D_CFG_* knob reaches this header. It needs long long
* (D_ENV_HAS_LONG_LONG) and is empty without it.
*
*
* path:      /inc/djinterp/c/util/timer/timer_atomic.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.09
*                                                            revised: 2026.11.10
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  TYPES
    -----
    1.  The atomic timer
         1.  D_TIMER_ATOMIC_MAX
         2.  dj_timer_atomic
2.  OPERATIONS
    ----------
    1.  Initialization
    2.  Starting and stopping
    3.  The five operations
    4.  Queries
    5.  Any timer
*/

#ifndef DJINTERP_C_UTIL_TIMER_ATOMIC_H
#define DJINTERP_C_UTIL_TIMER_ATOMIC_H 1

// FLOOR: the state is a dj_atomic_llong, which exists only where long long
// does. Without it -- ISO strict C++98 -- this file is empty, rather than an
// error (round 2's rule), and timer_mutex.h is the shared timer.
#include "../../../env/c/env_long_long.h"  // D_ENV_HAS_LONG_LONG
#if D_ENV_HAS_LONG_LONG

// djinterp
#include "../../djinterp.h"  // framework root
#include "../../sync/datomic.h"  // dj_atomic_llong
#include "./timer.h"             // dj_timer, fn_timer_clock
// re_std
#include "../../../../re_std/c/cstdint/dstdint.h"  // int64_t


D_EXTERN_C_BEGIN


//==============================================================================
// 1.  TYPES
//==============================================================================


// 1.1    The atomic timer
//------------------------------------------------------------------------------
// 1.1.1
// D_TIMER_ATOMIC_MAX
//   constant: the most nanoseconds an atomic timer holds, 2^62 - 1: the
// largest v the state word can carry beside its running bit.
#define D_TIMER_ATOMIC_MAX  (INT64_MAX / 2)

// 1.1.2
// dj_timer_atomic
//   struct: the state word, and the configuration fixed before sharing.
// Touch `state` only through the functions below.
struct dj_timer_atomic
{
    dj_atomic_llong state;      // 2 * v + running; see the file's banner
    int64_t         limit;      // nanoseconds, when has_limit
    bool            has_limit;  // whether the timer can expire
    fn_timer_clock  clock;      // the clock; NULL is the monotonic clock
    void*           context;    // the clock's argument
};


//==============================================================================
// 2.  OPERATIONS
//==============================================================================
// timer.h's contracts, function for function, but for the costs the file's
// banner names: elapsed time saturates at D_TIMER_ATOMIC_MAX, and decrement
// may shorten an open interval. NULL is accepted everywhere.


// 2.1    Initialization
//------------------------------------------------------------------------------
// None is thread-safe: a timer is set up before it is shared.
/**
 * @brief Sets a stopped atomic timer at zero with no limit, on the monotonic
 *        clock.
 *
 * @param[out] _timer  the timer to set; `NULL` is ignored.
 */
void
d_timer_atomic_init(
    struct dj_timer_atomic* _timer
);

/**
 * @brief Sets a stopped atomic timer at zero that expires at `_limit`.
 *
 * @note A limit above D_TIMER_ATOMIC_MAX is accepted, and never reached.
 *
 * @param[out] _timer  the timer to set.
 * @param[in]  _limit  the limit in nanoseconds; not negative.
 * @return `true` when set; `false` when `_timer` is `NULL` or `_limit` is
 *         negative, and then nothing changes.
 */
bool
d_timer_atomic_init_limited(
    struct dj_timer_atomic* _timer,
    int64_t                 _limit
);

/**
 * @brief Changes the clock an atomic timer reads.
 *
 * @warning As d_timer_set_clock: change it only while the timer is stopped,
 *          since a running timer's state was read from the old clock.
 *
 * @param[in,out] _timer    the timer; `NULL` is ignored.
 * @param[in]     _clock    the clock; `NULL` restores the monotonic one.
 * @param[in]     _context  the clock's argument.
 */
void
d_timer_atomic_set_clock(
    struct dj_timer_atomic* _timer,
    fn_timer_clock          _clock,
    void*                   _context
);

// 2.2    Starting and stopping
//------------------------------------------------------------------------------
// Each is one successful CAS: of two threads starting a stopped timer, one
// starts it and the other is refused, as a second start is in timer.h.
/**
 * @brief Opens an interval at the clock's reading.
 *
 * @param[in,out] _timer  the timer.
 * @return as d_timer_start.
 */
bool
d_timer_atomic_start(
    struct dj_timer_atomic* _timer
);

/**
 * @brief Closes the open interval into the elapsed time.
 *
 * @param[in,out] _timer  the timer.
 * @return as d_timer_stop.
 */
bool
d_timer_atomic_stop(
    struct dj_timer_atomic* _timer
);

// 2.3    The five operations
//------------------------------------------------------------------------------
/**
 * @brief Adds `_amount` nanoseconds, in one CAS step.
 *
 * @param[in,out] _timer   the timer.
 * @param[in]     _amount  the nanoseconds; negative removes time.
 * @return `true` when the full amount applied; `false` when the result was
 *         cut at zero or at D_TIMER_ATOMIC_MAX, or `_timer` is `NULL`.
 */
bool
d_timer_atomic_increment(
    struct dj_timer_atomic* _timer,
    int64_t                 _amount
);

/**
 * @brief Removes `_amount` nanoseconds, in one CAS step.
 *
 * @note Running, the time comes off the whole, so the open interval may be
 *       shortened; timer.h takes it from the closed intervals only.
 *
 * @param[in,out] _timer   the timer.
 * @param[in]     _amount  the nanoseconds; negative adds time.
 * @return as d_timer_atomic_increment.
 */
bool
d_timer_atomic_decrement(
    struct dj_timer_atomic* _timer,
    int64_t                 _amount
);

/**
 * @brief Orders two atomic timers by elapsed time, each read once.
 *
 * @param[in] _a  the first timer; may be `NULL`.
 * @param[in] _b  the second timer; may be `NULL`.
 * @return as d_timer_compare.
 */
int
d_timer_atomic_compare(
    const struct dj_timer_atomic* _a,
    const struct dj_timer_atomic* _b
);

/**
 * @brief Adds `_source`'s elapsed time to `_destination`.
 *
 * @note `_source` is read once, so a running one contributes what it had
 *       timed then. A timer merged into itself doubles its time once.
 *
 * @param[in,out] _destination  the timer that gains the time.
 * @param[in]     _source       the timer whose elapsed time is added.
 * @return as d_timer_atomic_increment; `false` too when `_source` is `NULL`.
 */
bool
d_timer_atomic_merge(
    struct dj_timer_atomic*       _destination,
    const struct dj_timer_atomic* _source
);

/**
 * @brief Adds a plain timer's elapsed time to `_destination`.
 *
 * @param[in,out] _destination  the atomic timer that gains the time.
 * @param[in]     _source       the plain timer; the caller's, and not
 *                              synchronized.
 * @return as d_timer_atomic_increment; `false` too when `_source` is `NULL`.
 */
bool
d_timer_atomic_merge_timer(
    struct dj_timer_atomic* _destination,
    const struct dj_timer*  _source
);

/**
 * @brief Stops an atomic timer and returns it to zero, in one store.
 *
 * @param[in,out] _timer  the timer; `NULL` is ignored.
 * @post The timer is stopped at zero; its limit and clock are unchanged.
 */
void
d_timer_atomic_reset(
    struct dj_timer_atomic* _timer
);

// 2.4    Queries
//------------------------------------------------------------------------------
// Each reads the state once.
/**
 * @brief Returns an atomic timer's elapsed time.
 *
 * @param[in] _timer  the timer; may be `NULL`.
 * @return as d_timer_elapsed, saturating at D_TIMER_ATOMIC_MAX.
 */
int64_t
d_timer_atomic_elapsed(
    const struct dj_timer_atomic* _timer
);

/**
 * @brief Returns the time left before an atomic timer's limit.
 *
 * @param[in] _timer  the timer; may be `NULL`.
 * @return as d_timer_remaining.
 */
int64_t
d_timer_atomic_remaining(
    const struct dj_timer_atomic* _timer
);

/**
 * @brief Whether an atomic timer has an open interval.
 *
 * @param[in] _timer  the timer; may be `NULL`.
 * @return as d_timer_running.
 */
bool
d_timer_atomic_running(
    const struct dj_timer_atomic* _timer
);

/**
 * @brief Whether an atomic timer has reached its limit.
 *
 * @param[in] _timer  the timer; may be `NULL`.
 * @return as d_timer_expired.
 */
bool
d_timer_atomic_expired(
    const struct dj_timer_atomic* _timer
);

/**
 * @brief Copies an atomic timer into a plain one.
 *
 * @note A running copy holds the time so far as closed and opens its interval
 *       at the reading taken, so it reads on from there as the atomic timer
 *       does.
 *
 * @param[in]  _timer  the timer; may be `NULL`.
 * @param[out] _out    the copy; may be `NULL`.
 * @return `true` when copied; `false` when either argument is `NULL`.
 */
bool
d_timer_atomic_snapshot(
    const struct dj_timer_atomic* _timer,
    struct dj_timer*              _out
);

// 2.5    Any timer
//------------------------------------------------------------------------------
/**
 * @brief Refers to an atomic timer, for timer.h's d_timer_ref_* functions.
 *
 * @param[in] _timer  the timer; `NULL` gives a reference to no timer.
 * @return the reference.
 */
struct dj_timer_ref
d_timer_atomic_ref(
    struct dj_timer_atomic* _timer
);


D_EXTERN_C_END


#endif  // floor: D_ENV_HAS_LONG_LONG

#endif  // DJINTERP_C_UTIL_TIMER_ATOMIC_H
