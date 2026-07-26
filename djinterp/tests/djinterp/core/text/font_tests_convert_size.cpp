// djinterp [test]  font_tests_convert_size.cpp
//   Section 9d: fn_convert_size - the only function in the header with real
// branching, and therefore the only one where "100% coverage" means
// something arithmetic rather than structural.
//
//   The shape of the function is: an identity short-circuit, a switch that
// normalises the source unit to POINTS, and a switch that projects points
// onto the target unit.  Every arm of both switches is walked below, both
// guard arms of the `_em_pts != 0` test are walked, and the two
// device_units arms - which are EARLY RETURNS that bypass the arithmetic
// entirely - get tests of their own, because they are the two places the
// function deliberately does not convert at all.
//
//   The trailing `return _value` after the second switch is unreachable:
// both switches enumerate every font_size_unit and the second one returns
// from each arm.  Reaching it would require passing a value outside the
// enum, which is undefined behaviour, so it is not exercised.

// djinterp
#include "font_tests.hpp"


NS_DJINTERP
NS_TESTING

namespace
{
    // the five units, in enumeration order - the sweep basis
    constexpr font_size_unit k_units[5] =
    {
        font_size_unit::points,
        font_size_unit::pixels,
        font_size_unit::em,
        font_size_unit::percent,
        font_size_unit::device_units
    };
}

/*
tests_convert_identity_for_every_unit
  The `_from == _to` short-circuit, walked for all five units.  It returns
BEFORE the arithmetic, so it is exact even for values the round-trip would
perturb, and it holds whatever the dpi and em size are.
  Tests the following:
  - each of the five units converts to itself unchanged
  - the identity holds for odd values, zero, and negatives
  - the identity holds regardless of dpi / em_pts, including degenerate ones
*/
bool
tests_convert_identity_for_every_unit()
{
    // every unit, to itself
    for (std::size_t i = 0; i < 5u; ++i)
    {
        D_FT_CHECK_NEAR(fn_convert_size(13.7f, k_units[i], k_units[i]), 13.7f);
        D_FT_CHECK_NEAR(fn_convert_size(0.0f,  k_units[i], k_units[i]), 0.0f);
        D_FT_CHECK_NEAR(fn_convert_size(-5.5f, k_units[i], k_units[i]), -5.5f);
    }

    // the short-circuit fires before dpi / em_pts are ever consulted, so
    // even a zero em size cannot disturb it
    D_FT_CHECK_NEAR(fn_convert_size(12.0f, font_size_unit::em,
                                    font_size_unit::em, 96.0f, 0.0f), 12.0f);
    D_FT_CHECK_NEAR(fn_convert_size(12.0f, font_size_unit::pixels,
                                    font_size_unit::pixels, 1.0f, 1.0f), 12.0f);

    return true;
}

/*
tests_convert_points_to_pixels
  Tests the following:
  - 12pt at 96 dpi is 16px (12 * 96 / 72)
  - the relation is linear
*/
bool
tests_convert_points_to_pixels()
{
    D_FT_CHECK_NEAR(fn_convert_size(12.0f, font_size_unit::points,
                                    font_size_unit::pixels), 16.0f);
    D_FT_CHECK_NEAR(fn_convert_size(9.0f, font_size_unit::points,
                                    font_size_unit::pixels), 12.0f);
    D_FT_CHECK_NEAR(fn_convert_size(0.0f, font_size_unit::points,
                                    font_size_unit::pixels), 0.0f);

    // linear: doubling the points doubles the pixels
    const float one  = fn_convert_size(10.0f, font_size_unit::points,
                                       font_size_unit::pixels);
    const float two  = fn_convert_size(20.0f, font_size_unit::points,
                                       font_size_unit::pixels);

    D_FT_CHECK_NEAR(two, (one * 2.0f));

    return true;
}

/*
tests_convert_pixels_to_points
  Tests the following:
  - 16px at 96 dpi is 12pt (16 * 72 / 96)
  - it is the exact inverse of points -> pixels
*/
bool
tests_convert_pixels_to_points()
{
    D_FT_CHECK_NEAR(fn_convert_size(16.0f, font_size_unit::pixels,
                                    font_size_unit::points), 12.0f);
    D_FT_CHECK_NEAR(fn_convert_size(12.0f, font_size_unit::pixels,
                                    font_size_unit::points), 9.0f);

    // inverse of the forward direction
    const float px = fn_convert_size(14.0f, font_size_unit::points,
                                     font_size_unit::pixels);
    const float pt = fn_convert_size(px, font_size_unit::pixels,
                                     font_size_unit::points);

    D_FT_CHECK_NEAR(pt, 14.0f);

    return true;
}

/*
tests_convert_points_to_pixels_honours_the_dpi
  The dpi argument is the whole reason this function exists - a point is an
absolute length and a pixel is not.
  Tests the following:
  - at 72 dpi a point IS a pixel (the identity dpi)
  - at 144 dpi the pixel count doubles
  - at 300 dpi 12pt is 50px
  - the default dpi really is 96
*/
bool
tests_convert_points_to_pixels_honours_the_dpi()
{
    // 72 dpi: 1pt == 1px
    D_FT_CHECK_NEAR(fn_convert_size(12.0f, font_size_unit::points,
                                    font_size_unit::pixels, 72.0f), 12.0f);

    // 144 dpi: double
    D_FT_CHECK_NEAR(fn_convert_size(12.0f, font_size_unit::points,
                                    font_size_unit::pixels, 144.0f), 24.0f);

    // 300 dpi: 12 * 300 / 72
    D_FT_CHECK_NEAR(fn_convert_size(12.0f, font_size_unit::points,
                                    font_size_unit::pixels, 300.0f), 50.0f);

    // the default matches an explicit 96
    D_FT_CHECK_NEAR(fn_convert_size(12.0f, font_size_unit::points,
                                    font_size_unit::pixels),
                    fn_convert_size(12.0f, font_size_unit::points,
                                    font_size_unit::pixels, 96.0f));

    return true;
}

/*
tests_convert_pixels_to_points_honours_the_dpi
  Tests the following:
  - the reverse direction scales inversely with dpi
  - a round trip through any dpi returns the original
*/
bool
tests_convert_pixels_to_points_honours_the_dpi()
{
    D_FT_CHECK_NEAR(fn_convert_size(12.0f, font_size_unit::pixels,
                                    font_size_unit::points, 72.0f), 12.0f);
    D_FT_CHECK_NEAR(fn_convert_size(24.0f, font_size_unit::pixels,
                                    font_size_unit::points, 144.0f), 12.0f);
    D_FT_CHECK_NEAR(fn_convert_size(50.0f, font_size_unit::pixels,
                                    font_size_unit::points, 300.0f), 12.0f);

    // round trip at a few dpi
    const float dpis[] = { 72.0f, 96.0f, 120.0f, 144.0f, 300.0f };

    for (std::size_t i = 0; i < 5u; ++i)
    {
        const float px = fn_convert_size(11.0f, font_size_unit::points,
                                         font_size_unit::pixels, dpis[i]);
        const float pt = fn_convert_size(px, font_size_unit::pixels,
                                         font_size_unit::points, dpis[i]);

        D_FT_CHECK_NEAR(pt, 11.0f);
    }

    return true;
}

/*
tests_convert_em_to_points
  Tests the following:
  - an em is _em_pts points, so 1.5em at a 16pt em is 24pt
  - the default em size is 10pt
*/
bool
tests_convert_em_to_points()
{
    D_FT_CHECK_NEAR(fn_convert_size(1.5f, font_size_unit::em,
                                    font_size_unit::points, 96.0f, 16.0f),
                    24.0f);
    D_FT_CHECK_NEAR(fn_convert_size(2.0f, font_size_unit::em,
                                    font_size_unit::points, 96.0f, 12.0f),
                    24.0f);

    // the default em anchor is 10pt
    D_FT_CHECK_NEAR(fn_convert_size(1.2f, font_size_unit::em,
                                    font_size_unit::points), 12.0f);
    D_FT_CHECK_NEAR(fn_convert_size(1.2f, font_size_unit::em,
                                    font_size_unit::points, 96.0f, 10.0f),
                    12.0f);

    return true;
}

/*
tests_convert_points_to_em
  Tests the following:
  - 24pt at a 16pt em is 1.5em
  - it inverts em -> points
*/
bool
tests_convert_points_to_em()
{
    D_FT_CHECK_NEAR(fn_convert_size(24.0f, font_size_unit::points,
                                    font_size_unit::em, 96.0f, 16.0f), 1.5f);
    D_FT_CHECK_NEAR(fn_convert_size(12.0f, font_size_unit::points,
                                    font_size_unit::em), 1.2f);

    // inverse
    const float em = fn_convert_size(18.0f, font_size_unit::points,
                                     font_size_unit::em, 96.0f, 12.0f);
    const float pt = fn_convert_size(em, font_size_unit::em,
                                     font_size_unit::points, 96.0f, 12.0f);

    D_FT_CHECK_NEAR(pt, 18.0f);

    return true;
}

/*
tests_convert_percent_to_points
  percent is em scaled by 100: value * 0.01 * em_pts.
  Tests the following:
  - 150% of a 10pt em is 15pt
  - 100% is exactly one em
  - 0% is 0pt
*/
bool
tests_convert_percent_to_points()
{
    D_FT_CHECK_NEAR(fn_convert_size(150.0f, font_size_unit::percent,
                                    font_size_unit::points), 15.0f);
    D_FT_CHECK_NEAR(fn_convert_size(100.0f, font_size_unit::percent,
                                    font_size_unit::points), 10.0f);
    D_FT_CHECK_NEAR(fn_convert_size(0.0f, font_size_unit::percent,
                                    font_size_unit::points), 0.0f);

    // with a different em anchor
    D_FT_CHECK_NEAR(fn_convert_size(50.0f, font_size_unit::percent,
                                    font_size_unit::points, 96.0f, 16.0f),
                    8.0f);

    return true;
}

/*
tests_convert_points_to_percent
  Tests the following:
  - 15pt against a 10pt em is 150%
  - one em is 100%
  - it inverts percent -> points
*/
bool
tests_convert_points_to_percent()
{
    D_FT_CHECK_NEAR(fn_convert_size(15.0f, font_size_unit::points,
                                    font_size_unit::percent), 150.0f);
    D_FT_CHECK_NEAR(fn_convert_size(10.0f, font_size_unit::points,
                                    font_size_unit::percent), 100.0f);
    D_FT_CHECK_NEAR(fn_convert_size(16.0f, font_size_unit::points,
                                    font_size_unit::percent, 96.0f, 16.0f),
                    100.0f);

    // inverse
    const float pct = fn_convert_size(13.0f, font_size_unit::points,
                                      font_size_unit::percent);
    const float pt  = fn_convert_size(pct, font_size_unit::percent,
                                      font_size_unit::points);

    D_FT_CHECK_NEAR(pt, 13.0f);

    return true;
}

/*
tests_convert_em_to_percent
  Both are em-relative, so this is a pure x100 - and, notably, the em anchor
cancels out entirely.
  Tests the following:
  - 1.5em is 150%
  - 1em is 100%
  - the result does not depend on the em anchor (it cancels)
*/
bool
tests_convert_em_to_percent()
{
    D_FT_CHECK_NEAR(fn_convert_size(1.5f, font_size_unit::em,
                                    font_size_unit::percent), 150.0f);
    D_FT_CHECK_NEAR(fn_convert_size(1.0f, font_size_unit::em,
                                    font_size_unit::percent), 100.0f);

    // the anchor cancels: em -> pts -> em is (v * e) / e
    D_FT_CHECK_NEAR(fn_convert_size(1.5f, font_size_unit::em,
                                    font_size_unit::percent, 96.0f, 16.0f),
                    150.0f);
    D_FT_CHECK_NEAR(fn_convert_size(1.5f, font_size_unit::em,
                                    font_size_unit::percent, 96.0f, 7.5f),
                    150.0f);

    return true;
}

/*
tests_convert_percent_to_em
  Tests the following:
  - 150% is 1.5em
  - the em anchor cancels here too
*/
bool
tests_convert_percent_to_em()
{
    D_FT_CHECK_NEAR(fn_convert_size(150.0f, font_size_unit::percent,
                                    font_size_unit::em), 1.5f);
    D_FT_CHECK_NEAR(fn_convert_size(100.0f, font_size_unit::percent,
                                    font_size_unit::em), 1.0f);

    D_FT_CHECK_NEAR(fn_convert_size(150.0f, font_size_unit::percent,
                                    font_size_unit::em, 96.0f, 16.0f), 1.5f);

    return true;
}

/*
tests_convert_em_to_pixels
  Two hops through the points pivot: em -> points -> pixels.  Both the em
anchor AND the dpi are in play.
  Tests the following:
  - 1em at a 12pt em and 96 dpi is 16px
  - both arguments matter
*/
bool
tests_convert_em_to_pixels()
{
    D_FT_CHECK_NEAR(fn_convert_size(1.0f, font_size_unit::em,
                                    font_size_unit::pixels, 96.0f, 12.0f),
                    16.0f);

    // 2em at a 9pt em == 18pt == 24px at 96 dpi
    D_FT_CHECK_NEAR(fn_convert_size(2.0f, font_size_unit::em,
                                    font_size_unit::pixels, 96.0f, 9.0f),
                    24.0f);

    // the dpi matters too: the same em at 72 dpi is 12px
    D_FT_CHECK_NEAR(fn_convert_size(1.0f, font_size_unit::em,
                                    font_size_unit::pixels, 72.0f, 12.0f),
                    12.0f);

    return true;
}

/*
tests_convert_pixels_to_em
  Tests the following:
  - 16px at 96 dpi against a 12pt em is 1em
  - it inverts em -> pixels
*/
bool
tests_convert_pixels_to_em()
{
    D_FT_CHECK_NEAR(fn_convert_size(16.0f, font_size_unit::pixels,
                                    font_size_unit::em, 96.0f, 12.0f), 1.0f);

    // inverse
    const float px = fn_convert_size(1.75f, font_size_unit::em,
                                     font_size_unit::pixels, 120.0f, 14.0f);
    const float em = fn_convert_size(px, font_size_unit::pixels,
                                     font_size_unit::em, 120.0f, 14.0f);

    D_FT_CHECK_NEAR(em, 1.75f);

    return true;
}

/*
tests_convert_percent_to_pixels
  Tests the following:
  - 150% of a 12pt em at 96 dpi is 24px (18pt -> 24px)
*/
bool
tests_convert_percent_to_pixels()
{
    D_FT_CHECK_NEAR(fn_convert_size(150.0f, font_size_unit::percent,
                                    font_size_unit::pixels, 96.0f, 12.0f),
                    24.0f);

    // 100% of a 9pt em at 96 dpi is 12px
    D_FT_CHECK_NEAR(fn_convert_size(100.0f, font_size_unit::percent,
                                    font_size_unit::pixels, 96.0f, 9.0f),
                    12.0f);

    return true;
}

/*
tests_convert_pixels_to_percent
  Tests the following:
  - 24px at 96 dpi against a 12pt em is 150%
  - it inverts percent -> pixels
*/
bool
tests_convert_pixels_to_percent()
{
    D_FT_CHECK_NEAR(fn_convert_size(24.0f, font_size_unit::pixels,
                                    font_size_unit::percent, 96.0f, 12.0f),
                    150.0f);

    // inverse
    const float px  = fn_convert_size(125.0f, font_size_unit::percent,
                                      font_size_unit::pixels, 96.0f, 16.0f);
    const float pct = fn_convert_size(px, font_size_unit::pixels,
                                      font_size_unit::percent, 96.0f, 16.0f);

    D_FT_CHECK_NEAR(pct, 125.0f);

    return true;
}

/*
tests_convert_device_units_as_source_is_opaque
  device_units means "already resolved" - the value is in the surface's own
coordinate system and the function has no idea what that is.  So the SOURCE
switch returns immediately, bypassing the second switch entirely: converting
FROM device_units is always a no-op, whatever the target.
  Tests the following:
  - device -> points returns the value UNCHANGED (it is not scaled)
  - device -> pixels, em, and percent likewise
  - no combination of dpi and em size changes that
*/
bool
tests_convert_device_units_as_source_is_opaque()
{
    const float v = 42.0f;

    D_FT_CHECK_NEAR(fn_convert_size(v, font_size_unit::device_units,
                                    font_size_unit::points), v);
    D_FT_CHECK_NEAR(fn_convert_size(v, font_size_unit::device_units,
                                    font_size_unit::pixels), v);
    D_FT_CHECK_NEAR(fn_convert_size(v, font_size_unit::device_units,
                                    font_size_unit::em), v);
    D_FT_CHECK_NEAR(fn_convert_size(v, font_size_unit::device_units,
                                    font_size_unit::percent), v);

    // and the dpi / em arguments are simply never consulted
    D_FT_CHECK_NEAR(fn_convert_size(v, font_size_unit::device_units,
                                    font_size_unit::pixels, 300.0f, 72.0f), v);
    D_FT_CHECK_NEAR(fn_convert_size(v, font_size_unit::device_units,
                                    font_size_unit::em, 1.0f, 1.0f), v);

    return true;
}

/*
tests_convert_device_units_as_target_is_opaque
  The mirror image, and the subtler of the two: the TARGET switch's
device_units arm returns `_value`, not `pts`.  So converting pixels to
device_units hands back the original PIXEL number, not the points it was
normalised to along the way.  The function is deliberately a no-op in this
direction rather than half-converting into a space it cannot reason about.
  Tests the following:
  - points -> device returns the point value unchanged
  - pixels -> device returns the PIXEL value, not the intermediate points
  - em -> device returns the em value, not the points
  - percent -> device likewise
*/
bool
tests_convert_device_units_as_target_is_opaque()
{
    // points -> device: unchanged
    D_FT_CHECK_NEAR(fn_convert_size(12.0f, font_size_unit::points,
                                    font_size_unit::device_units), 12.0f);

    // pixels -> device: the ORIGINAL 16, not the 12 points it normalised to
    D_FT_CHECK_NEAR(fn_convert_size(16.0f, font_size_unit::pixels,
                                    font_size_unit::device_units), 16.0f);

    const float via_points = fn_convert_size(16.0f, font_size_unit::pixels,
                                             font_size_unit::points);

    D_FT_CHECK_NEAR(via_points, 12.0f);
    D_FT_CHECK(!approx_eq(fn_convert_size(16.0f, font_size_unit::pixels,
                                          font_size_unit::device_units),
                          via_points));

    // em -> device: the em number, not its points
    D_FT_CHECK_NEAR(fn_convert_size(1.5f, font_size_unit::em,
                                    font_size_unit::device_units, 96.0f,
                                    16.0f), 1.5f);

    // percent -> device: the percentage, not its points
    D_FT_CHECK_NEAR(fn_convert_size(150.0f, font_size_unit::percent,
                                    font_size_unit::device_units), 150.0f);

    return true;
}

/*
tests_convert_zero_em_pts_guards_the_em_target
  The `(_em_pts != 0.0f)` guard on the em arm of the target switch.  With no
em anchor there is nothing to divide by, so the function returns the ORIGINAL
value rather than an infinity - the header's stated policy of "unsupported
conversions are no-ops rather than hard errors, so it can sit on the hot path
without branching traps".
  Tests the following:
  - points -> em with em_pts == 0 returns the input unchanged
  - pixels -> em with em_pts == 0 returns the input unchanged - note this is
    the ORIGINAL pixel value, not the points it was normalised to
  - a nonzero em anchor takes the other arm and does convert
*/
bool
tests_convert_zero_em_pts_guards_the_em_target()
{
    // the guarded arm: return _value
    D_FT_CHECK_NEAR(fn_convert_size(24.0f, font_size_unit::points,
                                    font_size_unit::em, 96.0f, 0.0f), 24.0f);

    // and the value returned is the ORIGINAL, not the normalised points
    D_FT_CHECK_NEAR(fn_convert_size(16.0f, font_size_unit::pixels,
                                    font_size_unit::em, 96.0f, 0.0f), 16.0f);

    // the unguarded arm still works
    D_FT_CHECK_NEAR(fn_convert_size(24.0f, font_size_unit::points,
                                    font_size_unit::em, 96.0f, 16.0f), 1.5f);

    return true;
}

/*
tests_convert_zero_em_pts_guards_the_percent_target
  The same guard on the percent arm.
  Tests the following:
  - points -> percent with em_pts == 0 returns the input unchanged
  - pixels -> percent with em_pts == 0 returns the ORIGINAL pixel value
  - a nonzero em anchor converts normally
*/
bool
tests_convert_zero_em_pts_guards_the_percent_target()
{
    D_FT_CHECK_NEAR(fn_convert_size(15.0f, font_size_unit::points,
                                    font_size_unit::percent, 96.0f, 0.0f),
                    15.0f);

    D_FT_CHECK_NEAR(fn_convert_size(16.0f, font_size_unit::pixels,
                                    font_size_unit::percent, 96.0f, 0.0f),
                    16.0f);

    // the unguarded arm
    D_FT_CHECK_NEAR(fn_convert_size(15.0f, font_size_unit::points,
                                    font_size_unit::percent, 96.0f, 10.0f),
                    150.0f);

    return true;
}

/*
tests_convert_zero_em_pts_is_unguarded_on_the_source_side
  The asymmetry.  The guard protects the two DIVISIONS by _em_pts in the
target switch - but the source switch MULTIPLIES by _em_pts, and a
multiplication by zero needs no guard to be safe.  It is, however, not a
no-op: an em (or percent) source with a zero em anchor normalises to ZERO
points, and the function then faithfully converts that zero.  So the caller
gets 0, not the original value - the opposite of what the target-side guard
does.  Worth pinning: the two sides of this function treat a missing em
anchor differently.
  Tests the following:
  - em -> points with em_pts == 0 yields 0, not the original
  - percent -> points with em_pts == 0 yields 0
  - em -> pixels with em_pts == 0 yields 0
  - contrast: points -> em with em_pts == 0 yields the ORIGINAL (guarded)
*/
bool
tests_convert_zero_em_pts_is_unguarded_on_the_source_side()
{
    // source side: multiplied by zero, so it collapses to zero
    D_FT_CHECK_NEAR(fn_convert_size(3.0f, font_size_unit::em,
                                    font_size_unit::points, 96.0f, 0.0f),
                    0.0f);
    D_FT_CHECK_NEAR(fn_convert_size(150.0f, font_size_unit::percent,
                                    font_size_unit::points, 96.0f, 0.0f),
                    0.0f);
    D_FT_CHECK_NEAR(fn_convert_size(3.0f, font_size_unit::em,
                                    font_size_unit::pixels, 96.0f, 0.0f),
                    0.0f);

    // target side: guarded, so it passes the original through
    D_FT_CHECK_NEAR(fn_convert_size(3.0f, font_size_unit::points,
                                    font_size_unit::em, 96.0f, 0.0f), 3.0f);

    // the two sides really do disagree
    D_FT_CHECK(!approx_eq(fn_convert_size(3.0f, font_size_unit::em,
                                          font_size_unit::points, 96.0f, 0.0f),
                          fn_convert_size(3.0f, font_size_unit::points,
                                          font_size_unit::em, 96.0f, 0.0f)));

    return true;
}

/*
tests_convert_round_trips
  Tests the following:
  - points <-> pixels round-trips at several dpi
  - points <-> em round-trips at several em anchors
  - points <-> percent round-trips
  - pixels <-> em round-trips (two hops each way)
*/
bool
tests_convert_round_trips()
{
    const float values[] = { 1.0f, 8.0f, 12.0f, 13.5f, 72.0f, 144.0f };

    // points <-> pixels
    for (std::size_t i = 0; i < 6u; ++i)
    {
        const float px = fn_convert_size(values[i], font_size_unit::points,
                                         font_size_unit::pixels, 144.0f);
        const float pt = fn_convert_size(px, font_size_unit::pixels,
                                         font_size_unit::points, 144.0f);

        D_FT_CHECK_NEAR(pt, values[i]);
    }

    // points <-> em
    for (std::size_t i = 0; i < 6u; ++i)
    {
        const float em = fn_convert_size(values[i], font_size_unit::points,
                                         font_size_unit::em, 96.0f, 16.0f);
        const float pt = fn_convert_size(em, font_size_unit::em,
                                         font_size_unit::points, 96.0f, 16.0f);

        D_FT_CHECK_NEAR(pt, values[i]);
    }

    // points <-> percent
    for (std::size_t i = 0; i < 6u; ++i)
    {
        const float pct = fn_convert_size(values[i], font_size_unit::points,
                                          font_size_unit::percent);
        const float pt  = fn_convert_size(pct, font_size_unit::percent,
                                          font_size_unit::points);

        D_FT_CHECK_NEAR(pt, values[i]);
    }

    // pixels <-> em, two hops in each direction
    for (std::size_t i = 0; i < 6u; ++i)
    {
        const float em = fn_convert_size(values[i], font_size_unit::pixels,
                                         font_size_unit::em, 120.0f, 14.0f);
        const float px = fn_convert_size(em, font_size_unit::em,
                                         font_size_unit::pixels, 120.0f, 14.0f);

        D_FT_CHECK_NEAR(px, values[i]);
    }

    return true;
}

/*
tests_convert_the_whole_unit_matrix
  All twenty-five (from, to) pairs, swept - so no arm of either switch can
be missing or mis-wired without this failing.  Two invariants are asserted
across the sweep:

    1. POINTS IS THE PIVOT.  The function normalises to points and then
       projects.  So for any A and B that are not device_units, converting
       A -> B directly must equal converting A -> points and then
       points -> B.  A swapped case label in either switch breaks this
       immediately.
    2. DEVICE_UNITS IS ABSORBING.  Any pair involving device_units on
       either side is the identity.

  Tests the following:
  - all 25 pairs are exercised
  - the pivot identity holds for all 12 non-device cross conversions
  - device_units absorbs on both sides, for all 9 pairs that touch it
  - every non-device pair is invertible
*/
bool
tests_convert_the_whole_unit_matrix()
{
    const float v = 12.0f;

    std::size_t pairs_seen = 0;

    for (std::size_t i = 0; i < 5u; ++i)
    {
        for (std::size_t j = 0; j < 5u; ++j)
        {
            const font_size_unit from = k_units[i];
            const font_size_unit to   = k_units[j];

            const float direct = fn_convert_size(v, from, to);

            ++pairs_seen;

            // identity on the diagonal
            if (from == to)
            {
                D_FT_CHECK_NEAR(direct, v);

                continue;
            }

            // device_units absorbs on either side
            if ( (from == font_size_unit::device_units) ||
                 (to   == font_size_unit::device_units) )
            {
                D_FT_CHECK_NEAR(direct, v);

                continue;
            }

            // the pivot identity: A -> B == (A -> points) -> B
            const float pts   = fn_convert_size(v, from,
                                                font_size_unit::points);
            const float piped = fn_convert_size(pts, font_size_unit::points,
                                                to);

            D_FT_CHECK_NEAR(direct, piped);

            // and every non-device conversion is invertible
            const float back = fn_convert_size(direct, to, from);

            D_FT_CHECK_NEAR(back, v);
        }
    }

    D_FT_CHECK(pairs_seen == 25u);

    return true;
}

/*
tests_convert_zero_and_negative_values
  The function does no validation - it is arithmetic, and it stays
arithmetic.
  Tests the following:
  - zero converts to zero in every direction
  - a negative size converts as a negative size (the sign survives)
*/
bool
tests_convert_zero_and_negative_values()
{
    // zero is a fixed point of every conversion
    for (std::size_t i = 0; i < 5u; ++i)
    {
        for (std::size_t j = 0; j < 5u; ++j)
        {
            D_FT_CHECK_NEAR(fn_convert_size(0.0f, k_units[i], k_units[j]),
                            0.0f);
        }
    }

    // negatives are scaled, not clamped
    D_FT_CHECK_NEAR(fn_convert_size(-12.0f, font_size_unit::points,
                                    font_size_unit::pixels), -16.0f);
    D_FT_CHECK_NEAR(fn_convert_size(-16.0f, font_size_unit::pixels,
                                    font_size_unit::points), -12.0f);
    D_FT_CHECK(fn_convert_size(-1.5f, font_size_unit::em,
                               font_size_unit::points) < 0.0f);

    return true;
}

/*
tests_convert_default_arguments
  Tests the following:
  - the dpi defaults to 96
  - the em size defaults to 10 points
  - omitting both matches supplying them explicitly, for every unit pair
*/
bool
tests_convert_default_arguments()
{
    // spelled out, the defaults must agree with the omitted form
    for (std::size_t i = 0; i < 5u; ++i)
    {
        for (std::size_t j = 0; j < 5u; ++j)
        {
            const float omitted  = fn_convert_size(12.0f, k_units[i],
                                                   k_units[j]);
            const float explicit_ = fn_convert_size(12.0f, k_units[i],
                                                    k_units[j], 96.0f, 10.0f);

            D_FT_CHECK_NEAR(omitted, explicit_);
        }
    }

    // and the dpi default alone
    D_FT_CHECK_NEAR(fn_convert_size(12.0f, font_size_unit::points,
                                    font_size_unit::pixels),
                    fn_convert_size(12.0f, font_size_unit::points,
                                    font_size_unit::pixels, 96.0f));

    // 96 dpi really is what makes 12pt into 16px
    D_FT_CHECK_NEAR(fn_convert_size(12.0f, font_size_unit::points,
                                    font_size_unit::pixels), 16.0f);

    // a 10pt em really is what makes 1.2em into 12pt
    D_FT_CHECK_NEAR(fn_convert_size(1.2f, font_size_unit::em,
                                    font_size_unit::points), 12.0f);

    return true;
}

/*
tests_convert_is_constexpr_and_noexcept
  The function is meant to fold at compile time for the common case - a
layout constant should cost nothing at run time.
  Tests the following:
  - it folds in a static_assert, for the identity, a scaling conversion, an
    opaque device_units conversion, and a guarded zero-em conversion
  - it is noexcept
  - the folded results are EXACT where the arithmetic is exact (12pt at
    96dpi really is 16.0f, not 15.999999)
*/
bool
tests_convert_is_constexpr_and_noexcept()
{
    // exact, so exact equality is the right assertion here
    static_assert((fn_convert_size(12.0f, font_size_unit::points,
                                   font_size_unit::pixels) == 16.0f),
                  "12pt at 96dpi must fold to exactly 16px");
    static_assert((fn_convert_size(16.0f, font_size_unit::pixels,
                                   font_size_unit::points) == 12.0f),
                  "16px at 96dpi must fold to exactly 12pt");
    static_assert((fn_convert_size(7.0f, font_size_unit::em,
                                   font_size_unit::em) == 7.0f),
                  "the identity must fold");
    static_assert((fn_convert_size(42.0f, font_size_unit::device_units,
                                   font_size_unit::points) == 42.0f),
                  "device_units must fold to a no-op");
    static_assert((fn_convert_size(24.0f, font_size_unit::points,
                                   font_size_unit::em, 96.0f, 0.0f) == 24.0f),
                  "the zero-em guard must fold");
    static_assert(noexcept(fn_convert_size(12.0f, font_size_unit::points,
                                           font_size_unit::pixels)),
                  "fn_convert_size must be noexcept");

    constexpr float px = fn_convert_size(9.0f, font_size_unit::points,
                                         font_size_unit::pixels);

    D_FT_CHECK_NEAR(px, 12.0f);
    D_FT_CHECK(noexcept(fn_convert_size(12.0f, font_size_unit::points,
                                        font_size_unit::pixels)));
    D_FT_CHECK((std::is_same<decltype(fn_convert_size(1.0f,
                                                      font_size_unit::points,
                                                      font_size_unit::pixels)),
                             float>::value));

    return true;
}

NS_END  // testing
NS_END  // djinterp
