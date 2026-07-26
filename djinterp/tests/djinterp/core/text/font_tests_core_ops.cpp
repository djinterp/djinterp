// djinterp [test]  font_tests_core_ops.cpp
//   Section 9a: the free functions over the always-present core - family,
// style name, size, size unit, the two-track weight (symbolic + numeric),
// slant, and the bold / italic conveniences layered on top of them.

// djinterp
#include "font_tests.hpp"


NS_DJINTERP
NS_TESTING

/*
tests_fn_set_family
  Tests the following:
  - the family lands on the font
  - an lvalue source is copied, not stolen
  - the call touches nothing else
*/
bool
tests_fn_set_family()
{
    bare_font f;

    fn_set_family(f, "Inter");

    D_FT_CHECK(f.family == "Inter");
    D_FT_CHECK(!f.empty());

    // an lvalue string survives the call
    const std::string name = "Iosevka";

    fn_set_family(f, name);

    D_FT_CHECK(f.family == "Iosevka");
    D_FT_CHECK(name     == "Iosevka");

    // overwriting replaces rather than appends
    fn_set_family(f, "Fira Code");

    D_FT_CHECK(f.family == "Fira Code");

    // nothing else moved
    D_FT_CHECK(f.style_name.empty());
    D_FT_CHECK_NEAR(f.size, 10.0f);
    D_FT_CHECK(f.weight == font_weight::normal);
    D_FT_CHECK(f.slant  == font_slant::upright);

    return true;
}

/*
tests_fn_set_style_name
  style_name is free text ("Bold Italic"), independent of the weight and
slant enums - a face can be named "Semibold" while carrying weight 600, and
the two never talk to each other.
  Tests the following:
  - the style name lands on the font
  - it does NOT alter the weight or the slant
  - it does not affect empty()
*/
bool
tests_fn_set_style_name()
{
    bare_font f("Inter");

    fn_set_style_name(f, "Bold Italic");

    D_FT_CHECK(f.style_name == "Bold Italic");

    // the enums are untouched: the string is a label, not a command
    D_FT_CHECK(f.weight == font_weight::normal);
    D_FT_CHECK(f.slant  == font_slant::upright);
    D_FT_CHECK(!fn_is_bold(f));
    D_FT_CHECK(!fn_is_italic(f));

    // and it has no say in empty()
    bare_font g;

    fn_set_style_name(g, "Regular");

    D_FT_CHECK(g.empty());

    return true;
}

/*
tests_fn_set_size
  Tests the following:
  - the size lands on the font
  - the size UNIT is left alone (setting a size does not reinterpret it)
*/
bool
tests_fn_set_size()
{
    bare_font f;

    fn_set_size(f, 14.0f);

    D_FT_CHECK_NEAR(f.size, 14.0f);
    D_FT_CHECK(f.size_unit == font_size_unit::points);

    fn_set_size_unit(f, font_size_unit::pixels);
    fn_set_size(f, 24.0f);

    D_FT_CHECK_NEAR(f.size, 24.0f);
    D_FT_CHECK(f.size_unit == font_size_unit::pixels);   // still pixels

    // fractional sizes survive
    fn_set_size(f, 11.5f);

    D_FT_CHECK_NEAR(f.size, 11.5f);

    return true;
}

/*
tests_fn_set_size_does_not_validate
  font.hpp is a data model, not a policy layer: it stores what it is given.
Zero and negative sizes are accepted, and it is the BACKEND's job to reject
or clamp them.  Recording that here means a future validation pass is a
deliberate change, not a silent one.
  Tests the following:
  - a size of 0.0f is stored verbatim
  - a negative size is stored verbatim
  - a very large size is stored verbatim
*/
bool
tests_fn_set_size_does_not_validate()
{
    bare_font f;

    fn_set_size(f, 0.0f);

    D_FT_CHECK_NEAR(f.size, 0.0f);

    fn_set_size(f, -12.0f);

    D_FT_CHECK_NEAR(f.size, -12.0f);
    D_FT_CHECK(f.size < 0.0f);

    fn_set_size(f, 4096.0f);

    D_FT_CHECK_NEAR(f.size, 4096.0f);

    return true;
}

/*
tests_fn_set_size_unit
  Tests the following:
  - each of the five units can be set
  - setting the unit does NOT convert the numeric size (that is
    fn_convert_size's job, and it is a separate, explicit call)
*/
bool
tests_fn_set_size_unit()
{
    bare_font f;

    fn_set_size(f, 12.0f);

    fn_set_size_unit(f, font_size_unit::pixels);
    D_FT_CHECK(f.size_unit == font_size_unit::pixels);
    D_FT_CHECK_NEAR(f.size, 12.0f);      // the NUMBER did not move

    fn_set_size_unit(f, font_size_unit::em);
    D_FT_CHECK(f.size_unit == font_size_unit::em);
    D_FT_CHECK_NEAR(f.size, 12.0f);

    fn_set_size_unit(f, font_size_unit::percent);
    D_FT_CHECK(f.size_unit == font_size_unit::percent);

    fn_set_size_unit(f, font_size_unit::device_units);
    D_FT_CHECK(f.size_unit == font_size_unit::device_units);

    fn_set_size_unit(f, font_size_unit::points);
    D_FT_CHECK(f.size_unit == font_size_unit::points);
    D_FT_CHECK_NEAR(f.size, 12.0f);

    return true;
}

/*
tests_fn_set_weight
  Tests the following:
  - each symbolic weight lands on the font
  - fn_effective_weight then reports that weight's number
*/
bool
tests_fn_set_weight()
{
    bare_font f;

    fn_set_weight(f, font_weight::thin);
    D_FT_CHECK(f.weight == font_weight::thin);
    D_FT_CHECK(fn_effective_weight(f) == 100u);

    fn_set_weight(f, font_weight::semi_bold);
    D_FT_CHECK(f.weight == font_weight::semi_bold);
    D_FT_CHECK(fn_effective_weight(f) == 600u);

    fn_set_weight(f, font_weight::extra_black);
    D_FT_CHECK(f.weight == font_weight::extra_black);
    D_FT_CHECK(fn_effective_weight(f) == 950u);

    fn_set_weight(f, font_weight::normal);
    D_FT_CHECK(f.weight == font_weight::normal);
    D_FT_CHECK(fn_effective_weight(f) == 400u);

    return true;
}

/*
tests_fn_set_weight_clears_the_numeric_override
  The load-bearing side effect.  weight_numeric WINS over the symbolic
weight, so a fn_set_weight that left a stale numeric behind would appear to
do nothing at all.  It therefore zeroes the override on the way through.
  Tests the following:
  - a numeric override in place is cleared by fn_set_weight
  - the new symbolic weight is what fn_effective_weight then reports
  - the sequence numeric(850) -> set_weight(light) really does end up light,
    not 850
*/
bool
tests_fn_set_weight_clears_the_numeric_override()
{
    bare_font f;

    fn_set_weight_numeric(f, 850u);

    D_FT_CHECK(f.weight_numeric == 850u);
    D_FT_CHECK(fn_effective_weight(f) == 850u);

    // the symbolic setter must sweep the override away
    fn_set_weight(f, font_weight::light);

    D_FT_CHECK(f.weight         == font_weight::light);
    D_FT_CHECK(f.weight_numeric == 0u);
    D_FT_CHECK(fn_effective_weight(f) == 300u);

    // the override does not come back
    D_FT_CHECK(fn_effective_weight(f) != 850u);

    return true;
}

/*
tests_fn_set_weight_numeric
  Tests the following:
  - the numeric override lands on the font
  - it then WINS over the symbolic weight
  - the symbolic weight is left in place underneath it (not overwritten)
  - off-scale values (e.g. 123, 1000) are stored verbatim - variable fonts
    have continuous weight axes, so the model does not snap to the ladder
*/
bool
tests_fn_set_weight_numeric()
{
    bare_font f;

    fn_set_weight(f, font_weight::normal);
    fn_set_weight_numeric(f, 725u);

    D_FT_CHECK(f.weight_numeric == 725u);
    D_FT_CHECK(f.weight         == font_weight::normal);   // still there
    D_FT_CHECK(fn_effective_weight(f) == 725u);            // but overridden

    // a value that is not on the symbolic ladder at all
    fn_set_weight_numeric(f, 123u);

    D_FT_CHECK(fn_effective_weight(f) == 123u);

    fn_set_weight_numeric(f, 1000u);

    D_FT_CHECK(fn_effective_weight(f) == 1000u);

    return true;
}

/*
tests_fn_set_weight_numeric_zero_restores_the_symbolic_weight
  Zero is not a weight - it is the sentinel for "no override".  So passing 0
does not make the font weightless; it hands control back to the enum that
was sitting underneath all along.
  Tests the following:
  - fn_set_weight_numeric(f, 0) clears the override
  - the symbolic weight, untouched throughout, takes effect again
  - the effective weight is never 0
*/
bool
tests_fn_set_weight_numeric_zero_restores_the_symbolic_weight()
{
    bare_font f;

    fn_set_weight(f, font_weight::black);      // 900, symbolically
    fn_set_weight_numeric(f, 250u);            // overridden to 250

    D_FT_CHECK(fn_effective_weight(f) == 250u);

    // hand control back
    fn_set_weight_numeric(f, 0u);

    D_FT_CHECK(f.weight_numeric == 0u);
    D_FT_CHECK(f.weight         == font_weight::black);
    D_FT_CHECK(fn_effective_weight(f) == 900u);

    // the effective weight is never the sentinel itself
    D_FT_CHECK(fn_effective_weight(f) != 0u);

    return true;
}

/*
tests_fn_effective_weight_prefers_the_numeric
  Tests the following:
  - with a nonzero override, the numeric wins whatever the enum says
  - it wins in BOTH directions: a heavy enum with a light override reports
    light, and vice versa
*/
bool
tests_fn_effective_weight_prefers_the_numeric()
{
    bare_font heavy_enum_light_numeric;

    heavy_enum_light_numeric.weight         = font_weight::black;   // 900
    heavy_enum_light_numeric.weight_numeric = 100u;                 // thin

    D_FT_CHECK(fn_effective_weight(heavy_enum_light_numeric) == 100u);
    D_FT_CHECK(!fn_is_bold(heavy_enum_light_numeric));   // the numeric rules

    bare_font light_enum_heavy_numeric;

    light_enum_heavy_numeric.weight         = font_weight::thin;    // 100
    light_enum_heavy_numeric.weight_numeric = 900u;                 // black

    D_FT_CHECK(fn_effective_weight(light_enum_heavy_numeric) == 900u);
    D_FT_CHECK(fn_is_bold(light_enum_heavy_numeric));

    return true;
}

/*
tests_fn_effective_weight_falls_back_to_the_enum
  Tests the following:
  - with the override at 0, the enum's numeric value is returned
  - this holds for every rung of the symbolic ladder
*/
bool
tests_fn_effective_weight_falls_back_to_the_enum()
{
    const font_weight ladder[] =
    {
        font_weight::thin,        font_weight::extra_light,
        font_weight::light,       font_weight::normal,
        font_weight::medium,      font_weight::semi_bold,
        font_weight::bold,        font_weight::extra_bold,
        font_weight::black,       font_weight::extra_black
    };

    // every rung, with no override in play
    for (std::size_t i = 0; i < 10u; ++i)
    {
        bare_font f;

        fn_set_weight(f, ladder[i]);

        D_FT_CHECK(f.weight_numeric == 0u);
        D_FT_CHECK(fn_effective_weight(f) == weight_of(ladder[i]));
    }

    return true;
}

/*
tests_fn_effective_weight_honours_any_nonzero_numeric
  The test is `!= 0`, not `>= 100`.  So an override of 1 - absurd as a
weight - still wins.  Boundary-checking the sentinel matters: an
implementation that guarded with `> 100` would break this and nothing else.
  Tests the following:
  - an override of 1 is honoured
  - an override of 65535 (the uint16 ceiling) is honoured
  - only exactly 0 falls back
*/
bool
tests_fn_effective_weight_honours_any_nonzero_numeric()
{
    bare_font f;

    fn_set_weight(f, font_weight::normal);

    fn_set_weight_numeric(f, 1u);

    D_FT_CHECK(fn_effective_weight(f) == 1u);
    D_FT_CHECK(fn_effective_weight(f) != 400u);

    fn_set_weight_numeric(f, 65535u);

    D_FT_CHECK(fn_effective_weight(f) == 65535u);

    // and exactly zero, and only zero, falls back
    fn_set_weight_numeric(f, 0u);

    D_FT_CHECK(fn_effective_weight(f) == 400u);

    return true;
}

/*
tests_fn_effective_weight_is_noexcept
  Tests the following:
  - fn_effective_weight is noexcept (a renderer calls it on the hot path)
  - it returns std::uint16_t
  - it takes the font by const reference
*/
bool
tests_fn_effective_weight_is_noexcept()
{
    const bare_font f;

    D_FT_CHECK(noexcept(fn_effective_weight(f)));
    D_FT_CHECK((std::is_same<decltype(fn_effective_weight(f)),
                             std::uint16_t>::value));

    // fn_is_bold rides on it and is noexcept too
    D_FT_CHECK(noexcept(fn_is_bold(f)));
    D_FT_CHECK(noexcept(fn_is_italic(f)));

    return true;
}

/*
tests_fn_set_slant
  Tests the following:
  - each of the three slants can be set
  - oblique is preserved as oblique, never folded into italic
*/
bool
tests_fn_set_slant()
{
    bare_font f;

    fn_set_slant(f, font_slant::italic);
    D_FT_CHECK(f.slant == font_slant::italic);

    fn_set_slant(f, font_slant::oblique);
    D_FT_CHECK(f.slant == font_slant::oblique);
    D_FT_CHECK(f.slant != font_slant::italic);   // NOT collapsed

    fn_set_slant(f, font_slant::upright);
    D_FT_CHECK(f.slant == font_slant::upright);

    return true;
}

/*
tests_fn_set_bold_true
  Tests the following:
  - fn_set_bold(f, true) sets the weight to bold (700)
  - fn_is_bold then agrees
*/
bool
tests_fn_set_bold_true()
{
    bare_font f;

    D_FT_CHECK(!fn_is_bold(f));

    fn_set_bold(f, true);

    D_FT_CHECK(f.weight == font_weight::bold);
    D_FT_CHECK(fn_effective_weight(f) == 700u);
    D_FT_CHECK(fn_is_bold(f));

    // idempotent
    fn_set_bold(f, true);

    D_FT_CHECK(f.weight == font_weight::bold);

    return true;
}

/*
tests_fn_set_bold_false_forces_normal
  fn_set_bold(f, false) is NOT "un-embolden" - it assigns normal (400)
outright.  So a black font asked to stop being bold becomes NORMAL, losing
its weight entirely.  A caller wanting to preserve a non-bold weight must
use fn_set_weight, not fn_set_bold(false).
  Tests the following:
  - a bold font goes to normal
  - a BLACK font (900) also goes to normal, not back to black
  - a THIN font (100), which was never bold, is also dragged to normal
*/
bool
tests_fn_set_bold_false_forces_normal()
{
    bare_font f;

    fn_set_bold(f, true);
    fn_set_bold(f, false);

    D_FT_CHECK(f.weight == font_weight::normal);
    D_FT_CHECK(!fn_is_bold(f));

    // the sharp case: black is not "extra bold", it is destroyed
    bare_font black;

    fn_set_weight(black, font_weight::black);
    D_FT_CHECK(fn_is_bold(black));

    fn_set_bold(black, false);

    D_FT_CHECK(black.weight == font_weight::normal);
    D_FT_CHECK(black.weight != font_weight::black);
    D_FT_CHECK(fn_effective_weight(black) == 400u);

    // and a weight BELOW normal is raised to it
    bare_font thin;

    fn_set_weight(thin, font_weight::thin);
    D_FT_CHECK(!fn_is_bold(thin));

    fn_set_bold(thin, false);

    D_FT_CHECK(thin.weight == font_weight::normal);   // not thin any more
    D_FT_CHECK(fn_effective_weight(thin) == 400u);

    return true;
}

/*
tests_fn_set_bold_clears_the_numeric_override
  Same trap as fn_set_weight: with a numeric override in place, a
fn_set_bold that did not clear it would silently do nothing.
  Tests the following:
  - fn_set_bold(true) zeroes weight_numeric and the font really is bold
  - fn_set_bold(false) zeroes it too
*/
bool
tests_fn_set_bold_clears_the_numeric_override()
{
    bare_font f;

    fn_set_weight_numeric(f, 250u);

    D_FT_CHECK(fn_effective_weight(f) == 250u);
    D_FT_CHECK(!fn_is_bold(f));

    fn_set_bold(f, true);

    D_FT_CHECK(f.weight_numeric == 0u);
    D_FT_CHECK(fn_effective_weight(f) == 700u);
    D_FT_CHECK(fn_is_bold(f));                 // it actually took effect

    // and the other direction
    fn_set_weight_numeric(f, 900u);

    D_FT_CHECK(fn_is_bold(f));

    fn_set_bold(f, false);

    D_FT_CHECK(f.weight_numeric == 0u);
    D_FT_CHECK(fn_effective_weight(f) == 400u);
    D_FT_CHECK(!fn_is_bold(f));

    return true;
}

/*
tests_fn_is_bold_threshold_is_semi_bold
  The predicate is `effective >= 600`, so SEMI-BOLD counts as bold.  That is
the CSS convention, and it is a boundary, so it gets tested exactly at the
edge rather than near it.
  Tests the following:
  - semi_bold (600) IS bold - the boundary is inclusive
  - bold (700), extra_bold (800), black (900), extra_black (950) are bold
  - medium (500) is NOT
*/
bool
tests_fn_is_bold_threshold_is_semi_bold()
{
    bare_font f;

    fn_set_weight(f, font_weight::medium);         // 500
    D_FT_CHECK(!fn_is_bold(f));

    fn_set_weight(f, font_weight::semi_bold);      // 600 - the boundary
    D_FT_CHECK(fn_is_bold(f));

    fn_set_weight(f, font_weight::bold);           // 700
    D_FT_CHECK(fn_is_bold(f));

    fn_set_weight(f, font_weight::extra_bold);     // 800
    D_FT_CHECK(fn_is_bold(f));

    fn_set_weight(f, font_weight::black);          // 900
    D_FT_CHECK(fn_is_bold(f));

    fn_set_weight(f, font_weight::extra_black);    // 950
    D_FT_CHECK(fn_is_bold(f));

    // the threshold really is semi_bold's number
    D_FT_CHECK(weight_of(font_weight::semi_bold) == 600u);

    return true;
}

/*
tests_fn_is_bold_below_the_threshold
  Tests the following:
  - every rung below semi_bold reports false
  - a default-constructed font (normal, 400) is not bold
*/
bool
tests_fn_is_bold_below_the_threshold()
{
    const font_weight light_rungs[] =
    {
        font_weight::thin,        font_weight::extra_light,
        font_weight::light,       font_weight::normal,
        font_weight::medium
    };

    // none of the sub-600 rungs is bold
    for (std::size_t i = 0; i < 5u; ++i)
    {
        bare_font f;

        fn_set_weight(f, light_rungs[i]);

        D_FT_CHECK(!fn_is_bold(f));
        D_FT_CHECK(fn_effective_weight(f) < 600u);
    }

    const bare_font fresh;

    D_FT_CHECK(!fn_is_bold(fresh));

    return true;
}

/*
tests_fn_is_bold_reads_the_effective_weight
  fn_is_bold delegates to fn_effective_weight, so the numeric override
governs it too - a font whose ENUM says black can report "not bold" if its
override says otherwise.
  Tests the following:
  - a light enum with a heavy override is bold
  - a heavy enum with a light override is not
*/
bool
tests_fn_is_bold_reads_the_effective_weight()
{
    bare_font f;

    fn_set_weight(f, font_weight::thin);
    fn_set_weight_numeric(f, 800u);

    D_FT_CHECK(f.weight == font_weight::thin);   // the enum still says thin
    D_FT_CHECK(fn_is_bold(f));                   // but the override rules

    bare_font g;

    fn_set_weight(g, font_weight::black);
    fn_set_weight_numeric(g, 300u);

    D_FT_CHECK(g.weight == font_weight::black);
    D_FT_CHECK(!fn_is_bold(g));

    return true;
}

/*
tests_fn_is_bold_numeric_boundary
  The same 600 boundary, walked through the numeric channel where a variable
font actually lives - 599 and 600 are one unit apart and land on opposite
sides.
  Tests the following:
  - a numeric override of 599 is not bold
  - a numeric override of 600 is
  - 601 is
*/
bool
tests_fn_is_bold_numeric_boundary()
{
    bare_font f;

    fn_set_weight_numeric(f, 599u);
    D_FT_CHECK(!fn_is_bold(f));

    fn_set_weight_numeric(f, 600u);
    D_FT_CHECK(fn_is_bold(f));

    fn_set_weight_numeric(f, 601u);
    D_FT_CHECK(fn_is_bold(f));

    // and at the extremes
    fn_set_weight_numeric(f, 1u);
    D_FT_CHECK(!fn_is_bold(f));

    fn_set_weight_numeric(f, 65535u);
    D_FT_CHECK(fn_is_bold(f));

    return true;
}

/*
tests_fn_set_italic
  Tests the following:
  - fn_set_italic(f, true) sets the slant to italic
  - fn_set_italic(f, false) sets it to upright
  - fn_is_italic agrees in both directions
*/
bool
tests_fn_set_italic()
{
    bare_font f;

    D_FT_CHECK(!fn_is_italic(f));

    fn_set_italic(f, true);

    D_FT_CHECK(f.slant == font_slant::italic);
    D_FT_CHECK(fn_is_italic(f));

    fn_set_italic(f, false);

    D_FT_CHECK(f.slant == font_slant::upright);
    D_FT_CHECK(!fn_is_italic(f));

    return true;
}

/*
tests_fn_set_italic_false_from_oblique
  fn_set_italic(false) assigns upright outright, so it clears an OBLIQUE
slant as well - even though oblique is not italic.  The setter is a two-state
switch laid over a three-state axis, and this is where that shows.
  Tests the following:
  - an oblique font asked to stop being italic becomes upright
  - it does not stay oblique
*/
bool
tests_fn_set_italic_false_from_oblique()
{
    bare_font f;

    fn_set_slant(f, font_slant::oblique);

    D_FT_CHECK(f.slant == font_slant::oblique);
    D_FT_CHECK(fn_is_italic(f));          // oblique reads as italic-like

    fn_set_italic(f, false);

    D_FT_CHECK(f.slant == font_slant::upright);
    D_FT_CHECK(f.slant != font_slant::oblique);
    D_FT_CHECK(!fn_is_italic(f));

    return true;
}

/*
tests_fn_set_italic_true_overwrites_oblique
  The other half of the same asymmetry: an oblique font asked to BE italic
becomes genuinely italic, losing the distinction.  A caller who cares about
the difference must use fn_set_slant.
  Tests the following:
  - fn_set_italic(true) on an oblique font yields italic, not oblique
  - fn_set_slant preserves the distinction where fn_set_italic cannot
*/
bool
tests_fn_set_italic_true_overwrites_oblique()
{
    bare_font f;

    fn_set_slant(f, font_slant::oblique);
    fn_set_italic(f, true);

    D_FT_CHECK(f.slant == font_slant::italic);
    D_FT_CHECK(f.slant != font_slant::oblique);   // the distinction is gone

    // the setter that preserves it
    bare_font g;

    fn_set_slant(g, font_slant::oblique);
    fn_set_slant(g, font_slant::oblique);

    D_FT_CHECK(g.slant == font_slant::oblique);

    return true;
}

/*
tests_fn_is_italic_covers_italic_and_oblique
  The predicate is deliberately generous: an adapter that cannot tell the
two apart should slant either one.
  Tests the following:
  - italic reports true
  - oblique reports true
  - upright reports false
  - the three cases are exhaustive over font_slant
*/
bool
tests_fn_is_italic_covers_italic_and_oblique()
{
    bare_font f;

    fn_set_slant(f, font_slant::upright);
    D_FT_CHECK(!fn_is_italic(f));

    fn_set_slant(f, font_slant::italic);
    D_FT_CHECK(fn_is_italic(f));

    fn_set_slant(f, font_slant::oblique);
    D_FT_CHECK(fn_is_italic(f));

    // exhaustive: the enum has exactly these three
    D_FT_CHECK(static_cast<std::uint8_t>(font_slant::oblique) == 2u);

    return true;
}

/*
tests_fn_bold_italic_round_trip
  Tests the following:
  - bold and italic are orthogonal: setting one never disturbs the other
  - all four combinations are reachable and reportable
*/
bool
tests_fn_bold_italic_round_trip()
{
    bare_font f("Inter", 12.0f);

    // all four corners
    fn_set_bold(f, false);
    fn_set_italic(f, false);
    D_FT_CHECK(!fn_is_bold(f));
    D_FT_CHECK(!fn_is_italic(f));

    fn_set_bold(f, true);
    D_FT_CHECK(fn_is_bold(f));
    D_FT_CHECK(!fn_is_italic(f));      // italic untouched

    fn_set_italic(f, true);
    D_FT_CHECK(fn_is_bold(f));         // bold untouched
    D_FT_CHECK(fn_is_italic(f));

    fn_set_bold(f, false);
    D_FT_CHECK(!fn_is_bold(f));
    D_FT_CHECK(fn_is_italic(f));       // still italic

    fn_set_italic(f, false);
    D_FT_CHECK(!fn_is_bold(f));
    D_FT_CHECK(!fn_is_italic(f));

    // and the family / size rode through all of it untouched
    D_FT_CHECK(f.family == "Inter");
    D_FT_CHECK_NEAR(f.size, 12.0f);

    return true;
}

/*
tests_fn_core_ops_are_generic_over_the_feature_set
  The core setters are templated on <unsigned _F, typename _C> and take no
static_assert guard - they must work on ANY font, from the bare one to the
fully-loaded one, and on any color type.
  Tests the following:
  - the whole core setter family runs on font<ff_none>
  - the same calls run on font<ff_all> and on a wide-color font
  - the results agree across all three
*/
bool
tests_fn_core_ops_are_generic_over_the_feature_set()
{
    bare_font                        bare;
    full_font                        full;
    font<ff_all, wide_color>         wide;

    fn_set_family(bare, "Inter");
    fn_set_family(full, "Inter");
    fn_set_family(wide, "Inter");

    fn_set_size(bare, 13.0f);
    fn_set_size(full, 13.0f);
    fn_set_size(wide, 13.0f);

    fn_set_size_unit(bare, font_size_unit::pixels);
    fn_set_size_unit(full, font_size_unit::pixels);
    fn_set_size_unit(wide, font_size_unit::pixels);

    fn_set_style_name(bare, "Semibold");
    fn_set_style_name(full, "Semibold");
    fn_set_style_name(wide, "Semibold");

    fn_set_weight(bare, font_weight::semi_bold);
    fn_set_weight(full, font_weight::semi_bold);
    fn_set_weight(wide, font_weight::semi_bold);

    fn_set_italic(bare, true);
    fn_set_italic(full, true);
    fn_set_italic(wide, true);

    // identical outcomes, whatever the feature set or color type
    D_FT_CHECK(bare.family == full.family);
    D_FT_CHECK(full.family == wide.family);
    D_FT_CHECK(bare.style_name == "Semibold");

    D_FT_CHECK(fn_effective_weight(bare) == fn_effective_weight(full));
    D_FT_CHECK(fn_effective_weight(full) == fn_effective_weight(wide));
    D_FT_CHECK(fn_effective_weight(bare) == 600u);

    D_FT_CHECK(fn_is_bold(bare)   == fn_is_bold(full));
    D_FT_CHECK(fn_is_italic(bare) == fn_is_italic(wide));
    D_FT_CHECK(fn_is_bold(full));
    D_FT_CHECK(fn_is_italic(wide));

    D_FT_CHECK(bare.size_unit == font_size_unit::pixels);
    D_FT_CHECK(!bare.empty());
    D_FT_CHECK(!full.empty());
    D_FT_CHECK(!wide.empty());

    return true;
}

NS_END  // testing
NS_END  // djinterp
