/*******************************************************************************
* djinterp [c][utility]                                         counter_atomic.c
*
* The atomic counter's steps. Each loads the value, computes the next one
* with counter.h's d_counter_bounds_add, and publishes it by compare-and-swap,
* retrying when another thread got there first. The exactness reported is the
* one computed for the value that was published.
*
*
* path:      /src/djinterp/c/util/counter/counter_atomic.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.09
*                                                            revised: 2026.11.10
*******************************************************************************/
#include "../../../../../inc/djinterp/c/util/counter/counter_atomic.h"  // corresponding header


// FLOOR: the header is empty where there is no long long, and so is this file
#if D_ENV_HAS_LONG_LONG

/**
 * @brief A counter's bounds, for the arithmetic.
 *
 * @param[in] _counter  the counter; not `NULL`.
 * @return its bounds, or `NULL` when it has none.
 */
static const struct d_interval_imax*
d_internal_counter_atomic_bounds(
    const struct dj_counter_atomic* _counter
)
{
    return (_counter->has_bounds) ? &_counter->bounds : NULL;
}

/**
 * @brief Moves the value by `_amount` with one successful CAS.
 *
 * @param[in,out] _counter       the counter; not `NULL`.
 * @param[in]     _amount        how far to move it.
 * @param[in]     _is_decrement  `true` to subtract `_amount`, `false` to add
 *                               it.
 * @return whether the published step applied in full.
 *
 * The next value is a pure function of the loaded one, so a failed CAS,
 * which reloads the value into `current`, simply computes again, with
 * counter.h's arithmetic, so the two forms saturate alike.
 */
static bool
d_internal_counter_atomic_step(
    struct dj_counter_atomic* _counter,
    int64_t                   _amount,
    bool                      _is_decrement
)
{
    const struct d_interval_imax* bounds   =
        d_internal_counter_atomic_bounds(_counter);
    long long                     current  =
        d_atomic_load_llong(&_counter->value);
    bool                          is_exact = true;

    // compute from what was loaded; retry when someone else moved it
    for (;;)
    {
        const int64_t next = (_is_decrement)
                                 ? d_counter_bounds_sub(bounds,
                                                        (int64_t)current,
                                                        _amount,
                                                        &is_exact)
                                 : d_counter_bounds_add(bounds,
                                                        (int64_t)current,
                                                        _amount,
                                                        &is_exact);

        // publish; on failure `current` holds the newer value
        if (d_atomic_compare_exchange_weak_llong(&_counter->value,
                                                 &current,
                                                 (long long)next))
        {
            break;
        }
    }

    return is_exact;
}

/*
d_counter_atomic_init
  d_atomic_init_llong rather than a plain store: an atomic object is not
initialized by assignment under C11's rules.
*/
void
d_counter_atomic_init(
    struct dj_counter_atomic* _counter,
    int64_t                   _initial
)
{
    // parameter validation
    if (!_counter)
    {
        return;
    }

    const struct dj_counter none = D_COUNTER_INIT;

    _counter->initial = _initial;
    d_atomic_init_llong(&_counter->value,
                        (long long)_initial);
    _counter->bounds     = none.bounds;
    _counter->has_bounds = false;

    return;
}

/*
d_counter_atomic_init_bounded
  The plain counter's validation, reused through a scratch counter, so the
two forms refuse exactly the same arguments.
*/
bool
d_counter_atomic_init_bounded(
    struct dj_counter_atomic*     _counter,
    int64_t                       _initial,
    const struct d_interval_imax* _bounds
)
{
    struct dj_counter checked = D_COUNTER_INIT;

    // parameter validation, the plain counter's
    if ( (!_counter) ||
         (!d_counter_init_bounded(&checked,
                                  _initial,
                                  _bounds)) )
    {
        return false;
    }

    _counter->initial = checked.initial;
    d_atomic_init_llong(&_counter->value,
                        (long long)_initial);
    _counter->bounds     = checked.bounds;
    _counter->has_bounds = checked.has_bounds;

    return true;
}

/*
d_counter_atomic_increment
  One CAS step up.
*/
bool
d_counter_atomic_increment(
    struct dj_counter_atomic* _counter,
    int64_t                   _amount
)
{
    // parameter validation
    if (!_counter)
    {
        return false;
    }

    return d_internal_counter_atomic_step(_counter,
                                          _amount,
                                          false);
}

/*
d_counter_atomic_decrement
  One CAS step down.
*/
bool
d_counter_atomic_decrement(
    struct dj_counter_atomic* _counter,
    int64_t                   _amount
)
{
    // parameter validation
    if (!_counter)
    {
        return false;
    }

    return d_internal_counter_atomic_step(_counter,
                                          _amount,
                                          true);
}

/*
d_counter_atomic_compare
  Each value is loaded once, so the answer is true of two moments that may
differ; that is the most a lock-free comparison can say.
*/
int
d_counter_atomic_compare(
    const struct dj_counter_atomic* _a,
    const struct dj_counter_atomic* _b
)
{
    // NULL before any counter, and equal to itself
    if ( (!_a) ||
         (!_b) )
    {
        return (_a == _b) ? 0 : ( (!_a) ? -1 : 1 );
    }

    const long long a = d_atomic_load_llong(&_a->value);
    const long long b = d_atomic_load_llong(&_b->value);

    // compared, not subtracted: the difference could overflow
    if (a < b)
    {
        return -1;
    }

    return (a > b) ? 1 : 0;
}

/*
d_counter_atomic_merge
  The source is read once, through a snapshot, and its movement added in one
step. Merging a counter into itself adds its own movement once.
*/
bool
d_counter_atomic_merge(
    struct dj_counter_atomic*       _destination,
    const struct dj_counter_atomic* _source
)
{
    struct dj_counter source = D_COUNTER_INIT;

    // parameter validation
    if ( (!_destination) ||
         (!d_counter_atomic_snapshot(_source,
                                     &source)) )
    {
        return false;
    }

    return d_counter_atomic_merge_counter(_destination,
                                          &source);
}

/*
d_counter_atomic_merge_counter
  A plain counter's movement, added as one step. The delta is taken first,
saturating, and a saturated delta makes the merge inexact even when the step
fits.
*/
bool
d_counter_atomic_merge_counter(
    struct dj_counter_atomic* _destination,
    const struct dj_counter*  _source
)
{
    // parameter validation
    if ( (!_destination) ||
         (!_source) )
    {
        return false;
    }

    bool          is_delta_exact = true;
    const int64_t delta          = d_counter_bounds_sub(NULL,
                                                        _source->value,
                                                        _source->initial,
                                                        &is_delta_exact);
    const bool    is_step_exact  =
        d_internal_counter_atomic_step(_destination,
                                       delta,
                                       false);

    return ( (is_delta_exact) &&
             (is_step_exact) );
}

/*
d_counter_atomic_reset
  A store: whatever the value was, it becomes the start.
*/
void
d_counter_atomic_reset(
    struct dj_counter_atomic* _counter
)
{
    // parameter validation
    if (!_counter)
    {
        return;
    }

    d_atomic_store_llong(&_counter->value,
                         (long long)_counter->initial);

    return;
}

/*
d_counter_atomic_value
  One load.
*/
int64_t
d_counter_atomic_value(
    const struct dj_counter_atomic* _counter
)
{
    return (_counter)
               ? (int64_t)d_atomic_load_llong(&_counter->value)
               : 0;
}

/*
d_counter_atomic_delta
  The plain counter's delta, of a snapshot.
*/
int64_t
d_counter_atomic_delta(
    const struct dj_counter_atomic* _counter
)
{
    struct dj_counter snapshot = D_COUNTER_INIT;

    // NULL has no movement
    if (!d_counter_atomic_snapshot(_counter,
                                   &snapshot))
    {
        return 0;
    }

    return d_counter_delta(&snapshot);
}

/*
d_counter_atomic_at_min
  The plain counter's test, of a snapshot.
*/
bool
d_counter_atomic_at_min(
    const struct dj_counter_atomic* _counter
)
{
    struct dj_counter snapshot = D_COUNTER_INIT;

    return ( (d_counter_atomic_snapshot(_counter,
                                        &snapshot)) &&
             (d_counter_at_min(&snapshot)) );
}

/*
d_counter_atomic_at_max
  The plain counter's test, of a snapshot.
*/
bool
d_counter_atomic_at_max(
    const struct dj_counter_atomic* _counter
)
{
    struct dj_counter snapshot = D_COUNTER_INIT;

    return ( (d_counter_atomic_snapshot(_counter,
                                        &snapshot)) &&
             (d_counter_at_max(&snapshot)) );
}

/*
d_counter_atomic_snapshot
  The value is the one atomic field; the start and the bounds never change
after init, so copying them needs no synchronization.
*/
bool
d_counter_atomic_snapshot(
    const struct dj_counter_atomic* _counter,
    struct dj_counter*              _out
)
{
    // parameter validation
    if ( (!_counter) ||
         (!_out) )
    {
        return false;
    }

    _out->initial    = _counter->initial;
    _out->value      = (int64_t)d_atomic_load_llong(&_counter->value);
    _out->bounds     = _counter->bounds;
    _out->has_bounds = _counter->has_bounds;

    return true;
}

// the atomic kind's table: this file's functions over an untyped pointer

/**
 * @brief The atomic kind's increment slot.
 *
 * @param[in,out] _counter  a struct dj_counter_atomic.
 * @param[in]     _amount   as d_counter_atomic_increment's.
 * @return as d_counter_atomic_increment.
 */
static bool
d_internal_counter_atomic_slot_increment(
    void*   _counter,
    int64_t _amount
)
{
    return d_counter_atomic_increment((struct dj_counter_atomic*)_counter,
                                      _amount);
}

/**
 * @brief The atomic kind's decrement slot.
 *
 * @param[in,out] _counter  a struct dj_counter_atomic.
 * @param[in]     _amount   as d_counter_atomic_decrement's.
 * @return as d_counter_atomic_decrement.
 */
static bool
d_internal_counter_atomic_slot_decrement(
    void*   _counter,
    int64_t _amount
)
{
    return d_counter_atomic_decrement((struct dj_counter_atomic*)_counter,
                                      _amount);
}

/**
 * @brief The atomic kind's merge slot.
 *
 * @param[in,out] _destination  a struct dj_counter_atomic.
 * @param[in]     _source       the counter whose movement is added.
 * @return as d_counter_atomic_merge_counter.
 */
static bool
d_internal_counter_atomic_slot_merge(
    void*                    _destination,
    const struct dj_counter* _source
)
{
    struct dj_counter_atomic* destination =
        (struct dj_counter_atomic*)_destination;

    return d_counter_atomic_merge_counter(destination,
                                          _source);
}

/**
 * @brief The atomic kind's reset slot.
 *
 * @param[in,out] _counter  a struct dj_counter_atomic.
 */
static void
d_internal_counter_atomic_slot_reset(
    void* _counter
)
{
    d_counter_atomic_reset((struct dj_counter_atomic*)_counter);

    return;
}

/**
 * @brief The atomic kind's value slot.
 *
 * @param[in] _counter  a struct dj_counter_atomic.
 * @return as d_counter_atomic_value.
 */
static int64_t
d_internal_counter_atomic_slot_value(
    const void* _counter
)
{
    return d_counter_atomic_value((const struct dj_counter_atomic*)_counter);
}

/**
 * @brief The atomic kind's snapshot slot.
 *
 * @param[in]  _counter  a struct dj_counter_atomic.
 * @param[out] _out      the copy.
 * @return as d_counter_atomic_snapshot.
 */
static bool
d_internal_counter_atomic_slot_snapshot(
    const void*        _counter,
    struct dj_counter* _out
)
{
    return d_counter_atomic_snapshot((const struct dj_counter_atomic*)_counter,
                                     _out);
}

// d_internal_counter_atomic_vtable
//   constant: the atomic kind's operations.
static const struct dj_counter_vtable d_internal_counter_atomic_vtable =
{
    d_internal_counter_atomic_slot_increment,
    d_internal_counter_atomic_slot_decrement,
    d_internal_counter_atomic_slot_merge,
    d_internal_counter_atomic_slot_reset,
    d_internal_counter_atomic_slot_value,
    d_internal_counter_atomic_slot_snapshot
};

/*
d_counter_atomic_ref
  Every slot is one of this file's lock-free functions, so a reference
shares the counter as safely as the functions do.
*/
struct dj_counter_ref
d_counter_atomic_ref(
    struct dj_counter_atomic* _counter
)
{
    struct dj_counter_ref ref = D_COUNTER_REF_NONE;

    // a counter to refer to
    if (_counter)
    {
        ref.vtable  = &d_internal_counter_atomic_vtable;
        ref.counter = _counter;
    }

    return ref;
}

#endif  // floor: D_ENV_HAS_LONG_LONG
