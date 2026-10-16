/*******************************************************************************
* djinterp [core]                               color_convert_tests_identity.cpp
*
*
* path:      /tests/djinterp/core/util/color/color_convert_tests_identity.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "color_convert_tests.hpp"

NS_DJINTERP
NS_TESTING

/*
tests_color_convert_identity_returns_the_input_unchanged
  Verifies converting a model to its own type yields a bit-exact copy.
  Tests the following:
  - every channel matches exactly, not merely within tolerance
  - the result compares equal to the source under operator==
*/
bool
tests_color_convert_identity_returns_the_input_unchanged()
{
    const hsl source(channel_t(210), channel_t(0.25), channel_t(0.75));
    const hsl result = color_cast<hsl>(source);

    D_CX_CHECK(result.h == source.h);
    D_CX_CHECK(result.s == source.s);
    D_CX_CHECK(result.l == source.l);
    D_CX_CHECK(result == source);

    return true;
}

/*
tests_color_convert_identity_covers_every_model
  Verifies the identity specialisation is available for all seven models, so
  no model is a special case at the self-edge.
  Tests the following:
  - each model converts to itself and compares equal to its source
*/
bool
tests_color_convert_identity_covers_every_model()
{
    const rgb     a(channel_t(0.25), channel_t(0.5), channel_t(0.75));
    const hsl     b(channel_t(210), channel_t(0.5), channel_t(0.5));
    const hsv     c(channel_t(210), channel_t(0.5), channel_t(0.5));
    const cmyk    d(channel_t(0.1), channel_t(0.2), channel_t(0.3),
                    channel_t(0.4));
    const ycbcr   e(channel_t(0.5), channel_t(0.25), channel_t(-0.25));
    const cie_xyz f(channel_t(0.25), channel_t(0.5), channel_t(0.75));
    const cie_lab g(channel_t(50), channel_t(25), channel_t(-25));

    D_CX_CHECK(color_cast<rgb>(a) == a);
    D_CX_CHECK(color_cast<hsl>(b) == b);
    D_CX_CHECK(color_cast<hsv>(c) == c);
    D_CX_CHECK(color_cast<cmyk>(d) == d);
    D_CX_CHECK(color_cast<ycbcr>(e) == e);
    D_CX_CHECK(color_cast<cie_xyz>(f) == f);
    D_CX_CHECK(color_cast<cie_lab>(g) == g);

    return true;
}

/*
tests_color_convert_identity_does_not_round_trip_through_rgb
  Verifies the specialisation is really taken, by using a value that would NOT
  survive an RGB round trip and showing it survives the identity conversion
  exactly.  This is the test that would catch the specialisation being dropped.
  Tests the following:
  - an HSL value whose hue is undefined at zero saturation keeps its hue
    through the identity conversion
  - the same value routed deliberately through RGB and back LOSES that hue,
    establishing the two paths are genuinely different
*/
bool
tests_color_convert_identity_does_not_round_trip_through_rgb()
{
    // zero saturation makes the hue arbitrary: any RGB round trip discards it
    const hsl source(channel_t(210), channel_t(0), channel_t(0.5));

    const hsl identity = color_cast<hsl>(source);
    D_CX_CHECK(identity.h == channel_t(210));
    D_CX_CHECK(identity == source);

    // the deliberate detour collapses the hue to zero
    const hsl detoured = color_cast<hsl>(color_cast<rgb>(source));
    D_CX_CHECK(detoured.h == channel_t(0));
    D_CX_CHECK(!(detoured == source));

    return true;
}

/*
tests_color_convert_identity_preserves_out_of_gamut_values
  Verifies the identity path applies no validation or clamping, so
  out-of-range channels pass through untouched.  Conversion and
  canonicalisation are separate concerns.
  Tests the following:
  - an invalid HSL survives the identity conversion unchanged and stays
    invalid
  - a LAB value with absurd chroma is likewise untouched
*/
bool
tests_color_convert_identity_preserves_out_of_gamut_values()
{
    const hsl wild(channel_t(400), channel_t(2), channel_t(-1));
    const hsl out = color_cast<hsl>(wild);

    D_CX_CHECK(out.h == channel_t(400));
    D_CX_CHECK(out.s == channel_t(2));
    D_CX_CHECK(out.l == channel_t(-1));
    D_CX_CHECK(!out.is_valid());

    const cie_lab wild_lab(channel_t(50), channel_t(1000), channel_t(-1000));
    const cie_lab lab_out = color_cast<cie_lab>(wild_lab);
    D_CX_CHECK(lab_out.a == channel_t(1000));
    D_CX_CHECK(lab_out.b == channel_t(-1000));

    return true;
}

/*
tests_color_convert_identity_is_constexpr
  Verifies the identity conversion is usable in a constant expression - it
  performs no kernel call, so nothing blocks constant evaluation.
  Tests the following:
  - a compile-time model converts to itself at compile time, checked by
    static_assert
*/
bool
tests_color_convert_identity_is_constexpr()
{
    D_CONSTEXPR hsl source(channel_t(210), channel_t(0.25), channel_t(0.75));
    D_CONSTEXPR hsl result = color_convert<hsl, hsl>::apply(source);

    static_assert(result.h == channel_t(210), "identity must be constexpr");
    static_assert(result.s == channel_t(0.25), "identity must be constexpr");

    D_CX_CHECK(result.h == channel_t(210));
    D_CX_CHECK(result == source);

    return true;
}

NS_END
NS_END
