/*******************************************************************************
* djinterp [c][utility]                                                  timer.c
*
* The generic timer. Elapsed time is a count of nanoseconds held between zero
* and INT64_MAX, so every change to it goes through counter.h's saturating
* arithmetic with those two bounds; the clock is read only to open and close
* intervals and to report a running timer's time.
*
*
* path:      /src/djinterp/c/util/timer/timer.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.09
*                                                            revised: 2026.11.10
*******************************************************************************/
#include "../../../../../inc/djinterp/c/util/timer/timer.h"  // corresponding header
// djinterp
#include "../../../../../inc/djinterp/c/dtime.h"  // d_monotonic_time_ns
#include "../../../../../inc/djinterp/c/util/counter/counter.h"  // arithmetic


/**
 * @brief The range elapsed time may take: zero to INT64_MAX.
 *
 * @return the bounds.
 */
static struct dj_interval_imax
d_internal_timer_bounds(void)
{
    return d_interval_imax_make(0,
                                (dj_math_imax)INT64_MAX,
                                D_INTERVAL_CLOSED,
                                0);
}

/**
 * @brief Reads a timer's clock.
 *
 * @param[in] _timer  the timer; not `NULL`.
 * @return the reading, in nanoseconds.
 */
static int64_t
d_internal_timer_now(
    const struct dj_timer* _timer
)
{
    return (_timer->clock)
               ? _timer->clock(_timer->context)
               : d_timer_clock_monotonic(NULL);
}

/**
 * @brief The length of a running timer's open interval.
 *
 * @param[in] _timer  a running timer; not `NULL`.
 * @return the nanoseconds since it started, never negative.
 *
 * A clock that reads earlier than the start (a fallback clock that is not
 * monotonic, or a test clock wound back) counts as no time passing.
 */
static int64_t
d_internal_timer_open(
    const struct dj_timer* _timer
)
{
    const struct dj_interval_imax bounds = d_internal_timer_bounds();

    return d_counter_bounds_sub(&bounds,
                                d_internal_timer_now(_timer),
                                _timer->started,
                                NULL);
}

/*
d_timer_clock_monotonic
  dtime.h's clock as an fn_timer_clock.
*/
int64_t
d_timer_clock_monotonic(
    void* _context
)
{
    (void)_context;

    return d_monotonic_time_ns();
}

/*
d_timer_init
  The zero timer.
*/
void
d_timer_init(
    struct dj_timer* _timer
)
{
    // parameter validation
    if (!_timer)
    {
        return;
    }

    const struct dj_timer zero = D_TIMER_INIT;

    *_timer = zero;

    return;
}

/*
d_timer_init_limited
  A negative limit is refused: no elapsed time is below zero, so it could
never be told from a limit of zero.
*/
bool
d_timer_init_limited(
    struct dj_timer* _timer,
    int64_t          _limit
)
{
    // parameter validation
    if ( (!_timer) ||
         (_limit < 0) )
    {
        return false;
    }

    d_timer_init(_timer);
    _timer->limit     = _limit;
    _timer->has_limit = true;

    return true;
}

/*
d_timer_set_clock
  The clock and its argument travel together.
*/
void
d_timer_set_clock(
    struct dj_timer* _timer,
    fn_timer_clock   _clock,
    void*            _context
)
{
    // parameter validation
    if (!_timer)
    {
        return;
    }

    _timer->clock   = _clock;
    _timer->context = _context;

    return;
}

/*
d_timer_start
  An expired timer does not restart: its limit has been met, and only reset
or decrement can take it back under.
*/
bool
d_timer_start(
    struct dj_timer* _timer
)
{
    // parameter validation, and the two states that cannot start
    if ( (!_timer)         ||
         (_timer->running) ||
         (d_timer_expired(_timer)) )
    {
        return false;
    }

    _timer->started = d_internal_timer_now(_timer);
    _timer->running = true;

    return true;
}

/*
d_timer_stop
  The open interval closes into `elapsed` through the saturating add.
*/
bool
d_timer_stop(
    struct dj_timer* _timer
)
{
    // parameter validation, and nothing to stop
    if ( (!_timer) ||
         (!_timer->running) )
    {
        return false;
    }

    const struct dj_interval_imax bounds = d_internal_timer_bounds();

    _timer->elapsed = d_counter_bounds_add(&bounds,
                                           _timer->elapsed,
                                           d_internal_timer_open(_timer),
                                           NULL);
    _timer->running = false;

    return true;
}

/*
d_timer_increment
  Time added directly. While running it joins the closed intervals; the open
one keeps running on top of it.
*/
bool
d_timer_increment(
    struct dj_timer* _timer,
    int64_t          _amount
)
{
    // parameter validation
    if (!_timer)
    {
        return false;
    }

    bool                         is_exact = true;
    const struct dj_interval_imax bounds  = d_internal_timer_bounds();

    _timer->elapsed = d_counter_bounds_add(&bounds,
                                           _timer->elapsed,
                                           _amount,
                                           &is_exact);

    return is_exact;
}

/*
d_timer_decrement
  Time removed directly, from the closed intervals only: a running timer's
open interval is not shortened, so removing more than was closed stops at
zero and reports the cut.
*/
bool
d_timer_decrement(
    struct dj_timer* _timer,
    int64_t          _amount
)
{
    // parameter validation
    if (!_timer)
    {
        return false;
    }

    bool                         is_exact = true;
    const struct dj_interval_imax bounds  = d_internal_timer_bounds();

    _timer->elapsed = d_counter_bounds_sub(&bounds,
                                           _timer->elapsed,
                                           _amount,
                                           &is_exact);

    return is_exact;
}

/*
d_timer_compare
  By elapsed time, each read once; NULL sorts first.
*/
int
d_timer_compare(
    const struct dj_timer* _a,
    const struct dj_timer* _b
)
{
    // NULL before any timer, and equal to itself
    if ( (!_a) ||
         (!_b) )
    {
        return (_a == _b) ? 0 : ( (!_a) ? -1 : 1 );
    }

    const int64_t a = d_timer_elapsed(_a);
    const int64_t b = d_timer_elapsed(_b);

    // compared, not subtracted
    if (a < b)
    {
        return -1;
    }

    return (a > b) ? 1 : 0;
}

/*
d_timer_merge
  The source's elapsed time, read once, added to the destination's closed
intervals. A timer merged into itself adds its own time once.
*/
bool
d_timer_merge(
    struct dj_timer*       _destination,
    const struct dj_timer* _source
)
{
    // parameter validation
    if ( (!_destination) ||
         (!_source) )
    {
        return false;
    }

    return d_timer_increment(_destination,
                             d_timer_elapsed(_source));
}

/*
d_timer_reset
  Stopped and zero; the limit and the clock are the timer's configuration,
not its state, and stay.
*/
void
d_timer_reset(
    struct dj_timer* _timer
)
{
    // parameter validation
    if (!_timer)
    {
        return;
    }

    _timer->elapsed = 0;
    _timer->started = 0;
    _timer->running = false;

    return;
}

/*
d_timer_elapsed
  The closed intervals, plus the open one while running, saturating.
*/
int64_t
d_timer_elapsed(
    const struct dj_timer* _timer
)
{
    // parameter validation
    if (!_timer)
    {
        return 0;
    }

    // a stopped timer's time is all closed
    if (!_timer->running)
    {
        return _timer->elapsed;
    }

    const struct dj_interval_imax bounds = d_internal_timer_bounds();

    return d_counter_bounds_add(&bounds,
                                _timer->elapsed,
                                d_internal_timer_open(_timer),
                                NULL);
}

/*
d_timer_remaining
  The limit less the elapsed time, never below zero.
*/
int64_t
d_timer_remaining(
    const struct dj_timer* _timer
)
{
    // parameter validation, and no limit to approach
    if ( (!_timer) ||
         (!_timer->has_limit) )
    {
        return 0;
    }

    const int64_t elapsed = d_timer_elapsed(_timer);

    return (elapsed < _timer->limit) ? (_timer->limit - elapsed) : 0;
}

/*
d_timer_running
  Whether an interval is open.
*/
bool
d_timer_running(
    const struct dj_timer* _timer
)
{
    return ( (_timer) &&
             (_timer->running) );
}

/*
d_timer_expired
  A limit of zero is met at once.
*/
bool
d_timer_expired(
    const struct dj_timer* _timer
)
{
    return ( (_timer)            &&
             (_timer->has_limit) &&
             (d_timer_elapsed(_timer) >= _timer->limit) );
}

// the plain kind's table: section 2's functions over an untyped pointer

/**
 * @brief The plain kind's start slot.
 *
 * @param[in,out] _timer  a struct dj_timer.
 * @return as d_timer_start.
 */
static bool
d_internal_timer_slot_start(
    void* _timer
)
{
    return d_timer_start((struct dj_timer*)_timer);
}

/**
 * @brief The plain kind's stop slot.
 *
 * @param[in,out] _timer  a struct dj_timer.
 * @return as d_timer_stop.
 */
static bool
d_internal_timer_slot_stop(
    void* _timer
)
{
    return d_timer_stop((struct dj_timer*)_timer);
}

/**
 * @brief The plain kind's increment slot.
 *
 * @param[in,out] _timer   a struct dj_timer.
 * @param[in]     _amount  as d_timer_increment's.
 * @return as d_timer_increment.
 */
static bool
d_internal_timer_slot_increment(
    void*   _timer,
    int64_t _amount
)
{
    return d_timer_increment((struct dj_timer*)_timer,
                             _amount);
}

/**
 * @brief The plain kind's decrement slot.
 *
 * @param[in,out] _timer   a struct dj_timer.
 * @param[in]     _amount  as d_timer_decrement's.
 * @return as d_timer_decrement.
 */
static bool
d_internal_timer_slot_decrement(
    void*   _timer,
    int64_t _amount
)
{
    return d_timer_decrement((struct dj_timer*)_timer,
                             _amount);
}

/**
 * @brief The plain kind's merge slot.
 *
 * @param[in,out] _destination  a struct dj_timer.
 * @param[in]     _source       as d_timer_merge's.
 * @return as d_timer_merge.
 */
static bool
d_internal_timer_slot_merge(
    void*                  _destination,
    const struct dj_timer* _source
)
{
    return d_timer_merge((struct dj_timer*)_destination,
                         _source);
}

/**
 * @brief The plain kind's reset slot.
 *
 * @param[in,out] _timer  a struct dj_timer.
 */
static void
d_internal_timer_slot_reset(
    void* _timer
)
{
    d_timer_reset((struct dj_timer*)_timer);

    return;
}

/**
 * @brief The plain kind's elapsed slot.
 *
 * @param[in] _timer  a struct dj_timer.
 * @return as d_timer_elapsed.
 */
static int64_t
d_internal_timer_slot_elapsed(
    const void* _timer
)
{
    return d_timer_elapsed((const struct dj_timer*)_timer);
}

/**
 * @brief The plain kind's running slot.
 *
 * @param[in] _timer  a struct dj_timer.
 * @return as d_timer_running.
 */
static bool
d_internal_timer_slot_running(
    const void* _timer
)
{
    return d_timer_running((const struct dj_timer*)_timer);
}

/**
 * @brief The plain kind's snapshot slot: a copy.
 *
 * @param[in]  _timer  a struct dj_timer.
 * @param[out] _out    the copy.
 * @return `true` when copied; `false` when either argument is `NULL`.
 */
static bool
d_internal_timer_slot_snapshot(
    const void*      _timer,
    struct dj_timer* _out
)
{
    // parameter validation
    if ( (!_timer) ||
         (!_out) )
    {
        return false;
    }

    *_out = *(const struct dj_timer*)_timer;

    return true;
}

// d_internal_timer_vtable
//   constant: the plain kind's operations.
static const struct dj_timer_vtable d_internal_timer_vtable =
{
    d_internal_timer_slot_start,
    d_internal_timer_slot_stop,
    d_internal_timer_slot_increment,
    d_internal_timer_slot_decrement,
    d_internal_timer_slot_merge,
    d_internal_timer_slot_reset,
    d_internal_timer_slot_elapsed,
    d_internal_timer_slot_running,
    d_internal_timer_slot_snapshot
};

/*
d_timer_ref
  A NULL timer makes the reference to no timer, so the table is never
paired with nothing.
*/
struct dj_timer_ref
d_timer_ref(
    struct dj_timer* _timer
)
{
    struct dj_timer_ref ref = D_TIMER_REF_NONE;

    // a timer to refer to
    if (_timer)
    {
        ref.vtable = &d_internal_timer_vtable;
        ref.timer  = _timer;
    }

    return ref;
}

/*
d_timer_ref_is_valid
  Both halves, since a table without a timer would pass NULL to a slot.
*/
bool
d_timer_ref_is_valid(
    struct dj_timer_ref _ref
)
{
    return ( (_ref.vtable != NULL) &&
             (_ref.timer  != NULL) );
}

/*
d_timer_ref_start
  The kind's slot, or failure for no timer.
*/
bool
d_timer_ref_start(
    struct dj_timer_ref _ref
)
{
    return (d_timer_ref_is_valid(_ref))
               ? _ref.vtable->start(_ref.timer)
               : false;
}

/*
d_timer_ref_stop
  The kind's slot, or failure for no timer.
*/
bool
d_timer_ref_stop(
    struct dj_timer_ref _ref
)
{
    return (d_timer_ref_is_valid(_ref))
               ? _ref.vtable->stop(_ref.timer)
               : false;
}

/*
d_timer_ref_increment
  The kind's slot, or failure for no timer.
*/
bool
d_timer_ref_increment(
    struct dj_timer_ref _ref,
    int64_t             _amount
)
{
    return (d_timer_ref_is_valid(_ref))
               ? _ref.vtable->increment(_ref.timer,
                                        _amount)
               : false;
}

/*
d_timer_ref_decrement
  The kind's slot, or failure for no timer.
*/
bool
d_timer_ref_decrement(
    struct dj_timer_ref _ref,
    int64_t             _amount
)
{
    return (d_timer_ref_is_valid(_ref))
               ? _ref.vtable->decrement(_ref.timer,
                                        _amount)
               : false;
}

/*
d_timer_ref_compare
  By elapsed time, as d_timer_compare, with no timer in NULL's place.
*/
int
d_timer_ref_compare(
    struct dj_timer_ref _a,
    struct dj_timer_ref _b
)
{
    const bool is_a = d_timer_ref_is_valid(_a);
    const bool is_b = d_timer_ref_is_valid(_b);

    // no timer orders first, and equal to itself
    if ( (!is_a) ||
         (!is_b) )
    {
        return (is_a == is_b) ? 0 : ( (is_a) ? 1 : -1 );
    }

    const int64_t a = _a.vtable->elapsed(_a.timer);
    const int64_t b = _b.vtable->elapsed(_b.timer);

    return (a < b) ? -1 : ( (a > b) ? 1 : 0 );
}

/*
d_timer_ref_merge
  The source is read once, through its snapshot, before the destination is
touched.
*/
bool
d_timer_ref_merge(
    struct dj_timer_ref _destination,
    struct dj_timer_ref _source
)
{
    struct dj_timer source = D_TIMER_INIT;

    // both timers, and the source readable
    if ( (!d_timer_ref_is_valid(_destination)) ||
         (!d_timer_ref_snapshot(_source,
                                &source)) )
    {
        return false;
    }

    return _destination.vtable->merge(_destination.timer,
                                      &source);
}

/*
d_timer_ref_reset
  The kind's slot; nothing for no timer.
*/
void
d_timer_ref_reset(
    struct dj_timer_ref _ref
)
{
    // a timer to reset
    if (d_timer_ref_is_valid(_ref))
    {
        _ref.vtable->reset(_ref.timer);
    }

    return;
}

/*
d_timer_ref_elapsed
  The kind's slot, or 0 for no timer.
*/
int64_t
d_timer_ref_elapsed(
    struct dj_timer_ref _ref
)
{
    return (d_timer_ref_is_valid(_ref))
               ? _ref.vtable->elapsed(_ref.timer)
               : 0;
}

/*
d_timer_ref_remaining
  From a snapshot, which carries the limit and reads on as the timer does.
*/
int64_t
d_timer_ref_remaining(
    struct dj_timer_ref _ref
)
{
    struct dj_timer snapshot = D_TIMER_INIT;

    return (d_timer_ref_snapshot(_ref,
                                 &snapshot))
               ? d_timer_remaining(&snapshot)
               : 0;
}

/*
d_timer_ref_running
  The kind's slot, or false for no timer.
*/
bool
d_timer_ref_running(
    struct dj_timer_ref _ref
)
{
    return ( (d_timer_ref_is_valid(_ref)) &&
             (_ref.vtable->running(_ref.timer)) );
}

/*
d_timer_ref_expired
  From a snapshot, as d_timer_ref_remaining.
*/
bool
d_timer_ref_expired(
    struct dj_timer_ref _ref
)
{
    struct dj_timer snapshot = D_TIMER_INIT;

    return ( (d_timer_ref_snapshot(_ref,
                                   &snapshot)) &&
             (d_timer_expired(&snapshot)) );
}

/*
d_timer_ref_snapshot
  The kind's slot, which owns the reading: a lock, an atomic load, or a copy.
*/
bool
d_timer_ref_snapshot(
    struct dj_timer_ref _ref,
    struct dj_timer*    _out
)
{
    // parameter validation
    if ( (!d_timer_ref_is_valid(_ref)) ||
         (!_out) )
    {
        return false;
    }

    return _ref.vtable->snapshot(_ref.timer,
                                 _out);
}
