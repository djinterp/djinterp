/*******************************************************************************
* djinterp [core]                               color_ycbcr_tests_validation.cpp
*
*
* path:      /tests/djinterp/core/util/color/color_ycbcr_tests_validation.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "color_ycbcr_tests.hpp"

NS_DJINTERP
NS_TESTING

/*
tests_color_ycbcr_is_valid_accepts_canonical_ranges
  Verifies interior values on all three channels are valid.
  Tests the following:
  - several mid-domain probes, including negative chroma, are accepted
*/
bool
tests_color_ycbcr_is_valid_accepts_canonical_ranges()
{
    D_CY_CHECK(ycbcr(channel_t(0.5), channel_t(0), channel_t(0)).is_valid());
    D_CY_CHECK(ycbcr(channel_t(0.25), channel_t(0.25),
                     channel_t(-0.25)).is_valid());
    D_CY_CHECK(ycbcr(channel_t(0.9), channel_t(-0.4),
                     channel_t(0.4)).is_valid());

    return true;
}

/*
tests_color_ycbcr_is_valid_luma_bounds_are_inclusive
  Verifies luma of exactly 0 and exactly 1 is valid - the luma interval is
  closed at both ends.
  Tests the following:
  - both luma bounds are accepted, with chroma held canonical
*/
bool
tests_color_ycbcr_is_valid_luma_bounds_are_inclusive()
{
    D_CY_CHECK(ycbcr(channel_t(0), channel_t(0), channel_t(0)).is_valid());
    D_CY_CHECK(ycbcr(channel_t(1), channel_t(0), channel_t(0)).is_valid());
    D_CY_CHECK(ycbcr(channel_t(1), channel_t(0.5),
                     channel_t(-0.5)).is_valid());

    return true;
}

/*
tests_color_ycbcr_is_valid_chroma_bounds_are_inclusive
  Verifies chroma of exactly -0.5 and exactly +0.5 is valid on both Cb and Cr.
  Tests the following:
  - all four sign/channel bound combinations are accepted
*/
bool
tests_color_ycbcr_is_valid_chroma_bounds_are_inclusive()
{
    const channel_t y = channel_t(0.5);

    D_CY_CHECK(ycbcr(y, channel_t(-0.5), channel_t(0)).is_valid());
    D_CY_CHECK(ycbcr(y, channel_t(0.5), channel_t(0)).is_valid());
    D_CY_CHECK(ycbcr(y, channel_t(0), channel_t(-0.5)).is_valid());
    D_CY_CHECK(ycbcr(y, channel_t(0), channel_t(0.5)).is_valid());
    D_CY_CHECK(ycbcr(y, channel_t(-0.5), channel_t(0.5)).is_valid());

    return true;
}

/*
tests_color_ycbcr_is_valid_rejects_out_of_range_luma
  Verifies luma below zero or above one invalidates the color.
  Tests the following:
  - barely-out and far-out luma on both sides are rejected
*/
bool
tests_color_ycbcr_is_valid_rejects_out_of_range_luma()
{
    D_CY_CHECK(!ycbcr(channel_t(-0.001), channel_t(0),
                      channel_t(0)).is_valid());
    D_CY_CHECK(!ycbcr(channel_t(1.001), channel_t(0),
                      channel_t(0)).is_valid());
    D_CY_CHECK(!ycbcr(channel_t(-1), channel_t(0), channel_t(0)).is_valid());
    D_CY_CHECK(!ycbcr(channel_t(2), channel_t(0), channel_t(0)).is_valid());

    return true;
}

/*
tests_color_ycbcr_is_valid_rejects_out_of_range_chroma
  Verifies either chroma channel outside [-0.5, 0.5] invalidates the color,
  independently of the other.
  Tests the following:
  - each of Cb and Cr pushed past either bound in isolation is rejected
*/
bool
tests_color_ycbcr_is_valid_rejects_out_of_range_chroma()
{
    const channel_t y = channel_t(0.5);

    D_CY_CHECK(!ycbcr(y, channel_t(-0.501), channel_t(0)).is_valid());
    D_CY_CHECK(!ycbcr(y, channel_t(0.501), channel_t(0)).is_valid());
    D_CY_CHECK(!ycbcr(y, channel_t(0), channel_t(-0.501)).is_valid());
    D_CY_CHECK(!ycbcr(y, channel_t(0), channel_t(0.501)).is_valid());

    D_CY_CHECK(!ycbcr(y, channel_t(1), channel_t(0)).is_valid());
    D_CY_CHECK(!ycbcr(y, channel_t(0), channel_t(-1)).is_valid());

    // the reference value on its own is fine, isolating each perturbation
    D_CY_CHECK(ycbcr(y, channel_t(0), channel_t(0)).is_valid());

    return true;
}

/*
tests_color_ycbcr_is_valid_chroma_admits_negatives_luma_does_not
  Verifies the asymmetry at the heart of this model: a negative chroma is
  valid while the same negative value on luma is not.
  Tests the following:
  - -0.25 is accepted on Cb and on Cr but rejected on Y
  - +0.75 is accepted on Y but rejected on both chroma channels
*/
bool
tests_color_ycbcr_is_valid_chroma_admits_negatives_luma_does_not()
{
    const channel_t neg = channel_t(-0.25);

    D_CY_CHECK(ycbcr(channel_t(0.5), neg, channel_t(0)).is_valid());
    D_CY_CHECK(ycbcr(channel_t(0.5), channel_t(0), neg).is_valid());
    D_CY_CHECK(!ycbcr(neg, channel_t(0), channel_t(0)).is_valid());

    const channel_t high = channel_t(0.75);

    D_CY_CHECK(ycbcr(high, channel_t(0), channel_t(0)).is_valid());
    D_CY_CHECK(!ycbcr(channel_t(0.5), high, channel_t(0)).is_valid());
    D_CY_CHECK(!ycbcr(channel_t(0.5), channel_t(0), high).is_valid());

    return true;
}

/*
tests_color_ycbcr_is_valid_rejects_non_finite
  Verifies NaN and the infinities are rejected on every channel.  NaN fails
  because every comparison against it is false, so the conjunction collapses.
  Tests the following:
  - a NaN on any channel is rejected
  - positive and negative infinity are rejected on any channel
*/
bool
tests_color_ycbcr_is_valid_rejects_non_finite()
{
    const channel_t nan = cy_nan();
    const channel_t inf = cy_inf();

    D_CY_CHECK(!ycbcr(nan, channel_t(0), channel_t(0)).is_valid());
    D_CY_CHECK(!ycbcr(channel_t(0.5), nan, channel_t(0)).is_valid());
    D_CY_CHECK(!ycbcr(channel_t(0.5), channel_t(0), nan).is_valid());

    D_CY_CHECK(!ycbcr(inf, channel_t(0), channel_t(0)).is_valid());
    D_CY_CHECK(!ycbcr(-inf, channel_t(0), channel_t(0)).is_valid());
    D_CY_CHECK(!ycbcr(channel_t(0.5), inf, channel_t(0)).is_valid());
    D_CY_CHECK(!ycbcr(channel_t(0.5), channel_t(0), -inf).is_valid());

    return true;
}

/*
tests_color_ycbcr_is_valid_is_constexpr
  Verifies is_valid is usable in a constant expression.
  Tests the following:
  - a valid and an invalid probe both evaluate at compile time
*/
bool
tests_color_ycbcr_is_valid_is_constexpr()
{
    D_CONSTEXPR ycbcr good(channel_t(1), channel_t(-0.5), channel_t(0.5));
    D_CONSTEXPR ycbcr bad(channel_t(1), channel_t(-0.6), channel_t(0.5));

    D_CONSTEXPR bool good_v = good.is_valid();
    D_CONSTEXPR bool bad_v  = bad.is_valid();

    static_assert(good_v, "is_valid true case must be constexpr");
    static_assert(!bad_v, "is_valid false case must be constexpr");

    D_CY_CHECK(good_v);
    D_CY_CHECK(!bad_v);

    return true;
}

/*
tests_color_ycbcr_is_valid_agrees_with_kernel
  Verifies the wrapper's verdict matches d_color_ycbcr_is_valid on the sliced
  POD - the member adds no policy of its own.
  Tests the following:
  - wrapper and kernel agree across valid, boundary, out-of-range and
    non-finite probes
*/
bool
tests_color_ycbcr_is_valid_agrees_with_kernel()
{
    const ycbcr probes[] = {
        ycbcr(),
        ycbcr(channel_t(0.5), channel_t(0), channel_t(0)),
        ycbcr(channel_t(1), channel_t(0.5), channel_t(-0.5)),
        ycbcr(channel_t(-0.1), channel_t(0), channel_t(0)),
        ycbcr(channel_t(0.5), channel_t(0.6), channel_t(0)),
        ycbcr(channel_t(0.5), channel_t(0), channel_t(-0.6)),
        ycbcr(cy_nan(), channel_t(0), channel_t(0)),
        ycbcr(cy_inf(), channel_t(0), channel_t(0)),
    };

    std::size_t i = 0;
    for (i = 0; i < sizeof(probes) / sizeof(probes[0]); ++i)
    {
        const d_color_ycbcr pod = probes[i];
        D_CY_CHECK(probes[i].is_valid() == d_color_ycbcr_is_valid(pod));
    }

    return true;
}

NS_END
NS_END
