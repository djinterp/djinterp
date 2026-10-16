/*******************************************************************************
* djinterp [core]                               color_rgb_tests_construction.cpp
*
*
* path:      /tests/djinterp/core/util/color/color_rgb_tests_construction.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "color_rgb_tests.hpp"

NS_DJINTERP
NS_TESTING

/*
tests_color_rgb_default_is_zeroed
  Verifies the rgb default constructor zeroes all three channels.
  Tests the following:
  - r, g and b are each exactly zero
  - the zeroed value denotes black
*/
bool
tests_color_rgb_default_is_zeroed()
{
    const rgb v;

    D_CR_CHECK(cr_rgb_same(v, channel_t(0), channel_t(0), channel_t(0)));
    D_CR_CHECK(v.r == channel_t(0));
    D_CR_CHECK(v.g == channel_t(0));
    D_CR_CHECK(v.b == channel_t(0));

    return true;
}

/*
tests_color_rgb_parameterized_stores_channels_verbatim
  Verifies the rgb channel constructor stores its arguments unchanged and in
  order.
  Tests the following:
  - values land on the matching channel, in r, g, b order
  - out-of-unit values are stored as given - this type imposes no policy
*/
bool
tests_color_rgb_parameterized_stores_channels_verbatim()
{
    const rgb v(channel_t(0.25), channel_t(0.5), channel_t(0.75));

    D_CR_CHECK(cr_rgb_same(v, channel_t(0.25), channel_t(0.5),
                              channel_t(0.75)));

    // argument order is r, g, b - no two channels are transposed
    D_CR_CHECK(v.r != v.g);
    D_CR_CHECK(v.g != v.b);

    const rgb wild(channel_t(-1), channel_t(2), channel_t(1e6));
    D_CR_CHECK(cr_rgb_same(wild, channel_t(-1), channel_t(2),
                                 channel_t(1e6)));

    return true;
}

/*
tests_color_rgb_copy_and_assignment
  Verifies rgb copy construction and assignment reproduce every channel, and
  that copies are independent of their source.
  Tests the following:
  - a copy carries the same channels
  - assignment overwrites all three
  - mutating the copy leaves the original alone
*/
bool
tests_color_rgb_copy_and_assignment()
{
    const rgb src(channel_t(0.1), channel_t(0.2), channel_t(0.3));

    const rgb copied(src);
    D_CR_CHECK(cr_rgb_same(copied, channel_t(0.1), channel_t(0.2),
                                   channel_t(0.3)));

    rgb target;
    target = src;
    D_CR_CHECK(cr_rgb_same(target, channel_t(0.1), channel_t(0.2),
                                   channel_t(0.3)));

    target.r = channel_t(0.9);
    D_CR_CHECK(target.r == channel_t(0.9));
    D_CR_CHECK(src.r == channel_t(0.1));

    D_CR_CHECK(std::is_copy_constructible<rgb>::value);
    D_CR_CHECK(std::is_copy_assignable<rgb>::value);

    return true;
}

/*
tests_color_rgb_construction_is_constexpr
  Verifies both rgb constructors are usable in a constant expression, so an
  rgb can be a compile-time colour constant.
  Tests the following:
  - default and channel construction evaluate at compile time, checked by
    static_assert on the resulting channels
*/
bool
tests_color_rgb_construction_is_constexpr()
{
    D_CONSTEXPR rgb zero;
    D_CONSTEXPR rgb red(channel_t(1), channel_t(0), channel_t(0));

    static_assert(zero.r == channel_t(0), "default ctor must be constexpr");
    static_assert(red.r == channel_t(1), "channel ctor must be constexpr");
    static_assert(red.g == channel_t(0), "channel ctor must be constexpr");

    D_CR_CHECK(zero.r == channel_t(0));
    D_CR_CHECK(red.r == channel_t(1));

    return true;
}

/*
tests_color_rgb_layout_is_flat
  Verifies rgb is a standard-layout, trivially copyable aggregate of exactly
  three channels with no hidden state - it can be memcpy'd and handed to a
  graphics API expecting three floats.
  Tests the following:
  - size is exactly three channels and there is no vtable
  - standard layout, trivially copyable and destructible
  - not trivially default constructible, because the default ctor zeroes
*/
bool
tests_color_rgb_layout_is_flat()
{
    D_CR_CHECK(sizeof(rgb) == 3u * sizeof(channel_t));
    D_CR_CHECK(alignof(rgb) == alignof(channel_t));

    D_CR_CHECK(std::is_standard_layout<rgb>::value);
    D_CR_CHECK(std::is_trivially_copyable<rgb>::value);
    D_CR_CHECK(std::is_trivially_destructible<rgb>::value);
    D_CR_CHECK(!std::is_polymorphic<rgb>::value);
    D_CR_CHECK(!std::is_trivially_default_constructible<rgb>::value);

    return true;
}

/*
tests_color_rgba_default_is_zeroed
  Verifies the rgba default constructor zeroes colour AND alpha.  The default
  is therefore fully transparent black, not opaque black - worth pinning down,
  because the opposite convention is common elsewhere.
  Tests the following:
  - all four channels are exactly zero
  - alpha specifically is zero, not one
*/
bool
tests_color_rgba_default_is_zeroed()
{
    const rgba v;

    D_CR_CHECK(cr_rgba_same(v, channel_t(0), channel_t(0), channel_t(0),
                               channel_t(0)));
    D_CR_CHECK(v.a == channel_t(0));
    D_CR_CHECK(v.a != channel_t(1));

    return true;
}

/*
tests_color_rgba_parameterized_stores_channels_verbatim
  Verifies the rgba channel constructor stores all four arguments unchanged
  and in order.
  Tests the following:
  - values land on the matching channel, in r, g, b, a order
  - alpha is the fourth argument, distinct from the colour channels
*/
bool
tests_color_rgba_parameterized_stores_channels_verbatim()
{
    const rgba v(channel_t(0.1), channel_t(0.2), channel_t(0.3),
                 channel_t(0.4));

    D_CR_CHECK(cr_rgba_same(v, channel_t(0.1), channel_t(0.2), channel_t(0.3),
                               channel_t(0.4)));

    D_CR_CHECK(v.r != v.g);
    D_CR_CHECK(v.g != v.b);
    D_CR_CHECK(v.b != v.a);

    return true;
}

/*
tests_color_rgba_from_rgb_and_alpha
  Verifies the composing constructor takes an rgb and an alpha and copies the
  three colour channels across intact - the normal way an opaque colour picks
  up transparency.
  Tests the following:
  - the colour channels match the source rgb exactly
  - alpha takes the supplied value
  - the source rgb is unaffected
*/
bool
tests_color_rgba_from_rgb_and_alpha()
{
    const rgb  colour(channel_t(0.25), channel_t(0.5), channel_t(0.75));
    const rgba v(colour, channel_t(0.5));

    D_CR_CHECK(cr_rgba_same(v, channel_t(0.25), channel_t(0.5),
                               channel_t(0.75), channel_t(0.5)));

    D_CR_CHECK(v.r == colour.r);
    D_CR_CHECK(v.g == colour.g);
    D_CR_CHECK(v.b == colour.b);

    // fully opaque and fully transparent both compose cleanly
    const rgba opaque(colour, channel_t(1));
    const rgba clear(colour, channel_t(0));
    D_CR_CHECK(opaque.a == channel_t(1));
    D_CR_CHECK(clear.a == channel_t(0));
    D_CR_CHECK(opaque.r == clear.r);

    return true;
}

/*
tests_color_rgba_copy_and_assignment
  Verifies rgba copy construction and assignment reproduce every channel
  including alpha.
  Tests the following:
  - a copy carries all four channels
  - assignment overwrites all four
  - mutating alpha on the copy leaves the original alone
*/
bool
tests_color_rgba_copy_and_assignment()
{
    const rgba src(channel_t(0.1), channel_t(0.2), channel_t(0.3),
                   channel_t(0.4));

    const rgba copied(src);
    D_CR_CHECK(cr_rgba_same(copied, channel_t(0.1), channel_t(0.2),
                                    channel_t(0.3), channel_t(0.4)));

    rgba target;
    target = src;
    D_CR_CHECK(cr_rgba_same(target, channel_t(0.1), channel_t(0.2),
                                    channel_t(0.3), channel_t(0.4)));

    target.a = channel_t(1);
    D_CR_CHECK(target.a == channel_t(1));
    D_CR_CHECK(src.a == channel_t(0.4));

    D_CR_CHECK(std::is_copy_constructible<rgba>::value);
    D_CR_CHECK(std::is_copy_assignable<rgba>::value);

    return true;
}

/*
tests_color_rgba_construction_is_constexpr
  Verifies all three rgba constructors are usable in a constant expression.
  Tests the following:
  - default, four-channel and composing construction all evaluate at compile
    time, checked by static_assert on the resulting channels
*/
bool
tests_color_rgba_construction_is_constexpr()
{
    D_CONSTEXPR rgba zero;
    D_CONSTEXPR rgba made(channel_t(0.1), channel_t(0.2), channel_t(0.3),
                          channel_t(0.4));
    D_CONSTEXPR rgb  colour(channel_t(1), channel_t(0), channel_t(0));
    D_CONSTEXPR rgba composed(colour, channel_t(0.5));

    static_assert(zero.a == channel_t(0), "default ctor must be constexpr");
    static_assert(made.a == channel_t(0.4), "channel ctor must be constexpr");
    static_assert(composed.r == channel_t(1), "composing ctor is constexpr");
    static_assert(composed.a == channel_t(0.5), "composing ctor is constexpr");

    D_CR_CHECK(zero.a == channel_t(0));
    D_CR_CHECK(made.a == channel_t(0.4));
    D_CR_CHECK(composed.r == channel_t(1));

    return true;
}

/*
tests_color_rgba_layout_is_flat
  Verifies rgba is a standard-layout, trivially copyable aggregate of exactly
  four channels, so a buffer of them can be uploaded verbatim.
  Tests the following:
  - size is exactly four channels, one more than rgb
  - standard layout, trivially copyable and destructible
  - not trivially default constructible, because the default ctor zeroes
*/
bool
tests_color_rgba_layout_is_flat()
{
    D_CR_CHECK(sizeof(rgba) == 4u * sizeof(channel_t));
    D_CR_CHECK(sizeof(rgba) == sizeof(rgb) + sizeof(channel_t));
    D_CR_CHECK(alignof(rgba) == alignof(channel_t));

    D_CR_CHECK(std::is_standard_layout<rgba>::value);
    D_CR_CHECK(std::is_trivially_copyable<rgba>::value);
    D_CR_CHECK(std::is_trivially_destructible<rgba>::value);
    D_CR_CHECK(!std::is_polymorphic<rgba>::value);
    D_CR_CHECK(!std::is_trivially_default_constructible<rgba>::value);

    return true;
}

NS_END
NS_END
