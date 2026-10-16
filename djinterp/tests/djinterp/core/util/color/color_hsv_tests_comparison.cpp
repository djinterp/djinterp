/*******************************************************************************
* djinterp [core]                                 color_hsv_tests_comparison.cpp
*
*
* path:      /tests/djinterp/core/util/color/color_hsv_tests_comparison.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "color_hsv_tests.hpp"

NS_DJINTERP
NS_TESTING

/*
tests_color_hsv_equality_is_reflexive
  Verifies a value equals itself and equals a separately built copy of the
  same channels.
  Tests the following:
  - self-comparison holds
  - two independently constructed equal values compare equal
  - the default-constructed value equals an explicit all-zero value
*/
bool
tests_color_hsv_equality_is_reflexive()
{
    const hsv v(channel_t(210), channel_t(0.5), channel_t(0.5));

    D_CV_CHECK(v == v);

    const hsv twin(channel_t(210), channel_t(0.5), channel_t(0.5));
    D_CV_CHECK(v == twin);

    const hsv defaulted;
    const hsv explicit_zero(channel_t(0), channel_t(0), channel_t(0));
    D_CV_CHECK(defaulted == explicit_zero);

    return true;
}

/*
tests_color_hsv_equality_detects_each_channel
  Verifies changing hue, saturation or value alone breaks equality, so no
  channel is omitted from the comparison.
  Tests the following:
  - three one-channel perturbations of a base value each compare unequal
*/
bool
tests_color_hsv_equality_detects_each_channel()
{
    const hsv base(channel_t(120), channel_t(0.3), channel_t(0.6));

    const hsv diff_h(channel_t(240), channel_t(0.3), channel_t(0.6));
    const hsv diff_s(channel_t(120), channel_t(0.9), channel_t(0.6));
    const hsv diff_t(channel_t(120), channel_t(0.3), channel_t(0.1));

    D_CV_CHECK(!(base == diff_h));
    D_CV_CHECK(!(base == diff_s));
    D_CV_CHECK(!(base == diff_t));

    return true;
}

/*
tests_color_hsv_equality_is_symmetric
  Verifies the comparison gives the same verdict in either operand order.
  Tests the following:
  - equal and unequal pairs agree under operand swap
*/
bool
tests_color_hsv_equality_is_symmetric()
{
    const hsv a(channel_t(60), channel_t(0.5), channel_t(0.5));
    const hsv b(channel_t(60), channel_t(0.5), channel_t(0.5));
    const hsv c(channel_t(300), channel_t(0.1), channel_t(0.9));

    D_CV_CHECK((a == b) == (b == a));
    D_CV_CHECK((a == c) == (c == a));
    D_CV_CHECK(a == b);
    D_CV_CHECK(!(a == c));

    return true;
}

/*
tests_color_hsv_equality_is_exact_not_approximate
  Verifies a one-ulp channel difference compares unequal - the operator is
  exact channel equality with no built-in tolerance.
  Tests the following:
  - a value perturbed by one epsilon is not equal
  - approx_equal on the same pair DOES accept it, isolating the distinction
*/
bool
tests_color_hsv_equality_is_exact_not_approximate()
{
    const channel_t eps = std::numeric_limits<channel_t>::epsilon();

    const hsv base(channel_t(1), channel_t(0.5), channel_t(0.25));
    const hsv nudged(channel_t(1) + eps, channel_t(0.5), channel_t(0.25));

    D_CV_CHECK(!(base == nudged));
    D_CV_CHECK(base.h != nudged.h);

    D_CV_CHECK(approx_equal(base.h, nudged.h, eps));
    D_CV_CHECK(cv_near(base.h, nudged.h));

    return true;
}

/*
tests_color_hsv_equality_signed_zero_and_nan
  Verifies the IEEE corner cases inherited from comparing floats directly.
  Tests the following:
  - negative zero compares equal to positive zero
  - a channel holding NaN makes the value unequal even to itself
  - the inequality is confined to the offending channel
*/
bool
tests_color_hsv_equality_signed_zero_and_nan()
{
    const hsv pos_zero(channel_t(0), channel_t(0), channel_t(0));
    const hsv neg_zero(-channel_t(0), -channel_t(0), -channel_t(0));

    D_CV_CHECK(pos_zero == neg_zero);

    const hsv with_nan(cv_nan(), channel_t(0.5), channel_t(0.5));
    D_CV_CHECK(!(with_nan == with_nan));
    D_CV_CHECK(with_nan.h != with_nan.h);
    D_CV_CHECK(with_nan.s == with_nan.s);

    const hsv nan_third(channel_t(0), channel_t(0.5), cv_nan());
    D_CV_CHECK(!(nan_third == nan_third));

    return true;
}

/*
tests_color_hsv_equality_does_not_canonicalize_hue
  Verifies hues a full turn apart denote the same color but compare unequal:
  equality is on stored channels, and clamp is what canonicalises.
  Tests the following:
  - hue 0 and hue 360 compare unequal despite denoting one color
  - clamping first makes them compare equal, showing where the responsibility
    for canonicalisation sits
*/
bool
tests_color_hsv_equality_does_not_canonicalize_hue()
{
    const hsv at_zero(channel_t(0), channel_t(1), channel_t(0.5));
    const hsv at_turn(channel_t(360), channel_t(1), channel_t(0.5));

    D_CV_CHECK(!(at_zero == at_turn));

    // canonicalise first and they agree
    D_CV_CHECK(at_zero.clamp() == at_turn.clamp());

    const hsv negative(channel_t(-360), channel_t(1), channel_t(0.5));
    D_CV_CHECK(!(at_zero == negative));
    D_CV_CHECK(at_zero.clamp() == negative.clamp());

    return true;
}

/*
tests_color_hsv_equality_is_constexpr
  Verifies operator== is usable in a constant expression.
  Tests the following:
  - a true and a false comparison both evaluate at compile time
*/
bool
tests_color_hsv_equality_is_constexpr()
{
    D_CONSTEXPR hsv a(channel_t(180), channel_t(0.5), channel_t(0.5));
    D_CONSTEXPR hsv b(channel_t(180), channel_t(0.5), channel_t(0.5));
    D_CONSTEXPR hsv c(channel_t(0), channel_t(0), channel_t(0));

    D_CONSTEXPR bool same = (a == b);
    D_CONSTEXPR bool diff = (a == c);

    static_assert(same, "operator== true case must be constexpr");
    static_assert(!diff, "operator== false case must be constexpr");

    D_CV_CHECK(same);
    D_CV_CHECK(!diff);

    return true;
}

/*
tests_color_hsv_inequality_is_the_negation_of_equality
  Verifies operator!= agrees with the negation of operator==.  The header
  declares only operator==; from C++20 the compiler rewrites != from it, so
  this check is gated on the language level.
  Tests the following:
  - != is false for equal values and true for a differing one
  - != always equals !(==)
*/
bool
tests_color_hsv_inequality_is_the_negation_of_equality()
{
    const hsv a(channel_t(120), channel_t(0.3), channel_t(0.6));
    const hsv b(channel_t(120), channel_t(0.3), channel_t(0.6));
    const hsv c(channel_t(120), channel_t(0.3), channel_t(0.9));

#if D_ENV_LANG_IS_CPP20_OR_HIGHER
    D_CV_CHECK(!(a != b));
    D_CV_CHECK(a != c);
    D_CV_CHECK((a != c) == !(a == c));
    D_CV_CHECK((a != b) == !(a == b));
#else
    D_CV_CHECK(!!(a == b));
    D_CV_CHECK(!(a == c));
#endif

    return true;
}

NS_END
NS_END
