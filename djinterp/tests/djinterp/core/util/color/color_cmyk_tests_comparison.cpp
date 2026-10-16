/*******************************************************************************
* djinterp [core]                                color_cmyk_tests_comparison.cpp
*
*
* path:      /tests/djinterp/core/util/color/color_cmyk_tests_comparison.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "color_cmyk_tests.hpp"

NS_DJINTERP
NS_TESTING

/*
tests_color_cmyk_equality_is_reflexive
  Verifies a value equals itself and equals a separately built copy of the
  same channels.
  Tests the following:
  - self-comparison holds
  - two independently constructed values with equal channels compare equal
  - the default-constructed value equals an explicit all-zero value
*/
bool
tests_color_cmyk_equality_is_reflexive()
{
    const cmyk v(channel_t(0.1), channel_t(0.2),
                 channel_t(0.3), channel_t(0.4));

    D_CK_CHECK(v == v);

    const cmyk twin(channel_t(0.1), channel_t(0.2),
                    channel_t(0.3), channel_t(0.4));
    D_CK_CHECK(v == twin);

    const cmyk defaulted;
    const cmyk explicit_zero(channel_t(0), channel_t(0),
                             channel_t(0), channel_t(0));
    D_CK_CHECK(defaulted == explicit_zero);

    return true;
}

/*
tests_color_cmyk_equality_detects_each_channel
  Verifies changing any one of c, m, y or k alone breaks equality, so no
  channel is omitted from the comparison.
  Tests the following:
  - four one-channel perturbations of a base value each compare unequal
*/
bool
tests_color_cmyk_equality_detects_each_channel()
{
    const cmyk base(channel_t(0.1), channel_t(0.2),
                    channel_t(0.3), channel_t(0.4));

    const cmyk diff_c(channel_t(0.9), channel_t(0.2),
                      channel_t(0.3), channel_t(0.4));
    const cmyk diff_m(channel_t(0.1), channel_t(0.9),
                      channel_t(0.3), channel_t(0.4));
    const cmyk diff_y(channel_t(0.1), channel_t(0.2),
                      channel_t(0.9), channel_t(0.4));
    const cmyk diff_k(channel_t(0.1), channel_t(0.2),
                      channel_t(0.3), channel_t(0.9));

    D_CK_CHECK(!(base == diff_c));
    D_CK_CHECK(!(base == diff_m));
    D_CK_CHECK(!(base == diff_y));
    D_CK_CHECK(!(base == diff_k));

    return true;
}

/*
tests_color_cmyk_equality_is_symmetric
  Verifies the comparison gives the same verdict in either operand order.
  Tests the following:
  - equal and unequal pairs agree under operand swap
*/
bool
tests_color_cmyk_equality_is_symmetric()
{
    const cmyk a(channel_t(0.1), channel_t(0.2),
                 channel_t(0.3), channel_t(0.4));
    const cmyk b(channel_t(0.1), channel_t(0.2),
                 channel_t(0.3), channel_t(0.4));
    const cmyk c(channel_t(0.5), channel_t(0.6),
                 channel_t(0.7), channel_t(0.8));

    D_CK_CHECK((a == b) == (b == a));
    D_CK_CHECK((a == c) == (c == a));
    D_CK_CHECK(a == b);
    D_CK_CHECK(!(a == c));

    return true;
}

/*
tests_color_cmyk_equality_is_exact_not_approximate
  Verifies a one-ulp channel difference compares unequal - the operator is
  exact channel equality with no built-in tolerance.
  Tests the following:
  - a value perturbed by one epsilon is not equal
  - approx_equal on the same pair DOES accept it, isolating the distinction
*/
bool
tests_color_cmyk_equality_is_exact_not_approximate()
{
    const channel_t eps = std::numeric_limits<channel_t>::epsilon();

    const cmyk base(channel_t(1), channel_t(0.5),
                    channel_t(0.25), channel_t(0));
    const cmyk nudged(channel_t(1) + eps, channel_t(0.5),
                      channel_t(0.25), channel_t(0));

    D_CK_CHECK(!(base == nudged));
    D_CK_CHECK(base.c != nudged.c);

    // the difference is real but tiny: an approximate comparison accepts it
    D_CK_CHECK(approx_equal(base.c, nudged.c, eps));
    D_CK_CHECK(ck_near(base.c, nudged.c));

    return true;
}

/*
tests_color_cmyk_equality_signed_zero_and_nan
  Verifies the IEEE corner cases inherited from comparing floats directly.
  Tests the following:
  - negative zero compares equal to positive zero
  - a channel holding NaN makes the value unequal even to itself
  - the NaN inequality is confined to the offending channel's comparison
*/
bool
tests_color_cmyk_equality_signed_zero_and_nan()
{
    const cmyk pos_zero(channel_t(0), channel_t(0),
                        channel_t(0), channel_t(0));
    const cmyk neg_zero(-channel_t(0), -channel_t(0),
                        -channel_t(0), -channel_t(0));

    D_CK_CHECK(pos_zero == neg_zero);

    const cmyk with_nan(ck_nan(), channel_t(0), channel_t(0), channel_t(0));
    D_CK_CHECK(!(with_nan == with_nan));

    // only the NaN channel is at fault
    D_CK_CHECK(with_nan.c != with_nan.c);
    D_CK_CHECK(with_nan.m == with_nan.m);

    const cmyk nan_k(channel_t(0), channel_t(0), channel_t(0), ck_nan());
    D_CK_CHECK(!(nan_k == nan_k));

    return true;
}

/*
tests_color_cmyk_equality_is_constexpr
  Verifies operator== is usable in a constant expression.
  Tests the following:
  - a true and a false comparison both evaluate at compile time
*/
bool
tests_color_cmyk_equality_is_constexpr()
{
    D_CONSTEXPR cmyk a(channel_t(0.25), channel_t(0.5),
                       channel_t(0.75), channel_t(1));
    D_CONSTEXPR cmyk b(channel_t(0.25), channel_t(0.5),
                       channel_t(0.75), channel_t(1));
    D_CONSTEXPR cmyk c(channel_t(0), channel_t(0),
                       channel_t(0), channel_t(0));

    D_CONSTEXPR bool same = (a == b);
    D_CONSTEXPR bool diff = (a == c);

    static_assert(same, "operator== true case must be constexpr");
    static_assert(!diff, "operator== false case must be constexpr");

    D_CK_CHECK(same);
    D_CK_CHECK(!diff);

    return true;
}

/*
tests_color_cmyk_inequality_is_the_negation_of_equality
  Verifies operator!= agrees with the negation of operator==.  The header
  declares only operator==; from C++20 the compiler rewrites != from it, so
  this check is gated on the language level.
  Tests the following:
  - != is false for equal values and true for each single-channel difference
  - != always equals !(==)
*/
bool
tests_color_cmyk_inequality_is_the_negation_of_equality()
{
    const cmyk a(channel_t(0.1), channel_t(0.2),
                 channel_t(0.3), channel_t(0.4));
    const cmyk b(channel_t(0.1), channel_t(0.2),
                 channel_t(0.3), channel_t(0.4));
    const cmyk c(channel_t(0.1), channel_t(0.2),
                 channel_t(0.3), channel_t(0.9));

#if D_ENV_LANG_IS_CPP20_OR_HIGHER
    D_CK_CHECK(!(a != b));
    D_CK_CHECK(a != c);
    D_CK_CHECK((a != c) == !(a == c));
    D_CK_CHECK((a != b) == !(a == b));
#else
    // pre-C++20 there is no rewritten !=; the negation is the whole contract
    D_CK_CHECK(!!(a == b));
    D_CK_CHECK(!(a == c));
#endif

    return true;
}

NS_END
NS_END
