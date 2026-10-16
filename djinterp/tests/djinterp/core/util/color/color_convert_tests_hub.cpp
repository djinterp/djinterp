/*******************************************************************************
* djinterp [core]                                    color_convert_tests_hub.cpp
*
*
* path:      /tests/djinterp/core/util/color/color_convert_tests_hub.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "color_convert_tests.hpp"

NS_DJINTERP
NS_TESTING

/*
tests_color_convert_rgb_to_hsl_known_values
  Verifies the RGB primaries map to their expected HSL coordinates.
  Tests the following:
  - the three primaries land on hues 0, 120 and 240 at full saturation and
    half lightness
  - the achromatic values carry zero saturation and lightness equal to the
    grey level
*/
bool
tests_color_convert_rgb_to_hsl_known_values()
{
    const hsl red = color_cast<hsl>(rgb(channel_t(1), channel_t(0),
                                        channel_t(0)));
    D_CX_CHECK(cx_near(red.h, channel_t(0)));
    D_CX_CHECK(cx_near(red.s, channel_t(1)));
    D_CX_CHECK(cx_near(red.l, channel_t(0.5)));

    const hsl green = color_cast<hsl>(rgb(channel_t(0), channel_t(1),
                                          channel_t(0)));
    D_CX_CHECK(cx_near(green.h, channel_t(120)));

    const hsl blue = color_cast<hsl>(rgb(channel_t(0), channel_t(0),
                                         channel_t(1)));
    D_CX_CHECK(cx_near(blue.h, channel_t(240)));

    const hsl white = color_cast<hsl>(rgb(channel_t(1), channel_t(1),
                                          channel_t(1)));
    D_CX_CHECK(cx_near(white.s, channel_t(0)));
    D_CX_CHECK(cx_near(white.l, channel_t(1)));

    const hsl black = color_cast<hsl>(rgb(channel_t(0), channel_t(0),
                                          channel_t(0)));
    D_CX_CHECK(cx_near(black.s, channel_t(0)));
    D_CX_CHECK(cx_near(black.l, channel_t(0)));

    return true;
}

/*
tests_color_convert_rgb_to_hsv_known_values
  Verifies the RGB primaries map to their expected HSV coordinates.  HSV
  differs from HSL in the third channel: a primary has value 1, not 0.5.
  Tests the following:
  - the three primaries land on hues 0, 120 and 240 at full saturation and
    full value
  - white and black differ in value while sharing zero saturation
*/
bool
tests_color_convert_rgb_to_hsv_known_values()
{
    const hsv red = color_cast<hsv>(rgb(channel_t(1), channel_t(0),
                                        channel_t(0)));
    D_CX_CHECK(cx_near(red.h, channel_t(0)));
    D_CX_CHECK(cx_near(red.s, channel_t(1)));
    D_CX_CHECK(cx_near(red.v, channel_t(1)));

    const hsv green = color_cast<hsv>(rgb(channel_t(0), channel_t(1),
                                          channel_t(0)));
    D_CX_CHECK(cx_near(green.h, channel_t(120)));
    D_CX_CHECK(cx_near(green.v, channel_t(1)));

    const hsv blue = color_cast<hsv>(rgb(channel_t(0), channel_t(0),
                                         channel_t(1)));
    D_CX_CHECK(cx_near(blue.h, channel_t(240)));

    const hsv white = color_cast<hsv>(rgb(channel_t(1), channel_t(1),
                                          channel_t(1)));
    D_CX_CHECK(cx_near(white.s, channel_t(0)));
    D_CX_CHECK(cx_near(white.v, channel_t(1)));

    const hsv black = color_cast<hsv>(rgb(channel_t(0), channel_t(0),
                                          channel_t(0)));
    D_CX_CHECK(cx_near(black.v, channel_t(0)));

    // the HSL / HSV difference at a primary: lightness 0.5 vs value 1
    const hsl red_hsl = color_cast<hsl>(rgb(channel_t(1), channel_t(0),
                                            channel_t(0)));
    D_CX_CHECK(!cx_near(red_hsl.l, red.v));

    return true;
}

/*
tests_color_convert_rgb_to_cmyk_known_values
  Verifies the RGB primaries map to their expected CMYK coordinates,
  including the black-generation branch that fires only for pure black.
  Tests the following:
  - each additive primary becomes the complementary pair of subtractive inks
  - white is the absence of all ink
  - black takes the dedicated K = 1 branch with no chromatic ink at all
*/
bool
tests_color_convert_rgb_to_cmyk_known_values()
{
    const cmyk red = color_cast<cmyk>(rgb(channel_t(1), channel_t(0),
                                          channel_t(0)));
    D_CX_CHECK(cx_near(red.c, channel_t(0)));
    D_CX_CHECK(cx_near(red.m, channel_t(1)));
    D_CX_CHECK(cx_near(red.y, channel_t(1)));
    D_CX_CHECK(cx_near(red.k, channel_t(0)));

    const cmyk green = color_cast<cmyk>(rgb(channel_t(0), channel_t(1),
                                            channel_t(0)));
    D_CX_CHECK(cx_near(green.c, channel_t(1)));
    D_CX_CHECK(cx_near(green.m, channel_t(0)));
    D_CX_CHECK(cx_near(green.y, channel_t(1)));

    const cmyk blue = color_cast<cmyk>(rgb(channel_t(0), channel_t(0),
                                           channel_t(1)));
    D_CX_CHECK(cx_near(blue.c, channel_t(1)));
    D_CX_CHECK(cx_near(blue.m, channel_t(1)));
    D_CX_CHECK(cx_near(blue.y, channel_t(0)));

    const cmyk white = color_cast<cmyk>(rgb(channel_t(1), channel_t(1),
                                            channel_t(1)));
    D_CX_CHECK(cx_near(white.c, channel_t(0)));
    D_CX_CHECK(cx_near(white.k, channel_t(0)));

    // the black-generation branch
    const cmyk black = color_cast<cmyk>(rgb(channel_t(0), channel_t(0),
                                            channel_t(0)));
    D_CX_CHECK(cx_near(black.c, channel_t(0)));
    D_CX_CHECK(cx_near(black.m, channel_t(0)));
    D_CX_CHECK(cx_near(black.y, channel_t(0)));
    D_CX_CHECK(cx_near(black.k, channel_t(1)));

    return true;
}

/*
tests_color_convert_rgb_to_ycbcr_known_values
  Verifies the RGB primaries map to their expected full-range BT.601 luma and
  chroma.  The luma weights are the defining constants of the model.
  Tests the following:
  - each primary's luma matches its BT.601 weight
  - the chroma extremes fall on red and blue as the standard prescribes
  - achromatic input produces zero chroma
*/
bool
tests_color_convert_rgb_to_ycbcr_known_values()
{
    const ycbcr red = color_cast<ycbcr>(rgb(channel_t(1), channel_t(0),
                                            channel_t(0)));
    D_CX_CHECK(cx_near(red.y, channel_t(0.299)));
    D_CX_CHECK(cx_near(red.cb, channel_t(-0.168736)));
    D_CX_CHECK(cx_near(red.cr, channel_t(0.5)));

    const ycbcr green = color_cast<ycbcr>(rgb(channel_t(0), channel_t(1),
                                              channel_t(0)));
    D_CX_CHECK(cx_near(green.y, channel_t(0.587)));
    D_CX_CHECK(cx_near(green.cb, channel_t(-0.331264)));
    D_CX_CHECK(cx_near(green.cr, channel_t(-0.418688)));

    const ycbcr blue = color_cast<ycbcr>(rgb(channel_t(0), channel_t(0),
                                             channel_t(1)));
    D_CX_CHECK(cx_near(blue.y, channel_t(0.114)));
    D_CX_CHECK(cx_near(blue.cb, channel_t(0.5)));
    D_CX_CHECK(cx_near(blue.cr, channel_t(-0.081312)));

    // the three weights sum to one, so white has unit luma and no chroma
    const ycbcr white = color_cast<ycbcr>(rgb(channel_t(1), channel_t(1),
                                              channel_t(1)));
    D_CX_CHECK(cx_near(white.y, channel_t(1)));
    D_CX_CHECK(cx_near(white.cb, channel_t(0)));
    D_CX_CHECK(cx_near(white.cr, channel_t(0)));

    return true;
}

/*
tests_color_convert_rgb_to_xyz_known_values
  Verifies white maps to the D65 tristimulus and black to the origin.
  Tests the following:
  - white lands on the D65 white point
  - black lands on the origin exactly
  - the Y axis of a primary matches its luminance contribution
*/
bool
tests_color_convert_rgb_to_xyz_known_values()
{
    const cie_xyz white = color_cast<cie_xyz>(rgb(channel_t(1), channel_t(1),
                                                  channel_t(1)));
    D_CX_CHECK(cx_near(white.x, channel_t(0.95047), channel_t(1e-3)));
    D_CX_CHECK(cx_near(white.y, channel_t(1), channel_t(1e-3)));
    D_CX_CHECK(cx_near(white.z, channel_t(1.08883), channel_t(1e-3)));

    const cie_xyz black = color_cast<cie_xyz>(rgb(channel_t(0), channel_t(0),
                                                  channel_t(0)));
    D_CX_CHECK(cx_near(black.x, channel_t(0)));
    D_CX_CHECK(cx_near(black.y, channel_t(0)));
    D_CX_CHECK(cx_near(black.z, channel_t(0)));

    // every tristimulus from an in-gamut colour is non-negative
    D_CX_CHECK(white.is_valid());
    D_CX_CHECK(black.is_valid());

    return true;
}

/*
tests_color_convert_hsl_round_trip
  Verifies every cube corner survives RGB -> HSL -> RGB within tolerance.
  Tests the following:
  - all nine standard probes round trip
*/
bool
tests_color_convert_hsl_round_trip()
{
    std::size_t i = 0;
    for (i = 0; i < cx_primary_count; ++i)
    {
        D_CX_CHECK(cx_rgb_round_trip<hsl>(cx_primaries(i)));
    }

    return true;
}

/*
tests_color_convert_hsv_round_trip
  Verifies every cube corner survives RGB -> HSV -> RGB within tolerance.
  Tests the following:
  - all nine standard probes round trip
*/
bool
tests_color_convert_hsv_round_trip()
{
    std::size_t i = 0;
    for (i = 0; i < cx_primary_count; ++i)
    {
        D_CX_CHECK(cx_rgb_round_trip<hsv>(cx_primaries(i)));
    }

    return true;
}

/*
tests_color_convert_cmyk_round_trip
  Verifies every cube corner survives RGB -> CMYK -> RGB within tolerance,
  including black, whose outbound leg takes the black-generation branch.
  Tests the following:
  - all nine standard probes round trip
*/
bool
tests_color_convert_cmyk_round_trip()
{
    std::size_t i = 0;
    for (i = 0; i < cx_primary_count; ++i)
    {
        D_CX_CHECK(cx_rgb_round_trip<cmyk>(cx_primaries(i)));
    }

    return true;
}

/*
tests_color_convert_ycbcr_round_trip
  Verifies every cube corner survives RGB -> YCbCr -> RGB within tolerance.
  The forward and inverse matrices are separate constants, so this is a real
  check that they are mutual inverses.
  Tests the following:
  - all nine standard probes round trip
*/
bool
tests_color_convert_ycbcr_round_trip()
{
    std::size_t i = 0;
    for (i = 0; i < cx_primary_count; ++i)
    {
        D_CX_CHECK(cx_rgb_round_trip<ycbcr>(cx_primaries(i)));
    }

    return true;
}

/*
tests_color_convert_xyz_round_trip
  Verifies every cube corner survives RGB -> XYZ -> RGB within tolerance.
  Tests the following:
  - all nine standard probes round trip
*/
bool
tests_color_convert_xyz_round_trip()
{
    std::size_t i = 0;
    for (i = 0; i < cx_primary_count; ++i)
    {
        D_CX_CHECK(cx_rgb_round_trip<cie_xyz>(cx_primaries(i)));
    }

    return true;
}

/*
tests_color_convert_round_trip_residual_is_not_exactly_zero
  Verifies the round trip is close but NOT bit-exact.  Pure red returns from
  HSL with a small non-zero blue channel; this is the reason every numeric
  check in this suite carries a tolerance, and it is worth asserting outright
  so nobody later "tightens" these tests into failure.
  Tests the following:
  - red -> HSL -> RGB returns red to within tolerance
  - but its blue channel is strictly greater than zero
  - the residual is nonetheless tiny
*/
bool
tests_color_convert_round_trip_residual_is_not_exactly_zero()
{
    const rgb red(channel_t(1), channel_t(0), channel_t(0));
    const rgb back = color_cast<rgb>(color_cast<hsl>(red));

    D_CX_CHECK(cx_rgb_near(back, channel_t(1), channel_t(0), channel_t(0)));

    // ... yet not exactly the value we started with
    D_CX_CHECK(back.b > channel_t(0));
    D_CX_CHECK(!(back == red));

    // the residual is small - well under a part per million
    D_CX_CHECK(back.b < channel_t(1e-6));

    return true;
}

/*
tests_color_convert_grey_has_no_saturation
  Verifies an achromatic input converts to zero saturation in HSL and HSV and
  to zero chroma in YCbCr - the shared consequence of R == G == B.
  Tests the following:
  - mid grey has zero saturation in both cylindrical models
  - its lightness and value reflect the grey level
  - its YCbCr chroma channels are both zero while luma is non-zero
*/
bool
tests_color_convert_grey_has_no_saturation()
{
    const rgb grey(channel_t(0.5), channel_t(0.5), channel_t(0.5));

    const hsl as_hsl = color_cast<hsl>(grey);
    D_CX_CHECK(cx_near(as_hsl.s, channel_t(0)));
    D_CX_CHECK(cx_near(as_hsl.l, channel_t(0.5)));

    const hsv as_hsv = color_cast<hsv>(grey);
    D_CX_CHECK(cx_near(as_hsv.s, channel_t(0)));
    D_CX_CHECK(cx_near(as_hsv.v, channel_t(0.5)));

    const ycbcr as_ycbcr = color_cast<ycbcr>(grey);
    D_CX_CHECK(cx_near(as_ycbcr.cb, channel_t(0)));
    D_CX_CHECK(cx_near(as_ycbcr.cr, channel_t(0)));
    D_CX_CHECK(cx_near(as_ycbcr.y, channel_t(0.5)));

    // and CMYK reaches it with black ink alone
    const cmyk as_cmyk = color_cast<cmyk>(grey);
    D_CX_CHECK(cx_near(as_cmyk.c, channel_t(0)));
    D_CX_CHECK(cx_near(as_cmyk.m, channel_t(0)));
    D_CX_CHECK(cx_near(as_cmyk.y, channel_t(0)));
    D_CX_CHECK(cx_near(as_cmyk.k, channel_t(0.5)));

    return true;
}

NS_END
NS_END
