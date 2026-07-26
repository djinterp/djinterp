// djinterp [test]  font_tests_traits.cpp
//   Section 10, parts I and II: the expression detectors and the individual
// capability traits.
//
//   These traits are STRUCTURAL.  They ask "does this type have a member
// called `underline`?", not "does this type derive from font<>?".  That is
// the whole point - an adapter must be able to accept a Win32 LOGFONT, a
// fontconfig pattern, or a bespoke struct, none of which has ever heard of
// font.hpp.  So the tests here drive the traits from BOTH sides: font<>
// across its feature lattice, and duck-typed probes that share nothing with
// it but a member name.
//
//   The probes also witness things a font<> structurally CANNOT.  Two traits
// are conjunctions (has_font_background, has_font_script_hint) and two are
// disjunctions (has_font_decorations, has_font_backend_handles); a font<>
// always gets a whole mixin or none of it, so only a hand-built half-mixin
// can tell a conjunction from a single detector.

// djinterp
#include "font_tests.hpp"


NS_DJINTERP
NS_TESTING

/*
tests_detector_core_member_types
  Each detector is `decltype(declval<T&>().member)`, so it names the
member's DECLARED type.  Pinning those types matters: an adapter writes
`font_size_t<F> px = ...` and would silently get a double if the model
changed underneath it.
  Tests the following:
  - font_family_t and font_style_name_t are std::string
  - font_size_t is float
  - font_size_unit_t is font_size_unit
  - font_weight_t is font_weight, font_weight_numeric_t is std::uint16_t
  - font_slant_t is font_slant
*/
bool
tests_detector_core_member_types()
{
    D_FT_CHECK((std::is_same<font_family_t<bare_font>, std::string>::value));
    D_FT_CHECK((std::is_same<font_style_name_t<bare_font>,
                             std::string>::value));
    D_FT_CHECK((std::is_same<font_size_t<bare_font>, float>::value));
    D_FT_CHECK((std::is_same<font_size_unit_t<bare_font>,
                             font_size_unit>::value));
    D_FT_CHECK((std::is_same<font_weight_t<bare_font>, font_weight>::value));
    D_FT_CHECK((std::is_same<font_weight_numeric_t<bare_font>,
                             std::uint16_t>::value));
    D_FT_CHECK((std::is_same<font_slant_t<bare_font>, font_slant>::value));

    // and on a duck type that never met font<>
    D_FT_CHECK((std::is_same<font_family_t<duck_font>, std::string>::value));
    D_FT_CHECK((std::is_same<font_size_t<duck_font>, float>::value));

    return true;
}

/*
tests_detector_yields_the_declared_type
  A subtlety with teeth.  decltype of an UNPARENTHESISED member access names
the member's declared type - it does NOT pick up a reference, and it does NOT
pick up the const of the object.  So font_family_t<const font<>> is
std::string, not `const std::string&`.  If the detectors were ever rewritten
with parentheses, every one of these would change type and quietly break the
`is_detected_exact` style checks an adapter might build on them.
  Tests the following:
  - the detected type is a plain value type, not a reference
  - it is not const-qualified, even when probed through a const type
*/
bool
tests_detector_yields_the_declared_type()
{
    // not a reference
    D_FT_CHECK(!std::is_reference<font_family_t<bare_font>>::value);
    D_FT_CHECK(!std::is_reference<font_size_t<bare_font>>::value);

    // not const, even through a const object type
    D_FT_CHECK(!std::is_const<font_family_t<const bare_font>>::value);
    D_FT_CHECK((std::is_same<font_family_t<const bare_font>,
                             std::string>::value));
    D_FT_CHECK((std::is_same<font_size_t<const bare_font>, float>::value));

    // it is exactly the declared type
    D_FT_CHECK((std::is_same<font_family_t<bare_font>,
                             font_family_t<const bare_font>>::value));

    return true;
}

/*
tests_detector_feature_member_types
  Tests the following:
  - the decoration and casing detectors are all bool
  - the metric detectors are float
  - the axis detectors are the axis enums
  - the color detectors are the font's _ColorType, and background_enabled is
    bool
  - the list detectors are the vector types
  - the script and backend string detectors are std::string
  - face_index is int and native_handle is void*
*/
bool
tests_detector_feature_member_types()
{
    // decorations + casing: bool
    D_FT_CHECK((std::is_same<font_underline_t<full_font>,     bool>::value));
    D_FT_CHECK((std::is_same<font_strikethrough_t<full_font>, bool>::value));
    D_FT_CHECK((std::is_same<font_overline_t<full_font>,      bool>::value));
    D_FT_CHECK((std::is_same<font_small_caps_t<full_font>,    bool>::value));
    D_FT_CHECK((std::is_same<font_all_caps_t<full_font>,      bool>::value));
    D_FT_CHECK((std::is_same<font_subscript_t<full_font>,     bool>::value));
    D_FT_CHECK((std::is_same<font_superscript_t<full_font>,   bool>::value));

    // metrics: float
    D_FT_CHECK((std::is_same<font_letter_spacing_t<full_font>, float>::value));
    D_FT_CHECK((std::is_same<font_line_height_t<full_font>,    float>::value));

    // axes: the enums
    D_FT_CHECK((std::is_same<font_stretch_t<full_font>, font_stretch>::value));
    D_FT_CHECK((std::is_same<font_spacing_t<full_font>, font_spacing>::value));

    // color: the parameterised type, plus a bool flag
    D_FT_CHECK((std::is_same<font_foreground_t<full_font>,
                             probe_color>::value));
    D_FT_CHECK((std::is_same<font_background_t<full_font>,
                             probe_color>::value));
    D_FT_CHECK((std::is_same<font_background_enabled_t<full_font>,
                             bool>::value));
    D_FT_CHECK((std::is_same<font_foreground_t<font<ff_color, wide_color>>,
                             wide_color>::value));

    // the lists
    D_FT_CHECK((std::is_same<font_opentype_features_t<full_font>,
                             std::vector<opentype_feature>>::value));
    D_FT_CHECK((std::is_same<font_variable_axes_t<full_font>,
                             std::vector<variable_axis>>::value));

    // script + backend
    D_FT_CHECK((std::is_same<font_script_tag_t<full_font>,
                             std::string>::value));
    D_FT_CHECK((std::is_same<font_language_tag_t<full_font>,
                             std::string>::value));
    D_FT_CHECK((std::is_same<font_postscript_name_t<full_font>,
                             std::string>::value));
    D_FT_CHECK((std::is_same<font_full_name_t<full_font>,
                             std::string>::value));
    D_FT_CHECK((std::is_same<font_file_path_t<full_font>,
                             std::string>::value));
    D_FT_CHECK((std::is_same<font_face_index_t<full_font>, int>::value));
    D_FT_CHECK((std::is_same<font_native_handle_t<full_font>, void*>::value));

    return true;
}

/*
tests_detector_empty_returns_bool
  font_empty_t probes a CALL, not a member - and it probes it through a
`const T&`, which is what makes it a check for a const-callable empty().
  Tests the following:
  - font_empty_t<font<>> is bool
  - it is detected on font<>
  - it is NOT detected on a type with no empty() at all
*/
bool
tests_detector_empty_returns_bool()
{
    D_FT_CHECK((std::is_same<font_empty_t<bare_font>, bool>::value));
    D_FT_CHECK((std::is_same<font_empty_t<full_font>, bool>::value));

    D_FT_CHECK((djinterp::is_detected<font_empty_t, bare_font>::value));

    // duck_font has no empty()
    D_FT_CHECK(!(djinterp::is_detected<font_empty_t, duck_font>::value));
    D_FT_CHECK(!(djinterp::is_detected<font_empty_t, int>::value));

    // std::string DOES have one - the detector is structural, so it fires
    D_FT_CHECK((djinterp::is_detected<font_empty_t, std::string>::value));

    return true;
}

/*
tests_detector_color_type_alias
  font_color_type_t is the odd detector out: it names a nested TYPE ALIAS
rather than a member, and it runs its argument through clean_t first - so it
sees through references and const where the member detectors do so only by
accident.
  Tests the following:
  - it names the font's color_type
  - it works through a reference and through const
  - it does not fire on a type with no color_type alias
*/
bool
tests_detector_color_type_alias()
{
    D_FT_CHECK((std::is_same<font_color_type_t<bare_font>,
                             probe_color>::value));
    D_FT_CHECK((std::is_same<font_color_type_t<font<ff_color, wide_color>>,
                             wide_color>::value));
    D_FT_CHECK((std::is_same<font_color_type_t<font<>>, rgb>::value));

    // clean_t strips the reference and the const
    D_FT_CHECK((std::is_same<font_color_type_t<const bare_font&>,
                             probe_color>::value));
    D_FT_CHECK((std::is_same<font_color_type_t<bare_font&&>,
                             probe_color>::value));

    // and it does not fire on a type without the alias
    D_FT_CHECK(!(djinterp::is_detected<font_color_type_t, duck_font>::value));
    D_FT_CHECK(!(djinterp::is_detected<font_color_type_t, int>::value));

    return true;
}

/*
tests_detectors_are_const_and_reference_tolerant
  The member detectors probe `declval<_Type&>()`, so a const or reference
argument collapses harmlessly - has_font_family<const font<>&> is true.  That
is worth knowing: an adapter constraining on `has_font_family<F>` need not
strip cv-ref from F first.
  Tests the following:
  - the traits fire through const, lvalue reference, and rvalue reference
  - they still reject a genuine non-font behind the same qualifiers
*/
bool
tests_detectors_are_const_and_reference_tolerant()
{
    D_FT_CHECK(has_font_family_v<bare_font>);
    D_FT_CHECK(has_font_family_v<const bare_font>);
    D_FT_CHECK(has_font_family_v<bare_font&>);
    D_FT_CHECK(has_font_family_v<const bare_font&>);
    D_FT_CHECK(has_font_family_v<bare_font&&>);

    D_FT_CHECK(has_font_size_v<const bare_font&>);
    D_FT_CHECK(has_font_weight_v<const bare_font&>);
    D_FT_CHECK(has_font_slant_v<bare_font&>);
    D_FT_CHECK(has_font_underline_v<const underline_font_t&>);

    // and the composite gate survives the qualifiers too
    D_FT_CHECK(is_font_like_v<const bare_font&>);
    D_FT_CHECK(is_font_like_v<duck_font&>);

    // a non-font stays a non-font however it is spelled
    D_FT_CHECK(!has_font_family_v<const int&>);
    D_FT_CHECK(!is_font_like_v<const duck_not_a_font&>);

    return true;
}

/*
tests_has_font_core_traits_on_font
  Tests the following:
  - all seven core traits fire on a font, whatever its feature set
  - they fire on the bare font as readily as on the full one, because the
    core is never gated
*/
bool
tests_has_font_core_traits_on_font()
{
    D_FT_CHECK(has_font_family_v<bare_font>);
    D_FT_CHECK(has_font_style_name_v<bare_font>);
    D_FT_CHECK(has_font_size_v<bare_font>);
    D_FT_CHECK(has_font_size_unit_v<bare_font>);
    D_FT_CHECK(has_font_weight_v<bare_font>);
    D_FT_CHECK(has_font_weight_numeric_v<bare_font>);
    D_FT_CHECK(has_font_slant_v<bare_font>);

    D_FT_CHECK(has_font_family_v<full_font>);
    D_FT_CHECK(has_font_style_name_v<full_font>);
    D_FT_CHECK(has_font_size_v<full_font>);
    D_FT_CHECK(has_font_size_unit_v<full_font>);
    D_FT_CHECK(has_font_weight_v<full_font>);
    D_FT_CHECK(has_font_weight_numeric_v<full_font>);
    D_FT_CHECK(has_font_slant_v<full_font>);

    D_FT_CHECK(has_font_family_v<terminal_font>);
    D_FT_CHECK(has_font_slant_v<gui_std_font>);

    return true;
}

/*
tests_has_font_core_traits_reject_non_fonts
  Tests the following:
  - the traits are false for int, std::string, a vector, and an unrelated
    struct
  - a duck type that has SOME of the core members still fails the ones it
    lacks (has_font_style_name is false on duck_font, which has no
    style_name)
*/
bool
tests_has_font_core_traits_reject_non_fonts()
{
    D_FT_CHECK(!has_font_family_v<int>);
    D_FT_CHECK(!has_font_size_v<int>);
    D_FT_CHECK(!has_font_weight_v<double>);
    D_FT_CHECK(!has_font_slant_v<void*>);

    D_FT_CHECK(!has_font_family_v<std::string>);
    D_FT_CHECK(!has_font_family_v<std::vector<int>>);

    D_FT_CHECK(!has_font_family_v<duck_not_a_font>);
    D_FT_CHECK(!has_font_size_v<duck_not_a_font>);
    D_FT_CHECK(!has_font_weight_v<duck_not_a_font>);
    D_FT_CHECK(!has_font_slant_v<duck_not_a_font>);

    // duck_font has the four required members but NOT the conveniences
    D_FT_CHECK(has_font_family_v<duck_font>);
    D_FT_CHECK(has_font_size_v<duck_font>);
    D_FT_CHECK(has_font_weight_v<duck_font>);
    D_FT_CHECK(has_font_slant_v<duck_font>);

    D_FT_CHECK(!has_font_style_name_v<duck_font>);
    D_FT_CHECK(!has_font_size_unit_v<duck_font>);
    D_FT_CHECK(!has_font_weight_numeric_v<duck_font>);

    return true;
}

/*
tests_has_font_decoration_traits_track_the_flags
  Tests the following:
  - has_font_underline fires exactly when ff_underline is set
  - likewise strikethrough and overline
  - a font with ONE decoration does not report the other two
*/
bool
tests_has_font_decoration_traits_track_the_flags()
{
    D_FT_CHECK(has_font_underline_v<underline_font_t>);
    D_FT_CHECK(!has_font_strikethrough_v<underline_font_t>);
    D_FT_CHECK(!has_font_overline_v<underline_font_t>);

    D_FT_CHECK(!has_font_underline_v<strike_font>);
    D_FT_CHECK(has_font_strikethrough_v<strike_font>);
    D_FT_CHECK(!has_font_overline_v<strike_font>);

    D_FT_CHECK(!has_font_underline_v<overline_font_t>);
    D_FT_CHECK(!has_font_strikethrough_v<overline_font_t>);
    D_FT_CHECK(has_font_overline_v<overline_font_t>);

    D_FT_CHECK(has_font_underline_v<decorated_font_t>);
    D_FT_CHECK(has_font_strikethrough_v<decorated_font_t>);
    D_FT_CHECK(has_font_overline_v<decorated_font_t>);

    D_FT_CHECK(!has_font_underline_v<bare_font>);
    D_FT_CHECK(!has_font_strikethrough_v<bare_font>);
    D_FT_CHECK(!has_font_overline_v<bare_font>);

    return true;
}

/*
tests_has_font_decorations_is_a_disjunction
  has_font_decorations is an OR of the three, so ANY one of them is enough.
Driving it with a font that enables exactly one decoration at a time is what
proves it is not secretly an AND.
  Tests the following:
  - a font with only underline satisfies it
  - a font with only strikethrough satisfies it
  - a font with only overline satisfies it
  - a bare font does not
  - a duck type carrying just `underline` satisfies it too - the trait is
    structural and does not require the other two members to exist
*/
bool
tests_has_font_decorations_is_a_disjunction()
{
    D_FT_CHECK(has_font_decorations_v<underline_font_t>);
    D_FT_CHECK(has_font_decorations_v<strike_font>);
    D_FT_CHECK(has_font_decorations_v<overline_font_t>);
    D_FT_CHECK(has_font_decorations_v<decorated_font_t>);
    D_FT_CHECK(has_font_decorations_v<full_font>);

    D_FT_CHECK(!has_font_decorations_v<bare_font>);
    D_FT_CHECK(!has_font_decorations_v<casing_font_t>);
    D_FT_CHECK(!has_font_decorations_v<terminal_font>);   // color only

    // a bare member is enough - no inheritance, no siblings
    D_FT_CHECK(has_font_decorations_v<duck_underline_only>);
    D_FT_CHECK(has_font_underline_v<duck_underline_only>);
    D_FT_CHECK(!has_font_strikethrough_v<duck_underline_only>);
    D_FT_CHECK(!has_font_overline_v<duck_underline_only>);

    D_FT_CHECK(!has_font_decorations_v<duck_not_a_font>);

    return true;
}

/*
tests_has_font_casing_traits_track_the_flags
  Tests the following:
  - each of the four casing traits fires exactly when its flag is set
  - a font with only small caps does not report the other three
*/
bool
tests_has_font_casing_traits_track_the_flags()
{
    D_FT_CHECK(has_font_small_caps_v<small_caps_font_t>);
    D_FT_CHECK(!has_font_all_caps_v<small_caps_font_t>);
    D_FT_CHECK(!has_font_subscript_v<small_caps_font_t>);
    D_FT_CHECK(!has_font_superscript_v<small_caps_font_t>);

    D_FT_CHECK(has_font_small_caps_v<casing_font_t>);
    D_FT_CHECK(has_font_all_caps_v<casing_font_t>);
    D_FT_CHECK(has_font_subscript_v<casing_font_t>);
    D_FT_CHECK(has_font_superscript_v<casing_font_t>);

    D_FT_CHECK(!has_font_small_caps_v<bare_font>);
    D_FT_CHECK(!has_font_all_caps_v<bare_font>);
    D_FT_CHECK(!has_font_subscript_v<bare_font>);
    D_FT_CHECK(!has_font_superscript_v<bare_font>);

    // individually gated
    D_FT_CHECK(has_font_subscript_v<font<ff_subscript, probe_color>>);
    D_FT_CHECK(!has_font_superscript_v<font<ff_subscript, probe_color>>);
    D_FT_CHECK(has_font_superscript_v<font<ff_superscript, probe_color>>);
    D_FT_CHECK(!has_font_subscript_v<font<ff_superscript, probe_color>>);

    return true;
}

/*
tests_has_font_casing_is_a_disjunction
  Tests the following:
  - any ONE of the four casing members satisfies has_font_casing
  - a bare font does not
  - a decorations-only font does not (casing and decorations are disjoint)
*/
bool
tests_has_font_casing_is_a_disjunction()
{
    D_FT_CHECK(has_font_casing_v<small_caps_font_t>);
    D_FT_CHECK(has_font_casing_v<font<ff_all_caps,    probe_color>>);
    D_FT_CHECK(has_font_casing_v<font<ff_subscript,   probe_color>>);
    D_FT_CHECK(has_font_casing_v<font<ff_superscript, probe_color>>);
    D_FT_CHECK(has_font_casing_v<casing_font_t>);
    D_FT_CHECK(has_font_casing_v<full_font>);

    D_FT_CHECK(!has_font_casing_v<bare_font>);
    D_FT_CHECK(!has_font_casing_v<decorated_font_t>);
    D_FT_CHECK(!has_font_casing_v<duck_not_a_font>);

    return true;
}

/*
tests_has_font_metrics_traits_track_the_flags
  Tests the following:
  - has_font_letter_spacing and has_font_line_height track their own flags
  - a font with only one of them does not report the other
*/
bool
tests_has_font_metrics_traits_track_the_flags()
{
    D_FT_CHECK(has_font_letter_spacing_v<letter_font>);
    D_FT_CHECK(!has_font_line_height_v<letter_font>);

    D_FT_CHECK(!has_font_letter_spacing_v<line_font>);
    D_FT_CHECK(has_font_line_height_v<line_font>);

    D_FT_CHECK(has_font_letter_spacing_v<metrics_font>);
    D_FT_CHECK(has_font_line_height_v<metrics_font>);

    D_FT_CHECK(!has_font_letter_spacing_v<bare_font>);
    D_FT_CHECK(!has_font_line_height_v<bare_font>);

    return true;
}

/*
tests_has_font_metrics_overrides_is_a_disjunction
  Tests the following:
  - either metric alone satisfies has_font_metrics_overrides
  - neither, and it is false
*/
bool
tests_has_font_metrics_overrides_is_a_disjunction()
{
    D_FT_CHECK(has_font_metrics_overrides_v<letter_font>);
    D_FT_CHECK(has_font_metrics_overrides_v<line_font>);
    D_FT_CHECK(has_font_metrics_overrides_v<metrics_font>);
    D_FT_CHECK(has_font_metrics_overrides_v<gui_basic_font>);

    D_FT_CHECK(!has_font_metrics_overrides_v<bare_font>);
    D_FT_CHECK(!has_font_metrics_overrides_v<terminal_font>);
    D_FT_CHECK(!has_font_metrics_overrides_v<decorated_font_t>);

    return true;
}

/*
tests_has_font_axis_traits_track_the_flags
  Tests the following:
  - has_font_stretch and has_font_spacing track their own flags
  - neither is implied by the OTHER kind of axis (the variable-font axes)
*/
bool
tests_has_font_axis_traits_track_the_flags()
{
    D_FT_CHECK(has_font_stretch_v<stretch_font_t>);
    D_FT_CHECK(!has_font_spacing_v<stretch_font_t>);

    D_FT_CHECK(!has_font_stretch_v<spacing_font_t>);
    D_FT_CHECK(has_font_spacing_v<spacing_font_t>);

    D_FT_CHECK(has_font_stretch_v<axes_font>);
    D_FT_CHECK(has_font_spacing_v<axes_font>);

    D_FT_CHECK(!has_font_stretch_v<bare_font>);
    D_FT_CHECK(!has_font_spacing_v<bare_font>);

    // ff_variable_axes is a DIFFERENT concept and implies neither
    D_FT_CHECK(!has_font_stretch_v<variable_font_t>);
    D_FT_CHECK(!has_font_spacing_v<variable_font_t>);
    D_FT_CHECK(has_font_variable_axes_v<variable_font_t>);
    D_FT_CHECK(!has_font_variable_axes_v<axes_font>);

    return true;
}

/*
tests_has_font_color_tracks_the_flag
  has_font_color is a single detector on `foreground` - it does NOT require
a background, and it does NOT require the font to be font-like.
  Tests the following:
  - it fires exactly when ff_color is set
  - a background-only font does NOT satisfy it
  - a duck type carrying only a foreground satisfies it
*/
bool
tests_has_font_color_tracks_the_flag()
{
    D_FT_CHECK(has_font_color_v<color_font>);
    D_FT_CHECK(has_font_color_v<terminal_font>);
    D_FT_CHECK(has_font_color_v<full_font>);

    D_FT_CHECK(!has_font_color_v<bare_font>);
    D_FT_CHECK(!has_font_color_v<decorated_font_t>);

    // background alone is not color
    D_FT_CHECK(!has_font_color_v<background_font>);
    D_FT_CHECK(has_font_background_v<background_font>);

    // and the bare member is enough
    D_FT_CHECK(has_font_color_v<duck_foreground_only>);
    D_FT_CHECK(!is_font_like_v<duck_foreground_only>);

    return true;
}

/*
tests_has_font_background_requires_both_members
  The conjunction, and the one trait in the header that exists specifically
to REJECT a shape rather than accept one.  Its comment says the paired check
"avoids false positives on types that carry only a nominal background field"
- so a struct with a `background` and no `background_enabled` must NOT
satisfy it.  A font<> can never witness this, because its background mixin
always supplies both; only a duck type can.
  Tests the following:
  - a background field with no enable flag does NOT satisfy the trait
  - an enable flag with no background field does NOT satisfy it
  - both together DO
  - font<ff_background> supplies both, so it does
  - the two halves are individually detectable, so the conjunction really is
    doing the work
*/
bool
tests_has_font_background_requires_both_members()
{
    // the false positive the conjunction exists to reject
    D_FT_CHECK(!has_font_background_v<duck_background_only>);
    D_FT_CHECK((djinterp::is_detected<font_background_t,
                                      duck_background_only>::value));
    D_FT_CHECK(!(djinterp::is_detected<font_background_enabled_t,
                                       duck_background_only>::value));

    // the other half, also rejected
    D_FT_CHECK(!has_font_background_v<duck_background_flag_only>);
    D_FT_CHECK(!(djinterp::is_detected<font_background_t,
                                       duck_background_flag_only>::value));
    D_FT_CHECK((djinterp::is_detected<font_background_enabled_t,
                                      duck_background_flag_only>::value));

    // both halves: accepted
    D_FT_CHECK(has_font_background_v<duck_background_paired>);

    // and font<ff_background> always brings both
    D_FT_CHECK(has_font_background_v<background_font>);
    D_FT_CHECK(has_font_background_v<terminal_font>);
    D_FT_CHECK(!has_font_background_v<bare_font>);
    D_FT_CHECK(!has_font_background_v<color_font>);   // foreground only

    return true;
}

/*
tests_has_font_opentype_and_variable_track_the_flags
  Tests the following:
  - has_font_opentype_features fires exactly on ff_opentype_features
  - has_font_variable_axes fires exactly on ff_variable_axes
  - neither implies the other, even though both are vectors of tagged values
*/
bool
tests_has_font_opentype_and_variable_track_the_flags()
{
    D_FT_CHECK(has_font_opentype_features_v<opentype_font>);
    D_FT_CHECK(!has_font_variable_axes_v<opentype_font>);

    D_FT_CHECK(!has_font_opentype_features_v<variable_font_t>);
    D_FT_CHECK(has_font_variable_axes_v<variable_font_t>);

    D_FT_CHECK(has_font_opentype_features_v<full_font>);
    D_FT_CHECK(has_font_variable_axes_v<full_font>);

    D_FT_CHECK(!has_font_opentype_features_v<bare_font>);
    D_FT_CHECK(!has_font_variable_axes_v<bare_font>);

    // ff_gui_standard carries neither - the profile that stops short
    D_FT_CHECK(!has_font_opentype_features_v<gui_std_font>);
    D_FT_CHECK(!has_font_variable_axes_v<gui_std_font>);
    D_FT_CHECK(has_font_opentype_features_v<gui_rich_font>);
    D_FT_CHECK(has_font_variable_axes_v<gui_rich_font>);

    return true;
}

/*
tests_has_font_script_hint_requires_both_tags
  The second conjunction.  A type carrying only a script_tag - or only a
language_tag - does not satisfy it; the shaping selection needs both slots
to exist, even if one is empty at run time.
  Tests the following:
  - script_tag alone does NOT satisfy the trait
  - language_tag alone does NOT satisfy it
  - both together DO
  - font<ff_script_hint> supplies both
*/
bool
tests_has_font_script_hint_requires_both_tags()
{
    D_FT_CHECK(!has_font_script_hint_v<duck_script_only>);
    D_FT_CHECK((djinterp::is_detected<font_script_tag_t,
                                      duck_script_only>::value));
    D_FT_CHECK(!(djinterp::is_detected<font_language_tag_t,
                                       duck_script_only>::value));

    D_FT_CHECK(!has_font_script_hint_v<duck_language_only>);
    D_FT_CHECK((djinterp::is_detected<font_language_tag_t,
                                      duck_language_only>::value));

    D_FT_CHECK(has_font_script_hint_v<duck_script_paired>);

    D_FT_CHECK(has_font_script_hint_v<script_font>);
    D_FT_CHECK(has_font_script_hint_v<full_font>);
    D_FT_CHECK(!has_font_script_hint_v<bare_font>);
    D_FT_CHECK(!has_font_script_hint_v<gui_std_font>);

    return true;
}

/*
tests_has_font_backend_traits_track_the_flags
  Tests the following:
  - has_font_file_path, has_font_postscript_name, and has_font_native_handle
    all fire on ff_backend_handles - they share ONE mixin, so a font gets
    all three or none
  - none of them fires on a bare font
*/
bool
tests_has_font_backend_traits_track_the_flags()
{
    D_FT_CHECK(has_font_file_path_v<backend_font>);
    D_FT_CHECK(has_font_postscript_name_v<backend_font>);
    D_FT_CHECK(has_font_native_handle_v<backend_font>);

    D_FT_CHECK(!has_font_file_path_v<bare_font>);
    D_FT_CHECK(!has_font_postscript_name_v<bare_font>);
    D_FT_CHECK(!has_font_native_handle_v<bare_font>);

    // they arrive together on a font, always
    D_FT_CHECK(has_font_file_path_v<gui_std_font>);
    D_FT_CHECK(has_font_postscript_name_v<gui_std_font>);
    D_FT_CHECK(has_font_native_handle_v<gui_std_font>);

    D_FT_CHECK(!has_font_file_path_v<terminal_font>);

    return true;
}

/*
tests_has_font_backend_handles_is_a_disjunction
  The trait is an OR over the three handles - and because a font<> always
brings all three at once, the ONLY way to see that it really is an OR is with
a duck type carrying exactly one of them.  A file-path-only struct must
satisfy has_font_backend_handles while failing the other two individual
traits.
  Tests the following:
  - a struct with only a file_path satisfies the disjunction
  - it does NOT satisfy has_font_postscript_name or has_font_native_handle
  - a struct with only a native_handle also satisfies the disjunction
  - a struct with none of the three does not
*/
bool
tests_has_font_backend_handles_is_a_disjunction()
{
    // one handle out of three is enough
    D_FT_CHECK(has_font_backend_handles_v<duck_file_path_only>);
    D_FT_CHECK(has_font_file_path_v<duck_file_path_only>);
    D_FT_CHECK(!has_font_postscript_name_v<duck_file_path_only>);
    D_FT_CHECK(!has_font_native_handle_v<duck_file_path_only>);

    // a different single handle works just as well
    D_FT_CHECK(has_font_backend_handles_v<duck_native_handle_only>);
    D_FT_CHECK(has_font_native_handle_v<duck_native_handle_only>);
    D_FT_CHECK(!has_font_file_path_v<duck_native_handle_only>);

    // none of the three
    D_FT_CHECK(!has_font_backend_handles_v<duck_not_a_font>);
    D_FT_CHECK(!has_font_backend_handles_v<bare_font>);
    D_FT_CHECK(!has_font_backend_handles_v<duck_font>);

    // and the font that has the whole mixin
    D_FT_CHECK(has_font_backend_handles_v<backend_font>);
    D_FT_CHECK(has_font_backend_handles_v<full_font>);

    return true;
}

/*
tests_has_font_color_type_alias_is_independent_of_ff_color
  A sharp one.  `color_type` is declared in font<>'s BODY, not in a gated
mixin - so every font has it, including one with no color feature at all.
has_font_color_type_alias<font<ff_none>> is therefore TRUE while
has_font_color<font<ff_none>> is FALSE.  The two traits answer different
questions ("can you name the color type?" vs "is there a color to paint?")
and an adapter that conflated them would try to read a foreground that does
not exist.
  Tests the following:
  - a font with NO color feature still exposes color_type
  - has_font_color on that same font is false
  - the alias is present across the whole lattice
  - a duck type without the alias is rejected
*/
bool
tests_has_font_color_type_alias_is_independent_of_ff_color()
{
    // the alias is there even with no color feature
    D_FT_CHECK(has_font_color_type_alias_v<bare_font>);
    D_FT_CHECK(!has_font_color_v<bare_font>);
    D_FT_CHECK(!has_font_background_v<bare_font>);

    // and it names the right type
    D_FT_CHECK((std::is_same<font_color_type_t<bare_font>,
                             probe_color>::value));

    // present everywhere on the lattice
    D_FT_CHECK(has_font_color_type_alias_v<decorated_font_t>);
    D_FT_CHECK(has_font_color_type_alias_v<opentype_font>);
    D_FT_CHECK(has_font_color_type_alias_v<color_font>);
    D_FT_CHECK(has_font_color_type_alias_v<full_font>);

    // but never on a type that simply does not have it
    D_FT_CHECK(!has_font_color_type_alias_v<duck_font>);
    D_FT_CHECK(!has_font_color_type_alias_v<duck_terminal>);
    D_FT_CHECK(!has_font_color_type_alias_v<int>);

    // duck_terminal has a foreground but no alias - the two really are
    // separate questions
    D_FT_CHECK(has_font_color_v<duck_terminal>);
    D_FT_CHECK(!has_font_color_type_alias_v<duck_terminal>);

    return true;
}

/*
tests_traits_track_the_flags_across_the_lattice
  The seventeen structural traits, checked against the seventeen feature
bits, over every interesting point of the lattice.  This is the test that
would catch a mixin wired to the wrong flag, or a trait probing the wrong
member name - a whole class of copy-paste bug that no single-feature test
can see.
  Tests the following:
  - every has_font_* trait agrees with has_ff for the empty set, every
    singleton, every aggregate, every platform profile, and the full set
*/
bool
tests_traits_track_the_flags_across_the_lattice()
{
    static_assert(traits_track_the_flags<ff_none>(),
                  "the traits must track the flags");
    static_assert(traits_track_the_flags<ff_all>(),
                  "the traits must track the flags");

    // the empty set and the full set
    D_FT_CHECK(traits_track_the_flags<ff_none>());
    D_FT_CHECK(traits_track_the_flags<ff_all>());

    // every singleton
    D_FT_CHECK(traits_track_the_flags<ff_underline>());
    D_FT_CHECK(traits_track_the_flags<ff_strikethrough>());
    D_FT_CHECK(traits_track_the_flags<ff_overline>());
    D_FT_CHECK(traits_track_the_flags<ff_small_caps>());
    D_FT_CHECK(traits_track_the_flags<ff_all_caps>());
    D_FT_CHECK(traits_track_the_flags<ff_subscript>());
    D_FT_CHECK(traits_track_the_flags<ff_superscript>());
    D_FT_CHECK(traits_track_the_flags<ff_letter_spacing>());
    D_FT_CHECK(traits_track_the_flags<ff_line_height>());
    D_FT_CHECK(traits_track_the_flags<ff_stretch>());
    D_FT_CHECK(traits_track_the_flags<ff_spacing>());
    D_FT_CHECK(traits_track_the_flags<ff_color>());
    D_FT_CHECK(traits_track_the_flags<ff_background>());
    D_FT_CHECK(traits_track_the_flags<ff_opentype_features>());
    D_FT_CHECK(traits_track_the_flags<ff_variable_axes>());
    D_FT_CHECK(traits_track_the_flags<ff_script_hint>());
    D_FT_CHECK(traits_track_the_flags<ff_backend_handles>());

    // the aggregates
    D_FT_CHECK(traits_track_the_flags<ff_decorations>());
    D_FT_CHECK(traits_track_the_flags<ff_casing>());
    D_FT_CHECK(traits_track_the_flags<ff_metrics>());
    D_FT_CHECK(traits_track_the_flags<ff_axes>());

    // the platform profiles
    D_FT_CHECK(traits_track_the_flags<ff_terminal_basic>());
    D_FT_CHECK(traits_track_the_flags<ff_terminal_rich>());
    D_FT_CHECK(traits_track_the_flags<ff_gui_basic>());
    D_FT_CHECK(traits_track_the_flags<ff_gui_standard>());
    D_FT_CHECK(traits_track_the_flags<ff_gui_rich>());

    // and a sparse, arbitrary combination
    D_FT_CHECK(traits_track_the_flags<ff_overline | ff_subscript |
                                      ff_variable_axes>());

    return true;
}

/*
tests_trait_v_aliases_agree_with_value
  Every trait ships a _v variable template alongside it.  A _v that named
the wrong trait would be a silent, one-character bug, so each is checked
against its own ::value.
  Tests the following:
  - all thirty-odd _v aliases agree with the trait they abbreviate, on a
    font that has the feature and on one that does not
*/
bool
tests_trait_v_aliases_agree_with_value()
{
    // core
    D_FT_CHECK(has_font_family_v<full_font> ==
               has_font_family<full_font>::value);
    D_FT_CHECK(has_font_style_name_v<full_font> ==
               has_font_style_name<full_font>::value);
    D_FT_CHECK(has_font_size_v<full_font> == has_font_size<full_font>::value);
    D_FT_CHECK(has_font_size_unit_v<full_font> ==
               has_font_size_unit<full_font>::value);
    D_FT_CHECK(has_font_weight_v<full_font> ==
               has_font_weight<full_font>::value);
    D_FT_CHECK(has_font_weight_numeric_v<full_font> ==
               has_font_weight_numeric<full_font>::value);
    D_FT_CHECK(has_font_slant_v<full_font> ==
               has_font_slant<full_font>::value);

    // decorations + casing
    D_FT_CHECK(has_font_underline_v<full_font> ==
               has_font_underline<full_font>::value);
    D_FT_CHECK(has_font_strikethrough_v<full_font> ==
               has_font_strikethrough<full_font>::value);
    D_FT_CHECK(has_font_overline_v<full_font> ==
               has_font_overline<full_font>::value);
    D_FT_CHECK(has_font_decorations_v<full_font> ==
               has_font_decorations<full_font>::value);
    D_FT_CHECK(has_font_small_caps_v<full_font> ==
               has_font_small_caps<full_font>::value);
    D_FT_CHECK(has_font_all_caps_v<full_font> ==
               has_font_all_caps<full_font>::value);
    D_FT_CHECK(has_font_subscript_v<full_font> ==
               has_font_subscript<full_font>::value);
    D_FT_CHECK(has_font_superscript_v<full_font> ==
               has_font_superscript<full_font>::value);
    D_FT_CHECK(has_font_casing_v<full_font> ==
               has_font_casing<full_font>::value);

    // metrics + axes
    D_FT_CHECK(has_font_letter_spacing_v<full_font> ==
               has_font_letter_spacing<full_font>::value);
    D_FT_CHECK(has_font_line_height_v<full_font> ==
               has_font_line_height<full_font>::value);
    D_FT_CHECK(has_font_metrics_overrides_v<full_font> ==
               has_font_metrics_overrides<full_font>::value);
    D_FT_CHECK(has_font_stretch_v<full_font> ==
               has_font_stretch<full_font>::value);
    D_FT_CHECK(has_font_spacing_v<full_font> ==
               has_font_spacing<full_font>::value);

    // color
    D_FT_CHECK(has_font_color_v<full_font> ==
               has_font_color<full_font>::value);
    D_FT_CHECK(has_font_background_v<full_font> ==
               has_font_background<full_font>::value);
    D_FT_CHECK(has_font_color_type_alias_v<full_font> ==
               has_font_color_type_alias<full_font>::value);

    // opentype / variable / script / backend
    D_FT_CHECK(has_font_opentype_features_v<full_font> ==
               has_font_opentype_features<full_font>::value);
    D_FT_CHECK(has_font_variable_axes_v<full_font> ==
               has_font_variable_axes<full_font>::value);
    D_FT_CHECK(has_font_script_hint_v<full_font> ==
               has_font_script_hint<full_font>::value);
    D_FT_CHECK(has_font_file_path_v<full_font> ==
               has_font_file_path<full_font>::value);
    D_FT_CHECK(has_font_postscript_name_v<full_font> ==
               has_font_postscript_name<full_font>::value);
    D_FT_CHECK(has_font_native_handle_v<full_font> ==
               has_font_native_handle<full_font>::value);
    D_FT_CHECK(has_font_backend_handles_v<full_font> ==
               has_font_backend_handles<full_font>::value);

    // and the same on a font that has NONE of them, so a _v hard-wired to
    // `true` would be caught
    D_FT_CHECK(has_font_underline_v<bare_font> ==
               has_font_underline<bare_font>::value);
    D_FT_CHECK(has_font_casing_v<bare_font> ==
               has_font_casing<bare_font>::value);
    D_FT_CHECK(has_font_background_v<bare_font> ==
               has_font_background<bare_font>::value);
    D_FT_CHECK(has_font_backend_handles_v<bare_font> ==
               has_font_backend_handles<bare_font>::value);
    D_FT_CHECK(!has_font_underline_v<bare_font>);
    D_FT_CHECK(!has_font_backend_handles_v<bare_font>);

    return true;
}

/*
tests_traits_derive_from_integral_constant
  Every trait inherits from std::true_type / std::false_type (through
is_detected, conjunction, or disjunction), which is what lets them be used
as tag-dispatch types and not merely as ::value bags.
  Tests the following:
  - a satisfied trait is a std::true_type
  - an unsatisfied one is a std::false_type
  - ::value is a bool, and ::type round-trips
  - this holds for a plain detector trait, a conjunction, and a disjunction
    alike
*/
bool
tests_traits_derive_from_integral_constant()
{
    // a plain detector trait
    D_FT_CHECK((std::is_base_of<std::true_type,
                                has_font_underline<underline_font_t>>::value));
    D_FT_CHECK((std::is_base_of<std::false_type,
                                has_font_underline<bare_font>>::value));

    // a disjunction
    D_FT_CHECK((std::is_base_of<std::true_type,
                                has_font_decorations<overline_font_t>>::value));

    // a conjunction
    D_FT_CHECK((std::is_base_of<std::true_type,
                                has_font_background<background_font>>::value));
    D_FT_CHECK((std::is_base_of<std::false_type,
                                has_font_background<
                                    duck_background_only>>::value));

    // ::value is a bool and ::type round-trips
    D_FT_CHECK((std::is_same<decltype(has_font_color<color_font>::value),
                             const bool>::value));
    D_FT_CHECK((std::is_same<has_font_color<color_font>::value_type,
                             bool>::value));
    D_FT_CHECK(has_font_color<color_font>::type::value);

    return true;
}

NS_END  // testing
NS_END  // djinterp
