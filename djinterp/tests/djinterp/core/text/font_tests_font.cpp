// djinterp [test]  font_tests_font.cpp
//   Section 8: font<_Feat, _ColorType> itself - the always-present core, the
// seventeen static has_* gates, empty(), the constructor set, and value
// semantics.

// djinterp
#include "font_tests.hpp"


NS_DJINTERP
NS_TESTING

/*
tests_font_default_member_values
  The core is present on EVERY font, whatever the feature set, and a
default-constructed one must land on the documented values.  size = 10.0f in
points is the one a caller is most likely to assume rather than check.
  Tests the following:
  - family and style_name start empty
  - size starts at 10.0f
  - size_unit starts at points
  - weight starts at normal, weight_numeric at 0 (i.e. "use the enum")
  - slant starts at upright
  - the same holds for a font with every feature enabled
*/
bool
tests_font_default_member_values()
{
    const bare_font f;

    D_FT_CHECK(f.family.empty());
    D_FT_CHECK(f.style_name.empty());
    D_FT_CHECK_NEAR(f.size, 10.0f);
    D_FT_CHECK(f.size_unit      == font_size_unit::points);
    D_FT_CHECK(f.weight         == font_weight::normal);
    D_FT_CHECK(f.weight_numeric == 0u);
    D_FT_CHECK(f.slant          == font_slant::upright);

    // the core does not change when features are piled on
    const full_font g;

    D_FT_CHECK(g.family.empty());
    D_FT_CHECK_NEAR(g.size, 10.0f);
    D_FT_CHECK(g.size_unit      == font_size_unit::points);
    D_FT_CHECK(g.weight         == font_weight::normal);
    D_FT_CHECK(g.weight_numeric == 0u);
    D_FT_CHECK(g.slant          == font_slant::upright);

    return true;
}

/*
tests_font_default_color_type_is_rgb
  The default _ColorType is the native djinterp::rgb, NOT the legacy
font_color that also lives in this header.
  Tests the following:
  - font<>::color_type is rgb
  - font<ff_color>::color_type is rgb as well (the default is independent of
    the feature set)
  - font_color remains available as an explicit opt-in
*/
bool
tests_font_default_color_type_is_rgb()
{
    D_FT_CHECK((std::is_same<font<>::color_type, rgb>::value));
    D_FT_CHECK((std::is_same<font<ff_color>::color_type, rgb>::value));
    D_FT_CHECK((std::is_same<font<ff_all>::color_type, rgb>::value));

    // the legacy type still works when named
    D_FT_CHECK((std::is_same<font<ff_color, font_color>::color_type,
                             font_color>::value));

    // and rgb is not font_color - the default really did move
    D_FT_CHECK(!(std::is_same<rgb, font_color>::value));

    return true;
}

/*
tests_font_color_type_alias_follows_the_parameter
  Tests the following:
  - font<F, C>::color_type is C for several C
  - the alias is present even when NO color feature is enabled, which is
    what lets an adapter name the color type before deciding whether to use
    it
*/
bool
tests_font_color_type_alias_follows_the_parameter()
{
    D_FT_CHECK((std::is_same<bare_font::color_type,  probe_color>::value));
    D_FT_CHECK((std::is_same<full_font::color_type,  probe_color>::value));
    D_FT_CHECK((std::is_same<font<ff_color, wide_color>::color_type,
                             wide_color>::value));
    D_FT_CHECK((std::is_same<font<ff_none, font_color>::color_type,
                             font_color>::value));

    // present on a font with no color feature at all
    D_FT_CHECK(!bare_font::has_color);
    D_FT_CHECK(!bare_font::has_background);
    D_FT_CHECK((std::is_same<bare_font::color_type, probe_color>::value));

    return true;
}

/*
tests_font_features_constant_echoes_feat
  Tests the following:
  - font<F>::features is exactly F, for the empty set, singletons, profiles,
    and the full set
  - `features` is a constant expression
*/
bool
tests_font_features_constant_echoes_feat()
{
    static_assert((bare_font::features == static_cast<unsigned>(ff_none)),
                  "font<>::features must echo _Feat");
    static_assert((full_font::features == static_cast<unsigned>(ff_all)),
                  "font<>::features must echo _Feat");

    D_FT_CHECK(bare_font::features        == static_cast<unsigned>(ff_none));
    D_FT_CHECK(underline_font_t::features == static_cast<unsigned>(ff_underline));
    D_FT_CHECK(decorated_font_t::features == static_cast<unsigned>(ff_decorations));
    D_FT_CHECK(terminal_font::features    == static_cast<unsigned>(ff_terminal_basic));
    D_FT_CHECK(gui_std_font::features     == static_cast<unsigned>(ff_gui_standard));
    D_FT_CHECK(full_font::features        == static_cast<unsigned>(ff_all));

    // a hand-rolled combination round-trips too
    using combo = font<ff_underline | ff_color, probe_color>;

    D_FT_CHECK(combo::features == (ff_underline | ff_color));

    return true;
}

/*
tests_font_static_flags_agree_with_has_ff
  The seventeen static has_* constants are what an adapter branches on with
`if constexpr`.  Each must be exactly has_ff(_Feat, its bit) - a single
copy-paste slip in that block would misroute a whole capability.  The helper
checks all seventeen at once, so the sweep below covers 17 x 12 gates.
  Tests the following:
  - every has_* constant agrees with has_ff across the empty set, each
    aggregate, every platform profile, and the full set
*/
bool
tests_font_static_flags_agree_with_has_ff()
{
    static_assert(static_flags_agree<ff_none>(), "flags must track _Feat");
    static_assert(static_flags_agree<ff_all>(),  "flags must track _Feat");

    D_FT_CHECK(static_flags_agree<ff_none>());
    D_FT_CHECK(static_flags_agree<ff_underline>());
    D_FT_CHECK(static_flags_agree<ff_strikethrough>());
    D_FT_CHECK(static_flags_agree<ff_overline>());
    D_FT_CHECK(static_flags_agree<ff_decorations>());
    D_FT_CHECK(static_flags_agree<ff_casing>());
    D_FT_CHECK(static_flags_agree<ff_metrics>());
    D_FT_CHECK(static_flags_agree<ff_axes>());
    D_FT_CHECK(static_flags_agree<ff_terminal_basic>());
    D_FT_CHECK(static_flags_agree<ff_terminal_rich>());
    D_FT_CHECK(static_flags_agree<ff_gui_basic>());
    D_FT_CHECK(static_flags_agree<ff_gui_standard>());
    D_FT_CHECK(static_flags_agree<ff_gui_rich>());
    D_FT_CHECK(static_flags_agree<ff_all>());

    // and on an arbitrary sparse combination
    D_FT_CHECK(static_flags_agree<ff_superscript | ff_script_hint>());

    return true;
}

/*
tests_font_static_flags_are_constant_expressions
  `if constexpr (F::has_underline)` is the whole point of the static gates,
so they must be usable where only a constant expression will do.
  Tests the following:
  - the flags fold in a static_assert
  - they are usable as non-type template arguments
  - if constexpr branches on them
*/
bool
tests_font_static_flags_are_constant_expressions()
{
    static_assert(underline_font_t::has_underline,
                  "has_underline must be a constant expression");
    static_assert(!underline_font_t::has_overline,
                  "has_overline must be a constant expression");

    using probe = std::integral_constant<bool, full_font::has_variable_axes>;

    D_FT_CHECK(probe::value);

    // the branch an adapter actually writes
    int taken = 0;

    if constexpr (underline_font_t::has_underline)
    {
        taken = 1;
    }
    else
    {
        taken = 2;
    }

    D_FT_CHECK(taken == 1);

    if constexpr (bare_font::has_underline)
    {
        taken = 1;
    }
    else
    {
        taken = 2;
    }

    D_FT_CHECK(taken == 2);

    return true;
}

/*
tests_font_static_flags_all_false_on_ff_none
  Tests the following:
  - every one of the seventeen gates is false on a bare font
*/
bool
tests_font_static_flags_all_false_on_ff_none()
{
    D_FT_CHECK(!bare_font::has_underline);
    D_FT_CHECK(!bare_font::has_strikethrough);
    D_FT_CHECK(!bare_font::has_overline);
    D_FT_CHECK(!bare_font::has_small_caps);
    D_FT_CHECK(!bare_font::has_all_caps);
    D_FT_CHECK(!bare_font::has_subscript);
    D_FT_CHECK(!bare_font::has_superscript);
    D_FT_CHECK(!bare_font::has_letter_spacing);
    D_FT_CHECK(!bare_font::has_line_height);
    D_FT_CHECK(!bare_font::has_stretch);
    D_FT_CHECK(!bare_font::has_spacing);
    D_FT_CHECK(!bare_font::has_color);
    D_FT_CHECK(!bare_font::has_background);
    D_FT_CHECK(!bare_font::has_opentype_features);
    D_FT_CHECK(!bare_font::has_variable_axes);
    D_FT_CHECK(!bare_font::has_script_hint);
    D_FT_CHECK(!bare_font::has_backend_handles);

    return true;
}

/*
tests_font_static_flags_all_true_on_ff_all
  Tests the following:
  - every one of the seventeen gates is true on a full font
*/
bool
tests_font_static_flags_all_true_on_ff_all()
{
    D_FT_CHECK(full_font::has_underline);
    D_FT_CHECK(full_font::has_strikethrough);
    D_FT_CHECK(full_font::has_overline);
    D_FT_CHECK(full_font::has_small_caps);
    D_FT_CHECK(full_font::has_all_caps);
    D_FT_CHECK(full_font::has_subscript);
    D_FT_CHECK(full_font::has_superscript);
    D_FT_CHECK(full_font::has_letter_spacing);
    D_FT_CHECK(full_font::has_line_height);
    D_FT_CHECK(full_font::has_stretch);
    D_FT_CHECK(full_font::has_spacing);
    D_FT_CHECK(full_font::has_color);
    D_FT_CHECK(full_font::has_background);
    D_FT_CHECK(full_font::has_opentype_features);
    D_FT_CHECK(full_font::has_variable_axes);
    D_FT_CHECK(full_font::has_script_hint);
    D_FT_CHECK(full_font::has_backend_handles);

    return true;
}

/*
tests_font_empty_on_default
  Tests the following:
  - a default-constructed font is empty()
  - a font constructed with a family is not
*/
bool
tests_font_empty_on_default()
{
    const bare_font blank;

    D_FT_CHECK(blank.empty());

    const bare_font named("Inter");

    D_FT_CHECK(!named.empty());

    return true;
}

/*
tests_font_empty_tracks_the_family_only
  empty() is `family.empty()` and nothing else.  So a font carrying a size,
a style name, a weight, a slant, and a full set of decorations is STILL
"empty" if nobody named a family.  That is the intended meaning - empty
means unresolvable, not untouched - but it is easy to misread, so it gets
its own test.
  Tests the following:
  - setting style_name alone leaves the font empty
  - setting size, weight, slant, and a decoration leaves it empty
  - only a non-empty family clears it
  - a family of " " (a space) is NOT empty - it is a one-character name
*/
bool
tests_font_empty_tracks_the_family_only()
{
    full_font f;

    D_FT_CHECK(f.empty());

    f.style_name = "Bold Italic";

    D_FT_CHECK(f.empty());

    f.size      = 72.0f;
    f.size_unit = font_size_unit::pixels;
    f.weight    = font_weight::black;
    f.slant     = font_slant::oblique;
    f.underline = true;

    D_FT_CHECK(f.empty());

    // only the family speaks
    f.family = "Inter";

    D_FT_CHECK(!f.empty());

    // a blank-but-present name is a name
    bare_font spaced;
    spaced.family = " ";

    D_FT_CHECK(!spaced.empty());

    return true;
}

/*
tests_font_empty_is_const_and_noexcept
  Tests the following:
  - empty() is callable on a const font
  - empty() is noexcept
  - empty() returns bool
  - it is [[nodiscard]] (checked indirectly - the value is used here, as it
    must be at every call site)
*/
bool
tests_font_empty_is_const_and_noexcept()
{
    const full_font f;

    D_FT_CHECK(noexcept(f.empty()));
    D_FT_CHECK((std::is_same<decltype(f.empty()), bool>::value));

    const bool value = f.empty();

    D_FT_CHECK(value);

    return true;
}

/*
tests_font_empty_after_clearing_the_family
  Tests the following:
  - a named font that has its family cleared becomes empty again
  - clearing via fn_set_family with an empty string works too
*/
bool
tests_font_empty_after_clearing_the_family()
{
    bare_font f("Inter", 12.0f);

    D_FT_CHECK(!f.empty());

    f.family.clear();

    D_FT_CHECK(f.empty());

    // the size survives - empty() says nothing about the rest
    D_FT_CHECK_NEAR(f.size, 12.0f);

    f.family = "Iosevka";

    D_FT_CHECK(!f.empty());

    fn_set_family(f, std::string());

    D_FT_CHECK(f.empty());

    return true;
}

/*
tests_font_family_ctor_is_explicit
  The one-argument constructor is `explicit`, so a bare string never becomes
a font by accident - a function taking font<> cannot silently swallow
"Inter".
  Tests the following:
  - font<> IS constructible from std::string
  - font<> is NOT convertible from std::string (the explicit gate holds)
  - the constructed font carries the family and defaults everything else
*/
bool
tests_font_family_ctor_is_explicit()
{
    D_FT_CHECK((std::is_constructible<bare_font, std::string>::value));
    D_FT_CHECK(!(std::is_convertible<std::string, bare_font>::value));

    // and the same for a font with features
    D_FT_CHECK((std::is_constructible<full_font, std::string>::value));
    D_FT_CHECK(!(std::is_convertible<std::string, full_font>::value));

    const bare_font f("Inter");

    D_FT_CHECK(f.family == "Inter");
    D_FT_CHECK(f.style_name.empty());
    D_FT_CHECK_NEAR(f.size, 10.0f);
    D_FT_CHECK(f.weight == font_weight::normal);
    D_FT_CHECK(f.slant  == font_slant::upright);

    return true;
}

/*
tests_font_family_size_ctor
  Tests the following:
  - font(family, size) sets both and leaves the rest at their defaults
  - it accepts an rvalue family and an lvalue family alike
*/
bool
tests_font_family_size_ctor()
{
    const bare_font f("Inter", 14.0f);

    D_FT_CHECK(f.family == "Inter");
    D_FT_CHECK_NEAR(f.size, 14.0f);
    D_FT_CHECK(f.size_unit      == font_size_unit::points);
    D_FT_CHECK(f.weight         == font_weight::normal);
    D_FT_CHECK(f.weight_numeric == 0u);
    D_FT_CHECK(f.slant          == font_slant::upright);

    // an lvalue family is copied in
    const std::string  name = "Iosevka";
    const bare_font    g(name, 11.5f);

    D_FT_CHECK(g.family == "Iosevka");
    D_FT_CHECK(name     == "Iosevka");   // the source is untouched
    D_FT_CHECK_NEAR(g.size, 11.5f);

    return true;
}

/*
tests_font_family_size_weight_ctor
  Tests the following:
  - font(family, size, weight) sets all three
  - the slant parameter defaults to upright
  - weight_numeric stays 0, so the symbolic weight is what takes effect
*/
bool
tests_font_family_size_weight_ctor()
{
    const bare_font f("Inter", 16.0f, font_weight::bold);

    D_FT_CHECK(f.family == "Inter");
    D_FT_CHECK_NEAR(f.size, 16.0f);
    D_FT_CHECK(f.weight         == font_weight::bold);
    D_FT_CHECK(f.weight_numeric == 0u);
    D_FT_CHECK(f.slant          == font_slant::upright);   // the default

    D_FT_CHECK(fn_effective_weight(f) == 700u);
    D_FT_CHECK(fn_is_bold(f));
    D_FT_CHECK(!fn_is_italic(f));

    return true;
}

/*
tests_font_family_size_weight_slant_ctor
  Tests the following:
  - font(family, size, weight, slant) sets all four
  - an oblique slant survives (it is not folded into italic)
*/
bool
tests_font_family_size_weight_slant_ctor()
{
    const bare_font f("Inter", 18.0f, font_weight::light, font_slant::italic);

    D_FT_CHECK(f.family == "Inter");
    D_FT_CHECK_NEAR(f.size, 18.0f);
    D_FT_CHECK(f.weight == font_weight::light);
    D_FT_CHECK(f.slant  == font_slant::italic);
    D_FT_CHECK(fn_is_italic(f));
    D_FT_CHECK(!fn_is_bold(f));

    // oblique is preserved as oblique
    const bare_font o("Inter", 18.0f, font_weight::normal,
                      font_slant::oblique);

    D_FT_CHECK(o.slant == font_slant::oblique);
    D_FT_CHECK(o.slant != font_slant::italic);
    D_FT_CHECK(fn_is_italic(o));

    return true;
}

/*
tests_font_ctors_leave_the_other_members_default
  A constructor that touched a mixin member would be a surprise; none of
them do.
  Tests the following:
  - the four-argument constructor on a full font leaves every mixin member
    at its documented default
*/
bool
tests_font_ctors_leave_the_other_members_default()
{
    const full_font f("Inter", 12.0f, font_weight::bold, font_slant::italic);

    // core: set
    D_FT_CHECK(f.family == "Inter");
    D_FT_CHECK(f.weight == font_weight::bold);

    // everything else: untouched
    D_FT_CHECK(f.style_name.empty());
    D_FT_CHECK(f.weight_numeric == 0u);
    D_FT_CHECK(!f.underline);
    D_FT_CHECK(!f.strikethrough);
    D_FT_CHECK(!f.overline);
    D_FT_CHECK(!f.small_caps);
    D_FT_CHECK(!f.all_caps);
    D_FT_CHECK(!f.subscript);
    D_FT_CHECK(!f.superscript);
    D_FT_CHECK_NEAR(f.letter_spacing, 0.0f);
    D_FT_CHECK_NEAR(f.line_height,    0.0f);
    D_FT_CHECK(f.stretch == font_stretch::normal);
    D_FT_CHECK(f.spacing == font_spacing::any);
    D_FT_CHECK(f.foreground == probe_color {});
    D_FT_CHECK(f.background == probe_color {});
    D_FT_CHECK(!f.background_enabled);
    D_FT_CHECK(f.opentype_features.empty());
    D_FT_CHECK(f.variable_axes.empty());
    D_FT_CHECK(f.script_tag.empty());
    D_FT_CHECK(f.language_tag.empty());
    D_FT_CHECK(f.postscript_name.empty());
    D_FT_CHECK(f.full_name.empty());
    D_FT_CHECK(f.file_path.empty());
    D_FT_CHECK(f.face_index == 0);
    D_FT_CHECK(f.native_handle == nullptr);

    return true;
}

/*
tests_font_is_copyable_and_movable
  font is a plain data aggregate with value semantics; nothing about the
mixins may take that away.
  Tests the following:
  - default-constructible, copy/move constructible, copy/move assignable,
    destructible - for both the bare and the full feature set
  - the move operations are noexcept (the members are string / vector /
    scalars, all of which move without throwing)
*/
bool
tests_font_is_copyable_and_movable()
{
    D_FT_CHECK(std::is_default_constructible<bare_font>::value);
    D_FT_CHECK(std::is_copy_constructible<bare_font>::value);
    D_FT_CHECK(std::is_copy_assignable<bare_font>::value);
    D_FT_CHECK(std::is_move_constructible<bare_font>::value);
    D_FT_CHECK(std::is_move_assignable<bare_font>::value);
    D_FT_CHECK(std::is_destructible<bare_font>::value);

    D_FT_CHECK(std::is_default_constructible<full_font>::value);
    D_FT_CHECK(std::is_copy_constructible<full_font>::value);
    D_FT_CHECK(std::is_copy_assignable<full_font>::value);
    D_FT_CHECK(std::is_move_constructible<full_font>::value);
    D_FT_CHECK(std::is_move_assignable<full_font>::value);
    D_FT_CHECK(std::is_destructible<full_font>::value);

    D_FT_CHECK(std::is_nothrow_move_constructible<bare_font>::value);
    D_FT_CHECK(std::is_nothrow_move_constructible<full_font>::value);

    return true;
}

/*
tests_font_copy_is_deep
  The mixins hold vectors and strings; a copy must not share them.
  Tests the following:
  - mutating a copy's feature vector does not touch the original's
  - mutating a copy's family does not touch the original's
  - the copy starts out carrying every value the original had
*/
bool
tests_font_copy_is_deep()
{
    full_font original("Inter", 12.0f);

    fn_set_opentype_feature(original, ot_tag("liga"), 1u);
    fn_set_variable_axis(original, ot_tag("wght"), 650.0f);
    fn_set_script(original, "latn", "en");
    fn_set_underline(original, true);

    full_font copy = original;

    // the copy arrived intact
    D_FT_CHECK(copy.family == "Inter");
    D_FT_CHECK(copy.opentype_features.size() == 1u);
    D_FT_CHECK(copy.variable_axes.size()     == 1u);
    D_FT_CHECK(copy.script_tag   == "latn");
    D_FT_CHECK(copy.language_tag == "en");
    D_FT_CHECK(copy.underline);

    // and it is independent
    fn_set_opentype_feature(copy, ot_tag("kern"), 1u);
    fn_set_family(copy, "Iosevka");
    fn_set_underline(copy, false);

    D_FT_CHECK(copy.opentype_features.size()     == 2u);
    D_FT_CHECK(original.opentype_features.size() == 1u);
    D_FT_CHECK(copy.family     == "Iosevka");
    D_FT_CHECK(original.family == "Inter");
    D_FT_CHECK(!copy.underline);
    D_FT_CHECK(original.underline);

    return true;
}

/*
tests_font_move_preserves_the_value
  Tests the following:
  - a moved-to font carries every value the source held
  - move assignment does the same
  (The moved-FROM state is deliberately not asserted: std::string's and
  std::vector's moved-from states are valid-but-unspecified, so pinning them
  would be testing the standard library, not font.hpp.)
*/
bool
tests_font_move_preserves_the_value()
{
    full_font source("Inter", 12.0f, font_weight::bold);

    fn_set_opentype_feature(source, ot_tag("liga"), 2u);
    fn_set_script(source, "cyrl", "ru");
    fn_set_letter_spacing(source, 0.05f);

    const full_font moved = std::move(source);

    D_FT_CHECK(moved.family == "Inter");
    D_FT_CHECK_NEAR(moved.size, 12.0f);
    D_FT_CHECK(moved.weight == font_weight::bold);
    D_FT_CHECK(moved.opentype_features.size() == 1u);
    D_FT_CHECK(moved.opentype_features[0].value == 2u);
    D_FT_CHECK(moved.script_tag   == "cyrl");
    D_FT_CHECK(moved.language_tag == "ru");
    D_FT_CHECK_NEAR(moved.letter_spacing, 0.05f);

    // move assignment
    full_font other("Iosevka", 9.0f);
    full_font sink;

    sink = std::move(other);

    D_FT_CHECK(sink.family == "Iosevka");
    D_FT_CHECK_NEAR(sink.size, 9.0f);

    return true;
}

/*
tests_font_mixin_members_are_reachable
  Inheritance, not composition: the mixin members must appear as DIRECT
members of the font, reachable without qualification, or every call site in
the header (`_fn.underline = _on`) would fail.
  Tests the following:
  - every one of the mixin members is readable and writable through the font
  - the values written come back
*/
bool
tests_font_mixin_members_are_reachable()
{
    full_font f;

    f.underline         = true;
    f.strikethrough     = true;
    f.overline          = true;
    f.small_caps        = true;
    f.all_caps          = true;
    f.subscript         = true;
    f.superscript       = true;
    f.letter_spacing    = 0.125f;
    f.line_height       = 1.5f;
    f.stretch           = font_stretch::condensed;
    f.spacing           = font_spacing::monospace;
    f.foreground        = probe_color { 1u, 2u, 3u };
    f.background        = probe_color { 4u, 5u, 6u };
    f.background_enabled= true;
    f.opentype_features.push_back({ ot_tag("liga"), 1u });
    f.variable_axes.push_back({ ot_tag("wght"), 400.0f });
    f.script_tag        = "latn";
    f.language_tag      = "en-GB";
    f.postscript_name   = "Inter-Regular";
    f.full_name         = "Inter Regular";
    f.file_path         = "/usr/share/fonts/Inter.ttc";
    f.face_index        = 2;

    int  anchor  = 0;
    f.native_handle = &anchor;

    D_FT_CHECK(f.underline);
    D_FT_CHECK(f.strikethrough);
    D_FT_CHECK(f.overline);
    D_FT_CHECK(f.small_caps);
    D_FT_CHECK(f.all_caps);
    D_FT_CHECK(f.subscript);
    D_FT_CHECK(f.superscript);
    D_FT_CHECK_NEAR(f.letter_spacing, 0.125f);
    D_FT_CHECK_NEAR(f.line_height,    1.5f);
    D_FT_CHECK(f.stretch == font_stretch::condensed);
    D_FT_CHECK(f.spacing == font_spacing::monospace);
    D_FT_CHECK(f.foreground == (probe_color { 1u, 2u, 3u }));
    D_FT_CHECK(f.background == (probe_color { 4u, 5u, 6u }));
    D_FT_CHECK(f.background_enabled);
    D_FT_CHECK(f.opentype_features.size() == 1u);
    D_FT_CHECK(f.variable_axes.size()     == 1u);
    D_FT_CHECK(f.script_tag      == "latn");
    D_FT_CHECK(f.language_tag    == "en-GB");
    D_FT_CHECK(f.postscript_name == "Inter-Regular");
    D_FT_CHECK(f.full_name       == "Inter Regular");
    D_FT_CHECK(f.file_path       == "/usr/share/fonts/Inter.ttc");
    D_FT_CHECK(f.face_index      == 2);
    D_FT_CHECK(f.native_handle   == &anchor);

    return true;
}

/*
tests_font_distinct_feature_sets_are_distinct_types
  font<F> and font<G> are different types whenever F != G, which is what
stops a terminal font being passed where a GUI font is expected.
  Tests the following:
  - fonts with different feature sets are different types
  - fonts with different color types are different types
  - the same feature set and color type yields the SAME type, however it is
    spelled (ff_all vs ff_gui_rich)
*/
bool
tests_font_distinct_feature_sets_are_distinct_types()
{
    D_FT_CHECK(!(std::is_same<bare_font, underline_font_t>::value));
    D_FT_CHECK(!(std::is_same<terminal_font, gui_basic_font>::value));

    D_FT_CHECK(!(std::is_same<font<ff_color, probe_color>,
                              font<ff_color, wide_color>>::value));

    // spelled differently, the same type
    D_FT_CHECK((std::is_same<font<ff_all,      probe_color>,
                             font<ff_gui_rich, probe_color>>::value));

    // and an OR of the parts reproduces the aggregate exactly
    D_FT_CHECK((std::is_same<
        font<ff_underline | ff_strikethrough | ff_overline, probe_color>,
        font<ff_decorations, probe_color>>::value));

    return true;
}

NS_END  // testing
NS_END  // djinterp
