/*******************************************************************************
* djinterp [test]                              color_rgb_tests_interpolation.cpp
*
*   The interpolation block for color_rgb.hpp: lerp on rgb and on rgba. lerp
* forwards to the C kernel's d_color_rgb_lerp and d_color_rgba_lerp, which
* compute a * (1 - t) + b * t per channel and clamp nothing. Every value
* below is a sum of powers of two, so each expected channel is exact in
* float and the checks compare with ==.
*
*
* path:      /tests/djinterp/cpp/util/color/color_rgb_tests_interpolation.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.17
*                                                            revised: 2026.11.17
*******************************************************************************/

// djinterp
#include "color_rgb_tests.hpp"  // the suite's declarations and helpers


NS_DJINTERP
NS_TESTING


/*
tests_color_rgb_lerp_reaches_its_endpoints
  Verifies lerp gives its first color at t = 0 and its second at t = 1.
  Tests the following:
  - t = 0 returns a, channel for channel
  - t = 1 returns b, channel for channel
*/
bool tests_color_rgb_lerp_reaches_its_endpoints()
{
    const rgb a(channel_t(0.25), channel_t(0.5),  channel_t(0.75));
    const rgb b(channel_t(1.0),  channel_t(0.0),  channel_t(0.125));

    D_CR_CHECK(cr_rgb_same(lerp(a, b, channel_t(0.0)),
                           channel_t(0.25), channel_t(0.5),
                           channel_t(0.75)));
    D_CR_CHECK(cr_rgb_same(lerp(a, b, channel_t(1.0)),
                           channel_t(1.0), channel_t(0.0),
                           channel_t(0.125)));

    return true;
}

/*
tests_color_rgb_lerp_is_linear_per_channel
  Verifies each channel moves on its own line from a to b.
  Tests the following:
  - t = 0.5 is the midpoint of each channel
  - t = 0.25 is a quarter of the way, with channels moving in opposite
    directions and one channel not moving at all
*/
bool tests_color_rgb_lerp_is_linear_per_channel()
{
    const rgb a(channel_t(0.0), channel_t(1.0), channel_t(0.5));
    const rgb b(channel_t(1.0), channel_t(0.0), channel_t(0.5));

    D_CR_CHECK(cr_rgb_same(lerp(a, b, channel_t(0.5)),
                           channel_t(0.5), channel_t(0.5),
                           channel_t(0.5)));
    D_CR_CHECK(cr_rgb_same(lerp(a, b, channel_t(0.25)),
                           channel_t(0.25), channel_t(0.75),
                           channel_t(0.5)));

    return true;
}

/*
tests_color_rgb_lerp_does_not_clamp
  Verifies lerp extrapolates outside [0, 1] rather than clamping, for t and
for the channels it produces: linear RGB may exceed the unit range.
  Tests the following:
  - t = 2 gives 2b - a, a channel above 1 included
  - t = -1 gives 2a - b, a channel below 0 included
*/
bool tests_color_rgb_lerp_does_not_clamp()
{
    const rgb a(channel_t(0.25), channel_t(0.5), channel_t(0.0));
    const rgb b(channel_t(0.75), channel_t(0.5), channel_t(1.0));

    D_CR_CHECK(cr_rgb_same(lerp(a, b, channel_t(2.0)),
                           channel_t(1.25), channel_t(0.5),
                           channel_t(2.0)));
    D_CR_CHECK(cr_rgb_same(lerp(a, b, channel_t(-1.0)),
                           channel_t(-0.25), channel_t(0.5),
                           channel_t(-1.0)));

    return true;
}

/*
tests_color_rgb_lerp_matches_the_c_kernel
  Verifies the C++ lerp is the C kernel's, value for value.
  Tests the following:
  - for several t, lerp's channels equal d_color_rgb_lerp's exactly
*/
bool tests_color_rgb_lerp_matches_the_c_kernel()
{
    const rgb       a(channel_t(0.1), channel_t(0.7), channel_t(0.3));
    const rgb       b(channel_t(0.9), channel_t(0.2), channel_t(0.6));
    const channel_t ts[] = { channel_t(0.0),  channel_t(0.3),
                             channel_t(0.5),  channel_t(0.9),
                             channel_t(1.0),  channel_t(1.7) };

    for (const channel_t t : ts)
    {
        const struct dj_color_rgb k = d_color_rgb_lerp(a, b, t);

        D_CR_CHECK(cr_rgb_same(lerp(a, b, t), k.r, k.g, k.b));
    }

    return true;
}

/*
tests_color_rgba_lerp_includes_alpha
  Verifies rgba's lerp moves alpha on the same line as the color channels.
  Tests the following:
  - the endpoints are a and b, alpha included
  - t = 0.5 is the midpoint of all four channels
*/
bool tests_color_rgba_lerp_includes_alpha()
{
    const rgba a(channel_t(0.0), channel_t(0.5), channel_t(1.0),
                 channel_t(0.0));
    const rgba b(channel_t(1.0), channel_t(0.5), channel_t(0.0),
                 channel_t(1.0));

    D_CR_CHECK(cr_rgba_same(lerp(a, b, channel_t(0.0)),
                            channel_t(0.0), channel_t(0.5),
                            channel_t(1.0), channel_t(0.0)));
    D_CR_CHECK(cr_rgba_same(lerp(a, b, channel_t(1.0)),
                            channel_t(1.0), channel_t(0.5),
                            channel_t(0.0), channel_t(1.0)));
    D_CR_CHECK(cr_rgba_same(lerp(a, b, channel_t(0.5)),
                            channel_t(0.5), channel_t(0.5),
                            channel_t(0.5), channel_t(0.5)));

    return true;
}

/*
tests_color_rgba_lerp_returns_rgba
  Verifies the rgba overload keeps the transport type: alpha is not dropped
on the way through.
  Tests the following:
  - lerp on two rgba returns an rgba, not an rgb
  - lerp on two rgb returns an rgb
*/
bool tests_color_rgba_lerp_returns_rgba()
{
    D_CR_CHECK((std::is_same<decltype(lerp(rgba(), rgba(), channel_t(0.5))),
                             rgba>::value));
    D_CR_CHECK((std::is_same<decltype(lerp(rgb(), rgb(), channel_t(0.5))),
                             rgb>::value));

    return true;
}


NS_END   // testing
NS_END   // djinterp
