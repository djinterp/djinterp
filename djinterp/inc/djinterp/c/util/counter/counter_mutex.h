/*******************************************************************************
* djinterp [c][utility]                                          counter_mutex.h
*
* The generic counter, safe to share between threads: counter.h's counter
* behind a mutex. Every function is counter.h's, function for function, taken
* under the lock, so the two forms cannot drift.
*   counter_atomic.h is the lock-free form, and the one to reach for first. A
* mutex costs a lock per step but needs no 64-bit atomic, so this form exists
* wherever dmutex.h does -- ISO strict C++98 included -- and a caller that
* must hold a counter still across several steps can do so with snapshot.
*
*   Set up, then share; stop sharing, then destroy. init creates the mutex and
* can fail; destroy releases it. Neither is thread-safe.
*
* MERGING TWO SHARED COUNTERS
* ---------------------------
*   Merge reads its source under the source's lock and adds to its destination
* under the destination's, one lock at a time, so two threads merging two
* counters into each other cannot deadlock.
*
* HOW TO USE
* ----------
*   Set it up, use it from any thread, and destroy it; setting up creates the
* lock and can fail.
*
*     struct dj_counter_mutex hits;
*
*     if (d_counter_mutex_init(&hits, 0))
*     {
*         d_counter_mutex_increment(&hits, 1);
*         d_counter_mutex_destroy(&hits);
*     }
*
*   d_counter_mutex_snapshot copies it into a plain counter; d_counter_mutex_ref
* makes it any counter.
*
* OPTIONS
* -------
*   The start and the bounds, given at init (d_counter_mutex_init,
* d_counter_mutex_init_bounded).
*
* CONFIGURATION
* -------------
*   None: no D_CFG_* knob reaches this header.
*
*
* path:      /inc/djinterp/c/util/counter/counter_mutex.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.09
*                                                            revised: 2026.11.10
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  TYPES
    -----
    1.  The guarded counter
         1.  dj_counter_mutex
2.  OPERATIONS
    ----------
    1.  Lifetime
    2.  The five operations
    3.  Queries
    4.  Any counter
*/

#ifndef DJINTERP_C_UTIL_COUNTER_MUTEX_H
#define DJINTERP_C_UTIL_COUNTER_MUTEX_H 1

// djinterp
#include "../../djinterp.h"  // framework root
#include "../../sync/dmutex.h"  // dj_mutex_t
#include "./counter.h"          // dj_counter
// re_std
#include "../../../../re_std/c/cstdint/dstdint.h"  // int64_t


D_EXTERN_C_BEGIN


//==============================================================================
// 1.  TYPES
//==============================================================================


// 1.1    The guarded counter
//------------------------------------------------------------------------------
// 1.1.1
// dj_counter_mutex
//   struct: a counter and the mutex that guards it. Touch `counter` only
// through the functions below once the counter is shared.
struct dj_counter_mutex
{
    struct dj_counter counter;  // the guarded counter
    dj_mutex_t        lock;     // held for every operation
    bool              ready;    // whether `lock` was created
};


//==============================================================================
// 2.  OPERATIONS
//==============================================================================
// counter.h's contracts, function for function. A counter whose init failed,
// or that was destroyed, is not ready: operations on it do nothing and report
// failure, and queries answer as for NULL.


// 2.1    Lifetime
//------------------------------------------------------------------------------
// None is thread-safe: a counter is set up before it is shared, and destroyed
// after it no longer is.
/**
 * @brief Sets up a guarded counter at `_initial` with no bounds.
 *
 * @param[out] _counter  the counter to set up.
 * @param[in]  _initial  the start.
 * @return `true` when ready; `false` when `_counter` is `NULL` or the mutex
 *         could not be created.
 * @post On success the counter must be released with
 *       d_counter_mutex_destroy.
 */
bool
d_counter_mutex_init(
    struct dj_counter_mutex* _counter,
    int64_t                  _initial
);

/**
 * @brief Sets up a guarded counter at `_initial` within `_bounds`.
 *
 * @param[out] _counter  the counter to set up.
 * @param[in]  _initial  the start; must lie within `_bounds`.
 * @param[in]  _bounds   the range; `NULL` means none. Refused as
 *                       d_counter_init_bounded refuses it.
 * @return `true` when ready; `false` when `_counter` is `NULL`,
 *         d_counter_init_bounded would refuse the arguments, or the mutex
 *         could not be created.
 * @post On success the counter must be released with
 *       d_counter_mutex_destroy.
 */
bool
d_counter_mutex_init_bounded(
    struct dj_counter_mutex*      _counter,
    int64_t                       _initial,
    const struct dj_interval_imax* _bounds
);

/**
 * @brief Releases a guarded counter's mutex.
 *
 * @param[in,out] _counter  the counter; `NULL`, or one not ready, is ignored.
 * @pre No thread is using the counter.
 * @post The counter is not ready.
 */
void
d_counter_mutex_destroy(
    struct dj_counter_mutex* _counter
);

// 2.2    The five operations
//------------------------------------------------------------------------------
/**
 * @brief Moves a guarded counter up by `_amount`, under its lock.
 *
 * @param[in,out] _counter  the counter.
 * @param[in]     _amount   how far to move it; negative moves it down.
 * @return as d_counter_increment; `false` too for a counter not ready.
 */
bool
d_counter_mutex_increment(
    struct dj_counter_mutex* _counter,
    int64_t                  _amount
);

/**
 * @brief Moves a guarded counter down by `_amount`, under its lock.
 *
 * @param[in,out] _counter  the counter.
 * @param[in]     _amount   how far to move it; negative moves it up.
 * @return as d_counter_decrement; `false` too for a counter not ready.
 */
bool
d_counter_mutex_decrement(
    struct dj_counter_mutex* _counter,
    int64_t                  _amount
);

/**
 * @brief Orders two guarded counters by value.
 *
 * @note Each counter is read under its own lock, one at a time.
 *
 * @param[in] _a  the first counter; may be `NULL`.
 * @param[in] _b  the second counter; may be `NULL`.
 * @return as d_counter_compare; a counter not ready orders as `NULL`.
 */
int
d_counter_mutex_compare(
    const struct dj_counter_mutex* _a,
    const struct dj_counter_mutex* _b
);

/**
 * @brief Adds `_source`'s movement since its start to `_destination`.
 *
 * @note One lock at a time: `_source` is read and released before
 *       `_destination` is locked.
 *
 * @param[in,out] _destination  the counter that gains the movement.
 * @param[in]     _source       the counter whose movement is added.
 * @return as d_counter_merge; `false` too when either is not ready.
 */
bool
d_counter_mutex_merge(
    struct dj_counter_mutex*       _destination,
    const struct dj_counter_mutex* _source
);

/**
 * @brief Adds a plain counter's movement since its start to `_destination`.
 *
 * @param[in,out] _destination  the guarded counter that gains the movement.
 * @param[in]     _source       the plain counter; the caller's, and not
 *                              locked.
 * @return as d_counter_merge; `false` too when `_destination` is not ready.
 */
bool
d_counter_mutex_merge_counter(
    struct dj_counter_mutex* _destination,
    const struct dj_counter* _source
);

/**
 * @brief Returns a guarded counter to its start, under its lock.
 *
 * @param[in,out] _counter  the counter; `NULL`, or one not ready, is ignored.
 */
void
d_counter_mutex_reset(
    struct dj_counter_mutex* _counter
);

// 2.3    Queries
//------------------------------------------------------------------------------
// Each reads the counter once, under its lock.
/**
 * @brief Returns a guarded counter's value.
 *
 * @param[in] _counter  the counter; may be `NULL`.
 * @return the value, or 0 for `NULL` or a counter not ready.
 */
int64_t
d_counter_mutex_value(
    const struct dj_counter_mutex* _counter
);

/**
 * @brief Returns a guarded counter's movement since its start.
 *
 * @param[in] _counter  the counter; may be `NULL`.
 * @return as d_counter_delta; 0 for a counter not ready.
 */
int64_t
d_counter_mutex_delta(
    const struct dj_counter_mutex* _counter
);

/**
 * @brief Whether a guarded counter stands at its lower bound.
 *
 * @param[in] _counter  the counter; may be `NULL`.
 * @return as d_counter_at_min; `false` for a counter not ready.
 */
bool
d_counter_mutex_at_min(
    const struct dj_counter_mutex* _counter
);

/**
 * @brief Whether a guarded counter stands at its upper bound.
 *
 * @param[in] _counter  the counter; may be `NULL`.
 * @return as d_counter_at_max; `false` for a counter not ready.
 */
bool
d_counter_mutex_at_max(
    const struct dj_counter_mutex* _counter
);

/**
 * @brief Copies a guarded counter, under its lock, into a plain one.
 *
 * @param[in]  _counter  the counter; may be `NULL`.
 * @param[out] _out      the copy; may be `NULL`.
 * @return `true` when copied; `false` when either argument is `NULL` or the
 *         counter is not ready.
 */
bool
d_counter_mutex_snapshot(
    const struct dj_counter_mutex* _counter,
    struct dj_counter*             _out
);

// 2.4    Any counter
//------------------------------------------------------------------------------
/**
 * @brief Refers to a guarded counter, for counter.h's d_counter_ref_*
 *        functions.
 *
 * @param[in] _counter  the counter; `NULL` gives a reference to no counter.
 * @return the reference.
 */
struct dj_counter_ref
d_counter_mutex_ref(
    struct dj_counter_mutex* _counter
);


D_EXTERN_C_END


#endif  // DJINTERP_C_UTIL_COUNTER_MUTEX_H
