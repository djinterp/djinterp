/*******************************************************************************
* djinterp [core]                                 color_hsl_tests_validation.cpp
*
*
* path:      /tests/djinterp/core/util/color/color_hsl_tests_validation.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "color_hsl_tests.hpp"

NS_DJINTERP
NS_TESTING

/*
tests_color_hsl_is_valid_accepts_canonical_ranges
  Verifies interior values across all three channels are valid.
  Tests the following:
  - several mid-range probes spanning the hue circle are accepted
*/
bool
tests_color_hsl_is_valid_accepts_canonical_ranges()
{
    D_CL_CHECK(hsl(channel_t(0.5), channel_t(0.5), channel_t(0.5)).is_valid());
    D_CL_CHECK(hsl(channel_t(120), channel_t(0.25), channel_t(0.75)).is_valid());
    D_CL_CHECK(hsl(channel_t(359), channel_t(1), channel_t(0)).is_valid());
    D_CL_CHECK(hsl(channel_t(180), channel_t(0), channel_t(1)).is_valid());

    return true;
}

/*
tests_color_hsl_is_valid_hue_lower_bound_is_inclusive
  Verifies a hue of exactly 0 is valid - the interval is closed at the low end.
  Tests the following:
  - hue 0 with varied saturation and lightness is accepted
*/
bool
tests_color_hsl_is_valid_hue_lower_bound_is_inclusive()
{
    D_CL_CHECK(hsl(channel_t(0), channel_t(0), channel_t(0)).is_valid());
    D_CL_CHECK(hsl(channel_t(0), channel_t(1), channel_t(1)).is_valid());
    D_CL_CHECK(hsl(channel_t(0), channel_t(0.5), channel_t(0.5)).is_valid());

    return true;
}

/*
tests_color_hsl_is_valid_hue_upper_bound_is_exclusive
  Verifies a hue of exactly 360 is REJECTED while the value just below it is
  accepted - the hue interval is half-open, [0, 360).
  Tests the following:
  - hue 360 is invalid even though it denotes the same color as hue 0
  - hue 359.9 is valid
  - hues beyond a full turn are invalid too
*/
bool
tests_color_hsl_is_valid_hue_upper_bound_is_exclusive()
{
    D_CL_CHECK(!hsl(channel_t(360), channel_t(0.5), channel_t(0.5)).is_valid());
    D_CL_CHECK(hsl(channel_t(359.9), channel_t(0.5), channel_t(0.5)).is_valid());
    D_CL_CHECK(!hsl(channel_t(360.001), channel_t(0.5), channel_t(0.5)).is_valid());
    D_CL_CHECK(!hsl(channel_t(720), channel_t(0.5), channel_t(0.5)).is_valid());

    // hue 0 IS valid, so the rejection is genuinely about the upper end
    D_CL_CHECK(hsl(channel_t(0), channel_t(0.5), channel_t(0.5)).is_valid());

    return true;
}

/*
tests_color_hsl_is_valid_rejects_negative_hue
  Verifies any negative hue is rejected, however small.
  Tests the following:
  - a barely-negative and a large negative hue are both invalid
*/
bool
tests_color_hsl_is_valid_rejects_negative_hue()
{
    D_CL_CHECK(!hsl(channel_t(-0.001), channel_t(0.5), channel_t(0.5)).is_valid());
    D_CL_CHECK(!hsl(channel_t(-1), channel_t(0.5), channel_t(0.5)).is_valid());
    D_CL_CHECK(!hsl(channel_t(-360), channel_t(0.5), channel_t(0.5)).is_valid());

    return true;
}

/*
tests_color_hsl_is_valid_unit_bounds_are_inclusive
  Verifies saturation and lightness of exactly 0 and exactly 1 are valid - both
  unit intervals are closed.
  Tests the following:
  - each of the four bound combinations is accepted
*/
bool
tests_color_hsl_is_valid_unit_bounds_are_inclusive()
{
    D_CL_CHECK(hsl(channel_t(90), channel_t(0), channel_t(0)).is_valid());
    D_CL_CHECK(hsl(channel_t(90), channel_t(0), channel_t(1)).is_valid());
    D_CL_CHECK(hsl(channel_t(90), channel_t(1), channel_t(0)).is_valid());
    D_CL_CHECK(hsl(channel_t(90), channel_t(1), channel_t(1)).is_valid());

    return true;
}

/*
tests_color_hsl_is_valid_rejects_out_of_unit_range
  Verifies saturation or lightness outside [0, 1] invalidates the color, each
  channel independently.
  Tests the following:
  - below-zero and above-one probes on each unit channel in isolation
*/
bool
tests_color_hsl_is_valid_rejects_out_of_unit_range()
{
    const channel_t h = channel_t(90);

    D_CL_CHECK(!hsl(h, channel_t(-0.001), channel_t(0.5)).is_valid());
    D_CL_CHECK(!hsl(h, channel_t(1.001), channel_t(0.5)).is_valid());
    D_CL_CHECK(!hsl(h, channel_t(0.5), channel_t(-0.001)).is_valid());
    D_CL_CHECK(!hsl(h, channel_t(0.5), channel_t(1.001)).is_valid());

    D_CL_CHECK(!hsl(h, channel_t(-1), channel_t(2)).is_valid());

    // the reference value on its own is fine, isolating each perturbation
    D_CL_CHECK(hsl(h, channel_t(0.5), channel_t(0.5)).is_valid());

    return true;
}

/*
tests_color_hsl_is_valid_rejects_non_finite
  Verifies NaN and the infinities are rejected on every channel.  NaN fails
  because every comparison against it is false, so the conjunction collapses.
  Tests the following:
  - a NaN on any channel is rejected
  - positive and negative infinity are rejected on any channel
*/
bool
tests_color_hsl_is_valid_rejects_non_finite()
{
    const channel_t nan = cl_nan();
    const channel_t inf = cl_inf();
    const channel_t ok  = channel_t(0.5);

    D_CL_CHECK(!hsl(nan, ok, ok).is_valid());
    D_CL_CHECK(!hsl(channel_t(90), nan, ok).is_valid());
    D_CL_CHECK(!hsl(channel_t(90), ok, nan).is_valid());

    D_CL_CHECK(!hsl(inf, ok, ok).is_valid());
    D_CL_CHECK(!hsl(-inf, ok, ok).is_valid());
    D_CL_CHECK(!hsl(channel_t(90), inf, ok).is_valid());
    D_CL_CHECK(!hsl(channel_t(90), ok, -inf).is_valid());

    return true;
}

/*
tests_color_hsl_is_valid_is_constexpr
  Verifies is_valid is usable in a constant expression.
  Tests the following:
  - a valid and an invalid probe both evaluate at compile time
*/
bool
tests_color_hsl_is_valid_is_constexpr()
{
    D_CONSTEXPR hsl good(channel_t(180), channel_t(0), channel_t(1));
    D_CONSTEXPR hsl bad(channel_t(360), channel_t(0), channel_t(1));

    D_CONSTEXPR bool good_v = good.is_valid();
    D_CONSTEXPR bool bad_v  = bad.is_valid();

    static_assert(good_v, "is_valid true case must be constexpr");
    static_assert(!bad_v, "is_valid false case must be constexpr");

    D_CL_CHECK(good_v);
    D_CL_CHECK(!bad_v);

    return true;
}

/*
tests_color_hsl_is_valid_agrees_with_kernel
  Verifies the wrapper's verdict matches d_color_hsl_is_valid on the sliced
  POD - the member adds no policy of its own.
  Tests the following:
  - wrapper and kernel agree across valid, boundary, out-of-range and
    non-finite probes
*/
bool
tests_color_hsl_is_valid_agrees_with_kernel()
{
    const hsl probes[] = {
        hsl(),
        hsl(channel_t(0), channel_t(0), channel_t(0)),
        hsl(channel_t(359.9), channel_t(1), channel_t(1)),
        hsl(channel_t(360), channel_t(0.5), channel_t(0.5)),
        hsl(channel_t(-1), channel_t(0.5), channel_t(0.5)),
        hsl(channel_t(90), channel_t(-0.5), channel_t(0.5)),
        hsl(channel_t(90), channel_t(0.5), channel_t(1.5)),
        hsl(cl_nan(), channel_t(0.5), channel_t(0.5)),
        hsl(cl_inf(), channel_t(0.5), channel_t(0.5)),
    };

    std::size_t i = 0;
    for (i = 0; i < sizeof(probes) / sizeof(probes[0]); ++i)
    {
        const d_color_hsl pod = probes[i];
        D_CL_CHECK(probes[i].is_valid() == d_color_hsl_is_valid(pod));
    }

    return true;
}

NS_END
NS_END
