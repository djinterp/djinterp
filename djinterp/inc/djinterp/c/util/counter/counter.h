/*******************************************************************************
* djinterp [c][utility]                                                counter.h
*
* The generic counter: a signed 64-bit value, optionally held within a range,
* and the five operations every counter in djinterp shares -- increment,
* decrement, compare, merge and reset.
*   A counter knows nothing about what it counts, nor about events: every
* counter is used by calling its functions. Its thread-safe forms are
* counter_atomic.h (lock-free) and counter_mutex.h (locked). DTest's
* test_counter.h builds its tallies from it.
*
* ANY COUNTER
* -----------
*   A struct dj_counter_ref is any of the three kinds: a table of the kind's
* operations and a pointer to the counter. d_counter_ref, d_counter_atomic_ref
* and d_counter_mutex_ref make one, and the d_counter_ref_* functions work on it
* whatever the kind, so code written once -- counter_event.h's handler, which
* moves a counter when an event fires, for one -- serves every counter.
*
* BOUNDS ARE MATH'S INTERVALS
* ---------------------------
*   A counter's range is a struct dj_interval_imax (c/modules/math/interval.h),
* closed or open at either end. A side with no limit is that side of
* dj_math_imax's range. The interval's stride is not used: a counter steps
* by any amount, so the init functions refuse a discrete interval, as they
* refuse an empty or malformed one.
*
* SATURATING, NEVER WRAPPING
* --------------------------
*   A step that would cross a bound, or leave the int64_t range, stops at the
* edge and reports that it was cut short. Signed overflow is undefined in C, so
* the arithmetic checks before it adds.
*
* ZERO IS A COUNTER
* -----------------
*   A zero-initialized struct dj_counter, or one set from D_COUNTER_INIT, is a
* valid counter at 0 with no bounds, so a counter needs no init call to be used.
*
* HOW TO USE
* ----------
*   A zero-filled counter is ready to use; call the functions on it.
*
*     struct dj_counter hits = D_COUNTER_INIT;
*
*     d_counter_increment(&hits, 1);
*     d_counter_value(&hits);              // 1
*
*   Code that serves any kind of counter takes a struct dj_counter_ref and calls
* the d_counter_ref_* functions:
*
*     d_counter_ref_increment(d_counter_ref(&hits), 1);
*
* OPTIONS
* -------
*   A counter's bounds, given once to d_counter_init_bounded as a struct
* dj_interval_imax, closed or open at either end. Nothing else is chosen at run
* time.
*
* CONFIGURATION
* -------------
*   None: no D_CFG_* knob reaches this header.
*
*
* path:      /inc/djinterp/c/util/counter/counter.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.09
*                                                            revised: 2026.11.10
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  TYPES
    -----
    1.  The counter
         1.  dj_counter
         2.  D_COUNTER_INIT
    2.  Any counter
         1.  dj_counter_vtable
         2.  dj_counter_ref
         3.  D_COUNTER_REF_NONE
2.  OPERATIONS
    ----------
    1.  Bounded arithmetic
    2.  Initialization
    3.  The five operations
    4.  Queries
3.  ANY COUNTER
    -----------
    1.  Making a reference
    2.  The five operations
    3.  Queries
*/

#ifndef DJINTERP_C_UTIL_COUNTER_H
#define DJINTERP_C_UTIL_COUNTER_H 1

// djinterp
#include "../../djinterp.h"  // framework root
#include "../../modules/math/interval.h"  // dj_interval_imax
// re_std
#include "../../../../re_std/c/cstdint/dstdint.h"  // int64_t


D_EXTERN_C_BEGIN


//==============================================================================
// 1.  TYPES
//==============================================================================


// 1.1    The counter
//------------------------------------------------------------------------------
// 1.1.1
// dj_counter
//   struct: a count, the value it starts from, and the range it is held in.
//
//   `initial` is what reset returns to and what merge measures from: merging
// adds another counter's movement since its own start, not its raw value, so
// two counters that start at 10 and each count 3 merge to 16, not 23.
struct dj_counter
{
    int64_t                 initial;    // the start, for reset and merge
    int64_t                 value;      // the count now
    struct dj_interval_imax bounds;     // the range, when has_bounds
    bool                    has_bounds; // whether bounds applies
};

// 1.1.2
// D_COUNTER_INIT
//   constant: an initializer for a counter at 0 with no bounds, the same
// counter a zero-filled struct is.
#define D_COUNTER_INIT  { 0, 0, { 0, 0, 0, 0u }, false }

// 1.2    Any counter
//------------------------------------------------------------------------------
// 1.2.1
// dj_counter_vtable
//   struct: the operations of one kind of counter -- plain, atomic or guarded
// -- over an untyped pointer to it. Each kind keeps one constant table, shared
// by every counter of that kind; every slot is set. A slot does what the
// kind's function of the same name does.
struct dj_counter_vtable
{
    // increment
    //   moves the counter up by _amount; false when cut short.
    bool    (*increment)(void*   _counter,
                         int64_t _amount);

    // decrement
    //   moves the counter down by _amount; false when cut short.
    bool    (*decrement)(void*   _counter,
                         int64_t _amount);

    // merge
    //   adds _source's movement since its start to the counter.
    bool    (*merge)(void*                    _destination,
                     const struct dj_counter* _source);

    // reset
    //   returns the counter to its start.
    void    (*reset)(void* _counter);

    // value
    //   the counter's value.
    int64_t (*value)(const void* _counter);

    // snapshot
    //   copies the counter into a plain one; false when it cannot.
    bool    (*snapshot)(const void*        _counter,
                        struct dj_counter* _out);
};

// 1.2.2
// dj_counter_ref
//   struct: any counter: its kind's table and the counter it acts on. Two
// pointers, copied by value; the counter stays the caller's, and must outlive
// every reference to it. A zero-filled reference is no counter: its
// operations do nothing and fail, and its queries answer 0.
struct dj_counter_ref
{
    const struct dj_counter_vtable* vtable;   // the kind's operations
    void*                           counter;  // the counter they act on
};

// 1.2.3
// D_COUNTER_REF_NONE
//   constant: an initializer for a reference to no counter.
#define D_COUNTER_REF_NONE  { NULL, NULL }


//==============================================================================
// 2.  OPERATIONS
//==============================================================================
// Every function accepts NULL: a query on NULL answers as an empty counter
// would, and an operation on NULL does nothing and reports failure.


// 2.1    Bounded arithmetic
//------------------------------------------------------------------------------
// The arithmetic every counter and timer in djinterp shares. A bound is taken
// inclusively -- an open end one integer inward -- and the interval's stride
// is ignored.
/**
 * @brief Adds `_amount` to `_value` within `_bounds`, saturating.
 *
 * @note The sum is held inside the int64_t range first, then inside the
 *       bounds; a value already outside the bounds is pulled back in by any
 *       step, and that counts as a cut.
 *
 * @param[in]  _bounds    the range; `NULL` means none.
 * @param[in]  _value     the value to move.
 * @param[in]  _amount    how far to move it; negative moves it down.
 * @param[out] _is_exact  set to `true` when the full amount applied, `false`
 *                        when the result was cut at an edge; may be `NULL`.
 * @return the moved value.
 */
int64_t
d_counter_bounds_add(
    const struct dj_interval_imax* _bounds,
    int64_t                       _value,
    int64_t                       _amount,
    bool*                         _is_exact
);

/**
 * @brief Subtracts `_amount` from `_value` within `_bounds`, saturating.
 *
 * @note d_counter_bounds_add's mirror. INT64_MIN, which has no negation, is
 *       taken as a step down by INT64_MAX and then by one, each within the
 *       bounds.
 *
 * @param[in]  _bounds    the range; `NULL` means none.
 * @param[in]  _value     the value to move.
 * @param[in]  _amount    how far to move it down; negative moves it up.
 * @param[out] _is_exact  set as d_counter_bounds_add sets it; may be `NULL`.
 * @return the moved value.
 */
int64_t
d_counter_bounds_sub(
    const struct dj_interval_imax* _bounds,
    int64_t                       _value,
    int64_t                       _amount,
    bool*                         _is_exact
);

// 2.2    Initialization
//------------------------------------------------------------------------------
/**
 * @brief Sets a counter at `_initial` with no bounds.
 *
 * @param[out] _counter  the counter to set; `NULL` is ignored.
 * @param[in]  _initial  the start.
 * @post `_counter` equals D_COUNTER_INIT moved to `_initial`.
 */
void
d_counter_init(
    struct dj_counter* _counter,
    int64_t            _initial
);

/**
 * @brief Sets a counter at `_initial` within `_bounds`.
 *
 * @param[out] _counter  the counter to set.
 * @param[in]  _initial  the start; must lie within `_bounds`.
 * @param[in]  _bounds   the range; `NULL` means none. It must be valid,
 *                       non-empty and continuous (a stride of 0).
 * @return `true` when set; `false` when `_counter` is `NULL`, `_bounds` is
 *         refused, or `_initial` lies outside it, and then nothing changes.
 */
bool
d_counter_init_bounded(
    struct dj_counter*            _counter,
    int64_t                       _initial,
    const struct dj_interval_imax* _bounds
);

// 2.3    The five operations
//------------------------------------------------------------------------------
/**
 * @brief Moves a counter up by `_amount`, saturating at its bounds.
 *
 * @param[in,out] _counter  the counter.
 * @param[in]     _amount   how far to move it; negative moves it down.
 * @return `true` when the full amount applied; `false` when the move was cut
 *         at a bound or at the int64_t range, or `_counter` is `NULL`.
 */
bool
d_counter_increment(
    struct dj_counter* _counter,
    int64_t            _amount
);

/**
 * @brief Moves a counter down by `_amount`, saturating at its bounds.
 *
 * @param[in,out] _counter  the counter.
 * @param[in]     _amount   how far to move it; negative moves it up.
 * @return as d_counter_increment.
 */
bool
d_counter_decrement(
    struct dj_counter* _counter,
    int64_t            _amount
);

/**
 * @brief Orders two counters by value.
 *
 * @param[in] _a  the first counter; may be `NULL`.
 * @param[in] _b  the second counter; may be `NULL`.
 * @return -1, 0 or 1 as `_a`'s value is below, equal to or above `_b`'s;
 *         `NULL` orders before any counter and equal to itself.
 */
int
d_counter_compare(
    const struct dj_counter* _a,
    const struct dj_counter* _b
);

/**
 * @brief Adds `_source`'s movement since its start to `_destination`.
 *
 * @note Merging measures from `_source`'s `initial`, so tallies kept apart and
 *       merged equal one tally kept throughout, whatever each started from.
 *
 * @param[in,out] _destination  the counter that gains the movement.
 * @param[in]     _source       the counter whose movement is added; it does
 *                              not change, and may be `_destination` itself.
 * @return `true` when the movement applied in full; `false` when it, or the
 *         add, saturated, or either argument is `NULL`.
 */
bool
d_counter_merge(
    struct dj_counter*       _destination,
    const struct dj_counter* _source
);

/**
 * @brief Returns a counter to its start.
 *
 * @param[in,out] _counter  the counter; `NULL` is ignored.
 * @post `value` equals `initial`; the bounds are unchanged.
 */
void
d_counter_reset(
    struct dj_counter* _counter
);

// 2.4    Queries
//------------------------------------------------------------------------------
/**
 * @brief Returns a counter's value.
 *
 * @param[in] _counter  the counter; may be `NULL`.
 * @return the value, or 0 for `NULL`.
 */
int64_t
d_counter_value(
    const struct dj_counter* _counter
);

/**
 * @brief Returns a counter's movement since its start.
 *
 * @param[in] _counter  the counter; may be `NULL`.
 * @return `value` - `initial`, saturating at the int64_t range, or 0 for
 *         `NULL`; the amount d_counter_merge adds.
 */
int64_t
d_counter_delta(
    const struct dj_counter* _counter
);

/**
 * @brief Whether a counter stands at its lower bound.
 *
 * @param[in] _counter  the counter; may be `NULL`.
 * @return `true` when the counter has bounds and its value is at or below
 *         their inclusive lower end; `false` otherwise, and for `NULL`.
 */
bool
d_counter_at_min(
    const struct dj_counter* _counter
);

/**
 * @brief Whether a counter stands at its upper bound.
 *
 * @param[in] _counter  the counter; may be `NULL`.
 * @return `true` when the counter has bounds and its value is at or above
 *         their inclusive upper end; `false` otherwise, and for `NULL`.
 */
bool
d_counter_at_max(
    const struct dj_counter* _counter
);


//==============================================================================
// 3.  ANY COUNTER
//==============================================================================
// The operations of section 2, on a struct dj_counter_ref of any kind. A
// reference to no counter is accepted everywhere, as NULL is in section 2.


// 3.1    Making a reference
//------------------------------------------------------------------------------
/**
 * @brief Refers to a plain counter.
 *
 * @note counter_atomic.h's d_counter_atomic_ref and counter_mutex.h's
 *       d_counter_mutex_ref refer to the other two kinds.
 *
 * @param[in] _counter  the counter; `NULL` gives a reference to no counter.
 * @return the reference.
 */
struct dj_counter_ref
d_counter_ref(
    struct dj_counter* _counter
);

/**
 * @brief Whether a reference refers to a counter.
 *
 * @param[in] _ref  the reference.
 * @return `true` when it has both a table and a counter.
 */
bool
d_counter_ref_is_valid(
    struct dj_counter_ref _ref
);

// 3.2    The five operations
//------------------------------------------------------------------------------
/**
 * @brief Moves any counter up by `_amount`, saturating at its bounds.
 *
 * @param[in] _ref     the counter.
 * @param[in] _amount  how far to move it; negative moves it down.
 * @return as the kind's increment; `false` for no counter.
 */
bool
d_counter_ref_increment(
    struct dj_counter_ref _ref,
    int64_t               _amount
);

/**
 * @brief Moves any counter down by `_amount`, saturating at its bounds.
 *
 * @param[in] _ref     the counter.
 * @param[in] _amount  how far to move it; negative moves it up.
 * @return as the kind's decrement; `false` for no counter.
 */
bool
d_counter_ref_decrement(
    struct dj_counter_ref _ref,
    int64_t               _amount
);

/**
 * @brief Orders two counters, of any kinds, by value.
 *
 * @param[in] _a  the first counter.
 * @param[in] _b  the second counter.
 * @return -1, 0 or 1 as `_a`'s value is below, equal to or above `_b`'s; no
 *         counter orders before any counter and equal to no counter.
 */
int
d_counter_ref_compare(
    struct dj_counter_ref _a,
    struct dj_counter_ref _b
);

/**
 * @brief Adds `_source`'s movement since its start to `_destination`, of any
 *        kinds.
 *
 * @note `_source` is read through a snapshot first, so the two may be the same
 *       counter, and a guarded counter's lock is never held twice.
 *
 * @param[in] _destination  the counter that gains the movement.
 * @param[in] _source       the counter whose movement is added.
 * @return `true` when the movement applied in full; `false` when it
 *         saturated, `_source` could not be read, or either is no counter.
 */
bool
d_counter_ref_merge(
    struct dj_counter_ref _destination,
    struct dj_counter_ref _source
);

/**
 * @brief Returns any counter to its start.
 *
 * @param[in] _ref  the counter; no counter is ignored.
 */
void
d_counter_ref_reset(
    struct dj_counter_ref _ref
);

// 3.3    Queries
//------------------------------------------------------------------------------
/**
 * @brief Returns any counter's value.
 *
 * @param[in] _ref  the counter.
 * @return the value, or 0 for no counter.
 */
int64_t
d_counter_ref_value(
    struct dj_counter_ref _ref
);

/**
 * @brief Copies any counter into a plain one, for section 2's queries.
 *
 * @param[in]  _ref  the counter.
 * @param[out] _out  the copy; may be `NULL`.
 * @return `true` when copied; `false` for no counter, a `NULL` `_out`, or a
 *         counter its kind cannot read (a guarded counter not set up).
 */
bool
d_counter_ref_snapshot(
    struct dj_counter_ref _ref,
    struct dj_counter*    _out
);


D_EXTERN_C_END


#endif  // DJINTERP_C_UTIL_COUNTER_H
