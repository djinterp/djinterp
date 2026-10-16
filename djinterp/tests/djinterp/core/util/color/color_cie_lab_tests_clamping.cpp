/*******************************************************************************
* djinterp [core]                               color_cie_lab_tests_clamping.cpp
*
*
* path:      /tests/djinterp/core/util/color/color_cie_lab_tests_clamping.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "color_cie_lab_tests.hpp"

NS_DJINTERP
NS_TESTING

/*
tests_color_cie_xyz_clamp_lifts_negative_to_zero
  Verifies negative tristimulus values are raised to exactly zero.
  Tests the following:
  - each axis is lifted independently
  - the result is exactly zero, not merely non-negative
*/
bool
tests_color_cie_xyz_clamp_lifts_negative_to_zero()
{
    const cie_xyz v(channel_t(-1), channel_t(-2), channel_t(-3));
    const cie_xyz c = v.clamp();

    D_CE_CHECK(ce_xyz_same(c, channel_t(0), channel_t(0), channel_t(0)));

    D_CE_CHECK(cie_xyz(channel_t(-0.001), channel_t(1),
                       channel_t(1)).clamp().x == channel_t(0));
    D_CE_CHECK(cie_xyz(channel_t(1), channel_t(-0.001),
                       channel_t(1)).clamp().y == channel_t(0));
    D_CE_CHECK(cie_xyz(channel_t(1), channel_t(1),
                       channel_t(-0.001)).clamp().z == channel_t(0));

    D_CE_CHECK(cie_xyz(-ce_inf(), channel_t(1),
                       channel_t(1)).clamp().x == channel_t(0));

    return true;
}

/*
tests_color_cie_xyz_clamp_preserves_non_negative
  Verifies non-negative values pass through bit-exact, so clamp is inert on
  canonical input.
  Tests the following:
  - the D65 white point and an interior value survive unchanged
  - a zero axis is left at zero rather than perturbed
*/
bool
tests_color_cie_xyz_clamp_preserves_non_negative()
{
    const cie_xyz white(channel_t(0.95047), channel_t(1), channel_t(1.08883));
    D_CE_CHECK(white.clamp() == white);

    const cie_xyz interior(channel_t(0.25), channel_t(0.5), channel_t(0.75));
    D_CE_CHECK(interior.clamp() == interior);

    const cie_xyz zero;
    D_CE_CHECK(zero.clamp() == zero);
    D_CE_CHECK(zero.clamp().x == channel_t(0));

    return true;
}

/*
tests_color_cie_xyz_clamp_has_no_upper_bound
  Verifies large values are NOT lowered - clamp mirrors is_valid in
  constraining only the sign, so a bright emissive tristimulus survives the
  round trip through canonicalisation.
  Tests the following:
  - values far above the white point pass through unchanged
  - positive infinity is likewise untouched
*/
bool
tests_color_cie_xyz_clamp_has_no_upper_bound()
{
    const cie_xyz big(channel_t(1e6), channel_t(1e6), channel_t(1e6));
    D_CE_CHECK(big.clamp() == big);
    D_CE_CHECK(big.clamp().x == channel_t(1e6));

    const cie_xyz huge(channel_t(1e30), channel_t(2e30), channel_t(3e30));
    D_CE_CHECK(huge.clamp() == huge);

    const cie_xyz infinite(ce_inf(), ce_inf(), ce_inf());
    D_CE_CHECK(infinite.clamp().x == ce_inf());

    return true;
}

/*
tests_color_cie_xyz_clamp_is_idempotent
  Verifies clamping an already-clamped tristimulus changes nothing, so clamp
  is a projection onto the non-negative orthant.
  Tests the following:
  - clamp(clamp(v)) == clamp(v) for negative, interior, zero and large inputs
*/
bool
tests_color_cie_xyz_clamp_is_idempotent()
{
    const cie_xyz probes[] = {
        cie_xyz(channel_t(-1), channel_t(-2), channel_t(-3)),
        cie_xyz(channel_t(0.25), channel_t(0.5), channel_t(0.75)),
        cie_xyz(),
        cie_xyz(channel_t(1e30), channel_t(0), channel_t(-1)),
    };

    std::size_t i = 0;
    for (i = 0; i < sizeof(probes) / sizeof(probes[0]); ++i)
    {
        const cie_xyz once  = probes[i].clamp();
        const cie_xyz twice = once.clamp();
        D_CE_CHECK(once == twice);
    }

    return true;
}

/*
tests_color_cie_xyz_clamp_result_is_valid
  Verifies that for any non-NaN input the clamped tristimulus satisfies
  is_valid.  NaN is excluded deliberately: the comparison against zero is
  false for NaN, so it passes through the clamp untouched and the result
  stays invalid.
  Tests the following:
  - a spread of negative and mixed inputs all become valid after clamping
  - a NaN axis is documented as the exception: it survives and stays invalid
*/
bool
tests_color_cie_xyz_clamp_result_is_valid()
{
    const cie_xyz probes[] = {
        cie_xyz(channel_t(-1), channel_t(-1), channel_t(-1)),
        cie_xyz(channel_t(-1), channel_t(1), channel_t(-1)),
        cie_xyz(channel_t(1e30), channel_t(-1e30), channel_t(0)),
        cie_xyz(-ce_inf(), -ce_inf(), -ce_inf()),
        cie_xyz(),
    };

    std::size_t i = 0;
    for (i = 0; i < sizeof(probes) / sizeof(probes[0]); ++i)
    {
        D_CE_CHECK(probes[i].clamp().is_valid());
    }

    // the documented exception
    const cie_xyz nan_v(ce_nan(), channel_t(1), channel_t(1));
    D_CE_CHECK(nan_v.clamp().x != nan_v.clamp().x);
    D_CE_CHECK(!nan_v.clamp().is_valid());

    return true;
}

/*
tests_color_cie_xyz_clamp_is_constexpr
  Verifies cie_xyz clamp is usable in a constant expression.
  Tests the following:
  - the lifting and pass-through arms both evaluate at compile time
*/
bool
tests_color_cie_xyz_clamp_is_constexpr()
{
    D_CONSTEXPR cie_xyz raw(channel_t(-1), channel_t(0.5), channel_t(2));
    D_CONSTEXPR cie_xyz c = raw.clamp();

    static_assert(c.x == channel_t(0), "lift arm must be constexpr");
    static_assert(c.y == channel_t(0.5), "pass-through must be constexpr");
    static_assert(c.z == channel_t(2), "no upper bound, at compile time too");

    D_CE_CHECK(c.x == channel_t(0));
    D_CE_CHECK(c.y == channel_t(0.5));
    D_CE_CHECK(c.z == channel_t(2));

    return true;
}

/*
tests_color_cie_xyz_clamp_agrees_with_kernel
  Verifies the wrapper's result matches d_color_xyz_clamp on the sliced POD -
  the member adds no policy of its own.
  Tests the following:
  - wrapper and kernel agree axis-for-axis across every probe
*/
bool
tests_color_cie_xyz_clamp_agrees_with_kernel()
{
    const cie_xyz probes[] = {
        cie_xyz(),
        cie_xyz(channel_t(0.95047), channel_t(1), channel_t(1.08883)),
        cie_xyz(channel_t(-1), channel_t(-2), channel_t(-3)),
        cie_xyz(channel_t(1e30), channel_t(-1e30), channel_t(0)),
        cie_xyz(ce_inf(), -ce_inf(), channel_t(1)),
    };

    std::size_t i = 0;
    for (i = 0; i < sizeof(probes) / sizeof(probes[0]); ++i)
    {
        const d_color_xyz pod       = probes[i];
        const d_color_xyz from_kern = d_color_xyz_clamp(pod);
        const cie_xyz     from_wrap = probes[i].clamp();

        D_CE_CHECK(from_wrap.x == from_kern.x);
        D_CE_CHECK(from_wrap.y == from_kern.y);
        D_CE_CHECK(from_wrap.z == from_kern.z);
    }

    return true;
}

/*
tests_color_cie_lab_clamp_constrains_lightness
  Verifies L* saturates onto [0, 100] in both directions.
  Tests the following:
  - below-zero lightness lifts to 0 and above-100 lowers to 100
  - the results are exact bounds
*/
bool
tests_color_cie_lab_clamp_constrains_lightness()
{
    D_CE_CHECK(cie_lab(channel_t(-50), channel_t(0),
                       channel_t(0)).clamp().l == channel_t(0));
    D_CE_CHECK(cie_lab(channel_t(150), channel_t(0),
                       channel_t(0)).clamp().l == channel_t(100));
    D_CE_CHECK(cie_lab(channel_t(-0.001), channel_t(0),
                       channel_t(0)).clamp().l == channel_t(0));
    D_CE_CHECK(cie_lab(channel_t(100.001), channel_t(0),
                       channel_t(0)).clamp().l == channel_t(100));

    // an in-range lightness is untouched
    D_CE_CHECK(cie_lab(channel_t(50), channel_t(0),
                       channel_t(0)).clamp().l == channel_t(50));

    return true;
}

/*
tests_color_cie_lab_clamp_constrains_the_chroma_axes
  Verifies a* and b* saturate onto the asymmetric [-128, 127] interval - the
  signed-byte range the kernel adopts for storage compatibility.
  Tests the following:
  - both axes clamp to -128 from below and to +127 from above
  - the interval is asymmetric: the low bound is not the negation of the high
  - in-range chroma passes through
*/
bool
tests_color_cie_lab_clamp_constrains_the_chroma_axes()
{
    const channel_t l = channel_t(50);

    D_CE_CHECK(cie_lab(l, channel_t(-1000), channel_t(0)).clamp().a
               == channel_t(-128));
    D_CE_CHECK(cie_lab(l, channel_t(1000), channel_t(0)).clamp().a
               == channel_t(127));
    D_CE_CHECK(cie_lab(l, channel_t(0), channel_t(-1000)).clamp().b
               == channel_t(-128));
    D_CE_CHECK(cie_lab(l, channel_t(0), channel_t(1000)).clamp().b
               == channel_t(127));

    // asymmetric by one: -128 is reachable but -127 is not the low bound
    D_CE_CHECK(cie_lab(l, channel_t(-1000), channel_t(1000)).clamp().a
               != -cie_lab(l, channel_t(-1000), channel_t(1000)).clamp().b);

    D_CE_CHECK(cie_lab(l, channel_t(80.0925), channel_t(67.2032)).clamp().a
               == channel_t(80.0925));

    return true;
}

/*
tests_color_cie_lab_clamp_is_stricter_than_is_valid
  Verifies the asymmetry between the two operations: clamp bounds a* and b*,
  which is_valid never inspects.  A cie_lab can therefore be VALID and still
  be changed by clamp - so "already valid" does not imply "already canonical",
  and the usual `v.is_valid() implies v.clamp() == v` shortcut is unsound for
  this model.
  Tests the following:
  - a value with in-range L* and absurd chroma is valid
  - clamping it nonetheless changes both chroma axes
  - the same shortcut DOES hold once chroma is inside the clamp interval
*/
bool
tests_color_cie_lab_clamp_is_stricter_than_is_valid()
{
    const cie_lab wild(channel_t(50), channel_t(1000), channel_t(-1000));

    D_CE_CHECK(wild.is_valid());          // valid: only L* is inspected
    D_CE_CHECK(!(wild.clamp() == wild));  // yet clamp still changes it

    D_CE_CHECK(wild.clamp().l == channel_t(50));
    D_CE_CHECK(wild.clamp().a == channel_t(127));
    D_CE_CHECK(wild.clamp().b == channel_t(-128));

    // where chroma is already inside the clamp interval, the two agree
    const cie_lab tame(channel_t(50), channel_t(25), channel_t(-25));
    D_CE_CHECK(tame.is_valid());
    D_CE_CHECK(tame.clamp() == tame);

    return true;
}

/*
tests_color_cie_lab_clamp_is_idempotent
  Verifies clamping an already-clamped cie_lab changes nothing, so clamp is a
  projection.
  Tests the following:
  - clamp(clamp(v)) == clamp(v) for out-of-range, wild-chroma, in-range and
    boundary inputs
*/
bool
tests_color_cie_lab_clamp_is_idempotent()
{
    const cie_lab probes[] = {
        cie_lab(channel_t(-50), channel_t(1000), channel_t(-1000)),
        cie_lab(channel_t(150), channel_t(0), channel_t(0)),
        cie_lab(channel_t(50), channel_t(25), channel_t(-25)),
        cie_lab(channel_t(100), channel_t(127), channel_t(-128)),
        cie_lab(),
    };

    std::size_t i = 0;
    for (i = 0; i < sizeof(probes) / sizeof(probes[0]); ++i)
    {
        const cie_lab once  = probes[i].clamp();
        const cie_lab twice = once.clamp();
        D_CE_CHECK(once == twice);
    }

    return true;
}

/*
tests_color_cie_lab_clamp_result_is_valid
  Verifies that for any non-NaN input the clamped result satisfies is_valid.
  Tests the following:
  - a spread of out-of-range inputs all become valid after clamping
  - a NaN L* is the exception: it survives the clamp and stays invalid
*/
bool
tests_color_cie_lab_clamp_result_is_valid()
{
    const cie_lab probes[] = {
        cie_lab(channel_t(-50), channel_t(0), channel_t(0)),
        cie_lab(channel_t(150), channel_t(0), channel_t(0)),
        cie_lab(channel_t(-1e30), channel_t(1e30), channel_t(-1e30)),
        cie_lab(ce_inf(), ce_inf(), -ce_inf()),
        cie_lab(),
    };

    std::size_t i = 0;
    for (i = 0; i < sizeof(probes) / sizeof(probes[0]); ++i)
    {
        D_CE_CHECK(probes[i].clamp().is_valid());
    }

    const cie_lab nan_l(ce_nan(), channel_t(0), channel_t(0));
    D_CE_CHECK(!nan_l.clamp().is_valid());

    return true;
}

/*
tests_color_cie_lab_clamp_non_finite
  Verifies non-finite behaviour: infinities saturate to the bounds, while NaN
  passes through unchanged because both range comparisons are false.
  Tests the following:
  - +inf and -inf saturate to the correct per-axis bounds
  - a NaN axis survives clamping unchanged
*/
bool
tests_color_cie_lab_clamp_non_finite()
{
    const cie_lab inf_v(ce_inf(), -ce_inf(), ce_inf());
    const cie_lab ci = inf_v.clamp();

    D_CE_CHECK(ci.l == channel_t(100));
    D_CE_CHECK(ci.a == channel_t(-128));
    D_CE_CHECK(ci.b == channel_t(127));
    D_CE_CHECK(ci.is_valid());

    const cie_lab nan_v(ce_nan(), ce_nan(), channel_t(25));
    const cie_lab cn = nan_v.clamp();

    D_CE_CHECK(cn.l != cn.l);            // still NaN
    D_CE_CHECK(cn.a != cn.a);            // still NaN
    D_CE_CHECK(cn.b == channel_t(25));   // untouched
    D_CE_CHECK(!cn.is_valid());

    return true;
}

/*
tests_color_cie_lab_clamp_is_constexpr
  Verifies cie_lab clamp is usable in a constant expression.
  Tests the following:
  - the lightness and chroma saturation arms all evaluate at compile time
*/
bool
tests_color_cie_lab_clamp_is_constexpr()
{
    D_CONSTEXPR cie_lab raw(channel_t(150), channel_t(1000),
                            channel_t(-1000));
    D_CONSTEXPR cie_lab c = raw.clamp();

    static_assert(c.l == channel_t(100), "L* high arm must be constexpr");
    static_assert(c.a == channel_t(127), "a* high arm must be constexpr");
    static_assert(c.b == channel_t(-128), "b* low arm must be constexpr");

    D_CE_CHECK(c.l == channel_t(100));
    D_CE_CHECK(c.a == channel_t(127));
    D_CE_CHECK(c.b == channel_t(-128));

    return true;
}

/*
tests_color_cie_lab_clamp_agrees_with_kernel
  Verifies the wrapper's result matches d_color_lab_clamp on the sliced POD -
  the member adds no policy of its own.
  Tests the following:
  - wrapper and kernel agree axis-for-axis across every probe
*/
bool
tests_color_cie_lab_clamp_agrees_with_kernel()
{
    const cie_lab probes[] = {
        cie_lab(),
        cie_lab(channel_t(50), channel_t(25), channel_t(-25)),
        cie_lab(channel_t(-50), channel_t(1000), channel_t(-1000)),
        cie_lab(channel_t(150), channel_t(-1e6), channel_t(1e6)),
        cie_lab(channel_t(100), channel_t(127), channel_t(-128)),
        cie_lab(ce_inf(), -ce_inf(), channel_t(0)),
    };

    std::size_t i = 0;
    for (i = 0; i < sizeof(probes) / sizeof(probes[0]); ++i)
    {
        const d_color_lab pod       = probes[i];
        const d_color_lab from_kern = d_color_lab_clamp(pod);
        const cie_lab     from_wrap = probes[i].clamp();

        D_CE_CHECK(from_wrap.l == from_kern.l);
        D_CE_CHECK(from_wrap.a == from_kern.a);
        D_CE_CHECK(from_wrap.b == from_kern.b);
    }

    return true;
}

NS_END
NS_END
