/*******************************************************************************
* djinterp [core]                             color_cie_lab_tests_comparison.cpp
*
*
* path:      /tests/djinterp/core/util/color/color_cie_lab_tests_comparison.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "color_cie_lab_tests.hpp"

NS_DJINTERP
NS_TESTING

/*
tests_color_cie_xyz_equality_is_reflexive
  Verifies a cie_xyz equals itself and equals a separately built copy of the
  same channels.
  Tests the following:
  - self-comparison holds
  - two independently constructed equal values compare equal
  - the default-constructed value equals an explicit all-zero value
*/
bool
tests_color_cie_xyz_equality_is_reflexive()
{
    const cie_xyz v(channel_t(0.95047), channel_t(1), channel_t(1.08883));

    D_CE_CHECK(v == v);

    const cie_xyz twin(channel_t(0.95047), channel_t(1), channel_t(1.08883));
    D_CE_CHECK(v == twin);

    const cie_xyz defaulted;
    const cie_xyz explicit_zero(channel_t(0), channel_t(0), channel_t(0));
    D_CE_CHECK(defaulted == explicit_zero);

    return true;
}

/*
tests_color_cie_xyz_equality_detects_each_channel
  Verifies changing X, Y or Z alone breaks equality, so no axis is omitted
  from the comparison.
  Tests the following:
  - three one-axis perturbations of a base value each compare unequal
*/
bool
tests_color_cie_xyz_equality_detects_each_channel()
{
    const cie_xyz base(channel_t(0.25), channel_t(0.5), channel_t(0.75));

    const cie_xyz diff_x(channel_t(0.9), channel_t(0.5), channel_t(0.75));
    const cie_xyz diff_y(channel_t(0.25), channel_t(0.1), channel_t(0.75));
    const cie_xyz diff_z(channel_t(0.25), channel_t(0.5), channel_t(0.2));

    D_CE_CHECK(!(base == diff_x));
    D_CE_CHECK(!(base == diff_y));
    D_CE_CHECK(!(base == diff_z));

    // the axes are distinct slots: permuting them is observable
    const cie_xyz permuted(channel_t(0.75), channel_t(0.5), channel_t(0.25));
    D_CE_CHECK(!(base == permuted));

    return true;
}

/*
tests_color_cie_xyz_equality_signed_zero_and_nan
  Verifies the IEEE corner cases inherited from comparing floats directly.
  Tests the following:
  - negative zero compares equal to positive zero
  - a NaN axis makes the value unequal even to itself
*/
bool
tests_color_cie_xyz_equality_signed_zero_and_nan()
{
    const cie_xyz pos_zero(channel_t(0), channel_t(0), channel_t(0));
    const cie_xyz neg_zero(-channel_t(0), -channel_t(0), -channel_t(0));

    D_CE_CHECK(pos_zero == neg_zero);

    const cie_xyz with_nan(ce_nan(), channel_t(1), channel_t(1));
    D_CE_CHECK(!(with_nan == with_nan));
    D_CE_CHECK(with_nan.x != with_nan.x);
    D_CE_CHECK(with_nan.y == with_nan.y);

    const cie_xyz nan_z(channel_t(1), channel_t(1), ce_nan());
    D_CE_CHECK(!(nan_z == nan_z));

    return true;
}

/*
tests_color_cie_xyz_equality_is_constexpr
  Verifies cie_xyz operator== is usable in a constant expression.
  Tests the following:
  - a true and a false comparison both evaluate at compile time
*/
bool
tests_color_cie_xyz_equality_is_constexpr()
{
    D_CONSTEXPR cie_xyz a(channel_t(0.25), channel_t(0.5), channel_t(0.75));
    D_CONSTEXPR cie_xyz b(channel_t(0.25), channel_t(0.5), channel_t(0.75));
    D_CONSTEXPR cie_xyz c(channel_t(0), channel_t(0), channel_t(0));

    D_CONSTEXPR bool same = (a == b);
    D_CONSTEXPR bool diff = (a == c);

    static_assert(same, "operator== true case must be constexpr");
    static_assert(!diff, "operator== false case must be constexpr");

    D_CE_CHECK(same);
    D_CE_CHECK(!diff);

    return true;
}

/*
tests_color_cie_lab_equality_is_reflexive
  Verifies a cie_lab equals itself and equals a separately built copy of the
  same channels.
  Tests the following:
  - self-comparison holds
  - two independently constructed equal values compare equal
  - the default-constructed value equals an explicit all-zero value
*/
bool
tests_color_cie_lab_equality_is_reflexive()
{
    const cie_lab v(channel_t(50), channel_t(25), channel_t(-25));

    D_CE_CHECK(v == v);

    const cie_lab twin(channel_t(50), channel_t(25), channel_t(-25));
    D_CE_CHECK(v == twin);

    const cie_lab defaulted;
    const cie_lab explicit_zero(channel_t(0), channel_t(0), channel_t(0));
    D_CE_CHECK(defaulted == explicit_zero);

    return true;
}

/*
tests_color_cie_lab_equality_detects_each_channel
  Verifies changing L*, a* or b* alone breaks equality, so no axis is omitted
  from the comparison.
  Tests the following:
  - three one-axis perturbations of a base value each compare unequal
  - swapping the signed chroma axes is observable
*/
bool
tests_color_cie_lab_equality_detects_each_channel()
{
    const cie_lab base(channel_t(50), channel_t(25), channel_t(-25));

    const cie_lab diff_l(channel_t(75), channel_t(25), channel_t(-25));
    const cie_lab diff_a(channel_t(50), channel_t(-25), channel_t(-25));
    const cie_lab diff_b(channel_t(50), channel_t(25), channel_t(25));

    D_CE_CHECK(!(base == diff_l));
    D_CE_CHECK(!(base == diff_a));
    D_CE_CHECK(!(base == diff_b));

    const cie_lab swapped(channel_t(50), channel_t(-25), channel_t(25));
    D_CE_CHECK(!(base == swapped));

    return true;
}

/*
tests_color_cie_lab_equality_is_exact_not_approximate
  Verifies a one-ulp channel difference compares unequal.  L*a*b* is a
  perceptual space, but operator== carries no perceptual tolerance - a
  delta-E comparison is a separate concern.
  Tests the following:
  - a value perturbed by one epsilon is not equal
  - approx_equal on the same pair DOES accept it, isolating the distinction
*/
bool
tests_color_cie_lab_equality_is_exact_not_approximate()
{
    const channel_t eps = std::numeric_limits<channel_t>::epsilon();

    const cie_lab base(channel_t(1), channel_t(0), channel_t(0));
    const cie_lab nudged(channel_t(1) + eps, channel_t(0), channel_t(0));

    D_CE_CHECK(!(base == nudged));
    D_CE_CHECK(base.l != nudged.l);

    D_CE_CHECK(approx_equal(base.l, nudged.l, eps));
    D_CE_CHECK(ce_near(base.l, nudged.l));

    return true;
}

/*
tests_color_cie_lab_equality_signed_zero_and_nan
  Verifies the IEEE corner cases inherited from comparing floats directly.
  The chroma axes are signed, so negative zero is a reachable value there.
  Tests the following:
  - negative zero compares equal to positive zero on every axis
  - a NaN axis makes the value unequal even to itself
*/
bool
tests_color_cie_lab_equality_signed_zero_and_nan()
{
    const cie_lab pos_zero(channel_t(0), channel_t(0), channel_t(0));
    const cie_lab neg_zero(-channel_t(0), -channel_t(0), -channel_t(0));

    D_CE_CHECK(pos_zero == neg_zero);

    const cie_lab with_nan(ce_nan(), channel_t(0), channel_t(0));
    D_CE_CHECK(!(with_nan == with_nan));
    D_CE_CHECK(with_nan.l != with_nan.l);

    // a NaN chroma axis also breaks equality, even though is_valid ignores it
    const cie_lab nan_a(channel_t(50), ce_nan(), channel_t(0));
    D_CE_CHECK(!(nan_a == nan_a));
    D_CE_CHECK(nan_a.is_valid());

    return true;
}

/*
tests_color_cie_lab_equality_is_constexpr
  Verifies cie_lab operator== is usable in a constant expression.
  Tests the following:
  - a true and a false comparison both evaluate at compile time
*/
bool
tests_color_cie_lab_equality_is_constexpr()
{
    D_CONSTEXPR cie_lab a(channel_t(50), channel_t(25), channel_t(-25));
    D_CONSTEXPR cie_lab b(channel_t(50), channel_t(25), channel_t(-25));
    D_CONSTEXPR cie_lab c(channel_t(0), channel_t(0), channel_t(0));

    D_CONSTEXPR bool same = (a == b);
    D_CONSTEXPR bool diff = (a == c);

    static_assert(same, "operator== true case must be constexpr");
    static_assert(!diff, "operator== false case must be constexpr");

    D_CE_CHECK(same);
    D_CE_CHECK(!diff);

    return true;
}

/*
tests_color_cie_lab_inequality_is_the_negation_of_equality
  Verifies operator!= agrees with the negation of operator== on both CIE
  models.  Each header declares only operator==; from C++20 the compiler
  rewrites != from it, so this check is gated on the language level.
  Tests the following:
  - != is false for equal values and true for a differing one
  - != always equals !(==), for cie_lab and for cie_xyz
*/
bool
tests_color_cie_lab_inequality_is_the_negation_of_equality()
{
    const cie_lab a(channel_t(50), channel_t(25), channel_t(-25));
    const cie_lab b(channel_t(50), channel_t(25), channel_t(-25));
    const cie_lab c(channel_t(50), channel_t(25), channel_t(0));

    const cie_xyz x(channel_t(1), channel_t(1), channel_t(1));
    const cie_xyz y(channel_t(1), channel_t(1), channel_t(1));
    const cie_xyz z(channel_t(1), channel_t(1), channel_t(2));

#if D_ENV_LANG_IS_CPP20_OR_HIGHER
    D_CE_CHECK(!(a != b));
    D_CE_CHECK(a != c);
    D_CE_CHECK((a != c) == !(a == c));
    D_CE_CHECK((a != b) == !(a == b));

    D_CE_CHECK(!(x != y));
    D_CE_CHECK(x != z);
    D_CE_CHECK((x != z) == !(x == z));
#else
    D_CE_CHECK(!!(a == b));
    D_CE_CHECK(!(a == c));
    D_CE_CHECK(!!(x == y));
    D_CE_CHECK(!(x == z));
#endif

    return true;
}

NS_END
NS_END
