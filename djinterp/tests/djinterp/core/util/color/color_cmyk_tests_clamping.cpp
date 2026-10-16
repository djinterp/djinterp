/*******************************************************************************
* djinterp [core]                                  color_cmyk_tests_clamping.cpp
*
*
* path:      /tests/djinterp/core/util/color/color_cmyk_tests_clamping.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "color_cmyk_tests.hpp"

NS_DJINTERP
NS_TESTING

/*
tests_color_cmyk_clamp_lifts_below_zero
  Verifies negative channels are raised to exactly zero.
  Tests the following:
  - an all-negative value clamps to all-zero
  - the result is exactly zero, not merely near it
*/
bool
tests_color_cmyk_clamp_lifts_below_zero()
{
    const cmyk v(channel_t(-1), channel_t(-0.001),
                 channel_t(-1000), channel_t(-0.5));
    const cmyk c = v.clamp();

    D_CK_CHECK(ck_same(c, channel_t(0), channel_t(0),
                          channel_t(0), channel_t(0)));
    D_CK_CHECK(c.c == channel_t(0));
    D_CK_CHECK(c.k == channel_t(0));

    return true;
}

/*
tests_color_cmyk_clamp_lowers_above_one
  Verifies above-unity channels are lowered to exactly one.
  Tests the following:
  - an all-above value clamps to all-one
  - a barely-above value clamps too (the bound is not fuzzy)
*/
bool
tests_color_cmyk_clamp_lowers_above_one()
{
    const cmyk v(channel_t(2), channel_t(1.001),
                 channel_t(1000), channel_t(1.5));
    const cmyk c = v.clamp();

    D_CK_CHECK(ck_same(c, channel_t(1), channel_t(1),
                          channel_t(1), channel_t(1)));

    const cmyk hair(channel_t(1.0000001), channel_t(1),
                    channel_t(1), channel_t(1));
    D_CK_CHECK(hair.clamp().c <= channel_t(1));

    return true;
}

/*
tests_color_cmyk_clamp_preserves_in_range
  Verifies channels already inside [0, 1], including the bounds, pass through
  bit-exact.
  Tests the following:
  - an interior value is unchanged
  - values exactly on 0 and on 1 are unchanged
*/
bool
tests_color_cmyk_clamp_preserves_in_range()
{
    const cmyk v(channel_t(0.125), channel_t(0.25),
                 channel_t(0.5), channel_t(0.75));
    D_CK_CHECK(v.clamp() == v);
    D_CK_CHECK(ck_same(v.clamp(), channel_t(0.125), channel_t(0.25),
                                  channel_t(0.5), channel_t(0.75)));

    const cmyk bounds(channel_t(0), channel_t(1),
                      channel_t(0), channel_t(1));
    D_CK_CHECK(bounds.clamp() == bounds);

    const cmyk zero;
    D_CK_CHECK(zero.clamp() == zero);

    return true;
}

/*
tests_color_cmyk_clamp_is_per_channel
  Verifies each channel is clamped independently; one saturating channel does
  not disturb the others.
  Tests the following:
  - a value mixing below-range, in-range and above-range channels resolves
    each on its own
*/
bool
tests_color_cmyk_clamp_is_per_channel()
{
    const cmyk mixed(channel_t(-0.5), channel_t(0.25),
                     channel_t(1.5), channel_t(1));
    const cmyk c = mixed.clamp();

    D_CK_CHECK(c.c == channel_t(0));       // lifted
    D_CK_CHECK(c.m == channel_t(0.25));    // untouched
    D_CK_CHECK(c.y == channel_t(1));       // lowered
    D_CK_CHECK(c.k == channel_t(1));       // on the bound, untouched

    const cmyk other(channel_t(0.25), channel_t(-2),
                     channel_t(0.75), channel_t(3));
    const cmyk d = other.clamp();
    D_CK_CHECK(ck_same(d, channel_t(0.25), channel_t(0),
                          channel_t(0.75), channel_t(1)));

    return true;
}

/*
tests_color_cmyk_clamp_is_idempotent
  Verifies clamping an already-clamped value changes nothing, so clamp is a
  projection.
  Tests the following:
  - clamp(clamp(v)) == clamp(v) for out-of-range, in-range and boundary inputs
*/
bool
tests_color_cmyk_clamp_is_idempotent()
{
    const cmyk probes[] = {
        cmyk(channel_t(-1), channel_t(2), channel_t(0.5), channel_t(-0.25)),
        cmyk(channel_t(0.1), channel_t(0.2), channel_t(0.3), channel_t(0.4)),
        cmyk(channel_t(0), channel_t(1), channel_t(0), channel_t(1)),
        cmyk(),
    };

    std::size_t i = 0;
    for (i = 0; i < sizeof(probes) / sizeof(probes[0]); ++i)
    {
        const cmyk once  = probes[i].clamp();
        const cmyk twice = once.clamp();
        D_CK_CHECK(once == twice);
    }

    return true;
}

/*
tests_color_cmyk_clamp_result_is_valid
  Verifies that for any finite input the clamped result satisfies is_valid -
  the postcondition that makes clamp a repair operation.
  Tests the following:
  - a spread of finite out-of-range inputs all become valid after clamping
*/
bool
tests_color_cmyk_clamp_result_is_valid()
{
    const cmyk probes[] = {
        cmyk(channel_t(-1), channel_t(-1), channel_t(-1), channel_t(-1)),
        cmyk(channel_t(2), channel_t(2), channel_t(2), channel_t(2)),
        cmyk(channel_t(-5), channel_t(0.5), channel_t(5), channel_t(0)),
        cmyk(channel_t(1e30), channel_t(-1e30),
             channel_t(0.5), channel_t(0.5)),
        cmyk(ck_inf(), -ck_inf(), channel_t(0.5), channel_t(0.5)),
    };

    std::size_t i = 0;
    for (i = 0; i < sizeof(probes) / sizeof(probes[0]); ++i)
    {
        D_CK_CHECK(probes[i].clamp().is_valid());
    }

    return true;
}

/*
tests_color_cmyk_clamp_non_finite
  Verifies non-finite behaviour: infinities saturate to the bounds, while NaN
  passes through unchanged because both range comparisons are false - so
  clamping a NaN channel does NOT make the value valid.
  Tests the following:
  - +inf clamps to 1 and -inf clamps to 0
  - a NaN channel survives clamping and leaves the result invalid
*/
bool
tests_color_cmyk_clamp_non_finite()
{
    const cmyk inf_v(ck_inf(), -ck_inf(), channel_t(0.5), channel_t(0.5));
    const cmyk ci = inf_v.clamp();

    D_CK_CHECK(ci.c == channel_t(1));
    D_CK_CHECK(ci.m == channel_t(0));
    D_CK_CHECK(ci.is_valid());

    const cmyk nan_v(ck_nan(), channel_t(0.5), channel_t(0.5), channel_t(0.5));
    const cmyk cn = nan_v.clamp();

    D_CK_CHECK(cn.c != cn.c);           // still NaN
    D_CK_CHECK(!cn.is_valid());         // and therefore still invalid
    D_CK_CHECK(cn.m == channel_t(0.5)); // the other channels are unaffected

    return true;
}

/*
tests_color_cmyk_clamp_is_constexpr
  Verifies clamp is usable in a constant expression.
  Tests the following:
  - clamping an out-of-range value evaluates at compile time, in both
    saturating directions
*/
bool
tests_color_cmyk_clamp_is_constexpr()
{
    D_CONSTEXPR cmyk raw(channel_t(-1), channel_t(2),
                         channel_t(0.5), channel_t(1));
    D_CONSTEXPR cmyk c = raw.clamp();

    static_assert(c.c == channel_t(0), "clamp low arm must be constexpr");
    static_assert(c.m == channel_t(1), "clamp high arm must be constexpr");
    static_assert(c.y == channel_t(0.5), "clamp pass-through must be constexpr");
    static_assert(c.k == channel_t(1), "clamp bound must be constexpr");

    D_CK_CHECK(c.c == channel_t(0));
    D_CK_CHECK(c.m == channel_t(1));

    return true;
}

/*
tests_color_cmyk_clamp_agrees_with_kernel
  Verifies the wrapper's result matches d_color_cmyk_clamp on the sliced POD -
  the member adds no policy of its own.
  Tests the following:
  - wrapper and kernel agree channel-for-channel across every probe
*/
bool
tests_color_cmyk_clamp_agrees_with_kernel()
{
    const cmyk probes[] = {
        cmyk(),
        cmyk(channel_t(0.5), channel_t(0.5), channel_t(0.5), channel_t(0.5)),
        cmyk(channel_t(-1), channel_t(2), channel_t(0), channel_t(1)),
        cmyk(channel_t(1e6), channel_t(-1e6),
             channel_t(0.25), channel_t(0.75)),
        cmyk(ck_inf(), -ck_inf(), channel_t(0), channel_t(1)),
    };

    std::size_t i = 0;
    for (i = 0; i < sizeof(probes) / sizeof(probes[0]); ++i)
    {
        const d_color_cmyk pod       = probes[i];
        const d_color_cmyk from_kern = d_color_cmyk_clamp(pod);
        const cmyk         from_wrap = probes[i].clamp();

        D_CK_CHECK(from_wrap.c == from_kern.c);
        D_CK_CHECK(from_wrap.m == from_kern.m);
        D_CK_CHECK(from_wrap.y == from_kern.y);
        D_CK_CHECK(from_wrap.k == from_kern.k);
    }

    return true;
}

NS_END
NS_END
