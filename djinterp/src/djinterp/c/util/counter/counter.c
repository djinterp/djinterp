/*******************************************************************************
* djinterp [c][utility]                                                counter.c
*
* The generic counter's arithmetic. Every step goes through
* d_counter_bounds_add, so the int64_t range and the bounds are checked in one
* place, and before the add, since signed overflow is undefined in C. The
* bounds are math's intervals, read through their inclusive ends.
*
*
* path:      /src/djinterp/c/util/counter/counter.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.09
*                                                            revised: 2026.11.10
*******************************************************************************/
#include "../../../../../inc/djinterp/c/util/counter/counter.h"  // corresponding header


/**
 * @brief Whether an interval can bound a counter.
 *
 * @param[in] _bounds  the interval; not `NULL`.
 * @return `true` when it is valid, continuous and holds at least one integer.
 */
static bool
d_internal_counter_bounds_usable(
    const struct d_interval_imax* _bounds
)
{
    return ( (d_interval_imax_is_valid(*_bounds))     &&
             (!d_interval_imax_is_discrete(*_bounds)) &&
             (!d_interval_imax_is_empty(*_bounds)) );
}

/**
 * @brief A counter's bounds, for the arithmetic.
 *
 * @param[in] _counter  the counter; not `NULL`.
 * @return its bounds, or `NULL` when it has none.
 */
static const struct d_interval_imax*
d_internal_counter_bounds(
    const struct dj_counter* _counter
)
{
    return (_counter->has_bounds) ? &_counter->bounds : NULL;
}

/*
d_counter_bounds_add
  The int64_t range is held first, by comparing against the room left before
adding, then the bounds, taken inclusively. A bound beyond the int64_t range
is never reached by the first test's result, so the cast back is exact.
*/
int64_t
d_counter_bounds_add(
    const struct d_interval_imax* _bounds,
    int64_t                       _value,
    int64_t                       _amount,
    bool*                         _is_exact
)
{
    bool    is_cut = false;
    int64_t sum    = 0;

    // hold the sum inside int64_t, checking before adding
    if ( (_amount > 0) &&
         (_value > (INT64_MAX - _amount)) )
    {
        sum    = INT64_MAX;
        is_cut = true;
    }
    else if ( (_amount < 0) &&
              (_value < (INT64_MIN - _amount)) )
    {
        sum    = INT64_MIN;
        is_cut = true;
    }
    else
    {
        sum = _value + _amount;
    }

    // then inside the bounds
    if (_bounds)
    {
        const d_math_imax upper = d_interval_imax_inclusive_upper(*_bounds);
        const d_math_imax lower = d_interval_imax_inclusive_lower(*_bounds);

        // above the top
        if (sum > upper)
        {
            sum    = (int64_t)upper;
            is_cut = true;
        }

        // below the bottom
        if (sum < lower)
        {
            sum    = (int64_t)lower;
            is_cut = true;
        }
    }

    // report whether anything was cut
    if (_is_exact)
    {
        *_is_exact = !is_cut;
    }

    return sum;
}

/*
d_counter_bounds_sub
  Every amount but INT64_MIN negates and goes through d_counter_bounds_add.
INT64_MIN is a step up by INT64_MAX and then by one: the bounds apply to
each step, and either step's cut makes the whole move inexact.
*/
int64_t
d_counter_bounds_sub(
    const struct d_interval_imax* _bounds,
    int64_t                       _value,
    int64_t                       _amount,
    bool*                         _is_exact
)
{
    // every amount but one has a negation
    if (_amount != INT64_MIN)
    {
        return d_counter_bounds_add(_bounds,
                                    _value,
                                    -_amount,
                                    _is_exact);
    }

    bool          is_first_exact  = true;
    bool          is_second_exact = true;
    const int64_t middle          = d_counter_bounds_add(_bounds,
                                                         _value,
                                                         INT64_MAX,
                                                         &is_first_exact);
    const int64_t result          = d_counter_bounds_add(_bounds,
                                                         middle,
                                                         1,
                                                         &is_second_exact);

    // report the two steps as one
    if (_is_exact)
    {
        *_is_exact = ( (is_first_exact) &&
                       (is_second_exact) );
    }

    return result;
}

/*
d_counter_init
  The same counter D_COUNTER_INIT gives, started at `_initial`.
*/
void
d_counter_init(
    struct dj_counter* _counter,
    int64_t            _initial
)
{
    // parameter validation
    if (!_counter)
    {
        return;
    }

    const struct dj_counter none = D_COUNTER_INIT;

    *_counter         = none;
    _counter->initial = _initial;
    _counter->value   = _initial;

    return;
}

/*
d_counter_init_bounded
  Everything is checked before anything is written, so a refused call leaves
the counter as it was. Membership is the interval's own, open ends included.
*/
bool
d_counter_init_bounded(
    struct dj_counter*            _counter,
    int64_t                       _initial,
    const struct d_interval_imax* _bounds
)
{
    // parameter validation
    if (!_counter)
    {
        return false;
    }

    // no bounds is d_counter_init
    if (!_bounds)
    {
        d_counter_init(_counter,
                       _initial);

        return true;
    }

    // the bounds must be usable, and the start must lie within them
    if ( (!d_internal_counter_bounds_usable(_bounds)) ||
         (!d_interval_imax_contains_in_range(*_bounds,
                                             (d_math_imax)_initial)) )
    {
        return false;
    }

    _counter->initial    = _initial;
    _counter->value      = _initial;
    _counter->bounds     = *_bounds;
    _counter->has_bounds = true;

    return true;
}

/*
d_counter_increment
  One saturating step through the shared arithmetic.
*/
bool
d_counter_increment(
    struct dj_counter* _counter,
    int64_t            _amount
)
{
    // parameter validation
    if (!_counter)
    {
        return false;
    }

    bool is_exact = true;

    _counter->value = d_counter_bounds_add(d_internal_counter_bounds(_counter),
                                           _counter->value,
                                           _amount,
                                           &is_exact);

    return is_exact;
}

/*
d_counter_decrement
  The mirror of increment, through d_counter_bounds_sub, which handles
INT64_MIN.
*/
bool
d_counter_decrement(
    struct dj_counter* _counter,
    int64_t            _amount
)
{
    // parameter validation
    if (!_counter)
    {
        return false;
    }

    bool is_exact = true;

    _counter->value = d_counter_bounds_sub(d_internal_counter_bounds(_counter),
                                           _counter->value,
                                           _amount,
                                           &is_exact);

    return is_exact;
}

/*
d_counter_compare
  Three-way, by value alone: two counters at the same value compare equal
whatever their starts and bounds. NULL sorts first, so a sort over pointers
needs no guard.
*/
int
d_counter_compare(
    const struct dj_counter* _a,
    const struct dj_counter* _b
)
{
    // NULL before any counter, and equal to itself
    if ( (!_a) ||
         (!_b) )
    {
        return (_a == _b) ? 0 : ( (!_a) ? -1 : 1 );
    }

    // compared, not subtracted: the difference could overflow
    if (_a->value < _b->value)
    {
        return -1;
    }

    return (_a->value > _b->value) ? 1 : 0;
}

/*
d_counter_merge
  Adds the source's movement since its start, which is what makes merge
compose: tallies kept apart and merged equal one tally kept throughout,
whatever each started from. A delta that itself saturated makes the merge
inexact even when the add fits.
*/
bool
d_counter_merge(
    struct dj_counter*       _destination,
    const struct dj_counter* _source
)
{
    // parameter validation
    if ( (!_destination) ||
         (!_source) )
    {
        return false;
    }

    bool          is_delta_exact = true;
    bool          is_add_exact   = true;
    const int64_t delta          = d_counter_bounds_sub(NULL,
                                                        _source->value,
                                                        _source->initial,
                                                        &is_delta_exact);

    _destination->value =
        d_counter_bounds_add(d_internal_counter_bounds(_destination),
                             _destination->value,
                             delta,
                             &is_add_exact);

    return ( (is_delta_exact) &&
             (is_add_exact) );
}

/*
d_counter_reset
  Back to the start; the bounds stay.
*/
void
d_counter_reset(
    struct dj_counter* _counter
)
{
    // parameter validation
    if (!_counter)
    {
        return;
    }

    _counter->value = _counter->initial;

    return;
}

/*
d_counter_value
  The count, or 0 for NULL.
*/
int64_t
d_counter_value(
    const struct dj_counter* _counter
)
{
    return (_counter) ? _counter->value : 0;
}

/*
d_counter_delta
  value - initial, saturating: the movement merge adds.
*/
int64_t
d_counter_delta(
    const struct dj_counter* _counter
)
{
    // parameter validation
    if (!_counter)
    {
        return 0;
    }

    return d_counter_bounds_sub(NULL,
                                _counter->value,
                                _counter->initial,
                                NULL);
}

/*
d_counter_at_min
  Only a bounded counter has a bottom to reach; an unlimited lower side is
d_math_imax's minimum, which a counter reaches only if int64_t does.
*/
bool
d_counter_at_min(
    const struct dj_counter* _counter
)
{
    return ( (_counter)             &&
             (_counter->has_bounds) &&
             (_counter->value <=
                  d_interval_imax_inclusive_lower(_counter->bounds)) );
}

/*
d_counter_at_max
  Only a bounded counter has a top to reach.
*/
bool
d_counter_at_max(
    const struct dj_counter* _counter
)
{
    return ( (_counter)             &&
             (_counter->has_bounds) &&
             (_counter->value >=
                  d_interval_imax_inclusive_upper(_counter->bounds)) );
}

// the plain kind's table: section 2's functions over an untyped pointer

/**
 * @brief The plain kind's increment slot.
 *
 * @param[in,out] _counter  a struct dj_counter.
 * @param[in]     _amount   as d_counter_increment's.
 * @return as d_counter_increment.
 */
static bool
d_internal_counter_slot_increment(
    void*   _counter,
    int64_t _amount
)
{
    return d_counter_increment((struct dj_counter*)_counter,
                               _amount);
}

/**
 * @brief The plain kind's decrement slot.
 *
 * @param[in,out] _counter  a struct dj_counter.
 * @param[in]     _amount   as d_counter_decrement's.
 * @return as d_counter_decrement.
 */
static bool
d_internal_counter_slot_decrement(
    void*   _counter,
    int64_t _amount
)
{
    return d_counter_decrement((struct dj_counter*)_counter,
                               _amount);
}

/**
 * @brief The plain kind's merge slot.
 *
 * @param[in,out] _destination  a struct dj_counter.
 * @param[in]     _source       the counter whose movement is added.
 * @return as d_counter_merge.
 */
static bool
d_internal_counter_slot_merge(
    void*                    _destination,
    const struct dj_counter* _source
)
{
    return d_counter_merge((struct dj_counter*)_destination,
                           _source);
}

/**
 * @brief The plain kind's reset slot.
 *
 * @param[in,out] _counter  a struct dj_counter.
 */
static void
d_internal_counter_slot_reset(
    void* _counter
)
{
    d_counter_reset((struct dj_counter*)_counter);

    return;
}

/**
 * @brief The plain kind's value slot.
 *
 * @param[in] _counter  a struct dj_counter.
 * @return as d_counter_value.
 */
static int64_t
d_internal_counter_slot_value(
    const void* _counter
)
{
    return d_counter_value((const struct dj_counter*)_counter);
}

/**
 * @brief The plain kind's snapshot slot: a copy.
 *
 * @param[in]  _counter  a struct dj_counter.
 * @param[out] _out      the copy.
 * @return `true` when copied; `false` when either argument is `NULL`.
 */
static bool
d_internal_counter_slot_snapshot(
    const void*        _counter,
    struct dj_counter* _out
)
{
    // parameter validation
    if ( (!_counter) ||
         (!_out) )
    {
        return false;
    }

    *_out = *(const struct dj_counter*)_counter;

    return true;
}

// d_internal_counter_vtable
//   constant: the plain kind's operations.
static const struct dj_counter_vtable d_internal_counter_vtable =
{
    d_internal_counter_slot_increment,
    d_internal_counter_slot_decrement,
    d_internal_counter_slot_merge,
    d_internal_counter_slot_reset,
    d_internal_counter_slot_value,
    d_internal_counter_slot_snapshot
};

/*
d_counter_ref
  A NULL counter makes the reference to no counter, so the table is never
paired with nothing.
*/
struct dj_counter_ref
d_counter_ref(
    struct dj_counter* _counter
)
{
    struct dj_counter_ref ref = D_COUNTER_REF_NONE;

    // a counter to refer to
    if (_counter)
    {
        ref.vtable  = &d_internal_counter_vtable;
        ref.counter = _counter;
    }

    return ref;
}

/*
d_counter_ref_is_valid
  Both halves, since a table without a counter would pass NULL to a slot.
*/
bool
d_counter_ref_is_valid(
    struct dj_counter_ref _ref
)
{
    return ( (_ref.vtable  != NULL) &&
             (_ref.counter != NULL) );
}

/*
d_counter_ref_increment
  The kind's slot, or failure for no counter.
*/
bool
d_counter_ref_increment(
    struct dj_counter_ref _ref,
    int64_t               _amount
)
{
    return (d_counter_ref_is_valid(_ref))
               ? _ref.vtable->increment(_ref.counter,
                                        _amount)
               : false;
}

/*
d_counter_ref_decrement
  The kind's slot, or failure for no counter.
*/
bool
d_counter_ref_decrement(
    struct dj_counter_ref _ref,
    int64_t               _amount
)
{
    return (d_counter_ref_is_valid(_ref))
               ? _ref.vtable->decrement(_ref.counter,
                                        _amount)
               : false;
}

/*
d_counter_ref_compare
  By value, as d_counter_compare, with no counter in NULL's place.
*/
int
d_counter_ref_compare(
    struct dj_counter_ref _a,
    struct dj_counter_ref _b
)
{
    const bool is_a = d_counter_ref_is_valid(_a);
    const bool is_b = d_counter_ref_is_valid(_b);

    // no counter orders first, and equal to itself
    if ( (!is_a) ||
         (!is_b) )
    {
        return (is_a == is_b) ? 0 : ( (is_a) ? 1 : -1 );
    }

    const int64_t a = _a.vtable->value(_a.counter);
    const int64_t b = _b.vtable->value(_b.counter);

    return (a < b) ? -1 : ( (a > b) ? 1 : 0 );
}

/*
d_counter_ref_merge
  The source is read once, through its snapshot, before the destination is
touched.
*/
bool
d_counter_ref_merge(
    struct dj_counter_ref _destination,
    struct dj_counter_ref _source
)
{
    struct dj_counter source = D_COUNTER_INIT;

    // both counters, and the source readable
    if ( (!d_counter_ref_is_valid(_destination)) ||
         (!d_counter_ref_snapshot(_source,
                                  &source)) )
    {
        return false;
    }

    return _destination.vtable->merge(_destination.counter,
                                      &source);
}

/*
d_counter_ref_reset
  The kind's slot; nothing for no counter.
*/
void
d_counter_ref_reset(
    struct dj_counter_ref _ref
)
{
    // a counter to reset
    if (d_counter_ref_is_valid(_ref))
    {
        _ref.vtable->reset(_ref.counter);
    }

    return;
}

/*
d_counter_ref_value
  The kind's slot, or 0 for no counter.
*/
int64_t
d_counter_ref_value(
    struct dj_counter_ref _ref
)
{
    return (d_counter_ref_is_valid(_ref))
               ? _ref.vtable->value(_ref.counter)
               : 0;
}

/*
d_counter_ref_snapshot
  The kind's slot, which owns the reading: a lock, an atomic load, or a copy.
*/
bool
d_counter_ref_snapshot(
    struct dj_counter_ref _ref,
    struct dj_counter*    _out
)
{
    // parameter validation
    if ( (!d_counter_ref_is_valid(_ref)) ||
         (!_out) )
    {
        return false;
    }

    return _ref.vtable->snapshot(_ref.counter,
                                 _out);
}
