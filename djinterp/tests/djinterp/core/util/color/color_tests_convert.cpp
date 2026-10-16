/*******************************************************************************
* djinterp [core]                                        color_tests_convert.cpp
*
*
* path:      /tests/djinterp/core/util/color/color_tests_convert.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "color_tests.hpp"

NS_DJINTERP
NS_TESTING

/*
tests_color_umbrella_identity_returns_the_input_unchanged
  Verifies converting a type to itself yields a bit-exact copy.
  Tests the following:
  - every channel matches exactly and the result compares equal
*/
bool
tests_color_umbrella_identity_returns_the_input_unchanged()
{
    const rgb source(channel_t(0.25), channel_t(0.5), channel_t(0.75));
    const rgb result = color_convert<rgb, rgb>::apply(source);

    D_CU_CHECK(result.r == source.r);
    D_CU_CHECK(result.g == source.g);
    D_CU_CHECK(result.b == source.b);
    D_CU_CHECK(result == source);

    return true;
}

/*
tests_color_umbrella_identity_covers_every_model
  Verifies the identity specialisation serves both models this header defines.
  Tests the following:
  - rgb and cmyk each convert to themselves exactly
*/
bool
tests_color_umbrella_identity_covers_every_model()
{
    const rgb  a(channel_t(0.25), channel_t(0.5), channel_t(0.75));
    const cmyk b(channel_t(0.1), channel_t(0.2), channel_t(0.3),
                 channel_t(0.4));

    D_CU_CHECK(color_convert<rgb, rgb>::apply(a) == a);
    D_CU_CHECK(color_convert<cmyk, cmyk>::apply(b) == b);

    return true;
}

/*
tests_color_umbrella_identity_accepts_rgba_despite_it_not_being_a_model
  Verifies the identity specialisation is keyed on type equality alone, so it
  accepts rgba even though rgba carries no model_tag.  This is a genuine gap
  between what the graph will do and what the trait reports, and it is
  asserted here so the behaviour is a decision rather than a surprise.
  Tests the following:
  - rgba converts to itself exactly, alpha included
  - yet is_color_model rejects rgba
  - and is_convertible_color rejects the very pair that just worked
*/
bool
tests_color_umbrella_identity_accepts_rgba_despite_it_not_being_a_model()
{
    const rgba source(channel_t(0.25), channel_t(0.5), channel_t(0.75),
                      channel_t(0.5));
    const rgba result = color_convert<rgba, rgba>::apply(source);

    D_CU_CHECK(result == source);
    D_CU_CHECK(result.a == channel_t(0.5));

    // ... even though rgba is not a colour model
    D_CU_CHECK(!is_color_model<rgba>::value);
    D_CU_CHECK(!(is_convertible_color<rgba, rgba>::value));

    return true;
}

/*
tests_color_umbrella_cmyk_to_rgb_known_values
  Verifies the subtractive-to-additive formula produces the expected values
  for the ink primaries.  Each channel is (1 - ink) * (1 - black).
  Tests the following:
  - no ink is white and full black ink is black
  - the three chromatic ink pairs give the RGB primaries
  - the black channel scales the result multiplicatively
*/
bool
tests_color_umbrella_cmyk_to_rgb_known_values()
{
    D_CU_CHECK(cu_rgb_near(
        color_convert<rgb, cmyk>::apply(
            cmyk(channel_t(0), channel_t(0), channel_t(0), channel_t(0))),
        channel_t(1), channel_t(1), channel_t(1)));

    D_CU_CHECK(cu_rgb_near(
        color_convert<rgb, cmyk>::apply(
            cmyk(channel_t(0), channel_t(0), channel_t(0), channel_t(1))),
        channel_t(0), channel_t(0), channel_t(0)));

    // magenta + yellow = red
    D_CU_CHECK(cu_rgb_near(
        color_convert<rgb, cmyk>::apply(
            cmyk(channel_t(0), channel_t(1), channel_t(1), channel_t(0))),
        channel_t(1), channel_t(0), channel_t(0)));

    // cyan + yellow = green
    D_CU_CHECK(cu_rgb_near(
        color_convert<rgb, cmyk>::apply(
            cmyk(channel_t(1), channel_t(0), channel_t(1), channel_t(0))),
        channel_t(0), channel_t(1), channel_t(0)));

    // cyan + magenta = blue
    D_CU_CHECK(cu_rgb_near(
        color_convert<rgb, cmyk>::apply(
            cmyk(channel_t(1), channel_t(1), channel_t(0), channel_t(0))),
        channel_t(0), channel_t(0), channel_t(1)));

    // black scales multiplicatively: half black over no ink is mid grey
    D_CU_CHECK(cu_rgb_near(
        color_convert<rgb, cmyk>::apply(
            cmyk(channel_t(0), channel_t(0), channel_t(0), channel_t(0.5))),
        channel_t(0.5), channel_t(0.5), channel_t(0.5)));

    return true;
}

/*
tests_color_umbrella_cmyk_to_rgb_is_constexpr
  Verifies the CMYK to RGB edge is a single expression and so is usable in a
  constant expression - unlike its inverse, which branches and is therefore
  runtime-only in this shim.
  Tests the following:
  - the conversion and its resulting channels evaluate at compile time
*/
bool
tests_color_umbrella_cmyk_to_rgb_is_constexpr()
{
    D_CONSTEXPR cmyk ink(channel_t(0), channel_t(1), channel_t(1),
                         channel_t(0));
    D_CONSTEXPR rgb  out = color_convert<rgb, cmyk>::apply(ink);

    static_assert(out.r == channel_t(1), "cmyk->rgb must be constexpr");
    static_assert(out.g == channel_t(0), "cmyk->rgb must be constexpr");
    static_assert(out.b == channel_t(0), "cmyk->rgb must be constexpr");

    D_CU_CHECK(out.r == channel_t(1));
    D_CU_CHECK(out == rgb(channel_t(1), channel_t(0), channel_t(0)));

    return true;
}

/*
tests_color_umbrella_rgb_to_cmyk_known_values
  Verifies the additive-to-subtractive formula produces the expected inks for
  the RGB primaries, taking the ordinary (non-black) branch.
  Tests the following:
  - white needs no ink at all
  - each RGB primary maps to its complementary ink pair with no black
  - mid grey is reached with black ink alone
*/
bool
tests_color_umbrella_rgb_to_cmyk_known_values()
{
    D_CU_CHECK(cu_cmyk_near(
        color_convert<cmyk, rgb>::apply(
            rgb(channel_t(1), channel_t(1), channel_t(1))),
        channel_t(0), channel_t(0), channel_t(0), channel_t(0)));

    D_CU_CHECK(cu_cmyk_near(
        color_convert<cmyk, rgb>::apply(
            rgb(channel_t(1), channel_t(0), channel_t(0))),
        channel_t(0), channel_t(1), channel_t(1), channel_t(0)));

    D_CU_CHECK(cu_cmyk_near(
        color_convert<cmyk, rgb>::apply(
            rgb(channel_t(0), channel_t(1), channel_t(0))),
        channel_t(1), channel_t(0), channel_t(1), channel_t(0)));

    D_CU_CHECK(cu_cmyk_near(
        color_convert<cmyk, rgb>::apply(
            rgb(channel_t(0), channel_t(0), channel_t(1))),
        channel_t(1), channel_t(1), channel_t(0), channel_t(0)));

    // an achromatic input needs only black ink
    D_CU_CHECK(cu_cmyk_near(
        color_convert<cmyk, rgb>::apply(
            rgb(channel_t(0.5), channel_t(0.5), channel_t(0.5))),
        channel_t(0), channel_t(0), channel_t(0), channel_t(0.5)));

    return true;
}

/*
tests_color_umbrella_rgb_to_cmyk_takes_the_black_branch
  Verifies an input with no luminance takes the guarded branch and returns
  pure black ink rather than dividing by zero.  The guard fires when the
  brightest channel is at or below zero, which makes the denominator vanish.
  Tests the following:
  - pure black returns exactly (0, 0, 0, 1)
  - the result is finite, so the division was genuinely avoided
  - a negative input also trips the guard rather than producing nonsense
  - the very next representable colour above black does NOT trip it
*/
bool
tests_color_umbrella_rgb_to_cmyk_takes_the_black_branch()
{
    const cmyk black = color_convert<cmyk, rgb>::apply(
        rgb(channel_t(0), channel_t(0), channel_t(0)));

    D_CU_CHECK(black.c == channel_t(0));
    D_CU_CHECK(black.m == channel_t(0));
    D_CU_CHECK(black.y == channel_t(0));
    D_CU_CHECK(black.k == channel_t(1));

    // finite, so no division by zero leaked through
    D_CU_CHECK(black.c == black.c);
    D_CU_CHECK(black.k < std::numeric_limits<channel_t>::infinity());

    // a negative input trips the same guard
    const cmyk negative = color_convert<cmyk, rgb>::apply(
        rgb(channel_t(-1), channel_t(-1), channel_t(-1)));
    D_CU_CHECK(negative.k == channel_t(1));
    D_CU_CHECK(negative.c == channel_t(0));

    // just above black, the ordinary branch runs instead
    const cmyk nearly = color_convert<cmyk, rgb>::apply(
        rgb(channel_t(0.001), channel_t(0), channel_t(0)));
    D_CU_CHECK(nearly.k != channel_t(1));
    D_CU_CHECK(nearly.m == channel_t(1));

    return true;
}

/*
tests_color_umbrella_rgb_cmyk_round_trip
  Verifies the two edges are mutual inverses across the cube corners and the
  greys.
  Tests the following:
  - every cube corner survives RGB -> CMYK -> RGB within tolerance
  - a grey ramp survives too, exercising the black channel throughout
*/
bool
tests_color_umbrella_rgb_cmyk_round_trip()
{
    const rgb corners[] = {
        rgb(channel_t(0), channel_t(0), channel_t(0)),
        rgb(channel_t(1), channel_t(0), channel_t(0)),
        rgb(channel_t(0), channel_t(1), channel_t(0)),
        rgb(channel_t(0), channel_t(0), channel_t(1)),
        rgb(channel_t(1), channel_t(1), channel_t(0)),
        rgb(channel_t(0), channel_t(1), channel_t(1)),
        rgb(channel_t(1), channel_t(0), channel_t(1)),
        rgb(channel_t(1), channel_t(1), channel_t(1)),
        rgb(channel_t(0.25), channel_t(0.5), channel_t(0.75)),
    };

    std::size_t i = 0;
    for (i = 0; i < sizeof(corners) / sizeof(corners[0]); ++i)
    {
        const cmyk out  = color_convert<cmyk, rgb>::apply(corners[i]);
        const rgb  back = color_convert<rgb, cmyk>::apply(out);

        D_CU_CHECK(cu_rgb_near(back, corners[i].r, corners[i].g,
                               corners[i].b));
    }

    const channel_t greys[] = {
        channel_t(0.1), channel_t(0.25), channel_t(0.5), channel_t(0.9),
    };

    for (i = 0; i < sizeof(greys) / sizeof(greys[0]); ++i)
    {
        const rgb  grey(greys[i], greys[i], greys[i]);
        const cmyk out  = color_convert<cmyk, rgb>::apply(grey);
        const rgb  back = color_convert<rgb, cmyk>::apply(out);

        D_CU_CHECK(cu_rgb_near(back, greys[i], greys[i], greys[i]));
        D_CU_CHECK(cu_near(out.k, channel_t(1) - greys[i]));
    }

    return true;
}

/*
tests_color_umbrella_only_the_declared_edges_exist
  Verifies the primary template is left INCOMPLETE, so exactly the declared
  pairs are usable and any other pair is a compile error.  That incompleteness
  is the whole mechanism by which this shim refuses to mis-convert, so it is
  detected here rather than assumed: cu_edge_exists reports whether a given
  specialisation can have its size taken.
  Tests the following:
  - the three declared edges are complete types
  - the identity is complete for any type, since it is keyed on type equality
  - a pair with no specialisation is INCOMPLETE and therefore unusable
*/
bool
tests_color_umbrella_only_the_declared_edges_exist()
{
    // the declared edges
    D_CU_CHECK((cu_edge_exists<color_convert<rgb, cmyk> >::value));
    D_CU_CHECK((cu_edge_exists<color_convert<cmyk, rgb> >::value));
    D_CU_CHECK((cu_edge_exists<color_convert<rgb, rgb> >::value));
    D_CU_CHECK((cu_edge_exists<color_convert<cmyk, cmyk> >::value));

    // the identity is keyed on type equality alone, so it covers any type
    D_CU_CHECK((cu_edge_exists<color_convert<rgba, rgba> >::value));

    // ... but there is no edge between rgba and either model
    D_CU_CHECK(!(cu_edge_exists<color_convert<rgb, rgba> >::value));
    D_CU_CHECK(!(cu_edge_exists<color_convert<rgba, rgb> >::value));
    D_CU_CHECK(!(cu_edge_exists<color_convert<cmyk, rgba> >::value));
    D_CU_CHECK(!(cu_edge_exists<color_convert<rgba, cmyk> >::value));

    return true;
}

NS_END
NS_END
