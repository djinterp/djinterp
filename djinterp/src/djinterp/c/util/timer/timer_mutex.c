/*******************************************************************************
* djinterp [c][utility]                                            timer_mutex.c
*
* The guarded timer: each function locks, calls timer.h's function of the
* same name, and unlocks. A query takes a const timer, but locking writes the
* mutex, so the lock is reached through a cast; the mutex is the timer's
* bookkeeping, not its value, and a guarded timer is never itself defined
* const.
*
*
* path:      /src/djinterp/c/util/timer/timer_mutex.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.09
*                                                            revised: 2026.11.10
*******************************************************************************/
#include "../../../../../inc/djinterp/c/util/timer/timer_mutex.h"  // corresponding header


/**
 * @brief Locks a guarded timer, if it is ready.
 *
 * @param[in] _timer  the timer; may be `NULL`.
 * @return the timer, locked, or `NULL` when it is `NULL`, not ready, or the
 *         lock failed; a `NULL` return holds no lock.
 */
static struct dj_timer_mutex*
d_internal_timer_mutex_lock(
    const struct dj_timer_mutex* _timer
)
{
    struct dj_timer_mutex* timer = (struct dj_timer_mutex*)_timer;

    // only a ready timer has a lock to take
    if ( (!timer)        ||
         (!timer->ready) ||
         (d_mutex_lock(&timer->lock) != D_MUTEX_SUCCESS) )
    {
        return NULL;
    }

    return timer;
}

/**
 * @brief Unlocks a timer locked by d_internal_timer_mutex_lock.
 *
 * @param[in] _timer  the locked timer; `NULL` is ignored.
 */
static void
d_internal_timer_mutex_unlock(
    struct dj_timer_mutex* _timer
)
{
    // nothing held for NULL
    if (_timer)
    {
        (void)d_mutex_unlock(&_timer->lock);
    }

    return;
}

/*
d_timer_mutex_init_limited
  The timer is set before the mutex is made, and `ready` last, so a failed
init leaves a timer every function treats as not ready.
*/
bool
d_timer_mutex_init_limited(
    struct dj_timer_mutex* _timer,
    int64_t                _limit
)
{
    // parameter validation
    if (!_timer)
    {
        return false;
    }

    _timer->ready = false;

    // timer.h refuses a negative limit, and then there is nothing to guard
    if (!d_timer_init_limited(&_timer->timer,
                              _limit))
    {
        return false;
    }

    // the mutex last: it is the step that can fail
    if (d_mutex_init(&_timer->lock) != D_MUTEX_SUCCESS)
    {
        return false;
    }

    _timer->ready = true;

    return true;
}

/*
d_timer_mutex_init
  init_limited with no limit: timer.h's init, then the mutex.
*/
bool
d_timer_mutex_init(
    struct dj_timer_mutex* _timer
)
{
    // parameter validation
    if (!_timer)
    {
        return false;
    }

    _timer->ready = false;
    d_timer_init(&_timer->timer);

    // the mutex last: it is the step that can fail
    if (d_mutex_init(&_timer->lock) != D_MUTEX_SUCCESS)
    {
        return false;
    }

    _timer->ready = true;

    return true;
}

/*
d_timer_mutex_destroy
  `ready` goes first, so nothing locks a mutex being destroyed by a caller
who broke the precondition; that is a mitigation, not a guarantee.
*/
void
d_timer_mutex_destroy(
    struct dj_timer_mutex* _timer
)
{
    // nothing to release
    if ( (!_timer) ||
         (!_timer->ready) )
    {
        return;
    }

    _timer->ready = false;
    (void)d_mutex_destroy(&_timer->lock);

    return;
}

/*
d_timer_mutex_set_clock
  timer.h's, under the lock.
*/
void
d_timer_mutex_set_clock(
    struct dj_timer_mutex* _timer,
    fn_timer_clock         _clock,
    void*                  _context
)
{
    struct dj_timer_mutex* locked =
        d_internal_timer_mutex_lock(_timer);

    // only a ready timer is changed
    if (locked)
    {
        d_timer_set_clock(&locked->timer,
                          _clock,
                          _context);
        d_internal_timer_mutex_unlock(locked);
    }

    return;
}

/*
d_timer_mutex_start
  timer.h's, under the lock.
*/
bool
d_timer_mutex_start(
    struct dj_timer_mutex* _timer
)
{
    struct dj_timer_mutex* locked =
        d_internal_timer_mutex_lock(_timer);

    // only a ready timer starts
    if (!locked)
    {
        return false;
    }

    const bool result = d_timer_start(&locked->timer);

    d_internal_timer_mutex_unlock(locked);

    return result;
}

/*
d_timer_mutex_stop
  timer.h's, under the lock.
*/
bool
d_timer_mutex_stop(
    struct dj_timer_mutex* _timer
)
{
    struct dj_timer_mutex* locked =
        d_internal_timer_mutex_lock(_timer);

    // only a ready timer stops
    if (!locked)
    {
        return false;
    }

    const bool result = d_timer_stop(&locked->timer);

    d_internal_timer_mutex_unlock(locked);

    return result;
}

/*
d_timer_mutex_increment
  timer.h's, under the lock.
*/
bool
d_timer_mutex_increment(
    struct dj_timer_mutex* _timer,
    int64_t                _amount
)
{
    struct dj_timer_mutex* locked =
        d_internal_timer_mutex_lock(_timer);

    // only a ready timer moves
    if (!locked)
    {
        return false;
    }

    const bool result = d_timer_increment(&locked->timer,
                                          _amount);

    d_internal_timer_mutex_unlock(locked);

    return result;
}

/*
d_timer_mutex_decrement
  timer.h's, under the lock.
*/
bool
d_timer_mutex_decrement(
    struct dj_timer_mutex* _timer,
    int64_t                _amount
)
{
    struct dj_timer_mutex* locked =
        d_internal_timer_mutex_lock(_timer);

    // only a ready timer moves
    if (!locked)
    {
        return false;
    }

    const bool result = d_timer_decrement(&locked->timer,
                                          _amount);

    d_internal_timer_mutex_unlock(locked);

    return result;
}

/*
d_timer_mutex_compare
  Two snapshots, taken one lock at a time, compared as plain timers. A timer
not ready compares as NULL does.
*/
int
d_timer_mutex_compare(
    const struct dj_timer_mutex* _a,
    const struct dj_timer_mutex* _b
)
{
    struct dj_timer a = D_TIMER_INIT;
    struct dj_timer b = D_TIMER_INIT;

    const bool has_a = d_timer_mutex_snapshot(_a,
                                              &a);
    const bool has_b = d_timer_mutex_snapshot(_b,
                                              &b);

    return d_timer_compare((has_a) ? &a : NULL,
                           (has_b) ? &b : NULL);
}

/*
d_timer_mutex_merge
  The source's elapsed time is read under its own lock and released before
the destination is locked, so no thread ever holds two of these locks.
*/
bool
d_timer_mutex_merge(
    struct dj_timer_mutex*       _destination,
    const struct dj_timer_mutex* _source
)
{
    struct dj_timer source = D_TIMER_INIT;

    // a source not ready has nothing to give
    if (!d_timer_mutex_snapshot(_source,
                                &source))
    {
        return false;
    }

    return d_timer_mutex_merge_timer(_destination,
                                     &source);
}

/*
d_timer_mutex_merge_timer
  The plain timer is read before the lock is taken: it is the caller's, and
reading its clock should not lengthen the critical section.
*/
bool
d_timer_mutex_merge_timer(
    struct dj_timer_mutex* _destination,
    const struct dj_timer* _source
)
{
    // parameter validation
    if (!_source)
    {
        return false;
    }

    return d_timer_mutex_increment(_destination,
                                   d_timer_elapsed(_source));
}

/*
d_timer_mutex_reset
  timer.h's, under the lock.
*/
void
d_timer_mutex_reset(
    struct dj_timer_mutex* _timer
)
{
    struct dj_timer_mutex* locked =
        d_internal_timer_mutex_lock(_timer);

    // only a ready timer is reset
    if (locked)
    {
        d_timer_reset(&locked->timer);
        d_internal_timer_mutex_unlock(locked);
    }

    return;
}

/*
d_timer_mutex_elapsed
  timer.h's, of a snapshot.
*/
int64_t
d_timer_mutex_elapsed(
    const struct dj_timer_mutex* _timer
)
{
    struct dj_timer snapshot = D_TIMER_INIT;

    return (d_timer_mutex_snapshot(_timer,
                                   &snapshot))
               ? d_timer_elapsed(&snapshot)
               : 0;
}

/*
d_timer_mutex_remaining
  timer.h's, of a snapshot.
*/
int64_t
d_timer_mutex_remaining(
    const struct dj_timer_mutex* _timer
)
{
    struct dj_timer snapshot = D_TIMER_INIT;

    return (d_timer_mutex_snapshot(_timer,
                                   &snapshot))
               ? d_timer_remaining(&snapshot)
               : 0;
}

/*
d_timer_mutex_running
  timer.h's, of a snapshot.
*/
bool
d_timer_mutex_running(
    const struct dj_timer_mutex* _timer
)
{
    struct dj_timer snapshot = D_TIMER_INIT;

    return ( (d_timer_mutex_snapshot(_timer,
                                     &snapshot)) &&
             (d_timer_running(&snapshot)) );
}

/*
d_timer_mutex_expired
  timer.h's, of a snapshot.
*/
bool
d_timer_mutex_expired(
    const struct dj_timer_mutex* _timer
)
{
    struct dj_timer snapshot = D_TIMER_INIT;

    return ( (d_timer_mutex_snapshot(_timer,
                                     &snapshot)) &&
             (d_timer_expired(&snapshot)) );
}

/*
d_timer_mutex_snapshot
  A copy under the lock. A running snapshot keeps running: its open
interval's start is copied, and its clock reads on from there.
*/
bool
d_timer_mutex_snapshot(
    const struct dj_timer_mutex* _timer,
    struct dj_timer*             _out
)
{
    // parameter validation
    if (!_out)
    {
        return false;
    }

    struct dj_timer_mutex* locked =
        d_internal_timer_mutex_lock(_timer);

    // only a ready timer has a value to copy
    if (!locked)
    {
        return false;
    }

    *_out = locked->timer;
    d_internal_timer_mutex_unlock(locked);

    return true;
}

// the guarded kind's table: this file's functions over an untyped pointer

/**
 * @brief The guarded kind's start slot.
 *
 * @param[in,out] _timer  a struct dj_timer_mutex.
 * @return as d_timer_mutex_start.
 */
static bool
d_internal_timer_mutex_slot_start(
    void* _timer
)
{
    return d_timer_mutex_start((struct dj_timer_mutex*)_timer);
}

/**
 * @brief The guarded kind's stop slot.
 *
 * @param[in,out] _timer  a struct dj_timer_mutex.
 * @return as d_timer_mutex_stop.
 */
static bool
d_internal_timer_mutex_slot_stop(
    void* _timer
)
{
    return d_timer_mutex_stop((struct dj_timer_mutex*)_timer);
}

/**
 * @brief The guarded kind's increment slot.
 *
 * @param[in,out] _timer   a struct dj_timer_mutex.
 * @param[in]     _amount  as d_timer_mutex_increment's.
 * @return as d_timer_mutex_increment.
 */
static bool
d_internal_timer_mutex_slot_increment(
    void*   _timer,
    int64_t _amount
)
{
    return d_timer_mutex_increment((struct dj_timer_mutex*)_timer,
                                   _amount);
}

/**
 * @brief The guarded kind's decrement slot.
 *
 * @param[in,out] _timer   a struct dj_timer_mutex.
 * @param[in]     _amount  as d_timer_mutex_decrement's.
 * @return as d_timer_mutex_decrement.
 */
static bool
d_internal_timer_mutex_slot_decrement(
    void*   _timer,
    int64_t _amount
)
{
    return d_timer_mutex_decrement((struct dj_timer_mutex*)_timer,
                                   _amount);
}

/**
 * @brief The guarded kind's merge slot.
 *
 * @param[in,out] _destination  a struct dj_timer_mutex.
 * @param[in]     _source       as d_timer_mutex_merge_timer's.
 * @return as d_timer_mutex_merge_timer.
 */
static bool
d_internal_timer_mutex_slot_merge(
    void*                  _destination,
    const struct dj_timer* _source
)
{
    struct dj_timer_mutex* destination =
        (struct dj_timer_mutex*)_destination;

    return d_timer_mutex_merge_timer(destination,
                                     _source);
}

/**
 * @brief The guarded kind's reset slot.
 *
 * @param[in,out] _timer  a struct dj_timer_mutex.
 */
static void
d_internal_timer_mutex_slot_reset(
    void* _timer
)
{
    d_timer_mutex_reset((struct dj_timer_mutex*)_timer);

    return;
}

/**
 * @brief The guarded kind's elapsed slot.
 *
 * @param[in] _timer  a struct dj_timer_mutex.
 * @return as d_timer_mutex_elapsed.
 */
static int64_t
d_internal_timer_mutex_slot_elapsed(
    const void* _timer
)
{
    return d_timer_mutex_elapsed((const struct dj_timer_mutex*)_timer);
}

/**
 * @brief The guarded kind's running slot.
 *
 * @param[in] _timer  a struct dj_timer_mutex.
 * @return as d_timer_mutex_running.
 */
static bool
d_internal_timer_mutex_slot_running(
    const void* _timer
)
{
    return d_timer_mutex_running((const struct dj_timer_mutex*)_timer);
}

/**
 * @brief The guarded kind's snapshot slot.
 *
 * @param[in]  _timer  a struct dj_timer_mutex.
 * @param[out] _out    as d_timer_mutex_snapshot's.
 * @return as d_timer_mutex_snapshot.
 */
static bool
d_internal_timer_mutex_slot_snapshot(
    const void*      _timer,
    struct dj_timer* _out
)
{
    return d_timer_mutex_snapshot((const struct dj_timer_mutex*)_timer,
                                  _out);
}

// d_internal_timer_mutex_vtable
//   constant: the guarded kind's operations.
static const struct dj_timer_vtable d_internal_timer_mutex_vtable =
{
    d_internal_timer_mutex_slot_start,
    d_internal_timer_mutex_slot_stop,
    d_internal_timer_mutex_slot_increment,
    d_internal_timer_mutex_slot_decrement,
    d_internal_timer_mutex_slot_merge,
    d_internal_timer_mutex_slot_reset,
    d_internal_timer_mutex_slot_elapsed,
    d_internal_timer_mutex_slot_running,
    d_internal_timer_mutex_slot_snapshot
};

/*
d_timer_mutex_ref
  Every slot takes the lock for one call, as this file's functions do;
a timer not set up stays one through the reference.
*/
struct dj_timer_ref
d_timer_mutex_ref(
    struct dj_timer_mutex* _timer
)
{
    struct dj_timer_ref ref = D_TIMER_REF_NONE;

    // a timer to refer to
    if (_timer)
    {
        ref.vtable = &d_internal_timer_mutex_vtable;
        ref.timer  = _timer;
    }

    return ref;
}
