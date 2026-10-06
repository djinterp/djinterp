/*******************************************************************************
* djinterp [c]                                                        interval.h
*
* Integer intervals: the C core of math/interval.
*   An interval is a lower and an upper bound, each closed or open, and a
* stride: 0 for a continuous interval, which holds every integer between its
* bounds, and a positive step for a discrete one, which holds the bounds'
* first member and every step after it. Every operation of the C++ interval
* templates (closed_interval, open_interval, discrete_interval, interval) is
* one of the kernels below; the templates forward to them.
*
*   TWO FAMILIES. C has no templates, so each kernel is written twice, once
* over the widest signed integer the build can spell (d_math_imax, the _imax
* family) and once over the widest unsigned (d_math_umax, the _umax family).
* An interval over any narrower integer type is the same interval in its
* family, reached by a conversion that loses nothing; its results convert
* back the same way.
*
*   OVERFLOW. No kernel overflows for any interval its family can hold. A
* difference of two bounds is taken as an unsigned magnitude, which is exact
* for any two values of the family; a count that exceeds d_math_umax (the
* full range of the unsigned family has one more member than it can count)
* saturates at D_MATH_UMAX_MAX.
*
*   EMPTY INTERVALS. An open or half-open interval can hold no integer: (2, 3)
* and [4, 4) are empty. count() is then 0 and contains() false. first(),
* last(), clamp() and at() still return a value, defined as if the inclusive
* bounds were taken as signed numbers -- (2, 3) clamps every value to 3 or 2
* -- so a caller that cares tests is_empty() first.
*
*
* path:      /inc/djinterp/c/math/interval.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.04
*                                                            revised: 2026.10.04
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  BOUNDS AND LAYOUT
    -----------------
    1.  Bound flags
         1.  d_interval_bound
    2.  Layout
         1.  d_interval_imax
         2.  d_interval_umax
2.  THE SIGNED FAMILY
    -----------------
    1.  Internal helpers
    2.  Construction and validity
    3.  Inclusive bounds and size
    4.  Membership
    5.  Clamping
    6.  Discrete access
    7.  Relations
    8.  Normalization
3.  THE UNSIGNED FAMILY
    -------------------
    1.  Internal helpers
    2.  Construction and validity
    3.  Inclusive bounds and size
    4.  Membership
    5.  Clamping
    6.  Discrete access
    7.  Relations
    8.  Normalization
4.  FORMATTING
    ----------
    1.  Internal helpers
    2.  d_interval_imax_format, d_interval_umax_format
*/

#ifndef DJINTERP_C_MATH_INTERVAL_H
#define DJINTERP_C_MATH_INTERVAL_H 1

// std
#include <stddef.h>          // size_t, NULL
// djinterp
#include "./math_common.h"   // D_MATH_FN, d_math_imax, d_math_umax


//==============================================================================
// 1.  BOUNDS AND LAYOUT
//==============================================================================


// 1.1    Bound flags
//------------------------------------------------------------------------------
// 1.1.1
// d_interval_bound
//   enum: which endpoints an interval excludes. The values are bits: an
// interval's `bounds` member is D_INTERVAL_CLOSED, either flag, or both.
enum d_interval_bound
{
    D_INTERVAL_CLOSED     = 0,
    D_INTERVAL_LEFT_OPEN  = 1,
    D_INTERVAL_RIGHT_OPEN = 2,
    D_INTERVAL_OPEN       = 3
};

// 1.2    Layout
//------------------------------------------------------------------------------
// 1.2.1
// d_interval_imax
//   struct: an interval over the widest signed integer the build can spell.
struct d_interval_imax
{
    d_math_imax lower;   // the lower bound
    d_math_imax upper;   // the upper bound
    d_math_imax step;    // the stride: 0 continuous, > 0 discrete
    unsigned    bounds;  // D_INTERVAL_LEFT_OPEN | D_INTERVAL_RIGHT_OPEN
};

// 1.2.2
// d_interval_umax
//   struct: an interval over the widest unsigned integer the build can spell.
struct d_interval_umax
{
    d_math_umax lower;   // the lower bound
    d_math_umax upper;   // the upper bound
    d_math_umax step;    // the stride: 0 continuous, > 0 discrete
    unsigned    bounds;  // D_INTERVAL_LEFT_OPEN | D_INTERVAL_RIGHT_OPEN
};


//==============================================================================
// 2.  THE SIGNED FAMILY
//==============================================================================
// Every kernel takes the interval by value: it is four words, and by value
// the same text is a constant expression from C++14.


// 2.1    Internal helpers
//------------------------------------------------------------------------------

// d_internal_interval_gcd
//   the greatest common divisor of two positive integers (both families).
D_MATH_FN d_math_umax
d_internal_interval_gcd(
    d_math_umax _a,
    d_math_umax _b
)
{
    d_math_umax a = _a;
    d_math_umax b = _b;

    // Euclid: the remainder replaces the larger until it vanishes
    while (b != 0u)
    {
        const d_math_umax rest = a % b;

        a = b;
        b = rest;
    }

    return a;
}

// d_internal_interval_mulmod
//   (_a * _b) mod _m without overflowing d_math_umax, _m > 0: doubling and
// adding, each step reduced, so no intermediate exceeds _m twice.
D_MATH_FN d_math_umax
d_internal_interval_mulmod(
    d_math_umax _a,
    d_math_umax _b,
    d_math_umax _m
)
{
    d_math_umax result = 0u;
    d_math_umax a      = _a % _m;
    d_math_umax b      = _b;

    // a * b = sum of a * 2^i over b's bits
    while (b != 0u)
    {
        if ((b & 1u) != 0u)
        {
            result = (result >= (_m - a)) ? (result - (_m - a))
                                          : (result + a);
        }

        a  = (a >= (_m - a)) ? (a - (_m - a)) : (a + a);
        b >>= 1;
    }

    return result;
}

// d_internal_interval_inverse
//   the inverse of _a modulo _m, for _a and _m coprime and _m > 0: extended
// Euclid with its coefficients kept modulo _m, so none is negative.
D_MATH_FN d_math_umax
d_internal_interval_inverse(
    d_math_umax _a,
    d_math_umax _m
)
{
    d_math_umax r0 = _m;
    d_math_umax r1 = _a % _m;
    d_math_umax t0 = 0u;
    d_math_umax t1 = 1u;

    // modulo 1 everything is 0
    if (_m == 1u)
    {
        return 0u;
    }

    // r0 = t0 * _a and r1 = t1 * _a, modulo _m, throughout
    while (r1 != 0u)
    {
        const d_math_umax q  = r0 / r1;
        const d_math_umax r2 = r0 - (q * r1);
        const d_math_umax qt = d_internal_interval_mulmod(q, t1, _m);
        const d_math_umax t2 = (t0 >= qt) ? (t0 - qt) : (t0 + (_m - qt));

        r0 = r1;
        r1 = r2;
        t0 = t1;
        t1 = t2;
    }

    return t0;
}

// d_internal_interval_lattice_meet
//   whether the lattices {_a0 + i * _a_step} up to _a_last and {_b0 + j *
// _b_step} up to _b_last share a value: the two congruences solved by the
// Chinese remainder theorem, then the least solution lifted into both
// ranges. Values are offsets from a common origin; steps are positive.
// Every product is checked before it is formed.
D_MATH_FN bool
d_internal_interval_lattice_meet(
    d_math_umax _a0,
    d_math_umax _a_step,
    d_math_umax _a_last,
    d_math_umax _b0,
    d_math_umax _b_step,
    d_math_umax _b_last
)
{
    const d_math_umax low  = (_a0 > _b0) ? _a0 : _b0;
    const d_math_umax high = (_a_last < _b_last) ? _a_last : _b_last;

    // the ranges must meet at all
    if (low > high)
    {
        return false;
    }

    const d_math_umax g = d_internal_interval_gcd(_a_step, _b_step);

    // the starts must agree modulo the steps' common divisor
    if ((_a0 % g) != (_b0 % g))
    {
        return false;
    }

    // x = _a0 + _a_step * k, with _a_step * k = _b0 - _a0 modulo _b_step
    const d_math_umax period = _b_step / g;
    const d_math_umax a_mod  = _a0 % _b_step;
    const d_math_umax b_mod  = _b0 % _b_step;
    const d_math_umax gap    = (b_mod >= a_mod) ? (b_mod - a_mod)
                                                : (_b_step - (a_mod - b_mod));
    const d_math_umax k      =
        d_internal_interval_mulmod(gap / g,
                                   d_internal_interval_inverse(
                                       (_a_step / g) % period,
                                       period),
                                   period);

    // the least common value at or above _a0; one past the family is none
    if ( (k != 0u) &&
         (_a_step > ((D_MATH_UMAX_MAX - _a0) / k)) )
    {
        return false;
    }

    const d_math_umax first = _a0 + (_a_step * k);

    // already inside both ranges, or past them
    if (first >= low)
    {
        return (first <= high);
    }

    // the common lattice's period, lcm(steps); wider than the family, the
    // next common value lies past every value
    if ((_a_step / g) > (D_MATH_UMAX_MAX / _b_step))
    {
        return false;
    }

    const d_math_umax lcm    = (_a_step / g) * _b_step;
    const d_math_umax behind = low - first;
    const d_math_umax lifts  = (behind / lcm) +
                               (((behind % lcm) != 0u) ? 1u : 0u);

    // lifted into the ranges, unless that passes the family's end
    if (lifts > ((D_MATH_UMAX_MAX - first) / lcm))
    {
        return false;
    }

    return ((first + (lifts * lcm)) <= high);
}

// d_internal_interval_imax_snap
//   the member reached from _from toward _to by whole steps: _from plus
// (_to - _from) / _step steps, the quotient truncated toward zero, as signed
// arithmetic computes it. _step must be positive.
D_MATH_FN d_math_imax
d_internal_interval_imax_snap(
    d_math_imax _from,
    d_math_imax _to,
    d_math_umax _step
)
{
    // the magnitude of the gap is exact in the unsigned family either way
    if (_to >= _from)
    {
        const d_math_umax up = ( ((d_math_umax)_to - (d_math_umax)_from) /
                                 _step );

        return (d_math_imax)((d_math_umax)_from + (up * _step));
    }

    const d_math_umax down = ( ((d_math_umax)_from - (d_math_umax)_to) /
                               _step );

    return (d_math_imax)((d_math_umax)_from - (down * _step));
}

// d_internal_interval_imax_terms
//   (_value - _origin) and (_end - _origin) as long doubles, each difference
// exact before its one rounding; {0, 1} when _end == _origin.
D_MATH_FN struct d_math_ratio
d_internal_interval_imax_terms(
    d_math_imax _origin,
    d_math_imax _end,
    d_math_imax _value
)
{
    struct d_math_ratio result = { 0.0L, 1.0L };

    // a degenerate span maps everything to 0
    if (_end == _origin)
    {
        return result;
    }

    result.numerator =
        (_value >= _origin)
            ?  (long double)((d_math_umax)_value  - (d_math_umax)_origin)
            : -(long double)((d_math_umax)_origin - (d_math_umax)_value);
    result.denominator =
        (_end >= _origin)
            ?  (long double)((d_math_umax)_end    - (d_math_umax)_origin)
            : -(long double)((d_math_umax)_origin - (d_math_umax)_end);

    return result;
}

// d_internal_interval_imax_is_void
//   true when the interval is empty on the real line: reversed bounds, or one
// point with either bound open.
D_MATH_FN bool
d_internal_interval_imax_is_void(
    struct d_interval_imax _interval
)
{
    return ( (_interval.lower > _interval.upper) ||
             ( (_interval.lower == _interval.upper) &&
               (_interval.bounds != D_INTERVAL_CLOSED) ) );
}

// 2.2    Construction and validity
//------------------------------------------------------------------------------
// make assembles an interval from its parts, checking nothing; is_valid is
// true when lower <= upper and the step is not negative.

D_MATH_FN struct d_interval_imax
d_interval_imax_make(
    d_math_imax _lower,
    d_math_imax _upper,
    unsigned    _bounds,
    d_math_imax _step
)
{
    struct d_interval_imax result = { 0, 0, 0, 0u };

    result.lower  = _lower;
    result.upper  = _upper;
    result.step   = _step;
    result.bounds = _bounds;

    return result;
}

D_MATH_FN bool
d_interval_imax_is_valid(
    struct d_interval_imax _interval
)
{
    return ( (_interval.lower <= _interval.upper) &&
             (_interval.step >= 0) );
}

D_MATH_FN bool
d_interval_imax_is_discrete(
    struct d_interval_imax _interval
)
{
    return (_interval.step > 0);
}

// 2.3    Inclusive bounds and size
//------------------------------------------------------------------------------
// inclusive_lower and inclusive_upper are the bounds as closed ones -- an
// open bound moved one integer inward -- and saturate rather than overflow.
// first is the smallest member and last the largest, which for a discrete
// interval is the last whole step at or below inclusive_upper. count
// saturates at D_MATH_UMAX_MAX.

D_MATH_FN d_math_imax
d_interval_imax_inclusive_lower(
    struct d_interval_imax _interval
)
{
    // an open lower bound excludes itself; at the family's maximum the
    // interval is empty, and the bound stays where it is
    if ( ((_interval.bounds & D_INTERVAL_LEFT_OPEN) != 0u) &&
         (_interval.lower < D_MATH_IMAX_MAX) )
    {
        return (d_math_imax)(_interval.lower + 1);
    }

    return _interval.lower;
}

D_MATH_FN d_math_imax
d_interval_imax_inclusive_upper(
    struct d_interval_imax _interval
)
{
    // an open upper bound excludes itself; at the family's minimum the
    // interval is empty, and the bound stays where it is
    if ( ((_interval.bounds & D_INTERVAL_RIGHT_OPEN) != 0u) &&
         (_interval.upper > D_MATH_IMAX_MIN) )
    {
        return (d_math_imax)(_interval.upper - 1);
    }

    return _interval.upper;
}

D_MATH_FN bool
d_interval_imax_is_empty(
    struct d_interval_imax _interval
)
{
    // reversed bounds hold nothing
    if (_interval.lower > _interval.upper)
    {
        return true;
    }

    const d_math_umax span  = ( (d_math_umax)_interval.upper -
                                (d_math_umax)_interval.lower );
    const d_math_umax opens =
        ( (((_interval.bounds & D_INTERVAL_LEFT_OPEN)  != 0u) ? 1u : 0u) +
          (((_interval.bounds & D_INTERVAL_RIGHT_OPEN) != 0u) ? 1u : 0u) );

    return (span < opens);
}

D_MATH_FN d_math_imax
d_interval_imax_first(
    struct d_interval_imax _interval
)
{
    return d_interval_imax_inclusive_lower(_interval);
}

D_MATH_FN d_math_imax
d_interval_imax_last(
    struct d_interval_imax _interval
)
{
    // a discrete interval ends on its last whole step
    if (_interval.step > 0)
    {
        return d_internal_interval_imax_snap(
                   d_interval_imax_inclusive_lower(_interval),
                   d_interval_imax_inclusive_upper(_interval),
                   (d_math_umax)_interval.step);
    }

    return d_interval_imax_inclusive_upper(_interval);
}

D_MATH_FN d_math_umax
d_interval_imax_count(
    struct d_interval_imax _interval
)
{
    // an empty interval counts nothing
    if (d_interval_imax_is_empty(_interval))
    {
        return 0u;
    }

    const d_math_umax span =
        ( (d_math_umax)d_interval_imax_inclusive_upper(_interval) -
          (d_math_umax)d_interval_imax_inclusive_lower(_interval) );
    const d_math_umax steps =
        (_interval.step > 0)
            ? (span / (d_math_umax)_interval.step)
            : span;

    return (steps == D_MATH_UMAX_MAX) ? steps : (steps + 1u);
}

// 2.4    Membership
//------------------------------------------------------------------------------
// contains honours both bounds and, for a discrete interval, the stride from
// the first member; contains_in_range honours the bounds alone.

D_MATH_FN bool
d_interval_imax_contains_in_range(
    struct d_interval_imax _interval,
    d_math_imax            _value
)
{
    const bool left_ok =
        ((_interval.bounds & D_INTERVAL_LEFT_OPEN) != 0u)
            ? (_value > _interval.lower)
            : (_value >= _interval.lower);
    const bool right_ok =
        ((_interval.bounds & D_INTERVAL_RIGHT_OPEN) != 0u)
            ? (_value < _interval.upper)
            : (_value <= _interval.upper);

    return (left_ok && right_ok);
}

D_MATH_FN bool
d_interval_imax_contains(
    struct d_interval_imax _interval,
    d_math_imax            _value
)
{
    // outside the bounds is outside the interval
    if (!d_interval_imax_contains_in_range(_interval, _value))
    {
        return false;
    }

    // a discrete member is a whole number of steps past the first
    if (_interval.step > 0)
    {
        const d_math_umax offset =
            ( (d_math_umax)_value -
              (d_math_umax)d_interval_imax_inclusive_lower(_interval) );

        return ((offset % (d_math_umax)_interval.step) == 0u);
    }

    return true;
}

// 2.5    Clamping
//------------------------------------------------------------------------------
// clamp moves a value to the nearest bound and, for a discrete interval,
// down to a whole step; clamp_nearest rounds to whichever whole step is
// nearer, a tie going down.

D_MATH_FN d_math_imax
d_interval_imax_clamp(
    struct d_interval_imax _interval,
    d_math_imax            _value
)
{
    const d_math_imax low  = d_interval_imax_inclusive_lower(_interval);
    const d_math_imax high = d_interval_imax_inclusive_upper(_interval);

    // below the interval: its first value
    if (_value < low)
    {
        return low;
    }

    // above the interval: its last value
    if (_value > high)
    {
        return d_interval_imax_last(_interval);
    }

    // inside a discrete interval: down to a whole step
    if (_interval.step > 0)
    {
        return d_internal_interval_imax_snap(low,
                                             _value,
                                             (d_math_umax)_interval.step);
    }

    return _value;
}

D_MATH_FN d_math_imax
d_interval_imax_clamp_nearest(
    struct d_interval_imax _interval,
    d_math_imax            _value
)
{
    // a continuous interval has no steps to round between
    if (_interval.step <= 0)
    {
        return d_interval_imax_clamp(_interval, _value);
    }

    const d_math_imax low = d_interval_imax_inclusive_lower(_interval);

    // below the interval: its first value
    if (_value < low)
    {
        return low;
    }

    const d_math_imax last = d_interval_imax_last(_interval);

    // above the last step: the last step
    if (_value > last)
    {
        return last;
    }

    const d_math_umax step  = (d_math_umax)_interval.step;
    const d_math_imax below = d_internal_interval_imax_snap(low,
                                                            _value,
                                                            step);
    const d_math_umax gap   = ((d_math_umax)_value - (d_math_umax)below);

    // the step above is nearer, and still inside the interval
    if ( ((d_math_umax)last - (d_math_umax)below >= step) &&
         (gap > (step - gap)) )
    {
        return (d_math_imax)((d_math_umax)below + step);
    }

    return below;
}

// 2.6    Discrete access
//------------------------------------------------------------------------------
// at is the member _index steps past the first (a continuous interval steps
// by 1); index_of is a member's index, or count() for a value that is not a
// member.

D_MATH_FN d_math_imax
d_interval_imax_at(
    struct d_interval_imax _interval,
    d_math_umax            _index
)
{
    const d_math_umax stride =
        (_interval.step > 0) ? (d_math_umax)_interval.step : 1u;

    return (d_math_imax)( (d_math_umax)d_interval_imax_inclusive_lower(
                                           _interval) +
                          (_index * stride) );
}

D_MATH_FN d_math_umax
d_interval_imax_index_of(
    struct d_interval_imax _interval,
    d_math_imax            _value
)
{
    // a value that is not a member has the index one past the last
    if (!d_interval_imax_contains(_interval, _value))
    {
        return d_interval_imax_count(_interval);
    }

    const d_math_umax offset =
        ( (d_math_umax)_value -
          (d_math_umax)d_interval_imax_inclusive_lower(_interval) );

    return (_interval.step > 0)
               ? (offset / (d_math_umax)_interval.step)
               : offset;
}

// 2.7    Relations
//------------------------------------------------------------------------------
// overlaps is true when neither interval is empty and their inclusive ranges
// meet; a discrete interval's stride is not considered. overlaps_continuous
// compares them as intervals of the real line: two bounds that meet overlap
// only if both include the point, and an interval empty there overlaps
// nothing. intersects is true when the intervals share a member: for two
// discrete intervals, a value on both strides, found by the Chinese
// remainder theorem in a few dozen operations however wide the intervals.

D_MATH_FN bool
d_interval_imax_overlaps(
    struct d_interval_imax _left,
    struct d_interval_imax _right
)
{
    // an empty interval shares a position with nothing
    if ( (d_interval_imax_is_empty(_left)) ||
         (d_interval_imax_is_empty(_right)) )
    {
        return false;
    }

    return !( (d_interval_imax_inclusive_upper(_left) <
               d_interval_imax_inclusive_lower(_right)) ||
              (d_interval_imax_inclusive_lower(_left) >
               d_interval_imax_inclusive_upper(_right)) );
}

D_MATH_FN bool
d_interval_imax_overlaps_continuous(
    struct d_interval_imax _left,
    struct d_interval_imax _right
)
{
    // an interval empty on the real line meets nothing
    if ( (d_internal_interval_imax_is_void(_left)) ||
         (d_internal_interval_imax_is_void(_right)) )
    {
        return false;
    }

    const bool left_meets_closed =
        ( ((_left.bounds  & D_INTERVAL_LEFT_OPEN)  == 0u) &&
          ((_right.bounds & D_INTERVAL_RIGHT_OPEN) == 0u) );
    const bool right_meets_closed =
        ( ((_left.bounds  & D_INTERVAL_RIGHT_OPEN) == 0u) &&
          ((_right.bounds & D_INTERVAL_LEFT_OPEN)  == 0u) );
    const bool left_ok =
        left_meets_closed ? (_left.lower <= _right.upper)
                          : (_left.lower <  _right.upper);
    const bool right_ok =
        right_meets_closed ? (_right.lower <= _left.upper)
                           : (_right.lower <  _left.upper);

    return (left_ok && right_ok);
}

D_MATH_FN bool
d_interval_imax_intersects(
    struct d_interval_imax _left,
    struct d_interval_imax _right
)
{
    // an empty interval has no member to share
    if ( (d_interval_imax_is_empty(_left)) ||
         (d_interval_imax_is_empty(_right)) )
    {
        return false;
    }

    const d_math_imax left_first  = d_interval_imax_first(_left);
    const d_math_imax right_first = d_interval_imax_first(_right);
    const d_math_imax origin      = (left_first < right_first) ? left_first
                                                             : right_first;
    const d_math_umax left_step  =
        (_left.step > 0) ? (d_math_umax)_left.step : 1u;
    const d_math_umax right_step =
        (_right.step > 0) ? (d_math_umax)_right.step : 1u;

    // the two lattices as offsets from the lesser first member
    return d_internal_interval_lattice_meet(
        (d_math_umax)left_first - (d_math_umax)origin,
        left_step,
        (d_math_umax)d_interval_imax_last(_left) - (d_math_umax)origin,
        (d_math_umax)right_first - (d_math_umax)origin,
        right_step,
        (d_math_umax)d_interval_imax_last(_right) - (d_math_umax)origin);
}

// 2.8    Normalization
//------------------------------------------------------------------------------
// the terms of a value's place in [0, 1] over a span, {0, 1} for a
// degenerate one: normalize_terms over [lower, upper], the _effective form over
// the inclusive bounds, the _discrete form over the step indices 0 to count()
// - 1 (the _effective form for a continuous interval). normalize and its forms
// divide the terms, in long double.

D_MATH_FN struct d_math_ratio
d_interval_imax_normalize_terms(
    struct d_interval_imax _interval,
    d_math_imax            _value
)
{
    return d_internal_interval_imax_terms(_interval.lower,
                                          _interval.upper,
                                          _value);
}

D_MATH_FN struct d_math_ratio
d_interval_imax_normalize_effective_terms(
    struct d_interval_imax _interval,
    d_math_imax            _value
)
{
    return d_internal_interval_imax_terms(
               d_interval_imax_inclusive_lower(_interval),
               d_interval_imax_inclusive_upper(_interval),
               _value);
}

D_MATH_FN struct d_math_ratio
d_interval_imax_normalize_discrete_terms(
    struct d_interval_imax _interval,
    d_math_imax            _value
)
{
    // a continuous interval normalizes over its inclusive bounds
    if (_interval.step <= 0)
    {
        return d_interval_imax_normalize_effective_terms(_interval, _value);
    }

    struct d_math_ratio result = { 0.0L, 1.0L };
    const d_math_umax   count  = d_interval_imax_count(_interval);

    // one member or none: nothing to spread over
    if (count <= 1u)
    {
        return result;
    }

    const d_math_imax low   = d_interval_imax_inclusive_lower(_interval);
    const d_math_umax step  = (d_math_umax)_interval.step;

    // below the first member the index is negative, truncated toward zero;
    // a zero index stays +0, so it prints and divides as 0 does
    if (_value >= low)
    {
        result.numerator =
            (long double)(((d_math_umax)_value - (d_math_umax)low) / step);
    }
    else
    {
        const d_math_umax back =
            ((d_math_umax)low - (d_math_umax)_value) / step;

        result.numerator = (back == 0u) ? 0.0L : -(long double)back;
    }

    result.denominator = (long double)(count - 1u);

    return result;
}

D_MATH_FN long double
d_interval_imax_normalize(
    struct d_interval_imax _interval,
    d_math_imax            _value
)
{
    const struct d_math_ratio terms =
        d_interval_imax_normalize_terms(_interval, _value);

    return (terms.numerator / terms.denominator);
}

D_MATH_FN long double
d_interval_imax_normalize_effective(
    struct d_interval_imax _interval,
    d_math_imax            _value
)
{
    const struct d_math_ratio terms =
        d_interval_imax_normalize_effective_terms(_interval, _value);

    return (terms.numerator / terms.denominator);
}

D_MATH_FN long double
d_interval_imax_normalize_discrete(
    struct d_interval_imax _interval,
    d_math_imax            _value
)
{
    const struct d_math_ratio terms =
        d_interval_imax_normalize_discrete_terms(_interval, _value);

    return (terms.numerator / terms.denominator);
}


//==============================================================================
// 3.  THE UNSIGNED FAMILY
//==============================================================================
// The same kernels over d_math_umax. A difference that would be negative --
// only in an empty interval, or for a value below one -- is taken as the
// signed family takes it, so the two families agree wherever both can hold
// the interval.


// 3.1    Internal helpers
//------------------------------------------------------------------------------

// d_internal_interval_umax_snap
//   the member reached from _from toward _to by whole steps, the quotient
// truncated toward zero. _step must be positive.
D_MATH_FN d_math_umax
d_internal_interval_umax_snap(
    d_math_umax _from,
    d_math_umax _to,
    d_math_umax _step
)
{
    // up and down are each exact; only the direction differs
    if (_to >= _from)
    {
        return (_from + (((_to - _from) / _step) * _step));
    }

    return (_from - (((_from - _to) / _step) * _step));
}

// d_internal_interval_umax_terms
//   (_value - _origin) and (_end - _origin) as long doubles, each difference
// exact before its one rounding; {0, 1} when _end == _origin.
D_MATH_FN struct d_math_ratio
d_internal_interval_umax_terms(
    d_math_umax _origin,
    d_math_umax _end,
    d_math_umax _value
)
{
    struct d_math_ratio result = { 0.0L, 1.0L };

    // a degenerate span maps everything to 0
    if (_end == _origin)
    {
        return result;
    }

    result.numerator =
        (_value >= _origin) ?  (long double)(_value  - _origin)
                            : -(long double)(_origin - _value);
    result.denominator =
        (_end >= _origin) ?  (long double)(_end    - _origin)
                          : -(long double)(_origin - _end);

    return result;
}

// d_internal_interval_umax_is_void
//   true when the interval is empty on the real line: reversed bounds, or one
// point with either bound open.
D_MATH_FN bool
d_internal_interval_umax_is_void(
    struct d_interval_umax _interval
)
{
    return ( (_interval.lower > _interval.upper) ||
             ( (_interval.lower == _interval.upper) &&
               (_interval.bounds != D_INTERVAL_CLOSED) ) );
}

// 3.2    Construction and validity
//------------------------------------------------------------------------------
// make assembles an interval from its parts, checking nothing; is_valid is
// true when lower <= upper.

D_MATH_FN struct d_interval_umax
d_interval_umax_make(
    d_math_umax _lower,
    d_math_umax _upper,
    unsigned    _bounds,
    d_math_umax _step
)
{
    struct d_interval_umax result = { 0u, 0u, 0u, 0u };

    result.lower  = _lower;
    result.upper  = _upper;
    result.step   = _step;
    result.bounds = _bounds;

    return result;
}

D_MATH_FN bool
d_interval_umax_is_valid(
    struct d_interval_umax _interval
)
{
    return (_interval.lower <= _interval.upper);
}

D_MATH_FN bool
d_interval_umax_is_discrete(
    struct d_interval_umax _interval
)
{
    return (_interval.step > 0u);
}

// 3.3    Inclusive bounds and size
//------------------------------------------------------------------------------
// as for the signed family (2.3).

D_MATH_FN d_math_umax
d_interval_umax_inclusive_lower(
    struct d_interval_umax _interval
)
{
    // an open lower bound excludes itself; at the family's maximum the
    // interval is empty, and the bound stays where it is
    if ( ((_interval.bounds & D_INTERVAL_LEFT_OPEN) != 0u) &&
         (_interval.lower < D_MATH_UMAX_MAX) )
    {
        return (_interval.lower + 1u);
    }

    return _interval.lower;
}

D_MATH_FN d_math_umax
d_interval_umax_inclusive_upper(
    struct d_interval_umax _interval
)
{
    // an open upper bound excludes itself; at 0 the interval is empty, and
    // the bound stays where it is
    if ( ((_interval.bounds & D_INTERVAL_RIGHT_OPEN) != 0u) &&
         (_interval.upper > 0u) )
    {
        return (_interval.upper - 1u);
    }

    return _interval.upper;
}

D_MATH_FN bool
d_interval_umax_is_empty(
    struct d_interval_umax _interval
)
{
    // reversed bounds hold nothing
    if (_interval.lower > _interval.upper)
    {
        return true;
    }

    const d_math_umax span  = (_interval.upper - _interval.lower);
    const d_math_umax opens =
        ( (((_interval.bounds & D_INTERVAL_LEFT_OPEN)  != 0u) ? 1u : 0u) +
          (((_interval.bounds & D_INTERVAL_RIGHT_OPEN) != 0u) ? 1u : 0u) );

    return (span < opens);
}

D_MATH_FN d_math_umax
d_interval_umax_first(
    struct d_interval_umax _interval
)
{
    return d_interval_umax_inclusive_lower(_interval);
}

D_MATH_FN d_math_umax
d_interval_umax_last(
    struct d_interval_umax _interval
)
{
    // a discrete interval ends on its last whole step
    if (_interval.step > 0u)
    {
        return d_internal_interval_umax_snap(
                   d_interval_umax_inclusive_lower(_interval),
                   d_interval_umax_inclusive_upper(_interval),
                   _interval.step);
    }

    return d_interval_umax_inclusive_upper(_interval);
}

D_MATH_FN d_math_umax
d_interval_umax_count(
    struct d_interval_umax _interval
)
{
    // an empty interval counts nothing
    if (d_interval_umax_is_empty(_interval))
    {
        return 0u;
    }

    const d_math_umax span =
        ( d_interval_umax_inclusive_upper(_interval) -
          d_interval_umax_inclusive_lower(_interval) );
    const d_math_umax steps =
        (_interval.step > 0u) ? (span / _interval.step) : span;

    return (steps == D_MATH_UMAX_MAX) ? steps : (steps + 1u);
}

// 3.4    Membership
//------------------------------------------------------------------------------
// as for the signed family (2.4).

D_MATH_FN bool
d_interval_umax_contains_in_range(
    struct d_interval_umax _interval,
    d_math_umax            _value
)
{
    const bool left_ok =
        ((_interval.bounds & D_INTERVAL_LEFT_OPEN) != 0u)
            ? (_value > _interval.lower)
            : (_value >= _interval.lower);
    const bool right_ok =
        ((_interval.bounds & D_INTERVAL_RIGHT_OPEN) != 0u)
            ? (_value < _interval.upper)
            : (_value <= _interval.upper);

    return (left_ok && right_ok);
}

D_MATH_FN bool
d_interval_umax_contains(
    struct d_interval_umax _interval,
    d_math_umax            _value
)
{
    // outside the bounds is outside the interval
    if (!d_interval_umax_contains_in_range(_interval, _value))
    {
        return false;
    }

    // a discrete member is a whole number of steps past the first
    if (_interval.step > 0u)
    {
        const d_math_umax offset =
            (_value - d_interval_umax_inclusive_lower(_interval));

        return ((offset % _interval.step) == 0u);
    }

    return true;
}

// 3.5    Clamping
//------------------------------------------------------------------------------
// as for the signed family (2.5).

D_MATH_FN d_math_umax
d_interval_umax_clamp(
    struct d_interval_umax _interval,
    d_math_umax            _value
)
{
    const d_math_umax low  = d_interval_umax_inclusive_lower(_interval);
    const d_math_umax high = d_interval_umax_inclusive_upper(_interval);

    // below the interval: its first value
    if (_value < low)
    {
        return low;
    }

    // above the interval: its last value
    if (_value > high)
    {
        return d_interval_umax_last(_interval);
    }

    // inside a discrete interval: down to a whole step
    if (_interval.step > 0u)
    {
        return d_internal_interval_umax_snap(low, _value, _interval.step);
    }

    return _value;
}

D_MATH_FN d_math_umax
d_interval_umax_clamp_nearest(
    struct d_interval_umax _interval,
    d_math_umax            _value
)
{
    // a continuous interval has no steps to round between
    if (_interval.step == 0u)
    {
        return d_interval_umax_clamp(_interval, _value);
    }

    const d_math_umax low = d_interval_umax_inclusive_lower(_interval);

    // below the interval: its first value
    if (_value < low)
    {
        return low;
    }

    const d_math_umax last = d_interval_umax_last(_interval);

    // above the last step: the last step
    if (_value > last)
    {
        return last;
    }

    const d_math_umax step  = _interval.step;
    const d_math_umax below = d_internal_interval_umax_snap(low,
                                                            _value,
                                                            step);
    const d_math_umax gap   = (_value - below);

    // the step above is nearer, and still inside the interval
    if ( ((last - below) >= step) &&
         (gap > (step - gap)) )
    {
        return (below + step);
    }

    return below;
}

// 3.6    Discrete access
//------------------------------------------------------------------------------
// as for the signed family (2.6).

D_MATH_FN d_math_umax
d_interval_umax_at(
    struct d_interval_umax _interval,
    d_math_umax            _index
)
{
    const d_math_umax stride = (_interval.step > 0u) ? _interval.step : 1u;

    return (d_interval_umax_inclusive_lower(_interval) + (_index * stride));
}

D_MATH_FN d_math_umax
d_interval_umax_index_of(
    struct d_interval_umax _interval,
    d_math_umax            _value
)
{
    // a value that is not a member has the index one past the last
    if (!d_interval_umax_contains(_interval, _value))
    {
        return d_interval_umax_count(_interval);
    }

    const d_math_umax offset =
        (_value - d_interval_umax_inclusive_lower(_interval));

    return (_interval.step > 0u) ? (offset / _interval.step) : offset;
}

// 3.7    Relations
//------------------------------------------------------------------------------
// as for the signed family (2.7).

D_MATH_FN bool
d_interval_umax_overlaps(
    struct d_interval_umax _left,
    struct d_interval_umax _right
)
{
    // an empty interval shares a position with nothing
    if ( (d_interval_umax_is_empty(_left)) ||
         (d_interval_umax_is_empty(_right)) )
    {
        return false;
    }

    return !( (d_interval_umax_inclusive_upper(_left) <
               d_interval_umax_inclusive_lower(_right)) ||
              (d_interval_umax_inclusive_lower(_left) >
               d_interval_umax_inclusive_upper(_right)) );
}

D_MATH_FN bool
d_interval_umax_overlaps_continuous(
    struct d_interval_umax _left,
    struct d_interval_umax _right
)
{
    // an interval empty on the real line meets nothing
    if ( (d_internal_interval_umax_is_void(_left)) ||
         (d_internal_interval_umax_is_void(_right)) )
    {
        return false;
    }

    const bool left_meets_closed =
        ( ((_left.bounds  & D_INTERVAL_LEFT_OPEN)  == 0u) &&
          ((_right.bounds & D_INTERVAL_RIGHT_OPEN) == 0u) );
    const bool right_meets_closed =
        ( ((_left.bounds  & D_INTERVAL_RIGHT_OPEN) == 0u) &&
          ((_right.bounds & D_INTERVAL_LEFT_OPEN)  == 0u) );
    const bool left_ok =
        left_meets_closed ? (_left.lower <= _right.upper)
                          : (_left.lower <  _right.upper);
    const bool right_ok =
        right_meets_closed ? (_right.lower <= _left.upper)
                           : (_right.lower <  _left.upper);

    return (left_ok && right_ok);
}

D_MATH_FN bool
d_interval_umax_intersects(
    struct d_interval_umax _left,
    struct d_interval_umax _right
)
{
    // an empty interval has no member to share
    if ( (d_interval_umax_is_empty(_left)) ||
         (d_interval_umax_is_empty(_right)) )
    {
        return false;
    }

    const d_math_umax left_first  = d_interval_umax_first(_left);
    const d_math_umax right_first = d_interval_umax_first(_right);
    const d_math_umax origin      = (left_first < right_first) ? left_first
                                                             : right_first;
    const d_math_umax left_step  =
        (_left.step > 0u) ? (d_math_umax)_left.step : 1u;
    const d_math_umax right_step =
        (_right.step > 0u) ? (d_math_umax)_right.step : 1u;

    // the two lattices as offsets from the lesser first member
    return d_internal_interval_lattice_meet(
        left_first - origin,
        left_step,
        d_interval_umax_last(_left) - origin,
        right_first - origin,
        right_step,
        d_interval_umax_last(_right) - origin);
}

// 3.8    Normalization
//------------------------------------------------------------------------------
// as for the signed family (2.8).

D_MATH_FN struct d_math_ratio
d_interval_umax_normalize_terms(
    struct d_interval_umax _interval,
    d_math_umax            _value
)
{
    return d_internal_interval_umax_terms(_interval.lower,
                                          _interval.upper,
                                          _value);
}

D_MATH_FN struct d_math_ratio
d_interval_umax_normalize_effective_terms(
    struct d_interval_umax _interval,
    d_math_umax            _value
)
{
    return d_internal_interval_umax_terms(
               d_interval_umax_inclusive_lower(_interval),
               d_interval_umax_inclusive_upper(_interval),
               _value);
}

D_MATH_FN struct d_math_ratio
d_interval_umax_normalize_discrete_terms(
    struct d_interval_umax _interval,
    d_math_umax            _value
)
{
    // a continuous interval normalizes over its inclusive bounds
    if (_interval.step == 0u)
    {
        return d_interval_umax_normalize_effective_terms(_interval, _value);
    }

    struct d_math_ratio result = { 0.0L, 1.0L };
    const d_math_umax   count  = d_interval_umax_count(_interval);

    // one member or none: nothing to spread over
    if (count <= 1u)
    {
        return result;
    }

    const d_math_umax low = d_interval_umax_inclusive_lower(_interval);

    // as for the signed family: a zero index stays +0
    if (_value >= low)
    {
        result.numerator = (long double)((_value - low) / _interval.step);
    }
    else
    {
        const d_math_umax back = (low - _value) / _interval.step;

        result.numerator = (back == 0u) ? 0.0L : -(long double)back;
    }

    result.denominator = (long double)(count - 1u);

    return result;
}

D_MATH_FN long double
d_interval_umax_normalize(
    struct d_interval_umax _interval,
    d_math_umax            _value
)
{
    const struct d_math_ratio terms =
        d_interval_umax_normalize_terms(_interval, _value);

    return (terms.numerator / terms.denominator);
}

D_MATH_FN long double
d_interval_umax_normalize_effective(
    struct d_interval_umax _interval,
    d_math_umax            _value
)
{
    const struct d_math_ratio terms =
        d_interval_umax_normalize_effective_terms(_interval, _value);

    return (terms.numerator / terms.denominator);
}

D_MATH_FN long double
d_interval_umax_normalize_discrete(
    struct d_interval_umax _interval,
    d_math_umax            _value
)
{
    const struct d_math_ratio terms =
        d_interval_umax_normalize_discrete_terms(_interval, _value);

    return (terms.numerator / terms.denominator);
}


//==============================================================================
// 4.  FORMATTING
//==============================================================================
// An interval as text: its brackets -- `[` or `(` on the left, `]` or `)` on
// the right -- around "lower, upper", or "lower:step:upper" for a discrete
// interval. Numbers are written in decimal, as printf's %jd and %ju would
// write them, without the C library.


// 4.1    Internal helpers
//------------------------------------------------------------------------------

// d_internal_interval_put_char
//   writes _char at _position while room remains for it and the terminator;
// returns the position after it, written or not.
D_MATH_FN_RT size_t
d_internal_interval_put_char(
    char*  _buffer,
    size_t _capacity,
    size_t _position,
    char   _char
)
{
    // a character is written only where the terminator still fits after it
    if ( (_buffer != NULL) &&
         ((_position + 1u) < _capacity) )
    {
        _buffer[_position] = _char;
    }

    return (_position + 1u);
}

// d_internal_interval_put_umax
//   writes _value in decimal at _position; returns the position after it.
D_MATH_FN_RT size_t
d_internal_interval_put_umax(
    char*       _buffer,
    size_t      _capacity,
    size_t      _position,
    d_math_umax _value
)
{
    char        digits[64] = { 0 };
    size_t      count      = 0u;
    d_math_umax rest       = _value;

    // collect the digits least significant first; 0 is one digit
    do
    {
        digits[count] = (char)('0' + (int)(rest % 10u));
        ++count;
        rest /= 10u;
    } while (rest != 0u);

    size_t position = _position;

    // emit them most significant first
    while (count > 0u)
    {
        --count;
        position = d_internal_interval_put_char(_buffer,
                                                _capacity,
                                                position,
                                                digits[count]);
    }

    return position;
}

// d_internal_interval_put_imax
//   writes _value in decimal at _position, a minus sign first when it is
// negative; returns the position after it.
D_MATH_FN_RT size_t
d_internal_interval_put_imax(
    char*       _buffer,
    size_t      _capacity,
    size_t      _position,
    d_math_imax _value
)
{
    // the magnitude is taken in the unsigned family, which holds that of the
    // family's minimum
    if (_value < 0)
    {
        const size_t after_sign = d_internal_interval_put_char(_buffer,
                                                               _capacity,
                                                               _position,
                                                               '-');

        return d_internal_interval_put_umax(_buffer,
                                            _capacity,
                                            after_sign,
                                            (d_math_umax)0u -
                                            (d_math_umax)_value);
    }

    return d_internal_interval_put_umax(_buffer,
                                        _capacity,
                                        _position,
                                        (d_math_umax)_value);
}

// d_internal_interval_finish
//   terminates the text at _length, or at the last byte when it did not
// fit; returns _length.
D_MATH_FN_RT size_t
d_internal_interval_finish(
    char*  _buffer,
    size_t _capacity,
    size_t _length
)
{
    // nothing is written into a buffer with no room at all
    if ( (_buffer != NULL) &&
         (_capacity > 0u) )
    {
        _buffer[(_length < _capacity) ? _length : (_capacity - 1u)] = '\0';
    }

    return _length;
}

// 4.2    d_interval_imax_format, d_interval_umax_format
//------------------------------------------------------------------------------

/**
 * @brief Writes an interval as text: "[a, b]", "(a, b)", "[a, b)", "(a, b]",
 *        or "[a:s:b]" (with the same brackets) for a discrete interval.
 *
 * @note As snprintf does, it writes at most _capacity bytes, the terminator
 *       among them, and returns the length the whole text has; a result not
 *       below _capacity means the text was cut short. A NULL _buffer with a
 *       _capacity of 0 measures without writing.
 *
 * @param[out] _buffer    where the text goes; may be NULL if _capacity is 0.
 * @param[in]  _capacity  the bytes _buffer holds.
 * @param[in]  _interval  the interval to write.
 * @return the length of the whole text, not counting the terminator.
 */
D_MATH_FN_RT size_t
d_interval_imax_format(
    char*                  _buffer,
    size_t                 _capacity,
    struct d_interval_imax _interval
)
{
    const char left  =
        ((_interval.bounds & D_INTERVAL_LEFT_OPEN) != 0u) ? '(' : '[';
    const char right =
        ((_interval.bounds & D_INTERVAL_RIGHT_OPEN) != 0u) ? ')' : ']';
    size_t     at    = d_internal_interval_put_char(_buffer,
                                                    _capacity,
                                                    0u,
                                                    left);

    at = d_internal_interval_put_imax(_buffer, _capacity, at, _interval.lower);

    // a discrete interval shows its stride between its bounds
    if (_interval.step > 0)
    {
        at = d_internal_interval_put_char(_buffer, _capacity, at, ':');
        at = d_internal_interval_put_imax(_buffer,
                                          _capacity,
                                          at,
                                          _interval.step);
        at = d_internal_interval_put_char(_buffer, _capacity, at, ':');
    }
    else
    {
        at = d_internal_interval_put_char(_buffer, _capacity, at, ',');
        at = d_internal_interval_put_char(_buffer, _capacity, at, ' ');
    }

    at = d_internal_interval_put_imax(_buffer, _capacity, at, _interval.upper);
    at = d_internal_interval_put_char(_buffer, _capacity, at, right);

    return d_internal_interval_finish(_buffer, _capacity, at);
}

/**
 * @brief Writes an interval of the unsigned family as text, as
 *        d_interval_imax_format does.
 *
 * @param[out] _buffer    where the text goes; may be NULL if _capacity is 0.
 * @param[in]  _capacity  the bytes _buffer holds.
 * @param[in]  _interval  the interval to write.
 * @return the length of the whole text, not counting the terminator.
 */
D_MATH_FN_RT size_t
d_interval_umax_format(
    char*                  _buffer,
    size_t                 _capacity,
    struct d_interval_umax _interval
)
{
    const char left  =
        ((_interval.bounds & D_INTERVAL_LEFT_OPEN) != 0u) ? '(' : '[';
    const char right =
        ((_interval.bounds & D_INTERVAL_RIGHT_OPEN) != 0u) ? ')' : ']';
    size_t     at    = d_internal_interval_put_char(_buffer,
                                                    _capacity,
                                                    0u,
                                                    left);

    at = d_internal_interval_put_umax(_buffer, _capacity, at, _interval.lower);

    // a discrete interval shows its stride between its bounds
    if (_interval.step > 0u)
    {
        at = d_internal_interval_put_char(_buffer, _capacity, at, ':');
        at = d_internal_interval_put_umax(_buffer,
                                          _capacity,
                                          at,
                                          _interval.step);
        at = d_internal_interval_put_char(_buffer, _capacity, at, ':');
    }
    else
    {
        at = d_internal_interval_put_char(_buffer, _capacity, at, ',');
        at = d_internal_interval_put_char(_buffer, _capacity, at, ' ');
    }

    at = d_internal_interval_put_umax(_buffer, _capacity, at, _interval.upper);
    at = d_internal_interval_put_char(_buffer, _capacity, at, right);

    return d_internal_interval_finish(_buffer, _capacity, at);
}


#endif  // DJINTERP_C_MATH_INTERVAL_H
