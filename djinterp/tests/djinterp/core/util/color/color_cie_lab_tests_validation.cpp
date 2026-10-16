/*******************************************************************************
* djinterp [core]                             color_cie_lab_tests_validation.cpp
*
*
* path:      /tests/djinterp/core/util/color/color_cie_lab_tests_validation.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "color_cie_lab_tests.hpp"

NS_DJINTERP
NS_TESTING

/*
tests_color_cie_xyz_is_valid_accepts_non_negative
  Verifies non-negative tristimulus values are valid.
  Tests the following:
  - the D65 white point and several interior probes are accepted
*/
bool
tests_color_cie_xyz_is_valid_accepts_non_negative()
{
    D_CE_CHECK(cie_xyz(channel_t(0.95047), channel_t(1),
                       channel_t(1.08883)).is_valid());
    D_CE_CHECK(cie_xyz(channel_t(0.25), channel_t(0.5),
                       channel_t(0.75)).is_valid());
    D_CE_CHECK(cie_xyz(channel_t(1), channel_t(1), channel_t(1)).is_valid());

    return true;
}

/*
tests_color_cie_xyz_is_valid_zero_is_valid
  Verifies the all-zero tristimulus is valid - the orthant is closed at zero,
  and zero is the legitimate encoding of black.
  Tests the following:
  - an explicit all-zero value and the default-constructed value are both
    accepted
  - a single zero axis alongside positive ones is fine
*/
bool
tests_color_cie_xyz_is_valid_zero_is_valid()
{
    D_CE_CHECK(cie_xyz(channel_t(0), channel_t(0), channel_t(0)).is_valid());
    D_CE_CHECK(cie_xyz().is_valid());

    D_CE_CHECK(cie_xyz(channel_t(0), channel_t(1), channel_t(1)).is_valid());
    D_CE_CHECK(cie_xyz(channel_t(1), channel_t(0), channel_t(1)).is_valid());
    D_CE_CHECK(cie_xyz(channel_t(1), channel_t(1), channel_t(0)).is_valid());

    return true;
}

/*
tests_color_cie_xyz_is_valid_has_no_upper_bound
  Verifies arbitrarily large tristimulus values remain valid: the predicate
  constrains only the sign.  This is deliberate - XYZ is a linear-light hub
  and must accommodate emissive sources brighter than the reference white.
  Tests the following:
  - values far above the white point are accepted
  - even positive infinity is accepted, since it too satisfies "not negative"
*/
bool
tests_color_cie_xyz_is_valid_has_no_upper_bound()
{
    D_CE_CHECK(cie_xyz(channel_t(2), channel_t(2), channel_t(2)).is_valid());
    D_CE_CHECK(cie_xyz(channel_t(1e6), channel_t(1e6),
                       channel_t(1e6)).is_valid());
    D_CE_CHECK(cie_xyz(channel_t(1e30), channel_t(1e30),
                       channel_t(1e30)).is_valid());

    // the predicate tests only the lower bound, so +inf passes
    D_CE_CHECK(cie_xyz(ce_inf(), ce_inf(), ce_inf()).is_valid());

    return true;
}

/*
tests_color_cie_xyz_is_valid_rejects_negative_per_channel
  Verifies a negative value on any single axis invalidates the whole
  tristimulus, independently of the others.
  Tests the following:
  - each axis pushed below zero in isolation is rejected
  - the reference value on its own is accepted, isolating each perturbation
*/
bool
tests_color_cie_xyz_is_valid_rejects_negative_per_channel()
{
    const channel_t ok = channel_t(1);

    D_CE_CHECK(!cie_xyz(channel_t(-0.001), ok, ok).is_valid());
    D_CE_CHECK(!cie_xyz(ok, channel_t(-0.001), ok).is_valid());
    D_CE_CHECK(!cie_xyz(ok, ok, channel_t(-0.001)).is_valid());

    D_CE_CHECK(!cie_xyz(channel_t(-1), channel_t(-1),
                        channel_t(-1)).is_valid());
    D_CE_CHECK(!cie_xyz(-ce_inf(), ok, ok).is_valid());

    D_CE_CHECK(cie_xyz(ok, ok, ok).is_valid());

    return true;
}

/*
tests_color_cie_xyz_is_valid_rejects_nan
  Verifies NaN is rejected on every axis.  It fails because the comparison
  against zero is false for NaN, so the conjunction collapses - the same
  mechanism that rejects a negative.
  Tests the following:
  - a NaN on any axis is rejected
  - negative infinity is rejected too
*/
bool
tests_color_cie_xyz_is_valid_rejects_nan()
{
    const channel_t nan = ce_nan();
    const channel_t ok  = channel_t(1);

    D_CE_CHECK(!cie_xyz(nan, ok, ok).is_valid());
    D_CE_CHECK(!cie_xyz(ok, nan, ok).is_valid());
    D_CE_CHECK(!cie_xyz(ok, ok, nan).is_valid());
    D_CE_CHECK(!cie_xyz(nan, nan, nan).is_valid());

    D_CE_CHECK(!cie_xyz(-ce_inf(), ok, ok).is_valid());
    D_CE_CHECK(!cie_xyz(ok, -ce_inf(), ok).is_valid());

    return true;
}

/*
tests_color_cie_xyz_is_valid_agrees_with_kernel
  Verifies the wrapper's verdict matches d_color_xyz_is_valid on the sliced
  POD - the member adds no policy of its own.
  Tests the following:
  - wrapper and kernel agree across zero, interior, huge, negative and
    non-finite probes
*/
bool
tests_color_cie_xyz_is_valid_agrees_with_kernel()
{
    const cie_xyz probes[] = {
        cie_xyz(),
        cie_xyz(channel_t(0.95047), channel_t(1), channel_t(1.08883)),
        cie_xyz(channel_t(1e30), channel_t(1e30), channel_t(1e30)),
        cie_xyz(channel_t(-0.001), channel_t(1), channel_t(1)),
        cie_xyz(channel_t(1), channel_t(-1), channel_t(1)),
        cie_xyz(ce_nan(), channel_t(1), channel_t(1)),
        cie_xyz(ce_inf(), ce_inf(), ce_inf()),
        cie_xyz(-ce_inf(), channel_t(1), channel_t(1)),
    };

    std::size_t i = 0;
    for (i = 0; i < sizeof(probes) / sizeof(probes[0]); ++i)
    {
        const d_color_xyz pod = probes[i];
        D_CE_CHECK(probes[i].is_valid() == d_color_xyz_is_valid(pod));
    }

    return true;
}

/*
tests_color_cie_lab_is_valid_accepts_lightness_range
  Verifies L* inside [0, 100] is valid.
  Tests the following:
  - several interior lightness probes with canonical chroma are accepted
*/
bool
tests_color_cie_lab_is_valid_accepts_lightness_range()
{
    D_CE_CHECK(cie_lab(channel_t(50), channel_t(0), channel_t(0)).is_valid());
    D_CE_CHECK(cie_lab(channel_t(53.2408), channel_t(80.0925),
                       channel_t(67.2032)).is_valid());
    D_CE_CHECK(cie_lab(channel_t(76.0693), channel_t(0),
                       channel_t(0)).is_valid());
    D_CE_CHECK(cie_lab(channel_t(1), channel_t(-50),
                       channel_t(50)).is_valid());

    return true;
}

/*
tests_color_cie_lab_is_valid_lightness_bounds_are_inclusive
  Verifies L* of exactly 0 and exactly 100 is valid - both ends of the
  lightness interval are closed, so black and reference white are admissible.
  Tests the following:
  - L* == 0 and L* == 100 are accepted, with varied chroma
*/
bool
tests_color_cie_lab_is_valid_lightness_bounds_are_inclusive()
{
    D_CE_CHECK(cie_lab(channel_t(0), channel_t(0), channel_t(0)).is_valid());
    D_CE_CHECK(cie_lab(channel_t(100), channel_t(0), channel_t(0)).is_valid());
    D_CE_CHECK(cie_lab(channel_t(0), channel_t(50),
                       channel_t(-50)).is_valid());
    D_CE_CHECK(cie_lab(channel_t(100), channel_t(-127),
                       channel_t(127)).is_valid());

    return true;
}

/*
tests_color_cie_lab_is_valid_rejects_out_of_range_lightness
  Verifies L* below 0 or above 100 invalidates the color.
  Tests the following:
  - barely-out and far-out lightness on both sides are rejected
*/
bool
tests_color_cie_lab_is_valid_rejects_out_of_range_lightness()
{
    D_CE_CHECK(!cie_lab(channel_t(-0.001), channel_t(0),
                        channel_t(0)).is_valid());
    D_CE_CHECK(!cie_lab(channel_t(100.001), channel_t(0),
                        channel_t(0)).is_valid());
    D_CE_CHECK(!cie_lab(channel_t(-50), channel_t(0), channel_t(0)).is_valid());
    D_CE_CHECK(!cie_lab(channel_t(1000), channel_t(0),
                        channel_t(0)).is_valid());

    D_CE_CHECK(!cie_lab(ce_inf(), channel_t(0), channel_t(0)).is_valid());
    D_CE_CHECK(!cie_lab(-ce_inf(), channel_t(0), channel_t(0)).is_valid());

    return true;
}

/*
tests_color_cie_lab_is_valid_ignores_the_chroma_axes
  Verifies a* and b* are not checked at all: the predicate constrains only
  L*.  This is a deliberate asymmetry - the chroma axes are unbounded in
  principle, and their practical bounds belong to clamp, not to validity.
  Tests the following:
  - absurd chroma leaves the value valid so long as L* is in range
  - conversely, canonical chroma cannot rescue an out-of-range L*
*/
bool
tests_color_cie_lab_is_valid_ignores_the_chroma_axes()
{
    D_CE_CHECK(cie_lab(channel_t(50), channel_t(1000),
                       channel_t(-1000)).is_valid());
    D_CE_CHECK(cie_lab(channel_t(50), channel_t(1e9),
                       channel_t(-1e9)).is_valid());
    D_CE_CHECK(cie_lab(channel_t(50), channel_t(-1e30),
                       channel_t(1e30)).is_valid());

    // even a non-finite chroma axis does not invalidate
    D_CE_CHECK(cie_lab(channel_t(50), ce_inf(), -ce_inf()).is_valid());

    // but L* out of range is fatal regardless of how canonical chroma is
    D_CE_CHECK(!cie_lab(channel_t(101), channel_t(0), channel_t(0)).is_valid());

    return true;
}

/*
tests_color_cie_lab_is_valid_rejects_nan_lightness_only
  Verifies a NaN L* is rejected while a NaN on a* or b* is not - a direct
  consequence of the predicate's scope, and worth pinning down because it
  surprises.
  Tests the following:
  - NaN on L* is rejected (both range comparisons are false)
  - NaN on a* or b* leaves the value valid
  - such a value is nonetheless unequal to itself, so validity and equality
    disagree here
*/
bool
tests_color_cie_lab_is_valid_rejects_nan_lightness_only()
{
    const channel_t nan = ce_nan();

    D_CE_CHECK(!cie_lab(nan, channel_t(0), channel_t(0)).is_valid());

    const cie_lab nan_a(channel_t(50), nan, channel_t(0));
    const cie_lab nan_b(channel_t(50), channel_t(0), nan);

    D_CE_CHECK(nan_a.is_valid());
    D_CE_CHECK(nan_b.is_valid());

    // valid, yet not equal to itself
    D_CE_CHECK(!(nan_a == nan_a));
    D_CE_CHECK(!(nan_b == nan_b));

    return true;
}

/*
tests_color_cie_lab_is_valid_is_constexpr
  Verifies cie_lab is_valid is usable in a constant expression.
  Tests the following:
  - a valid and an invalid probe both evaluate at compile time
  - the chroma-ignoring behaviour also holds at compile time
*/
bool
tests_color_cie_lab_is_valid_is_constexpr()
{
    D_CONSTEXPR cie_lab good(channel_t(100), channel_t(0), channel_t(0));
    D_CONSTEXPR cie_lab bad(channel_t(100.5), channel_t(0), channel_t(0));
    D_CONSTEXPR cie_lab wild(channel_t(50), channel_t(1e9), channel_t(-1e9));

    D_CONSTEXPR bool good_v = good.is_valid();
    D_CONSTEXPR bool bad_v  = bad.is_valid();
    D_CONSTEXPR bool wild_v = wild.is_valid();

    static_assert(good_v, "is_valid true case must be constexpr");
    static_assert(!bad_v, "is_valid false case must be constexpr");
    static_assert(wild_v, "chroma is not inspected, even at compile time");

    D_CE_CHECK(good_v);
    D_CE_CHECK(!bad_v);
    D_CE_CHECK(wild_v);

    return true;
}

/*
tests_color_cie_lab_is_valid_agrees_with_kernel
  Verifies the wrapper's verdict matches d_color_lab_is_valid on the sliced
  POD - the member adds no policy of its own.
  Tests the following:
  - wrapper and kernel agree across in-range, boundary, out-of-range, wild
    chroma and non-finite probes
*/
bool
tests_color_cie_lab_is_valid_agrees_with_kernel()
{
    const cie_lab probes[] = {
        cie_lab(),
        cie_lab(channel_t(50), channel_t(25), channel_t(-25)),
        cie_lab(channel_t(0), channel_t(0), channel_t(0)),
        cie_lab(channel_t(100), channel_t(0), channel_t(0)),
        cie_lab(channel_t(-1), channel_t(0), channel_t(0)),
        cie_lab(channel_t(101), channel_t(0), channel_t(0)),
        cie_lab(channel_t(50), channel_t(1e9), channel_t(-1e9)),
        cie_lab(ce_nan(), channel_t(0), channel_t(0)),
        cie_lab(channel_t(50), ce_nan(), channel_t(0)),
        cie_lab(ce_inf(), channel_t(0), channel_t(0)),
    };

    std::size_t i = 0;
    for (i = 0; i < sizeof(probes) / sizeof(probes[0]); ++i)
    {
        const d_color_lab pod = probes[i];
        D_CE_CHECK(probes[i].is_valid() == d_color_lab_is_valid(pod));
    }

    return true;
}

NS_END
NS_END
