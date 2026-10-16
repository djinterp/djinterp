/*******************************************************************************
* djinterp [core]                               color_ycbcr_tests_comparison.cpp
*
*
* path:      /tests/djinterp/core/util/color/color_ycbcr_tests_comparison.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "color_ycbcr_tests.hpp"

NS_DJINTERP
NS_TESTING

/*
tests_color_ycbcr_equality_is_reflexive
  Verifies a value equals itself and equals a separately built copy of the
  same channels.
  Tests the following:
  - self-comparison holds
  - two independently constructed equal values compare equal
  - the default-constructed value equals an explicit all-zero value
*/
bool
tests_color_ycbcr_equality_is_reflexive()
{
    const ycbcr v(channel_t(0.5), channel_t(0.25), channel_t(-0.25));

    D_CY_CHECK(v == v);

    const ycbcr twin(channel_t(0.5), channel_t(0.25), channel_t(-0.25));
    D_CY_CHECK(v == twin);

    const ycbcr defaulted;
    const ycbcr explicit_zero(channel_t(0), channel_t(0), channel_t(0));
    D_CY_CHECK(defaulted == explicit_zero);

    return true;
}

/*
tests_color_ycbcr_equality_detects_each_channel
  Verifies changing luma, Cb or Cr alone breaks equality, so no channel is
  omitted from the comparison.
  Tests the following:
  - three one-channel perturbations of a base value each compare unequal
*/
bool
tests_color_ycbcr_equality_detects_each_channel()
{
    const ycbcr base(channel_t(0.5), channel_t(0.1), channel_t(-0.1));

    const ycbcr diff_y(channel_t(0.9), channel_t(0.1), channel_t(-0.1));
    const ycbcr diff_cb(channel_t(0.5), channel_t(0.4), channel_t(-0.1));
    const ycbcr diff_cr(channel_t(0.5), channel_t(0.1), channel_t(0.3));

    D_CY_CHECK(!(base == diff_y));
    D_CY_CHECK(!(base == diff_cb));
    D_CY_CHECK(!(base == diff_cr));

    // Cb and Cr are distinct slots: swapping them is observable
    const ycbcr swapped(channel_t(0.5), channel_t(-0.1), channel_t(0.1));
    D_CY_CHECK(!(base == swapped));

    return true;
}

/*
tests_color_ycbcr_equality_is_symmetric
  Verifies the comparison gives the same verdict in either operand order.
  Tests the following:
  - equal and unequal pairs agree under operand swap
*/
bool
tests_color_ycbcr_equality_is_symmetric()
{
    const ycbcr a(channel_t(0.5), channel_t(0.25), channel_t(-0.25));
    const ycbcr b(channel_t(0.5), channel_t(0.25), channel_t(-0.25));
    const ycbcr c(channel_t(1), channel_t(-0.5), channel_t(0.5));

    D_CY_CHECK((a == b) == (b == a));
    D_CY_CHECK((a == c) == (c == a));
    D_CY_CHECK(a == b);
    D_CY_CHECK(!(a == c));

    return true;
}

/*
tests_color_ycbcr_equality_is_exact_not_approximate
  Verifies a one-ulp channel difference compares unequal - the operator is
  exact channel equality with no built-in tolerance.
  Tests the following:
  - a value perturbed by one epsilon is not equal
  - approx_equal on the same pair DOES accept it, isolating the distinction
*/
bool
tests_color_ycbcr_equality_is_exact_not_approximate()
{
    const channel_t eps = std::numeric_limits<channel_t>::epsilon();

    const ycbcr base(channel_t(1), channel_t(0.5), channel_t(-0.5));
    const ycbcr nudged(channel_t(1) + eps, channel_t(0.5), channel_t(-0.5));

    D_CY_CHECK(!(base == nudged));
    D_CY_CHECK(base.y != nudged.y);

    D_CY_CHECK(approx_equal(base.y, nudged.y, eps));
    D_CY_CHECK(cy_near(base.y, nudged.y));

    return true;
}

/*
tests_color_ycbcr_equality_signed_zero_and_nan
  Verifies the IEEE corner cases inherited from comparing floats directly.
  Tests the following:
  - negative zero compares equal to positive zero
  - a channel holding NaN makes the value unequal even to itself
  - the inequality is confined to the offending channel
*/
bool
tests_color_ycbcr_equality_signed_zero_and_nan()
{
    const ycbcr pos_zero(channel_t(0), channel_t(0), channel_t(0));
    const ycbcr neg_zero(-channel_t(0), -channel_t(0), -channel_t(0));

    D_CY_CHECK(pos_zero == neg_zero);

    const ycbcr with_nan(cy_nan(), channel_t(0), channel_t(0));
    D_CY_CHECK(!(with_nan == with_nan));
    D_CY_CHECK(with_nan.y != with_nan.y);
    D_CY_CHECK(with_nan.cb == with_nan.cb);

    const ycbcr nan_cr(channel_t(0.5), channel_t(0), cy_nan());
    D_CY_CHECK(!(nan_cr == nan_cr));

    return true;
}

/*
tests_color_ycbcr_equality_is_constexpr
  Verifies operator== is usable in a constant expression.
  Tests the following:
  - a true and a false comparison both evaluate at compile time
*/
bool
tests_color_ycbcr_equality_is_constexpr()
{
    D_CONSTEXPR ycbcr a(channel_t(0.5), channel_t(0.25), channel_t(-0.25));
    D_CONSTEXPR ycbcr b(channel_t(0.5), channel_t(0.25), channel_t(-0.25));
    D_CONSTEXPR ycbcr c(channel_t(0), channel_t(0), channel_t(0));

    D_CONSTEXPR bool same = (a == b);
    D_CONSTEXPR bool diff = (a == c);

    static_assert(same, "operator== true case must be constexpr");
    static_assert(!diff, "operator== false case must be constexpr");

    D_CY_CHECK(same);
    D_CY_CHECK(!diff);

    return true;
}

/*
tests_color_ycbcr_inequality_is_the_negation_of_equality
  Verifies operator!= agrees with the negation of operator==.  The header
  declares only operator==; from C++20 the compiler rewrites != from it, so
  this check is gated on the language level.
  Tests the following:
  - != is false for equal values and true for a differing one
  - != always equals !(==)
*/
bool
tests_color_ycbcr_inequality_is_the_negation_of_equality()
{
    const ycbcr a(channel_t(0.5), channel_t(0.1), channel_t(-0.1));
    const ycbcr b(channel_t(0.5), channel_t(0.1), channel_t(-0.1));
    const ycbcr c(channel_t(0.5), channel_t(0.1), channel_t(0.4));

#if D_ENV_LANG_IS_CPP20_OR_HIGHER
    D_CY_CHECK(!(a != b));
    D_CY_CHECK(a != c);
    D_CY_CHECK((a != c) == !(a == c));
    D_CY_CHECK((a != b) == !(a == b));
#else
    D_CY_CHECK(!!(a == b));
    D_CY_CHECK(!(a == c));
#endif

    return true;
}

NS_END
NS_END
