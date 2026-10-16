/*******************************************************************************
* djinterp [core]                                  color_rgb_tests_transport.cpp
*
*
* path:      /tests/djinterp/core/util/color/color_rgb_tests_transport.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "color_rgb_tests.hpp"

NS_DJINTERP
NS_TESTING

/*
tests_color_rgba_to_rgb_drops_alpha
  Verifies to_rgb returns an rgb, so alpha is discarded at a visible call site
  and cannot leak into a chromatic computation.
  Tests the following:
  - the return type is exactly rgb, not rgba
  - the result is a color model even though its source was not
*/
bool
tests_color_rgba_to_rgb_drops_alpha()
{
    const rgba v(channel_t(0.25), channel_t(0.5), channel_t(0.75),
                 channel_t(0.5));

    D_CR_CHECK((std::is_same<decltype(v.to_rgb()), rgb>::value));
    D_CR_CHECK(!(std::is_same<decltype(v.to_rgb()), rgba>::value));

    // the conversion is what promotes the value into the graph
    D_CR_CHECK(!is_color_model<rgba>::value);
    D_CR_CHECK(is_color_model<decltype(v.to_rgb())>::value);

    return true;
}

/*
tests_color_rgba_to_rgb_preserves_color_channels
  Verifies the three colour channels survive to_rgb bit-exact.
  Tests the following:
  - r, g and b match the source exactly
  - the result equals an rgb built from the same three values
*/
bool
tests_color_rgba_to_rgb_preserves_color_channels()
{
    const rgba v(channel_t(0.25), channel_t(0.5), channel_t(0.75),
                 channel_t(0.125));
    const rgb  c = v.to_rgb();

    D_CR_CHECK(cr_rgb_same(c, channel_t(0.25), channel_t(0.5),
                              channel_t(0.75)));
    D_CR_CHECK(c.r == v.r);
    D_CR_CHECK(c.g == v.g);
    D_CR_CHECK(c.b == v.b);

    D_CR_CHECK(c == rgb(channel_t(0.25), channel_t(0.5), channel_t(0.75)));

    return true;
}

/*
tests_color_rgba_to_rgb_is_constexpr
  Verifies to_rgb is usable in a constant expression, so a compile-time rgba
  constant can be reduced to a compile-time rgb.
  Tests the following:
  - the call and the resulting channels evaluate at compile time
*/
bool
tests_color_rgba_to_rgb_is_constexpr()
{
    D_CONSTEXPR rgba v(channel_t(1), channel_t(0), channel_t(0),
                       channel_t(0.5));
    D_CONSTEXPR rgb  c = v.to_rgb();

    static_assert(c.r == channel_t(1), "to_rgb must be constexpr");
    static_assert(c.g == channel_t(0), "to_rgb must be constexpr");
    static_assert(c.b == channel_t(0), "to_rgb must be constexpr");

    D_CR_CHECK(c.r == channel_t(1));
    D_CR_CHECK(c == rgb(channel_t(1), channel_t(0), channel_t(0)));

    return true;
}

/*
tests_color_rgba_round_trips_through_rgb
  Verifies composing an rgba from an rgb and then calling to_rgb recovers the
  original colour exactly, whatever alpha was attached.
  Tests the following:
  - the round trip is the identity on the colour channels
  - it holds at transparent, partial and opaque alpha alike
*/
bool
tests_color_rgba_round_trips_through_rgb()
{
    const rgb source(channel_t(0.25), channel_t(0.5), channel_t(0.75));

    const channel_t alphas[] = {
        channel_t(0), channel_t(0.25), channel_t(0.5), channel_t(1),
    };

    std::size_t i = 0;
    for (i = 0; i < sizeof(alphas) / sizeof(alphas[0]); ++i)
    {
        const rgba wrapped(source, alphas[i]);
        D_CR_CHECK(wrapped.to_rgb() == source);
        D_CR_CHECK(wrapped.a == alphas[i]);
    }

    return true;
}

/*
tests_color_rgba_alpha_is_independent_of_color
  Verifies writing alpha never disturbs the colour channels, and writing a
  colour channel never disturbs alpha - the four channels are genuinely
  orthogonal storage.
  Tests the following:
  - mutating alpha leaves r, g and b untouched
  - mutating a colour channel leaves alpha untouched
*/
bool
tests_color_rgba_alpha_is_independent_of_color()
{
    rgba v(channel_t(0.25), channel_t(0.5), channel_t(0.75), channel_t(1));

    v.a = channel_t(0);
    D_CR_CHECK(cr_rgba_same(v, channel_t(0.25), channel_t(0.5),
                               channel_t(0.75), channel_t(0)));

    v.g = channel_t(0.1);
    D_CR_CHECK(cr_rgba_same(v, channel_t(0.25), channel_t(0.1),
                               channel_t(0.75), channel_t(0)));

    D_CR_CHECK(v.a == channel_t(0));

    return true;
}

/*
tests_color_rgba_to_rgb_ignores_the_alpha_value
  Verifies to_rgb performs no premultiplication: the colour is returned
  unscaled even at zero alpha.  Straight alpha in, straight colour out.
  Tests the following:
  - two rgba values differing only in alpha yield the SAME rgb
  - a fully transparent colour still reports its stored channels rather than
    collapsing to black
*/
bool
tests_color_rgba_to_rgb_ignores_the_alpha_value()
{
    const rgb colour(channel_t(0.25), channel_t(0.5), channel_t(0.75));

    const rgba opaque(colour, channel_t(1));
    const rgba clear(colour, channel_t(0));

    D_CR_CHECK(opaque.to_rgb() == clear.to_rgb());
    D_CR_CHECK(!(opaque == clear));

    // no premultiplication: zero alpha does NOT zero the colour
    D_CR_CHECK(clear.to_rgb() == colour);
    D_CR_CHECK(!(clear.to_rgb() == rgb()));
    D_CR_CHECK(clear.to_rgb().r == channel_t(0.25));

    return true;
}

NS_END
NS_END
