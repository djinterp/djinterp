// djinterp [test]  font_tests_feature_ops.cpp
//   Section 9b: the gated free functions - decorations, casing, metric
// overrides, the stretch/spacing axes, and foreground/background color.
//
//   Each of these carries a static_assert on its feature flag.  That guard
// is a HARD compile error by design, not a SFINAE soft failure: the assert
// sits in the function BODY, which is not instantiated in an unevaluated
// context, so no detection idiom can observe it.  The observable consequence
// of a missing flag is therefore the missing MEMBER, and that is what
// section 10's traits pin down (has_font_underline_v<font<ff_none>> is
// false).  Here we test the positive side: with the flag on, each setter
// does exactly what it says and nothing else.

// djinterp
#include "font_tests.hpp"


NS_DJINTERP
NS_TESTING

/*
tests_fn_set_underline
  Tests the following:
  - the underline turns on and back off
  - the call is idempotent
  - nothing in the core moves
*/
bool
tests_fn_set_underline()
{
    decorated_font_t f("Inter", 12.0f);

    D_FT_CHECK(!f.underline);

    fn_set_underline(f, true);

    D_FT_CHECK(f.underline);

    // idempotent
    fn_set_underline(f, true);

    D_FT_CHECK(f.underline);

    fn_set_underline(f, false);

    D_FT_CHECK(!f.underline);

    // the core is untouched throughout
    D_FT_CHECK(f.family == "Inter");
    D_FT_CHECK_NEAR(f.size, 12.0f);
    D_FT_CHECK(f.weight == font_weight::normal);

    return true;
}

/*
tests_fn_set_strikethrough
  Tests the following:
  - the strikethrough turns on and back off
  - it works on a font that enables ONLY strikethrough
*/
bool
tests_fn_set_strikethrough()
{
    strike_font f;

    D_FT_CHECK(!f.strikethrough);

    fn_set_strikethrough(f, true);

    D_FT_CHECK(f.strikethrough);

    fn_set_strikethrough(f, false);

    D_FT_CHECK(!f.strikethrough);

    return true;
}

/*
tests_fn_set_overline
  Tests the following:
  - the overline turns on and back off
  - it works on a font that enables ONLY overline
*/
bool
tests_fn_set_overline()
{
    overline_font_t f;

    D_FT_CHECK(!f.overline);

    fn_set_overline(f, true);

    D_FT_CHECK(f.overline);

    fn_set_overline(f, false);

    D_FT_CHECK(!f.overline);

    return true;
}

/*
tests_fn_decorations_are_independent
  Three separate mixins, three separate bools.  A setter that wrote the
wrong member would still pass a test that only ever set one at a time.
  Tests the following:
  - setting underline leaves strikethrough and overline alone
  - setting strikethrough leaves the other two alone
  - setting overline leaves the other two alone
  - all eight combinations are reachable
*/
bool
tests_fn_decorations_are_independent()
{
    decorated_font_t f;

    fn_set_underline(f, true);

    D_FT_CHECK(f.underline);
    D_FT_CHECK(!f.strikethrough);
    D_FT_CHECK(!f.overline);

    fn_set_strikethrough(f, true);

    D_FT_CHECK(f.underline);          // still on
    D_FT_CHECK(f.strikethrough);
    D_FT_CHECK(!f.overline);

    fn_set_overline(f, true);

    D_FT_CHECK(f.underline);
    D_FT_CHECK(f.strikethrough);
    D_FT_CHECK(f.overline);

    // clearing one leaves the others
    fn_set_strikethrough(f, false);

    D_FT_CHECK(f.underline);
    D_FT_CHECK(!f.strikethrough);
    D_FT_CHECK(f.overline);

    // sweep all eight corners
    for (int mask = 0; mask < 8; ++mask)
    {
        decorated_font_t g;

        fn_set_underline(g,     (mask & 1) != 0);
        fn_set_strikethrough(g, (mask & 2) != 0);
        fn_set_overline(g,      (mask & 4) != 0);

        D_FT_CHECK(g.underline     == ((mask & 1) != 0));
        D_FT_CHECK(g.strikethrough == ((mask & 2) != 0));
        D_FT_CHECK(g.overline      == ((mask & 4) != 0));
    }

    return true;
}

/*
tests_fn_set_small_caps
  Tests the following:
  - small caps turns on and back off
*/
bool
tests_fn_set_small_caps()
{
    small_caps_font_t f;

    D_FT_CHECK(!f.small_caps);

    fn_set_small_caps(f, true);

    D_FT_CHECK(f.small_caps);

    fn_set_small_caps(f, false);

    D_FT_CHECK(!f.small_caps);

    return true;
}

/*
tests_fn_set_all_caps
  Tests the following:
  - all caps turns on and back off
*/
bool
tests_fn_set_all_caps()
{
    casing_font_t f;

    D_FT_CHECK(!f.all_caps);

    fn_set_all_caps(f, true);

    D_FT_CHECK(f.all_caps);

    fn_set_all_caps(f, false);

    D_FT_CHECK(!f.all_caps);

    return true;
}

/*
tests_fn_set_subscript
  Tests the following:
  - subscript turns on and back off
*/
bool
tests_fn_set_subscript()
{
    casing_font_t f;

    D_FT_CHECK(!f.subscript);

    fn_set_subscript(f, true);

    D_FT_CHECK(f.subscript);

    fn_set_subscript(f, false);

    D_FT_CHECK(!f.subscript);

    return true;
}

/*
tests_fn_set_superscript
  Tests the following:
  - superscript turns on and back off
*/
bool
tests_fn_set_superscript()
{
    casing_font_t f;

    D_FT_CHECK(!f.superscript);

    fn_set_superscript(f, true);

    D_FT_CHECK(f.superscript);

    fn_set_superscript(f, false);

    D_FT_CHECK(!f.superscript);

    return true;
}

/*
tests_fn_casing_are_independent
  Tests the following:
  - each of the four casing setters writes its own member and no other
  - all sixteen combinations are reachable
*/
bool
tests_fn_casing_are_independent()
{
    casing_font_t f;

    fn_set_small_caps(f, true);

    D_FT_CHECK(f.small_caps);
    D_FT_CHECK(!f.all_caps);
    D_FT_CHECK(!f.subscript);
    D_FT_CHECK(!f.superscript);

    fn_set_superscript(f, true);

    D_FT_CHECK(f.small_caps);       // untouched
    D_FT_CHECK(!f.all_caps);
    D_FT_CHECK(!f.subscript);
    D_FT_CHECK(f.superscript);

    // sweep all sixteen corners
    for (int mask = 0; mask < 16; ++mask)
    {
        casing_font_t g;

        fn_set_small_caps(g,  (mask & 1) != 0);
        fn_set_all_caps(g,    (mask & 2) != 0);
        fn_set_subscript(g,   (mask & 4) != 0);
        fn_set_superscript(g, (mask & 8) != 0);

        D_FT_CHECK(g.small_caps  == ((mask & 1) != 0));
        D_FT_CHECK(g.all_caps    == ((mask & 2) != 0));
        D_FT_CHECK(g.subscript   == ((mask & 4) != 0));
        D_FT_CHECK(g.superscript == ((mask & 8) != 0));
    }

    return true;
}

/*
tests_fn_casing_permits_contradictory_combinations
  The model enforces NO policy between the casing flags.  Subscript and
superscript can both be on; so can small caps and all caps.  Those are
typographically incoherent, and it is the RENDERER's job to say so - the
data model just carries what it is told.  Worth pinning: a future
"validation" that started rejecting these would be an API break.
  Tests the following:
  - subscript and superscript can be true at the same time
  - small_caps and all_caps can be true at the same time
  - all four can be true at once
*/
bool
tests_fn_casing_permits_contradictory_combinations()
{
    casing_font_t f;

    // sub and super, together
    fn_set_subscript(f, true);
    fn_set_superscript(f, true);

    D_FT_CHECK(f.subscript);
    D_FT_CHECK(f.superscript);

    // small caps and all caps, together
    fn_set_small_caps(f, true);
    fn_set_all_caps(f, true);

    D_FT_CHECK(f.small_caps);
    D_FT_CHECK(f.all_caps);

    // everything at once - accepted without complaint
    D_FT_CHECK(f.small_caps && f.all_caps && f.subscript && f.superscript);

    return true;
}

/*
tests_fn_set_letter_spacing
  Tests the following:
  - the tracking value lands on the font, in em units
  - fractional values survive
  - the value replaces rather than accumulates
*/
bool
tests_fn_set_letter_spacing()
{
    metrics_font f;

    D_FT_CHECK_NEAR(f.letter_spacing, 0.0f);

    fn_set_letter_spacing(f, 0.05f);

    D_FT_CHECK_NEAR(f.letter_spacing, 0.05f);

    // replaces, does not accumulate
    fn_set_letter_spacing(f, 0.125f);

    D_FT_CHECK_NEAR(f.letter_spacing, 0.125f);

    // and the line height beside it is untouched
    D_FT_CHECK_NEAR(f.line_height, 0.0f);

    return true;
}

/*
tests_fn_set_letter_spacing_accepts_negative
  Negative tracking is TIGHTENING, a real and common typographic request -
so the model must not clamp it to zero.
  Tests the following:
  - a negative letter spacing is stored verbatim
  - zero is a legal value (no tracking adjustment)
*/
bool
tests_fn_set_letter_spacing_accepts_negative()
{
    metrics_font f;

    fn_set_letter_spacing(f, -0.02f);

    D_FT_CHECK_NEAR(f.letter_spacing, -0.02f);
    D_FT_CHECK(f.letter_spacing < 0.0f);

    fn_set_letter_spacing(f, 0.0f);

    D_FT_CHECK_NEAR(f.letter_spacing, 0.0f);

    return true;
}

/*
tests_fn_set_line_height
  Tests the following:
  - the multiplier lands on the font
  - typical values (1.2, 1.5, 2.0) survive
*/
bool
tests_fn_set_line_height()
{
    metrics_font f;

    fn_set_line_height(f, 1.2f);

    D_FT_CHECK_NEAR(f.line_height, 1.2f);

    fn_set_line_height(f, 1.5f);

    D_FT_CHECK_NEAR(f.line_height, 1.5f);

    fn_set_line_height(f, 2.0f);

    D_FT_CHECK_NEAR(f.line_height, 2.0f);

    // the letter spacing beside it is untouched
    D_FT_CHECK_NEAR(f.letter_spacing, 0.0f);

    return true;
}

/*
tests_fn_line_height_zero_means_adapter_default
  Zero is a SENTINEL here, not a measurement: the header documents
"0 => adapter default".  So a font that has never had a line height set is
indistinguishable from one explicitly set to 0 - and that is intended, since
both mean "use the face's own metric".  A caller who wants zero leading has
no way to say so through this field.
  Tests the following:
  - the default line_height is 0.0f
  - setting it to 0.0f explicitly returns to the sentinel
  - a nonzero value is distinguishable from it
*/
bool
tests_fn_line_height_zero_means_adapter_default()
{
    metrics_font fresh;

    D_FT_CHECK_NEAR(fresh.line_height, 0.0f);

    metrics_font f;

    fn_set_line_height(f, 1.4f);

    D_FT_CHECK(f.line_height > 0.0f);

    // back to the sentinel - indistinguishable from never-set
    fn_set_line_height(f, 0.0f);

    D_FT_CHECK_NEAR(f.line_height, 0.0f);
    D_FT_CHECK_NEAR(f.line_height, fresh.line_height);

    return true;
}

/*
tests_fn_set_stretch
  Tests the following:
  - every one of the nine width classes can be set
  - the default (normal) is restorable
*/
bool
tests_fn_set_stretch()
{
    const font_stretch classes[] =
    {
        font_stretch::ultra_condensed, font_stretch::extra_condensed,
        font_stretch::condensed,       font_stretch::semi_condensed,
        font_stretch::normal,          font_stretch::semi_expanded,
        font_stretch::expanded,        font_stretch::extra_expanded,
        font_stretch::ultra_expanded
    };

    axes_font f;

    D_FT_CHECK(f.stretch == font_stretch::normal);

    // every width class round-trips
    for (std::size_t i = 0; i < 9u; ++i)
    {
        fn_set_stretch(f, classes[i]);

        D_FT_CHECK(f.stretch == classes[i]);
    }

    fn_set_stretch(f, font_stretch::normal);

    D_FT_CHECK(f.stretch == font_stretch::normal);

    // the spacing axis beside it never moved
    D_FT_CHECK(f.spacing == font_spacing::any);

    return true;
}

/*
tests_fn_set_spacing
  Tests the following:
  - every one of the five spacing modes can be set
  - the default (any) is restorable
*/
bool
tests_fn_set_spacing()
{
    const font_spacing modes[] =
    {
        font_spacing::any,        font_spacing::proportional,
        font_spacing::monospace,  font_spacing::dual_width,
        font_spacing::charcell
    };

    axes_font f;

    D_FT_CHECK(f.spacing == font_spacing::any);

    // every mode round-trips
    for (std::size_t i = 0; i < 5u; ++i)
    {
        fn_set_spacing(f, modes[i]);

        D_FT_CHECK(f.spacing == modes[i]);
    }

    fn_set_spacing(f, font_spacing::any);

    D_FT_CHECK(f.spacing == font_spacing::any);

    // the stretch axis beside it never moved
    D_FT_CHECK(f.stretch == font_stretch::normal);

    return true;
}

/*
tests_fn_set_foreground
  Tests the following:
  - the foreground color lands on the font
  - it replaces the previous value
  - setting it does NOT enable a background (they are separate mixins)
*/
bool
tests_fn_set_foreground()
{
    font<ff_color | ff_background, probe_color> f;

    const probe_color fresh {};

    D_FT_CHECK(f.foreground == fresh);

    const probe_color red { 255u, 0u, 0u };

    fn_set_foreground(f, red);

    D_FT_CHECK(f.foreground == red);

    const probe_color blue { 0u, 0u, 255u };

    fn_set_foreground(f, blue);

    D_FT_CHECK(f.foreground == blue);
    D_FT_CHECK(!(f.foreground == red));

    // the background is a different mixin and stays inert
    D_FT_CHECK(f.background == fresh);
    D_FT_CHECK(!f.background_enabled);

    return true;
}

/*
tests_fn_set_foreground_with_a_custom_color_type
  The color mixins are templated on _ColorType, so the setter must accept
whatever type the font was parameterised with - including the legacy
font_color and a type font.hpp has never seen.
  Tests the following:
  - a wide_color font accepts a wide_color foreground
  - a font_color font accepts a font_color foreground
  - each stores the exact value handed to it
*/
bool
tests_fn_set_foreground_with_a_custom_color_type()
{
    font<ff_color, wide_color> wide;

    wide_color w {};
    w.channel[0] = 0.25;
    w.channel[7] = 0.75;

    fn_set_foreground(wide, w);

    D_FT_CHECK(wide.foreground == w);
    D_FT_CHECK(wide.foreground.channel[0] == 0.25);
    D_FT_CHECK(wide.foreground.channel[7] == 0.75);

    // the legacy color type, still supported
    font<ff_color, font_color> legacy;

    const font_color amber { 255u, 191u, 0u, 255u };

    fn_set_foreground(legacy, amber);

    D_FT_CHECK(legacy.foreground == amber);
    D_FT_CHECK(legacy.foreground.a == 255u);

    return true;
}

/*
tests_fn_set_background_enables_it
  fn_set_background does TWO things: it stores the color AND raises
background_enabled.  That pairing is the whole reason the mixin carries a
flag beside the color, and the reason has_font_background is a conjunction.
  Tests the following:
  - the background color lands on the font
  - background_enabled is raised as a side effect - the caller does not have
    to set it by hand
  - the foreground is not touched
*/
bool
tests_fn_set_background_enables_it()
{
    font<ff_color | ff_background, probe_color> f;

    D_FT_CHECK(!f.background_enabled);

    const probe_color grey { 32u, 32u, 32u };

    fn_set_background(f, grey);

    D_FT_CHECK(f.background == grey);
    D_FT_CHECK(f.background_enabled);      // raised for us

    // the foreground stayed where it was
    D_FT_CHECK(f.foreground == probe_color {});

    // setting it again keeps it enabled
    const probe_color black { 0u, 0u, 0u };

    fn_set_background(f, black);

    D_FT_CHECK(f.background == black);
    D_FT_CHECK(f.background_enabled);

    return true;
}

/*
tests_fn_clear_background_preserves_the_color
  fn_clear_background lowers the FLAG and nothing else - the stored color
survives.  So "clear" means "stop painting it", not "forget it".  A caller
can therefore toggle a background off and back on without re-supplying the
color, but a caller who assumed clear() also reset the value would be wrong.
  Tests the following:
  - clearing lowers background_enabled
  - the background COLOR is still there afterwards
  - the foreground is untouched
*/
bool
tests_fn_clear_background_preserves_the_color()
{
    background_font f;

    const probe_color navy { 0u, 0u, 128u };

    fn_set_background(f, navy);

    D_FT_CHECK(f.background_enabled);
    D_FT_CHECK(f.background == navy);

    fn_clear_background(f);

    D_FT_CHECK(!f.background_enabled);     // the flag is down
    D_FT_CHECK(f.background == navy);      // but the color remains

    // it is NOT reset to a fresh color
    D_FT_CHECK(!(f.background == probe_color {}));

    return true;
}

/*
tests_fn_clear_background_then_set_again
  Tests the following:
  - clear then set re-enables with the NEW color
  - clear is idempotent
*/
bool
tests_fn_clear_background_then_set_again()
{
    background_font f;

    const probe_color navy  { 0u, 0u, 128u };
    const probe_color olive { 128u, 128u, 0u };

    fn_set_background(f, navy);
    fn_clear_background(f);

    D_FT_CHECK(!f.background_enabled);

    // idempotent
    fn_clear_background(f);

    D_FT_CHECK(!f.background_enabled);
    D_FT_CHECK(f.background == navy);      // still preserved

    fn_set_background(f, olive);

    D_FT_CHECK(f.background_enabled);
    D_FT_CHECK(f.background == olive);

    return true;
}

/*
tests_fn_clear_background_on_a_never_set_background
  Tests the following:
  - clearing a background that was never set is a harmless no-op
  - the flag stays down and the color stays value-initialised
*/
bool
tests_fn_clear_background_on_a_never_set_background()
{
    background_font f;

    D_FT_CHECK(!f.background_enabled);

    fn_clear_background(f);

    D_FT_CHECK(!f.background_enabled);
    D_FT_CHECK(f.background == probe_color {});

    return true;
}

/*
tests_fn_foreground_and_background_are_independent
  Two mixins, two colors - and both parameterised on the SAME _ColorType, so
a setter that wrote the wrong member would still typecheck.  That is exactly
the bug this test exists to catch.
  Tests the following:
  - setting the foreground leaves the background and its flag alone
  - setting the background leaves the foreground alone
  - the two hold DIFFERENT values simultaneously
*/
bool
tests_fn_foreground_and_background_are_independent()
{
    font<ff_color | ff_background, probe_color> f;

    const probe_color white { 255u, 255u, 255u };
    const probe_color black { 0u,   0u,   0u   };

    fn_set_foreground(f, white);

    D_FT_CHECK(f.foreground == white);
    D_FT_CHECK(f.background == probe_color {});
    D_FT_CHECK(!f.background_enabled);        // NOT raised by the foreground

    fn_set_background(f, black);

    D_FT_CHECK(f.foreground == white);        // still white
    D_FT_CHECK(f.background == black);
    D_FT_CHECK(f.background_enabled);

    // they really are distinct storage
    D_FT_CHECK(!(f.foreground == f.background));

    // clearing the background does not touch the foreground
    fn_clear_background(f);

    D_FT_CHECK(f.foreground == white);
    D_FT_CHECK(!f.background_enabled);

    return true;
}

/*
tests_fn_color_ops_leave_the_core_alone
  Tests the following:
  - the color setters do not disturb family, style, size, unit, weight,
    numeric weight, or slant
*/
bool
tests_fn_color_ops_leave_the_core_alone()
{
    font<ff_color | ff_background, probe_color> f("Inter", 12.0f,
                                                  font_weight::semi_bold,
                                                  font_slant::italic);

    fn_set_style_name(f, "Semibold Italic");
    fn_set_weight_numeric(f, 640u);
    fn_set_size_unit(f, font_size_unit::pixels);

    fn_set_foreground(f, probe_color { 1u, 2u, 3u });
    fn_set_background(f, probe_color { 4u, 5u, 6u });
    fn_clear_background(f);

    D_FT_CHECK(f.family         == "Inter");
    D_FT_CHECK(f.style_name     == "Semibold Italic");
    D_FT_CHECK_NEAR(f.size, 12.0f);
    D_FT_CHECK(f.size_unit      == font_size_unit::pixels);
    D_FT_CHECK(f.weight         == font_weight::semi_bold);
    D_FT_CHECK(f.weight_numeric == 640u);
    D_FT_CHECK(f.slant          == font_slant::italic);

    D_FT_CHECK(fn_effective_weight(f) == 640u);
    D_FT_CHECK(fn_is_bold(f));
    D_FT_CHECK(fn_is_italic(f));

    return true;
}

NS_END  // testing
NS_END  // djinterp
