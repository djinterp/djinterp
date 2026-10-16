/*******************************************************************************
* djinterp [core]                                           color_tests_cmyk.cpp
*
*
* path:      /tests/djinterp/core/util/color/color_tests_cmyk.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "color_tests.hpp"

NS_DJINTERP
NS_TESTING

/*
tests_color_umbrella_cmyk_default_is_zeroed
  Verifies the default constructor zeroes all four ink channels.
  Tests the following:
  - c, m, y and k are each exactly zero
  - no ink at all denotes white on the page, not black
*/
bool
tests_color_umbrella_cmyk_default_is_zeroed()
{
    const cmyk v;

    D_CU_CHECK(v.c == channel_t(0));
    D_CU_CHECK(v.m == channel_t(0));
    D_CU_CHECK(v.y == channel_t(0));
    D_CU_CHECK(v.k == channel_t(0));

    // no ink is white
    D_CU_CHECK(cu_rgb_near(color_convert<rgb, cmyk>::apply(v), channel_t(1),
                           channel_t(1), channel_t(1)));

    return true;
}

/*
tests_color_umbrella_cmyk_parameterized_stores_channels_verbatim
  Verifies the channel constructor stores its four arguments unchanged and in
  order.
  Tests the following:
  - values land on the matching ink, in c, m, y, k order
  - out-of-unit values are stored as given - this shim imposes no policy
*/
bool
tests_color_umbrella_cmyk_parameterized_stores_channels_verbatim()
{
    const cmyk v(channel_t(0.1), channel_t(0.2), channel_t(0.3),
                 channel_t(0.4));

    D_CU_CHECK(v.c == channel_t(0.1));
    D_CU_CHECK(v.m == channel_t(0.2));
    D_CU_CHECK(v.y == channel_t(0.3));
    D_CU_CHECK(v.k == channel_t(0.4));

    // no two inks are transposed
    D_CU_CHECK(v.c != v.m);
    D_CU_CHECK(v.m != v.y);
    D_CU_CHECK(v.y != v.k);

    const cmyk wild(channel_t(-1), channel_t(2), channel_t(0), channel_t(9));
    D_CU_CHECK(wild.c == channel_t(-1));
    D_CU_CHECK(wild.k == channel_t(9));

    return true;
}

/*
tests_color_umbrella_cmyk_copy_and_assignment
  Verifies copy construction and assignment reproduce every channel, and that
  copies are independent of their source.
  Tests the following:
  - a copy carries all four inks
  - assignment overwrites all four
  - mutating the copy leaves the original alone
*/
bool
tests_color_umbrella_cmyk_copy_and_assignment()
{
    const cmyk src(channel_t(0.1), channel_t(0.2), channel_t(0.3),
                   channel_t(0.4));

    const cmyk copied(src);
    D_CU_CHECK(copied == src);

    cmyk target;
    target = src;
    D_CU_CHECK(target == src);

    target.k = channel_t(1);
    D_CU_CHECK(target.k == channel_t(1));
    D_CU_CHECK(src.k == channel_t(0.4));
    D_CU_CHECK(!(target == src));

    D_CU_CHECK(std::is_copy_constructible<cmyk>::value);
    D_CU_CHECK(std::is_copy_assignable<cmyk>::value);

    return true;
}

/*
tests_color_umbrella_cmyk_equality_detects_each_channel
  Verifies changing any one of the four inks alone breaks equality, so no
  channel is omitted from the comparison.
  Tests the following:
  - four one-channel perturbations of a base value each compare unequal
  - an identical value compares equal
*/
bool
tests_color_umbrella_cmyk_equality_detects_each_channel()
{
    const cmyk base(channel_t(0.1), channel_t(0.2), channel_t(0.3),
                    channel_t(0.4));

    D_CU_CHECK(base == cmyk(channel_t(0.1), channel_t(0.2), channel_t(0.3),
                            channel_t(0.4)));

    D_CU_CHECK(!(base == cmyk(channel_t(0.9), channel_t(0.2), channel_t(0.3),
                              channel_t(0.4))));
    D_CU_CHECK(!(base == cmyk(channel_t(0.1), channel_t(0.9), channel_t(0.3),
                              channel_t(0.4))));
    D_CU_CHECK(!(base == cmyk(channel_t(0.1), channel_t(0.2), channel_t(0.9),
                              channel_t(0.4))));
    D_CU_CHECK(!(base == cmyk(channel_t(0.1), channel_t(0.2), channel_t(0.3),
                              channel_t(0.9))));

    return true;
}

/*
tests_color_umbrella_cmyk_inequality_is_declared_not_rewritten
  Verifies operator!= is declared outright on this type and agrees with the
  negation of operator==, so != is available at every language level and needs
  no C++20 gating.
  Tests the following:
  - != is false for equal values and true for a differing one
  - != always equals !(==)
*/
bool
tests_color_umbrella_cmyk_inequality_is_declared_not_rewritten()
{
    const cmyk a(channel_t(0.1), channel_t(0.2), channel_t(0.3),
                 channel_t(0.4));
    const cmyk b(channel_t(0.1), channel_t(0.2), channel_t(0.3),
                 channel_t(0.4));
    const cmyk c(channel_t(0.1), channel_t(0.2), channel_t(0.3),
                 channel_t(0.9));

    D_CU_CHECK(!(a != b));
    D_CU_CHECK(a != c);

    D_CU_CHECK((a != b) == !(a == b));
    D_CU_CHECK((a != c) == !(a == c));

    return true;
}

/*
tests_color_umbrella_cmyk_equality_is_constexpr
  Verifies both comparison operators are usable in a constant expression.
  Tests the following:
  - a true and a false comparison evaluate at compile time, under both
    operators
*/
bool
tests_color_umbrella_cmyk_equality_is_constexpr()
{
    D_CONSTEXPR cmyk a(channel_t(0), channel_t(1), channel_t(1),
                       channel_t(0));
    D_CONSTEXPR cmyk b(channel_t(0), channel_t(1), channel_t(1),
                       channel_t(0));
    D_CONSTEXPR cmyk c(channel_t(0), channel_t(0), channel_t(0),
                       channel_t(1));

    D_CONSTEXPR bool same = (a == b);
    D_CONSTEXPR bool diff = (a == c);
    D_CONSTEXPR bool ne   = (a != c);

    static_assert(same, "operator== true case must be constexpr");
    static_assert(!diff, "operator== false case must be constexpr");
    static_assert(ne, "operator!= must be constexpr");

    D_CU_CHECK(same);
    D_CU_CHECK(!diff);
    D_CU_CHECK(ne);

    return true;
}

/*
tests_color_umbrella_cmyk_construction_is_constexpr
  Verifies both constructors are usable in a constant expression, so an ink
  colour can be a compile-time constant.
  Tests the following:
  - default and channel construction evaluate at compile time
*/
bool
tests_color_umbrella_cmyk_construction_is_constexpr()
{
    D_CONSTEXPR cmyk zero;
    D_CONSTEXPR cmyk red(channel_t(0), channel_t(1), channel_t(1),
                         channel_t(0));

    static_assert(zero.c == channel_t(0), "default ctor must be constexpr");
    static_assert(red.m == channel_t(1), "channel ctor must be constexpr");
    static_assert(red.k == channel_t(0), "channel ctor must be constexpr");

    D_CU_CHECK(zero.c == channel_t(0));
    D_CU_CHECK(red.m == channel_t(1));

    return true;
}

/*
tests_color_umbrella_cmyk_layout_is_flat
  Verifies cmyk is a standard-layout, trivially copyable aggregate of exactly
  four channels.
  Tests the following:
  - size is exactly four channels and there is no vtable
  - standard layout, trivially copyable and destructible
  - not trivially default constructible, because the default ctor zeroes
  - it carries the model identity that admits it to the graph
*/
bool
tests_color_umbrella_cmyk_layout_is_flat()
{
    D_CU_CHECK(sizeof(cmyk) == 4u * sizeof(channel_t));
    D_CU_CHECK(alignof(cmyk) == alignof(channel_t));

    D_CU_CHECK(std::is_standard_layout<cmyk>::value);
    D_CU_CHECK(std::is_trivially_copyable<cmyk>::value);
    D_CU_CHECK(std::is_trivially_destructible<cmyk>::value);
    D_CU_CHECK(!std::is_polymorphic<cmyk>::value);
    D_CU_CHECK(!std::is_trivially_default_constructible<cmyk>::value);

    D_CU_CHECK((std::is_same<cmyk::model_tag, cmyk_tag>::value));
    D_CU_CHECK((std::is_same<cmyk::value_type, channel_t>::value));
    D_CU_CHECK(is_color_model<cmyk>::value);

    return true;
}

NS_END
NS_END
