/*******************************************************************************
* djinterp [core]                                    color_convert_tests_lab.cpp
*
*
* path:      /tests/djinterp/core/util/color/color_convert_tests_lab.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "color_convert_tests.hpp"

NS_DJINTERP
NS_TESTING

/*
tests_color_convert_xyz_to_lab_is_a_direct_edge
  Verifies XYZ to LAB uses the direct kernel path rather than detouring
  through RGB.  Established two ways: the result matches the kernel call
  bit-for-bit, and a tristimulus outside the RGB gamut survives instead of
  being clipped as the detour would clip it.
  Tests the following:
  - the conversion equals d_color_convert_xyz_to_lab on the sliced POD exactly
  - a super-white tristimulus yields L* above 100, which a route through RGB
    could not produce
*/
bool
tests_color_convert_xyz_to_lab_is_a_direct_edge()
{
    const cie_xyz source(channel_t(0.25), channel_t(0.5), channel_t(0.75));

    const cie_lab routed = color_cast<cie_lab>(source);
    const d_color_lab direct = d_color_convert_xyz_to_lab(source);

    D_CX_CHECK(routed.l == direct.l);
    D_CX_CHECK(routed.a == direct.a);
    D_CX_CHECK(routed.b == direct.b);

    // beyond the RGB gamut: the detour would have clipped this to L* <= 100
    const cie_xyz bright(channel_t(2), channel_t(2), channel_t(2));
    const cie_lab bright_lab = color_cast<cie_lab>(bright);
    D_CX_CHECK(bright_lab.l > channel_t(100));

    return true;
}

/*
tests_color_convert_lab_to_xyz_is_a_direct_edge
  Verifies LAB to XYZ uses the direct kernel path.
  Tests the following:
  - the conversion equals d_color_convert_lab_to_xyz on the sliced POD exactly
  - an L* above 100 produces a tristimulus above the white point rather than
    being clipped
*/
bool
tests_color_convert_lab_to_xyz_is_a_direct_edge()
{
    const cie_lab source(channel_t(50), channel_t(25), channel_t(-25));

    const cie_xyz routed = color_cast<cie_xyz>(source);
    const d_color_xyz direct = d_color_convert_lab_to_xyz(source);

    D_CX_CHECK(routed.x == direct.x);
    D_CX_CHECK(routed.y == direct.y);
    D_CX_CHECK(routed.z == direct.z);

    const cie_lab bright(channel_t(150), channel_t(0), channel_t(0));
    const cie_xyz bright_xyz = color_cast<cie_xyz>(bright);
    D_CX_CHECK(bright_xyz.y > channel_t(1));

    return true;
}

/*
tests_color_convert_xyz_lab_round_trip
  Verifies XYZ -> LAB -> XYZ recovers the tristimulus closely, including for
  values outside the RGB gamut that a hub route would have clipped.  This is
  the practical payoff of the shortcut.
  Tests the following:
  - several in-gamut tristimulus values round trip within tolerance
  - a super-white value round trips too, which would be impossible via RGB
*/
bool
tests_color_convert_xyz_lab_round_trip()
{
    const cie_xyz probes[] = {
        cie_xyz(channel_t(0.95047), channel_t(1), channel_t(1.08883)),
        cie_xyz(channel_t(0.25), channel_t(0.5), channel_t(0.75)),
        cie_xyz(channel_t(0.1), channel_t(0.1), channel_t(0.1)),
        cie_xyz(channel_t(2), channel_t(2), channel_t(2)),
    };

    std::size_t i = 0;
    for (i = 0; i < sizeof(probes) / sizeof(probes[0]); ++i)
    {
        const cie_lab out  = color_cast<cie_lab>(probes[i]);
        const cie_xyz back = color_cast<cie_xyz>(out);

        D_CX_CHECK(cx_near(back.x, probes[i].x, channel_t(1e-3)));
        D_CX_CHECK(cx_near(back.y, probes[i].y, channel_t(1e-3)));
        D_CX_CHECK(cx_near(back.z, probes[i].z, channel_t(1e-3)));
    }

    return true;
}

/*
tests_color_convert_white_point_maps_to_lightness_100
  Verifies the D65 white point converts to L* = 100 with vanishing chroma -
  the anchor that defines the whole L*a*b* scale.
  Tests the following:
  - the white point yields L* = 100 and near-zero a* and b*
  - the inverse recovers the white point
  - black yields L* = 0
*/
bool
tests_color_convert_white_point_maps_to_lightness_100()
{
    const cie_xyz white(channel_t(0.95047), channel_t(1), channel_t(1.08883));
    const cie_lab as_lab = color_cast<cie_lab>(white);

    D_CX_CHECK(cx_near(as_lab.l, channel_t(100), channel_t(1e-3)));
    D_CX_CHECK(cx_near(as_lab.a, channel_t(0), channel_t(1e-3)));
    D_CX_CHECK(cx_near(as_lab.b, channel_t(0), channel_t(1e-3)));

    // and back again
    const cie_xyz back = color_cast<cie_xyz>(
        cie_lab(channel_t(100), channel_t(0), channel_t(0)));
    D_CX_CHECK(cx_near(back.x, channel_t(0.95047), channel_t(1e-3)));
    D_CX_CHECK(cx_near(back.y, channel_t(1), channel_t(1e-3)));
    D_CX_CHECK(cx_near(back.z, channel_t(1.08883), channel_t(1e-3)));

    const cie_lab black = color_cast<cie_lab>(
        cie_xyz(channel_t(0), channel_t(0), channel_t(0)));
    D_CX_CHECK(cx_near(black.l, channel_t(0)));

    return true;
}

/*
tests_color_convert_rgb_to_lab_known_values
  Verifies the RGB primaries map to their expected L*a*b* coordinates.
  Tests the following:
  - pure red lands on its measured L*a*b* coordinates
  - white lands on L* = 100 and black on L* = 0
  - red's chroma is strongly positive on both axes, as a warm saturated colour
    must be
*/
bool
tests_color_convert_rgb_to_lab_known_values()
{
    const cie_lab red = color_cast<cie_lab>(rgb(channel_t(1), channel_t(0),
                                                channel_t(0)));
    D_CX_CHECK(cx_near(red.l, channel_t(53.2407913), channel_t(1e-3)));
    D_CX_CHECK(cx_near(red.a, channel_t(80.0924606), channel_t(1e-3)));
    D_CX_CHECK(cx_near(red.b, channel_t(67.2031937), channel_t(1e-3)));

    const cie_lab white = color_cast<cie_lab>(rgb(channel_t(1), channel_t(1),
                                                  channel_t(1)));
    D_CX_CHECK(cx_near(white.l, channel_t(100), channel_t(1e-3)));
    D_CX_CHECK(cx_near(white.a, channel_t(0), channel_t(1e-3)));

    const cie_lab black = color_cast<cie_lab>(rgb(channel_t(0), channel_t(0),
                                                  channel_t(0)));
    D_CX_CHECK(cx_near(black.l, channel_t(0)));

    // red is warm: both chroma axes positive
    D_CX_CHECK(red.a > channel_t(0));
    D_CX_CHECK(red.b > channel_t(0));

    return true;
}

/*
tests_color_convert_lab_round_trip_through_the_hub
  Verifies in-gamut colours survive RGB -> LAB -> RGB within tolerance.  This
  route is the composition of the RGB-XYZ leg with the XYZ-LAB shortcut.
  Tests the following:
  - all nine standard probes round trip
*/
bool
tests_color_convert_lab_round_trip_through_the_hub()
{
    std::size_t i = 0;
    for (i = 0; i < cx_primary_count; ++i)
    {
        D_CX_CHECK(cx_rgb_round_trip<cie_lab>(cx_primaries(i),
                                              channel_t(1e-3)));
    }

    return true;
}

/*
tests_color_convert_lab_lightness_tracks_luminance
  Verifies L* increases monotonically with luminance across a grey ramp, and
  that the relationship is non-linear: mid grey sits well above L* = 50,
  because the kernel treats the RGB channels as linear light and L* applies
  the CIE cube-root companding on top.
  Tests the following:
  - L* is strictly increasing across an ascending grey ramp
  - the endpoints are 0 and 100
  - mid grey lands near L* = 76, NOT near 50
  - a grey ramp stays achromatic throughout
*/
bool
tests_color_convert_lab_lightness_tracks_luminance()
{
    const channel_t levels[] = {
        channel_t(0), channel_t(0.25), channel_t(0.5), channel_t(0.75),
        channel_t(1),
    };

    channel_t   previous = channel_t(-1);
    std::size_t i        = 0;

    for (i = 0; i < sizeof(levels) / sizeof(levels[0]); ++i)
    {
        const rgb     grey(levels[i], levels[i], levels[i]);
        const cie_lab as_lab = color_cast<cie_lab>(grey);

        D_CX_CHECK(as_lab.l > previous);
        previous = as_lab.l;

        // a grey stays achromatic
        D_CX_CHECK(cx_near(as_lab.a, channel_t(0), channel_t(1e-3)));
        D_CX_CHECK(cx_near(as_lab.b, channel_t(0), channel_t(1e-3)));
    }

    const cie_lab black = color_cast<cie_lab>(rgb(channel_t(0), channel_t(0),
                                                  channel_t(0)));
    const cie_lab white = color_cast<cie_lab>(rgb(channel_t(1), channel_t(1),
                                                  channel_t(1)));
    D_CX_CHECK(cx_near(black.l, channel_t(0)));
    D_CX_CHECK(cx_near(white.l, channel_t(100), channel_t(1e-3)));

    // the companding is non-linear: half the light is far more than half the
    // perceived lightness
    const cie_lab mid = color_cast<cie_lab>(rgb(channel_t(0.5), channel_t(0.5),
                                                channel_t(0.5)));
    D_CX_CHECK(cx_near(mid.l, channel_t(76.0692673), channel_t(1e-3)));
    D_CX_CHECK(mid.l > channel_t(70));

    return true;
}

NS_END
NS_END
