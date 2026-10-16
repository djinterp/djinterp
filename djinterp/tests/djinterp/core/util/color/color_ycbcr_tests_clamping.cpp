/*******************************************************************************
* djinterp [core]                                 color_ycbcr_tests_clamping.cpp
*
*
* path:      /tests/djinterp/core/util/color/color_ycbcr_tests_clamping.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "color_ycbcr_tests.hpp"

NS_DJINTERP
NS_TESTING

/*
tests_color_ycbcr_clamp_constrains_luma
  Verifies luma saturates onto [0, 1] in both directions.
  Tests the following:
  - below-zero luma lifts to 0 and above-one luma lowers to 1
  - the results are exact bounds
*/
bool
tests_color_ycbcr_clamp_constrains_luma()
{
    D_CY_CHECK(ycbcr(channel_t(-1), channel_t(0),
                     channel_t(0)).clamp().y == channel_t(0));
    D_CY_CHECK(ycbcr(channel_t(2), channel_t(0),
                     channel_t(0)).clamp().y == channel_t(1));
    D_CY_CHECK(ycbcr(channel_t(-0.001), channel_t(0),
                     channel_t(0)).clamp().y == channel_t(0));
    D_CY_CHECK(ycbcr(channel_t(1000), channel_t(0),
                     channel_t(0)).clamp().y == channel_t(1));

    return true;
}

/*
tests_color_ycbcr_clamp_constrains_chroma_symmetrically
  Verifies both chroma channels saturate onto [-0.5, 0.5], so a negative
  excursion clamps to -0.5 and not to zero - the signed interval is honoured.
  Tests the following:
  - Cb and Cr each clamp to -0.5 from below and +0.5 from above
  - a negative chroma is NOT lifted to zero
*/
bool
tests_color_ycbcr_clamp_constrains_chroma_symmetrically()
{
    const channel_t y = channel_t(0.5);

    D_CY_CHECK(ycbcr(y, channel_t(-2), channel_t(0)).clamp().cb
               == channel_t(-0.5));
    D_CY_CHECK(ycbcr(y, channel_t(2), channel_t(0)).clamp().cb
               == channel_t(0.5));
    D_CY_CHECK(ycbcr(y, channel_t(0), channel_t(-2)).clamp().cr
               == channel_t(-0.5));
    D_CY_CHECK(ycbcr(y, channel_t(0), channel_t(2)).clamp().cr
               == channel_t(0.5));

    // the negative bound is -0.5, not 0
    D_CY_CHECK(ycbcr(y, channel_t(-2), channel_t(0)).clamp().cb
               != channel_t(0));
    D_CY_CHECK(ycbcr(y, channel_t(-0.25), channel_t(0)).clamp().cb
               == channel_t(-0.25));

    return true;
}

/*
tests_color_ycbcr_clamp_preserves_in_range
  Verifies values already inside the domain, including the bounds, pass
  through bit-exact.
  Tests the following:
  - an interior value is unchanged
  - values exactly on every bound are unchanged
*/
bool
tests_color_ycbcr_clamp_preserves_in_range()
{
    const ycbcr v(channel_t(0.25), channel_t(0.125), channel_t(-0.375));
    D_CY_CHECK(v.clamp() == v);
    D_CY_CHECK(cy_same(v.clamp(), channel_t(0.25), channel_t(0.125),
                                  channel_t(-0.375)));

    const ycbcr bounds(channel_t(1), channel_t(-0.5), channel_t(0.5));
    D_CY_CHECK(bounds.clamp() == bounds);

    const ycbcr zero;
    D_CY_CHECK(zero.clamp() == zero);

    return true;
}

/*
tests_color_ycbcr_clamp_is_per_channel
  Verifies each channel is clamped independently under its own bounds.
  Tests the following:
  - a value mixing below-range, in-range and above-range channels resolves
    each on its own terms and against its own interval
*/
bool
tests_color_ycbcr_clamp_is_per_channel()
{
    const ycbcr mixed(channel_t(-1), channel_t(0.25), channel_t(9));
    const ycbcr c = mixed.clamp();

    D_CY_CHECK(c.y == channel_t(0));        // lifted to the luma low bound
    D_CY_CHECK(c.cb == channel_t(0.25));    // untouched
    D_CY_CHECK(c.cr == channel_t(0.5));     // lowered to the chroma high bound

    const ycbcr other(channel_t(2), channel_t(-9), channel_t(0));
    const ycbcr d = other.clamp();
    D_CY_CHECK(cy_same(d, channel_t(1), channel_t(-0.5), channel_t(0)));

    return true;
}

/*
tests_color_ycbcr_clamp_is_idempotent
  Verifies clamping an already-clamped value changes nothing, so clamp is a
  projection.
  Tests the following:
  - clamp(clamp(v)) == clamp(v) for out-of-range, in-range and boundary inputs
*/
bool
tests_color_ycbcr_clamp_is_idempotent()
{
    const ycbcr probes[] = {
        ycbcr(channel_t(-1), channel_t(2), channel_t(-2)),
        ycbcr(channel_t(0.5), channel_t(0.25), channel_t(-0.25)),
        ycbcr(channel_t(1), channel_t(-0.5), channel_t(0.5)),
        ycbcr(),
    };

    std::size_t i = 0;
    for (i = 0; i < sizeof(probes) / sizeof(probes[0]); ++i)
    {
        const ycbcr once  = probes[i].clamp();
        const ycbcr twice = once.clamp();
        D_CY_CHECK(once == twice);
    }

    return true;
}

/*
tests_color_ycbcr_clamp_result_is_valid
  Verifies that for any finite input the clamped result satisfies is_valid -
  the postcondition that makes clamp a repair operation.
  Tests the following:
  - a spread of finite out-of-domain inputs all become valid after clamping
*/
bool
tests_color_ycbcr_clamp_result_is_valid()
{
    const ycbcr probes[] = {
        ycbcr(channel_t(-1), channel_t(-1), channel_t(-1)),
        ycbcr(channel_t(2), channel_t(2), channel_t(2)),
        ycbcr(channel_t(-5), channel_t(0.25), channel_t(5)),
        ycbcr(channel_t(1e30), channel_t(-1e30), channel_t(0)),
        ycbcr(cy_inf(), -cy_inf(), cy_inf()),
    };

    std::size_t i = 0;
    for (i = 0; i < sizeof(probes) / sizeof(probes[0]); ++i)
    {
        D_CY_CHECK(probes[i].clamp().is_valid());
    }

    return true;
}

/*
tests_color_ycbcr_clamp_non_finite
  Verifies non-finite behaviour: infinities saturate to the bounds, while NaN
  passes through unchanged because both range comparisons are false - so
  clamping a NaN channel does NOT make the value valid.
  Tests the following:
  - +inf and -inf saturate to the correct per-channel bounds
  - a NaN channel survives clamping and leaves the result invalid
*/
bool
tests_color_ycbcr_clamp_non_finite()
{
    const ycbcr inf_v(cy_inf(), -cy_inf(), cy_inf());
    const ycbcr ci = inf_v.clamp();

    D_CY_CHECK(ci.y == channel_t(1));
    D_CY_CHECK(ci.cb == channel_t(-0.5));
    D_CY_CHECK(ci.cr == channel_t(0.5));
    D_CY_CHECK(ci.is_valid());

    const ycbcr nan_v(cy_nan(), channel_t(0.25), channel_t(-0.25));
    const ycbcr cn = nan_v.clamp();

    D_CY_CHECK(cn.y != cn.y);            // still NaN
    D_CY_CHECK(!cn.is_valid());          // and therefore still invalid
    D_CY_CHECK(cn.cb == channel_t(0.25));
    D_CY_CHECK(cn.cr == channel_t(-0.25));

    return true;
}

/*
tests_color_ycbcr_clamp_is_constexpr
  Verifies clamp is usable in a constant expression.
  Tests the following:
  - luma and chroma saturation both evaluate at compile time
*/
bool
tests_color_ycbcr_clamp_is_constexpr()
{
    D_CONSTEXPR ycbcr raw(channel_t(-1), channel_t(2), channel_t(-2));
    D_CONSTEXPR ycbcr c = raw.clamp();

    static_assert(c.y == channel_t(0), "luma low arm must be constexpr");
    static_assert(c.cb == channel_t(0.5), "chroma high arm must be constexpr");
    static_assert(c.cr == channel_t(-0.5), "chroma low arm must be constexpr");

    D_CY_CHECK(c.y == channel_t(0));
    D_CY_CHECK(c.cb == channel_t(0.5));
    D_CY_CHECK(c.cr == channel_t(-0.5));

    return true;
}

/*
tests_color_ycbcr_clamp_agrees_with_kernel
  Verifies the wrapper's result matches d_color_ycbcr_clamp on the sliced POD -
  the member adds no policy of its own.
  Tests the following:
  - wrapper and kernel agree channel-for-channel across every probe
*/
bool
tests_color_ycbcr_clamp_agrees_with_kernel()
{
    const ycbcr probes[] = {
        ycbcr(),
        ycbcr(channel_t(0.5), channel_t(0.25), channel_t(-0.25)),
        ycbcr(channel_t(-1), channel_t(2), channel_t(-2)),
        ycbcr(channel_t(1e6), channel_t(-1e6), channel_t(0.25)),
        ycbcr(channel_t(1), channel_t(-0.5), channel_t(0.5)),
        ycbcr(cy_inf(), -cy_inf(), channel_t(0)),
    };

    std::size_t i = 0;
    for (i = 0; i < sizeof(probes) / sizeof(probes[0]); ++i)
    {
        const d_color_ycbcr pod       = probes[i];
        const d_color_ycbcr from_kern = d_color_ycbcr_clamp(pod);
        const ycbcr         from_wrap = probes[i].clamp();

        D_CY_CHECK(from_wrap.y == from_kern.y);
        D_CY_CHECK(from_wrap.cb == from_kern.cb);
        D_CY_CHECK(from_wrap.cr == from_kern.cr);
    }

    return true;
}

NS_END
NS_END
