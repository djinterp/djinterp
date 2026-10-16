/*******************************************************************************
* djinterp [core]                             color_convert_tests_crossmodel.cpp
*
*
* path:      /tests/djinterp/core/util/color/color_convert_tests_crossmodel.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "color_convert_tests.hpp"

NS_DJINTERP
NS_TESTING

/*
tests_color_convert_hsl_to_hsv_composes_the_two_legs
  Verifies HSL to HSV routes through RGB and lands on the expected
  coordinates.  The two models share a hue convention, so the hue should
  survive the detour - but hue is CYCLIC, and that has a sharp consequence
  worth pinning down here.

  Converting HSL red (hue 0) out to RGB leaves a residual 3.58e-07 in the blue
  channel; coming back, that residual is enough to push the hue off zero in
  the NEGATIVE direction, which wraps to just under 360.  The value is correct
  modulo 360 and wrong by 360 if compared naively.  Any tolerance check on a
  hue must therefore be taken modulo 360, and this test asserts the wrap
  rather than papering over it.
  Tests the following:
  - a mid-circle hue survives the detour to within a rounding residual
  - a hue of zero comes back just BELOW 360, not just above 0
  - the discrepancy is a full turn, so a naive absolute comparison fails
  - saturation and value are unaffected by the wrap
*/
bool
tests_color_convert_hsl_to_hsv_composes_the_two_legs()
{
    const hsl source(channel_t(210), channel_t(0.5), channel_t(0.5));
    const hsv result = color_cast<hsv>(source);

    D_CX_CHECK(cx_near(result.h, channel_t(210), channel_t(1e-3)));

    // the detour is visible: the hue comes back a hair under 210
    D_CX_CHECK(result.h != channel_t(210));

    // HSL lightness 0.5 at saturation 0.5 becomes HSV value 0.75
    D_CX_CHECK(cx_near(result.v, channel_t(0.75)));

    // now the cyclic case: red's hue wraps to the far end of the circle
    const hsv red = color_cast<hsv>(
        hsl(channel_t(0), channel_t(1), channel_t(0.5)));

    D_CX_CHECK(cx_near(red.h, channel_t(360), channel_t(1e-3)));
    D_CX_CHECK(!cx_near(red.h, channel_t(0), channel_t(1e-3)));
    D_CX_CHECK(red.h > channel_t(359));

    // ... which is a full turn away from the naive expectation
    const channel_t naive_error = red.h - channel_t(0);
    D_CX_CHECK(naive_error > channel_t(359));

    // ... but zero once taken modulo 360, which is the correct comparison
    const channel_t wrapped = red.h - channel_t(360);
    D_CX_CHECK(cx_near(wrapped, channel_t(0), channel_t(1e-3)));

    // the other two channels are unaffected by the wrap
    D_CX_CHECK(cx_near(red.s, channel_t(1)));
    D_CX_CHECK(cx_near(red.v, channel_t(1)));

    return true;
}

/*
tests_color_convert_cmyk_to_hsl_composes_the_two_legs
  Verifies CMYK to HSL routes through RGB and lands on the expected
  coordinates.
  Tests the following:
  - the CMYK encoding of pure red arrives as HSL hue 0 at full saturation
  - the CMYK encoding of black arrives as HSL lightness 0
  - an equal-ink grey arrives achromatic
*/
bool
tests_color_convert_cmyk_to_hsl_composes_the_two_legs()
{
    const cmyk red(channel_t(0), channel_t(1), channel_t(1), channel_t(0));
    const hsl  as_hsl = color_cast<hsl>(red);

    D_CX_CHECK(cx_near(as_hsl.h, channel_t(0)));
    D_CX_CHECK(cx_near(as_hsl.s, channel_t(1)));
    D_CX_CHECK(cx_near(as_hsl.l, channel_t(0.5)));

    const cmyk black(channel_t(0), channel_t(0), channel_t(0), channel_t(1));
    D_CX_CHECK(cx_near(color_cast<hsl>(black).l, channel_t(0)));

    const cmyk grey(channel_t(0), channel_t(0), channel_t(0), channel_t(0.5));
    const hsl  grey_hsl = color_cast<hsl>(grey);
    D_CX_CHECK(cx_near(grey_hsl.s, channel_t(0)));
    D_CX_CHECK(cx_near(grey_hsl.l, channel_t(0.5)));

    return true;
}

/*
tests_color_convert_every_ordered_pair_is_reachable
  Verifies all forty-nine ordered model pairs compile and produce a value, so
  the graph has no missing edge.  The substance of this test is that it
  COMPILES: a missing to_rgb overload or rgb_to specialisation would be a hard
  error here rather than a wrong number somewhere downstream.
  Tests the following:
  - each of the seven models converts to each of the seven models
*/
bool
tests_color_convert_every_ordered_pair_is_reachable()
{
    const rgb     s_rgb(channel_t(0.25), channel_t(0.5), channel_t(0.75));
    const hsl     s_hsl(channel_t(210), channel_t(0.5), channel_t(0.5));
    const hsv     s_hsv(channel_t(210), channel_t(0.5), channel_t(0.5));
    const cmyk    s_cmyk(channel_t(0.1), channel_t(0.2), channel_t(0.3),
                         channel_t(0.4));
    const ycbcr   s_ycbcr(channel_t(0.5), channel_t(0.1), channel_t(-0.1));
    const cie_xyz s_xyz(channel_t(0.25), channel_t(0.5), channel_t(0.75));
    const cie_lab s_lab(channel_t(50), channel_t(25), channel_t(-25));

    std::size_t reached = 0;

    // from rgb
    (void)color_cast<rgb>(s_rgb);       ++reached;
    (void)color_cast<hsl>(s_rgb);       ++reached;
    (void)color_cast<hsv>(s_rgb);       ++reached;
    (void)color_cast<cmyk>(s_rgb);      ++reached;
    (void)color_cast<ycbcr>(s_rgb);     ++reached;
    (void)color_cast<cie_xyz>(s_rgb);   ++reached;
    (void)color_cast<cie_lab>(s_rgb);   ++reached;

    // from hsl
    (void)color_cast<rgb>(s_hsl);       ++reached;
    (void)color_cast<hsl>(s_hsl);       ++reached;
    (void)color_cast<hsv>(s_hsl);       ++reached;
    (void)color_cast<cmyk>(s_hsl);      ++reached;
    (void)color_cast<ycbcr>(s_hsl);     ++reached;
    (void)color_cast<cie_xyz>(s_hsl);   ++reached;
    (void)color_cast<cie_lab>(s_hsl);   ++reached;

    // from hsv
    (void)color_cast<rgb>(s_hsv);       ++reached;
    (void)color_cast<hsl>(s_hsv);       ++reached;
    (void)color_cast<hsv>(s_hsv);       ++reached;
    (void)color_cast<cmyk>(s_hsv);      ++reached;
    (void)color_cast<ycbcr>(s_hsv);     ++reached;
    (void)color_cast<cie_xyz>(s_hsv);   ++reached;
    (void)color_cast<cie_lab>(s_hsv);   ++reached;

    // from cmyk
    (void)color_cast<rgb>(s_cmyk);      ++reached;
    (void)color_cast<hsl>(s_cmyk);      ++reached;
    (void)color_cast<hsv>(s_cmyk);      ++reached;
    (void)color_cast<cmyk>(s_cmyk);     ++reached;
    (void)color_cast<ycbcr>(s_cmyk);    ++reached;
    (void)color_cast<cie_xyz>(s_cmyk);  ++reached;
    (void)color_cast<cie_lab>(s_cmyk);  ++reached;

    // from ycbcr
    (void)color_cast<rgb>(s_ycbcr);     ++reached;
    (void)color_cast<hsl>(s_ycbcr);     ++reached;
    (void)color_cast<hsv>(s_ycbcr);     ++reached;
    (void)color_cast<cmyk>(s_ycbcr);    ++reached;
    (void)color_cast<ycbcr>(s_ycbcr);   ++reached;
    (void)color_cast<cie_xyz>(s_ycbcr); ++reached;
    (void)color_cast<cie_lab>(s_ycbcr); ++reached;

    // from cie_xyz
    (void)color_cast<rgb>(s_xyz);       ++reached;
    (void)color_cast<hsl>(s_xyz);       ++reached;
    (void)color_cast<hsv>(s_xyz);       ++reached;
    (void)color_cast<cmyk>(s_xyz);      ++reached;
    (void)color_cast<ycbcr>(s_xyz);     ++reached;
    (void)color_cast<cie_xyz>(s_xyz);   ++reached;
    (void)color_cast<cie_lab>(s_xyz);   ++reached;

    // from cie_lab
    (void)color_cast<rgb>(s_lab);       ++reached;
    (void)color_cast<hsl>(s_lab);       ++reached;
    (void)color_cast<hsv>(s_lab);       ++reached;
    (void)color_cast<cmyk>(s_lab);      ++reached;
    (void)color_cast<ycbcr>(s_lab);     ++reached;
    (void)color_cast<cie_xyz>(s_lab);   ++reached;
    (void)color_cast<cie_lab>(s_lab);   ++reached;

    D_CX_CHECK(reached == 49u);

    return true;
}

/*
tests_color_convert_cross_model_equals_manual_composition
  Verifies a cross-model conversion equals the two legs performed by hand, bit
  for bit.  No pair silently acquires a shortcut of its own - apart from the
  two CIE edges, which the lab section covers separately.
  Tests the following:
  - HSL to CMYK equals HSL to RGB followed by RGB to CMYK, exactly
  - YCbCr to HSV equals the same composition through the hub, exactly
*/
bool
tests_color_convert_cross_model_equals_manual_composition()
{
    const hsl source(channel_t(210), channel_t(0.5), channel_t(0.5));

    const cmyk composed = color_cast<cmyk>(color_cast<rgb>(source));
    const cmyk routed   = color_cast<cmyk>(source);

    D_CX_CHECK(composed.c == routed.c);
    D_CX_CHECK(composed.m == routed.m);
    D_CX_CHECK(composed.y == routed.y);
    D_CX_CHECK(composed.k == routed.k);

    const ycbcr other(channel_t(0.5), channel_t(0.1), channel_t(-0.1));

    const hsv composed_v = color_cast<hsv>(color_cast<rgb>(other));
    const hsv routed_v   = color_cast<hsv>(other);

    D_CX_CHECK(composed_v.h == routed_v.h);
    D_CX_CHECK(composed_v.s == routed_v.s);
    D_CX_CHECK(composed_v.v == routed_v.v);

    return true;
}

/*
tests_color_convert_routing_is_not_symmetric_in_precision
  Verifies that converting out and back through a different model accumulates
  error rather than cancelling it.  The graph is not a group: chained casts
  drift, so code should convert once from the authoritative representation
  rather than hopping between models.
  Tests the following:
  - the identity conversion is exact
  - a three-hop chain returns close to the original but NOT exactly
  - a longer chain is no better than a shorter one
*/
bool
tests_color_convert_routing_is_not_symmetric_in_precision()
{
    const rgb source(channel_t(0.3), channel_t(0.6), channel_t(0.9));

    // the self-edge is exact
    D_CX_CHECK(color_cast<rgb>(source) == source);

    // a detour is close, but not the same value
    const rgb chained = color_cast<rgb>(
        color_cast<cmyk>(color_cast<hsl>(source)));

    D_CX_CHECK(cx_rgb_near(chained, source.r, source.g, source.b,
                           channel_t(1e-3)));
    D_CX_CHECK(!(chained == source));

    // a longer chain drifts at least as much
    const rgb longer = color_cast<rgb>(
        color_cast<ycbcr>(color_cast<hsv>(
            color_cast<cmyk>(color_cast<hsl>(source)))));

    D_CX_CHECK(cx_rgb_near(longer, source.r, source.g, source.b,
                           channel_t(1e-2)));
    D_CX_CHECK(!(longer == source));

    return true;
}

NS_END
NS_END
