/*******************************************************************************
* djinterp [c][utility]                                           timer_atomic.c
*
* The atomic timer. Every change loads the state word, decodes v and the
* running bit, computes the next state with counter.h's saturating
* arithmetic, and publishes it by compare-and-swap, retrying when another
* thread got there first. The clock is read once per call, before the loop,
* so a retry never reads it again.
*
*
* path:      /src/djinterp/c/util/timer/timer_atomic.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.09
*                                                            revised: 2026.11.10
*******************************************************************************/
#include "../../../../../inc/djinterp/c/util/timer/timer_atomic.h"  // corresponding header
// djinterp
#include "../../../../../inc/djinterp/c/util/counter/counter.h"  // arithmetic


// FLOOR: the header is empty where there is no long long, and so is this file
#if D_ENV_HAS_LONG_LONG

/**
 * @brief Encodes v and the running bit as a state word.
 *
 * @param[in] _v           the value; within +/-D_TIMER_ATOMIC_MAX.
 * @param[in] _is_running  whether an interval is open.
 * @return 2 * `_v` + `_is_running`.
 */
static long long
d_internal_timer_atomic_encode(
    int64_t _v,
    bool    _is_running
)
{
    return ((long long)_v * 2) + ( (_is_running) ? 1 : 0 );
}

/**
 * @brief Whether a state word's interval is open.
 *
 * @param[in] _state  the word.
 * @return its running bit.
 */
static bool
d_internal_timer_atomic_running(
    long long _state
)
{
    return ((_state % 2) != 0);
}

/**
 * @brief A state word's v.
 *
 * @param[in] _state  the word.
 * @return (`_state` - its running bit) / 2, exact for either sign.
 */
static int64_t
d_internal_timer_atomic_value(
    long long _state
)
{
    const long long bit = (d_internal_timer_atomic_running(_state)) ? 1 : 0;

    return (int64_t)((_state - bit) / 2);
}

/**
 * @brief The range a stopped timer's v, its elapsed time, may take.
 *
 * @return [0, D_TIMER_ATOMIC_MAX].
 */
static struct d_interval_imax
d_internal_timer_atomic_time_range(void)
{
    return d_interval_imax_make(0,
                                (d_math_imax)D_TIMER_ATOMIC_MAX,
                                D_INTERVAL_CLOSED,
                                0);
}

/**
 * @brief The range any v may take, so that it fits the state word.
 *
 * @return [-D_TIMER_ATOMIC_MAX, D_TIMER_ATOMIC_MAX].
 */
static struct d_interval_imax
d_internal_timer_atomic_word_range(void)
{
    return d_interval_imax_make(-(d_math_imax)D_TIMER_ATOMIC_MAX,
                                (d_math_imax)D_TIMER_ATOMIC_MAX,
                                D_INTERVAL_CLOSED,
                                0);
}

/**
 * @brief The range a running timer's v may take, given the clock's reading.
 *
 * @param[in] _now  the reading.
 * @return the v whose elapsed time, v + `_now`, lies in
 *         [0, D_TIMER_ATOMIC_MAX], held to the word's range.
 *
 * Each side is clamped to the word's range, and clamping keeps their order,
 * so the bounds are never inverted.
 */
static struct d_interval_imax
d_internal_timer_atomic_open_range(
    int64_t _now
)
{
    const struct d_interval_imax word = d_internal_timer_atomic_word_range();
    const int64_t                low  = d_counter_bounds_sub(&word,
                                                             0,
                                                             _now,
                                                             NULL);
    const int64_t                high = d_counter_bounds_sub(&word,
                                                             D_TIMER_ATOMIC_MAX,
                                                             _now,
                                                             NULL);

    return d_interval_imax_make((d_math_imax)low,
                                (d_math_imax)high,
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
d_internal_timer_atomic_now(
    const struct dj_timer_atomic* _timer
)
{
    return (_timer->clock)
               ? _timer->clock(_timer->context)
               : d_timer_clock_monotonic(NULL);
}

/**
 * @brief The elapsed time a decoded state stands for.
 *
 * @param[in] _v           the state's v.
 * @param[in] _is_running  the state's running bit.
 * @param[in] _now         the clock's reading; used only when running.
 * @return v when stopped; v + `_now` within [0, D_TIMER_ATOMIC_MAX] when
 *         running.
 */
static int64_t
d_internal_timer_atomic_time(
    int64_t _v,
    bool    _is_running,
    int64_t _now
)
{
    const struct d_interval_imax range = d_internal_timer_atomic_time_range();

    // a stopped timer's v is its time
    if (!_is_running)
    {
        return _v;
    }

    return d_counter_bounds_add(&range,
                                _v,
                                _now,
                                NULL);
}

/**
 * @brief Moves the time by `_amount` with one successful CAS.
 *
 * @param[in,out] _timer         the timer; not `NULL`.
 * @param[in]     _amount        how far to move it.
 * @param[in]     _is_decrement  `true` to subtract `_amount`, `false` to add
 *                               it.
 * @return whether the published step applied in full.
 *
 * Stopped, v is the time and moves within [0, D_TIMER_ATOMIC_MAX]; running,
 * v moves within the range that keeps v + now there. The clock is read only
 * when a running state is first seen.
 */
static bool
d_internal_timer_atomic_step(
    struct dj_timer_atomic* _timer,
    int64_t                 _amount,
    bool                    _is_decrement
)
{
    long long state    = d_atomic_load_llong(&_timer->state);
    int64_t   now      = 0;
    bool      has_now  = false;
    bool      is_exact = true;

    // compute from what was loaded; retry when someone else moved it
    for (;;)
    {
        const bool running = d_internal_timer_atomic_running(state);

        // the clock, once, and only for a running timer
        if ( (running) &&
             (!has_now) )
        {
            now     = d_internal_timer_atomic_now(_timer);
            has_now = true;
        }

        const struct d_interval_imax range =
            (running)
                ? d_internal_timer_atomic_open_range(now)
                : d_internal_timer_atomic_time_range();
        const int64_t v    = d_internal_timer_atomic_value(state);
        const int64_t next = (_is_decrement)
                                 ? d_counter_bounds_sub(&range,
                                                        v,
                                                        _amount,
                                                        &is_exact)
                                 : d_counter_bounds_add(&range,
                                                        v,
                                                        _amount,
                                                        &is_exact);

        // publish; on failure `state` holds the newer word
        if (d_atomic_compare_exchange_weak_llong(
                &_timer->state,
                &state,
                d_internal_timer_atomic_encode(next,
                                               running)))
        {
            return is_exact;
        }
    }
}

/*
d_timer_atomic_init
  A stopped timer at zero, with no limit, on the monotonic clock.
*/
void
d_timer_atomic_init(
    struct dj_timer_atomic* _timer
)
{
    // parameter validation
    if (!_timer)
    {
        return;
    }

    d_atomic_init_llong(&_timer->state,
                        0);
    _timer->limit     = 0;
    _timer->has_limit = false;
    _timer->clock     = NULL;
    _timer->context   = NULL;

    return;
}

/*
d_timer_atomic_init_limited
  As timer.h's: a negative limit is refused. A limit above
D_TIMER_ATOMIC_MAX is accepted, and is never reached.
*/
bool
d_timer_atomic_init_limited(
    struct dj_timer_atomic* _timer,
    int64_t                 _limit
)
{
    // parameter validation
    if ( (!_timer) ||
         (_limit < 0) )
    {
        return false;
    }

    d_timer_atomic_init(_timer);
    _timer->limit     = _limit;
    _timer->has_limit = true;

    return true;
}

/*
d_timer_atomic_set_clock
  The clock and its argument travel together.
*/
void
d_timer_atomic_set_clock(
    struct dj_timer_atomic* _timer,
    fn_timer_clock          _clock,
    void*                   _context
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
d_timer_atomic_start
  v becomes the time less the clock's reading. An expired timer does not
restart, as in timer.h.
*/
bool
d_timer_atomic_start(
    struct dj_timer_atomic* _timer
)
{
    // parameter validation
    if (!_timer)
    {
        return false;
    }

    const struct d_interval_imax word  = d_internal_timer_atomic_word_range();
    const int64_t                now   = d_internal_timer_atomic_now(_timer);
    long long                    state = d_atomic_load_llong(&_timer->state);

    // compute from what was loaded; retry when someone else moved it
    for (;;)
    {
        const int64_t v = d_internal_timer_atomic_value(state);

        // the two states that cannot start
        if ( (d_internal_timer_atomic_running(state)) ||
             ( (_timer->has_limit) &&
               (v >= _timer->limit) ) )
        {
            return false;
        }

        const int64_t opened = d_counter_bounds_sub(&word,
                                                    v,
                                                    now,
                                                    NULL);

        // publish; on failure `state` holds the newer word
        if (d_atomic_compare_exchange_weak_llong(
                &_timer->state,
                &state,
                d_internal_timer_atomic_encode(opened,
                                               true)))
        {
            return true;
        }
    }
}

/*
d_timer_atomic_stop
  v becomes the time it stands for at the clock's reading.
*/
bool
d_timer_atomic_stop(
    struct dj_timer_atomic* _timer
)
{
    // parameter validation
    if (!_timer)
    {
        return false;
    }

    const int64_t now   = d_internal_timer_atomic_now(_timer);
    long long     state = d_atomic_load_llong(&_timer->state);

    // compute from what was loaded; retry when someone else moved it
    for (;;)
    {
        // nothing to stop
        if (!d_internal_timer_atomic_running(state))
        {
            return false;
        }

        const int64_t closed = d_internal_timer_atomic_time(
                                   d_internal_timer_atomic_value(state),
                                   true,
                                   now);

        // publish; on failure `state` holds the newer word
        if (d_atomic_compare_exchange_weak_llong(
                &_timer->state,
                &state,
                d_internal_timer_atomic_encode(closed,
                                               false)))
        {
            return true;
        }
    }
}

/*
d_timer_atomic_increment
  One CAS step up.
*/
bool
d_timer_atomic_increment(
    struct dj_timer_atomic* _timer,
    int64_t                 _amount
)
{
    // parameter validation
    if (!_timer)
    {
        return false;
    }

    return d_internal_timer_atomic_step(_timer,
                                        _amount,
                                        false);
}

/*
d_timer_atomic_decrement
  One CAS step down; running, the open interval may be shortened.
*/
bool
d_timer_atomic_decrement(
    struct dj_timer_atomic* _timer,
    int64_t                 _amount
)
{
    // parameter validation
    if (!_timer)
    {
        return false;
    }

    return d_internal_timer_atomic_step(_timer,
                                        _amount,
                                        true);
}

/*
d_timer_atomic_compare
  By elapsed time, each read once; NULL sorts first.
*/
int
d_timer_atomic_compare(
    const struct dj_timer_atomic* _a,
    const struct dj_timer_atomic* _b
)
{
    // NULL before any timer, and equal to itself
    if ( (!_a) ||
         (!_b) )
    {
        return (_a == _b) ? 0 : ( (!_a) ? -1 : 1 );
    }

    const int64_t a = d_timer_atomic_elapsed(_a);
    const int64_t b = d_timer_atomic_elapsed(_b);

    // compared, not subtracted
    if (a < b)
    {
        return -1;
    }

    return (a > b) ? 1 : 0;
}

/*
d_timer_atomic_merge
  The source's elapsed time, read once, added in one step. A timer merged
into itself adds its own time once.
*/
bool
d_timer_atomic_merge(
    struct dj_timer_atomic*       _destination,
    const struct dj_timer_atomic* _source
)
{
    // parameter validation
    if ( (!_destination) ||
         (!_source) )
    {
        return false;
    }

    return d_timer_atomic_increment(_destination,
                                    d_timer_atomic_elapsed(_source));
}

/*
d_timer_atomic_merge_timer
  A plain timer's elapsed time, read before the step.
*/
bool
d_timer_atomic_merge_timer(
    struct dj_timer_atomic* _destination,
    const struct dj_timer*  _source
)
{
    // parameter validation
    if ( (!_destination) ||
         (!_source) )
    {
        return false;
    }

    return d_timer_atomic_increment(_destination,
                                    d_timer_elapsed(_source));
}

/*
d_timer_atomic_reset
  One store: stopped and zero. The limit and the clock stay.
*/
void
d_timer_atomic_reset(
    struct dj_timer_atomic* _timer
)
{
    // parameter validation
    if (!_timer)
    {
        return;
    }

    d_atomic_store_llong(&_timer->state,
                         0);

    return;
}

/*
d_timer_atomic_elapsed
  One load, and the clock only when running.
*/
int64_t
d_timer_atomic_elapsed(
    const struct dj_timer_atomic* _timer
)
{
    // parameter validation
    if (!_timer)
    {
        return 0;
    }

    const long long state   = d_atomic_load_llong(&_timer->state);
    const bool      running = d_internal_timer_atomic_running(state);

    return d_internal_timer_atomic_time(
               d_internal_timer_atomic_value(state),
               running,
               (running) ? d_internal_timer_atomic_now(_timer) : 0);
}

/*
d_timer_atomic_remaining
  The limit less the elapsed time, never below zero.
*/
int64_t
d_timer_atomic_remaining(
    const struct dj_timer_atomic* _timer
)
{
    // parameter validation, and no limit to approach
    if ( (!_timer) ||
         (!_timer->has_limit) )
    {
        return 0;
    }

    const int64_t elapsed = d_timer_atomic_elapsed(_timer);

    return (elapsed < _timer->limit) ? (_timer->limit - elapsed) : 0;
}

/*
d_timer_atomic_running
  The running bit of one load.
*/
bool
d_timer_atomic_running(
    const struct dj_timer_atomic* _timer
)
{
    return ( (_timer) &&
             (d_internal_timer_atomic_running(
                  d_atomic_load_llong(&_timer->state))) );
}

/*
d_timer_atomic_expired
  A limit of zero is met at once.
*/
bool
d_timer_atomic_expired(
    const struct dj_timer_atomic* _timer
)
{
    return ( (_timer)            &&
             (_timer->has_limit) &&
             (d_timer_atomic_elapsed(_timer) >= _timer->limit) );
}

/*
d_timer_atomic_snapshot
  A plain timer with the same configuration and the time of one load. A
running snapshot holds the time so far as closed and opens its interval at
the reading taken, so it reads on from there as the atomic timer does.
*/
bool
d_timer_atomic_snapshot(
    const struct dj_timer_atomic* _timer,
    struct dj_timer*              _out
)
{
    // parameter validation
    if ( (!_timer) ||
         (!_out) )
    {
        return false;
    }

    const long long state   = d_atomic_load_llong(&_timer->state);
    const bool      running = d_internal_timer_atomic_running(state);
    const int64_t   now     = (running) ? d_internal_timer_atomic_now(_timer)
                                        : 0;

    d_timer_init(_out);
    _out->elapsed   = d_internal_timer_atomic_time(
                          d_internal_timer_atomic_value(state),
                          running,
                          now);
    _out->started   = now;
    _out->running   = running;
    _out->limit     = _timer->limit;
    _out->has_limit = _timer->has_limit;
    _out->clock     = _timer->clock;
    _out->context   = _timer->context;

    return true;
}

// the atomic kind's table: this file's functions over an untyped pointer

/**
 * @brief The atomic kind's start slot.
 *
 * @param[in,out] _timer  a struct dj_timer_atomic.
 * @return as d_timer_atomic_start.
 */
static bool
d_internal_timer_atomic_slot_start(
    void* _timer
)
{
    return d_timer_atomic_start((struct dj_timer_atomic*)_timer);
}

/**
 * @brief The atomic kind's stop slot.
 *
 * @param[in,out] _timer  a struct dj_timer_atomic.
 * @return as d_timer_atomic_stop.
 */
static bool
d_internal_timer_atomic_slot_stop(
    void* _timer
)
{
    return d_timer_atomic_stop((struct dj_timer_atomic*)_timer);
}

/**
 * @brief The atomic kind's increment slot.
 *
 * @param[in,out] _timer   a struct dj_timer_atomic.
 * @param[in]     _amount  as d_timer_atomic_increment's.
 * @return as d_timer_atomic_increment.
 */
static bool
d_internal_timer_atomic_slot_increment(
    void*   _timer,
    int64_t _amount
)
{
    return d_timer_atomic_increment((struct dj_timer_atomic*)_timer,
                                    _amount);
}

/**
 * @brief The atomic kind's decrement slot.
 *
 * @param[in,out] _timer   a struct dj_timer_atomic.
 * @param[in]     _amount  as d_timer_atomic_decrement's.
 * @return as d_timer_atomic_decrement.
 */
static bool
d_internal_timer_atomic_slot_decrement(
    void*   _timer,
    int64_t _amount
)
{
    return d_timer_atomic_decrement((struct dj_timer_atomic*)_timer,
                                    _amount);
}

/**
 * @brief The atomic kind's merge slot.
 *
 * @param[in,out] _destination  a struct dj_timer_atomic.
 * @param[in]     _source       as d_timer_atomic_merge_timer's.
 * @return as d_timer_atomic_merge_timer.
 */
static bool
d_internal_timer_atomic_slot_merge(
    void*                  _destination,
    const struct dj_timer* _source
)
{
    struct dj_timer_atomic* destination =
        (struct dj_timer_atomic*)_destination;

    return d_timer_atomic_merge_timer(destination,
                                      _source);
}

/**
 * @brief The atomic kind's reset slot.
 *
 * @param[in,out] _timer  a struct dj_timer_atomic.
 */
static void
d_internal_timer_atomic_slot_reset(
    void* _timer
)
{
    d_timer_atomic_reset((struct dj_timer_atomic*)_timer);

    return;
}

/**
 * @brief The atomic kind's elapsed slot.
 *
 * @param[in] _timer  a struct dj_timer_atomic.
 * @return as d_timer_atomic_elapsed.
 */
static int64_t
d_internal_timer_atomic_slot_elapsed(
    const void* _timer
)
{
    return d_timer_atomic_elapsed((const struct dj_timer_atomic*)_timer);
}

/**
 * @brief The atomic kind's running slot.
 *
 * @param[in] _timer  a struct dj_timer_atomic.
 * @return as d_timer_atomic_running.
 */
static bool
d_internal_timer_atomic_slot_running(
    const void* _timer
)
{
    return d_timer_atomic_running((const struct dj_timer_atomic*)_timer);
}

/**
 * @brief The atomic kind's snapshot slot.
 *
 * @param[in]  _timer  a struct dj_timer_atomic.
 * @param[out] _out    as d_timer_atomic_snapshot's.
 * @return as d_timer_atomic_snapshot.
 */
static bool
d_internal_timer_atomic_slot_snapshot(
    const void*      _timer,
    struct dj_timer* _out
)
{
    return d_timer_atomic_snapshot((const struct dj_timer_atomic*)_timer,
                                   _out);
}

// d_internal_timer_atomic_vtable
//   constant: the atomic kind's operations.
static const struct dj_timer_vtable d_internal_timer_atomic_vtable =
{
    d_internal_timer_atomic_slot_start,
    d_internal_timer_atomic_slot_stop,
    d_internal_timer_atomic_slot_increment,
    d_internal_timer_atomic_slot_decrement,
    d_internal_timer_atomic_slot_merge,
    d_internal_timer_atomic_slot_reset,
    d_internal_timer_atomic_slot_elapsed,
    d_internal_timer_atomic_slot_running,
    d_internal_timer_atomic_slot_snapshot
};

/*
d_timer_atomic_ref
  Every slot is one of this file's lock-free functions, so a reference
shares the timer as safely as the functions do.
*/
struct dj_timer_ref
d_timer_atomic_ref(
    struct dj_timer_atomic* _timer
)
{
    struct dj_timer_ref ref = D_TIMER_REF_NONE;

    // a timer to refer to
    if (_timer)
    {
        ref.vtable = &d_internal_timer_atomic_vtable;
        ref.timer  = _timer;
    }

    return ref;
}

#endif  // floor: D_ENV_HAS_LONG_LONG
