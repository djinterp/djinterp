/*******************************************************************************
* djinterp [core]                                  color_common_tests_scalar.cpp
*
*
* path:      /tests/djinterp/core/util/color/color_common_tests_scalar.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "color_common_tests.hpp"

NS_DJINTERP
NS_TESTING

/*
tests_color_common_clamp_channel_below_range
  Verifies a value below the low bound clamps to the low bound (the first arm
  of the three-way select).
  Tests the following:
  - just-below, far-below and negative inputs all yield the low bound
  - the result is exactly the bound, not an approximation
*/
bool
tests_color_common_clamp_channel_below_range()
{
    D_CC_CHECK(clamp_channel(channel_t(-1), channel_t(0), channel_t(1))
               == channel_t(0));
    D_CC_CHECK(clamp_channel(channel_t(-0.001), channel_t(0), channel_t(1))
               == channel_t(0));
    D_CC_CHECK(clamp_channel(channel_t(-1000), channel_t(0), channel_t(1))
               == channel_t(0));
    D_CC_CHECK(clamp_channel(channel_t(0.25), channel_t(0.5), channel_t(1))
               == channel_t(0.5));
    D_CC_CHECK(clamp_channel(channel_t(-2), channel_t(-1), channel_t(1))
               == channel_t(-1));

    return true;
}

/*
tests_color_common_clamp_channel_above_range
  Verifies a value above the high bound clamps to the high bound (the second
  arm of the select).
  Tests the following:
  - just-above, far-above inputs yield the high bound
  - a negative range's upper bound is honoured too
*/
bool
tests_color_common_clamp_channel_above_range()
{
    D_CC_CHECK(clamp_channel(channel_t(2), channel_t(0), channel_t(1))
               == channel_t(1));
    D_CC_CHECK(clamp_channel(channel_t(1.001), channel_t(0), channel_t(1))
               == channel_t(1));
    D_CC_CHECK(clamp_channel(channel_t(1e9), channel_t(0), channel_t(1))
               == channel_t(1));
    D_CC_CHECK(clamp_channel(channel_t(0.75), channel_t(0), channel_t(0.5))
               == channel_t(0.5));
    D_CC_CHECK(clamp_channel(channel_t(0), channel_t(-1), channel_t(-0.5))
               == channel_t(-0.5));

    return true;
}

/*
tests_color_common_clamp_channel_within_range
  Verifies a value inside the range is returned unchanged (the fall-through
  arm of the select).
  Tests the following:
  - interior values pass through bit-exact
  - both signs and a range straddling zero behave
*/
bool
tests_color_common_clamp_channel_within_range()
{
    D_CC_CHECK(clamp_channel(channel_t(0.5), channel_t(0), channel_t(1))
               == channel_t(0.5));
    D_CC_CHECK(clamp_channel(channel_t(0.125), channel_t(0), channel_t(1))
               == channel_t(0.125));
    D_CC_CHECK(clamp_channel(channel_t(-0.25), channel_t(-1), channel_t(1))
               == channel_t(-0.25));
    D_CC_CHECK(clamp_channel(channel_t(0), channel_t(-0.5), channel_t(0.5))
               == channel_t(0));

    return true;
}

/*
tests_color_common_clamp_channel_boundaries_inclusive
  Verifies values exactly on either bound are returned unchanged, so the
  admissible interval is closed at both ends.
  Tests the following:
  - v == lo and v == hi both pass through
  - clamping is idempotent on an already-clamped value
*/
bool
tests_color_common_clamp_channel_boundaries_inclusive()
{
    D_CC_CHECK(clamp_channel(channel_t(0), channel_t(0), channel_t(1))
               == channel_t(0));
    D_CC_CHECK(clamp_channel(channel_t(1), channel_t(0), channel_t(1))
               == channel_t(1));
    D_CC_CHECK(clamp_channel(channel_t(-0.5), channel_t(-0.5), channel_t(0.5))
               == channel_t(-0.5));

    // idempotence
    const channel_t once =
        clamp_channel(channel_t(5), channel_t(0), channel_t(1));
    const channel_t twice =
        clamp_channel(once, channel_t(0), channel_t(1));
    D_CC_CHECK(once == twice);

    return true;
}

/*
tests_color_common_clamp_channel_degenerate_range
  Verifies a zero-width range collapses every input to the single admissible
  value.
  Tests the following:
  - lo == hi returns that value for inputs below, at, and above it
*/
bool
tests_color_common_clamp_channel_degenerate_range()
{
    D_CC_CHECK(clamp_channel(channel_t(-1), channel_t(0.5), channel_t(0.5))
               == channel_t(0.5));
    D_CC_CHECK(clamp_channel(channel_t(0.5), channel_t(0.5), channel_t(0.5))
               == channel_t(0.5));
    D_CC_CHECK(clamp_channel(channel_t(9), channel_t(0.5), channel_t(0.5))
               == channel_t(0.5));
    D_CC_CHECK(clamp_channel(channel_t(1), channel_t(0), channel_t(0))
               == channel_t(0));

    return true;
}

/*
tests_color_common_clamp_channel_inverted_range
  Verifies an inverted range (lo > hi) resolves by the documented select order
  rather than hard-erroring: the low-bound test runs first and wins.
  Tests the following:
  - v < lo yields lo even though lo exceeds hi
  - otherwise hi < v yields hi
  This pins existing behaviour so a future guard is a deliberate change.
*/
bool
tests_color_common_clamp_channel_inverted_range()
{
    // v(5) < lo(10) -> lo
    D_CC_CHECK(clamp_channel(channel_t(5), channel_t(10), channel_t(0))
               == channel_t(10));
    // v(-5) < lo(10) -> lo
    D_CC_CHECK(clamp_channel(channel_t(-5), channel_t(10), channel_t(0))
               == channel_t(10));
    // v(20) not < lo(10); hi(0) < v -> hi
    D_CC_CHECK(clamp_channel(channel_t(20), channel_t(10), channel_t(0))
               == channel_t(0));

    return true;
}

/*
tests_color_common_clamp_channel_non_finite
  Verifies non-finite inputs behave predictably: infinities clamp to the
  bounds, and NaN passes through unchanged because both comparisons are false.
  Tests the following:
  - +inf clamps to hi, -inf clamps to lo
  - NaN input yields NaN output (documented consequence of the select)
  - infinite bounds are honoured
*/
bool
tests_color_common_clamp_channel_non_finite()
{
    const channel_t inf = cc_inf();
    const channel_t nan = cc_nan();

    D_CC_CHECK(clamp_channel(inf, channel_t(0), channel_t(1)) == channel_t(1));
    D_CC_CHECK(clamp_channel(-inf, channel_t(0), channel_t(1)) == channel_t(0));

    // NaN < lo is false and hi < NaN is false, so the value falls through
    const channel_t out = clamp_channel(nan, channel_t(0), channel_t(1));
    D_CC_CHECK(out != out);

    // an unbounded high end never clamps from above
    D_CC_CHECK(clamp_channel(channel_t(1e30), channel_t(0), inf)
               == channel_t(1e30));

    return true;
}

/*
tests_color_common_clamp_channel_generic_over_int
  Verifies clamp_channel is a template usable beyond the channel scalar, with
  identical semantics on an integral type.
  Tests the following:
  - int instantiation clamps below/within/above
  - the return type is the deduced parameter type
*/
bool
tests_color_common_clamp_channel_generic_over_int()
{
    D_CC_CHECK(clamp_channel<int>(-5, 0, 10) == 0);
    D_CC_CHECK(clamp_channel<int>(5, 0, 10) == 5);
    D_CC_CHECK(clamp_channel<int>(50, 0, 10) == 10);
    D_CC_CHECK(clamp_channel<int>(0, 0, 10) == 0);
    D_CC_CHECK(clamp_channel<int>(10, 0, 10) == 10);

    D_CC_CHECK(std::is_same<decltype(clamp_channel<int>(1, 0, 2)), int>::value);
    D_CC_CHECK(std::is_same<decltype(clamp_channel(channel_t(1),
                                                   channel_t(0),
                                                   channel_t(2))),
                            channel_t>::value);

    return true;
}

/*
tests_color_common_clamp_channel_is_constexpr
  Verifies clamp_channel is usable in a constant expression - the single-return
  body is the reason it stays constant-expression-safe down to C++11.
  Tests the following:
  - all three select arms evaluate at compile time via static_assert
*/
bool
tests_color_common_clamp_channel_is_constexpr()
{
    D_CONSTEXPR channel_t lo_arm =
        clamp_channel(channel_t(-1), channel_t(0), channel_t(1));
    D_CONSTEXPR channel_t hi_arm =
        clamp_channel(channel_t(2), channel_t(0), channel_t(1));
    D_CONSTEXPR channel_t mid_arm =
        clamp_channel(channel_t(0.5), channel_t(0), channel_t(1));

    static_assert(lo_arm == channel_t(0), "clamp_channel low arm constexpr");
    static_assert(hi_arm == channel_t(1), "clamp_channel high arm constexpr");
    static_assert(mid_arm == channel_t(0.5), "clamp_channel mid arm constexpr");

    D_CC_CHECK(lo_arm == channel_t(0));
    D_CC_CHECK(hi_arm == channel_t(1));
    D_CC_CHECK(mid_arm == channel_t(0.5));

    return true;
}

/*
tests_color_common_approx_equal_identical
  Verifies identical operands compare approximately equal at any non-negative
  tolerance.
  Tests the following:
  - equal values at generous, tiny and zero tolerance
  - the property holds for zero, unity and negative operands
*/
bool
tests_color_common_approx_equal_identical()
{
    D_CC_CHECK(approx_equal(channel_t(0.5), channel_t(0.5), channel_t(1e-6)));
    D_CC_CHECK(approx_equal(channel_t(0.5), channel_t(0.5), channel_t(0)));
    D_CC_CHECK(approx_equal(channel_t(0), channel_t(0), channel_t(0)));
    D_CC_CHECK(approx_equal(channel_t(1), channel_t(1), channel_t(1e-9)));
    D_CC_CHECK(approx_equal(channel_t(-7.5), channel_t(-7.5), channel_t(0)));

    return true;
}

/*
tests_color_common_approx_equal_within_tolerance
  Verifies operands differing by less than the tolerance compare equal.
  Tests the following:
  - a difference an order of magnitude under the tolerance passes
  - the check is on absolute, not relative, difference
*/
bool
tests_color_common_approx_equal_within_tolerance()
{
    D_CC_CHECK(approx_equal(channel_t(1.0), channel_t(1.0001),
                            channel_t(1e-3)));
    D_CC_CHECK(approx_equal(channel_t(0.25), channel_t(0.2500001),
                            channel_t(1e-5)));

    // absolute, not relative: a large pair inside a large tolerance passes
    D_CC_CHECK(approx_equal(channel_t(1000), channel_t(1000.5),
                            channel_t(1)));
    // and a large pair inside a tight tolerance does not
    D_CC_CHECK(!approx_equal(channel_t(1000), channel_t(1000.5),
                             channel_t(1e-3)));

    return true;
}

/*
tests_color_common_approx_equal_tolerance_is_inclusive
  Verifies a difference exactly equal to the tolerance still compares equal -
  the predicate tests <=, not <.
  Tests the following:
  - an exactly representable difference equal to eps passes
  - the next representable step beyond it fails
*/
bool
tests_color_common_approx_equal_tolerance_is_inclusive()
{
    // 0.5 and 0.75 differ by exactly 0.25 (all exactly representable)
    D_CC_CHECK(approx_equal(channel_t(0.5), channel_t(0.75),
                            channel_t(0.25)));
    D_CC_CHECK(approx_equal(channel_t(0.75), channel_t(0.5),
                            channel_t(0.25)));

    // a hair under the difference must fail
    D_CC_CHECK(!approx_equal(channel_t(0.5), channel_t(0.75),
                             channel_t(0.1249999)));

    D_CC_CHECK(approx_equal(channel_t(0), channel_t(1), channel_t(1)));
    D_CC_CHECK(!approx_equal(channel_t(0), channel_t(1), channel_t(0.5)));

    return true;
}

/*
tests_color_common_approx_equal_beyond_tolerance
  Verifies operands differing by more than the tolerance compare unequal.
  Tests the following:
  - clearly separated values fail at a tight tolerance
  - the failure holds in both argument orders
*/
bool
tests_color_common_approx_equal_beyond_tolerance()
{
    D_CC_CHECK(!approx_equal(channel_t(0), channel_t(1), channel_t(1e-6)));
    D_CC_CHECK(!approx_equal(channel_t(1), channel_t(0), channel_t(1e-6)));
    D_CC_CHECK(!approx_equal(channel_t(0.5), channel_t(0.6), channel_t(0.05)));
    D_CC_CHECK(!approx_equal(channel_t(-1), channel_t(1), channel_t(1.5)));

    return true;
}

/*
tests_color_common_approx_equal_is_symmetric
  Verifies the comparison is symmetric in its operands, exercising both arms
  of the absolute-difference select.
  Tests the following:
  - swapping the operands never changes the verdict, for a < b, a > b, a == b
*/
bool
tests_color_common_approx_equal_is_symmetric()
{
    const channel_t eps = channel_t(0.01);

    // a < b  -> takes the (b - a) arm
    D_CC_CHECK(approx_equal(channel_t(0.5), channel_t(0.505), eps)
               == approx_equal(channel_t(0.505), channel_t(0.5), eps));
    // a > b  -> takes the (a - b) arm
    D_CC_CHECK(approx_equal(channel_t(0.9), channel_t(0.1), eps)
               == approx_equal(channel_t(0.1), channel_t(0.9), eps));
    // a == b -> the (a - b) arm with a zero difference
    D_CC_CHECK(approx_equal(channel_t(0.3), channel_t(0.3), eps)
               == approx_equal(channel_t(0.3), channel_t(0.3), eps));

    // mixed signs, both orders
    D_CC_CHECK(approx_equal(channel_t(-0.5), channel_t(0.5), channel_t(1)));
    D_CC_CHECK(approx_equal(channel_t(0.5), channel_t(-0.5), channel_t(1)));

    return true;
}

/*
tests_color_common_approx_equal_zero_tolerance
  Verifies a zero tolerance degrades to exact equality.
  Tests the following:
  - equal values pass, any difference fails
  - a one-epsilon difference fails at zero tolerance
*/
bool
tests_color_common_approx_equal_zero_tolerance()
{
    const channel_t eps = std::numeric_limits<channel_t>::epsilon();

    D_CC_CHECK(approx_equal(channel_t(0.5), channel_t(0.5), channel_t(0)));
    D_CC_CHECK(!approx_equal(channel_t(1), channel_t(1) + eps, channel_t(0)));
    D_CC_CHECK(approx_equal(channel_t(1), channel_t(1) + eps, eps));

    return true;
}

/*
tests_color_common_approx_equal_negative_operands
  Verifies negative and mixed-sign operands compare on true absolute
  difference rather than on signed subtraction.
  Tests the following:
  - two negatives close together pass
  - values straddling zero are separated by their full distance
*/
bool
tests_color_common_approx_equal_negative_operands()
{
    D_CC_CHECK(approx_equal(channel_t(-1.0), channel_t(-1.001),
                            channel_t(0.01)));
    D_CC_CHECK(!approx_equal(channel_t(-1.0), channel_t(-2.0),
                             channel_t(0.5)));

    // straddling zero: the distance is 0.5, not 0
    D_CC_CHECK(!approx_equal(channel_t(-0.25), channel_t(0.25),
                             channel_t(0.25)));
    D_CC_CHECK(approx_equal(channel_t(-0.25), channel_t(0.25),
                            channel_t(0.5)));

    return true;
}

/*
tests_color_common_approx_equal_nan_is_never_equal
  Verifies NaN is not approximately equal to anything, including itself: both
  select arms produce NaN and every NaN comparison is false.
  Tests the following:
  - NaN vs NaN, NaN vs finite, finite vs NaN all fail at any tolerance
  - infinities compare equal to themselves but not to finite values
*/
bool
tests_color_common_approx_equal_nan_is_never_equal()
{
    const channel_t nan = cc_nan();
    const channel_t inf = cc_inf();

    D_CC_CHECK(!approx_equal(nan, nan, channel_t(1e9)));
    D_CC_CHECK(!approx_equal(nan, channel_t(0), channel_t(1e9)));
    D_CC_CHECK(!approx_equal(channel_t(0), nan, channel_t(1e9)));

    // inf - inf is NaN, so even identical infinities do not compare equal
    D_CC_CHECK(!approx_equal(inf, inf, channel_t(1e9)));
    D_CC_CHECK(!approx_equal(inf, channel_t(0), channel_t(1e9)));

    return true;
}

/*
tests_color_common_approx_equal_is_constexpr
  Verifies approx_equal is usable in a constant expression.
  Tests the following:
  - a passing and a failing comparison both evaluate at compile time
*/
bool
tests_color_common_approx_equal_is_constexpr()
{
    D_CONSTEXPR bool near =
        approx_equal(channel_t(0.5), channel_t(0.75), channel_t(0.25));
    D_CONSTEXPR bool far =
        approx_equal(channel_t(0), channel_t(1), channel_t(0.25));

    static_assert(near, "approx_equal true case must be constexpr");
    static_assert(!far, "approx_equal false case must be constexpr");

    D_CC_CHECK(near);
    D_CC_CHECK(!far);

    return true;
}

NS_END
NS_END
