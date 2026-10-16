/*******************************************************************************
* djinterp [core]                                   color_hsv_tests_clamping.cpp
*
*
* path:      /tests/djinterp/core/util/color/color_hsv_tests_clamping.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "color_hsv_tests.hpp"

NS_DJINTERP
NS_TESTING

/*
tests_color_hsv_clamp_wraps_hue_above_range
  Verifies hues at or above 360 wrap into range rather than saturating - the
  distinguishing behaviour of a cylindrical channel.
  Tests the following:
  - 360 becomes 0, 400 becomes 40, 720.5 becomes 0.5
  - the wrapped result is strictly below 360
*/
bool
tests_color_hsv_clamp_wraps_hue_above_range()
{
    const channel_t s = channel_t(0.5);
    const channel_t t = channel_t(0.5);

    D_CV_CHECK(hsv(channel_t(360), s, t).clamp().h == channel_t(0));
    D_CV_CHECK(cv_near(hsv(channel_t(400), s, t).clamp().h, channel_t(40)));
    D_CV_CHECK(cv_near(hsv(channel_t(720.5), s, t).clamp().h, channel_t(0.5)));
    D_CV_CHECK(cv_near(hsv(channel_t(1080), s, t).clamp().h, channel_t(0)));

    // wrapping, not saturating: a large hue does NOT become 360
    D_CV_CHECK(hsv(channel_t(400), s, t).clamp().h < channel_t(360));
    D_CV_CHECK(hsv(channel_t(400), s, t).clamp().h != channel_t(360));

    return true;
}

/*
tests_color_hsv_clamp_wraps_negative_hue
  Verifies negative hues wrap up into range, including magnitudes beyond a
  full turn - the kernel adds a turn after the truncating remainder.
  Tests the following:
  - -10 becomes 350 and -370 becomes 350
  - -360 becomes 0
  - every result lands in [0, 360)
*/
bool
tests_color_hsv_clamp_wraps_negative_hue()
{
    const channel_t s = channel_t(0.5);
    const channel_t t = channel_t(0.5);

    D_CV_CHECK(cv_near(hsv(channel_t(-10), s, t).clamp().h, channel_t(350)));
    D_CV_CHECK(cv_near(hsv(channel_t(-370), s, t).clamp().h, channel_t(350)));
    D_CV_CHECK(cv_near(hsv(channel_t(-360), s, t).clamp().h, channel_t(0)));
    D_CV_CHECK(cv_near(hsv(channel_t(-90), s, t).clamp().h, channel_t(270)));

    const channel_t wrapped = hsv(channel_t(-10), s, t).clamp().h;
    D_CV_CHECK(wrapped >= channel_t(0));
    D_CV_CHECK(wrapped < channel_t(360));

    return true;
}

/*
tests_color_hsv_clamp_preserves_canonical_hue
  Verifies a hue already in [0, 360) passes through unchanged, so clamp is
  inert on canonical input.
  Tests the following:
  - representative hues around the circle survive bit-exact
*/
bool
tests_color_hsv_clamp_preserves_canonical_hue()
{
    const channel_t s = channel_t(0.5);
    const channel_t t = channel_t(0.5);

    D_CV_CHECK(hsv(channel_t(0), s, t).clamp().h == channel_t(0));
    D_CV_CHECK(hsv(channel_t(120), s, t).clamp().h == channel_t(120));
    D_CV_CHECK(hsv(channel_t(359), s, t).clamp().h == channel_t(359));
    D_CV_CHECK(hsv(channel_t(0.5), s, t).clamp().h == channel_t(0.5));

    const hsv canonical(channel_t(210), channel_t(0.25), channel_t(0.75));
    D_CV_CHECK(canonical.clamp() == canonical);

    return true;
}

/*
tests_color_hsv_clamp_constrains_unit_channels
  Verifies saturation and value are clamped, not wrapped, onto [0, 1] - the two
  channels obey a different rule from hue.
  Tests the following:
  - below-zero lifts to 0 and above-one lowers to 1
  - a value of 1.5 becomes 1 rather than wrapping to 0.5
*/
bool
tests_color_hsv_clamp_constrains_unit_channels()
{
    const channel_t h = channel_t(90);

    D_CV_CHECK(hsv(h, channel_t(-1), channel_t(0.5)).clamp().s == channel_t(0));
    D_CV_CHECK(hsv(h, channel_t(2), channel_t(0.5)).clamp().s == channel_t(1));
    D_CV_CHECK(hsv(h, channel_t(0.5), channel_t(-1)).clamp().v == channel_t(0));
    D_CV_CHECK(hsv(h, channel_t(0.5), channel_t(2)).clamp().v == channel_t(1));

    // saturating, not wrapping
    D_CV_CHECK(hsv(h, channel_t(1.5), channel_t(0.5)).clamp().s == channel_t(1));
    D_CV_CHECK(hsv(h, channel_t(1.5), channel_t(0.5)).clamp().s != channel_t(0.5));

    return true;
}

/*
tests_color_hsv_clamp_is_per_channel
  Verifies each channel is canonicalised independently under its own rule.
  Tests the following:
  - a value mixing an out-of-range hue with out-of-range unit channels
    resolves each on its own terms
*/
bool
tests_color_hsv_clamp_is_per_channel()
{
    const hsv mixed(channel_t(400), channel_t(2), channel_t(-1));
    const hsv c = mixed.clamp();

    D_CV_CHECK(cv_near(c.h, channel_t(40)));   // wrapped
    D_CV_CHECK(c.s == channel_t(1));            // saturated high
    D_CV_CHECK(c.v == channel_t(0));          // saturated low

    const hsv other(channel_t(-10), channel_t(0.25), channel_t(3));
    const hsv d = other.clamp();
    D_CV_CHECK(cv_near(d.h, channel_t(350)));
    D_CV_CHECK(d.s == channel_t(0.25));         // untouched
    D_CV_CHECK(d.v == channel_t(1));

    return true;
}

/*
tests_color_hsv_clamp_is_idempotent
  Verifies clamping an already-clamped value changes nothing, so clamp is a
  projection onto the canonical domain.
  Tests the following:
  - clamp(clamp(v)) == clamp(v) for wrapped, saturated and canonical inputs
*/
bool
tests_color_hsv_clamp_is_idempotent()
{
    const hsv probes[] = {
        hsv(channel_t(400), channel_t(2), channel_t(-1)),
        hsv(channel_t(-370), channel_t(0.5), channel_t(0.5)),
        hsv(channel_t(120), channel_t(0.25), channel_t(0.75)),
        hsv(channel_t(360), channel_t(1), channel_t(0)),
        hsv(),
    };

    std::size_t i = 0;
    for (i = 0; i < sizeof(probes) / sizeof(probes[0]); ++i)
    {
        const hsv once  = probes[i].clamp();
        const hsv twice = once.clamp();
        D_CV_CHECK(once == twice);
    }

    return true;
}

/*
tests_color_hsv_clamp_result_is_valid
  Verifies that for any finite input the clamped result satisfies is_valid,
  including the hue == 360 case that clamp repairs to 0.
  Tests the following:
  - a spread of finite invalid inputs all become valid after clamping
  - the hue == 360 boundary is specifically repaired
*/
bool
tests_color_hsv_clamp_result_is_valid()
{
    const hsv probes[] = {
        hsv(channel_t(360), channel_t(0.5), channel_t(0.5)),
        hsv(channel_t(-1), channel_t(-1), channel_t(-1)),
        hsv(channel_t(1000), channel_t(2), channel_t(2)),
        hsv(channel_t(-1000), channel_t(0.5), channel_t(0.5)),
        hsv(channel_t(720), channel_t(1), channel_t(0)),
    };

    std::size_t i = 0;
    for (i = 0; i < sizeof(probes) / sizeof(probes[0]); ++i)
    {
        D_CV_CHECK(probes[i].clamp().is_valid());
    }

    // the specific repair: an invalid hue of exactly 360 becomes a valid 0
    const hsv at_turn(channel_t(360), channel_t(0.5), channel_t(0.5));
    D_CV_CHECK(!at_turn.is_valid());
    D_CV_CHECK(at_turn.clamp().is_valid());
    D_CV_CHECK(at_turn.clamp().h == channel_t(0));

    return true;
}

/*
tests_color_hsv_clamp_non_finite_unit_channels
  Verifies non-finite behaviour on the two unit channels: infinities saturate
  and NaN passes through, leaving the result invalid.  Hue is deliberately
  excluded - the wrap truncates through an integer cast, so a non-finite hue
  is outside the kernel's contract and is not exercised here.
  Tests the following:
  - infinite saturation or value saturates to a bound and stays valid
  - a NaN unit channel survives clamping and leaves the result invalid
*/
bool
tests_color_hsv_clamp_non_finite_unit_channels()
{
    const channel_t h   = channel_t(90);
    const channel_t inf = cv_inf();
    const channel_t nan = cv_nan();

    const hsv inf_v(h, inf, -inf);
    const hsv ci = inf_v.clamp();
    D_CV_CHECK(ci.s == channel_t(1));
    D_CV_CHECK(ci.v == channel_t(0));
    D_CV_CHECK(ci.is_valid());

    const hsv nan_v(h, nan, channel_t(0.5));
    const hsv cn = nan_v.clamp();
    D_CV_CHECK(cn.s != cn.s);              // still NaN
    D_CV_CHECK(!cn.is_valid());            // and therefore still invalid
    D_CV_CHECK(cn.h == channel_t(90));     // hue untouched
    D_CV_CHECK(cn.v == channel_t(0.5));

    return true;
}

/*
tests_color_hsv_clamp_is_constexpr
  Verifies clamp is usable in a constant expression - the hue wrap is written
  to stay constant-expression-safe.
  Tests the following:
  - wrapping and saturating arms both evaluate at compile time
*/
bool
tests_color_hsv_clamp_is_constexpr()
{
    D_CONSTEXPR hsv raw(channel_t(400), channel_t(2), channel_t(-1));
    D_CONSTEXPR hsv c = raw.clamp();

    static_assert(c.s == channel_t(1), "clamp high arm must be constexpr");
    static_assert(c.v == channel_t(0), "clamp low arm must be constexpr");
    static_assert(c.h < channel_t(360), "hue wrap must be constexpr");

    D_CV_CHECK(c.s == channel_t(1));
    D_CV_CHECK(c.v == channel_t(0));
    D_CV_CHECK(cv_near(c.h, channel_t(40)));

    return true;
}

/*
tests_color_hsv_clamp_agrees_with_kernel
  Verifies the wrapper's result matches d_color_hsv_clamp on the sliced POD -
  the member adds no policy of its own.
  Tests the following:
  - wrapper and kernel agree channel-for-channel across every probe
*/
bool
tests_color_hsv_clamp_agrees_with_kernel()
{
    const hsv probes[] = {
        hsv(),
        hsv(channel_t(120), channel_t(0.5), channel_t(0.5)),
        hsv(channel_t(400), channel_t(2), channel_t(-1)),
        hsv(channel_t(-370), channel_t(0.25), channel_t(0.75)),
        hsv(channel_t(360), channel_t(1), channel_t(0)),
        hsv(channel_t(90), cv_inf(), -cv_inf()),
    };

    std::size_t i = 0;
    for (i = 0; i < sizeof(probes) / sizeof(probes[0]); ++i)
    {
        const d_color_hsv pod       = probes[i];
        const d_color_hsv from_kern = d_color_hsv_clamp(pod);
        const hsv         from_wrap = probes[i].clamp();

        D_CV_CHECK(from_wrap.h == from_kern.h);
        D_CV_CHECK(from_wrap.s == from_kern.s);
        D_CV_CHECK(from_wrap.v == from_kern.v);
    }

    return true;
}

NS_END
NS_END
