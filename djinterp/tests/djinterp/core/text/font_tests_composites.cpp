// djinterp [test]  font_tests_composites.cpp
//   Section 10, parts III and IV: the composite profile traits (is_font_like
// and the is_font_with_* / is_font_* gates built on it) and the enable_if_*
// SFINAE helpers.
//
//   is_font_like is the floor: family + size + weight + slant.  Every
// composite ANDs that floor with one capability trait, so each is really
// two claims - "font-like" and "has the capability" - and both must be
// tested.  The duck probes carry the capability WITHOUT the floor
// (duck_underline_only has an underline but no family), which is the only
// way to prove the AND is load-bearing rather than decorative.

// djinterp
#include "font_tests.hpp"


NS_DJINTERP
NS_TESTING

/*
tests_is_font_like_across_the_lattice
  Tests the following:
  - every font on the lattice is font-like, since the four core members are
    never gated
  - this holds for the bare font, every profile, and the full font
*/
bool
tests_is_font_like_across_the_lattice()
{
    D_FT_CHECK(is_font_like_v<bare_font>);
    D_FT_CHECK(is_font_like_v<underline_font_t>);
    D_FT_CHECK(is_font_like_v<decorated_font_t>);
    D_FT_CHECK(is_font_like_v<casing_font_t>);
    D_FT_CHECK(is_font_like_v<metrics_font>);
    D_FT_CHECK(is_font_like_v<axes_font>);
    D_FT_CHECK(is_font_like_v<color_font>);
    D_FT_CHECK(is_font_like_v<background_font>);
    D_FT_CHECK(is_font_like_v<opentype_font>);
    D_FT_CHECK(is_font_like_v<variable_font_t>);
    D_FT_CHECK(is_font_like_v<script_font>);
    D_FT_CHECK(is_font_like_v<backend_font>);
    D_FT_CHECK(is_font_like_v<terminal_font>);
    D_FT_CHECK(is_font_like_v<gui_basic_font>);
    D_FT_CHECK(is_font_like_v<gui_std_font>);
    D_FT_CHECK(is_font_like_v<gui_rich_font>);
    D_FT_CHECK(is_font_like_v<full_font>);

    // and with a different color type
    D_FT_CHECK(is_font_like_v<font<ff_none, wide_color>>);
    D_FT_CHECK(is_font_like_v<font<ff_all,  font_color>>);

    return true;
}

/*
tests_is_font_like_on_a_duck_typed_struct
  The point of the trait: a struct that shares NOTHING with font<> but the
four member names is still font-like.  This is what lets an adapter accept a
LOGFONT-shaped or fontconfig-shaped type.
  Tests the following:
  - duck_font (family + size + weight + slant, no inheritance) is font-like
  - it is NOT the same type as any font<>
*/
bool
tests_is_font_like_on_a_duck_typed_struct()
{
    D_FT_CHECK(is_font_like_v<duck_font>);

    // it really is unrelated to font<>
    D_FT_CHECK(!(std::is_base_of<bare_font, duck_font>::value));
    D_FT_CHECK(!(std::is_same<duck_font, bare_font>::value));

    // and the conveniences it lacks do not disqualify it
    D_FT_CHECK(!has_font_style_name_v<duck_font>);
    D_FT_CHECK(!has_font_size_unit_v<duck_font>);
    D_FT_CHECK(is_font_like_v<duck_font>);

    return true;
}

/*
tests_is_font_like_needs_the_family
  Tests the following:
  - a struct missing ONLY family fails is_font_like
  - the other three members are present, so the family is what tips it
*/
bool
tests_is_font_like_needs_the_family()
{
    D_FT_CHECK(!is_font_like_v<duck_no_family>);

    // the other three ARE there
    D_FT_CHECK(!has_font_family_v<duck_no_family>);
    D_FT_CHECK(has_font_size_v<duck_no_family>);
    D_FT_CHECK(has_font_weight_v<duck_no_family>);
    D_FT_CHECK(has_font_slant_v<duck_no_family>);

    return true;
}

/*
tests_is_font_like_needs_the_size
  Tests the following:
  - a struct missing ONLY size fails is_font_like
*/
bool
tests_is_font_like_needs_the_size()
{
    D_FT_CHECK(!is_font_like_v<duck_no_size>);

    D_FT_CHECK(has_font_family_v<duck_no_size>);
    D_FT_CHECK(!has_font_size_v<duck_no_size>);
    D_FT_CHECK(has_font_weight_v<duck_no_size>);
    D_FT_CHECK(has_font_slant_v<duck_no_size>);

    return true;
}

/*
tests_is_font_like_needs_the_weight
  Tests the following:
  - a struct missing ONLY weight fails is_font_like
*/
bool
tests_is_font_like_needs_the_weight()
{
    D_FT_CHECK(!is_font_like_v<duck_no_weight>);

    D_FT_CHECK(has_font_family_v<duck_no_weight>);
    D_FT_CHECK(has_font_size_v<duck_no_weight>);
    D_FT_CHECK(!has_font_weight_v<duck_no_weight>);
    D_FT_CHECK(has_font_slant_v<duck_no_weight>);

    return true;
}

/*
tests_is_font_like_needs_the_slant
  Tests the following:
  - a struct missing ONLY slant fails is_font_like
  - the four members are therefore each genuinely required (this test plus
    the three above cover every conjunct)
*/
bool
tests_is_font_like_needs_the_slant()
{
    D_FT_CHECK(!is_font_like_v<duck_no_slant>);

    D_FT_CHECK(has_font_family_v<duck_no_slant>);
    D_FT_CHECK(has_font_size_v<duck_no_slant>);
    D_FT_CHECK(has_font_weight_v<duck_no_slant>);
    D_FT_CHECK(!has_font_slant_v<duck_no_slant>);

    return true;
}

/*
tests_is_font_like_rejects_non_fonts
  Tests the following:
  - fundamental types, strings, containers, and unrelated structs are not
    font-like
  - a type carrying a decoration but none of the core (duck_underline_only)
    is not font-like
*/
bool
tests_is_font_like_rejects_non_fonts()
{
    D_FT_CHECK(!is_font_like_v<int>);
    D_FT_CHECK(!is_font_like_v<float>);
    D_FT_CHECK(!is_font_like_v<void*>);
    D_FT_CHECK(!is_font_like_v<std::string>);
    D_FT_CHECK(!is_font_like_v<std::vector<int>>);
    D_FT_CHECK(!is_font_like_v<duck_not_a_font>);
    D_FT_CHECK(!is_font_like_v<font_color>);
    D_FT_CHECK(!is_font_like_v<opentype_feature>);
    D_FT_CHECK(!is_font_like_v<variable_axis>);

    // has a capability, lacks the floor
    D_FT_CHECK(!is_font_like_v<duck_underline_only>);
    D_FT_CHECK(!is_font_like_v<duck_foreground_only>);
    D_FT_CHECK(!is_font_like_v<duck_background_paired>);

    return true;
}

/*
tests_is_font_bold_like_is_is_font_like
  is_font_bold_like ANDs is_font_like with has_font_weight - but weight is
already in the font-like floor, so the composite is EQUIVALENT to
is_font_like.  The header says as much ("Equivalent to is_font_like here").
The name exists for symmetry with is_font_italic_like and to document intent
at the call site.  Worth pinning so the equivalence is a stated fact.
  Tests the following:
  - is_font_bold_like matches is_font_like on every font and duck type tried
  - both accept duck_font and reject a non-font
*/
bool
tests_is_font_bold_like_is_is_font_like()
{
    D_FT_CHECK(is_font_bold_like_v<bare_font> == is_font_like_v<bare_font>);
    D_FT_CHECK(is_font_bold_like_v<full_font> == is_font_like_v<full_font>);
    D_FT_CHECK(is_font_bold_like_v<duck_font> == is_font_like_v<duck_font>);
    D_FT_CHECK(is_font_bold_like_v<duck_no_weight> ==
               is_font_like_v<duck_no_weight>);

    // spelled out both directions
    D_FT_CHECK(is_font_bold_like_v<bare_font>);
    D_FT_CHECK(is_font_bold_like_v<duck_font>);
    D_FT_CHECK(!is_font_bold_like_v<duck_no_weight>);
    D_FT_CHECK(!is_font_bold_like_v<int>);

    return true;
}

/*
tests_is_font_italic_like_is_is_font_like
  Same story: slant is in the floor, so is_font_italic_like is also
equivalent to is_font_like.  (A struct missing slant fails BOTH the floor
and the extra conjunct, so the two can never disagree.)
  Tests the following:
  - is_font_italic_like matches is_font_like everywhere tried
  - a slant-less duck fails both
*/
bool
tests_is_font_italic_like_is_is_font_like()
{
    D_FT_CHECK(is_font_italic_like_v<bare_font> == is_font_like_v<bare_font>);
    D_FT_CHECK(is_font_italic_like_v<full_font> == is_font_like_v<full_font>);
    D_FT_CHECK(is_font_italic_like_v<duck_font> == is_font_like_v<duck_font>);
    D_FT_CHECK(is_font_italic_like_v<duck_no_slant> ==
               is_font_like_v<duck_no_slant>);

    D_FT_CHECK(is_font_italic_like_v<bare_font>);
    D_FT_CHECK(!is_font_italic_like_v<duck_no_slant>);
    D_FT_CHECK(!is_font_italic_like_v<std::string>);

    return true;
}

/*
tests_is_font_with_decorations
  Tests the following:
  - a font-like type WITH a decoration satisfies it
  - a bare font (font-like, no decoration) does not
  - it holds for any single decoration, via the disjunction underneath
*/
bool
tests_is_font_with_decorations()
{
    D_FT_CHECK(is_font_with_decorations_v<underline_font_t>);
    D_FT_CHECK(is_font_with_decorations_v<strike_font>);
    D_FT_CHECK(is_font_with_decorations_v<overline_font_t>);
    D_FT_CHECK(is_font_with_decorations_v<decorated_font_t>);
    D_FT_CHECK(is_font_with_decorations_v<full_font>);

    D_FT_CHECK(!is_font_with_decorations_v<bare_font>);
    D_FT_CHECK(!is_font_with_decorations_v<casing_font_t>);
    D_FT_CHECK(!is_font_with_decorations_v<terminal_font>);

    return true;
}

/*
tests_is_font_with_decorations_needs_the_font_like_floor
  The AND that matters.  duck_underline_only HAS a decoration but is not
font-like, so it must fail is_font_with_decorations even though it passes
has_font_decorations.  Without the floor, the composite would accept it.
  Tests the following:
  - duck_underline_only passes has_font_decorations
  - but FAILS is_font_with_decorations, because it is not font-like
  - a font-like duck with a decoration bolted on passes both
*/
bool
tests_is_font_with_decorations_needs_the_font_like_floor()
{
    // the capability is present...
    D_FT_CHECK(has_font_decorations_v<duck_underline_only>);

    // ...but the floor is not, so the composite rejects it
    D_FT_CHECK(!is_font_like_v<duck_underline_only>);
    D_FT_CHECK(!is_font_with_decorations_v<duck_underline_only>);

    return true;
}

/*
tests_is_font_with_casing
  Tests the following:
  - a font-like type with any casing feature satisfies it
  - a bare font does not
  - a decorations-only font does not
*/
bool
tests_is_font_with_casing()
{
    D_FT_CHECK(is_font_with_casing_v<small_caps_font_t>);
    D_FT_CHECK(is_font_with_casing_v<casing_font_t>);
    D_FT_CHECK(is_font_with_casing_v<full_font>);

    D_FT_CHECK(!is_font_with_casing_v<bare_font>);
    D_FT_CHECK(!is_font_with_casing_v<decorated_font_t>);
    D_FT_CHECK(!is_font_with_casing_v<terminal_font>);

    return true;
}

/*
tests_is_font_with_color
  Tests the following:
  - a font-like type with a foreground satisfies it
  - a background-only font does NOT (color means foreground here)
  - a bare font does not
  - a foreground-only duck (not font-like) does not
*/
bool
tests_is_font_with_color()
{
    D_FT_CHECK(is_font_with_color_v<color_font>);
    D_FT_CHECK(is_font_with_color_v<terminal_font>);
    D_FT_CHECK(is_font_with_color_v<full_font>);

    D_FT_CHECK(!is_font_with_color_v<background_font>);   // background != color
    D_FT_CHECK(!is_font_with_color_v<bare_font>);
    D_FT_CHECK(!is_font_with_color_v<decorated_font_t>);

    // capability without the floor
    D_FT_CHECK(has_font_color_v<duck_foreground_only>);
    D_FT_CHECK(!is_font_with_color_v<duck_foreground_only>);

    return true;
}

/*
tests_is_font_with_background
  Tests the following:
  - a font-like type with the paired background members satisfies it
  - a foreground-only font does not
  - a nominal-background duck (no enable flag, not font-like) does not, for
    BOTH reasons
*/
bool
tests_is_font_with_background()
{
    D_FT_CHECK(is_font_with_background_v<background_font>);
    D_FT_CHECK(is_font_with_background_v<terminal_font>);
    D_FT_CHECK(is_font_with_background_v<full_font>);

    D_FT_CHECK(!is_font_with_background_v<color_font>);   // foreground only
    D_FT_CHECK(!is_font_with_background_v<bare_font>);

    // duck_background_paired has the pair but no floor
    D_FT_CHECK(has_font_background_v<duck_background_paired>);
    D_FT_CHECK(!is_font_with_background_v<duck_background_paired>);

    // duck_background_only fails the conjunction AND the floor
    D_FT_CHECK(!has_font_background_v<duck_background_only>);
    D_FT_CHECK(!is_font_with_background_v<duck_background_only>);

    return true;
}

/*
tests_is_font_with_metrics
  Tests the following:
  - a font-like type with either metric override satisfies it
  - a bare font does not
*/
bool
tests_is_font_with_metrics()
{
    D_FT_CHECK(is_font_with_metrics_v<letter_font>);
    D_FT_CHECK(is_font_with_metrics_v<line_font>);
    D_FT_CHECK(is_font_with_metrics_v<metrics_font>);
    D_FT_CHECK(is_font_with_metrics_v<gui_basic_font>);
    D_FT_CHECK(is_font_with_metrics_v<full_font>);

    D_FT_CHECK(!is_font_with_metrics_v<bare_font>);
    D_FT_CHECK(!is_font_with_metrics_v<terminal_font>);
    D_FT_CHECK(!is_font_with_metrics_v<decorated_font_t>);

    return true;
}

/*
tests_is_font_with_opentype
  Tests the following:
  - a font-like type with the OpenType feature list satisfies it
  - ff_gui_standard, which stops short of OpenType, does NOT
  - a bare font does not
*/
bool
tests_is_font_with_opentype()
{
    D_FT_CHECK(is_font_with_opentype_v<opentype_font>);
    D_FT_CHECK(is_font_with_opentype_v<gui_rich_font>);
    D_FT_CHECK(is_font_with_opentype_v<full_font>);

    D_FT_CHECK(!is_font_with_opentype_v<gui_std_font>);   // stops short
    D_FT_CHECK(!is_font_with_opentype_v<bare_font>);
    D_FT_CHECK(!is_font_with_opentype_v<variable_font_t>);

    return true;
}

/*
tests_is_font_variable
  Tests the following:
  - a font-like type carrying variable axes satisfies it
  - a font with the OTHER kind of axis (stretch/spacing) does not
  - a bare font does not
*/
bool
tests_is_font_variable()
{
    D_FT_CHECK(is_font_variable_v<variable_font_t>);
    D_FT_CHECK(is_font_variable_v<gui_rich_font>);
    D_FT_CHECK(is_font_variable_v<full_font>);

    D_FT_CHECK(!is_font_variable_v<axes_font>);        // stretch/spacing != var
    D_FT_CHECK(!is_font_variable_v<bare_font>);
    D_FT_CHECK(!is_font_variable_v<gui_std_font>);

    return true;
}

/*
tests_is_font_terminal_accepts_color_or_background
  is_font_terminal is font-like AND (color OR background) - a disjunction
nested inside a conjunction.  So a font needs the floor plus EITHER color
member, and both single-color cases must be checked, not just the profile
that has both.
  Tests the following:
  - a font-like type with only a foreground is terminal
  - a font-like type with only a background is terminal
  - the ff_terminal_basic profile (both) is terminal
  - a font-like type with NEITHER is not
*/
bool
tests_is_font_terminal_accepts_color_or_background()
{
    // color only
    D_FT_CHECK(is_font_terminal_v<color_font>);

    // background only
    D_FT_CHECK(is_font_terminal_v<background_font>);

    // both
    D_FT_CHECK(is_font_terminal_v<terminal_font>);
    D_FT_CHECK(is_font_terminal_v<full_font>);
    D_FT_CHECK(is_font_terminal_v<gui_basic_font>);   // has color

    // neither color nor background
    D_FT_CHECK(!is_font_terminal_v<bare_font>);
    D_FT_CHECK(!is_font_terminal_v<decorated_font_t>);
    D_FT_CHECK(!is_font_terminal_v<metrics_font>);

    // a duck that is font-like AND has a foreground qualifies
    D_FT_CHECK(is_font_terminal_v<duck_terminal>);

    return true;
}

/*
tests_is_font_terminal_needs_the_font_like_floor
  Tests the following:
  - a foreground-only duck has the color half but not the floor, so it is
    NOT terminal
*/
bool
tests_is_font_terminal_needs_the_font_like_floor()
{
    D_FT_CHECK(has_font_color_v<duck_foreground_only>);
    D_FT_CHECK(!is_font_like_v<duck_foreground_only>);
    D_FT_CHECK(!is_font_terminal_v<duck_foreground_only>);

    return true;
}

/*
tests_is_font_rich_on_the_rich_profile
  is_font_rich is the tallest gate: font-like AND decorations AND casing AND
metrics AND color AND opentype.  Six conjuncts.
  Tests the following:
  - ff_gui_rich and ff_all satisfy it
  - the dedicated minimal_rich_font satisfies it
*/
bool
tests_is_font_rich_on_the_rich_profile()
{
    D_FT_CHECK(is_font_rich_v<gui_rich_font>);
    D_FT_CHECK(is_font_rich_v<full_font>);
    D_FT_CHECK(is_font_rich_v<minimal_rich_font>);

    return true;
}

/*
tests_is_font_rich_rejects_gui_standard
  The sharp negative that section 1 set up.  ff_gui_standard has
decorations, casing, metrics, and color - four of the six conjuncts - but NO
OpenType feature list, so is_font_rich rejects it.  A caller must not assume
"the standard GUI profile" is rich enough for a shaping-aware renderer.
  Tests the following:
  - gui_std_font is font-like, decorated, cased, metric-capable, and colored
  - but it lacks OpenType features
  - so is_font_rich is false
*/
bool
tests_is_font_rich_rejects_gui_standard()
{
    // it has almost everything...
    D_FT_CHECK(is_font_like_v<gui_std_font>);
    D_FT_CHECK(has_font_decorations_v<gui_std_font>);
    D_FT_CHECK(has_font_casing_v<gui_std_font>);
    D_FT_CHECK(has_font_metrics_overrides_v<gui_std_font>);
    D_FT_CHECK(has_font_color_v<gui_std_font>);

    // ...except the one conjunct that tips it
    D_FT_CHECK(!has_font_opentype_features_v<gui_std_font>);
    D_FT_CHECK(!is_font_rich_v<gui_std_font>);

    return true;
}

/*
tests_is_font_rich_rejects_gui_basic
  Tests the following:
  - ff_gui_basic has decorations, metrics, and color but NO casing and NO
    opentype, so it fails is_font_rich on two counts
*/
bool
tests_is_font_rich_rejects_gui_basic()
{
    D_FT_CHECK(is_font_like_v<gui_basic_font>);
    D_FT_CHECK(has_font_decorations_v<gui_basic_font>);
    D_FT_CHECK(has_font_metrics_overrides_v<gui_basic_font>);
    D_FT_CHECK(has_font_color_v<gui_basic_font>);

    // missing casing AND opentype
    D_FT_CHECK(!has_font_casing_v<gui_basic_font>);
    D_FT_CHECK(!has_font_opentype_features_v<gui_basic_font>);
    D_FT_CHECK(!is_font_rich_v<gui_basic_font>);

    // and a few obviously-not-rich fonts
    D_FT_CHECK(!is_font_rich_v<bare_font>);
    D_FT_CHECK(!is_font_rich_v<terminal_font>);
    D_FT_CHECK(!is_font_rich_v<decorated_font_t>);

    return true;
}

/*
tests_is_font_rich_has_a_minimal_satisfying_set
  is_font_rich is a disjunction-free conjunction of six requirements, but
each requirement is itself satisfiable in the CHEAPEST possible way - one
decoration, one casing transform, one metric.  minimal_rich_font is exactly
that minimal witness, and it proves the gate does NOT secretly demand
ff_gui_rich's full bit set.  Dropping any single one of its features must
break it.
  Tests the following:
  - the minimal set (underline + small_caps + line_height + color +
    opentype) is rich
  - it carries NONE of: background, stretch, spacing, variable axes, script
    hint, backend handles - so richness is independent of those
  - removing the OpenType features from it makes it non-rich
  - removing the decoration from it makes it non-rich
*/
bool
tests_is_font_rich_has_a_minimal_satisfying_set()
{
    D_FT_CHECK(is_font_rich_v<minimal_rich_font>);

    // it is genuinely minimal: none of these are present
    D_FT_CHECK(!has_font_background_v<minimal_rich_font>);
    D_FT_CHECK(!has_font_stretch_v<minimal_rich_font>);
    D_FT_CHECK(!has_font_spacing_v<minimal_rich_font>);
    D_FT_CHECK(!has_font_variable_axes_v<minimal_rich_font>);
    D_FT_CHECK(!has_font_script_hint_v<minimal_rich_font>);
    D_FT_CHECK(!has_font_backend_handles_v<minimal_rich_font>);

    // drop the opentype conjunct -> not rich
    using no_ot = font<ff_underline | ff_small_caps | ff_line_height
                     | ff_color, probe_color>;

    D_FT_CHECK(!has_font_opentype_features_v<no_ot>);
    D_FT_CHECK(!is_font_rich_v<no_ot>);

    // drop the decoration conjunct -> not rich
    using no_dec = font<ff_small_caps | ff_line_height | ff_color
                      | ff_opentype_features, probe_color>;

    D_FT_CHECK(!has_font_decorations_v<no_dec>);
    D_FT_CHECK(!is_font_rich_v<no_dec>);

    // drop the casing conjunct -> not rich
    using no_cas = font<ff_underline | ff_line_height | ff_color
                      | ff_opentype_features, probe_color>;

    D_FT_CHECK(!has_font_casing_v<no_cas>);
    D_FT_CHECK(!is_font_rich_v<no_cas>);

    // drop the metrics conjunct -> not rich
    using no_met = font<ff_underline | ff_small_caps | ff_color
                      | ff_opentype_features, probe_color>;

    D_FT_CHECK(!has_font_metrics_overrides_v<no_met>);
    D_FT_CHECK(!is_font_rich_v<no_met>);

    // drop the color conjunct -> not rich
    using no_col = font<ff_underline | ff_small_caps | ff_line_height
                      | ff_opentype_features, probe_color>;

    D_FT_CHECK(!has_font_color_v<no_col>);
    D_FT_CHECK(!is_font_rich_v<no_col>);

    return true;
}

/*
tests_composite_v_aliases_agree_with_value
  Tests the following:
  - every is_font_* composite _v alias agrees with its ::value, on a font
    that satisfies it and one that does not
*/
bool
tests_composite_v_aliases_agree_with_value()
{
    D_FT_CHECK(is_font_like_v<full_font> == is_font_like<full_font>::value);
    D_FT_CHECK(is_font_bold_like_v<full_font> ==
               is_font_bold_like<full_font>::value);
    D_FT_CHECK(is_font_italic_like_v<full_font> ==
               is_font_italic_like<full_font>::value);
    D_FT_CHECK(is_font_with_decorations_v<full_font> ==
               is_font_with_decorations<full_font>::value);
    D_FT_CHECK(is_font_with_casing_v<full_font> ==
               is_font_with_casing<full_font>::value);
    D_FT_CHECK(is_font_with_color_v<full_font> ==
               is_font_with_color<full_font>::value);
    D_FT_CHECK(is_font_with_background_v<full_font> ==
               is_font_with_background<full_font>::value);
    D_FT_CHECK(is_font_with_metrics_v<full_font> ==
               is_font_with_metrics<full_font>::value);
    D_FT_CHECK(is_font_with_opentype_v<full_font> ==
               is_font_with_opentype<full_font>::value);
    D_FT_CHECK(is_font_variable_v<full_font> ==
               is_font_variable<full_font>::value);
    D_FT_CHECK(is_font_terminal_v<full_font> ==
               is_font_terminal<full_font>::value);
    D_FT_CHECK(is_font_rich_v<full_font> == is_font_rich<full_font>::value);

    // and on a bare font, where most are false
    D_FT_CHECK(is_font_with_decorations_v<bare_font> ==
               is_font_with_decorations<bare_font>::value);
    D_FT_CHECK(is_font_rich_v<bare_font> == is_font_rich<bare_font>::value);
    D_FT_CHECK(is_font_terminal_v<bare_font> ==
               is_font_terminal<bare_font>::value);
    D_FT_CHECK(!is_font_rich_v<bare_font>);
    D_FT_CHECK(!is_font_terminal_v<bare_font>);

    return true;
}

/*
tests_enable_if_helpers_steer_overload_resolution
  The enable_if_* aliases exist to gate overloads, and the only faithful
test of that is a real overload set.  The suite's probe_* helpers build one:
a constrained overload taking an int and a fallback taking a long, so a
literal 0 prefers the constrained one exactly when its enable_if_* alias is
well-formed.  A helper that evaluated to void but did NOT actually remove the
overload on failure would return 1 for a non-font and be caught here.
  Tests the following:
  - enable_if_font_like admits font-like types and duck_font, rejects
    non-fonts
  - enable_if_has_font_underline admits underline fonts, rejects the rest
  - enable_if_has_font_strikethrough likewise
  - enable_if_has_font_color admits colored fonts, rejects background-only
  - enable_if_is_font_variable admits variable fonts
  - enable_if_is_font_rich admits rich fonts, rejects gui_standard
*/
bool
tests_enable_if_helpers_steer_overload_resolution()
{
    // font-like
    D_FT_CHECK(probe_font_like<bare_font>()   == 1);
    D_FT_CHECK(probe_font_like<full_font>()   == 1);
    D_FT_CHECK(probe_font_like<duck_font>()   == 1);
    D_FT_CHECK(probe_font_like<int>()         == 0);
    D_FT_CHECK(probe_font_like<duck_no_size>()== 0);

    // underline
    D_FT_CHECK(probe_underline<underline_font_t>() == 1);
    D_FT_CHECK(probe_underline<full_font>()        == 1);
    D_FT_CHECK(probe_underline<bare_font>()        == 0);
    D_FT_CHECK(probe_underline<strike_font>()      == 0);

    // strikethrough
    D_FT_CHECK(probe_strikethrough<strike_font>()   == 1);
    D_FT_CHECK(probe_strikethrough<full_font>()     == 1);
    D_FT_CHECK(probe_strikethrough<underline_font_t>()== 0);
    D_FT_CHECK(probe_strikethrough<bare_font>()     == 0);

    // color
    D_FT_CHECK(probe_color_capable<color_font>()     == 1);
    D_FT_CHECK(probe_color_capable<terminal_font>()  == 1);
    D_FT_CHECK(probe_color_capable<background_font>() == 0);   // fg only
    D_FT_CHECK(probe_color_capable<bare_font>()      == 0);

    // variable
    D_FT_CHECK(probe_variable<variable_font_t>() == 1);
    D_FT_CHECK(probe_variable<full_font>()       == 1);
    D_FT_CHECK(probe_variable<axes_font>()       == 0);
    D_FT_CHECK(probe_variable<bare_font>()       == 0);

    // rich
    D_FT_CHECK(probe_rich<gui_rich_font>()     == 1);
    D_FT_CHECK(probe_rich<minimal_rich_font>() == 1);
    D_FT_CHECK(probe_rich<gui_std_font>()      == 0);
    D_FT_CHECK(probe_rich<bare_font>()         == 0);

    return true;
}

/*
tests_enable_if_helpers_are_void_on_success
  Tests the following:
  - each enable_if_* alias is exactly `void` when its condition holds - that
    is the std::enable_if success type, and code that spells
    `enable_if_font_like<F>* = nullptr` relies on it
*/
bool
tests_enable_if_helpers_are_void_on_success()
{
    D_FT_CHECK((std::is_same<enable_if_font_like<bare_font>, void>::value));
    D_FT_CHECK((std::is_same<enable_if_font_like<duck_font>, void>::value));
    D_FT_CHECK((std::is_same<enable_if_has_font_underline<underline_font_t>,
                             void>::value));
    D_FT_CHECK((std::is_same<
        enable_if_has_font_strikethrough<strike_font>, void>::value));
    D_FT_CHECK((std::is_same<enable_if_has_font_color<color_font>,
                             void>::value));
    D_FT_CHECK((std::is_same<enable_if_is_font_variable<variable_font_t>,
                             void>::value));
    D_FT_CHECK((std::is_same<enable_if_is_font_rich<gui_rich_font>,
                             void>::value));

    return true;
}

/*
tests_enable_if_helpers_are_ill_formed_on_failure
  The other half: when the condition fails, the alias must NOT name a type -
that ill-formedness is what removes an overload from a candidate set.  It
cannot be observed with std::is_same (naming the alias would be a hard
error), so it is observed the way the compiler observes it: through
is_detected on the alias template itself.
  Tests the following:
  - enable_if_font_like is detected on a font, NOT detected on a non-font
  - the same for the underline, color, variable, and rich helpers
*/
bool
tests_enable_if_helpers_are_ill_formed_on_failure()
{
    // well-formed on the yes case, ill-formed on the no case
    D_FT_CHECK((djinterp::is_detected<enable_if_font_like, bare_font>::value));
    D_FT_CHECK(!(djinterp::is_detected<enable_if_font_like, int>::value));
    D_FT_CHECK(!(djinterp::is_detected<enable_if_font_like,
                                       duck_no_slant>::value));

    D_FT_CHECK((djinterp::is_detected<enable_if_has_font_underline,
                                      underline_font_t>::value));
    D_FT_CHECK(!(djinterp::is_detected<enable_if_has_font_underline,
                                       bare_font>::value));

    D_FT_CHECK((djinterp::is_detected<enable_if_has_font_color,
                                      color_font>::value));
    D_FT_CHECK(!(djinterp::is_detected<enable_if_has_font_color,
                                       background_font>::value));

    D_FT_CHECK((djinterp::is_detected<enable_if_is_font_variable,
                                      variable_font_t>::value));
    D_FT_CHECK(!(djinterp::is_detected<enable_if_is_font_variable,
                                       axes_font>::value));

    D_FT_CHECK((djinterp::is_detected<enable_if_is_font_rich,
                                      gui_rich_font>::value));
    D_FT_CHECK(!(djinterp::is_detected<enable_if_is_font_rich,
                                       gui_std_font>::value));

    return true;
}

NS_END  // testing
NS_END  // djinterp
