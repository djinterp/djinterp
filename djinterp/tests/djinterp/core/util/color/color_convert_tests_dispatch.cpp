/*******************************************************************************
* djinterp [core]                               color_convert_tests_dispatch.cpp
*
*
* path:      /tests/djinterp/core/util/color/color_convert_tests_dispatch.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "color_convert_tests.hpp"

NS_DJINTERP
NS_TESTING

/*
tests_color_convert_to_rgb_overload_per_model
  Verifies every non-RGB model has a to_rgb overload that reaches the hub.
  Without one of these the graph would have an unreachable node.
  Tests the following:
  - each of the six non-RGB models resolves an overload and yields an rgb
  - a representative value from each lands somewhere sensible
*/
bool
tests_color_convert_to_rgb_overload_per_model()
{
    const rgb from_hsl = internal::to_rgb(
        hsl(channel_t(0), channel_t(1), channel_t(0.5)));
    const rgb from_hsv = internal::to_rgb(
        hsv(channel_t(0), channel_t(1), channel_t(1)));
    const rgb from_cmyk = internal::to_rgb(
        cmyk(channel_t(0), channel_t(1), channel_t(1), channel_t(0)));
    const rgb from_ycbcr = internal::to_rgb(
        ycbcr(channel_t(0.299), channel_t(-0.168736), channel_t(0.5)));
    const rgb from_xyz = internal::to_rgb(
        cie_xyz(channel_t(0), channel_t(0), channel_t(0)));
    const rgb from_lab = internal::to_rgb(
        cie_lab(channel_t(0), channel_t(0), channel_t(0)));

    // the first four all denote pure red
    D_CX_CHECK(cx_rgb_near(from_hsl, channel_t(1), channel_t(0),
                           channel_t(0)));
    D_CX_CHECK(cx_rgb_near(from_hsv, channel_t(1), channel_t(0),
                           channel_t(0)));
    D_CX_CHECK(cx_rgb_near(from_cmyk, channel_t(1), channel_t(0),
                           channel_t(0)));
    D_CX_CHECK(cx_rgb_near(from_ycbcr, channel_t(1), channel_t(0),
                           channel_t(0)));

    // and the last two denote black
    D_CX_CHECK(cx_rgb_near(from_xyz, channel_t(0), channel_t(0),
                           channel_t(0)));
    D_CX_CHECK(cx_rgb_near(from_lab, channel_t(0), channel_t(0),
                           channel_t(0)));

    return true;
}

/*
tests_color_convert_to_rgb_from_rgb_is_the_identity
  Verifies to_rgb on an rgb returns it unchanged rather than routing it
  through a conversion, so an rgb source costs nothing to bring to the hub.
  Tests the following:
  - the result is bit-exact, not merely close
  - it holds for an out-of-gamut value too, showing no clamping occurs
*/
bool
tests_color_convert_to_rgb_from_rgb_is_the_identity()
{
    const rgb source(channel_t(0.25), channel_t(0.5), channel_t(0.75));
    const rgb result = internal::to_rgb(source);

    D_CX_CHECK(result.r == source.r);
    D_CX_CHECK(result.g == source.g);
    D_CX_CHECK(result.b == source.b);

    const rgb wild(channel_t(-1), channel_t(2), channel_t(1e6));
    const rgb wild_out = internal::to_rgb(wild);
    D_CX_CHECK(wild_out.r == channel_t(-1));
    D_CX_CHECK(wild_out.g == channel_t(2));
    D_CX_CHECK(wild_out.b == channel_t(1e6));

    return true;
}

/*
tests_color_convert_to_rgb_returns_a_wrapped_rgb
  Verifies the overloads return the C++ wrapper rather than the bare kernel
  POD, so results compose without manual rewrapping at each step.
  Tests the following:
  - the static return type is exactly rgb
  - the result is usable directly as a color model
*/
bool
tests_color_convert_to_rgb_returns_a_wrapped_rgb()
{
    const hsl source(channel_t(120), channel_t(1), channel_t(0.5));

    D_CX_CHECK((std::is_same<decltype(internal::to_rgb(source)), rgb>::value));
    D_CX_CHECK(is_color_model<decltype(internal::to_rgb(source))>::value);

    // and it composes straight into the next leg without a rewrap
    const hsv onward = internal::rgb_to<hsv>::apply(internal::to_rgb(source));
    D_CX_CHECK(cx_near(onward.h, channel_t(120)));

    return true;
}

/*
tests_color_convert_rgb_to_specialization_per_model
  Verifies rgb_to is specialised for every model, so the hub can reach any
  target.  A missing specialisation would be a link-time or compile-time hole
  rather than a wrong answer.
  Tests the following:
  - each of the seven targets resolves and produces a plausible value from
    pure red
*/
bool
tests_color_convert_rgb_to_specialization_per_model()
{
    const rgb red(channel_t(1), channel_t(0), channel_t(0));

    const hsl     as_hsl   = internal::rgb_to<hsl>::apply(red);
    const hsv     as_hsv   = internal::rgb_to<hsv>::apply(red);
    const cmyk    as_cmyk  = internal::rgb_to<cmyk>::apply(red);
    const ycbcr   as_ycbcr = internal::rgb_to<ycbcr>::apply(red);
    const cie_xyz as_xyz   = internal::rgb_to<cie_xyz>::apply(red);
    const cie_lab as_lab   = internal::rgb_to<cie_lab>::apply(red);
    const rgb     as_rgb   = internal::rgb_to<rgb>::apply(red);

    D_CX_CHECK(cx_near(as_hsl.h, channel_t(0)));
    D_CX_CHECK(cx_near(as_hsl.s, channel_t(1)));
    D_CX_CHECK(cx_near(as_hsv.v, channel_t(1)));
    D_CX_CHECK(cx_near(as_cmyk.m, channel_t(1)));
    D_CX_CHECK(cx_near(as_ycbcr.y, channel_t(0.299)));
    D_CX_CHECK(as_xyz.x > channel_t(0));
    D_CX_CHECK(cx_near(as_lab.l, channel_t(53.2407913), channel_t(1e-3)));
    D_CX_CHECK(as_rgb.r == channel_t(1));

    return true;
}

/*
tests_color_convert_rgb_to_rgb_is_the_identity
  Verifies rgb_to<rgb> returns its input unchanged, so the hub-to-hub edge
  costs nothing.
  Tests the following:
  - the result is bit-exact
  - an out-of-gamut value is untouched, confirming no clamping
*/
bool
tests_color_convert_rgb_to_rgb_is_the_identity()
{
    const rgb source(channel_t(0.25), channel_t(0.5), channel_t(0.75));
    const rgb result = internal::rgb_to<rgb>::apply(source);

    D_CX_CHECK(result.r == source.r);
    D_CX_CHECK(result.g == source.g);
    D_CX_CHECK(result.b == source.b);

    const rgb wild(channel_t(-1), channel_t(2), channel_t(0));
    D_CX_CHECK(internal::rgb_to<rgb>::apply(wild).g == channel_t(2));

    return true;
}

/*
tests_color_convert_rgb_to_returns_the_wrapped_target
  Verifies each specialisation returns the wrapper type named in its
  parameter, not the corresponding POD.
  Tests the following:
  - the static return type matches the requested model for several targets
*/
bool
tests_color_convert_rgb_to_returns_the_wrapped_target()
{
    const rgb red(channel_t(1), channel_t(0), channel_t(0));

    D_CX_CHECK((std::is_same<decltype(internal::rgb_to<hsl>::apply(red)),
                             hsl>::value));
    D_CX_CHECK((std::is_same<decltype(internal::rgb_to<cmyk>::apply(red)),
                             cmyk>::value));
    D_CX_CHECK((std::is_same<decltype(internal::rgb_to<cie_lab>::apply(red)),
                             cie_lab>::value));
    D_CX_CHECK((std::is_same<decltype(internal::rgb_to<rgb>::apply(red)),
                             rgb>::value));

    return true;
}

/*
tests_color_convert_dispatch_matches_the_public_entry_point
  Verifies composing the two internal layers by hand gives exactly what
  color_convert produces, confirming the public API adds no arithmetic of its
  own and is purely a routing convenience.
  Tests the following:
  - manual composition and color_cast agree bit-for-bit on several pairs
*/
bool
tests_color_convert_dispatch_matches_the_public_entry_point()
{
    const hsl source(channel_t(210), channel_t(0.5), channel_t(0.5));

    const hsv manual = internal::rgb_to<hsv>::apply(internal::to_rgb(source));
    const hsv routed = color_cast<hsv>(source);

    D_CX_CHECK(manual.h == routed.h);
    D_CX_CHECK(manual.s == routed.s);
    D_CX_CHECK(manual.v == routed.v);

    const cmyk manual_c =
        internal::rgb_to<cmyk>::apply(internal::to_rgb(source));
    const cmyk routed_c = color_cast<cmyk>(source);

    D_CX_CHECK(manual_c.c == routed_c.c);
    D_CX_CHECK(manual_c.m == routed_c.m);
    D_CX_CHECK(manual_c.y == routed_c.y);
    D_CX_CHECK(manual_c.k == routed_c.k);

    return true;
}

NS_END
NS_END
