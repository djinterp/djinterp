/*******************************************************************************
* djinterp [core]                                 color_rgb_tests_comparison.cpp
*
*
* path:      /tests/djinterp/core/util/color/color_rgb_tests_comparison.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "color_rgb_tests.hpp"

NS_DJINTERP
NS_TESTING

/*
tests_color_rgb_equality_is_reflexive
  Verifies an rgb equals itself and equals a separately built copy of the same
  channels.
  Tests the following:
  - self-comparison holds
  - two independently constructed equal values compare equal
  - the default-constructed value equals an explicit all-zero value
*/
bool
tests_color_rgb_equality_is_reflexive()
{
    const rgb v(channel_t(0.25), channel_t(0.5), channel_t(0.75));

    D_CR_CHECK(v == v);

    const rgb twin(channel_t(0.25), channel_t(0.5), channel_t(0.75));
    D_CR_CHECK(v == twin);

    const rgb defaulted;
    const rgb explicit_zero(channel_t(0), channel_t(0), channel_t(0));
    D_CR_CHECK(defaulted == explicit_zero);

    return true;
}

/*
tests_color_rgb_equality_detects_each_channel
  Verifies changing red, green or blue alone breaks equality, so no channel is
  omitted from the comparison.
  Tests the following:
  - three one-channel perturbations of a base value each compare unequal
  - permuting the channels is observable
*/
bool
tests_color_rgb_equality_detects_each_channel()
{
    const rgb base(channel_t(0.25), channel_t(0.5), channel_t(0.75));

    const rgb diff_r(channel_t(0.9), channel_t(0.5), channel_t(0.75));
    const rgb diff_g(channel_t(0.25), channel_t(0.9), channel_t(0.75));
    const rgb diff_b(channel_t(0.25), channel_t(0.5), channel_t(0.9));

    D_CR_CHECK(!(base == diff_r));
    D_CR_CHECK(!(base == diff_g));
    D_CR_CHECK(!(base == diff_b));

    const rgb permuted(channel_t(0.75), channel_t(0.5), channel_t(0.25));
    D_CR_CHECK(!(base == permuted));

    return true;
}

/*
tests_color_rgb_equality_is_symmetric
  Verifies the comparison gives the same verdict in either operand order.
  Tests the following:
  - equal and unequal pairs agree under operand swap
*/
bool
tests_color_rgb_equality_is_symmetric()
{
    const rgb a(channel_t(1), channel_t(0), channel_t(0));
    const rgb b(channel_t(1), channel_t(0), channel_t(0));
    const rgb c(channel_t(0), channel_t(1), channel_t(0));

    D_CR_CHECK((a == b) == (b == a));
    D_CR_CHECK((a == c) == (c == a));
    D_CR_CHECK(a == b);
    D_CR_CHECK(!(a == c));

    return true;
}

/*
tests_color_rgb_inequality_is_declared_not_rewritten
  Verifies operator!= is declared outright on this type and agrees with the
  negation of operator==.  Unlike the kernel-backed models, this header does
  not rely on the C++20 rewrite, so != is available at every language level
  and needs no gating.
  Tests the following:
  - != is false for equal values and true for a differing one
  - != always equals !(==)
*/
bool
tests_color_rgb_inequality_is_declared_not_rewritten()
{
    const rgb a(channel_t(0.25), channel_t(0.5), channel_t(0.75));
    const rgb b(channel_t(0.25), channel_t(0.5), channel_t(0.75));
    const rgb c(channel_t(0.25), channel_t(0.5), channel_t(0.9));

    D_CR_CHECK(!(a != b));
    D_CR_CHECK(a != c);

    D_CR_CHECK((a != b) == !(a == b));
    D_CR_CHECK((a != c) == !(a == c));

    return true;
}

/*
tests_color_rgb_equality_signed_zero_and_nan
  Verifies the IEEE corner cases inherited from comparing floats directly.
  Tests the following:
  - negative zero compares equal to positive zero
  - a NaN channel makes the value unequal even to itself, under == and !=
    alike
*/
bool
tests_color_rgb_equality_signed_zero_and_nan()
{
    const rgb pos_zero(channel_t(0), channel_t(0), channel_t(0));
    const rgb neg_zero(-channel_t(0), -channel_t(0), -channel_t(0));

    D_CR_CHECK(pos_zero == neg_zero);
    D_CR_CHECK(!(pos_zero != neg_zero));

    const rgb with_nan(cr_nan(), channel_t(0), channel_t(0));
    D_CR_CHECK(!(with_nan == with_nan));
    D_CR_CHECK(with_nan != with_nan);
    D_CR_CHECK(with_nan.g == with_nan.g);

    return true;
}

/*
tests_color_rgb_equality_is_constexpr
  Verifies rgb operator== and operator!= are usable in a constant expression.
  Tests the following:
  - a true and a false comparison both evaluate at compile time, under both
    operators
*/
bool
tests_color_rgb_equality_is_constexpr()
{
    D_CONSTEXPR rgb a(channel_t(1), channel_t(0), channel_t(0));
    D_CONSTEXPR rgb b(channel_t(1), channel_t(0), channel_t(0));
    D_CONSTEXPR rgb c(channel_t(0), channel_t(1), channel_t(0));

    D_CONSTEXPR bool same = (a == b);
    D_CONSTEXPR bool diff = (a == c);
    D_CONSTEXPR bool ne   = (a != c);

    static_assert(same, "operator== true case must be constexpr");
    static_assert(!diff, "operator== false case must be constexpr");
    static_assert(ne, "operator!= must be constexpr");

    D_CR_CHECK(same);
    D_CR_CHECK(!diff);
    D_CR_CHECK(ne);

    return true;
}

/*
tests_color_rgba_equality_includes_alpha
  Verifies two rgba values with identical colour but different alpha compare
  unequal - alpha is a full participant in equality, not decoration.
  Tests the following:
  - same colour, different alpha compares unequal
  - same alpha, different colour compares unequal
  - identical values compare equal
*/
bool
tests_color_rgba_equality_includes_alpha()
{
    const rgb colour(channel_t(0.25), channel_t(0.5), channel_t(0.75));

    const rgba opaque(colour, channel_t(1));
    const rgba clear(colour, channel_t(0));

    D_CR_CHECK(!(opaque == clear));
    D_CR_CHECK(opaque != clear);

    const rgba twin(colour, channel_t(1));
    D_CR_CHECK(opaque == twin);

    const rgb  other(channel_t(1), channel_t(0), channel_t(0));
    const rgba other_opaque(other, channel_t(1));
    D_CR_CHECK(!(opaque == other_opaque));

    return true;
}

/*
tests_color_rgba_inequality_is_declared_not_rewritten
  Verifies rgba operator!= is declared outright and agrees with the negation
  of operator==, so != works at every language level.
  Tests the following:
  - != is false for equal values and true for a differing one
  - != always equals !(==), including when only alpha differs
*/
bool
tests_color_rgba_inequality_is_declared_not_rewritten()
{
    const rgba a(channel_t(0.1), channel_t(0.2), channel_t(0.3),
                 channel_t(0.4));
    const rgba b(channel_t(0.1), channel_t(0.2), channel_t(0.3),
                 channel_t(0.4));
    const rgba c(channel_t(0.1), channel_t(0.2), channel_t(0.3),
                 channel_t(0.9));

    D_CR_CHECK(!(a != b));
    D_CR_CHECK(a != c);

    D_CR_CHECK((a != b) == !(a == b));
    D_CR_CHECK((a != c) == !(a == c));

    return true;
}

/*
tests_color_rgba_equality_is_constexpr
  Verifies rgba operator== and operator!= are usable in a constant expression.
  Tests the following:
  - a true and a false comparison both evaluate at compile time, under both
    operators
*/
bool
tests_color_rgba_equality_is_constexpr()
{
    D_CONSTEXPR rgba a(channel_t(0.1), channel_t(0.2), channel_t(0.3),
                       channel_t(0.4));
    D_CONSTEXPR rgba b(channel_t(0.1), channel_t(0.2), channel_t(0.3),
                       channel_t(0.4));
    D_CONSTEXPR rgba c(channel_t(0.1), channel_t(0.2), channel_t(0.3),
                       channel_t(0.9));

    D_CONSTEXPR bool same = (a == b);
    D_CONSTEXPR bool diff = (a == c);
    D_CONSTEXPR bool ne   = (a != c);

    static_assert(same, "operator== true case must be constexpr");
    static_assert(!diff, "operator== false case must be constexpr");
    static_assert(ne, "operator!= must be constexpr");

    D_CR_CHECK(same);
    D_CR_CHECK(!diff);
    D_CR_CHECK(ne);

    return true;
}

NS_END
NS_END
