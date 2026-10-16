/*******************************************************************************
* djinterp [core]                                   color_hsl_tests_clamping.cpp
*
*
* path:      /tests/djinterp/core/util/color/color_hsl_tests_clamping.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "color_hsl_tests.hpp"

NS_DJINTERP
NS_TESTING

/*
tests_color_hsl_clamp_wraps_hue_above_range
  Verifies hues at or above 360 wrap into range rather than saturating - the
  distinguishing behaviour of a cylindrical channel.
  Tests the following:
  - 360 becomes 0, 400 becomes 40, 720.5 becomes 0.5
  - the wrapped result is strictly below 360
*/
bool
tests_color_hsl_clamp_wraps_hue_above_range()
{
    const channel_t s = channel_t(0.5);
    const channel_t t = channel_t(0.5);

    D_CL_CHECK(hsl(channel_t(360), s, t).clamp().h == channel_t(0));
    D_CL_CHECK(cl_near(hsl(channel_t(400), s, t).clamp().h, channel_t(40)));
    D_CL_CHECK(cl_near(hsl(channel_t(720.5), s, t).clamp().h, channel_t(0.5)));
    D_CL_CHECK(cl_near(hsl(channel_t(1080), s, t).clamp().h, channel_t(0)));

    // wrapping, not saturating: a large hue does NOT become 360
    D_CL_CHECK(hsl(channel_t(400), s, t).clamp().h < channel_t(360));
    D_CL_CHECK(hsl(channel_t(400), s, t).clamp().h != channel_t(360));

    return true;
}

/*
tests_color_hsl_clamp_wraps_negative_hue
  Verifies negative hues wrap up into range, including magnitudes beyond a
  full turn - the kernel adds a turn after the truncating remainder.
  Tests the following:
  - -10 becomes 350 and -370 becomes 350
  - -360 becomes 0
  - every result lands in [0, 360)
*/
bool
tests_color_hsl_clamp_wraps_negative_hue()
{
    const channel_t s = channel_t(0.5);
    const channel_t t = channel_t(0.5);

    D_CL_CHECK(cl_near(hsl(channel_t(-10), s, t).clamp().h, channel_t(350)));
    D_CL_CHECK(cl_near(hsl(channel_t(-370), s, t).clamp().h, channel_t(350)));
    D_CL_CHECK(cl_near(hsl(channel_t(-360), s, t).clamp().h, channel_t(0)));
    D_CL_CHECK(cl_near(hsl(channel_t(-90), s, t).clamp().h, channel_t(270)));

    const channel_t wrapped = hsl(channel_t(-10), s, t).clamp().h;
    D_CL_CHECK(wrapped >= channel_t(0));
    D_CL_CHECK(wrapped < channel_t(360));

    return true;
}

/*
tests_color_hsl_clamp_preserves_canonical_hue
  Verifies a hue already in [0, 360) passes through unchanged, so clamp is
  inert on canonical input.
  Tests the following:
  - representative hues around the circle survive bit-exact
*/
bool
tests_color_hsl_clamp_preserves_canonical_hue()
{
    const channel_t s = channel_t(0.5);
    const channel_t t = channel_t(0.5);

    D_CL_CHECK(hsl(channel_t(0), s, t).clamp().h == channel_t(0));
    D_CL_CHECK(hsl(channel_t(120), s, t).clamp().h == channel_t(120));
    D_CL_CHECK(hsl(channel_t(359), s, t).clamp().h == channel_t(359));
    D_CL_CHECK(hsl(channel_t(0.5), s, t).clamp().h == channel_t(0.5));

    const hsl canonical(channel_t(210), channel_t(0.25), channel_t(0.75));
    D_CL_CHECK(canonical.clamp() == canonical);

    return true;
}

/*
tests_color_hsl_clamp_constrains_unit_channels
  Verifies saturation and lightness are clamped, not wrapped, onto [0, 1] - the two
  channels obey a different rule from hue.
  Tests the following:
  - below-zero lifts to 0 and above-one lowers to 1
  - a value of 1.5 becomes 1 rather than wrapping to 0.5
*/
bool
tests_color_hsl_clamp_constrains_unit_channels()
{
    const channel_t h = channel_t(90);

    D_CL_CHECK(hsl(h, channel_t(-1), channel_t(0.5)).clamp().s == channel_t(0));
    D_CL_CHECK(hsl(h, channel_t(2), channel_t(0.5)).clamp().s == channel_t(1));
    D_CL_CHECK(hsl(h, channel_t(0.5), channel_t(-1)).clamp().l == channel_t(0));
    D_CL_CHECK(hsl(h, channel_t(0.5), channel_t(2)).clamp().l == channel_t(1));

    // saturating, not wrapping
    D_CL_CHECK(hsl(h, channel_t(1.5), channel_t(0.5)).clamp().s == channel_t(1));
    D_CL_CHECK(hsl(h, channel_t(1.5), channel_t(0.5)).clamp().s != channel_t(0.5));

    return true;
}

/*
tests_color_hsl_clamp_is_per_channel
  Verifies each channel is canonicalised independently under its own rule.
  Tests the following:
  - a value mixing an out-of-range hue with out-of-range unit channels
    resolves each on its own terms
*/
bool
tests_color_hsl_clamp_is_per_channel()
{
    const hsl mixed(channel_t(400), channel_t(2), channel_t(-1));
    const hsl c = mixed.clamp();

    D_CL_CHECK(cl_near(c.h, channel_t(40)));   // wrapped
    D_CL_CHECK(c.s == channel_t(1));            // saturated high
    D_CL_CHECK(c.l == channel_t(0));          // saturated low

    const hsl other(channel_t(-10), channel_t(0.25), channel_t(3));
    const hsl d = other.clamp();
    D_CL_CHECK(cl_near(d.h, channel_t(350)));
    D_CL_CHECK(d.s == channel_t(0.25));         // untouched
    D_CL_CHECK(d.l == channel_t(1));

    return true;
}

/*
tests_color_hsl_clamp_is_idempotent
  Verifies clamping an already-clamped value changes nothing, so clamp is a
  projection onto the canonical domain.
  Tests the following:
  - clamp(clamp(v)) == clamp(v) for wrapped, saturated and canonical inputs
*/
bool
tests_color_hsl_clamp_is_idempotent()
{
    const hsl probes[] = {
        hsl(channel_t(400), channel_t(2), channel_t(-1)),
        hsl(channel_t(-370), channel_t(0.5), channel_t(0.5)),
        hsl(channel_t(120), channel_t(0.25), channel_t(0.75)),
        hsl(channel_t(360), channel_t(1), channel_t(0)),
        hsl(),
    };

    std::size_t i = 0;
    for (i = 0; i < sizeof(probes) / sizeof(probes[0]); ++i)
    {
        const hsl once  = probes[i].clamp();
        const hsl twice = once.clamp();
        D_CL_CHECK(once == twice);
    }

    return true;
}

/*
tests_color_hsl_clamp_result_is_valid
  Verifies that for any finite input the clamped result satisfies is_valid,
  including the hue == 360 case that clamp repairs to 0.
  Tests the following:
  - a spread of finite invalid inputs all become valid after clamping
  - the hue == 360 boundary is specifically repaired
*/
bool
tests_color_hsl_clamp_result_is_valid()
{
    const hsl probes[] = {
        hsl(channel_t(360), channel_t(0.5), channel_t(0.5)),
        hsl(channel_t(-1), channel_t(-1), channel_t(-1)),
        hsl(channel_t(1000), channel_t(2), channel_t(2)),
        hsl(channel_t(-1000), channel_t(0.5), channel_t(0.5)),
        hsl(channel_t(720), channel_t(1), channel_t(0)),
    };

    std::size_t i = 0;
    for (i = 0; i < sizeof(probes) / sizeof(probes[0]); ++i)
    {
        D_CL_CHECK(probes[i].clamp().is_valid());
    }

    // the specific repair: an invalid hue of exactly 360 becomes a valid 0
    const hsl at_turn(channel_t(360), channel_t(0.5), channel_t(0.5));
    D_CL_CHECK(!at_turn.is_valid());
    D_CL_CHECK(at_turn.clamp().is_valid());
    D_CL_CHECK(at_turn.clamp().h == channel_t(0));

    return true;
}

/*
tests_color_hsl_clamp_non_finite_unit_channels
  Verifies non-finite behaviour on the two unit channels: infinities saturate
  and NaN passes through, leaving the result invalid.  Hue is deliberately
  excluded - the wrap truncates through an integer cast, so a non-finite hue
  is outside the kernel's contract and is not exercised here.
  Tests the following:
  - infinite saturation or lightness saturates to a bound and stays valid
  - a NaN unit channel survives clamping and leaves the result invalid
*/
bool
tests_color_hsl_clamp_non_finite_unit_channels()
{
    const channel_t h   = channel_t(90);
    const channel_t inf = cl_inf();
    const channel_t nan = cl_nan();

    const hsl inf_v(h, inf, -inf);
    const hsl ci = inf_v.clamp();
    D_CL_CHECK(ci.s == channel_t(1));
    D_CL_CHECK(ci.l == channel_t(0));
    D_CL_CHECK(ci.is_valid());

    const hsl nan_v(h, nan, channel_t(0.5));
    const hsl cn = nan_v.clamp();
    D_CL_CHECK(cn.s != cn.s);              // still NaN
    D_CL_CHECK(!cn.is_valid());            // and therefore still invalid
    D_CL_CHECK(cn.h == channel_t(90));     // hue untouched
    D_CL_CHECK(cn.l == channel_t(0.5));

    return true;
}

/*
tests_color_hsl_clamp_is_constexpr
  Verifies clamp is usable in a constant expression - the hue wrap is written
  to stay constant-expression-safe.
  Tests the following:
  - wrapping and saturating arms both evaluate at compile time
*/
bool
tests_color_hsl_clamp_is_constexpr()
{
    D_CONSTEXPR hsl raw(channel_t(400), channel_t(2), channel_t(-1));
    D_CONSTEXPR hsl c = raw.clamp();

    static_assert(c.s == channel_t(1), "clamp high arm must be constexpr");
    static_assert(c.l == channel_t(0), "clamp low arm must be constexpr");
    static_assert(c.h < channel_t(360), "hue wrap must be constexpr");

    D_CL_CHECK(c.s == channel_t(1));
    D_CL_CHECK(c.l == channel_t(0));
    D_CL_CHECK(cl_near(c.h, channel_t(40)));

    return true;
}

/*
tests_color_hsl_clamp_agrees_with_kernel
  Verifies the wrapper's result matches d_color_hsl_clamp on the sliced POD -
  the member adds no policy of its own.
  Tests the following:
  - wrapper and kernel agree channel-for-channel across every probe
*/
bool
tests_color_hsl_clamp_agrees_with_kernel()
{
    const hsl probes[] = {
        hsl(),
        hsl(channel_t(120), channel_t(0.5), channel_t(0.5)),
        hsl(channel_t(400), channel_t(2), channel_t(-1)),
        hsl(channel_t(-370), channel_t(0.25), channel_t(0.75)),
        hsl(channel_t(360), channel_t(1), channel_t(0)),
        hsl(channel_t(90), cl_inf(), -cl_inf()),
    };

    std::size_t i = 0;
    for (i = 0; i < sizeof(probes) / sizeof(probes[0]); ++i)
    {
        const d_color_hsl pod       = probes[i];
        const d_color_hsl from_kern = d_color_hsl_clamp(pod);
        const hsl         from_wrap = probes[i].clamp();

        D_CL_CHECK(from_wrap.h == from_kern.h);
        D_CL_CHECK(from_wrap.s == from_kern.s);
        D_CL_CHECK(from_wrap.l == from_kern.l);
    }

    return true;
}

NS_END
NS_END
