/*******************************************************************************
* djinterp [c][utility]                                                  timer.h
*
* The generic timer: elapsed nanoseconds accumulated over start/stop intervals,
* an optional limit, and the five operations every counter and timer in
* djinterp shares -- increment, decrement, compare, merge and reset.
*   A timer knows nothing about what it times, nor about events: every timer
* is used by calling its functions. Its thread-safe forms are timer_atomic.h
* (lock-free) and timer_mutex.h (locked). DTest's test_timer.h wraps it.
*
* ANY TIMER
* ---------
*   A struct dj_timer_ref is any of the three kinds: a table of the kind's
* operations and a pointer to the timer. d_timer_ref, d_timer_atomic_ref and
* d_timer_mutex_ref make one, and the d_timer_ref_* functions work on it
* whatever the kind, so code written once -- the handler of timer_event.h, which
* acts on a timer when an event fires, for one -- serves every timer.
*
* THE CLOCK IS AN ARGUMENT
* ------------------------
*   A timer reads whatever fn_timer_clock it is given, and the monotonic clock
* when it is given none, so a test can drive a timer from a counter of its own
* and get exact, repeatable times.
*
* THE FIVE OPERATIONS, FOR TIME
* -----------------------------
*   Increment and decrement add and remove time directly, as a counter's do;
* elapsed time never goes below zero and saturates at INT64_MAX nanoseconds
* (about 292 years), through counter.h's arithmetic. compare orders by elapsed
* time; merge adds another timer's elapsed time; reset stops the timer and
* returns it to zero.
*
* ZERO IS A TIMER
* ---------------
*   A zero-initialized struct dj_timer, or one set from D_TIMER_INIT, is a
* stopped timer at zero with no limit, on the monotonic clock.
*
* HOW TO USE
* ----------
*   A zero-filled timer is stopped at zero on the monotonic clock.
*
*     struct dj_timer span = D_TIMER_INIT;
*
*     d_timer_start(&span);
*     // the work
*     d_timer_stop(&span);
*     d_timer_elapsed(&span);              // nanoseconds
*
*   Code that serves any kind of timer takes a struct dj_timer_ref and calls the
* d_timer_ref_* functions.
*
* OPTIONS
* -------
*   The limit (d_timer_init_limited), after which the timer reads as expired,
* and the clock (d_timer_set_clock), which a test can drive by hand for exact
* times.
*
* CONFIGURATION
* -------------
*   None: no D_CFG_* knob reaches this header.
*
*
* path:      /inc/djinterp/c/util/timer/timer.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.09
*                                                            revised: 2026.11.10
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  TYPES
    -----
    1.  The clock
         1.  fn_timer_clock
    2.  The timer
         1.  dj_timer
         2.  D_TIMER_INIT
    3.  Any timer
         1.  dj_timer_vtable
         2.  dj_timer_ref
         3.  D_TIMER_REF_NONE
2.  OPERATIONS
    ----------
    1.  Clocks
    2.  Initialization
    3.  Starting and stopping
    4.  The five operations
    5.  Queries
3.  ANY TIMER
    ---------
    1.  Making a reference
    2.  Starting and stopping
    3.  The five operations
    4.  Queries
*/

#ifndef DJINTERP_C_UTIL_TIMER_H
#define DJINTERP_C_UTIL_TIMER_H 1

// djinterp
#include "../../djinterp.h"  // framework root
// re_std
#include "../../../../re_std/c/cstdint/dstdint.h"  // int64_t


D_EXTERN_C_BEGIN


//==============================================================================
// 1.  TYPES
//==============================================================================


// 1.1    The clock
//------------------------------------------------------------------------------
// 1.1.1
// fn_timer_clock
//   function pointer: the time now, in nanoseconds, from any fixed origin. A
// timer only subtracts two readings, so the origin never matters; a reading
// earlier than the last is taken as no time passing.
typedef int64_t (*fn_timer_clock)(void* _context);

// 1.2    The timer
//------------------------------------------------------------------------------
// 1.2.1
// dj_timer
//   struct: the time accumulated over closed intervals, the open interval's
// start while running, the limit, and the clock.
struct dj_timer
{
    int64_t        elapsed;    // nanoseconds over closed intervals
    int64_t        started;    // the clock's reading when last started
    int64_t        limit;      // nanoseconds, when has_limit
    bool           has_limit;  // whether the timer can expire
    bool           running;    // whether an interval is open
    fn_timer_clock clock;      // the clock; NULL is the monotonic clock
    void*          context;    // the clock's argument
};

// 1.2.2
// D_TIMER_INIT
//   constant: an initializer for a stopped timer at zero with no limit, on
// the monotonic clock: the same timer a zero-filled struct is.
#define D_TIMER_INIT  { 0, 0, 0, false, false, NULL, NULL }

// 1.3    Any timer
//------------------------------------------------------------------------------
// 1.3.1
// dj_timer_vtable
//   struct: the operations of one kind of timer -- plain, atomic or guarded
// -- over an untyped pointer to it. Each kind keeps one constant table, shared
// by every timer of that kind; every slot is set. A slot does what the kind's
// function of the same name does.
struct dj_timer_vtable
{
    // start
    //   opens an interval; false when one is open already.
    bool    (*start)(void* _timer);

    // stop
    //   closes the open interval; false when none is open.
    bool    (*stop)(void* _timer);

    // increment
    //   adds _amount nanoseconds; false when cut short.
    bool    (*increment)(void*   _timer,
                         int64_t _amount);

    // decrement
    //   removes _amount nanoseconds; false when cut short.
    bool    (*decrement)(void*   _timer,
                         int64_t _amount);

    // merge
    //   adds _source's elapsed time to the timer.
    bool    (*merge)(void*                  _destination,
                     const struct dj_timer* _source);

    // reset
    //   stops the timer and returns it to zero.
    void    (*reset)(void* _timer);

    // elapsed
    //   the elapsed nanoseconds, an open interval included.
    int64_t (*elapsed)(const void* _timer);

    // running
    //   whether an interval is open.
    bool    (*running)(const void* _timer);

    // snapshot
    //   copies the timer into a plain one; false when it cannot.
    bool    (*snapshot)(const void*      _timer,
                        struct dj_timer* _out);
};

// 1.3.2
// dj_timer_ref
//   struct: any timer: its kind's table and the timer it acts on. Two
// pointers, copied by value; the timer stays the caller's, and must outlive
// every reference to it. A zero-filled reference is no timer: its operations
// do nothing and fail, and its queries answer 0 or false.
struct dj_timer_ref
{
    const struct dj_timer_vtable* vtable;  // the kind's operations
    void*                         timer;   // the timer they act on
};

// 1.3.3
// D_TIMER_REF_NONE
//   constant: an initializer for a reference to no timer.
#define D_TIMER_REF_NONE  { NULL, NULL }


//==============================================================================
// 2.  OPERATIONS
//==============================================================================
// Every function accepts NULL: a query on NULL answers as a stopped timer at
// zero would, and an operation on NULL does nothing and reports failure.


// 2.1    Clocks
//------------------------------------------------------------------------------
/**
 * @brief The monotonic clock, as an fn_timer_clock.
 *
 * @param[in] _context  unused; may be `NULL`.
 * @return dtime.h's d_monotonic_time_ns.
 */
int64_t
d_timer_clock_monotonic(
    void* _context
);

// 2.2    Initialization
//------------------------------------------------------------------------------
/**
 * @brief Sets a stopped timer at zero with no limit, on the monotonic clock.
 *
 * @param[out] _timer  the timer to set; `NULL` is ignored.
 * @post `_timer` equals D_TIMER_INIT.
 */
void
d_timer_init(
    struct dj_timer* _timer
);

/**
 * @brief Sets a stopped timer at zero that expires at `_limit`.
 *
 * @param[out] _timer  the timer to set.
 * @param[in]  _limit  the limit in nanoseconds; not negative.
 * @return `true` when set; `false` when `_timer` is `NULL` or `_limit` is
 *         negative, and then nothing changes.
 */
bool
d_timer_init_limited(
    struct dj_timer* _timer,
    int64_t          _limit
);

/**
 * @brief Changes the clock a timer reads.
 *
 * @warning Change it only while the timer is stopped: the open interval's
 *          start was read from the old clock.
 *
 * @param[in,out] _timer    the timer; `NULL` is ignored.
 * @param[in]     _clock    the clock; `NULL` restores the monotonic one.
 * @param[in]     _context  the clock's argument, passed to every reading.
 */
void
d_timer_set_clock(
    struct dj_timer* _timer,
    fn_timer_clock   _clock,
    void*            _context
);

// 2.3    Starting and stopping
//------------------------------------------------------------------------------
/**
 * @brief Opens an interval at the clock's reading.
 *
 * @param[in,out] _timer  the timer.
 * @return `true` when started; `false` when an interval is open already, the
 *         timer has expired, or `_timer` is `NULL`.
 */
bool
d_timer_start(
    struct dj_timer* _timer
);

/**
 * @brief Closes the open interval into the elapsed time.
 *
 * @param[in,out] _timer  the timer.
 * @return `true` when stopped; `false` when no interval was open, or
 *         `_timer` is `NULL`.
 */
bool
d_timer_stop(
    struct dj_timer* _timer
);

// 2.4    The five operations
//------------------------------------------------------------------------------
/**
 * @brief Adds `_amount` nanoseconds to the elapsed time.
 *
 * @param[in,out] _timer   the timer.
 * @param[in]     _amount  the nanoseconds; negative removes time.
 * @return `true` when the full amount applied; `false` when the result was
 *         cut at zero or at INT64_MAX, or `_timer` is `NULL`.
 */
bool
d_timer_increment(
    struct dj_timer* _timer,
    int64_t          _amount
);

/**
 * @brief Removes `_amount` nanoseconds from the elapsed time.
 *
 * @note Time comes off the closed intervals only; an open one runs on.
 *
 * @param[in,out] _timer   the timer.
 * @param[in]     _amount  the nanoseconds; negative adds time.
 * @return as d_timer_increment.
 */
bool
d_timer_decrement(
    struct dj_timer* _timer,
    int64_t          _amount
);

/**
 * @brief Orders two timers by elapsed time, an open interval included.
 *
 * @param[in] _a  the first timer; may be `NULL`.
 * @param[in] _b  the second timer; may be `NULL`.
 * @return -1, 0 or 1 as `_a` has run less than, as long as, or longer than
 *         `_b`; `NULL` orders before any timer and equal to itself.
 */
int
d_timer_compare(
    const struct dj_timer* _a,
    const struct dj_timer* _b
);

/**
 * @brief Adds `_source`'s elapsed time to `_destination`.
 *
 * @param[in,out] _destination  the timer that gains the time.
 * @param[in]     _source       the timer whose elapsed time is added, its
 *                              open interval included; it does not change.
 * @return as d_timer_increment; `false` too when `_source` is `NULL`.
 */
bool
d_timer_merge(
    struct dj_timer*       _destination,
    const struct dj_timer* _source
);

/**
 * @brief Stops a timer and returns it to zero.
 *
 * @param[in,out] _timer  the timer; `NULL` is ignored.
 * @post The timer is stopped at zero; its limit and clock are unchanged.
 */
void
d_timer_reset(
    struct dj_timer* _timer
);

// 2.5    Queries
//------------------------------------------------------------------------------
/**
 * @brief Returns a timer's elapsed time.
 *
 * @param[in] _timer  the timer; may be `NULL`.
 * @return the nanoseconds over closed intervals, plus the open one's while
 *         running (a clock reading), or 0 for `NULL`.
 */
int64_t
d_timer_elapsed(
    const struct dj_timer* _timer
);

/**
 * @brief Returns the time left before a timer's limit.
 *
 * @param[in] _timer  the timer; may be `NULL`.
 * @return the nanoseconds to the limit; 0 with no limit, past it, or for
 *         `NULL`.
 */
int64_t
d_timer_remaining(
    const struct dj_timer* _timer
);

/**
 * @brief Whether a timer has an open interval.
 *
 * @param[in] _timer  the timer; may be `NULL`.
 * @return `true` while running; `false` otherwise, and for `NULL`.
 */
bool
d_timer_running(
    const struct dj_timer* _timer
);

/**
 * @brief Whether a timer has reached its limit.
 *
 * @param[in] _timer  the timer; may be `NULL`.
 * @return `true` only for a limited timer whose elapsed time has reached its
 *         limit; a limit of 0 is reached at once.
 */
bool
d_timer_expired(
    const struct dj_timer* _timer
);


//==============================================================================
// 3.  ANY TIMER
//==============================================================================
// The operations of section 2, on a struct dj_timer_ref of any kind. A
// reference to no timer is accepted everywhere, as NULL is in section 2.


// 3.1    Making a reference
//------------------------------------------------------------------------------
/**
 * @brief Refers to a plain timer.
 *
 * @note timer_atomic.h's d_timer_atomic_ref and timer_mutex.h's
 *       d_timer_mutex_ref refer to the other two kinds.
 *
 * @param[in] _timer  the timer; `NULL` gives a reference to no timer.
 * @return the reference.
 */
struct dj_timer_ref
d_timer_ref(
    struct dj_timer* _timer
);

/**
 * @brief Whether a reference refers to a timer.
 *
 * @param[in] _ref  the reference.
 * @return `true` when it has both a table and a timer.
 */
bool
d_timer_ref_is_valid(
    struct dj_timer_ref _ref
);

// 3.2    Starting and stopping
//------------------------------------------------------------------------------
/**
 * @brief Opens an interval on any timer.
 *
 * @param[in] _ref  the timer.
 * @return as the kind's start; `false` for no timer.
 */
bool
d_timer_ref_start(
    struct dj_timer_ref _ref
);

/**
 * @brief Closes any timer's open interval into its elapsed time.
 *
 * @param[in] _ref  the timer.
 * @return as the kind's stop; `false` for no timer.
 */
bool
d_timer_ref_stop(
    struct dj_timer_ref _ref
);

// 3.3    The five operations
//------------------------------------------------------------------------------
/**
 * @brief Adds `_amount` nanoseconds to any timer, saturating.
 *
 * @param[in] _ref     the timer.
 * @param[in] _amount  the nanoseconds; negative removes time.
 * @return as the kind's increment; `false` for no timer.
 */
bool
d_timer_ref_increment(
    struct dj_timer_ref _ref,
    int64_t             _amount
);

/**
 * @brief Removes `_amount` nanoseconds from any timer, saturating at zero.
 *
 * @param[in] _ref     the timer.
 * @param[in] _amount  the nanoseconds; negative adds time.
 * @return as the kind's decrement; `false` for no timer.
 */
bool
d_timer_ref_decrement(
    struct dj_timer_ref _ref,
    int64_t             _amount
);

/**
 * @brief Orders two timers, of any kinds, by elapsed time.
 *
 * @param[in] _a  the first timer.
 * @param[in] _b  the second timer.
 * @return -1, 0 or 1 as `_a`'s elapsed time is below, equal to or above
 *         `_b`'s; no timer orders before any timer and equal to no timer.
 */
int
d_timer_ref_compare(
    struct dj_timer_ref _a,
    struct dj_timer_ref _b
);

/**
 * @brief Adds `_source`'s elapsed time to `_destination`, of any kinds.
 *
 * @note `_source` is read through a snapshot first, so the two may be the same
 *       timer, and a guarded timer's lock is never held twice.
 *
 * @param[in] _destination  the timer that gains the time.
 * @param[in] _source       the timer whose elapsed time is added.
 * @return `true` when the time applied in full; `false` when it saturated,
 *         `_source` could not be read, or either is no timer.
 */
bool
d_timer_ref_merge(
    struct dj_timer_ref _destination,
    struct dj_timer_ref _source
);

/**
 * @brief Stops any timer and returns it to zero.
 *
 * @param[in] _ref  the timer; no timer is ignored.
 */
void
d_timer_ref_reset(
    struct dj_timer_ref _ref
);

// 3.4    Queries
//------------------------------------------------------------------------------
/**
 * @brief Returns any timer's elapsed time.
 *
 * @param[in] _ref  the timer.
 * @return the nanoseconds, an open interval included, or 0 for no timer.
 */
int64_t
d_timer_ref_elapsed(
    struct dj_timer_ref _ref
);

/**
 * @brief Returns the time left before any timer's limit.
 *
 * @param[in] _ref  the timer.
 * @return as d_timer_remaining on its snapshot; 0 for no timer, or one that
 *         cannot be read.
 */
int64_t
d_timer_ref_remaining(
    struct dj_timer_ref _ref
);

/**
 * @brief Whether any timer has an open interval.
 *
 * @param[in] _ref  the timer.
 * @return as the kind's running; `false` for no timer.
 */
bool
d_timer_ref_running(
    struct dj_timer_ref _ref
);

/**
 * @brief Whether any timer has reached its limit.
 *
 * @param[in] _ref  the timer.
 * @return as d_timer_expired on its snapshot; `false` for no timer, or one
 *         that cannot be read.
 */
bool
d_timer_ref_expired(
    struct dj_timer_ref _ref
);

/**
 * @brief Copies any timer into a plain one, for section 2's queries.
 *
 * @param[in]  _ref  the timer.
 * @param[out] _out  the copy; may be `NULL`.
 * @return `true` when copied; `false` for no timer, a `NULL` `_out`, or a
 *         timer its kind cannot read (a guarded timer not set up).
 */
bool
d_timer_ref_snapshot(
    struct dj_timer_ref _ref,
    struct dj_timer*    _out
);


D_EXTERN_C_END


#endif  // DJINTERP_C_UTIL_TIMER_H
