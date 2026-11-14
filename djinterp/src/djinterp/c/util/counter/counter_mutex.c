/*******************************************************************************
* djinterp [c][utility]                                          counter_mutex.c
*
* The guarded counter: each function locks, calls counter.h's function of the
* same name, and unlocks. A query takes a const counter, but locking writes
* the mutex, so the lock is reached through a cast; the mutex is the counter's
* bookkeeping, not its value, and a guarded counter is never itself defined
* const.
*
*
* path:      /src/djinterp/c/util/counter/counter_mutex.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.09
*                                                            revised: 2026.11.10
*******************************************************************************/
#include "../../../../../inc/djinterp/c/util/counter/counter_mutex.h"  // corresponding header


/**
 * @brief Locks a guarded counter, if it is ready.
 *
 * @param[in] _counter  the counter; may be `NULL`.
 * @return the counter, locked, or `NULL` when it is `NULL`, not ready, or the
 *         lock failed; a `NULL` return holds no lock.
 */
static struct dj_counter_mutex*
d_internal_counter_mutex_lock(
    const struct dj_counter_mutex* _counter
)
{
    struct dj_counter_mutex* counter = (struct dj_counter_mutex*)_counter;

    // only a ready counter has a lock to take
    if ( (!counter)        ||
         (!counter->ready) ||
         (d_mutex_lock(&counter->lock) != D_MUTEX_SUCCESS) )
    {
        return NULL;
    }

    return counter;
}

/**
 * @brief Unlocks a counter locked by d_internal_counter_mutex_lock.
 *
 * @param[in] _counter  the locked counter; `NULL` is ignored.
 */
static void
d_internal_counter_mutex_unlock(
    struct dj_counter_mutex* _counter
)
{
    // nothing held for NULL
    if (_counter)
    {
        (void)d_mutex_unlock(&_counter->lock);
    }

    return;
}

/*
d_counter_mutex_init_bounded
  The counter is set before the mutex is made, and `ready` last, so a failed
init leaves a counter every function treats as not ready.
*/
bool
d_counter_mutex_init_bounded(
    struct dj_counter_mutex*      _counter,
    int64_t                       _initial,
    const struct dj_interval_imax* _bounds
)
{
    // parameter validation
    if (!_counter)
    {
        return false;
    }

    _counter->ready = false;

    // counter.h refuses what it refuses, and then there is nothing to guard
    if (!d_counter_init_bounded(&_counter->counter,
                                _initial,
                                _bounds))
    {
        return false;
    }

    // the mutex last: it is the step that can fail
    if (d_mutex_init(&_counter->lock) != D_MUTEX_SUCCESS)
    {
        return false;
    }

    _counter->ready = true;

    return true;
}

/*
d_counter_mutex_init
  init_bounded with no bounds.
*/
bool
d_counter_mutex_init(
    struct dj_counter_mutex* _counter,
    int64_t                  _initial
)
{
    return d_counter_mutex_init_bounded(_counter,
                                        _initial,
                                        NULL);
}

/*
d_counter_mutex_destroy
  `ready` goes first, so nothing locks a mutex being destroyed by a caller
who broke the precondition; that is a mitigation, not a guarantee.
*/
void
d_counter_mutex_destroy(
    struct dj_counter_mutex* _counter
)
{
    // nothing to release
    if ( (!_counter) ||
         (!_counter->ready) )
    {
        return;
    }

    _counter->ready = false;
    (void)d_mutex_destroy(&_counter->lock);

    return;
}

/*
d_counter_mutex_increment
  counter.h's, under the lock.
*/
bool
d_counter_mutex_increment(
    struct dj_counter_mutex* _counter,
    int64_t                  _amount
)
{
    struct dj_counter_mutex* locked =
        d_internal_counter_mutex_lock(_counter);

    // only a ready counter moves
    if (!locked)
    {
        return false;
    }

    const bool result = d_counter_increment(&locked->counter,
                                            _amount);

    d_internal_counter_mutex_unlock(locked);

    return result;
}

/*
d_counter_mutex_decrement
  counter.h's, under the lock.
*/
bool
d_counter_mutex_decrement(
    struct dj_counter_mutex* _counter,
    int64_t                  _amount
)
{
    struct dj_counter_mutex* locked =
        d_internal_counter_mutex_lock(_counter);

    // only a ready counter moves
    if (!locked)
    {
        return false;
    }

    const bool result = d_counter_decrement(&locked->counter,
                                            _amount);

    d_internal_counter_mutex_unlock(locked);

    return result;
}

/*
d_counter_mutex_compare
  Two snapshots, taken one lock at a time, compared as plain counters. A
counter not ready compares as NULL does.
*/
int
d_counter_mutex_compare(
    const struct dj_counter_mutex* _a,
    const struct dj_counter_mutex* _b
)
{
    struct dj_counter a = D_COUNTER_INIT;
    struct dj_counter b = D_COUNTER_INIT;

    const bool has_a = d_counter_mutex_snapshot(_a,
                                                &a);
    const bool has_b = d_counter_mutex_snapshot(_b,
                                                &b);

    return d_counter_compare((has_a) ? &a : NULL,
                             (has_b) ? &b : NULL);
}

/*
d_counter_mutex_merge
  The source is read under its own lock and released before the destination
is locked, so no thread ever holds two of these locks.
*/
bool
d_counter_mutex_merge(
    struct dj_counter_mutex*       _destination,
    const struct dj_counter_mutex* _source
)
{
    struct dj_counter source = D_COUNTER_INIT;

    // a source not ready has nothing to give
    if (!d_counter_mutex_snapshot(_source,
                                  &source))
    {
        return false;
    }

    return d_counter_mutex_merge_counter(_destination,
                                         &source);
}

/*
d_counter_mutex_merge_counter
  counter.h's merge, under the destination's lock; the plain source is the
caller's, and is not locked.
*/
bool
d_counter_mutex_merge_counter(
    struct dj_counter_mutex* _destination,
    const struct dj_counter* _source
)
{
    // parameter validation
    if (!_source)
    {
        return false;
    }

    struct dj_counter_mutex* locked =
        d_internal_counter_mutex_lock(_destination);

    // only a ready counter takes a merge
    if (!locked)
    {
        return false;
    }

    const bool result = d_counter_merge(&locked->counter,
                                        _source);

    d_internal_counter_mutex_unlock(locked);

    return result;
}

/*
d_counter_mutex_reset
  counter.h's, under the lock.
*/
void
d_counter_mutex_reset(
    struct dj_counter_mutex* _counter
)
{
    struct dj_counter_mutex* locked =
        d_internal_counter_mutex_lock(_counter);

    // only a ready counter is reset
    if (locked)
    {
        d_counter_reset(&locked->counter);
        d_internal_counter_mutex_unlock(locked);
    }

    return;
}

/*
d_counter_mutex_value
  counter.h's, of a snapshot.
*/
int64_t
d_counter_mutex_value(
    const struct dj_counter_mutex* _counter
)
{
    struct dj_counter snapshot = D_COUNTER_INIT;

    return (d_counter_mutex_snapshot(_counter,
                                     &snapshot))
               ? d_counter_value(&snapshot)
               : 0;
}

/*
d_counter_mutex_delta
  counter.h's, of a snapshot.
*/
int64_t
d_counter_mutex_delta(
    const struct dj_counter_mutex* _counter
)
{
    struct dj_counter snapshot = D_COUNTER_INIT;

    return (d_counter_mutex_snapshot(_counter,
                                     &snapshot))
               ? d_counter_delta(&snapshot)
               : 0;
}

/*
d_counter_mutex_at_min
  counter.h's, of a snapshot.
*/
bool
d_counter_mutex_at_min(
    const struct dj_counter_mutex* _counter
)
{
    struct dj_counter snapshot = D_COUNTER_INIT;

    return ( (d_counter_mutex_snapshot(_counter,
                                       &snapshot)) &&
             (d_counter_at_min(&snapshot)) );
}

/*
d_counter_mutex_at_max
  counter.h's, of a snapshot.
*/
bool
d_counter_mutex_at_max(
    const struct dj_counter_mutex* _counter
)
{
    struct dj_counter snapshot = D_COUNTER_INIT;

    return ( (d_counter_mutex_snapshot(_counter,
                                       &snapshot)) &&
             (d_counter_at_max(&snapshot)) );
}

/*
d_counter_mutex_snapshot
  A copy under the lock.
*/
bool
d_counter_mutex_snapshot(
    const struct dj_counter_mutex* _counter,
    struct dj_counter*             _out
)
{
    // parameter validation
    if (!_out)
    {
        return false;
    }

    struct dj_counter_mutex* locked =
        d_internal_counter_mutex_lock(_counter);

    // only a ready counter has a value to copy
    if (!locked)
    {
        return false;
    }

    *_out = locked->counter;
    d_internal_counter_mutex_unlock(locked);

    return true;
}

// the guarded kind's table: this file's functions over an untyped pointer

/**
 * @brief The guarded kind's increment slot.
 *
 * @param[in,out] _counter  a struct dj_counter_mutex.
 * @param[in]     _amount   as d_counter_mutex_increment's.
 * @return as d_counter_mutex_increment.
 */
static bool
d_internal_counter_mutex_slot_increment(
    void*   _counter,
    int64_t _amount
)
{
    return d_counter_mutex_increment((struct dj_counter_mutex*)_counter,
                                     _amount);
}

/**
 * @brief The guarded kind's decrement slot.
 *
 * @param[in,out] _counter  a struct dj_counter_mutex.
 * @param[in]     _amount   as d_counter_mutex_decrement's.
 * @return as d_counter_mutex_decrement.
 */
static bool
d_internal_counter_mutex_slot_decrement(
    void*   _counter,
    int64_t _amount
)
{
    return d_counter_mutex_decrement((struct dj_counter_mutex*)_counter,
                                     _amount);
}

/**
 * @brief The guarded kind's merge slot.
 *
 * @param[in,out] _destination  a struct dj_counter_mutex.
 * @param[in]     _source       the counter whose movement is added.
 * @return as d_counter_mutex_merge_counter.
 */
static bool
d_internal_counter_mutex_slot_merge(
    void*                    _destination,
    const struct dj_counter* _source
)
{
    struct dj_counter_mutex* destination =
        (struct dj_counter_mutex*)_destination;

    return d_counter_mutex_merge_counter(destination,
                                         _source);
}

/**
 * @brief The guarded kind's reset slot.
 *
 * @param[in,out] _counter  a struct dj_counter_mutex.
 */
static void
d_internal_counter_mutex_slot_reset(
    void* _counter
)
{
    d_counter_mutex_reset((struct dj_counter_mutex*)_counter);

    return;
}

/**
 * @brief The guarded kind's value slot.
 *
 * @param[in] _counter  a struct dj_counter_mutex.
 * @return as d_counter_mutex_value.
 */
static int64_t
d_internal_counter_mutex_slot_value(
    const void* _counter
)
{
    return d_counter_mutex_value((const struct dj_counter_mutex*)_counter);
}

/**
 * @brief The guarded kind's snapshot slot.
 *
 * @param[in]  _counter  a struct dj_counter_mutex.
 * @param[out] _out      the copy.
 * @return as d_counter_mutex_snapshot.
 */
static bool
d_internal_counter_mutex_slot_snapshot(
    const void*        _counter,
    struct dj_counter* _out
)
{
    return d_counter_mutex_snapshot((const struct dj_counter_mutex*)_counter,
                                    _out);
}

// d_internal_counter_mutex_vtable
//   constant: the guarded kind's operations.
static const struct dj_counter_vtable d_internal_counter_mutex_vtable =
{
    d_internal_counter_mutex_slot_increment,
    d_internal_counter_mutex_slot_decrement,
    d_internal_counter_mutex_slot_merge,
    d_internal_counter_mutex_slot_reset,
    d_internal_counter_mutex_slot_value,
    d_internal_counter_mutex_slot_snapshot
};

/*
d_counter_mutex_ref
  Every slot takes the lock for one call, as this file's functions do;
a counter not set up stays one through the reference.
*/
struct dj_counter_ref
d_counter_mutex_ref(
    struct dj_counter_mutex* _counter
)
{
    struct dj_counter_ref ref = D_COUNTER_REF_NONE;

    // a counter to refer to
    if (_counter)
    {
        ref.vtable  = &d_internal_counter_mutex_vtable;
        ref.counter = _counter;
    }

    return ref;
}
