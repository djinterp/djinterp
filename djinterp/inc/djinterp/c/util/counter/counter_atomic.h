/*******************************************************************************
* djinterp [c][utility]                                         counter_atomic.h
*
* The generic counter, safe to share between threads without a lock: the same
* value, bounds and five operations as counter.h, with the value held in an
* atomic and every step made by compare-and-swap.
*   The start and the bounds are fixed at init; only the value moves, which is
* what lets a step be one CAS. The arithmetic is counter.h's own
* d_counter_bounds_add, so an atomic counter saturates exactly as a plain
* one does. counter_mutex.h is the locked form, for where there is no 64-bit
* atomic or a caller must hold a counter still across several steps.
*
* MERGING ACROSS THE FORMS
* ------------------------
*   A thread may keep a plain struct dj_counter and merge it into a shared
* atomic one when it finishes (d_counter_atomic_merge_counter), which costs one
* CAS per merge rather than one per count.
*
* HOW TO USE
* ----------
*   Initialize it once, then share it; every step is one compare-and-swap.
*
*     struct dj_counter_atomic hits;
*
*     d_counter_atomic_init(&hits, 0);
*     d_counter_atomic_increment(&hits, 1);   // from any thread
*
*   A thread that counts a lot may keep a plain counter and add it once with
* d_counter_atomic_merge_counter. d_counter_atomic_ref makes it any counter, for
* counter.h's d_counter_ref_* functions.
*
* OPTIONS
* -------
*   The start and the bounds, fixed at init (d_counter_atomic_init,
* d_counter_atomic_init_bounded).
*
* CONFIGURATION
* -------------
*   None: no D_CFG_* knob reaches this header. It needs long long
* (D_ENV_HAS_LONG_LONG) and is empty without it.
*
*
* path:      /inc/djinterp/c/util/counter/counter_atomic.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.09
*                                                            revised: 2026.11.10
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  TYPES
    -----
    1.  The atomic counter
         1.  dj_counter_atomic
2.  OPERATIONS
    ----------
    1.  Initialization
    2.  The five operations
    3.  Queries
    4.  Any counter
*/

#ifndef DJINTERP_C_UTIL_COUNTER_ATOMIC_H
#define DJINTERP_C_UTIL_COUNTER_ATOMIC_H 1

// FLOOR: the value is a dj_atomic_llong, which exists only where long long
// does. Without it -- ISO strict C++98 -- this file is empty, rather than an
// error (round 2's rule), and counter_mutex.h is the shared counter.
#include "../../../env/c/env_long_long.h"  // D_ENV_HAS_LONG_LONG
#if D_ENV_HAS_LONG_LONG

// djinterp
#include "../../djinterp.h"  // framework root
#include "../../sync/datomic.h"  // dj_atomic_llong
#include "./counter.h"           // dj_counter, d_counter_bounds_add
// re_std
#include "../../../../re_std/c/cstdint/dstdint.h"  // int64_t


D_EXTERN_C_BEGIN


//==============================================================================
// 1.  TYPES
//==============================================================================


// 1.1    The atomic counter
//------------------------------------------------------------------------------
// 1.1.1
// dj_counter_atomic
//   struct: a counter whose value is atomic. `initial` and the bounds are set
// once, by an init function, before the counter is shared; after that only
// the value changes, and only through the functions below.
struct dj_counter_atomic
{
    int64_t                initial;     // the start, for reset and merge
    dj_atomic_llong        value;       // the count now
    struct d_interval_imax bounds;      // the range, when has_bounds
    bool                   has_bounds;  // whether bounds applies
};


//==============================================================================
// 2.  OPERATIONS
//==============================================================================
// counter.h's contracts, function for function: NULL is accepted everywhere,
// and a step returns `false` when it was cut at an edge.


// 2.1    Initialization
//------------------------------------------------------------------------------
// Neither is thread-safe: a counter is set up before it is shared.
/**
 * @brief Sets an atomic counter at `_initial` with no bounds.
 *
 * @param[out] _counter  the counter to set; `NULL` is ignored.
 * @param[in]  _initial  the start.
 */
void
d_counter_atomic_init(
    struct dj_counter_atomic* _counter,
    int64_t                   _initial
);

/**
 * @brief Sets an atomic counter at `_initial` within `_bounds`.
 *
 * @param[out] _counter  the counter to set.
 * @param[in]  _initial  the start; must lie within `_bounds`.
 * @param[in]  _bounds   the range; `NULL` means none. Refused as
 *                       d_counter_init_bounded refuses it.
 * @return `true` when set; `false` when d_counter_init_bounded would refuse
 *         the same arguments, and then nothing changes.
 */
bool
d_counter_atomic_init_bounded(
    struct dj_counter_atomic*     _counter,
    int64_t                       _initial,
    const struct d_interval_imax* _bounds
);

// 2.2    The five operations
//------------------------------------------------------------------------------
// Each step is one successful CAS, retried under contention.
/**
 * @brief Moves an atomic counter up by `_amount`, saturating at its bounds.
 *
 * @param[in,out] _counter  the counter.
 * @param[in]     _amount   how far to move it; negative moves it down.
 * @return `true` when the published step applied in full; `false` when it
 *         was cut at an edge, or `_counter` is `NULL`.
 */
bool
d_counter_atomic_increment(
    struct dj_counter_atomic* _counter,
    int64_t                   _amount
);

/**
 * @brief Moves an atomic counter down by `_amount`, saturating at its bounds.
 *
 * @param[in,out] _counter  the counter.
 * @param[in]     _amount   how far to move it; negative moves it up.
 * @return as d_counter_atomic_increment.
 */
bool
d_counter_atomic_decrement(
    struct dj_counter_atomic* _counter,
    int64_t                   _amount
);

/**
 * @brief Orders two atomic counters by value.
 *
 * @note Each value is loaded once, so the answer is true of two moments that
 *       may differ: the most a lock-free comparison can say.
 *
 * @param[in] _a  the first counter; may be `NULL`.
 * @param[in] _b  the second counter; may be `NULL`.
 * @return as d_counter_compare.
 */
int
d_counter_atomic_compare(
    const struct dj_counter_atomic* _a,
    const struct dj_counter_atomic* _b
);

/**
 * @brief Adds `_source`'s movement since its start to `_destination`.
 *
 * @note `_source` is read once, so one still being counted contributes what
 *       it held then. A counter merged into itself adds its movement once.
 *
 * @param[in,out] _destination  the counter that gains the movement.
 * @param[in]     _source       the counter whose movement is added.
 * @return as d_counter_merge.
 */
bool
d_counter_atomic_merge(
    struct dj_counter_atomic*       _destination,
    const struct dj_counter_atomic* _source
);

/**
 * @brief Adds a plain counter's movement since its start to `_destination`.
 *
 * @param[in,out] _destination  the atomic counter that gains the movement.
 * @param[in]     _source       the plain counter; the caller's, and not
 *                              synchronized.
 * @return as d_counter_merge.
 */
bool
d_counter_atomic_merge_counter(
    struct dj_counter_atomic* _destination,
    const struct dj_counter*  _source
);

/**
 * @brief Returns an atomic counter to its start, in one store.
 *
 * @param[in,out] _counter  the counter; `NULL` is ignored.
 */
void
d_counter_atomic_reset(
    struct dj_counter_atomic* _counter
);

// 2.3    Queries
//------------------------------------------------------------------------------
// Each reads the value once.
/**
 * @brief Returns an atomic counter's value.
 *
 * @param[in] _counter  the counter; may be `NULL`.
 * @return the value, or 0 for `NULL`.
 */
int64_t
d_counter_atomic_value(
    const struct dj_counter_atomic* _counter
);

/**
 * @brief Returns an atomic counter's movement since its start.
 *
 * @param[in] _counter  the counter; may be `NULL`.
 * @return as d_counter_delta.
 */
int64_t
d_counter_atomic_delta(
    const struct dj_counter_atomic* _counter
);

/**
 * @brief Whether an atomic counter stands at its lower bound.
 *
 * @param[in] _counter  the counter; may be `NULL`.
 * @return as d_counter_at_min.
 */
bool
d_counter_atomic_at_min(
    const struct dj_counter_atomic* _counter
);

/**
 * @brief Whether an atomic counter stands at its upper bound.
 *
 * @param[in] _counter  the counter; may be `NULL`.
 * @return as d_counter_at_max.
 */
bool
d_counter_atomic_at_max(
    const struct dj_counter_atomic* _counter
);

/**
 * @brief Copies an atomic counter into a plain one.
 *
 * @note This is how an atomic counter is handed to counter.h's functions.
 *
 * @param[in]  _counter  the counter; may be `NULL`.
 * @param[out] _out      the copy; may be `NULL`.
 * @return `true` when copied; `false` when either argument is `NULL`.
 */
bool
d_counter_atomic_snapshot(
    const struct dj_counter_atomic* _counter,
    struct dj_counter*              _out
);

// 2.4    Any counter
//------------------------------------------------------------------------------
/**
 * @brief Refers to an atomic counter, for counter.h's d_counter_ref_*
 *        functions.
 *
 * @param[in] _counter  the counter; `NULL` gives a reference to no counter.
 * @return the reference.
 */
struct dj_counter_ref
d_counter_atomic_ref(
    struct dj_counter_atomic* _counter
);


D_EXTERN_C_END


#endif  // floor: D_ENV_HAS_LONG_LONG

#endif  // DJINTERP_C_UTIL_COUNTER_ATOMIC_H
