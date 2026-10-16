/*******************************************************************************
* djinterp [core]                                 color_hsl_tests_comparison.cpp
*
*
* path:      /tests/djinterp/core/util/color/color_hsl_tests_comparison.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "color_hsl_tests.hpp"

NS_DJINTERP
NS_TESTING

/*
tests_color_hsl_equality_is_reflexive
  Verifies a value equals itself and equals a separately built copy of the
  same channels.
  Tests the following:
  - self-comparison holds
  - two independently constructed equal values compare equal
  - the default-constructed value equals an explicit all-zero value
*/
bool
tests_color_hsl_equality_is_reflexive()
{
    const hsl v(channel_t(210), channel_t(0.5), channel_t(0.5));

    D_CL_CHECK(v == v);

    const hsl twin(channel_t(210), channel_t(0.5), channel_t(0.5));
    D_CL_CHECK(v == twin);

    const hsl defaulted;
    const hsl explicit_zero(channel_t(0), channel_t(0), channel_t(0));
    D_CL_CHECK(defaulted == explicit_zero);

    return true;
}

/*
tests_color_hsl_equality_detects_each_channel
  Verifies changing hue, saturation or lightness alone breaks equality, so no
  channel is omitted from the comparison.
  Tests the following:
  - three one-channel perturbations of a base value each compare unequal
*/
bool
tests_color_hsl_equality_detects_each_channel()
{
    const hsl base(channel_t(120), channel_t(0.3), channel_t(0.6));

    const hsl diff_h(channel_t(240), channel_t(0.3), channel_t(0.6));
    const hsl diff_s(channel_t(120), channel_t(0.9), channel_t(0.6));
    const hsl diff_t(channel_t(120), channel_t(0.3), channel_t(0.1));

    D_CL_CHECK(!(base == diff_h));
    D_CL_CHECK(!(base == diff_s));
    D_CL_CHECK(!(base == diff_t));

    return true;
}

/*
tests_color_hsl_equality_is_symmetric
  Verifies the comparison gives the same verdict in either operand order.
  Tests the following:
  - equal and unequal pairs agree under operand swap
*/
bool
tests_color_hsl_equality_is_symmetric()
{
    const hsl a(channel_t(60), channel_t(0.5), channel_t(0.5));
    const hsl b(channel_t(60), channel_t(0.5), channel_t(0.5));
    const hsl c(channel_t(300), channel_t(0.1), channel_t(0.9));

    D_CL_CHECK((a == b) == (b == a));
    D_CL_CHECK((a == c) == (c == a));
    D_CL_CHECK(a == b);
    D_CL_CHECK(!(a == c));

    return true;
}

/*
tests_color_hsl_equality_is_exact_not_approximate
  Verifies a one-ulp channel difference compares unequal - the operator is
  exact channel equality with no built-in tolerance.
  Tests the following:
  - a value perturbed by one epsilon is not equal
  - approx_equal on the same pair DOES accept it, isolating the distinction
*/
bool
tests_color_hsl_equality_is_exact_not_approximate()
{
    const channel_t eps = std::numeric_limits<channel_t>::epsilon();

    const hsl base(channel_t(1), channel_t(0.5), channel_t(0.25));
    const hsl nudged(channel_t(1) + eps, channel_t(0.5), channel_t(0.25));

    D_CL_CHECK(!(base == nudged));
    D_CL_CHECK(base.h != nudged.h);

    D_CL_CHECK(approx_equal(base.h, nudged.h, eps));
    D_CL_CHECK(cl_near(base.h, nudged.h));

    return true;
}

/*
tests_color_hsl_equality_signed_zero_and_nan
  Verifies the IEEE corner cases inherited from comparing floats directly.
  Tests the following:
  - negative zero compares equal to positive zero
  - a channel holding NaN makes the value unequal even to itself
  - the inequality is confined to the offending channel
*/
bool
tests_color_hsl_equality_signed_zero_and_nan()
{
    const hsl pos_zero(channel_t(0), channel_t(0), channel_t(0));
    const hsl neg_zero(-channel_t(0), -channel_t(0), -channel_t(0));

    D_CL_CHECK(pos_zero == neg_zero);

    const hsl with_nan(cl_nan(), channel_t(0.5), channel_t(0.5));
    D_CL_CHECK(!(with_nan == with_nan));
    D_CL_CHECK(with_nan.h != with_nan.h);
    D_CL_CHECK(with_nan.s == with_nan.s);

    const hsl nan_third(channel_t(0), channel_t(0.5), cl_nan());
    D_CL_CHECK(!(nan_third == nan_third));

    return true;
}

/*
tests_color_hsl_equality_does_not_canonicalize_hue
  Verifies hues a full turn apart denote the same color but compare unequal:
  equality is on stored channels, and clamp is what canonicalises.
  Tests the following:
  - hue 0 and hue 360 compare unequal despite denoting one color
  - clamping first makes them compare equal, showing where the responsibility
    for canonicalisation sits
*/
bool
tests_color_hsl_equality_does_not_canonicalize_hue()
{
    const hsl at_zero(channel_t(0), channel_t(1), channel_t(0.5));
    const hsl at_turn(channel_t(360), channel_t(1), channel_t(0.5));

    D_CL_CHECK(!(at_zero == at_turn));

    // canonicalise first and they agree
    D_CL_CHECK(at_zero.clamp() == at_turn.clamp());

    const hsl negative(channel_t(-360), channel_t(1), channel_t(0.5));
    D_CL_CHECK(!(at_zero == negative));
    D_CL_CHECK(at_zero.clamp() == negative.clamp());

    return true;
}

/*
tests_color_hsl_equality_is_constexpr
  Verifies operator== is usable in a constant expression.
  Tests the following:
  - a true and a false comparison both evaluate at compile time
*/
bool
tests_color_hsl_equality_is_constexpr()
{
    D_CONSTEXPR hsl a(channel_t(180), channel_t(0.5), channel_t(0.5));
    D_CONSTEXPR hsl b(channel_t(180), channel_t(0.5), channel_t(0.5));
    D_CONSTEXPR hsl c(channel_t(0), channel_t(0), channel_t(0));

    D_CONSTEXPR bool same = (a == b);
    D_CONSTEXPR bool diff = (a == c);

    static_assert(same, "operator== true case must be constexpr");
    static_assert(!diff, "operator== false case must be constexpr");

    D_CL_CHECK(same);
    D_CL_CHECK(!diff);

    return true;
}

/*
tests_color_hsl_inequality_is_the_negation_of_equality
  Verifies operator!= agrees with the negation of operator==.  The header
  declares only operator==; from C++20 the compiler rewrites != from it, so
  this check is gated on the language level.
  Tests the following:
  - != is false for equal values and true for a differing one
  - != always equals !(==)
*/
bool
tests_color_hsl_inequality_is_the_negation_of_equality()
{
    const hsl a(channel_t(120), channel_t(0.3), channel_t(0.6));
    const hsl b(channel_t(120), channel_t(0.3), channel_t(0.6));
    const hsl c(channel_t(120), channel_t(0.3), channel_t(0.9));

#if D_ENV_LANG_IS_CPP20_OR_HIGHER
    D_CL_CHECK(!(a != b));
    D_CL_CHECK(a != c);
    D_CL_CHECK((a != c) == !(a == c));
    D_CL_CHECK((a != b) == !(a == b));
#else
    D_CL_CHECK(!!(a == b));
    D_CL_CHECK(!(a == c));
#endif

    return true;
}

NS_END
NS_END
