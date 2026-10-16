/*******************************************************************************
* djinterp [core]                                color_cmyk_tests_validation.cpp
*
*
* path:      /tests/djinterp/core/util/color/color_cmyk_tests_validation.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "color_cmyk_tests.hpp"

NS_DJINTERP
NS_TESTING

/*
tests_color_cmyk_is_valid_accepts_unit_range
  Verifies interior values in [0, 1] on every channel are valid.
  Tests the following:
  - a mid-gamut value and several interior probes are accepted
*/
bool
tests_color_cmyk_is_valid_accepts_unit_range()
{
    D_CK_CHECK(cmyk(channel_t(0.5), channel_t(0.5),
                    channel_t(0.5), channel_t(0.5)).is_valid());
    D_CK_CHECK(cmyk(channel_t(0.1), channel_t(0.25),
                    channel_t(0.75), channel_t(0.9)).is_valid());
    D_CK_CHECK(cmyk(channel_t(0.999), channel_t(0.001),
                    channel_t(0.5), channel_t(0.5)).is_valid());

    return true;
}

/*
tests_color_cmyk_is_valid_boundaries_are_inclusive
  Verifies channels of exactly 0 and exactly 1 are valid - the admissible
  interval is closed at both ends.
  Tests the following:
  - all-zero and all-one are valid
  - each channel individually at either bound is valid
*/
bool
tests_color_cmyk_is_valid_boundaries_are_inclusive()
{
    D_CK_CHECK(cmyk(channel_t(0), channel_t(0),
                    channel_t(0), channel_t(0)).is_valid());
    D_CK_CHECK(cmyk(channel_t(1), channel_t(1),
                    channel_t(1), channel_t(1)).is_valid());

    D_CK_CHECK(cmyk(channel_t(0), channel_t(1),
                    channel_t(0), channel_t(1)).is_valid());
    D_CK_CHECK(cmyk(channel_t(1), channel_t(0),
                    channel_t(1), channel_t(0)).is_valid());

    // just inside either bound is still valid
    D_CK_CHECK(cmyk(channel_t(0.0000001), channel_t(0.9999999),
                    channel_t(0.5), channel_t(0.5)).is_valid());

    return true;
}

/*
tests_color_cmyk_is_valid_rejects_below_zero_per_channel
  Verifies a negative value on any single channel invalidates the whole color,
  so no channel is skipped by the predicate.
  Tests the following:
  - each of c, m, y, k made negative in isolation is rejected
  - a barely-negative value is rejected (the bound is not fuzzy)
*/
bool
tests_color_cmyk_is_valid_rejects_below_zero_per_channel()
{
    const channel_t ok  = channel_t(0.5);
    const channel_t bad = channel_t(-0.001);

    D_CK_CHECK(!cmyk(bad, ok, ok, ok).is_valid());
    D_CK_CHECK(!cmyk(ok, bad, ok, ok).is_valid());
    D_CK_CHECK(!cmyk(ok, ok, bad, ok).is_valid());
    D_CK_CHECK(!cmyk(ok, ok, ok, bad).is_valid());

    D_CK_CHECK(!cmyk(channel_t(-1), ok, ok, ok).is_valid());
    D_CK_CHECK(!cmyk(-ck_inf(), ok, ok, ok).is_valid());

    // the reference value on its own is fine, isolating the perturbation
    D_CK_CHECK(cmyk(ok, ok, ok, ok).is_valid());

    return true;
}

/*
tests_color_cmyk_is_valid_rejects_above_one_per_channel
  Verifies an above-unity value on any single channel invalidates the whole
  color.
  Tests the following:
  - each of c, m, y, k pushed above one in isolation is rejected
  - a barely-above value is rejected
*/
bool
tests_color_cmyk_is_valid_rejects_above_one_per_channel()
{
    const channel_t ok  = channel_t(0.5);
    const channel_t bad = channel_t(1.001);

    D_CK_CHECK(!cmyk(bad, ok, ok, ok).is_valid());
    D_CK_CHECK(!cmyk(ok, bad, ok, ok).is_valid());
    D_CK_CHECK(!cmyk(ok, ok, bad, ok).is_valid());
    D_CK_CHECK(!cmyk(ok, ok, ok, bad).is_valid());

    D_CK_CHECK(!cmyk(channel_t(2), ok, ok, ok).is_valid());
    D_CK_CHECK(!cmyk(ok, ok, ok, ck_inf()).is_valid());

    return true;
}

/*
tests_color_cmyk_is_valid_rejects_non_finite
  Verifies NaN and the infinities are rejected on every channel.  NaN fails
  because every comparison against it is false, so the conjunction collapses.
  Tests the following:
  - a NaN on any channel is rejected
  - positive and negative infinity are rejected
*/
bool
tests_color_cmyk_is_valid_rejects_non_finite()
{
    const channel_t ok  = channel_t(0.5);
    const channel_t nan = ck_nan();
    const channel_t inf = ck_inf();

    D_CK_CHECK(!cmyk(nan, ok, ok, ok).is_valid());
    D_CK_CHECK(!cmyk(ok, nan, ok, ok).is_valid());
    D_CK_CHECK(!cmyk(ok, ok, nan, ok).is_valid());
    D_CK_CHECK(!cmyk(ok, ok, ok, nan).is_valid());

    D_CK_CHECK(!cmyk(inf, ok, ok, ok).is_valid());
    D_CK_CHECK(!cmyk(-inf, ok, ok, ok).is_valid());
    D_CK_CHECK(!cmyk(nan, nan, nan, nan).is_valid());

    return true;
}

/*
tests_color_cmyk_is_valid_default_constructed
  Verifies a default-constructed cmyk is valid, so the type's zero state needs
  no repair before use.
  Tests the following:
  - the defaulted value is valid and equals all-zero
*/
bool
tests_color_cmyk_is_valid_default_constructed()
{
    const cmyk v;

    D_CK_CHECK(v.is_valid());
    D_CK_CHECK(ck_same(v, channel_t(0), channel_t(0),
                          channel_t(0), channel_t(0)));

    return true;
}

/*
tests_color_cmyk_is_valid_is_constexpr
  Verifies is_valid is usable in a constant expression.
  Tests the following:
  - a valid and an invalid probe both evaluate at compile time
*/
bool
tests_color_cmyk_is_valid_is_constexpr()
{
    D_CONSTEXPR cmyk good(channel_t(0), channel_t(0.5),
                          channel_t(1), channel_t(0.25));
    D_CONSTEXPR cmyk bad(channel_t(-1), channel_t(0.5),
                         channel_t(1), channel_t(0.25));

    D_CONSTEXPR bool good_v = good.is_valid();
    D_CONSTEXPR bool bad_v  = bad.is_valid();

    static_assert(good_v, "is_valid true case must be constexpr");
    static_assert(!bad_v, "is_valid false case must be constexpr");

    D_CK_CHECK(good_v);
    D_CK_CHECK(!bad_v);

    return true;
}

/*
tests_color_cmyk_is_valid_agrees_with_kernel
  Verifies the wrapper's verdict matches d_color_cmyk_is_valid on the sliced
  POD - the member adds no policy of its own.
  Tests the following:
  - wrapper and kernel agree across valid, out-of-range and non-finite probes
*/
bool
tests_color_cmyk_is_valid_agrees_with_kernel()
{
    const cmyk probes[] = {
        cmyk(),
        cmyk(channel_t(0.5), channel_t(0.5), channel_t(0.5), channel_t(0.5)),
        cmyk(channel_t(0), channel_t(1), channel_t(0), channel_t(1)),
        cmyk(channel_t(-0.5), channel_t(0.5), channel_t(0.5), channel_t(0.5)),
        cmyk(channel_t(0.5), channel_t(1.5), channel_t(0.5), channel_t(0.5)),
        cmyk(ck_nan(), channel_t(0.5), channel_t(0.5), channel_t(0.5)),
        cmyk(ck_inf(), channel_t(0.5), channel_t(0.5), channel_t(0.5)),
    };

    std::size_t i = 0;
    for (i = 0; i < sizeof(probes) / sizeof(probes[0]); ++i)
    {
        const d_color_cmyk pod = probes[i];
        D_CK_CHECK(probes[i].is_valid() == d_color_cmyk_is_valid(pod));
    }

    return true;
}

NS_END
NS_END
