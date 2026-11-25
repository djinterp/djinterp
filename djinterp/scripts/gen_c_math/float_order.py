"""gen_c_math.float_order - the text of float_order.h (S439).

One module of scripts/gen_c_math_float.py, which runs them all; see it."""

from .common import (DATE, FAMILIES, FLOAT_ORDER_OVERLOADS,
                     INTERVAL_FLOAT_OVERLOADS, banner, instantiate,
                     overload_section)

# -----------------------------------------------------------------------------
# float_order.h
# -----------------------------------------------------------------------------

FLOAT_ORDER_TEMPLATE = r"""
//==============================================================================
// @SECTION@.  THE @UPPER@ FAMILY (_@S@)
//==============================================================================


// @SECTION@.1    Internal helpers
//------------------------------------------------------------------------------

// dj_internal_math_@S@_binade
//   struct: a positive normal value's binade: the exponent E and 2^E, so that
// 2^E <= x < 2^(E + 1).
struct dj_internal_math_@S@_binade
{
    int exponent;  // E
    @T@ power;     // 2^E, exact
};

// d_internal_math_@S@_pow2
//   2^_exponent, exact, for |_exponent| up to @EMAX@ - 1 - every
// normal @NAME@'s exponent among them: 2^|_exponent| from its bits, the
// least first, each square 2^(2^k) formed only while a bit above k remains,
// so that none passes 2^|_exponent|; for a negative exponent, then, its
// reciprocal, one exact division.
D_MATH_FN @T@
d_internal_math_@S@_pow2(
    int _exponent
)
{
    @T@ result = (@T@)1;
    @T@ square = (@T@)2;
    int left   = (_exponent < 0) ? -_exponent : _exponent;

    while (left > 0)
    {
        if ((left % 2) == 1)
        {
            result *= square;
        }

        left /= 2;

        if (left > 0)
        {
            square *= square;
        }
    }

    return (_exponent < 0) ? ((@T@)1 / result) : result;
}

// d_internal_math_@S@_veltkamp
//   Veltkamp's constant 2^ceil(P/2) + 1, P the family's precision, which
// splits a value into halves whose products are exact (Dekker's product):
// written out for the precisions there are - 24, 53, 64 and 113 bits - so
// that no call builds it, and built for any other.
D_MATH_FN @T@
d_internal_math_@S@_veltkamp(void)
{
#if (@P@ == 24)
    return (@T@)4097.0L;                  // 2^12 + 1
#elif (@P@ == 53)
    return (@T@)134217729.0L;             // 2^27 + 1
#elif (@P@ == 64)
    return (@T@)4294967297.0L;            // 2^32 + 1
#elif (@P@ == 113)
    return (@T@)144115188075855873.0L;    // 2^57 + 1
#else
    return d_internal_math_@S@_pow2((@P@ + 1) / 2) + (@T@)1;
#endif
}

// d_internal_math_@S@_binade_of
//   the binade of _x, which must be normal and positive: @MIN@ <= _x <=
// @MAX@. A binary search over the squares 2^(2^k) - below 1, scaling _x
// itself up toward 1 and dividing once at the end; no value it forms leaves
// the normal range, so every step is exact and, from C++14, a constant
// expression.
D_MATH_FN struct dj_internal_math_@S@_binade
d_internal_math_@S@_binade_of(
    @T@ _x
)
{
    struct dj_internal_math_@S@_binade result = { 0, (@T@)1 };
    @T@ squares[16] = { 0 };
    int count       = 1;
    int k           = 0;

    squares[0] = (@T@)2;

    // 2^(2^k) for every k with 2^k at most the largest exponent
    while ( (count < 16) &&
            ((1 << count) <= (@EMAX@ - 1)) )
    {
        squares[count] = squares[count - 1] * squares[count - 1];
        ++count;
    }

    // at or above 1: climb while the next power stays at or below _x
    if (_x >= (@T@)1)
    {
        for (k = count - 1; k >= 0; --k)
        {
            if ( ((result.exponent + (1 << k)) <= (@EMAX@ - 1)) &&
                 ((result.power * squares[k]) <= _x) )
            {
                result.power    *= squares[k];
                result.exponent += (1 << k);
            }
        }

        return result;
    }

    // below 1: scale _x up by 2^e while it stays below 1 - then it lies in
    // [1/2, 1), E is -e - 1, and 2^E the reciprocal of 2^(e + 1)
    {
        @T@ climbed = _x;
        @T@ up     = (@T@)1;
        int e      = 0;

        for (k = count - 1; k >= 0; --k)
        {
            if ( ((e + (1 << k)) <= -@EMIN@) &&
                 ((climbed * squares[k]) < (@T@)1) )
            {
                climbed *= squares[k];
                up      *= squares[k];
                e       += (1 << k);
            }
        }

        result.exponent = -e - 1;
        result.power    = (@T@)1 / (up + up);
    }

    return result;
}

// d_internal_math_@S@_true_min
//   the least positive @NAME@: the subnormal spacing,
// @MIN@ * @EPS@.
D_MATH_FN @T@
d_internal_math_@S@_true_min(void)
{
    return (@MIN@ * @EPS@);
}

// dj_internal_math_@S@_place
//   a non-negative value's place in the order: binade B and offset O within
// it, so that its ordinal is B * 2^(P-1) + O. B is 0 for zero and the
// subnormals; O is an integer below 2^(P-1), held exactly in @T@;
// +inf is the first place past the largest binade.
struct dj_internal_math_@S@_place
{
    int binade;  // B
    @T@ offset;  // O
};

D_MATH_FN struct dj_internal_math_@S@_place
d_internal_math_@S@_place_of(
    @T@ _x
)
{
    struct dj_internal_math_@S@_place result = { 0, (@T@)0 };

    // +inf is the order's end: one step past the largest finite value, the
    // first place of the binade after the last
    if (_x > @MAX@)
    {
        result.binade = (@EMAX@ - (@EMIN@ - 1)) + 1;

        return result;
    }

    // zero and the subnormals: one binade, in steps of the least value
    if (_x < @MIN@)
    {
        result.offset = _x / d_internal_math_@S@_true_min();

        return result;
    }

    {
        const struct dj_internal_math_@S@_binade b =
            d_internal_math_@S@_binade_of(_x);

        result.binade = b.exponent - (@EMIN@ - 1) + 1;
        result.offset = (_x - b.power) / (b.power * @EPS@);
    }

    return result;
}

// d_internal_math_@S@_value_of
//   the inverse of place_of: the value at binade _binade, offset _offset.
D_MATH_FN @T@
d_internal_math_@S@_value_of(
    int _binade,
    @T@ _offset
)
{
    // the subnormal binade
    if (_binade == 0)
    {
        return (_offset * d_internal_math_@S@_true_min());
    }

    {
        const @T@ power =
            d_internal_math_@S@_pow2(_binade - 1 + (@EMIN@ - 1));

        return (power + (_offset * (power * @EPS@)));
    }
}

// d_internal_math_@S@_umax_bits
//   the bits of dj_math_umax, assuming none is padding.
D_MATH_FN int
d_internal_math_@S@_umax_bits(void)
{
    return (int)(sizeof(dj_math_umax) * 8u);
}

// d_internal_math_@S@_full
//   2^(P-1), the count of one binade, as dj_math_umax: asked for only where it
// fits, and the shift clamped so the expression is well-formed where it does
// not (a 32-bit dj_math_umax and a double's 52).
D_MATH_FN dj_math_umax
d_internal_math_@S@_full(void)
{
    return ((dj_math_umax)1)
               << ( ((@P@ - 1) < (int)(sizeof(dj_math_umax) * 8u))
                        ? (@P@ - 1)
                        : 0 );
}

// d_internal_math_@S@_steps_up
//   how many values lie in [_from, _to), for 0 <= _from <= _to, +inf among
// them: from their places, so the count is exact wherever it fits
// dj_math_umax, however far from zero the two lie; it saturates at
// D_MATH_UMAX_MAX.
D_MATH_FN dj_math_umax
d_internal_math_@S@_steps_up(
    @T@ _from,
    @T@ _to
)
{
    const struct dj_internal_math_@S@_place low  =
        d_internal_math_@S@_place_of(_from);
    const struct dj_internal_math_@S@_place high =
        d_internal_math_@S@_place_of(_to);
    const int binades = high.binade - low.binade;

    // a binade holds more than dj_math_umax counts: a count fits within one,
    // or across one edge - the rest of the lower binade and the start of
    // the upper, each below 2^(P-1) and their sum exact in @T@
    if ((@P@ - 1) >= d_internal_math_@S@_umax_bits())
    {
        const @T@ full = d_internal_math_@S@_pow2(@P@ - 1);
        const @T@ count =
            (binades == 0) ? (high.offset - low.offset)
                           : ((full - low.offset) + high.offset);

        if ( (binades > 1) ||
             (!(count < (@T@)D_MATH_UMAX_MAX)) )
        {
            return D_MATH_UMAX_MAX;
        }

        return (dj_math_umax)count;
    }

    {
        const dj_math_umax full        = d_internal_math_@S@_full();
        const dj_math_umax from_offset = (dj_math_umax)low.offset;
        const dj_math_umax to_offset   = (dj_math_umax)high.offset;
        const dj_math_umax whole       = (dj_math_umax)binades;

        // whole * full + to_offset - from_offset, saturating
        if (whole > ((D_MATH_UMAX_MAX - to_offset) / full))
        {
            return D_MATH_UMAX_MAX;
        }

        return (((whole * full) + to_offset) - from_offset);
    }
}

// d_internal_math_@S@_climb
//   the value _steps above a non-negative finite _from; +inf past the
// largest finite value.
D_MATH_FN @T@
d_internal_math_@S@_climb(
    @T@          _from,
    dj_math_umax _steps
)
{
    const struct dj_internal_math_@S@_place p =
        d_internal_math_@S@_place_of(_from);
    // the largest binade
    const int top    = (@EMAX@ - (@EMIN@ - 1));
    int       binade = p.binade;
    @T@       offset = p.offset;

    // a binade holds more than dj_math_umax: the steps cross at most one edge
    if ((@P@ - 1) >= d_internal_math_@S@_umax_bits())
    {
        const @T@ full = d_internal_math_@S@_pow2(@P@ - 1);

        offset += (@T@)_steps;

        if (!(offset < full))
        {
            offset -= full;
            ++binade;
        }
    }
    else
    {
        const dj_math_umax full  = d_internal_math_@S@_full();
        const dj_math_umax whole = _steps / full;
        dj_math_umax       rest  = ((dj_math_umax)offset) + (_steps % full);

        // whole binades first, then what is left within one
        if (whole > (dj_math_umax)top)
        {
            return (@T@)HUGE_VAL;
        }

        binade += (int)whole;

        if (rest >= full)
        {
            rest -= full;
            ++binade;
        }

        offset = (@T@)rest;
    }

    // past the largest binade lies infinity
    if (binade > top)
    {
        return (@T@)HUGE_VAL;
    }

    return d_internal_math_@S@_value_of(binade, offset);
}

// d_internal_math_@S@_descend
//   the value _steps below a positive _from, +inf among them; _steps must be
// fewer than the values in [0, _from), and from +inf at least one - its
// place lies past the last binade, which value_of does not build.
D_MATH_FN @T@
d_internal_math_@S@_descend(
    @T@          _from,
    dj_math_umax _steps
)
{
    const struct dj_internal_math_@S@_place p =
        d_internal_math_@S@_place_of(_from);
    int binade = p.binade;
    @T@ offset = p.offset;

    // a binade holds more than dj_math_umax: at most one edge is crossed
    if ((@P@ - 1) >= d_internal_math_@S@_umax_bits())
    {
        const @T@ back = (@T@)_steps;

        if (back <= offset)
        {
            offset -= back;
        }
        else
        {
            const @T@ full = d_internal_math_@S@_pow2(@P@ - 1);

            offset = (offset + full) - back;
            --binade;
        }
    }
    else
    {
        const dj_math_umax full  = d_internal_math_@S@_full();
        const dj_math_umax rest  = _steps % full;
        dj_math_umax       place = (dj_math_umax)offset;

        // whole binades first, then what is left within one
        binade -= (int)(_steps / full);

        if (rest > place)
        {
            place = (place + full) - rest;
            --binade;
        }
        else
        {
            place -= rest;
        }

        offset = (@T@)place;
    }

    return d_internal_math_@S@_value_of(binade, offset);
}

// @SECTION@.2    Successor and predecessor
//------------------------------------------------------------------------------
// next_up is the least value above _x, next_down the greatest below:
// nextafter toward +inf and toward -inf, in pure arithmetic. +0 and -0 are
// one value, so next_up(-least) is 0. NaN stays NaN; next_up(+inf) is +inf
// and next_up(-inf) the most negative finite value.

D_MATH_FN @T@
d_math_@S@_next_up(
    @T@ _x
)
{
    // NaN and +inf are their own successors
    if ( (_x != _x) ||
         (_x > @MAX@) )
    {
        return _x;
    }

    // -inf rises to the most negative finite value
    if (_x < -@MAX@)
    {
        return -@MAX@;
    }

    // the largest finite value rises to +inf
    if (_x == @MAX@)
    {
        return (@T@)HUGE_VAL;
    }

    // zero and the subnormals are evenly spaced by the least value
    if ( (_x < @MIN@) &&
         (_x > -@MIN@) )
    {
        return (_x + d_internal_math_@S@_true_min());
    }

    // a positive normal value: one spacing of its binade up
    if (_x > (@T@)0)
    {
        const struct dj_internal_math_@S@_binade b =
            d_internal_math_@S@_binade_of(_x);

        return (_x + (b.power * @EPS@));
    }

    {
        // a negative normal value: toward zero by the spacing below its
        // magnitude, which below a power of two is half its own - except
        // below the least normal value, where it is the subnormal spacing
        const @T@ magnitude = -_x;
        const struct dj_internal_math_@S@_binade b =
            d_internal_math_@S@_binade_of(magnitude);
        const @T@ spacing =
            ( (magnitude == b.power) && (magnitude > @MIN@) )
                ? ((b.power * @EPS@) / (@T@)2)
                : (b.power * @EPS@);

        return -(magnitude - spacing);
    }
}

D_MATH_FN @T@
d_math_@S@_next_down(
    @T@ _x
)
{
    return -d_math_@S@_next_up(-_x);
}

// @SECTION@.3    Ordinal distance and advance
//------------------------------------------------------------------------------
// steps is how many values lie in [_from, _to) - for adjacent values 1 -
// for _from <= _to, saturating at D_MATH_UMAX_MAX; advance is the value
// _steps above _from, +inf past the largest finite value, and no steps
// leave _from where it is. The infinities are the order's ends, one step
// past the largest finite values; NaN is in no order, so advance keeps a
// NaN, as next_up does, and a count with one saturates: there is none to
// give. So advance(a, steps(a, b)) == b wherever steps(a, b) does not
// saturate.

D_MATH_FN dj_math_umax
d_math_@S@_steps(
    @T@ _from,
    @T@ _to
)
{
    // a NaN: no count
    if ( (_from != _from) ||
         (_to != _to) )
    {
        return D_MATH_UMAX_MAX;
    }

    // both at or above zero
    if (_from >= (@T@)0)
    {
        return d_internal_math_@S@_steps_up(_from, _to);
    }

    // both at or below zero: the mirror image
    if (_to <= (@T@)0)
    {
        return d_internal_math_@S@_steps_up(-_to, -_from);
    }

    {
        // across zero: up to zero, then on from it, saturating
        const dj_math_umax below =
            d_internal_math_@S@_steps_up((@T@)0, -_from);
        const dj_math_umax above =
            d_internal_math_@S@_steps_up((@T@)0, _to);

        return (above > (D_MATH_UMAX_MAX - below)) ? D_MATH_UMAX_MAX
                                                   : (below + above);
    }
}

D_MATH_FN @T@
d_math_@S@_advance(
    @T@          _from,
    dj_math_umax _steps
)
{
    // a NaN stays NaN
    if (_from != _from)
    {
        return _from;
    }

    // no steps stay put - from -inf too, where descend would start past
    // the last binade; a zero comes back as +0, as the climb gives it
    if (_steps == 0u)
    {
        return (_from == (@T@)0) ? (@T@)0 : _from;
    }

    // from zero or above: climb
    if (_from >= (@T@)0)
    {
        return d_internal_math_@S@_climb(_from, _steps);
    }

    {
        // from below zero: as many steps as reach zero, at most, stay below
        // it - a count that saturates is more than _steps can be
        const dj_math_umax to_zero =
            d_internal_math_@S@_steps_up((@T@)0, -_from);

        if ( (_steps < to_zero) ||
             (to_zero == D_MATH_UMAX_MAX) )
        {
            return -d_internal_math_@S@_descend(-_from, _steps);
        }

        return d_internal_math_@S@_climb((@T@)0, _steps - to_zero);
    }
}
"""

FLOAT_ORDER_HEAD = r"""
/*
TABLE OF CONTENTS
=================
1.  THE FLOAT FAMILY (_f)
    ---------------------
    1.  Internal helpers
    2.  Successor and predecessor
    3.  Ordinal distance and advance
2.  THE DOUBLE FAMILY (_d)
    ----------------------
    1.  Internal helpers
    2.  Successor and predecessor
    3.  Ordinal distance and advance
3.  THE LONG DOUBLE FAMILY (_ld)
    ----------------------------
    1.  Internal helpers
    2.  Successor and predecessor
    3.  Ordinal distance and advance
4.  ONE NAME FOR EVERY FAMILY (C++)
    -------------------------------
*/

#ifndef DJINTERP_C_MATH_FLOAT_ORDER_H
#define DJINTERP_C_MATH_FLOAT_ORDER_H 1

// std
#include <float.h>           // FLT_MANT_DIG ... LDBL_EPSILON
#include <math.h>            // HUGE_VAL
// djinterp
#include "./math_common.h"   // D_MATH_FN, dj_math_umax, D_MATH_UMAX_MAX
"""

FLOAT_ORDER_TAIL = r"""

#endif  // DJINTERP_C_MATH_FLOAT_ORDER_H
"""


def float_order():
    lines = [
        "The order of a floating-point type's values: successor, predecessor,",
        "ordinal distance and advance, for float, double and long double.",
        "  GENERATED by scripts/gen_c_math_float.py from one text for the three",
        "families: edit the generator, not this file.",
        "",
        "  Every function is pure arithmetic - no bit pattern is read, no C",
        "library function called - so the same text is constexpr from C++14,",
        "where a face needs a successor in a template argument. The values are",
        "taken as numbers, so +0 and -0 are one value; NaN is no value's",
        "neighbour. Subnormals are assumed (IEEE 754): a build that flushes them",
        "to zero sees gaps the order does not know about.",
    ]
    text = banner("float_order.h", lines, DATE, "2026.11.21")
    text += "\n" + FLOAT_ORDER_HEAD
    for i, (suffix, ctype, prefix, prose) in enumerate(FAMILIES):
        upper = prose.upper()
        body = FLOAT_ORDER_TEMPLATE.replace("@SECTION@", str(i + 1)).replace("@UPPER@", upper)
        text += instantiate(body, suffix, ctype, prefix, prose)
    text += overload_section("4.  ONE NAME FOR EVERY FAMILY (C++)", FLOAT_ORDER_OVERLOADS)
    text += FLOAT_ORDER_TAIL
    return text


INTERVAL_FLOAT_TEMPLATE = r"""
//==============================================================================
// @SECTION@.  THE @UPPER@ FAMILY (_@S@)
//==============================================================================


// @SECTION@.1    Layout
//------------------------------------------------------------------------------
// @SECTION@.1.1
// dj_interval_@S@
//   struct: an interval over @NAME@.
struct dj_interval_@S@
{
    @T@      lower;   // the lower bound
    @T@      upper;   // the upper bound
    @T@      step;    // the stride: 0 continuous, > 0 discrete
    unsigned bounds;  // D_INTERVAL_LEFT_OPEN | D_INTERVAL_RIGHT_OPEN
};

// @SECTION@.2    Internal helpers
//------------------------------------------------------------------------------

// d_internal_interval_@S@_member
//   a discrete interval's member _index steps past _first: _first + _index *
// _step, the product formed apart so no compiler fuses it into the sum.
D_MATH_FN @T@
d_internal_interval_@S@_member(
    @T@          _first,
    @T@          _step,
    dj_math_umax _index
)
{
    // the first member is the first, whatever the step (0 * inf is NaN)
    if (_index == 0u)
    {
        return _first;
    }

    {
        const @T@ offset = (@T@)_index * _step;

        return (_first + offset);
    }
}

// d_internal_interval_@S@_snap_finite
//   the index of the last member at or below a finite _value, for _value >=
// _first and finite _first and _step: the rounded quotient, settled by
// testing the members beside it.
D_MATH_FN dj_math_umax
d_internal_interval_@S@_snap_finite(
    @T@ _first,
    @T@ _step,
    @T@ _value
)
{
    const @T@    quotient = (_value - _first) / _step;
    dj_math_umax index    = 0u;
    int          tries    = 0;

    // the quotient as an index: saturated, or truncated
    if (!(quotient < (@T@)D_MATH_UMAX_MAX))
    {
        index = D_MATH_UMAX_MAX;
    }
    else if (quotient > (@T@)0)
    {
        index = (dj_math_umax)quotient;
    }

    // the quotient was rounded: down while the member passes _value...
    for (tries = 0;
         ( (tries < 4) &&
           (index > 0u) &&
           (d_internal_interval_@S@_member(_first, _step, index) > _value) );
         ++tries)
    {
        --index;
    }

    // ...and up while the next one does not
    for (tries = 0;
         ( (tries < 4) &&
           (index < D_MATH_UMAX_MAX) &&
           (d_internal_interval_@S@_member(_first, _step, index + 1u) <=
            _value) );
         ++tries)
    {
        ++index;
    }

    return index;
}

// d_internal_interval_@S@_snap
//   the index of the last member at or below _value, for _value >= _first.
// An infinite first member or step leaves one member, the first; members
// past the largest finite value round to +inf, one member, after the last
// finite one.
D_MATH_FN dj_math_umax
d_internal_interval_@S@_snap(
    @T@ _first,
    @T@ _step,
    @T@ _value
)
{
    // an infinite first member or step: the first is the only member
    if ( (_first > @MAX@) ||
         (_first < -@MAX@) ||
         (_step > @MAX@) )
    {
        return 0u;
    }

    // +inf: the member after the last finite one
    if (_value > @MAX@)
    {
        const dj_math_umax last =
            d_internal_interval_@S@_snap_finite(_first, _step, @MAX@);

        return (last == D_MATH_UMAX_MAX) ? last : (last + 1u);
    }

    return d_internal_interval_@S@_snap_finite(_first, _step, _value);
}

// d_internal_interval_@S@_is_void
//   true when the interval is empty on the real line: reversed or NaN
// bounds, or one point with either bound open.
D_MATH_FN bool
d_internal_interval_@S@_is_void(
    struct dj_interval_@S@ _interval
)
{
    return ( (!(_interval.lower <= _interval.upper)) ||
             ( (_interval.lower == _interval.upper) &&
               (_interval.bounds != D_INTERVAL_CLOSED) ) );
}

// @SECTION@.3    Construction, validity and bounds
//------------------------------------------------------------------------------
// as for the integer families; an open bound moves to the neighbouring
// value (float_order.h's next_up and next_down), so inclusive_lower is the
// least value the interval holds and inclusive_upper the greatest.

D_MATH_FN struct dj_interval_@S@
d_interval_@S@_make(
    @T@      _lower,
    @T@      _upper,
    unsigned _bounds,
    @T@      _step
)
{
    struct dj_interval_@S@ result = { (@T@)0,
                                      (@T@)0,
                                      (@T@)0,
                                      0u };

    result.lower  = _lower;
    result.upper  = _upper;
    result.step   = _step;
    result.bounds = _bounds;

    return result;
}

D_MATH_FN bool
d_interval_@S@_is_valid(
    struct dj_interval_@S@ _interval
)
{
    return ( (_interval.lower <= _interval.upper) &&
             (_interval.step >= (@T@)0) );
}

D_MATH_FN bool
d_interval_@S@_is_discrete(
    struct dj_interval_@S@ _interval
)
{
    return (_interval.step > (@T@)0);
}

D_MATH_FN @T@
d_interval_@S@_inclusive_lower(
    struct dj_interval_@S@ _interval
)
{
    return ((_interval.bounds & D_INTERVAL_LEFT_OPEN) != 0u)
               ? d_math_@S@_next_up(_interval.lower)
               : _interval.lower;
}

D_MATH_FN @T@
d_interval_@S@_inclusive_upper(
    struct dj_interval_@S@ _interval
)
{
    return ((_interval.bounds & D_INTERVAL_RIGHT_OPEN) != 0u)
               ? d_math_@S@_next_down(_interval.upper)
               : _interval.upper;
}

D_MATH_FN bool
d_interval_@S@_is_empty(
    struct dj_interval_@S@ _interval
)
{
    // an open bound at an infinity excludes it, and nothing lies beyond it
    if ( ( ((_interval.bounds & D_INTERVAL_LEFT_OPEN) != 0u) &&
           (_interval.lower > @MAX@) ) ||
         ( ((_interval.bounds & D_INTERVAL_RIGHT_OPEN) != 0u) &&
           (_interval.upper < -@MAX@) ) )
    {
        return true;
    }

    return !( d_interval_@S@_inclusive_lower(_interval) <=
              d_interval_@S@_inclusive_upper(_interval) );
}

// @SECTION@.4    Size and access
//------------------------------------------------------------------------------
// A continuous interval's members are every value of the type within its
// bounds, so count is their number (float_order.h's steps, plus one), at
// the value that many steps up, and index_of the steps to a value. A
// discrete interval's members are first + k * step, each product formed
// apart. Both counts saturate at D_MATH_UMAX_MAX.

D_MATH_FN @T@
d_interval_@S@_first(
    struct dj_interval_@S@ _interval
)
{
    return d_interval_@S@_inclusive_lower(_interval);
}

D_MATH_FN @T@
d_interval_@S@_last(
    struct dj_interval_@S@ _interval
)
{
    const @T@ low  = d_interval_@S@_inclusive_lower(_interval);
    const @T@ high = d_interval_@S@_inclusive_upper(_interval);

    // a discrete interval ends on its last whole step
    if ( (_interval.step > (@T@)0) &&
         (low <= high) )
    {
        return d_internal_interval_@S@_member(
                   low,
                   _interval.step,
                   d_internal_interval_@S@_snap(low, _interval.step, high));
    }

    return high;
}

D_MATH_FN dj_math_umax
d_interval_@S@_count(
    struct dj_interval_@S@ _interval
)
{
    const @T@ low  = d_interval_@S@_inclusive_lower(_interval);
    const @T@ high = d_interval_@S@_inclusive_upper(_interval);
    dj_math_umax steps = 0u;

    // an empty interval counts nothing
    if (d_interval_@S@_is_empty(_interval))
    {
        return 0u;
    }

    steps = (_interval.step > (@T@)0)
                ? d_internal_interval_@S@_snap(low, _interval.step, high)
                : d_math_@S@_steps(low, high);

    return (steps == D_MATH_UMAX_MAX) ? steps : (steps + 1u);
}

D_MATH_FN @T@
d_interval_@S@_at(
    struct dj_interval_@S@ _interval,
    dj_math_umax           _index
)
{
    const @T@ low = d_interval_@S@_inclusive_lower(_interval);

    // a discrete member, or the value _index steps up
    if (_interval.step > (@T@)0)
    {
        return d_internal_interval_@S@_member(low, _interval.step, _index);
    }

    return d_math_@S@_advance(low, _index);
}

// @SECTION@.5    Membership and clamping
//------------------------------------------------------------------------------
// contains honours both bounds exactly and, for a discrete interval, the
// members; contains_in_range the bounds alone; NaN is in no interval. clamp
// moves a value to the nearest member toward the interval - the
// neighbouring value inside an open bound - and, inside a discrete one,
// down to a member; clamp_nearest to the nearer member, a tie going down.
// NaN clamps to NaN.

D_MATH_FN bool
d_interval_@S@_contains_in_range(
    struct dj_interval_@S@ _interval,
    @T@                    _value
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
d_interval_@S@_contains(
    struct dj_interval_@S@ _interval,
    @T@                    _value
)
{
    // outside the bounds is outside the interval
    if (!d_interval_@S@_contains_in_range(_interval, _value))
    {
        return false;
    }

    // a discrete member is one of the computed members
    if (_interval.step > (@T@)0)
    {
        const @T@ low = d_interval_@S@_inclusive_lower(_interval);

        return ( d_internal_interval_@S@_member(
                     low,
                     _interval.step,
                     d_internal_interval_@S@_snap(low,
                                                  _interval.step,
                                                  _value)) == _value );
    }

    return true;
}

D_MATH_FN @T@
d_interval_@S@_clamp(
    struct dj_interval_@S@ _interval,
    @T@                    _value
)
{
    const @T@ low  = d_interval_@S@_inclusive_lower(_interval);
    const @T@ high = d_interval_@S@_inclusive_upper(_interval);

    // NaN has no nearer member
    if (_value != _value)
    {
        return _value;
    }

    // below the interval: its first value
    if (_value < low)
    {
        return low;
    }

    // above the interval: its last value
    if (_value > high)
    {
        return d_interval_@S@_last(_interval);
    }

    // inside a discrete interval: down to a member
    if (_interval.step > (@T@)0)
    {
        return d_internal_interval_@S@_member(
                   low,
                   _interval.step,
                   d_internal_interval_@S@_snap(low, _interval.step, _value));
    }

    return _value;
}

D_MATH_FN @T@
d_interval_@S@_clamp_nearest(
    struct dj_interval_@S@ _interval,
    @T@                    _value
)
{
    // NaN, a continuous interval, and anything outside one: as clamp
    if ( (_value != _value) ||
         (!(_interval.step > (@T@)0)) ||
         (!d_interval_@S@_contains_in_range(_interval, _value)) )
    {
        return d_interval_@S@_clamp(_interval, _value);
    }

    const @T@ low = d_interval_@S@_inclusive_lower(_interval);
    const @T@ top = d_interval_@S@_last(_interval);

    // past the last member: the last member
    if ( (_value < low) ||
         (_value >= top) )
    {
        return d_interval_@S@_clamp(_interval, _value);
    }

    const dj_math_umax index =
        d_internal_interval_@S@_snap(low, _interval.step, _value);
    const @T@ below =
        d_internal_interval_@S@_member(low, _interval.step, index);
    const @T@ above =
        d_internal_interval_@S@_member(low, _interval.step, index + 1u);

    // the member above is nearer, and still inside the interval
    if ( (above <= top) &&
         ((_value - below) > (above - _value)) )
    {
        return above;
    }

    return below;
}

D_MATH_FN dj_math_umax
d_interval_@S@_index_of(
    struct dj_interval_@S@ _interval,
    @T@                    _value
)
{
    // a value that is not a member has the index one past the last
    if (!d_interval_@S@_contains(_interval, _value))
    {
        return d_interval_@S@_count(_interval);
    }

    {
        const @T@ low = d_interval_@S@_inclusive_lower(_interval);

        return (_interval.step > (@T@)0)
                   ? d_internal_interval_@S@_snap(low, _interval.step, _value)
                   : d_math_@S@_steps(low, _value);
    }
}

// @SECTION@.6    Relations
//------------------------------------------------------------------------------
// as for the integer families: overlaps compares the inclusive ranges,
// ignoring strides; overlaps_continuous the real line; intersects asks for
// a shared member - for two discrete intervals by testing the members of
// the sparser one within the ranges' common part, so its cost is the count
// of those.

D_MATH_FN bool
d_interval_@S@_overlaps(
    struct dj_interval_@S@ _left,
    struct dj_interval_@S@ _right
)
{
    // an empty interval shares a position with nothing
    if ( (d_interval_@S@_is_empty(_left)) ||
         (d_interval_@S@_is_empty(_right)) )
    {
        return false;
    }

    return !( (d_interval_@S@_inclusive_upper(_left) <
               d_interval_@S@_inclusive_lower(_right)) ||
              (d_interval_@S@_inclusive_lower(_left) >
               d_interval_@S@_inclusive_upper(_right)) );
}

D_MATH_FN bool
d_interval_@S@_overlaps_continuous(
    struct dj_interval_@S@ _left,
    struct dj_interval_@S@ _right
)
{
    // an interval empty on the real line meets nothing
    if ( (d_internal_interval_@S@_is_void(_left)) ||
         (d_internal_interval_@S@_is_void(_right)) )
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
d_interval_@S@_intersects(
    struct dj_interval_@S@ _left,
    struct dj_interval_@S@ _right
)
{
    // two continuous intervals share a value where their ranges meet
    if ( (!(_left.step > (@T@)0)) &&
         (!(_right.step > (@T@)0)) )
    {
        return d_interval_@S@_overlaps(_left, _right);
    }

    // an empty interval has no member to share
    if ( (d_interval_@S@_is_empty(_left)) ||
         (d_interval_@S@_is_empty(_right)) )
    {
        return false;
    }

    {
        // walk the sparser discrete one; test each member in the other
        const bool                   left_walks =
            ( (_left.step > (@T@)0) &&
              ( (!(_right.step > (@T@)0)) ||
                (_left.step >= _right.step) ) );
        const struct dj_interval_@S@ walk  = left_walks ? _left  : _right;
        const struct dj_interval_@S@ other = left_walks ? _right : _left;
        const @T@ first = d_interval_@S@_inclusive_lower(walk);
        const @T@ low   = (first > d_interval_@S@_inclusive_lower(other))
                              ? first
                              : d_interval_@S@_inclusive_lower(other);
        const @T@ high  = (d_interval_@S@_last(walk) <
                           d_interval_@S@_inclusive_upper(other))
                              ? d_interval_@S@_last(walk)
                              : d_interval_@S@_inclusive_upper(other);
        const dj_math_umax total = d_interval_@S@_count(walk);
        dj_math_umax index = d_internal_interval_@S@_snap(first, walk.step, low);

        // the first member at or above the common part's start
        if (d_internal_interval_@S@_member(first, walk.step, index) < low)
        {
            ++index;
        }

        // every member within the common part, until one is shared
        while ( (index < total) &&
                (d_internal_interval_@S@_member(first, walk.step, index) <=
                 high) )
        {
            if (d_interval_@S@_contains(
                    other,
                    d_internal_interval_@S@_member(first, walk.step, index)))
            {
                return true;
            }

            ++index;
        }

        return false;
    }
}

// @SECTION@.7    Normalization
//------------------------------------------------------------------------------
// as for the integer families, the terms in long double.

D_MATH_FN struct dj_math_ratio
d_internal_interval_@S@_terms(
    @T@ _origin,
    @T@ _end,
    @T@ _value
)
{
    struct dj_math_ratio result = { 0.0L, 1.0L };

    // a degenerate span maps everything to 0
    if (_end == _origin)
    {
        return result;
    }

    result.numerator   = (long double)_value - (long double)_origin;
    result.denominator = (long double)_end   - (long double)_origin;

    return result;
}

D_MATH_FN struct dj_math_ratio
d_interval_@S@_normalize_terms(
    struct dj_interval_@S@ _interval,
    @T@                    _value
)
{
    return d_internal_interval_@S@_terms(_interval.lower,
                                         _interval.upper,
                                         _value);
}

D_MATH_FN struct dj_math_ratio
d_interval_@S@_normalize_effective_terms(
    struct dj_interval_@S@ _interval,
    @T@                    _value
)
{
    return d_internal_interval_@S@_terms(
               d_interval_@S@_inclusive_lower(_interval),
               d_interval_@S@_inclusive_upper(_interval),
               _value);
}

D_MATH_FN struct dj_math_ratio
d_interval_@S@_normalize_discrete_terms(
    struct dj_interval_@S@ _interval,
    @T@                    _value
)
{
    // a continuous interval normalizes over its inclusive bounds
    if (!(_interval.step > (@T@)0))
    {
        return d_interval_@S@_normalize_effective_terms(_interval, _value);
    }

    struct dj_math_ratio result = { 0.0L, 1.0L };
    const dj_math_umax   count  = d_interval_@S@_count(_interval);
    const @T@            low    =
        d_interval_@S@_inclusive_lower(_interval);

    // one member or none: nothing to spread over
    if (count <= 1u)
    {
        return result;
    }

    // at or above the first member: the index of the member at or below;
    // below it, the quotient truncated toward zero, a zero kept +0
    if (_value >= low)
    {
        result.numerator = (long double)d_internal_interval_@S@_snap(
                               low,
                               _interval.step,
                               _value);
    }
    else
    {
        const long double back = ((long double)low - (long double)_value) /
                                 (long double)_interval.step;
        const dj_math_umax whole =
            (back < (long double)D_MATH_UMAX_MAX) ? (dj_math_umax)back
                                                  : D_MATH_UMAX_MAX;

        result.numerator = (whole == 0u) ? 0.0L : -(long double)whole;
    }

    result.denominator = (long double)(count - 1u);

    return result;
}

// @SECTION@.8    Formatting
//------------------------------------------------------------------------------

// d_internal_interval_@S@_put_value
//   writes _value at _position as the fewest significant digits (from
// @DIG@) that read back as the same value - 0.1, not 0.10000000000000001 -
// and returns the position after it.
D_MATH_FN_RT size_t
d_internal_interval_@S@_put_value(
    char*  _buffer,
    size_t _capacity,
    size_t _position,
    @T@    _value
)
{
    char   text[64] = { 0 };
    int    digits   = @DIG@;
    size_t i        = 0u;
    size_t position = _position;

    // the shortest round trip; NaN never compares equal, and keeps the last
    for (digits = @DIG@; digits <= (@DIG@ + 3); ++digits)
    {
        @T@ back = (@T@)0;

        (void)sprintf(text, "%.*@PRINTF@g", digits, (@PRINTCAST@)_value);

        if ( (sscanf(text, "%@SCANF@g", &back) == 1) &&
             (back == _value) )
        {
            break;
        }
    }

    for (i = 0u; text[i] != '\0'; ++i)
    {
        position = d_internal_interval_put_char(_buffer,
                                                _capacity,
                                                position,
                                                text[i]);
    }

    return position;
}

/**
 * @brief Writes an interval of the @NAME@ family as text, as
 *        d_interval_imax_format does, each value in the fewest digits that
 *        read back as it.
 *
 * @param[out] _buffer    where the text goes; may be NULL if _capacity is 0.
 * @param[in]  _capacity  the bytes _buffer holds.
 * @param[in]  _interval  the interval to write.
 * @return the length of the whole text, not counting the terminator.
 */
D_MATH_FN_RT size_t
d_interval_@S@_format(
    char*                  _buffer,
    size_t                 _capacity,
    struct dj_interval_@S@ _interval
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

    at = d_internal_interval_@S@_put_value(_buffer,
                                           _capacity,
                                           at,
                                           _interval.lower);

    // a discrete interval shows its stride between its bounds
    if (_interval.step > (@T@)0)
    {
        at = d_internal_interval_put_char(_buffer, _capacity, at, ':');
        at = d_internal_interval_@S@_put_value(_buffer,
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

    at = d_internal_interval_@S@_put_value(_buffer,
                                           _capacity,
                                           at,
                                           _interval.upper);
    at = d_internal_interval_put_char(_buffer, _capacity, at, right);

    return d_internal_interval_finish(_buffer, _capacity, at);
}
"""

INTERVAL_FLOAT_HEAD = r"""
/*
TABLE OF CONTENTS
=================
1.  THE FLOAT FAMILY (_f)
    ---------------------
    1.  Layout
         1.  dj_interval_f
    2.  Internal helpers
    3.  Construction, validity and bounds
    4.  Size and access
    5.  Membership and clamping
    6.  Relations
    7.  Normalization
    8.  Formatting
2.  THE DOUBLE FAMILY (_d)
    ----------------------
    1.  Layout
         1.  dj_interval_d
    2.  Internal helpers
    3.  Construction, validity and bounds
    4.  Size and access
    5.  Membership and clamping
    6.  Relations
    7.  Normalization
    8.  Formatting
3.  THE LONG DOUBLE FAMILY (_ld)
    ----------------------------
    1.  Layout
         1.  dj_interval_ld
    2.  Internal helpers
    3.  Construction, validity and bounds
    4.  Size and access
    5.  Membership and clamping
    6.  Relations
    7.  Normalization
    8.  Formatting
4.  ONE NAME FOR EVERY FAMILY (C++)
    -------------------------------
*/

#ifndef DJINTERP_C_MATH_INTERVAL_FLOAT_H
#define DJINTERP_C_MATH_INTERVAL_FLOAT_H 1

// std
#include <stddef.h>          // size_t, NULL
#include <stdio.h>           // sprintf, sscanf
// djinterp
#include "./float_order.h"   // d_math_*_next_up, _next_down, _steps, _advance
#include "./interval.h"      // dj_interval_bound, the formatting helpers
#include "./math_common.h"   // D_MATH_FN, dj_math_umax, dj_math_ratio
"""

INTERVAL_FLOAT_TAIL = r"""

#endif  // DJINTERP_C_MATH_INTERVAL_FLOAT_H
"""


def interval_float():
    lines = [
        "Floating-point intervals: the C core of math/interval for float,",
        "double and long double.",
        "  GENERATED by scripts/gen_c_math_float.py from one text for the three",
        "families: edit the generator, not this file.",
        "",
        "  An interval's members are the values of its type within its bounds,",
        "as for the integer families: here every representable value, so an open",
        "bound moves to the neighbouring value (float_order.h), count is how many",
        "values lie between the inclusive bounds, at and index_of walk them, and",
        "clamp lands inside an open bound on the nearest value it holds. A",
        "discrete interval's members are first + k * step, the product formed",
        "apart from the sum so no compiler fuses the two and the members are the",
        "same everywhere. An infinite first member or step leaves one member, the",
        "first; members past the largest finite value are one member, +inf. NaN",
        "is in no interval.",
        "Every kernel but formatting is pure arithmetic, so constexpr from C++14.",
    ]
    text = banner("interval_float.h", lines, DATE, "2026.11.21")
    text += "\n" + INTERVAL_FLOAT_HEAD
    for i, (suffix, ctype, prefix, prose) in enumerate(FAMILIES):
        upper = prose.upper()
        body = INTERVAL_FLOAT_TEMPLATE.replace("@SECTION@", str(i + 1)).replace("@UPPER@", upper)
        body = body.replace("@PRINTCAST@", "long double" if ctype == "long double" else "double")
        text += instantiate(body, suffix, ctype, prefix, prose)
    text += overload_section("4.  ONE NAME FOR EVERY FAMILY (C++)", INTERVAL_FLOAT_OVERLOADS)
    text += INTERVAL_FLOAT_TAIL
    return text
